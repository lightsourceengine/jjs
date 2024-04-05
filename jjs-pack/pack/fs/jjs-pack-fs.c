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

#include <errno.h>
#include <stdlib.h>

#include "jjs-pack-lib.h"
#include "jjs-pack.h"

#include "fs.h"

#if JJS_PACK_FS
extern uint8_t jjs_pack_fs_snapshot[];
extern const uint32_t jjs_pack_fs_snapshot_len;

#define BOM_SIZE 3
static bool fs_utf8_has_bom (const uint8_t* buffer, uint32_t length);
static jjs_value_t throw_file_error (const char* message, const char* path, int err);

static JJS_HANDLER (jjs_pack_fs_read)
{
  JJS_HANDLER_HEADER ();
  JJS_ARG (path_value, 0, jjs_value_is_string);

  uint8_t* buffer_p;
  uint32_t buffer_size;
  int read_result;
  jjs_value_t result;

  JJS_READ_STRING (path, path_value, 256);

  read_result = fs_read ((const char*) path_p, &buffer_p, &buffer_size);

  if (read_result != 0)
  {
    result = throw_file_error ("Failed to read file.", (const char*) path_p, read_result);
  }
  else
  {
    result = jjs_arraybuffer (buffer_size);

    if (jjs_value_is_arraybuffer (result) && jjs_arraybuffer_write (result, 0, buffer_p, buffer_size) == 0)
    {
        jjs_value_free (result);
        result = throw_file_error ("Failed to write to arraybuffer.", (const char*) path_p, EIO);
    }

    fs_read_free (buffer_p);
  }

  JJS_READ_STRING_FREE (path);

  return result;
} /* jjs_pack_fs_read */

static JJS_HANDLER (jjs_pack_fs_read_utf8)
{
  JJS_HANDLER_HEADER ();
  JJS_ARG (path_value, 0, jjs_value_is_string);

  uint8_t* buffer_p;
  uint32_t buffer_size;
  int read_result;
  jjs_value_t result;

  JJS_READ_STRING (path, path_value, 256);
  read_result = fs_read ((const char*) path_p, &buffer_p, &buffer_size);

  if (read_result != 0)
  {
    result = throw_file_error ("Failed to read file.", (const char*) path_p, read_result);
  }
  else
  {
    uint8_t* start_p;
    uint32_t size;

    if (fs_utf8_has_bom(buffer_p, buffer_size))
    {
      start_p = buffer_p + BOM_SIZE;
      size = buffer_size - BOM_SIZE;
    }
    else
    {
      start_p = buffer_p;
      size = buffer_size;
    }

    if (jjs_validate_string(start_p, size, JJS_ENCODING_UTF8))
    {
      result = jjs_string (start_p, size, JJS_ENCODING_UTF8);
    }
    else
    {
      result = throw_file_error ("Failed to decode UTF-8 text.", (const char*) path_p, EILSEQ);
    }

    fs_read_free(buffer_p);
  }

  JJS_READ_STRING_FREE (path);

  return result;
} /* jjs_pack_fs_read_utf8 */

static JJS_HANDLER (jjs_pack_fs_read_json)
{
  JJS_HANDLER_HEADER ();
  JJS_ARG (path_value, 0, jjs_value_is_string);

  uint8_t* buffer_p;
  uint32_t buffer_size;
  int read_result;
  jjs_value_t result;

  JJS_READ_STRING (path, path_value, 256);
  read_result = fs_read ((const char*) path_p, &buffer_p, &buffer_size);

  if (read_result != 0)
  {
    result = throw_file_error ("Failed to read file.", (const char*) path_p, read_result);
  }
  else
  {
    uint8_t* start_p;
    uint32_t size;

    if (fs_utf8_has_bom (buffer_p, buffer_size))
    {
      start_p = buffer_p + BOM_SIZE;
      size = buffer_size - BOM_SIZE;
    }
    else
    {
      start_p = buffer_p;
      size = buffer_size;
    }

    result = jjs_json_parse (start_p, size);

    fs_read_free (buffer_p);
  }

  JJS_READ_STRING_FREE (path);

  return result;
} /* jjs_pack_fs_read_json */

static JJS_HANDLER (jjs_pack_fs_size)
{
  JJS_HANDLER_HEADER ();
  JJS_ARG (path_value, 0, jjs_value_is_string);

  uint32_t size = 0;
  int result;

  JJS_READ_STRING (path, path_value, 256);
  result = fs_get_size ((const char*) path_p, &size);
  JJS_READ_STRING_FREE (path);

  return jjs_number ((double) (result == 0 ? size : 0));
} /* jjs_pack_fs_size */

static JJS_HANDLER (jjs_pack_fs_copy)
{
  JJS_HANDLER_HEADER ();

  JJS_ARG (destination_value, 0, jjs_value_is_string);
  JJS_ARG (source_value, 1, jjs_value_is_string);

  int err;
  jjs_size_t written;

  JJS_READ_STRING (destination, destination_value, 256);
  JJS_READ_STRING (source, source_value, 256);

  err = fs_copy ((const char*) destination_p, (const char*) source_p, &written);

  JJS_READ_STRING_FREE (destination);
  JJS_READ_STRING_FREE (source);

  (void) err;

  return jjs_number ((double) written);
} /* jjs_pack_fs_copy */

static JJS_HANDLER (jjs_pack_fs_write)
{
  JJS_HANDLER_HEADER ();
  JJS_ARG (destination_value, 0, jjs_value_is_string);

  int result;
  jjs_size_t written;
  jjs_value_t input_value = args_cnt > 1 ? args_p[1] : jjs_undefined ();
  jjs_value_t ret;
  uint8_t* buffer_p;
  jjs_size_t len;

  JJS_READ_STRING (destination, destination_value, 256);

  if (jjs_value_is_arraybuffer (input_value))
  {
    buffer_p = jjs_arraybuffer_data (input_value);
    len = jjs_arraybuffer_size (input_value);
  }
  else if (jjs_value_is_typedarray (input_value))
  {
    jjs_size_t offset;
    jjs_value_t buffer_value = jjs_typedarray_buffer (input_value, &offset, &len);

    if (jjs_value_is_exception(buffer_value))
    {
      ret = buffer_value;
      goto done;
    }

    buffer_p = jjs_arraybuffer_data (buffer_value);

    if (buffer_p != NULL)
    {
      buffer_p += offset;
    }

    jjs_value_free (buffer_value);
  }
  else
  {
    ret = jjs_throw_sz (JJS_ERROR_TYPE, "input arg must be an ArrayBuffer or TypedArray.");
    goto done;
  }

  result = fs_write ((const char*) destination_p, buffer_p, len, &written);

  if (result != 0)
  {
    ret = throw_file_error ("Failed to write file.", (const char*) destination_p, result);
  }
  else
  {
    ret = jjs_number ((double) written);
  }

done:
  JJS_READ_STRING_FREE (destination);

  return ret;
} /* jjs_pack_fs_write */

static JJS_HANDLER (jjs_pack_fs_write_string)
{
  JJS_HANDLER_HEADER ();
  JJS_ARG (destination_value, 0, jjs_value_is_string);
  JJS_ARG (string_value, 1, jjs_value_is_string);

  int result;
  jjs_size_t written;
  jjs_value_t ret;

  JJS_READ_STRING (destination, destination_value, 256);
  JJS_READ_STRING (string, string_value, 4096);

  result = fs_write ((const char*) destination_p, string_p, string_size, &written);

  if (result != 0)
  {
    ret = throw_file_error ("Failed to write file.", (const char*) destination_p, result);
  }
  else
  {
    ret = jjs_number ((double) written);
  }

  JJS_READ_STRING_FREE (string);
  JJS_READ_STRING_FREE (destination);

  return ret;
} /* jjs_pack_fs_write_string */

static JJS_HANDLER (jjs_pack_fs_remove)
{
  JJS_HANDLER_HEADER ();
  JJS_ARG (path_value, 0, jjs_value_is_string);

  JJS_READ_STRING (path, path_value, 256);
  int result = fs_remove ((const char*) path_p);
  JJS_READ_STRING_FREE (path);

  if (result != 0)
  {
    return throw_file_error ("Failed to remove file.", (const char*) path_p, result);
  }

  return jjs_undefined ();
} /* jjs_pack_fs_remove */

static bool
fs_utf8_has_bom (const uint8_t* buffer, uint32_t length)
{
  return length >= BOM_SIZE && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF;
} /* fs_utf8_has_bom */

static jjs_value_t throw_file_error (const char* message, const char* path, int err)
{
  jjs_value_t ex = jjs_throw_sz (JJS_ERROR_COMMON, message);
  jjs_value_t error = jjs_exception_value (ex, false);
  jjs_value_t code = jjs_string_sz (fs_errno_to_string (err));
  jjs_value_t errno_value = jjs_number ((double) err);
  jjs_value_t errno_message = jjs_string_sz(fs_errno_message(err));
  jjs_value_t path_value = jjs_string_sz (path);

  jjs_value_free (jjs_object_set_sz (error, "code", code));
  jjs_value_free (jjs_object_set_sz (error, "errno", errno_value));
  jjs_value_free (jjs_object_set_sz (error, "errnoMessage", errno_message));
  jjs_value_free (jjs_object_set_sz (error, "path", path_value));

  jjs_value_free (error);
  jjs_value_free (code);
  jjs_value_free (errno_value);
  jjs_value_free (errno_message);
  jjs_value_free (path_value);

  return ex;
} /* throw_file_error */

static JJS_HANDLER (jjs_pack_fs_vmod_callback)
{
  JJS_HANDLER_HEADER ();
  jjs_value_t bindings = jjs_bindings ();

  jjs_bindings_function (bindings, "read", &jjs_pack_fs_read);
  jjs_bindings_function (bindings, "readUTF8", &jjs_pack_fs_read_utf8);
  jjs_bindings_function (bindings, "readJSON", &jjs_pack_fs_read_json);
  jjs_bindings_function (bindings, "size", &jjs_pack_fs_size);
  jjs_bindings_function (bindings, "copy", &jjs_pack_fs_copy);
  jjs_bindings_function (bindings, "writeBuffer", &jjs_pack_fs_write);
  jjs_bindings_function (bindings, "writeString", &jjs_pack_fs_write_string);
  jjs_bindings_function (bindings, "remove", &jjs_pack_fs_remove);

  return jjs_pack_lib_read_exports (jjs_pack_fs_snapshot,
                                    jjs_pack_fs_snapshot_len,
                                    bindings,
                                    JJS_MOVE,
                                    JJS_PACK_LIB_EXPORTS_FORMAT_VMOD);
}

#endif /* JJS_PACK_FS */

jjs_value_t
jjs_pack_fs_init (void)
{
#if JJS_PACK_FS
  return jjs_pack_lib_main_vmod ("jjs:fs", &jjs_pack_fs_vmod_callback);
#else /* !JJS_PACK_FS */
  return jjs_throw_sz (JJS_ERROR_COMMON, "fs pack is not enabled");
#endif /* JJS_PACK_FS */
} /* jjs_pack_fs_init */
