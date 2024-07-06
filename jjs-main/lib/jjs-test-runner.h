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

#ifndef JJS_TEST_RUNNER_H
#define JJS_TEST_RUNNER_H

#include <jjs.h>

void jjs_test_runner_install (jjs_context_t *context);
static bool jjs_test_runner_run_all_tests (jjs_context_t*context);

#endif /* !JJS_TEST_RUNNER_H */

#ifdef JJS_TEST_RUNNER_IMPLEMENTATION

#define JJS_TEST_PACKAGE_NAME "jjs:test"
#define JJS_TEST_PACKAGE_FUNCTION "test"
#define JJS_TEST_META_PROP_DESCRIPTION "description"
#define JJS_TEST_META_PROP_TEST "test"
#define JJS_TEST_META_PROP_OPTIONS "options"
#define JJS_TEST_META_PROP_OPTIONS_SKIP "skip"

static jjs_value_t
get_internal_tests (jjs_context_t *context, jjs_value_t obj)
{
  jjs_value_t tests_prop = jjs_string_sz (context, "tests");
  jjs_value_t tests;

  if (jjs_object_has_internal (context, obj, tests_prop))
  {
    tests = jjs_object_get_internal (context, obj, tests_prop);
  }
  else
  {
    tests = jjs_array (context, 0);

    if (!jjs_object_set_internal (context, obj, tests_prop, tests, JJS_KEEP))
    {
      jjs_value_free (context, tests);
      tests = jjs_undefined (context);
    }
  }

  jjs_value_free (context, tests_prop);

  if (!jjs_value_is_array (context, tests))
  {
    jjs_value_free (context, tests);
    tests = jjs_throw_sz (context, JJS_ERROR_COMMON, "Failed to store internal tests array in test function");
  }

  return tests;
}

static jjs_value_t
js_print (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;

  jjs_value_t value = jjs_fmt_join_v (context_p, jjs_string_sz (context_p, " "), JJS_MOVE, args_p, args_cnt);

  if (jjs_value_is_exception (context_p, value))
  {
    return value;
  }

  jjs_platform_io_write (context_p, JJS_STDOUT, value, JJS_MOVE);
  jjs_platform_io_write (context_p, JJS_STDOUT, jjs_string_sz (context_p, "\n"), JJS_MOVE);
  jjs_platform_io_flush (context_p, JJS_STDOUT);

  return jjs_undefined (context_p);
}

static jjs_value_t
js_create_realm (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  (void) args_p;
  (void) args_cnt;
  return jjs_realm (call_info_p->context_p);
}

static jjs_value_t
js_queue_async_assert (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;
  jjs_value_t callback = args_cnt > 0 ? args_p[0] : jjs_undefined (context_p);

  if (!jjs_value_is_function (context_p, callback))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "queueAsyncAssert expected a function");
  }

  jjs_value_t key = jjs_string_sz (context_p, "queue");
  jjs_value_t queue = jjs_object_get_internal (context_p, call_info_p->function, key);

  if (jjs_value_is_undefined (context_p, queue) || jjs_value_is_exception (context_p, queue))
  {
    jjs_value_free (context_p, queue);
    queue = jjs_array (context_p, 0);
    jjs_cli_assert (jjs_object_set_internal (context_p, call_info_p->function, key, queue, JJS_KEEP),
                    "error setting internal async assert queue");
  }

  jjs_cli_assert (jjs_value_is_array (context_p, queue), "async assert queue must be an array");

  jjs_object_set_index (context_p, queue, jjs_array_length (context_p, queue), callback, JJS_KEEP);

  jjs_value_free (context_p, queue);
  jjs_value_free (context_p, key);

  return jjs_undefined (context_p);
}

static jjs_value_t
js_assert (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;

  if (args_cnt > 0 && jjs_value_is_true (context_p, args_p[0]))
  {
    return jjs_undefined (context_p);
  }

  if (args_cnt > 1 && jjs_value_is_string (context_p, args_p[1]))
  {
    return jjs_fmt_throw (context_p, JJS_ERROR_COMMON, "assertion failed: {}\n", &args_p[1], 1, JJS_KEEP);
  }

  return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "assertion failed");
}

static jjs_value_t
js_test (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;
  jjs_value_t arg0 = args_cnt > 0 ? args_p[0] : jjs_undefined (context_p);
  jjs_value_t arg1 = args_cnt > 1 ? args_p[1] : jjs_undefined (context_p);
  jjs_value_t arg2 = args_cnt > 2 ? args_p[2] : jjs_undefined (context_p);
  jjs_value_t options;
  jjs_value_t test_function;

  if (jjs_value_is_function (context_p, arg1))
  {
    test_function = arg1;
    options = jjs_undefined (context_p);
  }
  else
  {
    test_function = arg2;
    options = arg1;
  }

  if (!jjs_value_is_string (context_p, arg0))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "test(): expected a string description");
  }

  if (!jjs_value_is_function (context_p, test_function))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "test(): expected a function");
  }

  if (!jjs_value_is_undefined (context_p, options) && !jjs_value_is_object (context_p, options))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "test(): expected undefined or object for options");
  }

  jjs_value_t tests = get_internal_tests (context_p, call_info_p->function);

  if (jjs_value_is_exception (context_p, tests))
  {
    return tests;
  }

  jjs_value_t test_meta = jjs_object (context_p);

  jjs_value_free (context_p, jjs_object_set_sz (context_p, test_meta, JJS_TEST_META_PROP_DESCRIPTION, arg0, JJS_KEEP));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, test_meta, JJS_TEST_META_PROP_TEST, test_function, JJS_KEEP));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, test_meta, JJS_TEST_META_PROP_OPTIONS, options, JJS_KEEP));

  jjs_value_free (context_p, jjs_object_set_index (context_p, tests, jjs_array_length (context_p, tests), test_meta, JJS_MOVE));

  jjs_value_free (context_p, tests);

  return jjs_undefined (context_p);
}

static bool
should_run_test (jjs_context_t *context_p, jjs_value_t test_obj)
{
  if (!jjs_object_has_sz (context_p, test_obj, JJS_TEST_META_PROP_OPTIONS))
  {
    return true;
  }

  jjs_value_t options = jjs_object_get_sz (context_p, test_obj, JJS_TEST_META_PROP_OPTIONS);
  bool result;

  if (jjs_object_has_sz (context_p, options, JJS_TEST_META_PROP_OPTIONS_SKIP))
  {
    jjs_value_t skip = jjs_object_get_sz (context_p, options, JJS_TEST_META_PROP_OPTIONS_SKIP);

    result = !jjs_value_to_boolean (context_p, skip);
    jjs_value_free (context_p, skip);
  }
  else
  {
    result = true;
  }

  jjs_value_free (context_p, options);

  return result;
}

static bool
process_async_asserts (jjs_context_t*context)
{
  jjs_value_t realm = jjs_current_realm (context);
  jjs_value_t internal_key = jjs_string_sz (context, "queueAsyncAssert");
  jjs_value_t queue_async_assert = jjs_object_get_internal (context, realm, internal_key);
  jjs_value_t key = jjs_string_sz (context, "queue");
  jjs_value_t queue = jjs_object_get_internal (context, queue_async_assert, key);
  jjs_value_t self = jjs_current_realm (context);
  jjs_size_t len = jjs_array_length (context, queue);
  bool has_error = false;

  for (jjs_size_t i = 0; i < len; i++)
  {
    jjs_value_t fn = jjs_object_get_index (context, queue, i);
    jjs_value_t async_assert_result;

    if (jjs_value_is_function (context, fn))
    {
      async_assert_result = jjs_call_this_noargs (context, fn, self, JJS_KEEP);
    }
    else
    {
      async_assert_result = jjs_throw_sz (context, JJS_ERROR_COMMON, "Unknown object in async assert queue!");
    }

    jjs_value_free (context, fn);

    if (jjs_value_is_exception (context, async_assert_result))
    {
      jjs_cli_fmt_info (context, "{}\n", 1, async_assert_result);
      has_error = true;
      break;
    }

    jjs_value_free (context, async_assert_result);
  }

  jjs_value_free (context, key);
  jjs_value_free (context, queue);
  jjs_value_free (context, realm);
  jjs_value_free (context, internal_key);
  jjs_value_free (context, queue_async_assert);
  jjs_value_free (context, self);

  return !has_error;
}

bool
jjs_test_runner_run_all_tests (jjs_context_t*context)
{
  /* note: this runner assumes run-tests.py will do the reporting. */
  jjs_value_t pkg = jjs_vmod_resolve_sz (context, JJS_TEST_PACKAGE_NAME);
  jjs_value_t test_function = jjs_object_get_sz (context, pkg, JJS_TEST_PACKAGE_FUNCTION);
  jjs_value_t tests = get_internal_tests (context, test_function);
  jjs_size_t len = jjs_array_length (context, tests);
  jjs_value_t self = jjs_current_realm (context);
  bool result = true;

  for (jjs_size_t i = 0; i < len; i++)
  {
    jjs_value_t test_meta = jjs_object_get_index (context, tests, i);

    if (should_run_test (context, test_meta))
    {
      jjs_value_t test = jjs_object_get_sz (context, test_meta, JJS_TEST_META_PROP_TEST);
      jjs_value_t test_result = jjs_call_this_noargs (context, test, self, JJS_KEEP);
      jjs_value_t description = jjs_object_get_sz (context, test_meta, JJS_TEST_META_PROP_DESCRIPTION);

      if (jjs_value_is_exception (context, test_result))
      {
        jjs_cli_fmt_error (context, "unhandled exception in test: {}\n{}\n", description, test_result);
        result = false;
      }
      else if (jjs_value_is_promise (context, test_result))
      {
        jjs_value_t jobs_result = jjs_run_jobs (context);

        if (jjs_value_is_exception (context, jobs_result))
        {
          jjs_cli_fmt_error (context, "unhandled exception running async jobs after test: {}\n{}\n", description, jobs_result);
          result = false;
        }
        else if (jjs_promise_state (context, test_result) != JJS_PROMISE_STATE_FULFILLED)
        {
          jjs_cli_fmt_error (context, "unfulfilled promise after test: {}\n{}\n", description, test_result);
          result = false;
        }

        jjs_value_free (context, jobs_result);
      }

      jjs_value_free (context, description);
      jjs_value_free (context, test);
      jjs_value_free (context, test_result);
    }

    jjs_value_free (context, test_meta);
  }

  jjs_value_free (context, self);
  jjs_value_free (context, pkg);
  jjs_value_free (context, test_function);
  jjs_value_free (context, tests);

  return result;
}

void
jjs_test_runner_install (jjs_context_t*context)
{
  jjs_value_t queue_async_assert_fn = jjs_function_external (context, js_queue_async_assert);
  jjs_value_t assert_fn = jjs_function_external (context, js_assert);
  jjs_value_t print_fn = jjs_function_external (context, js_print);
  jjs_value_t create_realm_fn = jjs_function_external (context, js_create_realm);
  jjs_value_t queue_async_assert_key = jjs_string_sz (context, "queueAsyncAssert");
  jjs_value_t realm = jjs_current_realm (context);

  /* store the async function in internal so the queue can be retrieved later */
  jjs_cli_assert (jjs_object_set_internal (context, realm, queue_async_assert_key, queue_async_assert_fn, JJS_KEEP),
                  "cannot store queueAsyncAssert in internal global");

  jjs_value_free (context, jjs_object_set (context, realm, queue_async_assert_key, queue_async_assert_fn, JJS_MOVE));
  jjs_value_free (context, jjs_object_set_sz (context, realm, "print", print_fn, JJS_MOVE));
  jjs_value_free (context, jjs_object_set_sz (context, realm, "assert", assert_fn, JJS_MOVE));
  jjs_value_free (context, jjs_object_set_sz (context, realm, "createRealm", create_realm_fn, JJS_MOVE));

  jjs_value_free (context, queue_async_assert_key);
  jjs_value_free (context, realm);

  jjs_value_t test_function = jjs_function_external (context, js_test);
  jjs_value_t exports = jjs_object (context);

  jjs_value_free (context, jjs_object_set_sz (context, exports, JJS_TEST_PACKAGE_FUNCTION, test_function, JJS_MOVE));

  jjs_value_t pkg = jjs_object (context);
  jjs_value_t format = jjs_string_sz (context, "object");

  jjs_value_free (context, jjs_object_set_sz (context, pkg, "exports", exports, JJS_MOVE));
  jjs_value_free (context, jjs_object_set_sz (context, pkg, "format", format, JJS_MOVE));

  jjs_value_t vmod_result = jjs_vmod_sz (context, JJS_TEST_PACKAGE_NAME, pkg, JJS_MOVE);

  if (jjs_value_is_exception (context, vmod_result))
  {
    jjs_cli_fmt_error (context, "unhandled exception while loading jjs:test: {}\n", vmod_result);
  }

  jjs_cli_assert (!jjs_value_is_exception (context, vmod_result), "");
}

#endif /* JJS_TEST_RUNNER_IMPLEMENTATION */
