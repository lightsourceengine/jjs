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

#include "jjs-test.h"

static bool simple_callback_called = false;

static jjs_value_t
simple_callback (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  (void) call_info_p;
  (void) args_p;
  (void) args_cnt;
  simple_callback_called = true;
  return jjs_undefined (ctx ());
}

static jjs_value_t
throw_error_callback (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  (void) call_info_p;
  (void) args_p;
  (void) args_cnt;
  simple_callback_called = true;
  return jjs_throw_sz (ctx (), JJS_ERROR_COMMON, "throw_error_callback");
}

int
main (void)
{
  jjs_value_t result;
  jjs_value_t callback;

  ctx_open (NULL);

  callback = jjs_function_external (ctx (), simple_callback);
  result = jjs_queue_microtask (ctx (), callback);
  jjs_value_free (ctx (), callback);

  TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
  jjs_value_free (ctx (), result);
  TEST_ASSERT (jjs_has_pending_jobs (ctx ()));

  result = jjs_run_jobs (ctx ());
  TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
  TEST_ASSERT (simple_callback_called);
  jjs_value_free (ctx (), result);

  callback = jjs_function_external (ctx (), throw_error_callback);
  result = jjs_queue_microtask (ctx (), callback);
  jjs_value_free (ctx (), callback);

  TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
  jjs_value_free (ctx (), result);
  TEST_ASSERT (jjs_has_pending_jobs (ctx ()));

  result = jjs_run_jobs (ctx ());
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);
  TEST_ASSERT (!jjs_has_pending_jobs (ctx ()));

  result = jjs_queue_microtask (ctx (), jjs_undefined (ctx ()));
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);
  TEST_ASSERT (!jjs_has_pending_jobs (ctx ()));

  // ensure we get an uncaught error via jjs_run_jobs () if callback throws error
  jjs_esm_source_t source = jjs_esm_source_of_sz ("queueMicrotask(() => { throw new Error(); });");

  result = jjs_esm_evaluate_source (ctx (), &source, JJS_MOVE);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_run_jobs (ctx ());
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  ctx_close ();

  return 0;
} /* main */
