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

#include "jjs-pack.h"
#include "jjs-pack-lib.h"
#include "jjs-port.h"

#if JJS_PACK_PERFORMANCE

JJS_PACK_DEFINE_EXTERN_SOURCE (jjs_pack_performance)
static double time_origin = 0;
static uint64_t performance_now_time_origin = 0;

static JJS_HANDLER (jjs_pack_performance_now)
{
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_cnt);
  JJS_UNUSED (args_p);

  return jjs_number ((double)(jjs_port_hrtime() - performance_now_time_origin) / 1e6);
} /* jjs_pack_performance_now */

static jjs_value_t
jjs_pack_performance_bindings (void)
{
  jjs_value_t bindings = jjs_object ();
  jjs_value_t now = jjs_function_external (jjs_pack_performance_now);

  jjs_value_free (jjs_object_set_sz (bindings, "now", now));
  jjs_value_free (now);

  jjs_value_t time_origin_value = jjs_number (time_origin);
  jjs_value_free (jjs_object_set_sz (bindings, "timeOrigin", time_origin_value));
  jjs_value_free (time_origin_value);

  return bindings;
} /* jjs_pack_performance_bindings */

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

  return JJS_PACK_LIB_GLOBAL_SET ("performance", jjs_pack_performance, &jjs_pack_performance_bindings);
#else /* !JJS_PACK_PERFORMANCE */
  return jjs_throw_sz (JJS_ERROR_COMMON, "performance pack is not enabled");
#endif /* JJS_PACK_PERFORMANCE */
} /* jjs_pack_performance_init */
