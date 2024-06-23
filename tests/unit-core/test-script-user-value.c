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

static jjs_value_t user_values[4];

#define USER_VALUES_SIZE (sizeof (user_values) / sizeof (jjs_value_t))

static void
test_parse (const char *source_p, /**< source code */
            jjs_parse_options_t *options_p, /**< options passed to jjs_parse */
            bool run_code) /**< run the code after parsing */
{
  for (size_t i = 0; i < USER_VALUES_SIZE; i++)
  {
    options_p->user_value = jjs_optional_value (user_values[i]);

    jjs_value_t result = jjs_parse_sz (ctx (), source_p, options_p);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));

    if (run_code)
    {
      result = jjs_run (ctx (), result, JJS_MOVE);
      TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
    }

    jjs_value_t user_value = jjs_source_user_value (ctx (), result);
    jjs_value_t compare_value = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, user_value, user_values[i]);

    TEST_ASSERT (jjs_value_is_true (ctx (), compare_value));

    jjs_value_free (ctx (), compare_value);
    jjs_value_free (ctx (), user_value);
    jjs_value_free (ctx (), result);
  }
} /* test_parse */

static void
test_parse_function (const char *source_p, /**< source code */
                     jjs_parse_options_t *options_p, /**< options passed to jjs_parse */
                     bool run_code) /**< run the code after parsing */
{
  options_p->argument_list = jjs_optional_value (jjs_string_sz (ctx (), ""));

  for (size_t i = 0; i < USER_VALUES_SIZE; i++)
  {
    options_p->user_value = jjs_optional_value(user_values[i]);

    jjs_value_t result = jjs_parse (ctx (), (const jjs_char_t *) source_p, strlen (source_p), options_p);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));

    if (run_code)
    {
      jjs_value_t parse_result = result;
      jjs_value_t this_value = jjs_undefined (ctx ());
      result = jjs_call (ctx (), result, this_value, NULL, 0);
      jjs_value_free (ctx (), parse_result);
      jjs_value_free (ctx (), this_value);
      TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
    }

    jjs_value_t user_value = jjs_source_user_value (ctx (), result);
    jjs_value_t compare_value = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, user_value, user_values[i]);

    TEST_ASSERT (jjs_value_is_true (ctx (), compare_value));

    jjs_value_free (ctx (), compare_value);
    jjs_value_free (ctx (), user_value);
    jjs_value_free (ctx (), result);
  }

  jjs_value_free (ctx (), options_p->argument_list.value);
} /* test_parse_function */

int
main (void)
{
  ctx_open (NULL);

  user_values[0] = jjs_object (ctx ());
  user_values[1] = jjs_null (ctx ());
  user_values[2] = jjs_number (ctx (), 5.5);
  user_values[3] = jjs_string_sz (ctx (), "AnyString...");

  jjs_parse_options_t parse_options = jjs_parse_options ();
  const char *source_p = TEST_STRING_LITERAL ("");

  test_parse (source_p, &parse_options, false);
  test_parse_function (source_p, &parse_options, false);

  if (jjs_feature_enabled (JJS_FEATURE_MODULE))
  {
    parse_options = (jjs_parse_options_t) {
      .parse_module = true,
    };
    test_parse (source_p, &parse_options, false);
  }

  source_p = TEST_STRING_LITERAL ("function f() { }\n"
                                  "f");
  parse_options = jjs_parse_options ();
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("function f() { return function() {} }\n"
                                  "f()");
  parse_options = jjs_parse_options ();
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("return function() {}");
  parse_options = jjs_parse_options ();
  test_parse_function (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("(class {})");
  parse_options = jjs_parse_options ();
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("eval('function f() {}')\n"
                                  "f");
  parse_options = jjs_parse_options ();
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("eval('function f() { return eval(\\'(function () {})\\') }')\n"
                                  "f()");
  parse_options = jjs_parse_options ();
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("eval('function f() {}')\n"
                                  "return f");
  parse_options = jjs_parse_options ();
  test_parse_function (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("eval('function f() { return eval(\\'(function () {})\\') }')\n"
                                  "return f()");
  parse_options = jjs_parse_options ();
  test_parse_function (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("function f() {}\n"
                                  "f.bind(1)");
  parse_options = jjs_parse_options ();
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("function f() {}\n"
                                  "f.bind(1).bind(2, 3)");
  parse_options = jjs_parse_options ();
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("function f() {}\n"
                                  "return f.bind(1)");
  parse_options = jjs_parse_options ();
  test_parse_function (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("function f() {}\n"
                                  "return f.bind(1).bind(2, 3)");
  parse_options = jjs_parse_options ();
  test_parse_function (source_p, &parse_options, true);

  for (size_t i = 0; i < USER_VALUES_SIZE; i++)
  {
    jjs_value_t result = jjs_source_user_value (ctx (), user_values[i]);
    TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
    jjs_value_free (ctx (), result);
  }

  for (size_t i = 0; i < USER_VALUES_SIZE; i++)
  {
    jjs_value_free (ctx (), user_values[i]);
  }

  ctx_close ();
  return 0;
} /* main */
