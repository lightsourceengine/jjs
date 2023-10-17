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

#ifndef JJS_ANNEX_H
#define JJS_ANNEX_H

#include "ecma-builtins.h"

#define JJS_HANDLER(NAME) jjs_value_t NAME (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], uint32_t args_count)

void jjs_annex_init (void);
void jjs_annex_init_realm (ecma_global_object_t* global_p);
void jjs_annex_finalize (void);

jjs_value_t jjs_annex_create_require (jjs_value_t referrer);
jjs_value_t jjs_annex_pmap_resolve (jjs_value_t specifier, jjs_module_type_t module_type);

JJS_HANDLER(queue_microtask_handler);

/**
 * Assert that it is correct to call API in current state.
 *
 * Note:
 *         By convention, there are some states when API could not be invoked.
 *
 *         The API can be and only be invoked when the ECMA_STATUS_API_ENABLED
 *         flag is set.
 *
 *         This procedure checks whether the API is available, and terminates
 *         the engine if it is unavailable. Otherwise it is a no-op.
 *
 * Note:
 *         The API could not be invoked in the following cases:
 *           - before jjs_init and after jjs_cleanup
 *           - between enter to and return from a native free callback
 */
#define jjs_assert_api_enabled() JJS_ASSERT (JJS_CONTEXT (status_flags) & ECMA_STATUS_API_ENABLED)

#endif // JJS_ANNEX_H
