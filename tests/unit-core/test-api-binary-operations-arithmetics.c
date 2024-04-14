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

#define T_NAN(op, lhs, rhs) \
  {                         \
    op, lhs, rhs            \
  }

#define T_ERR(op, lsh, rhs) T_NAN (op, lsh, rhs)

#define T_ARI(lhs, rhs)                                                                                       \
  T_NAN (JJS_BIN_OP_SUB, lhs, rhs), T_NAN (JJS_BIN_OP_MUL, lhs, rhs), T_NAN (JJS_BIN_OP_DIV, lhs, rhs), \
    T_NAN (JJS_BIN_OP_REM, lhs, rhs)

typedef struct
{
  jjs_binary_op_t op;
  jjs_value_t lhs;
  jjs_value_t rhs;
  jjs_value_t expected;
} test_entry_t;

typedef struct
{
  jjs_binary_op_t op;
  jjs_value_t lhs;
  jjs_value_t rhs;
} test_nan_entry_t;

typedef test_nan_entry_t test_error_entry_t;

int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

  jjs_value_t obj1 = jjs_eval ((jjs_char_t *) "o={x:1};o", 9, JJS_PARSE_NO_OPTS);
  jjs_value_t obj2 = jjs_eval ((jjs_char_t *) "o={x:1};o", 9, JJS_PARSE_NO_OPTS);
  jjs_value_t err1 = jjs_throw_sz (JJS_ERROR_SYNTAX, "error");

  test_nan_entry_t test_nans[] = {
    /* Testing addition (+) */
    T_NAN (JJS_BIN_OP_ADD, jjs_number (3.1), jjs_undefined ()),
    T_NAN (JJS_BIN_OP_ADD, jjs_undefined (), jjs_undefined ()),
    T_NAN (JJS_BIN_OP_ADD, jjs_undefined (), jjs_null ()),

    /* Testing subtraction (-), multiplication (*), division (/), remainder (%) */
    T_ARI (jjs_number (3.1), jjs_undefined ()),
    T_ARI (jjs_string_sz ("foo"), jjs_string_sz ("bar")),
    T_ARI (jjs_string_sz ("foo"), jjs_undefined ()),
    T_ARI (jjs_string_sz ("foo"), jjs_null ()),
    T_ARI (jjs_string_sz ("foo"), jjs_number (5.0)),
    T_ARI (jjs_undefined (), jjs_string_sz ("foo")),
    T_ARI (jjs_null (), jjs_string_sz ("foo")),
    T_ARI (jjs_number (5.0), jjs_string_sz ("foo")),
    T_ARI (jjs_undefined (), jjs_undefined ()),
    T_ARI (jjs_undefined (), jjs_null ()),
    T_ARI (jjs_null (), jjs_undefined ()),
    T_ARI (jjs_value_copy (obj1), jjs_value_copy (obj1)),
    T_ARI (jjs_value_copy (obj1), jjs_value_copy (obj2)),
    T_ARI (jjs_value_copy (obj2), jjs_value_copy (obj1)),
    T_ARI (jjs_value_copy (obj2), jjs_undefined ()),
    T_ARI (jjs_value_copy (obj1), jjs_string_sz ("foo")),
    T_ARI (jjs_value_copy (obj1), jjs_null ()),
    T_ARI (jjs_value_copy (obj1), jjs_boolean (true)),
    T_ARI (jjs_value_copy (obj1), jjs_boolean (false)),
    T_ARI (jjs_value_copy (obj1), jjs_number (5.0)),

    /* Testing division (/) */
    T_NAN (JJS_BIN_OP_DIV, jjs_boolean (false), jjs_boolean (false)),
    T_NAN (JJS_BIN_OP_DIV, jjs_number (0.0), jjs_number (0.0)),
    T_NAN (JJS_BIN_OP_DIV, jjs_null (), jjs_null ()),

    /* Testing remainder (%) */
    T_NAN (JJS_BIN_OP_REM, jjs_boolean (true), jjs_boolean (false)),
    T_NAN (JJS_BIN_OP_REM, jjs_boolean (false), jjs_boolean (false)),
    T_NAN (JJS_BIN_OP_REM, jjs_number (0.0), jjs_number (0.0)),
    T_NAN (JJS_BIN_OP_REM, jjs_null (), jjs_null ()),
  };

  for (uint32_t idx = 0; idx < sizeof (test_nans) / sizeof (test_nan_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (test_nans[idx].op, test_nans[idx].lhs, test_nans[idx].rhs);
    TEST_ASSERT (jjs_value_is_number (result));

    double num = jjs_value_as_number (result);

    TEST_ASSERT (num != num);

    jjs_value_free (test_nans[idx].lhs);
    jjs_value_free (test_nans[idx].rhs);
    jjs_value_free (result);
  }

  test_entry_t tests[] = {
    /* Testing addition (+) */
    T (JJS_BIN_OP_ADD, jjs_number (5.0), jjs_number (5.0), jjs_number (10.0)),
    T (JJS_BIN_OP_ADD, jjs_number (3.1), jjs_number (10), jjs_number (13.1)),
    T (JJS_BIN_OP_ADD, jjs_number (3.1), jjs_boolean (true), jjs_number (4.1)),
    T (JJS_BIN_OP_ADD, jjs_string_sz ("foo"), jjs_string_sz ("bar"), jjs_string_sz ("foobar")),
    T (JJS_BIN_OP_ADD, jjs_string_sz ("foo"), jjs_undefined (), jjs_string_sz ("fooundefined")),
    T (JJS_BIN_OP_ADD, jjs_string_sz ("foo"), jjs_null (), jjs_string_sz ("foonull")),
    T (JJS_BIN_OP_ADD, jjs_string_sz ("foo"), jjs_number (5.0), jjs_string_sz ("foo5")),

    T (JJS_BIN_OP_ADD, jjs_null (), jjs_null (), jjs_number (0.0)),
    T (JJS_BIN_OP_ADD, jjs_boolean (true), jjs_boolean (true), jjs_number (2.0)),
    T (JJS_BIN_OP_ADD, jjs_boolean (true), jjs_boolean (false), jjs_number (1.0)),
    T (JJS_BIN_OP_ADD, jjs_boolean (false), jjs_boolean (true), jjs_number (1.0)),
    T (JJS_BIN_OP_ADD, jjs_boolean (false), jjs_boolean (false), jjs_number (0.0)),
    T (JJS_BIN_OP_ADD,
       jjs_value_copy (obj1),
       jjs_value_copy (obj1),
       jjs_string_sz ("[object Object][object Object]")),
    T (JJS_BIN_OP_ADD,
       jjs_value_copy (obj1),
       jjs_value_copy (obj2),
       jjs_string_sz ("[object Object][object Object]")),
    T (JJS_BIN_OP_ADD,
       jjs_value_copy (obj2),
       jjs_value_copy (obj1),
       jjs_string_sz ("[object Object][object Object]")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (obj1), jjs_null (), jjs_string_sz ("[object Object]null")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (obj1), jjs_undefined (), jjs_string_sz ("[object Object]undefined")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (obj1), jjs_boolean (true), jjs_string_sz ("[object Object]true")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (obj1), jjs_boolean (false), jjs_string_sz ("[object Object]false")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (obj1), jjs_number (5.0), jjs_string_sz ("[object Object]5")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (obj1), jjs_string_sz ("foo"), jjs_string_sz ("[object Object]foo")),

    /* Testing subtraction (-) */
    T (JJS_BIN_OP_SUB, jjs_number (5.0), jjs_number (5.0), jjs_number (0.0)),
    T (JJS_BIN_OP_SUB, jjs_number (3.1), jjs_number (10), jjs_number (-6.9)),
    T (JJS_BIN_OP_SUB, jjs_number (3.1), jjs_boolean (true), jjs_number (2.1)),
    T (JJS_BIN_OP_SUB, jjs_boolean (true), jjs_boolean (true), jjs_number (0.0)),
    T (JJS_BIN_OP_SUB, jjs_boolean (true), jjs_boolean (false), jjs_number (1.0)),
    T (JJS_BIN_OP_SUB, jjs_boolean (false), jjs_boolean (true), jjs_number (-1.0)),
    T (JJS_BIN_OP_SUB, jjs_boolean (false), jjs_boolean (false), jjs_number (0.0)),
    T (JJS_BIN_OP_SUB, jjs_null (), jjs_null (), jjs_number (-0.0)),

    /* Testing multiplication (*) */
    T (JJS_BIN_OP_MUL, jjs_number (5.0), jjs_number (5.0), jjs_number (25.0)),
    T (JJS_BIN_OP_MUL, jjs_number (3.1), jjs_number (10), jjs_number (31)),
    T (JJS_BIN_OP_MUL, jjs_number (3.1), jjs_boolean (true), jjs_number (3.1)),
    T (JJS_BIN_OP_MUL, jjs_boolean (true), jjs_boolean (true), jjs_number (1.0)),
    T (JJS_BIN_OP_MUL, jjs_boolean (true), jjs_boolean (false), jjs_number (0.0)),
    T (JJS_BIN_OP_MUL, jjs_boolean (false), jjs_boolean (true), jjs_number (0.0)),
    T (JJS_BIN_OP_MUL, jjs_boolean (false), jjs_boolean (false), jjs_number (0.0)),
    T (JJS_BIN_OP_MUL, jjs_null (), jjs_null (), jjs_number (0.0)),

    /* Testing division (/) */
    T (JJS_BIN_OP_DIV, jjs_number (5.0), jjs_number (5.0), jjs_number (1.0)),
    T (JJS_BIN_OP_DIV, jjs_number (3.1), jjs_number (10), jjs_number (0.31)),
    T (JJS_BIN_OP_DIV, jjs_number (3.1), jjs_boolean (true), jjs_number (3.1)),
    T (JJS_BIN_OP_DIV, jjs_boolean (true), jjs_boolean (true), jjs_number (1.0)),
    T (JJS_BIN_OP_DIV, jjs_boolean (true), jjs_boolean (false), jjs_infinity (false)),
    T (JJS_BIN_OP_DIV, jjs_boolean (false), jjs_boolean (true), jjs_number (0.0)),

    /* Testing remainder (%) */
    T (JJS_BIN_OP_REM, jjs_number (5.0), jjs_number (5.0), jjs_number (0.0)),
    T (JJS_BIN_OP_REM, jjs_number (5.0), jjs_number (2.0), jjs_number (1.0)),
    T (JJS_BIN_OP_REM, jjs_number (3.1), jjs_number (10), jjs_number (3.1)),
    T (JJS_BIN_OP_REM, jjs_number (3.1), jjs_boolean (true), jjs_number (0.10000000000000009)),
    T (JJS_BIN_OP_REM, jjs_boolean (true), jjs_boolean (true), jjs_number (0.0)),
    T (JJS_BIN_OP_REM, jjs_boolean (false), jjs_boolean (true), jjs_number (0.0)),

  };

  for (uint32_t idx = 0; idx < sizeof (tests) / sizeof (test_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (tests[idx].op, tests[idx].lhs, tests[idx].rhs);
    TEST_ASSERT (!jjs_value_is_exception (result));

    jjs_value_t equals = jjs_binary_op (JJS_BIN_OP_STRICT_EQUAL, result, tests[idx].expected);
    TEST_ASSERT (jjs_value_is_boolean (equals) && jjs_value_is_true (equals));
    jjs_value_free (equals);

    jjs_value_free (tests[idx].lhs);
    jjs_value_free (tests[idx].rhs);
    jjs_value_free (tests[idx].expected);
    jjs_value_free (result);
  }

  jjs_value_t obj3 = jjs_eval ((jjs_char_t *) "o={valueOf:function(){throw 5}};o", 33, JJS_PARSE_NO_OPTS);

  test_error_entry_t error_tests[] = {
    /* Testing addition (+) */
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (err1), jjs_value_copy (err1)),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (err1), jjs_undefined ()),
    T_ERR (JJS_BIN_OP_ADD, jjs_undefined (), jjs_value_copy (err1)),

    /* Testing subtraction (-), multiplication (*), division (/), remainder (%) */
    T_ARI (jjs_value_copy (err1), jjs_value_copy (err1)),
    T_ARI (jjs_value_copy (err1), jjs_undefined ()),
    T_ARI (jjs_undefined (), jjs_value_copy (err1)),

    /* Testing addition (+) */
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (obj3), jjs_undefined ()),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (obj3), jjs_null ()),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (obj3), jjs_boolean (true)),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (obj3), jjs_boolean (false)),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (obj3), jjs_value_copy (obj2)),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (obj3), jjs_string_sz ("foo")),

    /* Testing subtraction (-), multiplication (*), division (/), remainder (%) */
    T_ARI (jjs_value_copy (obj3), jjs_undefined ()),
    T_ARI (jjs_value_copy (obj3), jjs_null ()),
    T_ARI (jjs_value_copy (obj3), jjs_boolean (true)),
    T_ARI (jjs_value_copy (obj3), jjs_boolean (false)),
    T_ARI (jjs_value_copy (obj3), jjs_value_copy (obj2)),
    T_ARI (jjs_value_copy (obj3), jjs_string_sz ("foo")),
  };

  for (uint32_t idx = 0; idx < sizeof (error_tests) / sizeof (test_error_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (tests[idx].op, error_tests[idx].lhs, error_tests[idx].rhs);
    TEST_ASSERT (jjs_value_is_exception (result));
    jjs_value_free (error_tests[idx].lhs);
    jjs_value_free (error_tests[idx].rhs);
    jjs_value_free (result);
  }

  jjs_value_free (obj1);
  jjs_value_free (obj2);
  jjs_value_free (obj3);
  jjs_value_free (err1);

  jjs_cleanup ();

  return 0;
} /* main */
