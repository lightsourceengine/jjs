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

#ifndef MAIN_MODULE_H
#define MAIN_MODULE_H

#include "jjs.h"
#include "arguments/options.h"

char* main_realpath (const char* path_p);
void main_realpath_free (char* path_p);
jjs_value_t main_module_run (const char* path_p);

void main_register_jjs_test_object (void);
jjs_value_t main_exec_stdin (main_input_type_t input_type, const char* filename);

#define JJS_HANDLER(NAME) jjs_value_t NAME (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_cnt)

#endif /* !MAIN_MODULE_H */
