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

#include "jjs-platform.h"

#include "jjs-annex.h"
#include "jjs-compiler.h"
#include "jjs-core.h"
#include "jjs-stream.h"
#include "jjs-util.h"

#include "annex.h"
#include "jcontext.h"

static jjs_value_t jjsp_read_file_buffer (jjs_value_t path,
                                          jjs_allocator_t* path_allocator,
                                          jjs_allocator_t* buffer_allocator,
                                          jjs_platform_buffer_t* buffer_p);
static jjs_value_t jjsp_read_file (jjs_value_t path, jjs_encoding_t encoding);
static jjs_platform_path_t
jjs_platform_create_path (jjs_allocator_t* allocator, const uint8_t* path_p, jjs_size_t size, jjs_encoding_t encoding);
static ecma_value_t jjsp_buffer_view_to_string_value (jjs_platform_buffer_view_t* buffer_p, bool move);
static void jjs_platform_buffer_view_free (jjs_platform_buffer_view_t* self_p);

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

  jjs_allocator_t* allocator = jjs_util_context_acquire_scratch_allocator ();

  jjs_platform_buffer_view_t buffer = {
    .free = jjs_platform_buffer_view_free,
    .source = jjs_platform_buffer (NULL, 0, allocator),
  };

  if (cwd (allocator, &buffer) == JJS_STATUS_OK)
  {
    ecma_value_t result = jjsp_buffer_view_to_string_value (&buffer, true);

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
    JJS_DISOWN (path, path_o);
    return jjs_throw_sz (JJS_ERROR_COMMON, "platform api 'path_realpath' not installed");
  }

  if (!jjs_value_is_string (path))
  {
    JJS_DISOWN (path, path_o);
    return jjs_throw_sz (JJS_ERROR_TYPE, "expected path to be a string");
  }

  jjs_allocator_t* allocator = jjs_util_context_acquire_scratch_allocator ();

  ECMA_STRING_TO_UTF8_STRING (ecma_get_string_from_value (path), path_bytes_p, path_bytes_len);

  jjs_platform_path_t p = jjs_platform_create_path (
    allocator,
    path_bytes_p,
    path_bytes_len,
    ecma_string_get_length (ecma_get_string_from_value (path)) == path_bytes_len ? JJS_ENCODING_ASCII
                                                                                 : JJS_ENCODING_CESU8);
  jjs_platform_buffer_view_t buffer;
  jjs_status_t status = realpath_fn (allocator, &p, &buffer);
  jjs_value_t result;

  if (status == JJS_STATUS_OK)
  {
    result = jjsp_buffer_view_to_string_value (&buffer, true);
  }
  else
  {
    result = jjs_throw_sz (JJS_ERROR_COMMON, "failed to get realpath from path");
  }

  ECMA_FINALIZE_UTF8_STRING (path_bytes_p, path_bytes_len);

  jjs_util_context_release_scratch_allocator ();
  JJS_DISOWN (path, path_o);

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
 * Write a string to the platform stdout write stream.
 *
 * If the value is not a string or the platform does not have stdout stream installed,
 * this function does nothing.
 *
 * @param value JS string value
 * @param value_o value reference ownership
 */
void jjs_platform_stdout_write (jjs_value_t value, jjs_value_ownership_t value_o)
{
  jjs_assert_api_enabled();
  jjs_stream_write_string (JJS_STDOUT, value, value_o);
} /* jjs_platform_stdout_write */

/**
 * Flush the platform stdout write stream.
 *
 * If the platform does not have stdout stream installed, this function does nothing.
 */
void jjs_platform_stdout_flush (void)
{
  jjs_assert_api_enabled();
  jjs_stream_flush (JJS_STDOUT);
} /* jjs_platform_stdout_flush */

/**
 * Checks if the platform has stdout stream installed.
 *
 * @return boolean status
 */
bool
jjs_platform_has_stdout (void)
{
  jjs_assert_api_enabled ();
  return jjs_stream_is_installed (JJS_STDOUT);
} /* jjs_platform_has_stdout */

/**
 * Write a string to the platform stderr write stream.
 *
 * If the value is not a string or the platform does not have stderr stream installed,
 * this function does nothing.
 *
 * @param value JS string value
 * @param value_o value reference ownership
 */
void
jjs_platform_stderr_write (jjs_value_t value, jjs_value_ownership_t value_o)
{
  jjs_assert_api_enabled ();
  jjs_stream_write_string (JJS_STDERR, value, value_o);
} /* jjs_platform_stderr_write */

/**
 * Flush the platform stderr write stream.
 *
 * If the platform does not have stderr stream installed, this function does nothing.
 */
void
jjs_platform_stderr_flush (void)
{
  jjs_assert_api_enabled ();
  jjs_stream_flush (JJS_STDERR);
} /* jjs_platform_stderr_flush */

/**
 * Checks if the platform has stderr stream installed.
 *
 * @return boolean status
 */
bool
jjs_platform_has_stderr (void)
{
  jjs_assert_api_enabled ();
  return jjs_stream_is_installed (JJS_STDERR);
} /* jjs_platform_has_stderr */

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
    fatal_fn = jjsp_fatal_impl;
  }

  fatal_fn (code);
}

static void
jjs_platform_buffer_view_free (jjs_platform_buffer_view_t* self_p)
{
  self_p->source.free (&self_p->source);
  self_p->data_p = NULL;
  self_p->data_size = 0;
  self_p->encoding = JJS_ENCODING_NONE;
}

static void
jjs_platform_buffer_free (jjs_platform_buffer_t* buffer_p)
{
  if (buffer_p && buffer_p->allocator)
  {
    buffer_p->allocator->free (buffer_p->allocator, buffer_p->data_p, buffer_p->data_size);
    buffer_p->data_p = NULL;
    buffer_p->data_size = 0;
    buffer_p->allocator = NULL;
  }
}

void
jjs_platform_buffer_view_from_buffer (jjs_platform_buffer_view_t* self_p, jjs_platform_buffer_t* source_p, jjs_encoding_t encoding)
{
  self_p->data_p = source_p->data_p;
  self_p->data_size = source_p->data_size;
  self_p->encoding = encoding;
  self_p->source = *source_p;
  self_p->free = jjs_platform_buffer_view_free;
}

jjs_status_t
jjs_platform_buffer_view_new (jjs_platform_buffer_view_t* self_p,
                                jjs_allocator_t* allocator,
                                jjs_size_t size,
                                jjs_encoding_t encoding)
{
  jjs_status_t status = jjs_platform_buffer_new (&self_p->source, allocator, size);

  if (status != JJS_STATUS_OK)
  {
    return status;
  }

  self_p->data_p = self_p->source.data_p;
  self_p->data_size = self_p->source.data_size;
  self_p->encoding = encoding;

  *self_p = (jjs_platform_buffer_view_t) {
    .data_p = self_p->source.data_p,
    .data_size = self_p->source.data_size,
    .free = jjs_platform_buffer_view_free,
  };

  return JJS_STATUS_OK;
}

jjs_status_t
jjs_platform_buffer_new (jjs_platform_buffer_t* self_p, jjs_allocator_t* allocator, jjs_size_t size)
{
  void* p = allocator->alloc (allocator, size);

  if (p == NULL)
  {
    return JJS_STATUS_BAD_ALLOC;
  }

  *self_p = (jjs_platform_buffer_t) {
    .data_p = p,
    .data_size = size,
    .allocator = allocator,
    .free = jjs_platform_buffer_free,
  };

  return JJS_STATUS_OK;
}

jjs_platform_buffer_t
jjs_platform_buffer (void* data_p, jjs_size_t data_size, jjs_allocator_t* allocator)
{
  return (jjs_platform_buffer_t) {
    .data_p = data_p,
    .data_size = data_size,
    .allocator = allocator,
    .free = jjs_platform_buffer_free,
  };
}

jjs_platform_t
jjsp_defaults (void)
{
  jjs_platform_t platform = {0};

  platform.fatal = jjsp_fatal_impl;

#if JJS_PLATFORM_API_IO_WRITE
  platform.io_write = jjsp_io_write_impl;
#endif /* JJS_PLATFORM_API_IO_WRITE */

#if JJS_PLATFORM_API_IO_FLUSH
  platform.io_flush = jjsp_io_flush_impl;
#endif /* JJS_PLATFORM_API_IO_FLUSH */

  platform.io_stdout = stdout;
  platform.io_stdout_encoding = JJS_ENCODING_UTF8;

  platform.io_stderr = stderr;
  platform.io_stderr_encoding = JJS_ENCODING_UTF8;

#if JJS_PLATFORM_API_TIME_LOCAL_TZA
  platform.time_local_tza = jjsp_time_local_tza_impl;
#endif /* JJS_PLATFORM_API_TIME_LOCAL_TZA */

#if JJS_PLATFORM_API_TIME_NOW_MS
  platform.time_now_ms = jjsp_time_now_ms_impl;
#endif /* JJS_PLATFORM_API_TIME_NOW_MS */

#if JJS_PLATFORM_API_TIME_SLEEP
  platform.time_sleep = jjsp_time_sleep_impl;
#endif /* JJS_PLATFORM_API_TIME_SLEEP */

#if JJS_PLATFORM_API_FS_READ_FILE
  platform.fs_read_file = jjsp_fs_read_file_impl;
#endif /* JJS_PLATFORM_API_FS_READ_FILE */

#if JJS_PLATFORM_API_PATH_CWD
  platform.path_cwd = jjsp_cwd_impl;
#endif /* JJS_PLATFORM_API_PATH_CWD */

#if JJS_PLATFORM_API_PATH_REALPATH
  platform.path_realpath = jjsp_path_realpath_impl;
#endif /* JJS_PLATFORM_API_PATH_REALPATH */

  return platform;
}

static jjs_value_t
jjsp_read_file_buffer (jjs_value_t path,
                       jjs_allocator_t* path_allocator,
                       jjs_allocator_t* buffer_allocator,
                       jjs_platform_buffer_t* buffer_p)
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

  jjs_platform_path_t platform_path =
    jjs_platform_create_path (path_allocator,
                              path_bytes_p,
                              path_len,
                              ecma_string_get_length (path_p) == path_len ? JJS_ENCODING_ASCII : JJS_ENCODING_CESU8);

  jjs_status_t status = read_file (buffer_allocator, &platform_path, buffer_p);

  ECMA_FINALIZE_UTF8_STRING (path_bytes_p, path_len);

  if (status != JJS_STATUS_OK)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Failed to read source file");
  }

  return ECMA_VALUE_UNDEFINED;
}

static jjs_value_t
jjsp_read_file (jjs_value_t path, jjs_encoding_t encoding)
{
  jjs_value_t result;

  if (encoding == JJS_ENCODING_NONE)
  {
    jjs_allocator_t* path_allocator = jjs_util_context_acquire_scratch_allocator ();
    jjs_allocator_t buffer_allocator = jjs_util_arraybuffer_allocator ();
    jjs_platform_buffer_t buffer = jjs_platform_buffer (NULL, 0, &buffer_allocator);

    result = jjsp_read_file_buffer (path, path_allocator, &buffer_allocator, &buffer);

    if (jjs_value_is_exception (result))
    {
      jjs_util_context_release_scratch_allocator ();
      return result;
    }

    jjs_value_free (result);

    result = jjs_util_arraybuffer_allocator_move (&buffer_allocator);
    jjs_util_context_release_scratch_allocator ();
  }
  else if (encoding == JJS_ENCODING_UTF8 || encoding == JJS_ENCODING_CESU8)
  {
    jjs_allocator_t* allocator = jjs_util_context_acquire_scratch_allocator ();
    jjs_platform_buffer_t buffer = jjs_platform_buffer (NULL, 0, allocator);

    result = jjsp_read_file_buffer (path, allocator, allocator, &buffer);

    if (!jjs_value_is_exception (result))
    {
      if (jjs_validate_string (buffer.data_p, buffer.data_size, encoding))
      {
        result = jjs_string (buffer.data_p, buffer.data_size, encoding);
      }
      else
      {
        result = jjs_throw_sz (JJS_ERROR_COMMON, "file contents cannot be decoded");
      }
    }

    jjs_platform_buffer_free (&buffer);
    jjs_util_context_release_scratch_allocator ();
  }
  else
  {
    result = jjs_throw_sz (JJS_ERROR_TYPE, "unsupported read file encoding");
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
static ecma_value_t
jjsp_buffer_view_to_string_value (jjs_platform_buffer_view_t* buffer_p, bool move)
{
  ecma_value_t result;

  if (buffer_p->encoding == JJS_ENCODING_UTF8)
  {
    result = ecma_make_string_value (
      ecma_new_ecma_string_from_utf8 ((const lit_utf8_byte_t*) buffer_p->data_p, buffer_p->data_size));
  }
  else if (buffer_p->encoding == JJS_ENCODING_UTF16)
  {
    JJS_ASSERT (buffer_p->data_size % 2 == 0);
    // buffer.length is in bytes convert to utf16 array size
    result =
      ecma_make_string_value (ecma_new_ecma_string_from_utf16 ((const uint16_t*) buffer_p->data_p,
                                                               buffer_p->data_size / (uint32_t) sizeof (ecma_char_t)));
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
}

static jjs_status_t
jjs_platform_path_convert (jjs_platform_path_t* self_p,
                           jjs_encoding_t encoding,
                           uint32_t flags,
                           jjs_platform_buffer_view_t* buffer_view_p)
{
  void* dest_p;
  jjs_size_t dest_size;

  jjs_status_t status = jjs_util_convert (self_p->allocator,
                                          self_p->path_p,
                                          self_p->path_size,
                                          self_p->encoding,
                                          &dest_p,
                                          &dest_size,
                                          encoding,
                                          (flags & JJS_PATH_FLAG_NULL_TERMINATE) != 0,
                                          false);

  if (status == JJS_STATUS_OK)
  {
    jjs_platform_buffer_t buffer = jjs_platform_buffer (dest_p, dest_size, self_p->allocator);

    jjs_platform_buffer_view_from_buffer (buffer_view_p, &buffer, encoding);
  }

  return status;
}

static jjs_platform_path_t
jjs_platform_create_path (jjs_allocator_t* allocator, const uint8_t* path_p, jjs_size_t size, jjs_encoding_t encoding)
{
  return (jjs_platform_path_t){
    .path_p = path_p,
    .path_size = size,
    .encoding = encoding,
    .convert = jjs_platform_path_convert,
    .allocator = allocator,
  };
}

void
jjsp_io_write_impl (void* target_p, const uint8_t* data_p, uint32_t data_size, jjs_encoding_t encoding)
{
  JJS_ASSERT (target_p != NULL);
  JJS_ASSERT (encoding == JJS_ENCODING_ASCII || encoding == JJS_ENCODING_UTF8);
  fwrite (data_p, 1, data_size, target_p);
}

void
jjsp_io_flush_impl (void* target_p)
{
  JJS_ASSERT (target_p != NULL);
  fflush (target_p);
}

void
jjsp_fatal_impl (jjs_fatal_code_t code)
{
  if (code != 0 && code != JJS_FATAL_OUT_OF_MEMORY)
  {
    abort ();
  }

  exit ((int) code);
}
