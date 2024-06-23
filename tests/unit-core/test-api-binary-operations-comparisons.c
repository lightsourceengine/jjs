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

#define T(op, lhs, rhs, res) \
  {                          \
    op, lhs, rhs, res        \
  }

typedef struct
{
  jjs_binary_op_t op;
  jjs_value_t lhs;
  jjs_value_t rhs;
  bool expected;
} test_entry_t;

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t obj1 = jjs_eval_sz (ctx (), "o={x:1};o", JJS_PARSE_NO_OPTS);
  jjs_value_t obj2 = jjs_eval_sz (ctx (), "o={x:1};o", JJS_PARSE_NO_OPTS);
  jjs_value_t err1 = jjs_throw_sz (ctx (), JJS_ERROR_SYNTAX, "error");

  test_entry_t tests[] = {
    /* Testing strict equal comparison */
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.0), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_number (ctx (), 3.1), jjs_number (ctx (), 10), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_number (ctx (), 3.1), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_number (ctx (), 3.1), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_string_sz (ctx (), "example string"), jjs_string_sz (ctx (), "example string"), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_string_sz (ctx (), "example string"), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_string_sz (ctx (), "example string"), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_string_sz (ctx (), "example string"), jjs_number (ctx (), 5.0), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_undefined (ctx ()), jjs_undefined (ctx ()), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_undefined (ctx ()), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_null (ctx ()), jjs_null (ctx ()), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_boolean (ctx (), true), jjs_boolean (ctx (), true), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_boolean (ctx (), true), jjs_boolean (ctx (), false), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_boolean (ctx (), false), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_boolean (ctx (), false), jjs_boolean (ctx (), false), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (ctx (), obj1), jjs_value_copy (ctx (), obj1), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (ctx (), obj1), jjs_value_copy (ctx (), obj2), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (ctx (), obj2), jjs_value_copy (ctx (), obj1), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (ctx (), obj1), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (ctx (), obj1), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (ctx (), obj1), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (ctx (), obj1), jjs_boolean (ctx (), false), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (ctx (), obj1), jjs_number (ctx (), 5.0), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (ctx (), obj1), jjs_string_sz (ctx (), "example string"), false),

    /* Testing equal comparison */
    T (JJS_BIN_OP_EQUAL, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.0), true),
    T (JJS_BIN_OP_EQUAL, jjs_number (ctx (), 3.1), jjs_number (ctx (), 10), false),
    T (JJS_BIN_OP_EQUAL, jjs_number (ctx (), 3.1), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_EQUAL, jjs_number (ctx (), 3.1), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_EQUAL, jjs_string_sz (ctx (), "example string"), jjs_string_sz (ctx (), "example string"), true),
    T (JJS_BIN_OP_EQUAL, jjs_string_sz (ctx (), "example string"), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_EQUAL, jjs_string_sz (ctx (), "example string"), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_EQUAL, jjs_string_sz (ctx (), "example string"), jjs_number (ctx (), 5.0), false),
    T (JJS_BIN_OP_EQUAL, jjs_undefined (ctx ()), jjs_undefined (ctx ()), true),
    T (JJS_BIN_OP_EQUAL, jjs_undefined (ctx ()), jjs_null (ctx ()), true),
    T (JJS_BIN_OP_EQUAL, jjs_null (ctx ()), jjs_null (ctx ()), true),
    T (JJS_BIN_OP_EQUAL, jjs_boolean (ctx (), true), jjs_boolean (ctx (), true), true),
    T (JJS_BIN_OP_EQUAL, jjs_boolean (ctx (), true), jjs_boolean (ctx (), false), false),
    T (JJS_BIN_OP_EQUAL, jjs_boolean (ctx (), false), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_EQUAL, jjs_boolean (ctx (), false), jjs_boolean (ctx (), false), true),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (ctx (), obj1), jjs_value_copy (ctx (), obj1), true),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (ctx (), obj1), jjs_value_copy (ctx (), obj2), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (ctx (), obj2), jjs_value_copy (ctx (), obj1), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (ctx (), obj1), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (ctx (), obj1), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (ctx (), obj1), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (ctx (), obj1), jjs_boolean (ctx (), false), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (ctx (), obj1), jjs_number (ctx (), 5.0), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (ctx (), obj1), jjs_string_sz (ctx (), "example string"), false),

    /* Testing less comparison */
    T (JJS_BIN_OP_LESS, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.0), false),
    T (JJS_BIN_OP_LESS, jjs_number (ctx (), 3.1), jjs_number (ctx (), 10), true),
    T (JJS_BIN_OP_LESS, jjs_number (ctx (), 3.1), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_LESS, jjs_number (ctx (), 3.1), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_LESS, jjs_string_sz (ctx (), "1"), jjs_string_sz (ctx (), "2"), true),
    T (JJS_BIN_OP_LESS, jjs_string_sz (ctx (), "1"), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_LESS, jjs_string_sz (ctx (), "1"), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_LESS, jjs_string_sz (ctx (), "1"), jjs_number (ctx (), 5.0), true),
    T (JJS_BIN_OP_LESS, jjs_undefined (ctx ()), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_LESS, jjs_undefined (ctx ()), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_LESS, jjs_null (ctx ()), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_LESS, jjs_boolean (ctx (), true), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_LESS, jjs_boolean (ctx (), true), jjs_boolean (ctx (), false), false),
    T (JJS_BIN_OP_LESS, jjs_boolean (ctx (), false), jjs_boolean (ctx (), true), true),
    T (JJS_BIN_OP_LESS, jjs_boolean (ctx (), false), jjs_boolean (ctx (), false), false),

    /* Testing less or equal comparison */
    T (JJS_BIN_OP_LESS_EQUAL, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.0), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_number (ctx (), 5.1), jjs_number (ctx (), 5.0), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_number (ctx (), 3.1), jjs_number (ctx (), 10), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_number (ctx (), 3.1), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_number (ctx (), 3.1), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz (ctx (), "1"), jjs_string_sz (ctx (), "2"), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz (ctx (), "1"), jjs_string_sz (ctx (), "1"), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz (ctx (), "1"), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz (ctx (), "1"), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz (ctx (), "1"), jjs_number (ctx (), 5.0), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz (ctx (), "5.0"), jjs_number (ctx (), 5.0), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_undefined (ctx ()), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_undefined (ctx ()), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_null (ctx ()), jjs_null (ctx ()), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_boolean (ctx (), true), jjs_boolean (ctx (), true), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_boolean (ctx (), true), jjs_boolean (ctx (), false), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_boolean (ctx (), false), jjs_boolean (ctx (), true), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_boolean (ctx (), false), jjs_boolean (ctx (), false), true),

    /* Testing greater comparison */
    T (JJS_BIN_OP_GREATER, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.0), false),
    T (JJS_BIN_OP_GREATER, jjs_number (ctx (), 10), jjs_number (ctx (), 3.1), true),
    T (JJS_BIN_OP_GREATER, jjs_number (ctx (), 3.1), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_GREATER, jjs_number (ctx (), 3.1), jjs_boolean (ctx (), true), true),
    T (JJS_BIN_OP_GREATER, jjs_string_sz (ctx (), "2"), jjs_string_sz (ctx (), "1"), true),
    T (JJS_BIN_OP_GREATER, jjs_string_sz (ctx (), "1"), jjs_string_sz (ctx (), "2"), false),
    T (JJS_BIN_OP_GREATER, jjs_string_sz (ctx (), "1"), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_GREATER, jjs_string_sz (ctx (), "1"), jjs_null (ctx ()), true),
    T (JJS_BIN_OP_GREATER, jjs_number (ctx (), 5.0), jjs_string_sz (ctx (), "1"), true),
    T (JJS_BIN_OP_GREATER, jjs_undefined (ctx ()), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_GREATER, jjs_undefined (ctx ()), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_GREATER, jjs_null (ctx ()), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_GREATER, jjs_boolean (ctx (), true), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_GREATER, jjs_boolean (ctx (), true), jjs_boolean (ctx (), false), true),
    T (JJS_BIN_OP_GREATER, jjs_boolean (ctx (), false), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_GREATER, jjs_boolean (ctx (), false), jjs_boolean (ctx (), false), false),

    /* Testing greater or equal comparison */
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.0), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.1), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (ctx (), 10), jjs_number (ctx (), 3.1), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (ctx (), 3.1), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (ctx (), 3.1), jjs_boolean (ctx (), true), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_string_sz (ctx (), "2"), jjs_string_sz (ctx (), "1"), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_string_sz (ctx (), "1"), jjs_string_sz (ctx (), "1"), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_string_sz (ctx (), "1"), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_string_sz (ctx (), "1"), jjs_null (ctx ()), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (ctx (), 5.0), jjs_string_sz (ctx (), "1"), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_string_sz (ctx (), "5.0"), jjs_number (ctx (), 5.0), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_undefined (ctx ()), jjs_undefined (ctx ()), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_undefined (ctx ()), jjs_null (ctx ()), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_null (ctx ()), jjs_null (ctx ()), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_boolean (ctx (), true), jjs_boolean (ctx (), true), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_boolean (ctx (), true), jjs_boolean (ctx (), false), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_boolean (ctx (), false), jjs_boolean (ctx (), true), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_boolean (ctx (), false), jjs_boolean (ctx (), false), true),
  };

  for (uint32_t idx = 0; idx < sizeof (tests) / sizeof (test_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (ctx(), tests[idx].op, tests[idx].lhs, JJS_MOVE, tests[idx].rhs, JJS_MOVE);
    TEST_ASSERT (!jjs_value_is_exception (ctx(), result));
    TEST_ASSERT (jjs_value_is_true (ctx(), result) == tests[idx].expected);
    jjs_value_free (ctx (), result);
  }

  test_entry_t error_tests[] = {
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (ctx (), err1), jjs_value_copy (ctx (), err1), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (ctx (), err1), jjs_undefined (ctx ()), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_undefined (ctx ()), jjs_value_copy (ctx (), err1), true),
  };

  for (uint32_t idx = 0; idx < sizeof (error_tests) / sizeof (test_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (ctx(), tests[idx].op, error_tests[idx].lhs, JJS_MOVE, error_tests[idx].rhs, JJS_MOVE);
    TEST_ASSERT (jjs_value_is_exception (ctx(), result) == error_tests[idx].expected);
    jjs_value_free (ctx (), result);
  }

  jjs_value_free (ctx (), obj1);
  jjs_value_free (ctx (), obj2);
  jjs_value_free (ctx (), err1);

  ctx_close ();

  return 0;
} /* main */
