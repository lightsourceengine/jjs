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

#include "jjs-ext/sources.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "jjs-debugger.h"
#include "jjs-port.h"
#include "jjs.h"

#include "jjs-ext/print.h"

jjs_value_t
jjsx_source_parse_script (const char *path_p)
{
  jjs_size_t source_size;
  jjs_char_t *source_p = jjs_port_source_read (path_p, &source_size);

  if (source_p == NULL)
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Failed to open file: %s\n", path_p);
    return jjs_throw_sz (JJS_ERROR_SYNTAX, "Source file not found");
  }

  if (!jjs_validate_string (source_p, source_size, JJS_ENCODING_UTF8))
  {
    jjs_port_source_free (source_p);
    return jjs_throw_sz (JJS_ERROR_SYNTAX, "Input is not a valid UTF-8 encoded string.");
  }

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name =
    jjs_string ((const jjs_char_t *) path_p, (jjs_size_t) strlen (path_p), JJS_ENCODING_UTF8);

  jjs_value_t result = jjs_parse (source_p, source_size, &parse_options);

  jjs_value_free (parse_options.source_name);
  jjs_port_source_free (source_p);

  return result;
} /* jjsx_source_parse_script */

jjs_value_t
jjsx_source_exec_script (const char *path_p)
{
  jjs_value_t result = jjsx_source_parse_script (path_p);

  if (!jjs_value_is_exception (result))
  {
    jjs_value_t script = result;
    result = jjs_run (script);
    jjs_value_free (script);
  }

  return result;
} /* jjsx_source_exec_script */

jjs_value_t
jjsx_source_exec_module (const char *path_p)
{
  jjs_value_t specifier =
    jjs_string ((const jjs_char_t *) path_p, (jjs_size_t) strlen (path_p), JJS_ENCODING_UTF8);
  jjs_value_t referrer = jjs_undefined ();

  jjs_value_t module = jjs_module_resolve (specifier, referrer, NULL);

  jjs_value_free (referrer);
  jjs_value_free (specifier);

  if (jjs_value_is_exception (module))
  {
    return module;
  }

  if (jjs_module_state (module) == JJS_MODULE_STATE_UNLINKED)
  {
    jjs_value_t link_result = jjs_module_link (module, NULL, NULL);

    if (jjs_value_is_exception (link_result))
    {
      jjs_value_free (module);
      return link_result;
    }

    jjs_value_free (link_result);
  }

  jjs_value_t result = jjs_module_evaluate (module);
  jjs_value_free (module);

  jjs_module_cleanup (jjs_undefined ());
  return result;
} /* jjsx_source_exec_module */

jjs_value_t
jjsx_source_exec_snapshot (const char *path_p, size_t function_index)
{
  jjs_size_t source_size;
  jjs_char_t *source_p = jjs_port_source_read (path_p, &source_size);

  if (source_p == NULL)
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Failed to open file: %s\n", path_p);
    return jjs_throw_sz (JJS_ERROR_SYNTAX, "Snapshot file not found");
  }

  // set user_value and source_name to a canonical file path so import and require work
  jjs_char_t* normalized_p = jjs_port_path_normalize((jjs_char_t *) path_p, (jjs_size_t) strlen (path_p));
  jjs_value_t path;

  if (normalized_p)
  {
    path = jjs_string (normalized_p, (jjs_size_t) strlen ((const char*)normalized_p), JJS_ENCODING_UTF8);
    jjs_port_path_free (normalized_p);
  }
  else
  {
    // fallback to original path
    path = jjs_string_sz (path_p);
  }

  jjs_exec_snapshot_option_values_t opts = {
    .user_value = path,
    .source_name = path,
  };

  uint32_t snapshot_flags = JJS_SNAPSHOT_EXEC_COPY_DATA
                            | JJS_SNAPSHOT_EXEC_HAS_USER_VALUE
                            | JJS_SNAPSHOT_EXEC_HAS_SOURCE_NAME;

  jjs_value_t result = jjs_exec_snapshot ((uint32_t *) source_p,
                                          source_size,
                                          function_index,
                                          snapshot_flags,
                                          &opts);

  jjs_port_source_free (source_p);
  jjs_value_free (path);

  return result;
} /* jjsx_source_exec_snapshot */

jjs_value_t
jjsx_source_exec_stdin (void)
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

  jjs_value_t result = jjs_parse (source_p, source_size, NULL);
  free (source_p);

  if (jjs_value_is_exception (result))
  {
    return result;
  }

  jjs_value_t script = result;
  result = jjs_run (script);
  jjs_value_free (script);

  return result;
} /* jjsx_source_exec_stdin */
