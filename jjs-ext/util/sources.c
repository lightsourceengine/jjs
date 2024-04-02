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
jjsx_source_exec_snapshot (const char *path_p, size_t function_index)
{
  jjs_size_t source_size;
  jjs_char_t *source_p = jjs_port_source_read (path_p, &source_size);

  if (source_p == NULL)
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Failed to open file: %s\n", path_p);
    return jjs_throw_sz (JJS_ERROR_SYNTAX, "Snapshot file not found");
  }

  jjs_exec_snapshot_option_values_t opts = {
    .source_name = jjs_string_utf8_sz (path_p),
  };

  uint32_t snapshot_flags = JJS_SNAPSHOT_EXEC_COPY_DATA
                            | JJS_SNAPSHOT_EXEC_HAS_SOURCE_NAME;

  jjs_value_t result = jjs_exec_snapshot ((uint32_t *) source_p,
                                          source_size,
                                          function_index,
                                          snapshot_flags,
                                          &opts);

  jjs_port_source_free (source_p);
  jjs_value_free (opts.source_name);

  return result;
} /* jjsx_source_exec_snapshot */
