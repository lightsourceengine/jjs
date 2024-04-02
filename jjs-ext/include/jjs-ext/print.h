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

#ifndef JJSX_PRINT_H
#define JJSX_PRINT_H

#include "jjs-types.h"

JJS_C_API_BEGIN

jjs_value_t jjsx_print_value (const jjs_value_t value);
void jjsx_print_byte (jjs_char_t ch);
void jjsx_print_buffer (const jjs_char_t *buffer_p, jjs_size_t buffer_size);
void jjsx_print_string (const char *str_p);
void jjsx_print_backtrace (unsigned depth);
void jjsx_print_unhandled_exception (jjs_value_t exception);
void jjsx_print_unhandled_rejection (jjs_value_t exception);

JJS_C_API_END

#endif /* !JJSX_PRINT_H */
