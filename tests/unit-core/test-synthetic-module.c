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

static const char* SYNTHETIC_MODULE_EVALUATE_RESULT = "test_result";

/** create a synthetic module and move it to the linked state */
static jjs_value_t
create_synthetic_module_linked (jjs_synthetic_module_evaluate_cb_t callback,
                                const jjs_value_t* const exports_p,
                                size_t export_count)
{
  jjs_value_t module = jjs_synthetic_module (ctx (), callback, exports_p, export_count);

  JJS_EXPECT_NOT_EXCEPTION (module);
  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_UNLINKED);

  JJS_EXPECT_TRUE_MOVE (jjs_module_link (ctx (), module, NULL, NULL));
  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_LINKED);

  return module;
} /* create_synthetic_module_linked */

/** synthetic module evaluate callback that returns SYNTHETIC_MODULE_EVALUATE_RESULT */
static jjs_value_t
synthetic_module_evaluate (jjs_context_t *context_p, const jjs_value_t module)
{
  JJS_UNUSED (context_p);
  JJS_UNUSED (module);
  return jjs_string_sz (ctx (), SYNTHETIC_MODULE_EVALUATE_RESULT);
} /* synthetic_module_evaluate */

/** synthetic module evaluate callback that just throws an exception */
static jjs_value_t
synthetic_module_evaluate_throw (jjs_context_t *context_p, const jjs_value_t module)
{
  JJS_UNUSED (context_p);
  JJS_UNUSED (module);
  return jjs_throw_sz (ctx (), JJS_ERROR_COMMON, "from synthetic_module_evaluate_throw");
} /* synthetic_module_evaluate_throw */

/** link callback for run_with_synthetic_module */
static jjs_value_t
synthetic_module_link_cb (jjs_context_t *context_p, const jjs_value_t specifier, const jjs_value_t referrer, void* user_p)
{
  JJS_UNUSED (context_p);
  JJS_UNUSED (referrer);
  JJS_EXPECT_TRUE_MOVE (jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, specifier, ctx_cstr ("synthetic")));

  return jjs_value_copy (ctx (), (jjs_value_t) (uintptr_t) user_p);
} /* synthetic_module_link_cb */

/** runs a module code snippet in a context where synthetic_module can be imported with 'synthetic' */
static jjs_value_t
run_with_synthetic_module (const jjs_value_t synthetic_module, const char* code)
{
  jjs_parse_options_t opts = {
    .options = JJS_PARSE_MODULE,
  };

  jjs_value_t module = jjs_parse (ctx (), (const jjs_char_t*) code, (jjs_size_t) strlen (code), &opts);

  JJS_EXPECT_NOT_EXCEPTION (module);
  JJS_EXPECT_TRUE_MOVE (jjs_module_link (ctx (), module, synthetic_module_link_cb, (void*) (uintptr_t) synthetic_module));

  jjs_value_t result = jjs_module_evaluate (ctx (), ctx_defer_free (module));

  JJS_EXPECT_NOT_EXCEPTION (ctx_defer_free (jjs_run_jobs (ctx ())));

  return result;
} /* run_with_synthetic_module */

static void
test_synthetic_module (void)
{
  jjs_value_t module = ctx_defer_free (create_synthetic_module_linked (synthetic_module_evaluate, NULL, 0));
  jjs_value_t evaluate_result = ctx_defer_free (jjs_module_evaluate (ctx (), module));

  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_EVALUATED);
  JJS_EXPECT_NOT_EXCEPTION (evaluate_result);
  TEST_ASSERT (strict_equals_cstr (ctx (), evaluate_result, SYNTHETIC_MODULE_EVALUATE_RESULT));

  JJS_EXPECT_UNDEFINED_MOVE (run_with_synthetic_module (module, "import 'synthetic';"));
  JJS_EXPECT_PROMISE_MOVE (run_with_synthetic_module (module, "import('synthetic');"));
} /* test_synthetic_module */

static void
test_synthetic_module_no_evaluate_callback (void)
{
  jjs_value_t module = ctx_defer_free (create_synthetic_module_linked (NULL, NULL, 0));
  jjs_value_t evaluate_result = ctx_defer_free (jjs_module_evaluate (ctx (), module));

  TEST_ASSERT (jjs_value_is_undefined (ctx (), evaluate_result));
  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_EVALUATED);
} /* test_synthetic_module_no_evaluate_callback */

static void
test_synthetic_module_evaluate_callback_throws (void)
{
  jjs_value_t module = ctx_defer_free (create_synthetic_module_linked (synthetic_module_evaluate_throw, NULL, 0));
  jjs_value_t evaluate_result = ctx_defer_free (jjs_module_evaluate (ctx (), module));

  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_ERROR);
  TEST_ASSERT (jjs_value_is_exception (ctx (), evaluate_result));
} /* test_synthetic_module_evaluate_callback_throws */

static void
test_synthetic_module_set_exports (void)
{
  jjs_value_t export_names[] = {
    ctx_cstr ("five"),
  };
  jjs_value_t module = ctx_defer_free (
    jjs_synthetic_module (ctx (), NULL, export_names, sizeof (export_names) / sizeof (export_names[0])));

  JJS_EXPECT_NOT_EXCEPTION (module);
  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_UNLINKED);

  JJS_EXPECT_TRUE_MOVE (jjs_module_link (ctx (), module, NULL, NULL));
  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_LINKED);

  JJS_EXPECT_TRUE_MOVE (jjs_synthetic_module_set_export (ctx (), module, export_names[0],
                                                         ctx_defer_free (jjs_number_from_int32 (ctx (), 5))));

  JJS_EXPECT_UNDEFINED_MOVE (jjs_module_evaluate (ctx (), module));
  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_EVALUATED);

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
  jjs_value_t export_name = ctx_cstr ("name");
  jjs_value_t module = ctx_defer_free (create_synthetic_module_linked (NULL, &export_name, 1));

  // export name not in export list
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (ctx (), module, ctx_cstr ("xxx"), jjs_undefined (ctx ())));
  // export name is empty string
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (ctx (), module, ctx_cstr (""), jjs_undefined (ctx ())));
  // export name is not a string
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (ctx (), module, ctx_defer_free (jjs_object (ctx ())), jjs_undefined (ctx ())));

  // cannot set export on evaluated module
  JJS_EXPECT_TRUE_MOVE (jjs_synthetic_module_set_export (ctx (), module, export_name, jjs_undefined (ctx ())));
  JJS_EXPECT_UNDEFINED_MOVE (jjs_module_evaluate (ctx (), module));
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (ctx (), module, export_name, jjs_undefined (ctx ())));

  jjs_value_t module_no_exports = ctx_defer_free (create_synthetic_module_linked (NULL, NULL, 0));

  // invalid module
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (ctx (), jjs_null (ctx ()), export_name, jjs_undefined (ctx ())));
  // not exports declared
  JJS_EXPECT_EXCEPTION_MOVE (jjs_synthetic_module_set_export (ctx (), module_no_exports, export_name, jjs_undefined (ctx ())));
} /* test_synthetic_module_set_exports_invalid_args */

TEST_MAIN({
  test_synthetic_module ();
  test_synthetic_module_no_evaluate_callback ();
  test_synthetic_module_evaluate_callback_throws ();
  test_synthetic_module_set_exports ();
  test_synthetic_module_set_exports_invalid_args ();
})
