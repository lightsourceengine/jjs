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

#include "jrt.h"

static jjs_log_level_t jjs_log_level = JJS_LOG_LEVEL_ERROR;

/**
 * Get current log level
 *
 * @return log level
 */
jjs_log_level_t
jjs_jrt_get_log_level (void)
{
  return jjs_log_level;
} /* jjs_jrt_get_log_level */

/**
 * Set log level
 *
 * @param level: new log level
 */
void
jjs_jrt_set_log_level (jjs_log_level_t level)
{
  jjs_log_level = level;
} /* jjs_jrt_set_log_level */
