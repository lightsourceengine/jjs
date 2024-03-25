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

#include "jjs.h"

#include "test-common.h"

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
  TEST_INIT ();

  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  jjs_value_t obj1 = jjs_eval ((const jjs_char_t *) "o={x:1};o", 9, JJS_PARSE_NO_OPTS);
  jjs_value_t obj2 = jjs_eval ((const jjs_char_t *) "o={x:1};o", 9, JJS_PARSE_NO_OPTS);
  jjs_value_t err1 = jjs_throw_sz (JJS_ERROR_SYNTAX, "error");

  test_entry_t tests[] = {
    /* Testing strict equal comparison */
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_number (5.0), jjs_number (5.0), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_number (3.1), jjs_number (10), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_number (3.1), jjs_undefined (), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_number (3.1), jjs_boolean (true), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_string_sz ("example string"), jjs_string_sz ("example string"), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_string_sz ("example string"), jjs_undefined (), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_string_sz ("example string"), jjs_null (), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_string_sz ("example string"), jjs_number (5.0), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_undefined (), jjs_undefined (), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_undefined (), jjs_null (), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_null (), jjs_null (), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_boolean (true), jjs_boolean (true), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_boolean (true), jjs_boolean (false), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_boolean (false), jjs_boolean (true), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_boolean (false), jjs_boolean (false), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (obj1), jjs_value_copy (obj1), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (obj1), jjs_value_copy (obj2), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (obj2), jjs_value_copy (obj1), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (obj1), jjs_null (), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (obj1), jjs_undefined (), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (obj1), jjs_boolean (true), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (obj1), jjs_boolean (false), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (obj1), jjs_number (5.0), false),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (obj1), jjs_string_sz ("example string"), false),

    /* Testing equal comparison */
    T (JJS_BIN_OP_EQUAL, jjs_number (5.0), jjs_number (5.0), true),
    T (JJS_BIN_OP_EQUAL, jjs_number (3.1), jjs_number (10), false),
    T (JJS_BIN_OP_EQUAL, jjs_number (3.1), jjs_undefined (), false),
    T (JJS_BIN_OP_EQUAL, jjs_number (3.1), jjs_boolean (true), false),
    T (JJS_BIN_OP_EQUAL, jjs_string_sz ("example string"), jjs_string_sz ("example string"), true),
    T (JJS_BIN_OP_EQUAL, jjs_string_sz ("example string"), jjs_undefined (), false),
    T (JJS_BIN_OP_EQUAL, jjs_string_sz ("example string"), jjs_null (), false),
    T (JJS_BIN_OP_EQUAL, jjs_string_sz ("example string"), jjs_number (5.0), false),
    T (JJS_BIN_OP_EQUAL, jjs_undefined (), jjs_undefined (), true),
    T (JJS_BIN_OP_EQUAL, jjs_undefined (), jjs_null (), true),
    T (JJS_BIN_OP_EQUAL, jjs_null (), jjs_null (), true),
    T (JJS_BIN_OP_EQUAL, jjs_boolean (true), jjs_boolean (true), true),
    T (JJS_BIN_OP_EQUAL, jjs_boolean (true), jjs_boolean (false), false),
    T (JJS_BIN_OP_EQUAL, jjs_boolean (false), jjs_boolean (true), false),
    T (JJS_BIN_OP_EQUAL, jjs_boolean (false), jjs_boolean (false), true),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (obj1), jjs_value_copy (obj1), true),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (obj1), jjs_value_copy (obj2), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (obj2), jjs_value_copy (obj1), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (obj1), jjs_null (), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (obj1), jjs_undefined (), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (obj1), jjs_boolean (true), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (obj1), jjs_boolean (false), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (obj1), jjs_number (5.0), false),
    T (JJS_BIN_OP_EQUAL, jjs_value_copy (obj1), jjs_string_sz ("example string"), false),

    /* Testing less comparison */
    T (JJS_BIN_OP_LESS, jjs_number (5.0), jjs_number (5.0), false),
    T (JJS_BIN_OP_LESS, jjs_number (3.1), jjs_number (10), true),
    T (JJS_BIN_OP_LESS, jjs_number (3.1), jjs_undefined (), false),
    T (JJS_BIN_OP_LESS, jjs_number (3.1), jjs_boolean (true), false),
    T (JJS_BIN_OP_LESS, jjs_string_sz ("1"), jjs_string_sz ("2"), true),
    T (JJS_BIN_OP_LESS, jjs_string_sz ("1"), jjs_undefined (), false),
    T (JJS_BIN_OP_LESS, jjs_string_sz ("1"), jjs_null (), false),
    T (JJS_BIN_OP_LESS, jjs_string_sz ("1"), jjs_number (5.0), true),
    T (JJS_BIN_OP_LESS, jjs_undefined (), jjs_undefined (), false),
    T (JJS_BIN_OP_LESS, jjs_undefined (), jjs_null (), false),
    T (JJS_BIN_OP_LESS, jjs_null (), jjs_null (), false),
    T (JJS_BIN_OP_LESS, jjs_boolean (true), jjs_boolean (true), false),
    T (JJS_BIN_OP_LESS, jjs_boolean (true), jjs_boolean (false), false),
    T (JJS_BIN_OP_LESS, jjs_boolean (false), jjs_boolean (true), true),
    T (JJS_BIN_OP_LESS, jjs_boolean (false), jjs_boolean (false), false),

    /* Testing less or equal comparison */
    T (JJS_BIN_OP_LESS_EQUAL, jjs_number (5.0), jjs_number (5.0), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_number (5.1), jjs_number (5.0), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_number (3.1), jjs_number (10), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_number (3.1), jjs_undefined (), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_number (3.1), jjs_boolean (true), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz ("1"), jjs_string_sz ("2"), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz ("1"), jjs_string_sz ("1"), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz ("1"), jjs_undefined (), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz ("1"), jjs_null (), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz ("1"), jjs_number (5.0), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_string_sz ("5.0"), jjs_number (5.0), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_undefined (), jjs_undefined (), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_undefined (), jjs_null (), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_null (), jjs_null (), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_boolean (true), jjs_boolean (true), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_boolean (true), jjs_boolean (false), false),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_boolean (false), jjs_boolean (true), true),
    T (JJS_BIN_OP_LESS_EQUAL, jjs_boolean (false), jjs_boolean (false), true),

    /* Testing greater comparison */
    T (JJS_BIN_OP_GREATER, jjs_number (5.0), jjs_number (5.0), false),
    T (JJS_BIN_OP_GREATER, jjs_number (10), jjs_number (3.1), true),
    T (JJS_BIN_OP_GREATER, jjs_number (3.1), jjs_undefined (), false),
    T (JJS_BIN_OP_GREATER, jjs_number (3.1), jjs_boolean (true), true),
    T (JJS_BIN_OP_GREATER, jjs_string_sz ("2"), jjs_string_sz ("1"), true),
    T (JJS_BIN_OP_GREATER, jjs_string_sz ("1"), jjs_string_sz ("2"), false),
    T (JJS_BIN_OP_GREATER, jjs_string_sz ("1"), jjs_undefined (), false),
    T (JJS_BIN_OP_GREATER, jjs_string_sz ("1"), jjs_null (), true),
    T (JJS_BIN_OP_GREATER, jjs_number (5.0), jjs_string_sz ("1"), true),
    T (JJS_BIN_OP_GREATER, jjs_undefined (), jjs_undefined (), false),
    T (JJS_BIN_OP_GREATER, jjs_undefined (), jjs_null (), false),
    T (JJS_BIN_OP_GREATER, jjs_null (), jjs_null (), false),
    T (JJS_BIN_OP_GREATER, jjs_boolean (true), jjs_boolean (true), false),
    T (JJS_BIN_OP_GREATER, jjs_boolean (true), jjs_boolean (false), true),
    T (JJS_BIN_OP_GREATER, jjs_boolean (false), jjs_boolean (true), false),
    T (JJS_BIN_OP_GREATER, jjs_boolean (false), jjs_boolean (false), false),

    /* Testing greater or equal comparison */
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (5.0), jjs_number (5.0), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (5.0), jjs_number (5.1), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (10), jjs_number (3.1), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (3.1), jjs_undefined (), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (3.1), jjs_boolean (true), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_string_sz ("2"), jjs_string_sz ("1"), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_string_sz ("1"), jjs_string_sz ("1"), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_string_sz ("1"), jjs_undefined (), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_string_sz ("1"), jjs_null (), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_number (5.0), jjs_string_sz ("1"), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_string_sz ("5.0"), jjs_number (5.0), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_undefined (), jjs_undefined (), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_undefined (), jjs_null (), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_null (), jjs_null (), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_boolean (true), jjs_boolean (true), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_boolean (true), jjs_boolean (false), true),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_boolean (false), jjs_boolean (true), false),
    T (JJS_BIN_OP_GREATER_EQUAL, jjs_boolean (false), jjs_boolean (false), true),
  };

  for (uint32_t idx = 0; idx < sizeof (tests) / sizeof (test_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (tests[idx].op, tests[idx].lhs, tests[idx].rhs);
    TEST_ASSERT (!jjs_value_is_exception (result));
    TEST_ASSERT (jjs_value_is_true (result) == tests[idx].expected);
    jjs_value_free (tests[idx].lhs);
    jjs_value_free (tests[idx].rhs);
    jjs_value_free (result);
  }

  test_entry_t error_tests[] = {
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (err1), jjs_value_copy (err1), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_value_copy (err1), jjs_undefined (), true),
    T (JJS_BIN_OP_STRICT_EQUAL, jjs_undefined (), jjs_value_copy (err1), true),
  };

  for (uint32_t idx = 0; idx < sizeof (error_tests) / sizeof (test_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (tests[idx].op, error_tests[idx].lhs, error_tests[idx].rhs);
    TEST_ASSERT (jjs_value_is_exception (result) == error_tests[idx].expected);
    jjs_value_free (error_tests[idx].lhs);
    jjs_value_free (error_tests[idx].rhs);
    jjs_value_free (result);
  }

  jjs_value_free (obj1);
  jjs_value_free (obj2);
  jjs_value_free (err1);

  jjs_cleanup ();

  return 0;
} /* main */
