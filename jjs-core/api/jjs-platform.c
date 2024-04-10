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
#include "jjs-util.h"

#include "annex.h"
#include "jcontext.h"
#include "jjs-platform.h"
#include "jjs-compiler.h"

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
  jjs_platform_cwd_fn_t cwd = JJS_CONTEXT (platform_api).path_cwd;

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
 * Version of jjs_platform_realpath that takes a null-terminated string for the path.
 *
 * @see jjs_platform_realpath
 */
jjs_value_t
jjs_platform_realpath_sz (const char* path_p)
{
  jjs_assert_api_enabled();
  return jjs_platform_realpath (annex_util_create_string_utf8_sz (path_p), JJS_MOVE);
} /* jjs_platform_realpath_sz */

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
 * Version of jjs_platform_read_file that takes a null-terminated string for the path.
 *
 * @see jjs_platform_read_file
 */
jjs_value_t
jjs_platform_read_file_sz (const char* path_p, const jjs_platform_read_file_options_t* opts)
{
  jjs_assert_api_enabled();
  return jjs_platform_read_file (annex_util_create_string_utf8_sz (path_p), JJS_MOVE, opts);
} /* jjs_platform_read_file_sz */

/**
 * Checks if platform api platform.path.cwd is installed in the current context.
 *
 * If installed, jjs_platform_cwd () can be called.
 *
 * @return bool
 */
bool jjs_platform_has_cwd (void)
{
  jjs_assert_api_enabled();
  return (JJS_CONTEXT (platform_api).path_cwd != NULL);
} /* jjs_platform_has_cwd */

/**
 * Checks if platform api platform.path.realpath is installed in the current context.
 *
 * If installed, jjs_platform_realpath () can be called.
 *
 * @return bool
 */
bool jjs_platform_has_realpath (void)
{
  jjs_assert_api_enabled();
  return (JJS_CONTEXT (platform_api).path_realpath != NULL);
} /* jjs_platform_has_realpath */

/**
 * Checks if platform api platform.fs.read_file is installed in the current context.
 *
 * If installed, jjs_platform_read_file () can be called.
 *
 * @return bool
 */
bool jjs_platform_has_read_file (void)
{
  jjs_assert_api_enabled();
  return (JJS_CONTEXT (platform_api).fs_read_file != NULL);
} /* jjs_platform_has_read_file */

/**
 * Get the OS identifier as a JS string.
 *
 * Possible values: [ aix, darwin, freebsd, linux, openbsd, sunos, win32, unknown ]
 *
 * @see jjs_platform_os_type
 *
 * @return string os identifier.
 */
jjs_value_t
jjs_platform_os (void)
{
  jjs_assert_api_enabled ();

  lit_magic_string_id_t id;

  switch (jjs_platform_os_type ())
  {
    case JJS_PLATFORM_OS_AIX:
      id = LIT_MAGIC_STRING_OS_AIX;
      break;
    case JJS_PLATFORM_OS_DARWIN:
      id = LIT_MAGIC_STRING_OS_DARWIN;
      break;
    case JJS_PLATFORM_OS_FREEBSD:
      id = LIT_MAGIC_STRING_OS_FREEBSD;
      break;
    case JJS_PLATFORM_OS_LINUX:
      id = LIT_MAGIC_STRING_OS_LINUX;
      break;
    case JJS_PLATFORM_OS_OPENBSD:
      id = LIT_MAGIC_STRING_OS_OPENBSD;
      break;
    case JJS_PLATFORM_OS_SUNOS:
      id = LIT_MAGIC_STRING_OS_SUNOS;
      break;
    case JJS_PLATFORM_OS_WIN32:
      id = LIT_MAGIC_STRING_OS_WIN32;
      break;
    case JJS_PLATFORM_OS_UNKNOWN:
    default:
      id = LIT_MAGIC_STRING_UNKNOWN;
      break;
  }

  return ecma_make_magic_string_value (id);
} /* jjs_platform_os */

/**
 * Get the OS identifier of the system.
 *
 * The value is determined based on compiler flags in jjs-compiler.h. The
 * set of identifiers is based on node's process.platform.
 *
 * This method can be called before context initialization.
 *
 * @return os type
 */
jjs_platform_os_t
jjs_platform_os_type (void)
{
  return JJS_PLATFORM_OS_TYPE;
} /* jjs_platform_os_type */

/**
 * Get the platform's arch identifier.
 *
 * Possible values: [ arm, arm64, ia32, loong64, mips, mipsel, ppc, ppc64, riscv64, s390, s390x, x64, unknown ]
 *
 * @see jjs_platform_os_type
 *
 * @return string arch identifier.
 */
jjs_value_t
jjs_platform_arch (void)
{
  jjs_assert_api_enabled ();

  lit_magic_string_id_t id;
  
  switch (jjs_platform_arch_type ())
  {
    case JJS_PLATFORM_ARCH_ARM:
      id = LIT_MAGIC_STRING_ARCH_ARM;
      break;
    case JJS_PLATFORM_ARCH_ARM64:
      id = LIT_MAGIC_STRING_ARCH_ARM64;
      break;
    case JJS_PLATFORM_ARCH_IA32:
      id = LIT_MAGIC_STRING_ARCH_IA32;
      break;
    case JJS_PLATFORM_ARCH_LOONG64:
      id = LIT_MAGIC_STRING_ARCH_LOONG64;
      break;
    case JJS_PLATFORM_ARCH_MIPS:
      id = LIT_MAGIC_STRING_ARCH_MIPS;
      break;
    case JJS_PLATFORM_ARCH_MIPSEL:
      id = LIT_MAGIC_STRING_ARCH_MIPSEL;
      break;
    case JJS_PLATFORM_ARCH_PPC:
      id = LIT_MAGIC_STRING_ARCH_PPC;
      break;
    case JJS_PLATFORM_ARCH_PPC64:
      id = LIT_MAGIC_STRING_ARCH_PPC64;
      break;
    case JJS_PLATFORM_ARCH_RISCV64:
      id = LIT_MAGIC_STRING_ARCH_RISCV64;
      break;
    case JJS_PLATFORM_ARCH_S390:
      id = LIT_MAGIC_STRING_ARCH_S390;
      break;
    case JJS_PLATFORM_ARCH_S390X:
      id = LIT_MAGIC_STRING_ARCH_S390X;
      break;
    case JJS_PLATFORM_ARCH_X64:
      id = LIT_MAGIC_STRING_ARCH_X64;
      break;
    case JJS_PLATFORM_ARCH_UNKNOWN:
    default:
      id = LIT_MAGIC_STRING_UNKNOWN;
      break;
  }
  
  return ecma_make_magic_string_value (id);
} /* jjs_platform_arch */

/**
 * Get the CPU architecture of the system.
 *
 * The value is determined based on compiler flags in jjs-compiler.h. The
 * set of identifiers is based on node's process.arch.
 *
 * This method can be called before context initialization.
 *
 * @return arch type
 */
jjs_platform_arch_t
jjs_platform_arch_type (void)
{
  return JJS_PLATFORM_ARCH_TYPE;
} /* jjs_platform_arch_type */

/**
 * Immediately terminate the process due to an unrecoverable condition. It is
 * equivalent to an exit () or abort (), so this function will never return.
 *
 * The function can be called before engine initialization.
 *
 * @param code exit code
 */
void
jjs_platform_fatal (jjs_fatal_code_t code)
{
  jjs_platform_fatal_fn_t fatal_fn = JJS_CONTEXT (platform_api).fatal;

  if (!fatal_fn)
  {
    fatal_fn = jjsp_fatal;
  }

  fatal_fn (code);
} /* jjs_platform_fatal */

/**
 * Helper function to deal with cesu8 strings in platform api implementations.
 *
 * @param cesu8 bytes encoded in CESU8
 * @param cesu8_size number of bytes in cesu8 buffer
 * @param encoding encoding to convert to
 * @param with_null_terminator append a null terminator to output buffer
 * @param out_pp pointer to newly allocated buffer in target encoding format. for cesu8 or utf8, buffer is
 *               uint8_t. for utf16, buffer is uint16_t.
 * @param out_len_p number of elements (not byte size) in output buffer
 * @return true: conversion successful. caller must free out buffer with jjs_platform_convert_cesu8_free.
 *         false: conversion failed
 */
bool jjs_platform_convert_cesu8 (const jjs_char_t *cesu8,
                                 jjs_size_t cesu8_size,
                                 jjs_encoding_t encoding,
                                 bool with_null_terminator,
                                 void **out_pp,
                                 jjs_size_t *out_len_p)
{
  switch (encoding)
  {
    case JJS_ENCODING_UTF8:
    {
      char* buffer_p = jjsp_cesu8_to_utf8_sz (cesu8, cesu8_size, with_null_terminator, out_len_p);

      if (!buffer_p)
      {
        return false;
      }

      *out_pp = buffer_p;
      return true;
    }
    case JJS_ENCODING_CESU8:
    {
      uint8_t* buffer_p = malloc (cesu8_size + (with_null_terminator ? 1 : 0));

      if (!buffer_p)
      {
        return false;
      }

      memcpy (buffer_p, cesu8, cesu8_size);

      if (with_null_terminator)
      {
        buffer_p[cesu8_size] = '\0';
      }

      *out_pp = buffer_p;
      return true;
    }
    case JJS_ENCODING_UTF16:
    {
      uint16_t *buffer_p = jjsp_cesu8_to_utf16_sz (cesu8, cesu8_size, with_null_terminator, out_len_p);

      if (!buffer_p)
      {
        return false;
      }

      *out_pp = buffer_p;
      return true;
    }
    default:
    {
      return false;
    }
  }
} /* jjs_platform_convert_cesu8 */

void jjs_platform_convert_cesu8_free (void* converted)
{
  free (converted);
} /* jjs_platform_convert_cesu8_free */

jjs_platform_t
jjsp_defaults (void)
{
  jjs_platform_t platform = {0};

  platform.fatal = jjsp_fatal;

#if JJS_PLATFORM_API_IO_WRITE
  platform.io_write = jjsp_io_write;
#endif /* JJS_PLATFORM_API_IO_WRITE */

#if JJS_PLATFORM_API_IO_FLUSH
  platform.io_flush = jjsp_io_flush;
#endif /* JJS_PLATFORM_API_IO_FLUSH */

  platform.io_stdout = stdout;
  platform.io_stdout_encoding = JJS_ENCODING_UTF8;

  platform.io_stderr = stderr;
  platform.io_stderr_encoding = JJS_ENCODING_UTF8;

#if JJS_PLATFORM_API_TIME_LOCAL_TZA
  platform.time_local_tza = jjsp_time_local_tza;
#endif /* JJS_PLATFORM_API_TIME_LOCAL_TZA */

#if JJS_PLATFORM_API_TIME_NOW_MS
  platform.time_now_ms = jjsp_time_now_ms;
#endif /* JJS_PLATFORM_API_TIME_NOW_MS */

#if JJS_PLATFORM_API_TIME_SLEEP
  platform.time_sleep = jjsp_time_sleep;
#endif /* JJS_PLATFORM_API_TIME_SLEEP */

#if JJS_PLATFORM_API_FS_READ_FILE
  platform.fs_read_file = jjsp_fs_read_file;
#endif /* JJS_PLATFORM_API_FS_READ_FILE */

#if JJS_PLATFORM_API_PATH_CWD
  platform.path_cwd = jjsp_cwd;
#endif /* JJS_PLATFORM_API_PATH_CWD */

#if JJS_PLATFORM_API_PATH_REALPATH
  platform.path_realpath = jjsp_path_realpath;
#endif /* JJS_PLATFORM_API_PATH_REALPATH */

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
jjsp_cesu8_to_utf8_sz (const uint8_t* cesu8_p, uint32_t cesu8_size, bool is_null_terminated, lit_utf8_size_t* out_size)
{
  if (cesu8_size == 0)
  {
    return NULL;
  }

  lit_utf8_size_t utf8_size = lit_get_utf8_size_of_cesu8_string (cesu8_p, cesu8_size);
  lit_utf8_byte_t* utf8_p = malloc (utf8_size + (is_null_terminated ? 1 : 0));

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

  if (is_null_terminated)
  {
    utf8_p[utf8_size] = '\0';
  }

  if (out_size)
  {
    *out_size = utf8_size;
  }

  return (char*) utf8_p;
}

ecma_char_t*
jjsp_cesu8_to_utf16_sz (const uint8_t* cesu8_p, uint32_t cesu8_size, bool is_null_terminated, lit_utf8_size_t* out_size)
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

  result = result_cursor = malloc ((result_size + (is_null_terminated ? 1 : 0)) * sizeof (ecma_char_t));

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

  if (is_null_terminated)
  {
    *result_cursor = L'\0';
  }

  if (out_size)
  {
    *out_size = result_size;
  }

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
jjsp_io_write (void* target_p, const uint8_t* data_p, uint32_t data_size, jjs_encoding_t encoding)
{
  // TODO: check encoding?
  JJS_UNUSED (encoding);
  JJS_ASSERT (target_p != NULL);
  fwrite (data_p, 1, data_size, target_p);
} /* jjsp_io_write */

void
jjsp_io_flush (void* target_p)
{
  JJS_ASSERT (target_p != NULL);
  fflush (target_p);
} /* jjsp_io_write */

void
jjsp_fatal (jjs_fatal_code_t code)
{
  if (code != 0 && code != JJS_FATAL_OUT_OF_MEMORY)
  {
    abort ();
  }

  exit ((int) code);
}
