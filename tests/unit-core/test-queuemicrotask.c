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

#include "jjs-port.h"
#include "jjs.h"

#include "test-common.h"

static bool simple_callback_called = false;

static jjs_value_t
simple_callback (const jjs_call_info_t *call_info_p,
                 const jjs_value_t args_p[],
                 const jjs_length_t args_cnt)
{
  (void) call_info_p;
  (void) args_p;
  (void) args_cnt;
  simple_callback_called = true;
  return jjs_undefined();
}

static jjs_value_t
throw_error_callback (const jjs_call_info_t *call_info_p,
                 const jjs_value_t args_p[],
                 const jjs_length_t args_cnt)
{
  (void) call_info_p;
  (void) args_p;
  (void) args_cnt;
  simple_callback_called = true;
  return jjs_throw_sz(JJS_ERROR_COMMON, "throw_error_callback");
}

int
main (void)
{
  jjs_value_t result;
  jjs_value_t callback;

  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  callback = jjs_function_external(simple_callback);
  result = jjs_queue_microtask(callback);
  jjs_value_free(callback);

  TEST_ASSERT(jjs_value_is_undefined(result));
  jjs_value_free(result);
  TEST_ASSERT(jjs_has_pending_jobs());

  result = jjs_run_jobs();
  TEST_ASSERT(jjs_value_is_undefined(result));
  TEST_ASSERT (simple_callback_called);
  jjs_value_free(result);

  callback = jjs_function_external(throw_error_callback);
  result = jjs_queue_microtask(callback);
  jjs_value_free(callback);

  TEST_ASSERT(jjs_value_is_undefined(result));
  jjs_value_free(result);
  TEST_ASSERT(jjs_has_pending_jobs());

  result = jjs_run_jobs();
  TEST_ASSERT (jjs_value_is_exception(result));
  jjs_value_free(result);
  TEST_ASSERT(!jjs_has_pending_jobs());

  result = jjs_queue_microtask(jjs_undefined());
  TEST_ASSERT(jjs_value_is_exception(result));
  jjs_value_free(result);
  TEST_ASSERT(!jjs_has_pending_jobs());

  jjs_cleanup ();

  return 0;
} /* main */
