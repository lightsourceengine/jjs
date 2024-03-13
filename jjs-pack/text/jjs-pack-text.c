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

#include <stdlib.h>

#include "jjs-pack-lib.h"
#include "jjs-pack.h"

#if JJS_PACK_TEXT

#define BOM_SIZE    3
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

extern uint8_t jjs_pack_text_api_snapshot[];
extern const uint32_t jjs_pack_text_api_snapshot_len;

static bool jjs_pack_text_arraybuffer (jjs_value_t buffer_like, uint8_t** buffer_p, jjs_size_t* buffer_size_p);
static bool utf8_has_bom (const uint8_t* buffer, uint32_t length);
static uint32_t utf8_length_unsafe (const uint8_t* buffer, uint32_t size);
static uint32_t utf8_decode (uint32_t* state, uint32_t* codep, uint32_t byte);
static uint32_t utf8_size_with_replacements (const uint8_t* buffer, uint32_t size);
static void utf8_copy_with_replacements (const uint8_t* data, uint32_t data_size, uint8_t* result);

static JJS_HANDLER (jjs_pack_text_encode)
{
  JJS_HANDLER_HEADER ();
  jjs_value_t value = args_cnt > 0 ? args_p[0] : jjs_undefined ();
  jjs_size_t size = jjs_string_size (value, JJS_ENCODING_UTF8);
  jjs_value_t result = jjs_typedarray (JJS_TYPEDARRAY_UINT8, size);

  if (size == 0)
  {
    return result;
  }

  uint8_t* buffer_p;
  jjs_size_t buffer_size;

  if (jjs_pack_text_arraybuffer (result, &buffer_p, &buffer_size))
  {
    jjs_size_t written = jjs_string_to_buffer (value, JJS_ENCODING_UTF8, buffer_p, buffer_size);

    if (written != buffer_size)
    {
      jjs_value_free (result);
      result = jjs_typedarray (JJS_TYPEDARRAY_UINT8, 0);
    }
  }

  return result;
} /* jjs_pack_text_encode */

static JJS_HANDLER (jjs_pack_text_encode_into)
{
  JJS_HANDLER_HEADER ();
  jjs_value_t value = args_cnt > 0 ? args_p[0] : jjs_undefined ();
  jjs_value_t target = args_cnt > 1 ? args_p[1] : jjs_undefined ();
  jjs_size_t size = jjs_string_size (value, JJS_ENCODING_UTF8);

  if (!jjs_value_is_typedarray (target) || jjs_typedarray_type (target) != JJS_TYPEDARRAY_UINT8)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "encodeInto(): buffer argument is not a Uint8Array");
  }

  uint8_t* buffer_p;
  jjs_size_t buffer_size;
  jjs_size_t written;
  jjs_size_t read;

  if (size > 0 && jjs_pack_text_arraybuffer (target, &buffer_p, &buffer_size))
  {
    written = jjs_string_to_buffer (value, JJS_ENCODING_UTF8, buffer_p, buffer_size);
    read = utf8_length_unsafe (buffer_p, written);
  }
  else
  {
    written = 0;
    read = 0;
  }

  jjs_value_t result = jjs_object ();
  jjs_value_t read_value = jjs_number ((double) read);
  jjs_value_t written_value = jjs_number ((double) written);

  jjs_value_free (jjs_object_set_sz (result, "read", read_value));
  jjs_value_free (jjs_object_set_sz (result, "written", written_value));
  jjs_value_free (read_value);
  jjs_value_free (written_value);

  return result;
} /* jjs_pack_text_encode_into */

static JJS_HANDLER (jjs_pack_text_decode_utf8)
{
  JJS_HANDLER_HEADER ();
  jjs_value_t buffer = args_cnt > 0 ? args_p[0] : jjs_undefined ();
  bool ignore_bom = args_cnt > 1 ? jjs_value_is_true (args_p[1]) : jjs_undefined ();
  bool fatal = args_cnt > 2 ? jjs_value_is_true (args_p[2]) : jjs_undefined ();
  bool is_buffer_like = jjs_value_is_typedarray (buffer) || jjs_value_is_shared_arraybuffer (buffer)
                        || jjs_value_is_arraybuffer (buffer) || jjs_value_is_dataview (buffer);
  if (!is_buffer_like)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "decode(): buffer argument is not a buffer-like object");
  }

  uint8_t* buffer_p;
  jjs_size_t buffer_size;

  if (!jjs_pack_text_arraybuffer (buffer, &buffer_p, &buffer_size))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "decode(): failed to extract native buffer");
  }

  if (ignore_bom && utf8_has_bom (buffer_p, buffer_size))
  {
    buffer_p += BOM_SIZE;
    buffer_size -= BOM_SIZE;
  }

  if (buffer_size == 0)
  {
    return jjs_string_sz ("");
  }

  uint32_t actual_size = utf8_size_with_replacements (buffer_p, buffer_size);

  if (buffer_size != actual_size && fatal)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "decode(): invalid UTF8 sequence");
  }

  uint8_t* buffer_copy_p = malloc (actual_size);

  if (buffer_copy_p == NULL)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "decode(): failed to copy native buffer");
  }

  utf8_copy_with_replacements (buffer_p, buffer_size, buffer_copy_p);

  jjs_value_t result = jjs_string (buffer_copy_p, actual_size, JJS_ENCODING_UTF8);

  free (buffer_copy_p);

  return result;
} /* jjs_pack_text_decode_utf8 */

static bool
jjs_pack_text_arraybuffer (jjs_value_t buffer_like, uint8_t** buffer_p, jjs_size_t* buffer_size_p)
{
  if (jjs_value_is_typedarray (buffer_like))
  {
    jjs_length_t offset = 0;
    jjs_length_t length = 0;

    jjs_value_t array_buffer = jjs_typedarray_buffer (buffer_like, &offset, &length);

    if (jjs_value_is_exception (array_buffer))
    {
      jjs_value_free (array_buffer);
      return false;
    }

    *buffer_p = jjs_arraybuffer_data (array_buffer) + offset;
    *buffer_size_p = length;
    jjs_value_free (array_buffer);
  }
  else if (jjs_value_is_arraybuffer (buffer_like))
  {
    *buffer_p = jjs_arraybuffer_data (buffer_like);
    *buffer_size_p = jjs_arraybuffer_size (buffer_like);
  }
  else if (jjs_value_is_dataview (buffer_like))
  {
    jjs_length_t offset = 0;
    jjs_length_t length = 0;

    jjs_value_t array_buffer = jjs_dataview_buffer (buffer_like, &offset, &length);

    if (jjs_value_is_exception (array_buffer))
    {
      jjs_value_free (array_buffer);
      return false;
    }

    *buffer_p = jjs_arraybuffer_data (array_buffer) + offset;
    *buffer_size_p = length;
    jjs_value_free (array_buffer);
  }
  else
  {
    return false;
  }

  return true;
} /* jjs_pack_text_arraybuffer */

static uint32_t
utf8_length_unsafe (const uint8_t* buffer, uint32_t size)
{
  uint32_t result = 0;

  // assumes that the buffer is valid UTF-8
  for (uint32_t i = 0; i < size; ++i)
  {
    if ((buffer[i] & 0xC0) != 0x80)
    {
      ++result;
    }
  }

  return result;
} /* utf8_length_unsafe */

static uint32_t
utf8_size_with_replacements (const uint8_t* buffer, uint32_t size)
{
  uint32_t current = 0;
  uint32_t codepoint = 0;
  uint32_t actual_size = size;

  for (uint32_t i = 0; i < size; i++)
  {
    switch (utf8_decode (&current, &codepoint, buffer[i]))
    {
      case UTF8_REJECT:
        actual_size += 2;
        current = UTF8_ACCEPT;
        break;
      default:
        break;
    }
  }

  return actual_size;
} /* utf8_size_with_replacements */

static void
utf8_copy_with_replacements (const uint8_t* data, uint32_t data_size, uint8_t* result)
{
  uint32_t current = 0;
  uint32_t codepoint = 0;
  uint32_t j = 0;

  for (uint32_t i = 0; i < data_size; i++)
  {
    switch (utf8_decode (&current, &codepoint, data[i]))
    {
      case UTF8_ACCEPT:
        for (; j <= i; j++)
        {
          *result++ = data[j];
        }
        j = i + 1;
        break;
      case UTF8_REJECT:
        *result++ = 0xEF;
        *result++ = 0xBF;
        *result++ = 0xBD;
        current = UTF8_ACCEPT;
        j = i + 1;
        break;
      default:
        break;
    }
  }
} /* utf8_copy_with_replacements */

static bool
utf8_has_bom (const uint8_t* buffer, uint32_t length)
{
  return length >= BOM_SIZE && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF;
} /* utf8_has_bom */

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

static const uint8_t utf8d[] = {
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 00..1f
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 20..3f
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 40..5f
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 60..7f
  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
  9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9, // 80..9f
  7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
  7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7, // a0..bf
  8,   8,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
  2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2, // c0..df
  0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3, // e0..ef
  0xb, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, // f0..ff
  0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, 0x6, 0x1, 0x1, 0x1, 0x1, // s0..s0
  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
  1,   0,   1,   1,   1,   1,   1,   0,   1,   0,   1,   1,   1,   1,   1,   1, // s1..s2
  1,   2,   1,   1,   1,   1,   1,   2,   1,   2,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,   1,   1, // s3..s4
  1,   2,   1,   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   3,   1,   3,   1,   1,   1,   1,   1,   1, // s5..s6
  1,   3,   1,   1,   1,   1,   1,   3,   1,   3,   1,   1,   1,   1,   1,   1,
  1,   3,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1, // s7..s8
};

static uint32_t
utf8_decode (uint32_t* state, uint32_t* codep, uint32_t byte)
{
  uint32_t type = utf8d[byte];

  *codep = (*state > UTF8_REJECT) ? (byte & 0x3fu) | (*codep << 6u) : (0xffu >> type) & (byte);

  *state = utf8d[256 + *state * 16 + type];

  return *state;
} /* utf8_decode */

#endif /* JJS_PACK_TEXT */

jjs_value_t
jjs_pack_text_init (void)
{
#if JJS_PACK_TEXT
  jjs_value_t bindings = jjs_bindings ();

  jjs_bindings_function (bindings, "encode", jjs_pack_text_encode);
  jjs_bindings_function (bindings, "encodeInto", jjs_pack_text_encode_into);
  jjs_bindings_function (bindings, "decodeUTF8", jjs_pack_text_decode_utf8);

  return jjs_pack_lib_main (jjs_pack_text_api_snapshot, jjs_pack_text_api_snapshot_len, bindings, true);
#else /* !JJS_PACK_TEXT */
  return jjs_throw_sz (JJS_ERROR_COMMON, "text pack is not enabled");
#endif /* JJS_PACK_TEXT */
} /* jjs_pack_text_init */
