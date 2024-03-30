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

#ifndef JJS_PLATFORM_H
#define JJS_PLATFORM_H

#include "jjs-core.h"
#include "ecma-globals.h"

jjs_platform_t jjsp_defaults (void);

void jjsp_buffer_free (jjs_platform_buffer_t* buffer_p);
char* jjsp_strndup (char* str_p, uint32_t length);
ecma_value_t jjsp_buffer_to_string_value (jjs_platform_buffer_t* buffer_p, bool move);

jjs_platform_status_t jjsp_cwd (jjs_platform_buffer_t* buffer_p);

#endif /* JJS_PLATFORM_H */