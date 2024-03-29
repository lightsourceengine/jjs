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
static double time_origin = 0;
static uint64_t performance_now_time_origin = 0;

static JJS_HANDLER (jjs_pack_performance_now)
{
  JJS_HANDLER_HEADER ();
  return jjs_number ((double) (jjs_port_hrtime () - performance_now_time_origin) / 1e6);
} /* jjs_pack_performance_now */

#endif /* JJS_PACK_PERFORMANCE */

jjs_value_t
jjs_pack_performance_init (void)
{
#if JJS_PACK_PERFORMANCE
  if (performance_now_time_origin == 0)
  {
    jjs_port_hrtime ();
    time_origin = jjs_port_current_time ();
    performance_now_time_origin = jjs_port_hrtime ();
  }

  jjs_value_t bindings = jjs_bindings ();

  jjs_bindings_function (bindings, "now", &jjs_pack_performance_now);
  jjs_bindings_number (bindings, "timeOrigin", time_origin);

  return jjs_pack_lib_main (jjs_pack_performance_snapshot, jjs_pack_performance_snapshot_len, bindings, true);
#else /* !JJS_PACK_PERFORMANCE */
  return jjs_throw_sz (JJS_ERROR_COMMON, "performance pack is not enabled");
#endif /* JJS_PACK_PERFORMANCE */
} /* jjs_pack_performance_init */
