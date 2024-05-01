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

#include "jjs-test.h"

static const char instanceof_source[] = "var x = function(o, c) {return (o instanceof c);}; x";

static jjs_value_t
external_function (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_size_t args_count)
{
  (void) call_info_p;
  (void) args_p;
  (void) args_count;

  return jjs_undefined (ctx ());
} /* external_function */

static void
test_instanceof (jjs_value_t instanceof, jjs_value_t constructor)
{
  jjs_value_t instance = jjs_construct (ctx (), constructor, NULL, 0);
  jjs_value_t args[2] = { instance, constructor };

  jjs_value_t undefined = jjs_undefined (ctx ());
  jjs_value_t result = jjs_call (ctx (), instanceof, undefined, args, 2);
  jjs_value_free (ctx (), undefined);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result));

  TEST_ASSERT (jjs_value_is_true (ctx (), result));

  jjs_value_free (ctx (), instance);
  jjs_value_free (ctx (), result);
} /* test_instanceof */

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t instanceof = jjs_eval (ctx (), (jjs_char_t *) instanceof_source, sizeof (instanceof_source) - 1, true);

  /* Test for a native-backed function. */
  jjs_value_t constructor = jjs_function_external (ctx (), external_function);

  test_instanceof (instanceof, constructor);
  jjs_value_free (ctx (), constructor);

  /* Test for a JS constructor. */
  jjs_value_t global = jjs_current_realm (ctx ());
  jjs_value_t object_name = jjs_string_sz (ctx (), "Object");
  constructor = jjs_object_get (ctx (), global, object_name);
  jjs_value_free (ctx (), object_name);
  jjs_value_free (ctx (), global);

  test_instanceof (instanceof, constructor);
  jjs_value_free (ctx (), constructor);

  jjs_value_free (ctx (), instanceof);

  ctx_close ();

  return 0;
} /* main */
