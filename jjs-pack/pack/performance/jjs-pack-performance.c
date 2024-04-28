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

#include "jjs-pack-lib.h"
#include "jjs-pack.h"
#include "jjs-port.h"

#if JJS_PACK_PERFORMANCE

extern uint8_t jjs_pack_performance_snapshot[];
extern const uint32_t jjs_pack_performance_snapshot_len;

#endif /* JJS_PACK_PERFORMANCE */

jjs_value_t
jjs_pack_performance_init (jjs_context_t *context_p)
{
#if JJS_PACK_PERFORMANCE
  jjs_value_t bindings = jjs_bindings (context_p);

  jjs_bindings_function (context_p, bindings, "hrtime", jjs_pack_hrtime_handler);
  jjs_bindings_function (context_p, bindings, "DateNow", jjs_pack_date_now_handler);

  return jjs_pack_lib_main (context_p, jjs_pack_performance_snapshot, jjs_pack_performance_snapshot_len, bindings, JJS_MOVE);
#else /* !JJS_PACK_PERFORMANCE */
  return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "performance pack is not enabled");
#endif /* JJS_PACK_PERFORMANCE */
} /* jjs_pack_performance_init */
