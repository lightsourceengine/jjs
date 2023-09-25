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
#include "jjs-ext/print.h"

/**
 * Assert support adapted from ECMA 262 test harness (assert.js). It
 * add assert() and Test262Error() to the global object.
 *
 * Note: Embedding the js code is way easier than implementing it in C.
 *
 * Credit (test262/esnext/harness/assert.js):
 * Copyright (C) 2017 Ecma International.  All rights reserved.
 * This code is governed by the BSD license found in the LICENSE file.
 */
static const char* test262_assert_lib =
  "globalThis.Test262Error = function (id, path, description, \n"
  "    codeString, preconditionString, result, error) {\n"
  "  this.id = id;\n"
  "  this.path = path;\n"
  "  this.description = description;\n"
  "  this.result = result;\n"
  "  this.error = error;\n"
  "  this.code = codeString;\n"
  "  this.pre = preconditionString;\n"
  "};\n"
  "\n"
  "globalThis.Test262Error.prototype.toString = function() {\n"
  "  return this.result + ' ' + this.error;\n"
  "};\n"
  "\n"
  "globalThis.assert = function (mustBeTrue, message) {\n"
  "  if (mustBeTrue === true) {\n"
  "    return;\n"
  "  }\n"
  "\n"
  "  if (message === undefined) {\n"
  "    message = 'Expected true but got ' + assert._toString(mustBeTrue);\n"
  "  }\n"
  "  throw new Test262Error(message);\n"
  "};\n"
  "\n"
  "globalThis.assert._isSameValue = function (a, b) {\n"
  "  if (a === b) {\n"
  "    // Handle +/-0 vs. -/+0\n"
  "    return a !== 0 || 1 / a === 1 / b;\n"
  "  }\n"
  "\n"
  "  // Handle NaN vs. NaN\n"
  "  return a !== a && b !== b;\n"
  "};\n"
  "\n"
  "globalThis.assert.sameValue = function (actual, expected, message) {\n"
  "  try {\n"
  "    if (assert._isSameValue(actual, expected)) {\n"
  "      return;\n"
  "    }\n"
  "  } catch (error) {\n"
  "    throw new Test262Error(message + ' (_isSameValue operation threw) ' + error);\n"
  "    return;\n"
  "  }\n"
  "\n"
  "  if (message === undefined) {\n"
  "    message = '';\n"
  "  } else {\n"
  "    message += ' ';\n"
  "  }\n"
  "\n"
  "  message += 'Expected SameValue(«' + assert._toString(actual) + '», «' + assert._toString(expected) + '») to be true';\n"
  "\n"
  "  throw new Test262Error(message);\n"
  "};\n"
  "\n"
  "globalThis.assert.notSameValue = function (actual, unexpected, message) {\n"
  "  if (!assert._isSameValue(actual, unexpected)) {\n"
  "    return;\n"
  "  }\n"
  "\n"
  "  if (message === undefined) {\n"
  "    message = '';\n"
  "  } else {\n"
  "    message += ' ';\n"
  "  }\n"
  "\n"
  "  message += 'Expected SameValue(«' + assert._toString(actual) + '», «' + assert._toString(unexpected) + '») to be false';\n"
  "\n"
  "  throw new Test262Error(message);\n"
  "};\n"
  "\n"
  "globalThis.assert.throws = function (expectedErrorConstructor, func, message) {\n"
  "  var expectedName, actualName;\n"
  "  if (typeof func !== \"function\") {\n"
  "    throw new Test262Error('assert.throws requires two arguments: the error constructor ' +\n"
  "      'and a function to run');\n"
  "    return;\n"
  "  }\n"
  "  if (message === undefined) {\n"
  "    message = '';\n"
  "  } else {\n"
  "    message += ' ';\n"
  "  }\n"
  "\n"
  "  try {\n"
  "    func();\n"
  "  } catch (thrown) {\n"
  "    if (typeof thrown !== 'object' || thrown === null) {\n"
  "      message += 'Thrown value was not an object!';\n"
  "      throw new Test262Error(message);\n"
  "    } else if (thrown.constructor !== expectedErrorConstructor) {\n"
  "      expectedName = expectedErrorConstructor.name;\n"
  "      actualName = thrown.constructor.name;\n"
  "      if (expectedName === actualName) {\n"
  "        message += 'Expected a ' + expectedName + ' but got a different error constructor with the same name';\n"
  "      } else {\n"
  "        message += 'Expected a ' + expectedName + ' but got a ' + actualName;\n"
  "      }\n"
  "      throw new Test262Error(message);\n"
  "    }\n"
  "    return;\n"
  "  }\n"
  "\n"
  "  message += 'Expected a ' + expectedErrorConstructor.name + ' to be thrown but no exception was thrown at all';\n"
  "  throw new Test262Error(message);\n"
  "};\n"
  "\n"
  "globalThis.assert._toString = function (value) {\n"
  "  try {\n"
  "    if (value === 0 && 1 / value === -Infinity) {\n"
  "      return '-0';\n"
  "    }\n"
  "\n"
  "    return String(value);\n"
  "  } catch (err) {\n"
  "    if (err.name === 'TypeError') {\n"
  "      return Object.prototype.toString.call(value);\n"
  "    }\n"
  "\n"
  "    throw err;\n"
  "  }\n"
  "};";

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

void jjsx_test262_register_assert(void) {
  jjs_value_t result = jjs_eval ((const jjs_char_t *) test262_assert_lib, strlen (test262_assert_lib), 0);

  if (jjs_value_is_exception (result)) {
    uint8_t buffer[256];
    jjs_value_t value = jjs_exception_value (result, false);
    jjs_value_t string = jjs_value_to_string(value);
    jjs_size_t written = jjs_string_to_buffer (string, JJS_ENCODING_UTF8, buffer, sizeof(buffer) - 1);

    jjs_value_free (string);
    jjs_value_free (value);

    buffer[written] = '\0';

    jjs_log(JJS_LOG_LEVEL_ERROR, "Failed to register test262 assert library: %s", buffer);
    jjsx_print_backtrace (5);
    jjs_port_fatal (JJS_FATAL_FAILED_ASSERTION);
  }

  jjs_value_free (result);
} /* jjsx_test262_register_assert */
