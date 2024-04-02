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

#include "jjs-core.h"
#include "jjs-annex.h"

#include "jcontext.h"

#if JJS_ANNEX_QUEUE_MICROTASK
static jjs_value_t queue_microtask_impl (jjs_value_t callback);
#endif /* JJS_ANNEX_QUEUE_MICROTASK */

/**
 * Add a callback function to the microtask queue.
 *
 * The callback function will be called the next time jjs_run_jobs() is called.
 *
 * @param callback callback function
 * @return on success, undefined; if callback is not callable, throws a TypeError exception
 */
jjs_value_t
jjs_queue_microtask(const jjs_value_t callback)
{
  jjs_assert_api_enabled ();

#if JJS_ANNEX_QUEUE_MICROTASK
  return queue_microtask_impl (callback);
#else /* !JJS_ANNEX_QUEUE_MICROTASK */
  JJS_UNUSED (callback);
  return jjs_throw_sz(JJS_ERROR_TYPE, ecma_get_error_msg(ECMA_ERR_QUEUE_MICROTASK_NOT_SUPPORTED));
#endif /* JJS_ANNEX_QUEUE_MICROTASK */
} /* jjs_queue_microtask */

#if JJS_ANNEX_QUEUE_MICROTASK

/**
 * Handler for the queueMicrotask() function.
 */
JJS_HANDLER (queue_microtask_handler)
{
  (void) call_info_p;

  return queue_microtask_impl (args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED);
} /* queue_microtask_handler */

/**
 * queueMicrotask() implementation.
 *
 * @param callback callback function
 * @return undefined on success. If callback is not callable, throws a TypeError exception.
 *         Value must be freed by caller.
 */
static jjs_value_t
queue_microtask_impl (jjs_value_t callback)
{
  if (!jjs_value_is_function (callback))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CALLBACK_IS_NOT_CALLABLE));
  }

  ecma_enqueue_microtask_job (callback);

  return ECMA_VALUE_UNDEFINED;
} /* queue_microtask_impl */

#endif /* JJS_ANNEX_QUEUE_MICROTASK */
