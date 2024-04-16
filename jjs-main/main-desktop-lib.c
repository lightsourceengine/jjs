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

#include "cmdline.h"

// TODO: should come from port / platform
#ifdef _WIN32
#define platform_separator            '\\'
#else
#define platform_separator            '/'
#endif

static const char* DEFAULT_STDIN_FILENAME = "<stdin>";

/**
 * Joins cwd with filename that may or may not exist on disk.
 */
static jjs_value_t
cwd_append (jjs_value_t filename)
{
  jjs_value_t cwd = jjs_platform_cwd ();
  char separator_p [] = { platform_separator, 0 };
  jjs_value_t separator = jjs_string_sz (separator_p);
  jjs_value_t lhs = jjs_binary_op (JJS_BIN_OP_ADD, cwd, separator);
  jjs_value_t full_path = jjs_binary_op (JJS_BIN_OP_ADD, lhs, filename);

  jjs_value_free (separator);
  jjs_value_free (lhs);
  jjs_value_free (cwd);

  return full_path;
} /* cwd_append */

jjs_value_t
main_exec_stdin (main_input_type_t input_type, const char* filename)
{
  jjs_char_t *source_p = NULL;
  jjs_size_t source_size = 0;

  while (true)
  {
    jjs_size_t line_size;
    jjs_char_t *line_p = cmdline_stdin_readline (1024, &line_size);

    if (line_p == NULL)
    {
      break;
    }

    jjs_size_t new_size = source_size + line_size;
    source_p = realloc (source_p, new_size);

    memcpy (source_p + source_size, line_p, line_size);
    free (line_p);
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
    jjs_esm_source_t source = {
      .source_buffer_p = source_p,
      .source_buffer_size = source_size,
      .filename = jjs_string_utf8_sz (filename),
      // TODO: .dirname = dirname (filename),
    };

    result = jjs_esm_evaluate_source (&source, JJS_MOVE);
  }
  else if (input_type == INPUT_TYPE_SLOPPY_MODE || input_type == INPUT_TYPE_STRICT_MODE)
  {
    jjs_value_t filename_value = jjs_string_utf8_sz (filename);

    jjs_parse_options_t opts = {
      .options = JJS_PARSE_HAS_SOURCE_NAME,
      .source_name = filename_value,
      // TODO: this might not be correct without basename'ing filename
      .user_value = cwd_append (filename_value),
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
