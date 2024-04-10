/* Copyright Light Source Software, LLC and other contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jjs-stream.h"

#include "jcontext.h"
#include "jjs-util.h"
#include "ecma-helpers.h"
#include "annex.h"
#include "jjs-annex.h"

static void jjs_wstream_prototype_finalizer (void *native_p, const struct jjs_object_native_info_t *info_p);

static jjs_object_native_info_t jjs_wstream_class_info = {
  .free_cb = jjs_wstream_prototype_finalizer,
};

static void jjs_wstream_prototype_finalizer (void *native_p, const struct jjs_object_native_info_t *info_p)
{
  JJS_UNUSED (info_p);
  jjs_heap_free (native_p, (jjs_size_t) sizeof(jjs_wstream_t));
} /* jjs_wstream_prototype_finalizer */

void
jjs_wstream_write_string (const jjs_wstream_t* wstream_p, jjs_value_t value, jjs_value_t value_o)
{
  JJS_ASSERT (jjs_value_is_string (value));

  ecma_string_t *string_p = ecma_get_string_from_value (value);

  ECMA_STRING_TO_UTF8_STRING (string_p, string_bytes_p, string_bytes_len);

  // TODO: double check encoding. support ascii?
  if (ecma_string_get_length (string_p) == string_bytes_len || wstream_p->encoding == JJS_ENCODING_CESU8)
  {
    wstream_p->write (wstream_p, string_bytes_p, string_bytes_len);
  }
  else if (wstream_p->encoding == JJS_ENCODING_UTF8)
  {
    const lit_utf8_byte_t *cesu8_cursor_p = string_bytes_p;
    const lit_utf8_byte_t *cesu8_end_p = string_bytes_p + string_bytes_len;
    lit_utf8_byte_t utf8_buf_p[4];
    lit_code_point_t cp;
    lit_utf8_size_t read_size;
    lit_utf8_size_t encoded_size;

    while (cesu8_cursor_p < cesu8_end_p)
    {
      read_size = lit_read_code_point_from_cesu8 (cesu8_cursor_p, cesu8_end_p, &cp);
      encoded_size = (cp >= LIT_UTF16_FIRST_SURROGATE_CODE_POINT) ? 4 : read_size;

      if (cp >= LIT_UTF16_FIRST_SURROGATE_CODE_POINT)
      {
        wstream_p->write (wstream_p, utf8_buf_p, lit_code_point_to_utf8 (cp, utf8_buf_p));
      }
      else
      {
        wstream_p->write (wstream_p, cesu8_cursor_p, encoded_size);
      }

      cesu8_cursor_p += read_size;
    }

    JJS_ASSERT (cesu8_cursor_p <= cesu8_end_p);
  }
  else
  {
    JJS_ASSERT (wstream_p->encoding == JJS_ENCODING_UTF8 || wstream_p->encoding == JJS_ENCODING_CESU8);
    goto done;
  }

done:
  ECMA_FINALIZE_UTF8_STRING (string_bytes_p, string_bytes_len);

  JJS_DISOWN (value, value_o);
} /* jjs_wstream_write_string */

static JJS_HANDLER (jjs_wstream_prototype_write)
{
  if (args_count > 0 && jjs_value_is_string (args_p[0]))
  {
    jjs_wstream_t *wstream_p = jjs_object_get_native_ptr (call_info_p->this_value, &jjs_wstream_class_info);

    if (wstream_p)
    {
      jjs_wstream_write_string (wstream_p, args_p[0], JJS_KEEP);
    }
  }

  return jjs_undefined();
} /* jjs_wstream_prototype_write */

static void
wstream_io_write (const jjs_wstream_t *self_p, const uint8_t *data_p, uint32_t data_size)
{
  JJS_CONTEXT (platform_api).io_write (self_p->state_p, data_p, data_size, self_p->encoding);
} /* wstream_io_write */

static void
wstream_stringbuilder_write (const jjs_wstream_t *self_p, const uint8_t *buffer_p, jjs_size_t size)
{
  ecma_stringbuilder_t *builder_p = self_p->state_p;

  /* user of this stream is using CESU8, so we can just copy to the builder */
  ecma_stringbuilder_append_raw (builder_p, buffer_p, size);
} /* wstream_stringbuilder_write */

static void
wstream_memory_write (const jjs_wstream_t *self_p, const uint8_t *buffer_p, jjs_size_t size)
{
  jjs_wstream_buffer_t *target_p = self_p->state_p;

  if (target_p->buffer_index < target_p->buffer_size)
  {
    if (target_p->buffer_index + size < target_p->buffer_size)
    {
      memcpy (target_p->buffer + target_p->buffer_index, buffer_p, size);
      target_p->buffer_index += size;
    }
    else
    {
      jjs_size_t write_size = target_p->buffer_size - target_p->buffer_index;
      memcpy (target_p->buffer + target_p->buffer_index, buffer_p, write_size);
      target_p->buffer_index += write_size;
    }
  }
} /* wstream_memory_write */

bool
jjs_wstream_new (jjs_std_stream_id_t id, jjs_value_t* out)
{
  jjs_wstream_t wstream;

  if (!jjs_wstream_from_id (id, &wstream))
  {
    return false;
  }

  jjs_value_t wstream_value = jjs_object();
  jjs_wstream_t* wstream_p = jjs_heap_alloc (sizeof (jjs_wstream_t));

  if (wstream_p == NULL)
  {
    return false;
  }

  memcpy (wstream_p, &wstream, sizeof (jjs_wstream_t));
  jjs_object_set_native_ptr (wstream_value, &jjs_wstream_class_info, wstream_p);
  annex_util_define_function (ecma_get_object_from_value (wstream_value), LIT_MAGIC_STRING_WRITE, jjs_wstream_prototype_write);

  *out = wstream_value;

  return true;
} /* jjs_wstream_new */

bool
jjs_wstream_from_buffer (jjs_wstream_buffer_t* buffer_p, jjs_encoding_t encoding, jjs_wstream_t* out)
{
  out->write = wstream_memory_write;
  out->encoding = encoding;
  out->state_p = buffer_p;

  return true;
} /* jjs_wstream_from_buffer */

bool jjs_wstream_from_id (jjs_std_stream_id_t id, jjs_wstream_t* out)
{
  if (JJS_CONTEXT (platform_api).io_write)
  {
    switch (id)
    {
      case JJS_STDOUT:
      {
        if (JJS_CONTEXT (platform_api).io_stdout)
        {
          out->write = wstream_io_write;
          out->encoding = JJS_CONTEXT (platform_api).io_stdout_encoding;
          out->state_p = JJS_CONTEXT (platform_api).io_stdout;
          return true;
        }
      }
      case JJS_STDERR:
      {
        if (JJS_CONTEXT (platform_api).io_stderr)
        {
          out->write = wstream_io_write;
          out->encoding = JJS_CONTEXT (platform_api).io_stderr_encoding;
          out->state_p = JJS_CONTEXT (platform_api).io_stderr;
        }
        return true;
      }
      default:
        break;
    }
  }

  return false;
} /* jjs_wstream_from_id */

bool
jjs_wstream_from_stringbuilder (struct ecma_stringbuilder_t* builder, jjs_wstream_t* out)
{
  out->write = wstream_stringbuilder_write;
  out->encoding = JJS_ENCODING_CESU8;
  out->state_p = builder;

  return true;
} /* jjs_wstream_from_stringbuilder */
