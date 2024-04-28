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

static void
test_promise_resolve_success (void)
{
  jjs_value_t my_promise = jjs_promise ();

  // A created promise has an undefined promise result by default and a pending state
  {
    jjs_value_t promise_result = jjs_promise_result (my_promise);
    TEST_ASSERT (jjs_value_is_undefined (promise_result));

    jjs_promise_state_t promise_state = jjs_promise_state (my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_PENDING);

    jjs_value_free (promise_result);
  }

  jjs_value_t resolve_value = jjs_object ();
  {
    jjs_value_t obj_key = jjs_string_sz ("key_one");
    jjs_value_t set_result = jjs_object_set (resolve_value, obj_key, jjs_number (3));
    TEST_ASSERT (jjs_value_is_boolean (set_result) && (jjs_value_is_true (set_result)));
    jjs_value_free (set_result);
    jjs_value_free (obj_key);
  }

  // A resolved promise should have the result of from the resolve call and a fulfilled state
  {
    jjs_value_t resolve_result = jjs_promise_resolve (my_promise, resolve_value);

    // Release "old" value of resolve.
    jjs_value_free (resolve_value);

    jjs_value_t promise_result = jjs_promise_result (my_promise);
    {
      TEST_ASSERT (jjs_value_is_object (promise_result));
      jjs_value_t obj_key = jjs_string_sz ("key_one");
      jjs_value_t get_result = jjs_object_get (promise_result, obj_key);
      TEST_ASSERT (jjs_value_is_number (get_result));
      TEST_ASSERT (jjs_value_as_number (get_result) == 3.0);

      jjs_value_free (get_result);
      jjs_value_free (obj_key);
    }

    jjs_promise_state_t promise_state = jjs_promise_state (my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_FULFILLED);

    jjs_value_free (promise_result);

    jjs_value_free (resolve_result);
  }

  // Resolvind a promise again does not change the result/state
  {
    jjs_value_t resolve_result = jjs_promise_reject (my_promise, jjs_number (50));

    jjs_value_t promise_result = jjs_promise_result (my_promise);
    {
      TEST_ASSERT (jjs_value_is_object (promise_result));
      jjs_value_t obj_key = jjs_string_sz ("key_one");
      jjs_value_t get_result = jjs_object_get (promise_result, obj_key);
      TEST_ASSERT (jjs_value_is_number (get_result));
      TEST_ASSERT (jjs_value_as_number (get_result) == 3.0);

      jjs_value_free (get_result);
      jjs_value_free (obj_key);
    }

    jjs_promise_state_t promise_state = jjs_promise_state (my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_FULFILLED);

    jjs_value_free (promise_result);

    jjs_value_free (resolve_result);
  }

  jjs_value_free (my_promise);
} /* test_promise_resolve_success */

static void
test_promise_resolve_fail (void)
{
  jjs_value_t my_promise = jjs_promise ();

  // A created promise has an undefined promise result by default and a pending state
  {
    jjs_value_t promise_result = jjs_promise_result (my_promise);
    TEST_ASSERT (jjs_value_is_undefined (promise_result));

    jjs_promise_state_t promise_state = jjs_promise_state (my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_PENDING);

    jjs_value_free (promise_result);
  }

  // A resolved promise should have the result of from the resolve call and a fulfilled state
  {
    jjs_value_t error_obj = jjs_error_sz (JJS_ERROR_TYPE, "resolve_fail", jjs_undefined());
    jjs_value_t resolve_result = jjs_promise_reject (my_promise, error_obj);
    jjs_value_free (error_obj);

    jjs_value_t promise_result = jjs_promise_result (my_promise);
    // The error is not throw that's why it is only an error object.
    TEST_ASSERT (jjs_value_is_object (promise_result));
    TEST_ASSERT (jjs_error_type (promise_result) == JJS_ERROR_TYPE);

    jjs_promise_state_t promise_state = jjs_promise_state (my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_REJECTED);

    jjs_value_free (promise_result);

    jjs_value_free (resolve_result);
  }

  // Resolvind a promise again does not change the result/state
  {
    jjs_value_t resolve_result = jjs_promise_resolve (my_promise, jjs_number (50));

    jjs_value_t promise_result = jjs_promise_result (my_promise);
    TEST_ASSERT (jjs_value_is_object (promise_result));
    TEST_ASSERT (jjs_error_type (promise_result) == JJS_ERROR_TYPE);

    jjs_promise_state_t promise_state = jjs_promise_state (my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_REJECTED);

    jjs_value_free (promise_result);

    jjs_value_free (resolve_result);
  }

  jjs_value_free (my_promise);
} /* test_promise_resolve_fail */

static void
test_promise_from_js (void)
{
  const jjs_char_t test_source[] = "(new Promise(function(rs, rj) { rs(30); })).then(function(v) { return v + 1; })";

  jjs_value_t parsed_code_val = jjs_parse (test_source, sizeof (test_source) - 1, NULL);
  TEST_ASSERT (!jjs_value_is_exception (parsed_code_val));

  jjs_value_t res = jjs_run (parsed_code_val);
  TEST_ASSERT (jjs_value_is_promise (res));

  TEST_ASSERT (jjs_promise_state (res) == JJS_PROMISE_STATE_PENDING);

  jjs_value_t run_result = jjs_run_jobs ();
  TEST_ASSERT (jjs_value_is_undefined (run_result));
  jjs_value_free (run_result);

  TEST_ASSERT (jjs_promise_state (res) == JJS_PROMISE_STATE_FULFILLED);
  jjs_value_t promise_result = jjs_promise_result (res);
  TEST_ASSERT (jjs_value_is_number (promise_result));
  TEST_ASSERT (jjs_value_as_number (promise_result) == 31.0);

  jjs_value_free (promise_result);
  jjs_value_free (res);
  jjs_value_free (parsed_code_val);
} /* test_promise_from_js */

int
main (void)
{
  TEST_CONTEXT_NEW (context_p);

  test_promise_resolve_fail ();
  test_promise_resolve_success ();

  test_promise_from_js ();

  TEST_CONTEXT_FREE (context_p);

  return 0;
} /* main */
