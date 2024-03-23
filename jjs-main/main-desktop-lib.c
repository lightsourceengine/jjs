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

#include "main-desktop-lib.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "jjs-port.h"

#ifdef _WIN32
#define platform_separator            '\\'
#define platform_is_path_separator(c) ((c) == '/' || (c) == platform_separator)
#define platform_realpath(path_p)     _fullpath (NULL, path_p, 0)
static bool platform_is_absolute_path (const char* path_p, size_t path_len);
#else
#include <libgen.h>

#define platform_separator            '/'
#define platform_is_path_separator(c) ((c) == platform_separator)
#define platform_realpath(path_p)     realpath (path_p, NULL)
static bool platform_is_absolute_path (const char* path_p, size_t path_len);
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
static void object_set_sz (jjs_value_t object, const char* key_sz, jjs_value_t value);

static const char* DEFAULT_STDIN_FILENAME = "<stdin>";

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
  jjs_value_t result = jjs_esm_evaluate (specifier);

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
  return path_len > 2
         && ((path_p[0] == '.' && platform_is_path_separator (path_p[1]))
             || (path_p[0] == '.' && path_p[1] == '.' && platform_is_path_separator (path_p[2])));
} /* is_relative_path */

/**
 * Joins cwd with filename that may or may not exist on disk.
 */
static jjs_value_t
cwd_append (const char* filename)
{
  char* p = platform_realpath (".");

  if (!p)
  {
    return jjs_throw_sz (JJS_ERROR_COMMON, "cwd_append(): failed to realpath '.'");
  }

  size_t p_size = strlen (p);
  size_t filename_size = strlen (filename);

  if (p_size == 0 || filename_size == 0)
  {
    return jjs_throw_sz (JJS_ERROR_COMMON, "cwd_append(): filename is empty");
  }

  size_t buffer_size = p_size + filename_size + 1;
  jjs_char_t* buffer = malloc (buffer_size);

  memcpy (buffer, p, p_size);
  buffer[p_size] = platform_separator;
  memcpy (&buffer[p_size + 1], filename, filename_size);

  jjs_value_t result = jjs_string (buffer, (jjs_size_t) buffer_size, JJS_ENCODING_UTF8);

  free (p);
  free (buffer);

  return result;
} /* cwd_append */

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
platform_is_absolute_path (const char* path_p, size_t path_len)
{
  (void) path_len;
  return platform_is_path_separator (path_p[0]);
} /* platform_is_absolute_path */

#endif

/**
 * Set an object property to a value.
 */
static void
object_set_sz (jjs_value_t object, const char* key_sz, jjs_value_t value)
{
  jjs_value_t key = jjs_string_utf8_sz (key_sz);
  jjs_value_free (jjs_object_set (object, key, value));
  jjs_value_free (key);
} /* object_set_sz */

/**
 * Performs a strict equals comparison on a JJS value and an UTF8 encoded C-string.
 */
static bool
string_strict_equals_sz (jjs_value_t value, const char* string_p)
{
  jjs_value_t string = jjs_string_utf8_sz (string_p);
  jjs_value_t equal_result = jjs_binary_op (JJS_BIN_OP_STRICT_EQUAL, value, string);
  bool result = jjs_value_is_true (equal_result);

  jjs_value_free (string);
  jjs_value_free (equal_result);

  return result;
} /* string_strict_equals_sz */

/**
 * binding for pmap(). can be called with pmap(filename) or pmap(object, dirname)
 */
static JJS_HANDLER (pmap_handler)
{
  (void) call_info_p; /* unused */

  jjs_value_t arg = args_cnt > 0 ? args_p[0] : jjs_undefined ();

  if (jjs_value_is_string (arg))
  {
    return jjs_pmap_from_file (arg);
  }
  else if (jjs_value_is_object (arg))
  {
    // TODO: should pmap api accept a json object?
    jjs_value_t json = jjs_json_stringify (arg);

    if (jjs_value_is_exception (json))
    {
      return json;
    }

    jjs_value_t result = jjs_pmap_from_json (json, args_cnt > 1 ? args_p[1] : jjs_undefined ());

    jjs_value_free (json);

    return result;
  }

  return jjs_pmap_from_file (args_cnt > 0 ? args_p[0] : jjs_undefined ());
} /* pmap_handler */

/**
 * jjs_pmap_resolve_handler() binding
 */
static JJS_HANDLER (pmap_resolve_handler)
{
  (void) call_info_p; /* unused */

  jjs_value_t value = args_cnt > 1 ? args_p[1] : jjs_undefined ();
  jjs_module_type_t module_type;

  if (string_strict_equals_sz (value, "commonjs"))
  {
    module_type = JJS_MODULE_TYPE_COMMONJS;
  }
  else if (string_strict_equals_sz (value, "module"))
  {
    module_type = JJS_MODULE_TYPE_MODULE;
  }
  else
  {
    module_type = JJS_MODULE_TYPE_NONE;
  }

  return jjs_pmap_resolve (args_cnt > 0 ? args_p[0] : jjs_undefined (), module_type);
} /* pmap_resolve_handler */

/**
 * Register the $jjs object.
 *
 * This object contains native bindings for the JJS API. It is intended to
 * be used for testing purposes only.
 */
void
main_register_jjs_test_object (void)
{
  jjs_value_t global = jjs_current_realm ();
  jjs_value_t jjs = jjs_object ();
  jjs_value_t jjs_key = jjs_string_utf8_sz ("$jjs");
  jjs_property_descriptor_t desc = {
    .flags = JJS_PROP_IS_CONFIGURABLE | JJS_PROP_IS_WRITABLE | JJS_PROP_IS_CONFIGURABLE_DEFINED
             | JJS_PROP_IS_WRITABLE_DEFINED | JJS_PROP_IS_VALUE_DEFINED,
    .value = jjs,
  };

  jjs_value_free (jjs_object_define_own_prop (global, jjs_key, &desc));
  jjs_value_free (jjs_key);

  jjs_value_t pmap = jjs_function_external (pmap_handler);
  jjs_value_t pmap_resolve = jjs_function_external (pmap_resolve_handler);

  object_set_sz (pmap, "resolve", pmap_resolve);
  object_set_sz (jjs, "pmap", pmap);

  jjs_value_free (pmap);
  jjs_value_free (pmap_resolve);
  jjs_value_free (global);
  jjs_value_free (jjs);
} /* main_register_jjs_test_object */

jjs_value_t
main_exec_stdin (main_input_type_t input_type, const char* filename)
{
  jjs_char_t *source_p = NULL;
  jjs_size_t source_size = 0;

  while (true)
  {
    jjs_size_t line_size;
    jjs_char_t *line_p = jjs_port_line_read (&line_size);

    if (line_p == NULL)
    {
      break;
    }

    jjs_size_t new_size = source_size + line_size;
    source_p = realloc (source_p, new_size);

    memcpy (source_p + source_size, line_p, line_size);
    jjs_port_line_free (line_p);
    source_size = new_size;
  }

  if (!jjs_validate_string (source_p, source_size, JJS_ENCODING_UTF8))
  {
    free (source_p);
    return jjs_throw_sz (JJS_ERROR_SYNTAX, "Input is not a valid UTF-8 encoded string.");
  }

  if (filename == NULL)
  {
    filename = DEFAULT_STDIN_FILENAME;
  }

  jjs_value_t result;

  if (input_type == INPUT_TYPE_MODULE)
  {
    jjs_esm_source_t source;

    jjs_esm_source_init (&source, source_p, source_size);
    jjs_esm_source_set_filename (&source, jjs_string_sz (filename), true);

    result = jjs_esm_evaluate_source (&source);

    jjs_esm_source_deinit (&source);
  }
  else if (input_type == INPUT_TYPE_SLOPPY_MODE || input_type == INPUT_TYPE_STRICT_MODE)
  {
    jjs_parse_options_t opts = {
      .options = JJS_PARSE_HAS_SOURCE_NAME,
      .source_name = jjs_string_sz (filename),
      .user_value = cwd_append (filename),
    };

    if (input_type == INPUT_TYPE_STRICT_MODE)
    {
      opts.options |= JJS_PARSE_STRICT_MODE;
    }

    jjs_value_t parse_result;

    if (jjs_value_is_exception (opts.user_value))
    {
      parse_result = jjs_value_copy (opts.user_value);
    }
    else
    {
      parse_result = jjs_parse (source_p, source_size, &opts);
    }

    jjs_value_free (opts.source_name);
    jjs_value_free (opts.user_value);

    if (jjs_value_is_exception (parse_result))
    {
      result = parse_result;
    }
    else
    {
      result = jjs_run (parse_result);
      jjs_value_free (parse_result);
    }
  }
  else
  {
    result = jjs_throw_sz (JJS_ERROR_COMMON, "Invalid input type.");
  }

  free (source_p);

  return result;
} /* main_exec_stdin */
