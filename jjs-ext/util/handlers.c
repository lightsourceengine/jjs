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

#include "jjs-ext/handlers.h"

#include "jjs-core.h"
#include "jjs-debugger.h"
#include "jjs-port.h"

#include "jjs-ext/print.h"

#include <string.h>

/**
 * Provide a 'print' implementation for scripts.
 *
 * The routine converts all of its arguments to strings and outputs them
 * char-by-char using jjs_port_print_byte.
 *
 * The NUL character is output as "\u0000", other characters are output
 * bytewise.
 *
 * Note:
 *      This implementation does not use standard C `printf` to print its
 *      output. This allows more flexibility but also extends the core
 *      JJS engine port API. Applications that want to use
 *      `jjsx_handler_print` must ensure that their port implementation also
 *      provides `jjs_port_print_byte`.
 *
 * @return undefined - if all arguments could be converted to strings,
 *         error - otherwise.
 */
jjs_value_t
jjsx_handler_print (const jjs_call_info_t *call_info_p, /**< call information */
                      const jjs_value_t args_p[], /**< function arguments */
                      const jjs_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  for (jjs_length_t index = 0; index < args_cnt; index++)
  {
    if (index > 0)
    {
      jjsx_print_byte (' ');
    }

    jjs_value_t result = jjsx_print_value (args_p[index]);

    if (jjs_value_is_exception (result))
    {
      return result;
    }
  }

  jjsx_print_byte ('\n');
  return jjs_undefined ();
} /* jjsx_handler_print */

/**
 * Hard assert for scripts. The routine calls jjs_port_fatal on assertion failure.
 *
 * Notes:
 *  * If the `JJS_FEATURE_LINE_INFO` runtime feature is enabled (build option: `JJS_LINE_INFO`)
 *    a backtrace is also printed out.
 *
 * @return true - if only one argument was passed and that argument was a boolean true.
 *         Note that the function does not return otherwise.
 */
jjs_value_t
jjsx_handler_assert (const jjs_call_info_t *call_info_p, /**< call information */
                       const jjs_value_t args_p[], /**< function arguments */
                       const jjs_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  if (args_cnt > 0 && jjs_value_is_true (args_p[0]))
  {
    return jjs_boolean (true);
  }

  if (args_cnt > 1 && jjs_value_is_string (args_p[1]))
  {
    uint8_t buffer[256];

    jjs_size_t written = jjs_string_to_buffer (args_p[1], JJS_ENCODING_UTF8, buffer, sizeof (buffer) - 1);
    buffer[written] = '\0';

    jjs_log (JJS_LOG_LEVEL_ERROR, "Script Error: assertion failed: %s\n", buffer);
  }
  else
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Script Error: assertion failed\n");
  }

  /* Assert failed, print a bit of JS backtrace */

  jjsx_print_backtrace (5);

  jjs_port_fatal (JJS_FATAL_FAILED_ASSERTION);
} /* jjsx_handler_assert_fatal */

/**
 * Expose garbage collector to scripts.
 *
 * @return undefined.
 */
jjs_value_t
jjsx_handler_gc (const jjs_call_info_t *call_info_p, /**< call information */
                   const jjs_value_t args_p[], /**< function arguments */
                   const jjs_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  jjs_gc_mode_t mode =
    ((args_cnt > 0 && jjs_value_to_boolean (args_p[0])) ? JJS_GC_PRESSURE_HIGH : JJS_GC_PRESSURE_LOW);

  jjs_heap_gc (mode);
  return jjs_undefined ();
} /* jjsx_handler_gc */

/**
 * Get the resource name (usually a file name) of the currently executed script or the given function object
 *
 * Note: returned value must be freed with jjs_value_free, when it is no longer needed
 *
 * @return JS string constructed from
 *         - the currently executed function object's resource name, if the given value is undefined
 *         - resource name of the function object, if the given value is a function object
 *         - "<anonymous>", otherwise
 */
jjs_value_t
jjsx_handler_source_name (const jjs_call_info_t *call_info_p, /**< call information */
                            const jjs_value_t args_p[], /**< function arguments */
                            const jjs_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  jjs_value_t undefined_value = jjs_undefined ();
  jjs_value_t source_name = jjs_source_name (args_cnt > 0 ? args_p[0] : undefined_value);
  jjs_value_free (undefined_value);

  return source_name;
} /* jjsx_handler_source_name */

/**
 * Create a new realm.
 *
 * @return new Realm object
 */
jjs_value_t
jjsx_handler_create_realm (const jjs_call_info_t *call_info_p, /**< call information */
                             const jjs_value_t args_p[], /**< function arguments */
                             const jjs_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */
  (void) args_p; /* unused */
  (void) args_cnt; /* unused */
  return jjs_realm ();
} /* jjsx_handler_create_realm */

/**
 * Handler for unhandled promise rejection events.
 */
void
jjsx_handler_promise_reject (jjs_promise_event_type_t event_type, /**< event type */
                               const jjs_value_t object, /**< target object */
                               const jjs_value_t value, /**< optional argument */
                               void *user_p) /**< user pointer passed to the callback */
{
  (void) value;
  (void) user_p;

  if (event_type != JJS_PROMISE_EVENT_REJECT_WITHOUT_HANDLER)
  {
    return;
  }

  jjs_value_t result = jjs_promise_result (object);
  jjsx_print_unhandled_rejection (result);

  jjs_value_free (result);
} /* jjsx_handler_promise_reject */

/**
 * Runs the source code received by jjs_debugger_wait_for_client_source.
 *
 * @return result fo the source code execution
 */
jjs_value_t
jjsx_handler_source_received (const jjs_char_t *source_name_p, /**< resource name */
                                size_t source_name_size, /**< size of resource name */
                                const jjs_char_t *source_p, /**< source code */
                                size_t source_size, /**< source code size */
                                void *user_p) /**< user pointer */
{
  (void) user_p; /* unused */

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jjs_string (source_name_p, (jjs_size_t) source_name_size, JJS_ENCODING_UTF8);

  jjs_value_t ret_val = jjs_parse (source_p, source_size, &parse_options);

  jjs_value_free (parse_options.source_name);

  if (!jjs_value_is_exception (ret_val))
  {
    jjs_value_t func_val = ret_val;
    ret_val = jjs_run (func_val);
    jjs_value_free (func_val);
  }

  return ret_val;
} /* jjsx_handler_source_received */
