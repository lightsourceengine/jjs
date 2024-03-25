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

#include "jjs-port.h"
#include "jjs.h"

#include "test-common.h"

static int counter = 0;
static jjs_value_t global_module_value;

static jjs_value_t
global_assert (const jjs_call_info_t *call_info_p, /**< call information */
               const jjs_value_t args_p[], /**< arguments list */
               const jjs_length_t args_cnt) /**< arguments length */
{
  JJS_UNUSED (call_info_p);

  TEST_ASSERT (args_cnt == 1 && jjs_value_is_true (args_p[0]));
  return jjs_boolean (true);
} /* global_assert */

static void
register_assert (void)
{
  jjs_value_t global_object_value = jjs_current_realm ();

  jjs_value_t function_value = jjs_function_external (global_assert);
  jjs_value_t function_name_value = jjs_string_sz ("assert");
  jjs_value_t result_value = jjs_object_set (global_object_value, function_name_value, function_value);

  jjs_value_free (function_name_value);
  jjs_value_free (function_value);
  jjs_value_free (global_object_value);

  TEST_ASSERT (jjs_value_is_true (result_value));
  jjs_value_free (result_value);
} /* register_assert */

static void
module_import_meta_callback (const jjs_value_t module, /**< module */
                             const jjs_value_t meta_object, /**< import.meta object */
                             void *user_p) /**< user pointer */
{
  TEST_ASSERT (user_p == (void *) &counter);
  TEST_ASSERT (module == global_module_value);

  jjs_value_t property_name_value = jjs_string_sz ("prop");
  jjs_value_t result_value = jjs_object_set (meta_object, property_name_value, property_name_value);
  jjs_value_free (result_value);
  jjs_value_free (property_name_value);

  counter++;
} /* module_import_meta_callback */

static void
test_syntax_error (const char *source_p, /**< source code */
                   const jjs_parse_options_t *options_p) /**< parse options */
{
  jjs_value_t result_value = jjs_parse ((const jjs_char_t *) source_p, strlen (source_p), options_p);
  TEST_ASSERT (jjs_value_is_exception (result_value) && jjs_error_type (result_value) == JJS_ERROR_SYNTAX);
  jjs_value_free (result_value);
} /* test_syntax_error */

static void
run_module (const char *source_p, /* source code */
            jjs_parse_options_t *parse_options_p) /* parse options */
{
  global_module_value = jjs_parse ((const jjs_char_t *) source_p, strlen (source_p), parse_options_p);
  TEST_ASSERT (!jjs_value_is_exception (global_module_value));

  jjs_value_t result_value = jjs_module_link (global_module_value, NULL, NULL);
  TEST_ASSERT (!jjs_value_is_exception (result_value));
  jjs_value_free (result_value);

  result_value = jjs_module_evaluate (global_module_value);

  jjs_value_free (global_module_value);

  TEST_ASSERT (!jjs_value_is_exception (result_value));
  jjs_value_free (result_value);
} /* run_module */

int
main (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  if (!jjs_feature_enabled (JJS_FEATURE_MODULE))
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Module is disabled!\n");
    jjs_cleanup ();
    return 0;
  }

  register_assert ();
  jjs_module_on_import_meta (module_import_meta_callback, (void *) &counter);

  /* Syntax errors. */
  test_syntax_error ("import.meta", NULL);
  test_syntax_error ("var a = import.meta", NULL);

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_MODULE;

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

  jjs_cleanup ();
  return 0;
} /* main */
