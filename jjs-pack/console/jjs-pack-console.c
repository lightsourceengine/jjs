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

#if JJS_PACK_CONSOLE

JJS_PACK_DEFINE_EXTERN_SOURCE (jjs_pack_console)
static uint64_t console_now_time_origin = 0;

static void
println (jjs_value_t value)
{
  jjs_size_t size = jjs_string_size (value, JJS_ENCODING_UTF8);

  if (size < 256)
  {
    uint8_t small_buffer[256];
    jjs_size_t written = jjs_string_to_buffer (value,
                                               JJS_ENCODING_UTF8,
                                               &small_buffer[0],
                                               (jjs_size_t)(sizeof (small_buffer) / sizeof (small_buffer[0])));

    if (written > 0)
    {
      jjs_port_print_buffer (&small_buffer[0], written);
    }
  }
  else
  {
    uint8_t* buffer_p = jjs_heap_alloc (size);

    if (buffer_p)
    {
      jjs_size_t written = jjs_string_to_buffer (value, JJS_ENCODING_UTF8, buffer_p, size);

      if (written > 0)
      {
        jjs_port_print_buffer (buffer_p, written);
      }

      jjs_heap_free (buffer_p, size);
    }
  }

  jjs_port_print_byte ('\n');
} /* println */

static JJS_HANDLER (jjs_pack_console_println)
{
  JJS_UNUSED (call_info_p);

  if (args_cnt > 0)
  {
    println (args_p[0]);
  }

  return jjs_undefined ();
} /* jjs_pack_console_println */

static JJS_HANDLER (jjs_pack_console_now)
{
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_cnt);
  JJS_UNUSED (args_p);

  return jjs_number ((double)(jjs_port_hrtime() - console_now_time_origin) / 1e6);
} /* jjs_pack_console_now */

static jjs_value_t
jjs_pack_console_bindings (void)
{
  jjs_value_t bindings = jjs_object ();

  jjs_pack_lib_set_function_sz (bindings, "println", jjs_pack_console_println);
  jjs_pack_lib_set_function_sz(bindings, "now", jjs_pack_console_now);

  return bindings;
} /* jjs_pack_console_bindings */

#endif /* JJS_PACK_CONSOLE */

jjs_value_t
jjs_pack_console_init (void)
{
#if JJS_PACK_CONSOLE
  if (console_now_time_origin == 0)
  {
    console_now_time_origin = jjs_port_hrtime ();
  }

  return JJS_PACK_LIB_GLOBAL_SET ("console", jjs_pack_console, &jjs_pack_console_bindings);
#else /* !JJS_PACK_CONSOLE */
  return jjs_throw_sz (JJS_ERROR_COMMON, "console pack is not enabled");
#endif /* JJS_PACK_CONSOLE */
} /* jjs_pack_console_init */