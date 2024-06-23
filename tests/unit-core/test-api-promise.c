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

static void
test_promise_resolve_success (void)
{
  jjs_value_t my_promise = jjs_promise (ctx ());

  // A created promise has an undefined promise result by default and a pending state
  {
    jjs_value_t promise_result = jjs_promise_result (ctx (), my_promise);
    TEST_ASSERT (jjs_value_is_undefined (ctx (), promise_result));

    jjs_promise_state_t promise_state = jjs_promise_state (ctx (), my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_PENDING);

    jjs_value_free (ctx (), promise_result);
  }

  jjs_value_t resolve_value = jjs_object (ctx ());
  {
    jjs_value_t set_result = jjs_object_set_sz (ctx (), resolve_value, "key_one", jjs_number (ctx (), 3), JJS_MOVE);
    TEST_ASSERT (jjs_value_is_boolean (ctx (), set_result) && (jjs_value_is_true (ctx (), set_result)));
    jjs_value_free (ctx (), set_result);
  }

  // A resolved promise should have the result of from the resolve call and a fulfilled state
  {
    jjs_value_t resolve_result = jjs_promise_resolve (ctx (), my_promise, resolve_value, JJS_MOVE);

    jjs_value_t promise_result = jjs_promise_result (ctx (), my_promise);
    {
      TEST_ASSERT (jjs_value_is_object (ctx (), promise_result));
      jjs_value_t obj_key = jjs_string_sz (ctx (), "key_one");
      jjs_value_t get_result = jjs_object_get (ctx (), promise_result, obj_key);
      TEST_ASSERT (jjs_value_is_number (ctx (), get_result));
      TEST_ASSERT (jjs_value_as_number (ctx (), get_result) == 3.0);

      jjs_value_free (ctx (), get_result);
      jjs_value_free (ctx (), obj_key);
    }

    jjs_promise_state_t promise_state = jjs_promise_state (ctx (), my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_FULFILLED);

    jjs_value_free (ctx (), promise_result);

    jjs_value_free (ctx (), resolve_result);
  }

  // Resolvind a promise again does not change the result/state
  {
    jjs_value_t resolve_result = jjs_promise_reject (ctx (), my_promise, jjs_number (ctx (), 50), JJS_MOVE);

    jjs_value_t promise_result = jjs_promise_result (ctx (), my_promise);
    {
      TEST_ASSERT (jjs_value_is_object (ctx (), promise_result));
      jjs_value_t obj_key = jjs_string_sz (ctx (), "key_one");
      jjs_value_t get_result = jjs_object_get (ctx (), promise_result, obj_key);
      TEST_ASSERT (jjs_value_is_number (ctx (), get_result));
      TEST_ASSERT (jjs_value_as_number (ctx (), get_result) == 3.0);

      jjs_value_free (ctx (), get_result);
      jjs_value_free (ctx (), obj_key);
    }

    jjs_promise_state_t promise_state = jjs_promise_state (ctx (), my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_FULFILLED);

    jjs_value_free (ctx (), promise_result);

    jjs_value_free (ctx (), resolve_result);
  }

  jjs_value_free (ctx (), my_promise);
} /* test_promise_resolve_success */

static void
test_promise_resolve_fail (void)
{
  jjs_value_t my_promise = jjs_promise (ctx ());

  // A created promise has an undefined promise result by default and a pending state
  {
    jjs_value_t promise_result = jjs_promise_result (ctx (), my_promise);
    TEST_ASSERT (jjs_value_is_undefined (ctx (), promise_result));

    jjs_promise_state_t promise_state = jjs_promise_state (ctx (), my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_PENDING);

    jjs_value_free (ctx (), promise_result);
  }

  // A resolved promise should have the result of from the resolve call and a fulfilled state
  {
    jjs_value_t error_obj = jjs_error_sz (ctx (), JJS_ERROR_TYPE, "resolve_fail", jjs_undefined (ctx ()));
    jjs_value_t resolve_result = jjs_promise_reject (ctx (), my_promise, error_obj, JJS_MOVE);

    jjs_value_t promise_result = jjs_promise_result (ctx (), my_promise);
    // The error is not throw that's why it is only an error object.
    TEST_ASSERT (jjs_value_is_object (ctx (), promise_result));
    TEST_ASSERT (jjs_error_type (ctx (), promise_result) == JJS_ERROR_TYPE);

    jjs_promise_state_t promise_state = jjs_promise_state (ctx (), my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_REJECTED);

    jjs_value_free (ctx (), promise_result);

    jjs_value_free (ctx (), resolve_result);
  }

  // Resolvind a promise again does not change the result/state
  {
    jjs_value_t resolve_result = jjs_promise_resolve (ctx (), my_promise, jjs_number (ctx (), 50), JJS_MOVE);

    jjs_value_t promise_result = jjs_promise_result (ctx (), my_promise);
    TEST_ASSERT (jjs_value_is_object (ctx (), promise_result));
    TEST_ASSERT (jjs_error_type (ctx (), promise_result) == JJS_ERROR_TYPE);

    jjs_promise_state_t promise_state = jjs_promise_state (ctx (), my_promise);
    TEST_ASSERT (promise_state == JJS_PROMISE_STATE_REJECTED);

    jjs_value_free (ctx (), promise_result);

    jjs_value_free (ctx (), resolve_result);
  }

  jjs_value_free (ctx (), my_promise);
} /* test_promise_resolve_fail */

static void
test_promise_from_js (void)
{
  const char test_source[] = "(new Promise(function(rs, rj) { rs(30); })).then(function(v) { return v + 1; })";

  jjs_value_t parsed_code_val = jjs_parse_sz (ctx (), test_source, NULL);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), parsed_code_val));

  jjs_value_t res = jjs_run (ctx (), parsed_code_val, JJS_MOVE);
  TEST_ASSERT (jjs_value_is_promise (ctx (), res));

  TEST_ASSERT (jjs_promise_state (ctx (), res) == JJS_PROMISE_STATE_PENDING);

  jjs_value_t run_result = jjs_run_jobs (ctx ());
  TEST_ASSERT (jjs_value_is_undefined (ctx (), run_result));
  jjs_value_free (ctx (), run_result);

  TEST_ASSERT (jjs_promise_state (ctx (), res) == JJS_PROMISE_STATE_FULFILLED);
  jjs_value_t promise_result = jjs_promise_result (ctx (), res);
  TEST_ASSERT (jjs_value_is_number (ctx (), promise_result));
  TEST_ASSERT (jjs_value_as_number (ctx (), promise_result) == 31.0);

  jjs_value_free (ctx (), promise_result);
  jjs_value_free (ctx (), res);
} /* test_promise_from_js */

TEST_MAIN({
  test_promise_resolve_fail ();
  test_promise_resolve_success ();
  test_promise_from_js ();
})
