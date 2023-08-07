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

#include "jjs.h"

#include "test-common.h"

static const char instanceof_source[] = "var x = function(o, c) {return (o instanceof c);}; x";

static jjs_value_t
external_function (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_size_t args_count)
{
  (void) call_info_p;
  (void) args_p;
  (void) args_count;

  return jjs_undefined ();
} /* external_function */

static void
test_instanceof (jjs_value_t instanceof, jjs_value_t constructor)
{
  jjs_value_t instance = jjs_construct (constructor, NULL, 0);
  jjs_value_t args[2] = { instance, constructor };

  jjs_value_t undefined = jjs_undefined ();
  jjs_value_t result = jjs_call (instanceof, undefined, args, 2);
  jjs_value_free (undefined);

  TEST_ASSERT (!jjs_value_is_exception (result));
  TEST_ASSERT (jjs_value_is_boolean (result));

  TEST_ASSERT (jjs_value_is_true (result));

  jjs_value_free (instance);
  jjs_value_free (result);
} /* test_instanceof */

int
main (void)
{
  jjs_init (JJS_INIT_EMPTY);

  jjs_value_t instanceof = jjs_eval ((jjs_char_t *) instanceof_source, sizeof (instanceof_source) - 1, true);

  /* Test for a native-backed function. */
  jjs_value_t constructor = jjs_function_external (external_function);

  test_instanceof (instanceof, constructor);
  jjs_value_free (constructor);

  /* Test for a JS constructor. */
  jjs_value_t global = jjs_current_realm ();
  jjs_value_t object_name = jjs_string_sz ("Object");
  constructor = jjs_object_get (global, object_name);
  jjs_value_free (object_name);
  jjs_value_free (global);

  test_instanceof (instanceof, constructor);
  jjs_value_free (constructor);

  jjs_value_free (instanceof);

  jjs_cleanup ();

  return 0;
} /* main */
