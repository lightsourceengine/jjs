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

static jjs_value_t
source_name_handler (const jjs_call_info_t *call_info_p, /**< call information */
                     const jjs_value_t args_p[], /**< argument list */
                     const jjs_length_t args_count) /**< argument count */
{
  (void) call_info_p;

  jjs_value_t undefined_value = jjs_undefined (ctx ());
  jjs_value_t source_name = jjs_source_name (ctx (), args_count > 0 ? args_p[0] : undefined_value);
  jjs_value_free (ctx (), undefined_value);

  return source_name;
} /* source_name_handler */

int
main (void)
{
  if (!jjs_feature_enabled (JJS_FEATURE_LINE_INFO))
  {
    jjs_log (ctx (), JJS_LOG_LEVEL_ERROR, "Line info support is disabled!\n");
    return 0;
  }

  ctx_open (NULL);

  jjs_value_t global = jjs_current_realm (ctx ());

  /* Register the "sourceName" method. */
  {
    jjs_value_t func = jjs_function_external (ctx (), source_name_handler);
    jjs_value_t name = jjs_string_sz (ctx (), "sourceName");
    jjs_value_t result = jjs_object_set (ctx (), global, name, func);
    jjs_value_free (ctx (), result);
    jjs_value_free (ctx (), name);
    jjs_value_free (ctx (), func);
  }

  jjs_value_free (ctx (), global);

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_HAS_SOURCE_NAME;

  const char *source_1 = ("function f1 () {\n"
                          "  if (sourceName() !== 'demo1.js') return false; \n"
                          "  if (sourceName(f1) !== 'demo1.js') return false; \n"
                          "  if (sourceName(5) !== '<anonymous>') return false; \n"
                          "  return f1; \n"
                          "} \n"
                          "f1();");

  parse_options.source_name = jjs_string_sz (ctx (), "demo1.js");

  jjs_value_t program = jjs_parse (ctx (), (const jjs_char_t *) source_1, strlen (source_1), &parse_options);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), program));

  jjs_value_t run_result = jjs_run (ctx (), program, JJS_KEEP);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), run_result));
  TEST_ASSERT (jjs_value_is_object (ctx (), run_result));

  jjs_value_t source_name_value = jjs_source_name (ctx (), run_result);
  jjs_value_t compare_result =
    jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
  TEST_ASSERT (jjs_value_is_true (ctx (), compare_result));

  jjs_value_free (ctx (), compare_result);
  jjs_value_free (ctx (), source_name_value);
  jjs_value_free (ctx (), parse_options.source_name);

  jjs_value_free (ctx (), run_result);
  jjs_value_free (ctx (), program);

  const char *source_2 = ("function f2 () { \n"
                          "  if (sourceName() !== 'demo2.js') return false; \n"
                          "  if (sourceName(f2) !== 'demo2.js') return false; \n"
                          "  if (sourceName(f1) !== 'demo1.js') return false; \n"
                          "  if (sourceName(Object.prototype) !== '<anonymous>') return false; \n"
                          "  if (sourceName(Function) !== '<anonymous>') return false; \n"
                          "  return f2; \n"
                          "} \n"
                          "f2(); \n");

  parse_options.source_name = jjs_string_sz (ctx (), "demo2.js");

  program = jjs_parse (ctx (), (const jjs_char_t *) source_2, strlen (source_2), &parse_options);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), program));

  run_result = jjs_run (ctx (), program, JJS_KEEP);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), run_result));
  TEST_ASSERT (jjs_value_is_object (ctx (), run_result));

  source_name_value = jjs_source_name (ctx (), run_result);
  compare_result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
  TEST_ASSERT (jjs_value_is_true (ctx (), compare_result));

  jjs_value_free (ctx (), compare_result);
  jjs_value_free (ctx (), source_name_value);
  jjs_value_free (ctx (), parse_options.source_name);

  jjs_value_free (ctx (), run_result);
  jjs_value_free (ctx (), program);
  if (jjs_feature_enabled (JJS_FEATURE_MODULE))
  {
    jjs_value_t anon = jjs_string_sz (ctx (), "<anonymous>");
    const char *source_3 = "";

    parse_options.options = JJS_PARSE_MODULE | JJS_PARSE_HAS_SOURCE_NAME;
    parse_options.source_name = jjs_string_sz (ctx (), "demo3.js");

    program = jjs_parse (ctx (), (const jjs_char_t *) source_3, strlen (source_3), &parse_options);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), program));

    source_name_value = jjs_source_name (ctx (), program);
    compare_result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
    TEST_ASSERT (jjs_value_is_true (ctx (), compare_result));

    jjs_value_free (ctx (), compare_result);
    jjs_value_free (ctx (), source_name_value);

    run_result = jjs_module_link (ctx (), program, NULL, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), run_result));

    source_name_value = jjs_source_name (ctx (), run_result);
    compare_result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, source_name_value, anon);
    TEST_ASSERT (jjs_value_is_true (ctx (), compare_result));

    jjs_value_free (ctx (), compare_result);
    jjs_value_free (ctx (), source_name_value);
    jjs_value_free (ctx (), run_result);

    run_result = jjs_module_evaluate (ctx (), program);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), run_result));

    source_name_value = jjs_source_name (ctx (), run_result);
    compare_result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, source_name_value, anon);
    TEST_ASSERT (jjs_value_is_true (ctx (), compare_result));

    jjs_value_free (ctx (), compare_result);
    jjs_value_free (ctx (), source_name_value);
    jjs_value_free (ctx (), run_result);
    jjs_value_free (ctx (), program);
    jjs_value_free (ctx (), parse_options.source_name);
  }
  const char *source_4 = ("function f(){} \n"
                          "f.bind().bind();");

  parse_options.options = JJS_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jjs_string_sz (ctx (), "demo4.js");

  program = jjs_parse (ctx (), (const jjs_char_t *) source_4, strlen (source_4), &parse_options);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), program));

  run_result = jjs_run (ctx (), program, JJS_KEEP);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), run_result));
  TEST_ASSERT (jjs_value_is_object (ctx (), run_result));

  source_name_value = jjs_source_name (ctx (), run_result);
  compare_result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
  TEST_ASSERT (jjs_value_is_true (ctx (), compare_result));
  jjs_value_free (ctx (), compare_result);

  jjs_value_free (ctx (), source_name_value);
  jjs_value_free (ctx (), parse_options.source_name);
  jjs_value_free (ctx (), run_result);
  jjs_value_free (ctx (), program);

  const char *source_5 = "";

  parse_options.options = JJS_PARSE_HAS_USER_VALUE | JJS_PARSE_HAS_SOURCE_NAME;
  parse_options.user_value = jjs_object (ctx ());
  parse_options.source_name = jjs_string_sz (ctx (), "demo5.js");

  program = jjs_parse (ctx (), (const jjs_char_t *) source_5, strlen (source_5), &parse_options);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), program));

  source_name_value = jjs_source_name (ctx (), program);
  compare_result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
  TEST_ASSERT (jjs_value_is_true (ctx (), compare_result));

  jjs_value_free (ctx (), source_name_value);
  jjs_value_free (ctx (), compare_result);
  jjs_value_free (ctx (), parse_options.user_value);
  jjs_value_free (ctx (), parse_options.source_name);
  jjs_value_free (ctx (), program);

  const char *source_6 = "(class {})";

  parse_options.options = JJS_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jjs_string_sz (ctx (), "demo6.js");

  program = jjs_parse (ctx (), (const jjs_char_t *) source_6, strlen (source_6), &parse_options);
  if (!jjs_value_is_exception (ctx (), program))
  {
    source_name_value = jjs_source_name (ctx (), program);
    compare_result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
    TEST_ASSERT (jjs_value_is_true (ctx (), compare_result));

    jjs_value_free (ctx (), source_name_value);
    jjs_value_free (ctx (), compare_result);
  }

  jjs_value_free (ctx (), parse_options.source_name);
  jjs_value_free (ctx (), program);

  ctx_close ();

  return 0;
} /* main */
