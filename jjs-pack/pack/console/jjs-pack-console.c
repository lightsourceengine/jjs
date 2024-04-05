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

#include <stdio.h>

extern uint8_t jjs_pack_console_snapshot[];
extern const uint32_t jjs_pack_console_snapshot_len;

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
      fwrite (&small_buffer[0], sizeof (small_buffer[0]), written, stdout);
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
        fwrite (&buffer_p, sizeof (buffer_p[0]), written, stdout);
      }

      jjs_heap_free (buffer_p, size);
    }
  }

  putc ('\n', stdout);
} /* println */

static JJS_HANDLER (jjs_pack_console_println)
{
  JJS_HANDLER_HEADER ();

  if (args_cnt > 0)
  {
    println (args_p[0]);
  }

  return jjs_undefined ();
} /* jjs_pack_console_println */

#endif /* JJS_PACK_CONSOLE */

jjs_value_t
jjs_pack_console_init (void)
{
#if JJS_PACK_CONSOLE
  jjs_value_t bindings = jjs_bindings ();

  jjs_bindings_function (bindings, "println", &jjs_pack_console_println);
  jjs_bindings_function (bindings, "hrtime", jjs_pack_hrtime_handler);

  return jjs_pack_lib_main (jjs_pack_console_snapshot, jjs_pack_console_snapshot_len, bindings, JJS_MOVE);
#else /* !JJS_PACK_CONSOLE */
  return jjs_throw_sz (JJS_ERROR_COMMON, "console pack is not enabled");
#endif /* JJS_PACK_CONSOLE */
} /* jjs_pack_console_init */
