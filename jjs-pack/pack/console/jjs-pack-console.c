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

#if JJS_PACK_CONSOLE

extern uint8_t jjs_pack_console_snapshot[];
extern const uint32_t jjs_pack_console_snapshot_len;

static JJS_HANDLER (jjs_pack_console_println)
{
  JJS_HANDLER_HEADER ();
  jjs_context_t *context_p = call_info_p->context_p;

  if (args_cnt > 0 && jjs_value_is_string (context_p, args_p[0]))
  {
    jjs_platform_stdout_write (context_p, args_p[0], JJS_KEEP);
  }

  jjs_platform_stdout_write (context_p, jjs_string_sz(context_p, "\n"), JJS_MOVE);

  return jjs_undefined (call_info_p->context_p);
} /* jjs_pack_console_println */

#endif /* JJS_PACK_CONSOLE */

jjs_value_t
jjs_pack_console_init (jjs_context_t *context_p)
{
#if JJS_PACK_CONSOLE
  jjs_value_t bindings = jjs_bindings (context_p);

  jjs_bindings_function (context_p, bindings, "println", &jjs_pack_console_println);
  jjs_bindings_function (context_p, bindings, "hrtime", jjs_pack_hrtime_handler);

  return jjs_pack_lib_main (context_p, jjs_pack_console_snapshot, jjs_pack_console_snapshot_len, bindings, JJS_MOVE);
#else /* !JJS_PACK_CONSOLE */
  return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "console pack is not enabled");
#endif /* JJS_PACK_CONSOLE */
} /* jjs_pack_console_init */
