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

static int counter = 0;
static jjs_value_t global_module_value;

static jjs_value_t
global_assert (const jjs_call_info_t *call_info_p, /**< call information */
               const jjs_value_t args_p[], /**< arguments list */
               const jjs_length_t args_cnt) /**< arguments length */
{
  JJS_UNUSED (call_info_p);

  TEST_ASSERT (args_cnt == 1 && jjs_value_is_true (ctx (), args_p[0]));
  return jjs_boolean (ctx (), true);
} /* global_assert */

static void
register_assert (void)
{
  jjs_value_t global_object_value = jjs_current_realm (ctx ());

  jjs_value_t function_value = jjs_function_external (ctx (), global_assert);
  jjs_value_t function_name_value = jjs_string_sz (ctx (), "assert");
  jjs_value_t result_value = jjs_object_set (ctx (), global_object_value, function_name_value, function_value);

  jjs_value_free (ctx (), function_name_value);
  jjs_value_free (ctx (), function_value);
  jjs_value_free (ctx (), global_object_value);

  TEST_ASSERT (jjs_value_is_true (ctx (), result_value));
  jjs_value_free (ctx (), result_value);
} /* register_assert */

static void
module_import_meta_callback (jjs_context_t *context_p, /** JJS context */
                             const jjs_value_t module, /**< module */
                             const jjs_value_t meta_object, /**< import.meta object */
                             void *user_p) /**< user pointer */
{
  JJS_UNUSED (context_p);
  TEST_ASSERT (user_p == (void *) &counter);
  TEST_ASSERT (module == global_module_value);

  jjs_value_t property_name_value = jjs_string_sz (ctx (), "prop");
  jjs_value_t result_value = jjs_object_set (ctx (), meta_object, property_name_value, property_name_value);
  jjs_value_free (ctx (), result_value);
  jjs_value_free (ctx (), property_name_value);

  counter++;
} /* module_import_meta_callback */

static void
test_syntax_error (const char *source_p, /**< source code */
                   const jjs_parse_options_t *options_p) /**< parse options */
{
  jjs_value_t result_value = jjs_parse_sz (ctx (), source_p, options_p);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result_value) && jjs_error_type (ctx (), result_value) == JJS_ERROR_SYNTAX);
  jjs_value_free (ctx (), result_value);
} /* test_syntax_error */

static void
run_module (const char *source_p, /* source code */
            const jjs_parse_options_t *parse_options_p) /* parse options */
{
  global_module_value = jjs_parse_sz (ctx (), source_p, parse_options_p);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), global_module_value));

  jjs_value_t result_value = jjs_module_link (ctx (), global_module_value, NULL, NULL);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));
  jjs_value_free (ctx (), result_value);

  result_value = jjs_module_evaluate (ctx (), global_module_value);

  jjs_value_free (ctx (), global_module_value);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));
  jjs_value_free (ctx (), result_value);
} /* run_module */

int
main (void)
{
  if (!jjs_feature_enabled (JJS_FEATURE_MODULE))
  {
    jjs_log (ctx (), JJS_LOG_LEVEL_ERROR, "Module is disabled!\n");
    return 0;
  }

  ctx_open (NULL);

  register_assert ();
  jjs_module_on_import_meta (ctx (), module_import_meta_callback, (void *) &counter);

  /* Syntax errors. */
  test_syntax_error ("import.meta", NULL);
  test_syntax_error ("var a = import.meta", NULL);

  jjs_parse_options_t parse_options = {
    .parse_module = true,
  };

  test_syntax_error ("import.m\\u0065ta", &parse_options);
  test_syntax_error ("import.invalid", &parse_options);

  counter = 0;

  run_module (TEST_STRING_LITERAL ("assert(typeof import.meta === 'object')\n"), &parse_options);

  run_module (TEST_STRING_LITERAL ("assert(Object.getPrototypeOf(import.meta) === null)\n"), &parse_options);

  run_module (TEST_STRING_LITERAL ("var meta = import.meta\n"
                                   "assert(import.meta === meta)\n"
                                   "assert(import.meta === meta)\n"
                                   "function f() {\n"
                                   "  assert(import.meta === meta)\n"
                                   "}\n"
                                   "f()\n"),
              &parse_options);

  run_module (TEST_STRING_LITERAL ("import.meta.x = 5.5\n"
                                   "assert(import.meta.x === 5.5)\n"),
              &parse_options);

  run_module (TEST_STRING_LITERAL ("assert(import.meta.prop === 'prop')\n"
                                   "function f() {\n"
                                   "  import.meta.prop = 6.25\n"
                                   "  import.meta.prop2 = 's'\n"
                                   "\n"
                                   "  return function() {\n"
                                   "    assert(import.meta.prop === 6.25)\n"
                                   "    assert(import.meta.prop2 === 's')\n"
                                   "  }\n"
                                   "}\n"
                                   "f()()\n"),
              &parse_options);

  TEST_ASSERT (counter == 5);

  ctx_close ();
  return 0;
} /* main */
