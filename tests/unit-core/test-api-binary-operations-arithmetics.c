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
  ctx_open (NULL);

  jjs_value_t obj1 = jjs_eval_sz (ctx (), "o={x:1};o", JJS_PARSE_NO_OPTS);
  jjs_value_t obj2 = jjs_eval_sz (ctx (), "o={x:1};o", JJS_PARSE_NO_OPTS);
  jjs_value_t err1 = jjs_throw_sz (ctx (), JJS_ERROR_SYNTAX, "error");

  test_nan_entry_t test_nans[] = {
    /* Testing addition (+) */
    T_NAN (JJS_BIN_OP_ADD, jjs_number (ctx (), 3.1), jjs_undefined (ctx ())),
    T_NAN (JJS_BIN_OP_ADD, jjs_undefined (ctx ()), jjs_undefined (ctx ())),
    T_NAN (JJS_BIN_OP_ADD, jjs_undefined (ctx ()), jjs_null (ctx ())),

    /* Testing subtraction (-), multiplication (*), division (/), remainder (%) */
    T_ARI (jjs_number (ctx (), 3.1), jjs_undefined (ctx ())),
    T_ARI (jjs_string_sz (ctx (), "foo"), jjs_string_sz (ctx (), "bar")),
    T_ARI (jjs_string_sz (ctx (), "foo"), jjs_undefined (ctx ())),
    T_ARI (jjs_string_sz (ctx (), "foo"), jjs_null (ctx ())),
    T_ARI (jjs_string_sz (ctx (), "foo"), jjs_number (ctx (), 5.0)),
    T_ARI (jjs_undefined (ctx ()), jjs_string_sz (ctx (), "foo")),
    T_ARI (jjs_null (ctx ()), jjs_string_sz (ctx (), "foo")),
    T_ARI (jjs_number (ctx (), 5.0), jjs_string_sz (ctx (), "foo")),
    T_ARI (jjs_undefined (ctx ()), jjs_undefined (ctx ())),
    T_ARI (jjs_undefined (ctx ()), jjs_null (ctx ())),
    T_ARI (jjs_null (ctx ()), jjs_undefined (ctx ())),
    T_ARI (jjs_value_copy (ctx (), obj1), jjs_value_copy (ctx (), obj1)),
    T_ARI (jjs_value_copy (ctx (), obj1), jjs_value_copy (ctx (), obj2)),
    T_ARI (jjs_value_copy (ctx (), obj2), jjs_value_copy (ctx (), obj1)),
    T_ARI (jjs_value_copy (ctx (), obj2), jjs_undefined (ctx ())),
    T_ARI (jjs_value_copy (ctx (), obj1), jjs_string_sz (ctx (), "foo")),
    T_ARI (jjs_value_copy (ctx (), obj1), jjs_null (ctx ())),
    T_ARI (jjs_value_copy (ctx (), obj1), jjs_boolean (ctx (), true)),
    T_ARI (jjs_value_copy (ctx (), obj1), jjs_boolean (ctx (), false)),
    T_ARI (jjs_value_copy (ctx (), obj1), jjs_number (ctx (), 5.0)),

    /* Testing division (/) */
    T_NAN (JJS_BIN_OP_DIV, jjs_boolean (ctx (), false), jjs_boolean (ctx (), false)),
    T_NAN (JJS_BIN_OP_DIV, jjs_number (ctx (), 0.0), jjs_number (ctx (), 0.0)),
    T_NAN (JJS_BIN_OP_DIV, jjs_null (ctx ()), jjs_null (ctx ())),

    /* Testing remainder (%) */
    T_NAN (JJS_BIN_OP_REM, jjs_boolean (ctx (), true), jjs_boolean (ctx (), false)),
    T_NAN (JJS_BIN_OP_REM, jjs_boolean (ctx (), false), jjs_boolean (ctx (), false)),
    T_NAN (JJS_BIN_OP_REM, jjs_number (ctx (), 0.0), jjs_number (ctx (), 0.0)),
    T_NAN (JJS_BIN_OP_REM, jjs_null (ctx ()), jjs_null (ctx ())),
  };

  for (uint32_t idx = 0; idx < sizeof (test_nans) / sizeof (test_nan_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (ctx (), test_nans[idx].op, test_nans[idx].lhs, JJS_KEEP, test_nans[idx].rhs, JJS_KEEP);
    TEST_ASSERT (jjs_value_is_number (ctx (), result));

    double num = jjs_value_as_number (ctx (), result);

    TEST_ASSERT (num != num);

    jjs_value_free (ctx (), test_nans[idx].lhs);
    jjs_value_free (ctx (), test_nans[idx].rhs);
    jjs_value_free (ctx (), result);
  }

  test_entry_t tests[] = {
    /* Testing addition (+) */
    T (JJS_BIN_OP_ADD, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.0), jjs_number (ctx (), 10.0)),
    T (JJS_BIN_OP_ADD, jjs_number (ctx (), 3.1), jjs_number (ctx (), 10), jjs_number (ctx (), 13.1)),
    T (JJS_BIN_OP_ADD, jjs_number (ctx (), 3.1), jjs_boolean (ctx (), true), jjs_number (ctx (), 4.1)),
    T (JJS_BIN_OP_ADD, jjs_string_sz (ctx (), "foo"), jjs_string_sz (ctx (), "bar"), jjs_string_sz (ctx (), "foobar")),
    T (JJS_BIN_OP_ADD, jjs_string_sz (ctx (), "foo"), jjs_undefined (ctx ()), jjs_string_sz (ctx (), "fooundefined")),
    T (JJS_BIN_OP_ADD, jjs_string_sz (ctx (), "foo"), jjs_null (ctx ()), jjs_string_sz (ctx (), "foonull")),
    T (JJS_BIN_OP_ADD, jjs_string_sz (ctx (), "foo"), jjs_number (ctx (), 5.0), jjs_string_sz (ctx (), "foo5")),

    T (JJS_BIN_OP_ADD, jjs_null (ctx ()), jjs_null (ctx ()), jjs_number (ctx (), 0.0)),
    T (JJS_BIN_OP_ADD, jjs_boolean (ctx (), true), jjs_boolean (ctx (), true), jjs_number (ctx (), 2.0)),
    T (JJS_BIN_OP_ADD, jjs_boolean (ctx (), true), jjs_boolean (ctx (), false), jjs_number (ctx (), 1.0)),
    T (JJS_BIN_OP_ADD, jjs_boolean (ctx (), false), jjs_boolean (ctx (), true), jjs_number (ctx (), 1.0)),
    T (JJS_BIN_OP_ADD, jjs_boolean (ctx (), false), jjs_boolean (ctx (), false), jjs_number (ctx (), 0.0)),
    T (JJS_BIN_OP_ADD,
       jjs_value_copy (ctx (), obj1),
       jjs_value_copy (ctx (), obj1),
       jjs_string_sz (ctx (), "[object Object][object Object]")),
    T (JJS_BIN_OP_ADD,
       jjs_value_copy (ctx (), obj1),
       jjs_value_copy (ctx (), obj2),
       jjs_string_sz (ctx (), "[object Object][object Object]")),
    T (JJS_BIN_OP_ADD,
       jjs_value_copy (ctx (), obj2),
       jjs_value_copy (ctx (), obj1),
       jjs_string_sz (ctx (), "[object Object][object Object]")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj1), jjs_null (ctx ()), jjs_string_sz (ctx (), "[object Object]null")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj1), jjs_undefined (ctx ()), jjs_string_sz (ctx (), "[object Object]undefined")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj1), jjs_boolean (ctx (), true), jjs_string_sz (ctx (), "[object Object]true")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj1), jjs_boolean (ctx (), false), jjs_string_sz (ctx (), "[object Object]false")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj1), jjs_number (ctx (), 5.0), jjs_string_sz (ctx (), "[object Object]5")),
    T (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj1), jjs_string_sz (ctx (), "foo"), jjs_string_sz (ctx (), "[object Object]foo")),

    /* Testing subtraction (-) */
    T (JJS_BIN_OP_SUB, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.0), jjs_number (ctx (), 0.0)),
    T (JJS_BIN_OP_SUB, jjs_number (ctx (), 3.1), jjs_number (ctx (), 10), jjs_number (ctx (), -6.9)),
    T (JJS_BIN_OP_SUB, jjs_number (ctx (), 3.1), jjs_boolean (ctx (), true), jjs_number (ctx (), 2.1)),
    T (JJS_BIN_OP_SUB, jjs_boolean (ctx (), true), jjs_boolean (ctx (), true), jjs_number (ctx (), 0.0)),
    T (JJS_BIN_OP_SUB, jjs_boolean (ctx (), true), jjs_boolean (ctx (), false), jjs_number (ctx (), 1.0)),
    T (JJS_BIN_OP_SUB, jjs_boolean (ctx (), false), jjs_boolean (ctx (), true), jjs_number (ctx (), -1.0)),
    T (JJS_BIN_OP_SUB, jjs_boolean (ctx (), false), jjs_boolean (ctx (), false), jjs_number (ctx (), 0.0)),
    T (JJS_BIN_OP_SUB, jjs_null (ctx ()), jjs_null (ctx ()), jjs_number (ctx (), -0.0)),

    /* Testing multiplication (*) */
    T (JJS_BIN_OP_MUL, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.0), jjs_number (ctx (), 25.0)),
    T (JJS_BIN_OP_MUL, jjs_number (ctx (), 3.1), jjs_number (ctx (), 10), jjs_number (ctx (), 31)),
    T (JJS_BIN_OP_MUL, jjs_number (ctx (), 3.1), jjs_boolean (ctx (), true), jjs_number (ctx (), 3.1)),
    T (JJS_BIN_OP_MUL, jjs_boolean (ctx (), true), jjs_boolean (ctx (), true), jjs_number (ctx (), 1.0)),
    T (JJS_BIN_OP_MUL, jjs_boolean (ctx (), true), jjs_boolean (ctx (), false), jjs_number (ctx (), 0.0)),
    T (JJS_BIN_OP_MUL, jjs_boolean (ctx (), false), jjs_boolean (ctx (), true), jjs_number (ctx (), 0.0)),
    T (JJS_BIN_OP_MUL, jjs_boolean (ctx (), false), jjs_boolean (ctx (), false), jjs_number (ctx (), 0.0)),
    T (JJS_BIN_OP_MUL, jjs_null (ctx ()), jjs_null (ctx ()), jjs_number (ctx (), 0.0)),

    /* Testing division (/) */
    T (JJS_BIN_OP_DIV, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.0), jjs_number (ctx (), 1.0)),
    T (JJS_BIN_OP_DIV, jjs_number (ctx (), 3.1), jjs_number (ctx (), 10), jjs_number (ctx (), 0.31)),
    T (JJS_BIN_OP_DIV, jjs_number (ctx (), 3.1), jjs_boolean (ctx (), true), jjs_number (ctx (), 3.1)),
    T (JJS_BIN_OP_DIV, jjs_boolean (ctx (), true), jjs_boolean (ctx (), true), jjs_number (ctx (), 1.0)),
    T (JJS_BIN_OP_DIV, jjs_boolean (ctx (), true), jjs_boolean (ctx (), false), jjs_infinity (ctx (), false)),
    T (JJS_BIN_OP_DIV, jjs_boolean (ctx (), false), jjs_boolean (ctx (), true), jjs_number (ctx (), 0.0)),

    /* Testing remainder (%) */
    T (JJS_BIN_OP_REM, jjs_number (ctx (), 5.0), jjs_number (ctx (), 5.0), jjs_number (ctx (), 0.0)),
    T (JJS_BIN_OP_REM, jjs_number (ctx (), 5.0), jjs_number (ctx (), 2.0), jjs_number (ctx (), 1.0)),
    T (JJS_BIN_OP_REM, jjs_number (ctx (), 3.1), jjs_number (ctx (), 10), jjs_number (ctx (), 3.1)),
    T (JJS_BIN_OP_REM, jjs_number (ctx (), 3.1), jjs_boolean (ctx (), true), jjs_number (ctx (), 0.10000000000000009)),
    T (JJS_BIN_OP_REM, jjs_boolean (ctx (), true), jjs_boolean (ctx (), true), jjs_number (ctx (), 0.0)),
    T (JJS_BIN_OP_REM, jjs_boolean (ctx (), false), jjs_boolean (ctx (), true), jjs_number (ctx (), 0.0)),

  };

  for (uint32_t idx = 0; idx < sizeof (tests) / sizeof (test_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (ctx (), tests[idx].op, tests[idx].lhs, JJS_KEEP, tests[idx].rhs, JJS_KEEP);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));

    jjs_value_t equals = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, result, JJS_KEEP, tests[idx].expected, JJS_KEEP);
    TEST_ASSERT (jjs_value_is_boolean (ctx (), equals) && jjs_value_is_true (ctx (), equals));
    jjs_value_free (ctx (), equals);

    jjs_value_free (ctx (), tests[idx].lhs);
    jjs_value_free (ctx (), tests[idx].rhs);
    jjs_value_free (ctx (), tests[idx].expected);
    jjs_value_free (ctx (), result);
  }

  jjs_value_t obj3 = jjs_eval_sz (ctx (), "o={valueOf:function(){throw 5}};o", JJS_PARSE_NO_OPTS);

  test_error_entry_t error_tests[] = {
    /* Testing addition (+) */
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), err1), jjs_value_copy (ctx (), err1)),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), err1), jjs_undefined (ctx ())),
    T_ERR (JJS_BIN_OP_ADD, jjs_undefined (ctx ()), jjs_value_copy (ctx (), err1)),

    /* Testing subtraction (-), multiplication (*), division (/), remainder (%) */
    T_ARI (jjs_value_copy (ctx (), err1), jjs_value_copy (ctx (), err1)),
    T_ARI (jjs_value_copy (ctx (), err1), jjs_undefined (ctx ())),
    T_ARI (jjs_undefined (ctx ()), jjs_value_copy (ctx (), err1)),

    /* Testing addition (+) */
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj3), jjs_undefined (ctx ())),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj3), jjs_null (ctx ())),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj3), jjs_boolean (ctx (), true)),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj3), jjs_boolean (ctx (), false)),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj3), jjs_value_copy (ctx (), obj2)),
    T_ERR (JJS_BIN_OP_ADD, jjs_value_copy (ctx (), obj3), jjs_string_sz (ctx (), "foo")),

    /* Testing subtraction (-), multiplication (*), division (/), remainder (%) */
    T_ARI (jjs_value_copy (ctx (), obj3), jjs_undefined (ctx ())),
    T_ARI (jjs_value_copy (ctx (), obj3), jjs_null (ctx ())),
    T_ARI (jjs_value_copy (ctx (), obj3), jjs_boolean (ctx (), true)),
    T_ARI (jjs_value_copy (ctx (), obj3), jjs_boolean (ctx (), false)),
    T_ARI (jjs_value_copy (ctx (), obj3), jjs_value_copy (ctx (), obj2)),
    T_ARI (jjs_value_copy (ctx (), obj3), jjs_string_sz (ctx (), "foo")),
  };

  for (uint32_t idx = 0; idx < sizeof (error_tests) / sizeof (test_error_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (ctx (), tests[idx].op, error_tests[idx].lhs, JJS_KEEP, error_tests[idx].rhs, JJS_KEEP);
    TEST_ASSERT (jjs_value_is_exception (ctx (), result));
    jjs_value_free (ctx (), error_tests[idx].lhs);
    jjs_value_free (ctx (), error_tests[idx].rhs);
    jjs_value_free (ctx (), result);
  }

  jjs_value_free (ctx (), obj1);
  jjs_value_free (ctx (), obj2);
  jjs_value_free (ctx (), obj3);
  jjs_value_free (ctx (), err1);

  ctx_close ();

  return 0;
} /* main */
