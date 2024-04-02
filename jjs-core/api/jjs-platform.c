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

#include "jjs-annex.h"
#include "jjs-core.h"

#include "jcontext.h"
#include "jjs-platform.h"
#include "jjs-compiler.h"

static const char* const jjs_os_identifier_p =
#ifndef JJS_PLATFORM_OS
#if defined (JJS_OS_IS_WINDOWS)
"win32"
#elif defined (JJS_OS_IS_AIX)
"aix"
#elif defined (JJS_OS_IS_LINUX)
"linux"
#elif defined (JJS_OS_IS_MACOS)
"darwin"
#else
"unknown"
#endif
#endif /* JJS_PLATFORM_OS */
;

static const char* const jjs_arch_identifier_p =
#ifndef JJS_PLATFORM_ARCH
#if defined(JJS_ARCH_IS_X32)
"ia32"
#elif defined(JJS_ARCH_IS_ARM)
"arm"
#elif defined(JJS_ARCH_IS_X64)
"arm64"
#elif defined(JJS_ARCH_IS_X64)
"x64"
#else
"unknown"
#endif
#endif /* JJS_PLATFORM_ARCH */
;

static bool jjsp_set_string (const char* value_p, char* dest_p, uint32_t dest_len);

// TODO: should this be exposed publicly?
const jjs_platform_t*
jjs_platform (void)
{
  jjs_assert_api_enabled ();
  return &JJS_CONTEXT (platform_api);
} /* jjs_platform */

/**
 * Gets the current working directory.
 *
 * The platform provides the function to get the current working directory. If the function is not
 * installed or available, this function will throw an exception. If the platform function fails,
 * an exception will also be thrown.
 *
 * @return current working directory path string; otherwise, an exception is thrown. the return value
 * must be cleaned up with jjs_value_free
 */
jjs_value_t
jjs_platform_cwd (void)
{
  jjs_assert_api_enabled();
  jjs_platform_cwd_fn_t cwd = JJS_CONTEXT (platform_api).cwd;

  if (cwd == NULL)
  {
    return jjs_throw_sz (JJS_ERROR_COMMON, "platform cwd api not installed");
  }

  jjs_platform_buffer_t buffer;

  if (cwd (&buffer) == JJS_PLATFORM_STATUS_OK)
  {
    ecma_value_t result = jjsp_buffer_to_string_value (&buffer, true);

    if (jjs_value_is_string (result))
    {
      return result;
    }

    ecma_free_value(result);
  }

  return jjs_throw_sz (JJS_ERROR_COMMON, "platform failed to get cwd");
} /* jjs_platform_cwd */

/**
 * Calls the platform realpath on the given path. All symlinks are removed
 * and the returned path is absolute.
 *
 * If the path does not exist, this function will return an exception.
 *
 * If the platform does not have realpath installed, this function will return
 * an exception.
 *
 * @param path string path to transform
 * @param path_o path reference ownership
 * @return resolved path string; otherwise, an exception. the returned value must
 * be cleaned up with jjs_value_free.
 */
jjs_value_t
jjs_platform_realpath (jjs_value_t path, jjs_value_ownership_t path_o)
{
  jjs_assert_api_enabled();
  jjs_platform_path_realpath_fn_t realpath_fn = JJS_CONTEXT (platform_api).path_realpath;

  if (realpath_fn == NULL)
  {
    if (path_o == JJS_MOVE)
    {
      jjs_value_free (path);
    }

    return jjs_throw_sz (JJS_ERROR_COMMON, "platform api 'path_realpath' not installed");
  }

  if (!jjs_value_is_string (path))
  {
    if (path_o == JJS_MOVE)
    {
      jjs_value_free (path);
    }

    return jjs_throw_sz (JJS_ERROR_TYPE, "expected path to be a string");
  }

  ECMA_STRING_TO_UTF8_STRING (ecma_get_string_from_value(path), path_bytes_p, path_bytes_len);

  if (path_o == JJS_MOVE)
  {
    jjs_value_free (path);
  }

  jjs_platform_buffer_t buffer;
  jjs_platform_status_t status = realpath_fn (path_bytes_p, path_bytes_len, &buffer);
  jjs_value_t result;

  if (status == JJS_PLATFORM_STATUS_OK)
  {
    result = jjsp_buffer_to_string_value (&buffer, true);
  }
  else
  {
    result = jjs_throw_sz (JJS_ERROR_COMMON, "failed to get realpath from path");
  }

  ECMA_FINALIZE_UTF8_STRING (path_bytes_p, path_bytes_len);

  return result;
} /* jjs_platform_realpath */

/**
 * Read the contents of a file into a string or array buffer using the platform
 * fs read api. The function is used internally to load source files, snapshots and
 * json files. It is not intended to be a general purpose file read.
 *
 * If encoding is JJS_ENCODING_UTF8 or JJS_ENCODING_CESU8, the file is read as binary
 * and decoded as a string with the given encoding. If successful, a string value
 * is returned.
 *
 * If encoding is JJS_ENCODING_NONE, the file is read as binary and returned as an
 * array buffer.
 *
 * @param path string path to the file.
 * @param path_o path reference ownership
 * @param opts options. if NULL, JJS_ENCODING_NONE is used.
 * @return string or array buffer; otherwise, an exception is returned. the returned value
 * must be cleaned up with jjs_value_free
 */
jjs_value_t
jjs_platform_read_file (jjs_value_t path, jjs_value_ownership_t path_o, const jjs_platform_read_file_options_t* opts)
{
  jjs_assert_api_enabled();
  jjs_value_t result = jjsp_read_file (path, opts ? opts->encoding : JJS_ENCODING_NONE);

  if (path_o == JJS_MOVE)
  {
    jjs_value_free (path);
  }

  return result;
} /* jjs_platform_read_file */

/**
 * Helper function to set platform arch string. Should only be used at context initialization time.
 *
 * @param platform_p platform object
 * @param value_p string value. strlen(value_p) should be > 0 && < 16
 * @return true if string successfully set, false on error
 */
bool
jjs_platform_set_arch_sz (jjs_platform_t* platform_p, const char* value_p)
{
  return jjsp_set_string (value_p, &platform_p->arch_sz[0], (uint32_t) sizeof (platform_p->arch_sz));
} /* jjs_platform_set_arch_sz */

/**
 * Helper function to set platform os string. Should only be used at context initialization time.
 *
 * @param platform_p platform object
 * @param value_p string value. strlen(value_p) should be > 0 && < 16
 * @return true if string successfully set, false on error
 */
bool
jjs_platform_set_os_sz (jjs_platform_t* platform_p, const char* value_p)
{
  return jjsp_set_string (value_p, &platform_p->os_sz[0], (uint32_t) sizeof (platform_p->os_sz));
} /* jjs_platform_set_os_sz */

jjs_platform_t
jjsp_defaults (void)
{
  jjs_platform_t platform;

  jjs_platform_set_os_sz (&platform, jjs_os_identifier_p);
  jjs_platform_set_arch_sz (&platform, jjs_arch_identifier_p);

  platform.cwd = jjsp_cwd;
  platform.fatal = jjsp_fatal;

  platform.io_log = jjsp_io_log;

  platform.time_local_tza = jjsp_time_local_tza;
  platform.time_now_ms = jjsp_time_now_ms;
  platform.time_sleep = jjsp_time_sleep;
  platform.time_hrtime = jjsp_time_hrtime;

  platform.path_normalize = jjsp_path_normalize;
  platform.path_realpath = jjsp_path_realpath;

  platform.fs_read_file = jjsp_fs_read_file;

  return platform;
} /* jjsp_defaults */

jjs_value_t
jjsp_read_file_buffer (jjs_value_t path, jjs_platform_buffer_t* buffer_p)
{
  jjs_platform_fs_read_file_fn_t read_file = JJS_CONTEXT (platform_api).fs_read_file;

  if (!read_file)
  {
    return jjs_throw_sz (JJS_ERROR_COMMON, "platform api 'fs_read_file' not installed");
  }

  if (!ecma_is_value_string (path))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "expected path to be a string");
  }

  ecma_string_t* path_p = ecma_get_string_from_value (path);
  ECMA_STRING_TO_UTF8_STRING (path_p, path_bytes_p, path_len);
  jjs_platform_status_t status = read_file (path_bytes_p, path_len, buffer_p);

  ECMA_FINALIZE_UTF8_STRING (path_bytes_p, path_len);

  if (status != JJS_PLATFORM_STATUS_OK)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Failed to read source file");
  }

  return ECMA_VALUE_UNDEFINED;
}

jjs_value_t
jjsp_read_file (jjs_value_t path, jjs_encoding_t encoding)
{
  jjs_platform_buffer_t buffer;
  jjs_value_t result = jjsp_read_file_buffer (path, &buffer);

  if (jjs_value_is_exception (result))
  {
    return result;
  }

  jjs_value_free (result);

  if (encoding == JJS_ENCODING_NONE)
  {
    result = jjs_arraybuffer (buffer.length);

    if (!jjs_value_is_exception (result))
    {
      uint8_t* buffer_p = jjs_arraybuffer_data (result);
      JJS_ASSERT (buffer_p != NULL);
      memcpy (buffer_p, buffer.data_p, buffer.length);
    }
  }
  else if (encoding == JJS_ENCODING_UTF8 || encoding == JJS_ENCODING_CESU8)
  {
    if (!jjs_validate_string (buffer.data_p, buffer.length, encoding))
    {
      result = jjs_throw_sz (JJS_ERROR_COMMON, "file contents cannot be decoded");
    }
    else
    {
      result = jjs_string (buffer.data_p, buffer.length, encoding);
    }
  }
  else
  {
    result = jjs_throw_sz (JJS_ERROR_TYPE, "unsupported read file encoding");
  }

  buffer.free (&buffer);

  return result;
}

static bool
jjsp_set_string (const char* value_p, char* dest_p, uint32_t dest_len)
{
  // TODO: context check
  size_t len = (uint32_t) (value_p ? strlen (value_p) : 0);

  if (len > 0 && len < dest_len - 1)
  {
    memcpy (dest_p, value_p, len + 1);
    return true;
  }

  return false;
} /* jjsp_set_string */

void
jjsp_buffer_free (jjs_platform_buffer_t* buffer_p)
{
  if (buffer_p)
  {
    free (buffer_p->data_p);
    memset (buffer_p, 0, sizeof (jjs_platform_buffer_t));
  }
} /* jjsp_buffer_free */

char*
jjsp_strndup (const char* str_p, uint32_t length, bool is_null_terminated)
{
  if (str_p == NULL || length == 0)
  {
    return NULL;
  }

  char* result_p = malloc (length + (is_null_terminated ? 1 : 0));

  if (result_p != NULL)
  {
    memcpy (result_p, str_p, length);

    if (is_null_terminated)
    {
      result_p[length] = '\0';
    }
  }

  return result_p;
} /* jjsp_strndup */

char*
jjsp_cesu8_to_utf8_sz (const uint8_t* cesu8_p, uint32_t cesu8_size)
{
  if (cesu8_size == 0)
  {
    return NULL;
  }

  lit_utf8_size_t utf8_size = lit_get_utf8_size_of_cesu8_string (cesu8_p, cesu8_size);
  lit_utf8_byte_t* utf8_p = malloc (utf8_size + 1);

  if (utf8_p == NULL)
  {
    return NULL;
  }

  lit_utf8_size_t written = lit_convert_cesu8_string_to_utf8_string (cesu8_p, cesu8_size, utf8_p, utf8_size);

  if (written != utf8_size)
  {
    free (utf8_p);
    return NULL;
  }

  utf8_p[utf8_size] = '\0';

  return (char*) utf8_p;
}

ecma_char_t*
jjsp_cesu8_to_utf16_sz (const uint8_t* cesu8_p, uint32_t cesu8_size)
{
  lit_utf8_size_t advance;
  ecma_char_t ch;
  ecma_char_t* result;
  ecma_char_t* result_cursor;
  lit_utf8_size_t result_size = 0;
  lit_utf8_size_t index = 0;

  while (lit_peek_wchar_from_cesu8 (cesu8_p, cesu8_size, index, &advance, &ch))
  {
    result_size++;
    index += advance;
  }

  result = result_cursor = malloc ((result_size + 1) * sizeof (ecma_char_t));

  if (result == NULL)
  {
    return NULL;
  }

  index = 0;

  while (lit_peek_wchar_from_cesu8 (cesu8_p, cesu8_size, index, &advance, &ch))
  {
    index += advance;
    *result_cursor++ = ch;
  }

  *result_cursor = L'\0';

  return result;
}

/**
 * Create an ecma string value from a platform buffer.
 *
 * @param buffer_p buffer object
 * @param move if true, this function owns buffer_p and will call buffer_p->free
 * @return on success, ecma string value; on error, ECMA_VALUE_EMPTY. return value must be freed with ecma_free_value.
 */
ecma_value_t
jjsp_buffer_to_string_value (jjs_platform_buffer_t* buffer_p, bool move)
{
  ecma_value_t result;

  if (buffer_p->encoding == JJS_ENCODING_UTF8)
  {
    result =
      ecma_make_string_value (ecma_new_ecma_string_from_utf8 ((const lit_utf8_byte_t*) buffer_p->data_p, buffer_p->length));
  }
  else if (buffer_p->encoding == JJS_ENCODING_UTF16)
  {
    JJS_ASSERT(buffer_p->length % 2 == 0);
    // buffer.length is in bytes convert to utf16 array size
    result = ecma_make_string_value (ecma_new_ecma_string_from_utf16 ((const uint16_t*) buffer_p->data_p,
                                                                      buffer_p->length / (uint32_t) sizeof (ecma_char_t)));
  }
  else
  {
    result = ECMA_VALUE_EMPTY;
  }

  if (move)
  {
    buffer_p->free (buffer_p);
  }

  return result;
} /* jjsp_buffer_to_string */

void
jjsp_io_log (const char* message_p)
{
  fputs (message_p, stderr);
} /* jjsp_log */

void
jjsp_fatal (jjs_fatal_code_t code)
{
  if (code != 0 && code != JJS_FATAL_OUT_OF_MEMORY)
  {
    abort ();
  }

  exit ((int) code);
}
