/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "jjs-ext/repl.h"

#include <string.h>

#include "jjs-port.h"
#include "jjs.h"

#include "jjs-ext/print.h"

void
jjsx_repl (const char *prompt_p)
{
  jjs_value_t result;

  while (true)
  {
    jjsx_print_string (prompt_p);

    jjs_size_t length;
    jjs_char_t *line_p = jjs_port_line_read (&length);

    if (line_p == NULL)
    {
      jjsx_print_byte ('\n');
      return;
    }

    if (length == 0)
    {
      continue;
    }

    if (!jjs_validate_string (line_p, length, JJS_ENCODING_UTF8))
    {
      jjs_port_line_free (line_p);
      result = jjs_throw_sz (JJS_ERROR_SYNTAX, "Input is not a valid UTF-8 string");
      goto exception;
    }

    jjs_parse_options_t opts = {
      .options = JJS_PARSE_HAS_SOURCE_NAME,
      .source_name = jjs_string_sz ("<repl>"),
    };

    result = jjs_parse (line_p, length, &opts);
    jjs_port_line_free (line_p);

    jjs_value_free (opts.source_name);

    if (jjs_value_is_exception (result))
    {
      goto exception;
    }

    jjs_value_t script = result;
    result = jjs_run (script);
    jjs_value_free (script);

    if (jjs_value_is_exception (result))
    {
      goto exception;
    }

    jjs_value_t print_result = jjsx_print_value (result);
    jjs_value_free (result);
    result = print_result;

    if (jjs_value_is_exception (result))
    {
      goto exception;
    }

    jjsx_print_byte ('\n');

    jjs_value_free (result);
    result = jjs_run_jobs ();

    if (jjs_value_is_exception (result))
    {
      goto exception;
    }

    jjs_value_free (result);
    continue;

exception:
    jjsx_print_unhandled_exception (result);
  }
} /* jjsx_repl */
