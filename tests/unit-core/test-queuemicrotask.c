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

static void
run_pending_jobs (jjs_value_condition_fn_t expected_result_fn)
{
  TEST_ASSERT (jjs_has_pending_jobs (ctx ()));
  jjs_value_t result = ctx_defer_free (jjs_run_jobs (ctx ()));
  TEST_ASSERT (expected_result_fn (ctx (), result));
}

int
main (void)
{
  jjs_value_t result;
  jjs_value_t callback;

  ctx_open (NULL);

  {
    simple_callback_called = false;
    callback = jjs_function_external (ctx (), simple_callback);
    result = ctx_defer_free (jjs_queue_microtask (ctx (), callback, JJS_MOVE));
    TEST_ASSERT (jjs_value_is_undefined (ctx (), result));

    run_pending_jobs (jjs_value_is_undefined);
    TEST_ASSERT (simple_callback_called);
  }

  {
    simple_callback_called = false;
    result = ctx_defer_free (jjs_queue_microtask_fn (ctx (), simple_callback));
    TEST_ASSERT (jjs_value_is_undefined (ctx (), result));

    run_pending_jobs (jjs_value_is_undefined);
    TEST_ASSERT (simple_callback_called);
  }

  {
    result = ctx_defer_free (jjs_queue_microtask_fn (ctx (), NULL));
    TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  }

  {
    result = ctx_defer_free (jjs_queue_microtask_fn (ctx (), throw_error_callback));

    TEST_ASSERT (jjs_value_is_undefined (ctx (), result));

    run_pending_jobs (jjs_value_is_exception);
  }

  {
    result = ctx_defer_free (jjs_queue_microtask (ctx (), ctx_undefined (), JJS_KEEP));
    TEST_ASSERT (jjs_value_is_exception (ctx (), result));
    TEST_ASSERT (!jjs_has_pending_jobs (ctx ()));
  }

  {
    /* ensure we get an uncaught error via jjs_run_jobs () if callback throws error */
    jjs_esm_source_t source = jjs_esm_source_of_sz ("queueMicrotask(() => { throw new Error(); });");

    result = ctx_defer_free (jjs_esm_evaluate_source (ctx (), &source, JJS_MOVE));
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));

    run_pending_jobs (jjs_value_is_exception);
  }

  ctx_close ();

  return 0;
} /* main */
