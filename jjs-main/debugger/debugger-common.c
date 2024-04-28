/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include <assert.h>

#include "debugger.h"
#include "jext-common.h"

/**
 * Must be called after the connection has been initialized.
 */
void
jjsx_debugger_after_connect (jjs_context_t* context_p, /**< JJS context */
                             bool success) /**< tells whether the connection has been successfully established */
{
#if defined(JJS_DEBUGGER) && (JJS_DEBUGGER == 1)
  if (success)
  {
    jjs_debugger_transport_start (context_p);
  }
  else
  {
    jjs_debugger_transport_close (context_p);
  }
#else /* !(defined (JJS_DEBUGGER) && (JJS_DEBUGGER == 1)) */
  JJSX_UNUSED (context_p);
  JJSX_UNUSED (success);
#endif /* defined (JJS_DEBUGGER) && (JJS_DEBUGGER == 1) */
} /* jjsx_debugger_after_connect */

/**
 * Check that value contains the reset abort value.
 *
 * Note: if the value is the reset abort value, the value is released.
 *
 * return true, if reset abort
 *        false, otherwise
 */
bool
jjsx_debugger_is_reset (jjs_context_t* context_p, /**< JJS context */
                        jjs_value_t value) /**< JJS value */
{
  if (!jjs_value_is_abort (context_p, value))
  {
    return false;
  }

  jjs_value_t abort_value = jjs_exception_value (context_p, value, false);

  if (!jjs_value_is_string (context_p, abort_value))
  {
    jjs_value_free (context_p, abort_value);
    return false;
  }

  static const char restart_str[] = "r353t";

  jjs_size_t str_size = jjs_string_size (context_p, abort_value, JJS_ENCODING_CESU8);
  bool is_reset = false;

  if (str_size == sizeof (restart_str) - 1)
  {
    JJS_VLA (jjs_char_t, str_buf, str_size);
    jjs_string_to_buffer (context_p, abort_value, JJS_ENCODING_CESU8, str_buf, str_size);

    is_reset = memcmp (restart_str, (char *) (str_buf), str_size) == 0;

    if (is_reset)
    {
      jjs_value_free (context_p, value);
    }
  }

  jjs_value_free (context_p, abort_value);
  return is_reset;
} /* jjsx_debugger_is_reset */
