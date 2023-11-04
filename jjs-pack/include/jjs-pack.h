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

#ifndef JJS_PACK_H
#define JJS_PACK_H

#include "jjs.h"
#include "jjs-pack-config.h"

JJS_C_API_BEGIN

jjs_value_t jjs_pack_init(void);

jjs_value_t jjs_pack_console_init(void);
jjs_value_t jjs_pack_domexception_init(void);
jjs_value_t jjs_pack_path_init(void);
jjs_value_t jjs_pack_path_url_init (void);
jjs_value_t jjs_pack_performance_init(void);
jjs_value_t jjs_pack_text_init(void);
jjs_value_t jjs_pack_url_init(void);

JJS_C_API_END

#endif /* !JJS_PACK_H */
