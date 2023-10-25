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

#include "main-module.h"

#include "jjs-port.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#define platform_is_path_separator(c) ((c) == '/' || (c) == '\\')
#define platform_realpath(path_p) _fullpath(NULL, path_p, 0)
static bool platform_is_absolute_path(const char* path_p, size_t path_len);
#else
#include <libgen.h>

#define platform_is_path_separator(c) ((c) == '/')
#define platform_realpath(path_p) realpath(path_p, NULL)
static bool platform_is_absolute_path(const char* path_p, size_t path_len);
#endif

typedef struct raw_source_s
{
  jjs_char_t* buffer_p;
  jjs_size_t buffer_size;
  jjs_value_t status;
} raw_source_t;

static jjs_value_t jjs_string_utf8_sz (const char* str);
static raw_source_t read_source (const char* path_p);
static void free_source (raw_source_t* source);
static jjs_value_t resolve_specifier (const char* path_p);

/**
 * Run a module from a file.
 *
 * @param path_p filename of module to run
 * @return evaluation result or exception (read file, parse, JS runtime error)
 */
jjs_value_t
main_module_run_esm (const char* path_p)
{
  jjs_value_t specifier = resolve_specifier (path_p);
  jjs_value_t result = jjs_esm_run (specifier);

  jjs_value_free (specifier);

  return result;
}

/**
 * Run a non-ESM file.
 */
jjs_value_t
main_module_run (const char* path_p)
{
  char* full_path_p = platform_realpath (path_p);

  if (!full_path_p)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "cannot resolve require path");
  }

  raw_source_t source = read_source (full_path_p);

  if (jjs_value_is_exception (source.status))
  {
    free (full_path_p);
    return source.status;
  }

  jjs_value_t path_value = jjs_string_utf8_sz (full_path_p);

  jjs_parse_options_t parse_options = {
    .options = JJS_PARSE_HAS_SOURCE_NAME | JJS_PARSE_HAS_USER_VALUE,
    .source_name = path_value,
    .user_value = path_value,
  };

  jjs_value_t parsed = jjs_parse (source.buffer_p, source.buffer_size, &parse_options);

  free (full_path_p);
  free_source (&source);
  jjs_value_free (path_value);

  if (jjs_value_is_exception (parsed))
  {
    return parsed;
  }

  jjs_value_t result = jjs_run (parsed);

  jjs_value_free (parsed);

  return result;
} /* main_module_run */

/**
 * Check if the path is relative.
 */
static bool
is_relative_path (const char* path_p, size_t path_len)
{
    return path_len > 2 && ((path_p[0] == '.' && platform_is_path_separator (path_p[1])) ||
                            (path_p[0] == '.' && path_p[1] == '.' && platform_is_path_separator (path_p[2])));
} /* is_relative_path */

/**
 * Ensure the specifier is a relative or absolute path.
 */
static jjs_value_t
resolve_specifier (const char* path_p)
{
  jjs_value_t specifier;
  size_t path_len = strlen (path_p);

  if (platform_is_absolute_path (path_p, path_len) || is_relative_path (path_p, path_len))
  {
    specifier = jjs_string_utf8_sz (path_p);
  }
  else
  {
    jjs_value_t prefix = jjs_string_utf8_sz ("./");
    jjs_value_t path = jjs_string_utf8_sz (path_p);

    specifier = jjs_binary_op (JJS_BIN_OP_ADD, prefix, path);

    jjs_value_free (prefix);
    jjs_value_free (path);
  }

  return specifier;
} /* resolve_specifier */

/**
 * Creates JJS string from a UTF-8 encoded C string.
 */
static jjs_value_t
jjs_string_utf8_sz(const char* str)
{
  return jjs_string ((const jjs_char_t *) str, (jjs_size_t) strlen (str), JJS_ENCODING_UTF8);
} /* jjs_string_utf8_sz */

/**
 * Read source from a UTF-8 encoded file into a buffer.
 *
 * @param path_p path to file
 *
 * error: JJS exception
 * success: raw_source_t containing buffer and buffer size
 * memory: caller must call free_source() on result
 */
static raw_source_t
read_source (const char* path_p)
{
  jjs_size_t source_size;
  jjs_char_t *source_p = jjs_port_source_read (path_p, &source_size);

  if (source_p == NULL)
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Failed to open file: %s\n", path_p);
    return (raw_source_t) {
      .buffer_p = NULL,
      .buffer_size = 0,
      .status = jjs_throw_sz (JJS_ERROR_SYNTAX, "Source file not found"),
    };
  }
  else if (!jjs_validate_string (source_p, source_size, JJS_ENCODING_UTF8))
  {
    jjs_port_source_free (source_p);
    return (raw_source_t) {
      .buffer_p = NULL,
      .buffer_size = 0,
      .status = jjs_throw_sz (JJS_ERROR_SYNTAX, "Source is not a valid UTF-8 encoded string."),
    };
  }

  return  (raw_source_t){ .buffer_p = source_p, .buffer_size = source_size, .status = jjs_undefined() };
} /* read_source */

/**
 * Cleanup the result of read_source().
 */
static void
free_source (raw_source_t* source)
{
  jjs_port_source_free (source->buffer_p);
  jjs_value_free (source->status);
} /* free_source */

#ifdef _WIN32

static bool
platform_is_absolute_path(const char* path_p, size_t path_len)
{
  if (platform_is_path_separator (path_p[0]))
  {
    return true;
  }
  else
  {
    return path_len > 2 && isalpha(path_p[0]) && path_p[1] == ':' && platform_is_path_separator (path_p[2]);
  }
} /* platform_is_absolute_path */

#else

static bool
platform_is_absolute_path(const char* path_p, size_t path_len)
{
  (void) path_len;
  return platform_is_path_separator (path_p[0]);
} /* platform_is_absolute_path */

#endif
