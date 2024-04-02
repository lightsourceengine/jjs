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
#include "jjs.h"

#include "jjs-ext/print.h"

jjs_value_t
jjsx_source_parse_script (jjs_value_t path)
{
  jjs_platform_read_file_options_t options = {
    .encoding = JJS_ENCODING_UTF8,
  };

  jjs_value_t source = jjs_platform_read_file (path, JJS_KEEP, &options);

  if (jjs_value_is_exception (source))
  {
    jjs_value_free (source);
//    jjs_log (JJS_LOG_LEVEL_ERROR, "Failed to open file: %j", path);
    return jjs_throw_sz (JJS_ERROR_SYNTAX, "Source file not found");
  }

  jjs_parse_options_t parse_options = {
    .options = JJS_PARSE_HAS_SOURCE_NAME | JJS_PARSE_HAS_USER_VALUE,
    .user_value = path,
    .source_name = path,
  };

  jjs_value_t result = jjs_parse_value (source, &parse_options);

  jjs_value_free (source);

  return result;
} /* jjsx_source_parse_script */

jjs_value_t
jjsx_source_exec_snapshot (jjs_value_t path, size_t function_index)
{
  jjs_value_t source = jjs_platform_read_file(path, JJS_KEEP, NULL);

  if (jjs_value_is_exception (source))
  {
    jjs_value_free (source);
//    jjs_log (JJS_LOG_LEVEL_ERROR, "Failed to open file: %j", path);
    return jjs_throw_sz (JJS_ERROR_SYNTAX, "Snapshot file not found");
  }

  jjs_exec_snapshot_option_values_t opts = {
    .source_name = path,
  };

  uint32_t snapshot_flags = JJS_SNAPSHOT_EXEC_COPY_DATA
                            | JJS_SNAPSHOT_EXEC_HAS_SOURCE_NAME;

  jjs_value_t result = jjs_exec_snapshot ((uint32_t *) jjs_arraybuffer_data (source),
                                          jjs_array_length (source),
                                          function_index,
                                          snapshot_flags,
                                          &opts);

  jjs_value_free (source);
  jjs_value_free (opts.source_name);

  return result;
} /* jjsx_source_exec_snapshot */
