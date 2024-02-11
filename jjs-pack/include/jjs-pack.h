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

#define JJS_PACK_INIT_ALL           (0xFFFFFFFF)
#define JJS_PACK_INIT_CONSOLE       (1u)
#define JJS_PACK_INIT_DOMEXCEPTION  (1u << 1)
#define JJS_PACK_INIT_FS            (1u << 2)
#define JJS_PACK_INIT_PATH          (1u << 3)
#define JJS_PACK_INIT_PATH_URL      (1u << 4)
#define JJS_PACK_INIT_PERFORMANCE   (1u << 5)
#define JJS_PACK_INIT_TEXT          (1u << 6)
#define JJS_PACK_INIT_URL           (1u << 7)

void jjs_pack_init (uint32_t init_flags);
jjs_value_t jjs_pack_init_with_result (uint32_t init_flags);
bool jjs_pack_is_initialized (uint32_t init_flags);

JJS_C_API_END

#endif /* !JJS_PACK_H */
