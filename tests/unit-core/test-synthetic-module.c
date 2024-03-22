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

#include "jjs.h"

#include "config.h"
#define TEST_COMMON_IMPLEMENTATION
#include "test-common.h"

static const char* SYNTHETIC_MODULE_EVALUATE_RESULT = "test_result";

/** create a synthetic module and move it to the linked state */
static jjs_value_t
create_synthetic_module_linked (jjs_synthetic_module_evaluate_cb_t callback,
                                const jjs_value_t* const exports_p,
                                size_t export_count)
{
  jjs_value_t module = jjs_synthetic_module (callback, exports_p, export_count);

  JJS_EXPECT_NOT_EXCEPTION (module);
  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_UNLINKED);

  JJS_EXPECT_TRUE_MOVE (jjs_module_link (module, NULL, NULL));
  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_LINKED);

  return module;
} /* create_synthetic_module_linked */

/** synthetic module evaluate callback that returns SYNTHETIC_MODULE_EVALUATE_RESULT */
static jjs_value_t
synthetic_module_evaluate (const jjs_value_t module)
{
  JJS_UNUSED (module);
  return jjs_string_sz (SYNTHETIC_MODULE_EVALUATE_RESULT);
} /* synthetic_module_evaluate */

/** synthetic module evaluate callback that just throws an exception */
static jjs_value_t
synthetic_module_evaluate_throw (const jjs_value_t module)
{
  JJS_UNUSED (module);
  return jjs_throw_sz (JJS_ERROR_COMMON, "from synthetic_module_evaluate_throw");
} /* synthetic_module_evaluate_throw */

/** link callback for run_with_synthetic_module */
static jjs_value_t
synthetic_module_link_cb (const jjs_value_t specifier, const jjs_value_t referrer, void* user_p)
{
  JJS_UNUSED (referrer);
  JJS_EXPECT_TRUE_MOVE (jjs_binary_op (JJS_BIN_OP_STRICT_EQUAL, specifier, push_sz ("synthetic")));

  return jjs_value_copy ((jjs_value_t) (uintptr_t) user_p);
} /* synthetic_module_link_cb */

/** runs a module code snippet in a context where synthetic_module can be imported with 'synthetic' */
static jjs_value_t
run_with_synthetic_module (const jjs_value_t synthetic_module, const char* code)
{
  jjs_parse_options_t opts = {
    .options = JJS_PARSE_MODULE,
  };

  jjs_value_t module = jjs_parse ((const jjs_char_t*) code, (jjs_size_t) strlen (code), &opts);

  JJS_EXPECT_NOT_EXCEPTION (module);
  JJS_EXPECT_TRUE_MOVE (jjs_module_link (module, synthetic_module_link_cb, (void*) (uintptr_t) synthetic_module));

  jjs_value_t result = jjs_module_evaluate (push (module));

  JJS_EXPECT_NOT_EXCEPTION (push (jjs_run_jobs ()));

  return result;
} /* run_with_synthetic_module */

static void
test_synthetic_module (void)
{
  jjs_value_t module = push (create_synthetic_module_linked (synthetic_module_evaluate, NULL, 0));
  jjs_value_t evaluate_result = push (jjs_module_evaluate (module));

  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_EVALUATED);
  JJS_EXPECT_NOT_EXCEPTION (evaluate_result);
  TEST_ASSERT (strict_equals_cstr (evaluate_result, SYNTHETIC_MODULE_EVALUATE_RESULT));

  JJS_EXPECT_UNDEFINED_MOVE (run_with_synthetic_module (module, "import 'synthetic';"));
  JJS_EXPECT_PROMISE_MOVE (run_with_synthetic_module (module, "import('synthetic');"));
} /* test_synthetic_module */

static void
test_synthetic_module_no_evaluate_callback (void)
{
  jjs_value_t module = push (create_synthetic_module_linked (NULL, NULL, 0));
  jjs_value_t evaluate_result = push (jjs_module_evaluate (module));

  TEST_ASSERT (jjs_value_is_undefined (evaluate_result));
  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_EVALUATED);
} /* test_synthetic_module_no_evaluate_callback */

static void
test_synthetic_module_evaluate_callback_throws (void)
{
  jjs_value_t module = push (create_synthetic_module_linked (synthetic_module_evaluate_throw, NULL, 0));
  jjs_value_t evaluate_result = push (jjs_module_evaluate (module));

  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_ERROR);
  TEST_ASSERT (jjs_value_is_exception (evaluate_result));
} /* test_synthetic_module_evaluate_callback_throws */

static void
test_synthetic_module_set_exports (void)
{
  jjs_value_t export_names[] = {
    push_sz ("five"),
  };
  jjs_value_t module =
    push (jjs_synthetic_module (NULL, export_names, sizeof (export_names) / sizeof (export_names[0])));

  JJS_EXPECT_NOT_EXCEPTION (module);
  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_UNLINKED);

  JJS_EXPECT_TRUE_MOVE (jjs_module_link (module, NULL, NULL));
  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_LINKED);

  JJS_EXPECT_TRUE_MOVE (jjs_synthetic_module_set_export (module, export_names[0], push (jjs_number_from_int32 (5))));

  JJS_EXPECT_UNDEFINED_MOVE (jjs_module_evaluate (module));
  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_EVALUATED);

  JJS_EXPECT_UNDEFINED_MOVE (run_with_synthetic_module (module,
                                                        "import { five } from 'synthetic';"
                                                        "if (five !== 5) { throw Error('invalid synthetic export') }"));

  JJS_EXPECT_PROMISE_MOVE (run_with_synthetic_module (module,
                                                        "import('synthetic').then(ns => {"
                                                        "if (ns.five !== 5) { throw Error('invalid synthetic export') }"
                                                        "});"));
} /* test_synthetic_module_set_exports */

static void
test_synthetic_module_set_exports_invalid_args (void)
{
  jjs_value_t export_name = push_sz ("name");
  jjs_value_t module = push (create_synthetic_module_linked (NULL, &export_name, 1));

  // export name not in export list
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (module, push_sz ("xxx"), jjs_undefined ()));
  // export name is empty string
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (module, push_sz (""), jjs_undefined ()));
  // export name is not a string
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (module, push (jjs_object ()), jjs_undefined ()));

  // cannot set export on evaluated module
  JJS_EXPECT_TRUE_MOVE (jjs_synthetic_module_set_export (module, export_name, jjs_undefined ()));
  JJS_EXPECT_UNDEFINED_MOVE (jjs_module_evaluate (module));
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (module, export_name, jjs_undefined ()));

  jjs_value_t module_no_exports = push (create_synthetic_module_linked (NULL, NULL, 0));

  // invalid module
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (jjs_null (), export_name, jjs_undefined ()));
  // not exports declared
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (module_no_exports, export_name, jjs_undefined ()));
} /* test_synthetic_module_set_exports_invalid_args */

int
main (void)
{
  TEST_INIT ();

  jjs_init (JJS_INIT_EMPTY);

  test_synthetic_module ();
  test_synthetic_module_no_evaluate_callback ();
  test_synthetic_module_evaluate_callback_throws ();
  test_synthetic_module_set_exports ();
  test_synthetic_module_set_exports_invalid_args ();

  free_values ();
  jjs_cleanup ();
  return 0;
} /* main */
