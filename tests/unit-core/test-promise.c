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

static const char test_source[] = TEST_STRING_LITERAL ("var p1 = create_promise1();"
                                                               "var p2 = create_promise2();"
                                                               "p1.then(function(x) { "
                                                               "  assert(x==='resolved'); "
                                                               "}); "
                                                               "p2.catch(function(x) { "
                                                               "  assert(x==='rejected'); "
                                                               "}); ");

static int count_in_assert = 0;
static jjs_value_t my_promise1;
static jjs_value_t my_promise2;
static const char s1[] = "resolved";
static const char s2[] = "rejected";

static jjs_value_t
create_promise1_handler (const jjs_call_info_t *call_info_p, /**< call information */
                         const jjs_value_t args_p[], /**< arguments list */
                         const jjs_length_t args_cnt) /**< arguments length */
{
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_p);
  JJS_UNUSED (args_cnt);

  jjs_value_t ret = jjs_promise (ctx ());
  my_promise1 = jjs_value_copy (ctx (), ret);

  return ret;
} /* create_promise1_handler */

static jjs_value_t
create_promise2_handler (const jjs_call_info_t *call_info_p, /**< call information */
                         const jjs_value_t args_p[], /**< arguments list */
                         const jjs_length_t args_cnt) /**< arguments length */
{
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_p);
  JJS_UNUSED (args_cnt);

  jjs_value_t ret = jjs_promise (ctx ());
  my_promise2 = jjs_value_copy (ctx (), ret);

  return ret;
} /* create_promise2_handler */

static jjs_value_t
assert_handler (const jjs_call_info_t *call_info_p, /**< call information */
                const jjs_value_t args_p[], /**< function arguments */
                const jjs_length_t args_cnt) /**< number of function arguments */
{
  JJS_UNUSED (call_info_p);

  count_in_assert++;

  if (args_cnt == 1 && jjs_value_is_true (ctx (), args_p[0]))
  {
    return jjs_boolean (ctx (), true);
  }

  TEST_ASSERT (false);

  return jjs_undefined (ctx ());
} /* assert_handler */

/**
 * Register a JavaScript function in the global object.
 */
static void
register_js_function (const char *name_p, /**< name of the function */
                      jjs_external_handler_t handler_p) /**< function callback */
{
  jjs_value_t global_obj_val = jjs_current_realm (ctx ());

  jjs_value_t function_val = jjs_function_external (ctx (), handler_p);
  jjs_value_t function_name_val = jjs_string_sz (ctx (), name_p);
  jjs_value_t result_val = jjs_object_set (ctx (), global_obj_val, function_name_val, function_val);

  jjs_value_free (ctx (), function_name_val);
  jjs_value_free (ctx (), function_val);
  jjs_value_free (ctx (), global_obj_val);

  jjs_value_free (ctx (), result_val);
} /* register_js_function */

int
main (void)
{
  ctx_open (NULL);

  register_js_function ("create_promise1", create_promise1_handler);
  register_js_function ("create_promise2", create_promise2_handler);
  register_js_function ("assert", assert_handler);

  jjs_value_t parsed_code_val = jjs_parse_sz (ctx (), test_source, NULL);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), parsed_code_val));

  jjs_value_t res = jjs_run (ctx (), parsed_code_val, JJS_MOVE);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), res));

  jjs_value_free (ctx (), res);

  /* Test jjs_promise and jjs_value_is_promise. */
  TEST_ASSERT (jjs_value_is_promise (ctx (), my_promise1));
  TEST_ASSERT (jjs_value_is_promise (ctx (), my_promise2));

  TEST_ASSERT (count_in_assert == 0);

  /* Test jjs_resolve_or_reject_promise. */
  jjs_value_t str_resolve = jjs_string_sz (ctx (), s1);
  jjs_value_t str_reject = jjs_string_sz (ctx (), s2);

  jjs_promise_resolve (ctx (), my_promise1, str_resolve);
  jjs_promise_reject (ctx (), my_promise2, str_reject);

  /* The resolve/reject function should be invalid after the promise has the result. */
  jjs_promise_resolve (ctx (), my_promise2, str_resolve);
  jjs_promise_reject (ctx (), my_promise1, str_reject);

  /* Run the jobqueue. */
  res = jjs_run_jobs (ctx ());

  TEST_ASSERT (!jjs_value_is_exception (ctx (), res));
  TEST_ASSERT (count_in_assert == 2);

  jjs_value_free (ctx (), my_promise1);
  jjs_value_free (ctx (), my_promise2);
  jjs_value_free (ctx (), str_resolve);
  jjs_value_free (ctx (), str_reject);

  ctx_close ();

  return 0;
} /* main */
