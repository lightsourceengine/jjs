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

#ifndef JJSX_HANDLERS_H
#define JJSX_HANDLERS_H

#include "jjs-types.h"

JJS_C_API_BEGIN

jjs_value_t jjsx_handler_assert (const jjs_call_info_t *call_info_p,
                                     const jjs_value_t args_p[],
                                     const jjs_length_t args_cnt);
jjs_value_t
jjsx_handler_gc (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt);
jjs_value_t jjsx_handler_print (const jjs_call_info_t *call_info_p,
                                    const jjs_value_t args_p[],
                                    const jjs_length_t args_cnt);
jjs_value_t jjsx_handler_source_name (const jjs_call_info_t *call_info_p,
                                          const jjs_value_t args_p[],
                                          const jjs_length_t args_cnt);
jjs_value_t jjsx_handler_create_realm (const jjs_call_info_t *call_info_p,
                                           const jjs_value_t args_p[],
                                           const jjs_length_t args_cnt);
void jjsx_handler_promise_reject (jjs_promise_event_type_t event_type,
                                    const jjs_value_t object,
                                    const jjs_value_t value,
                                    void *user_p);
jjs_value_t jjsx_handler_source_received (const jjs_char_t *source_name_p,
                                              size_t source_name_size,
                                              const jjs_char_t *source_p,
                                              size_t source_size,
                                              void *user_p);
JJS_C_API_END

#endif /* !JJSX_HANDLERS_H */
