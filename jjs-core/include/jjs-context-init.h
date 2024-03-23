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

#ifndef JJS_CONTEXT_INIT_H
#define JJS_CONTEXT_INIT_H

#include "jjs-types.h"

void jjs_context_init (jjs_init_flag_t flags, jjs_init_options_t* options_p);
void jjs_context_cleanup (void);

#endif /* JJS_CONTEXT_INIT_H */
