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

#ifndef JJSX_SOURCES_H
#define JJSX_SOURCES_H

#include "jjs-types.h"

JJS_C_API_BEGIN

jjs_value_t jjsx_source_parse_script (const char* path);
jjs_value_t jjsx_source_exec_script (const char* path);
jjs_value_t jjsx_source_exec_module (const char* path);
jjs_value_t jjsx_source_exec_snapshot (const char* path, size_t function_index);
jjs_value_t jjsx_source_exec_stdin (void);

JJS_C_API_END

#endif /* !JJSX_EXEC_H */
