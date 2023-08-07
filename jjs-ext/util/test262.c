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

#include "jjs-ext/test262.h"

#include <assert.h>
#include <string.h>

#include "jjs.h"

#include "jjs-ext/handlers.h"

/**
 * Register a method for the $262 object.
 */
static void
jjsx_test262_register_function (jjs_value_t test262_obj, /** $262 object */
                                  const char *name_p, /**< name of the function */
                                  jjs_external_handler_t handler_p) /**< function callback */
{
  jjs_value_t function_val = jjs_function_external (handler_p);
  jjs_value_t result_val = jjs_object_set_sz (test262_obj, name_p, function_val);
  jjs_value_free (function_val);

  assert (!jjs_value_is_exception (result_val));
  jjs_value_free (result_val);
} /* jjsx_test262_register_function */

/**
 * $262.detachArrayBuffer
 *
 * A function which implements the DetachArrayBuffer abstract operation
 *
 * @return null value - if success
 *         value marked with error flag - otherwise
 */
static jjs_value_t
jjsx_test262_detach_array_buffer (const jjs_call_info_t *call_info_p, /**< call information */
                                    const jjs_value_t args_p[], /**< function arguments */
                                    const jjs_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  if (args_cnt < 1 || !jjs_value_is_arraybuffer (args_p[0]))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Expected an ArrayBuffer object");
  }

  /* TODO: support the optional 'key' argument */

  return jjs_arraybuffer_detach (args_p[0]);
} /* jjsx_test262_detach_array_buffer */

/**
 * $262.evalScript
 *
 * A function which accepts a string value as its first argument and executes it
 *
 * @return completion of the script parsing and execution.
 */
static jjs_value_t
jjsx_test262_eval_script (const jjs_call_info_t *call_info_p, /**< call information */
                            const jjs_value_t args_p[], /**< function arguments */
                            const jjs_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  if (args_cnt < 1 || !jjs_value_is_string (args_p[0]))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Expected a string");
  }

  jjs_value_t ret_value = jjs_parse_value (args_p[0], NULL);

  if (!jjs_value_is_exception (ret_value))
  {
    jjs_value_t func_val = ret_value;
    ret_value = jjs_run (func_val);
    jjs_value_free (func_val);
  }

  return ret_value;
} /* jjsx_test262_eval_script */

static jjs_value_t jjsx_test262_create (jjs_value_t global_obj);

/**
 * $262.createRealm
 *
 * A function which creates a new realm object, and returns a newly created $262 object
 *
 * @return a new $262 object
 */
static jjs_value_t
jjsx_test262_create_realm (const jjs_call_info_t *call_info_p, /**< call information */
                             const jjs_value_t args_p[], /**< function arguments */
                             const jjs_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */
  (void) args_p; /* unused */
  (void) args_cnt; /* unused */

  jjs_value_t realm_object = jjs_realm ();
  jjs_value_t previous_realm = jjs_set_realm (realm_object);
  assert (!jjs_value_is_exception (previous_realm));

  jjs_value_t test262_object = jjsx_test262_create (realm_object);
  jjs_set_realm (previous_realm);
  jjs_value_free (realm_object);

  return test262_object;
} /* jjsx_test262_create_realm */

/**
 * Create a new $262 object
 *
 * @return a new $262 object
 */
static jjs_value_t
jjsx_test262_create (jjs_value_t global_obj) /**< global object */
{
  jjs_value_t test262_object = jjs_object ();

  jjsx_test262_register_function (test262_object, "detachArrayBuffer", jjsx_test262_detach_array_buffer);
  jjsx_test262_register_function (test262_object, "evalScript", jjsx_test262_eval_script);
  jjsx_test262_register_function (test262_object, "createRealm", jjsx_test262_create_realm);
  jjsx_test262_register_function (test262_object, "gc", jjsx_handler_gc);

  jjs_value_t result = jjs_object_set_sz (test262_object, "global", global_obj);
  assert (!jjs_value_is_exception (result));
  jjs_value_free (result);

  return test262_object;
} /* create_test262 */

/**
 * Add a new test262 object to the current global object.
 */
void
jjsx_test262_register (void)
{
  jjs_value_t global_obj = jjs_current_realm ();
  jjs_value_t test262_obj = jjsx_test262_create (global_obj);

  jjs_value_t result = jjs_object_set_sz (global_obj, "$262", test262_obj);
  assert (!jjs_value_is_exception (result));

  jjs_value_free (result);
  jjs_value_free (test262_obj);
  jjs_value_free (global_obj);
} /* jjsx_test262_register */
