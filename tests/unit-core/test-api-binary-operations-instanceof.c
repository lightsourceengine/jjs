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

#define T(lhs, rhs, res) \
  {                      \
    lhs, rhs, res        \
  }

typedef struct
{
  jjs_value_t lhs;
  jjs_value_t rhs;
  bool expected;
} test_entry_t;

static jjs_value_t
my_constructor (const jjs_call_info_t *call_info_p, /**< call information */
                const jjs_value_t argv[], /**< arguments */
                const jjs_length_t argc) /**< number of arguments */
{
  (void) call_info_p;
  (void) argv;
  (void) argc;
  return jjs_undefined (ctx ());
} /* my_constructor */

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t base_obj = jjs_object (ctx ());
  jjs_value_t constructor = jjs_function_external (ctx (), my_constructor);

  jjs_value_t no_proto_instance_val = jjs_construct_noargs (ctx (), constructor);

  jjs_value_t res = jjs_object_set_sz (ctx (), constructor, "prototype", base_obj, JJS_KEEP);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), res));
  jjs_value_free (ctx (), res);

  jjs_value_t instance_val = jjs_construct_noargs (ctx (), constructor);

  jjs_value_t error = jjs_throw_value (ctx (), base_obj, JJS_KEEP);

  test_entry_t bool_tests[] = { T (jjs_value_copy (ctx (), instance_val), jjs_value_copy (ctx (), constructor), true),
                                T (jjs_value_copy (ctx (), no_proto_instance_val), jjs_value_copy (ctx (), constructor), false),
                                T (jjs_value_copy (ctx (), base_obj), jjs_value_copy (ctx (), constructor), false) };

  for (uint32_t idx = 0; idx < sizeof (bool_tests) / sizeof (test_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (ctx (), JJS_BIN_OP_INSTANCEOF, bool_tests[idx].lhs, JJS_MOVE, bool_tests[idx].rhs, JJS_MOVE);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
    TEST_ASSERT (jjs_value_is_true (ctx (), result) == bool_tests[idx].expected);
    jjs_value_free (ctx (), result);
  }

  test_entry_t error_tests[] = { T (jjs_value_copy (ctx (), constructor), jjs_value_copy (ctx (), instance_val), true),
                                 T (jjs_undefined (ctx ()), jjs_value_copy (ctx (), constructor), true),
                                 T (jjs_value_copy (ctx (), instance_val), jjs_undefined (ctx ()), true),
                                 T (jjs_value_copy (ctx (), instance_val), jjs_value_copy (ctx (), base_obj), true),
                                 T (jjs_value_copy (ctx (), error), jjs_value_copy (ctx (), constructor), true),
                                 T (jjs_value_copy (ctx (), instance_val), jjs_value_copy (ctx (), error), true),
                                 T (jjs_string_sz (ctx (), ""), jjs_string_sz (ctx (), ""), true),
                                 T (jjs_string_sz (ctx (), ""), jjs_number (ctx (), 5.0), true),
                                 T (jjs_number (ctx (), 5.0), jjs_string_sz (ctx (), ""), true),
                                 T (jjs_array (ctx (), 1), jjs_array (ctx (), 1), true),
                                 T (jjs_array (ctx (), 1), jjs_object (ctx ()), true),
                                 T (jjs_object (ctx ()), jjs_array (ctx (), 1), true),
                                 T (jjs_null (ctx ()), jjs_object (ctx ()), true),
                                 T (jjs_object (ctx ()), jjs_string_sz (ctx (), ""), true) };

  for (uint32_t idx = 0; idx < sizeof (error_tests) / sizeof (test_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (ctx (), JJS_BIN_OP_INSTANCEOF, error_tests[idx].lhs, JJS_MOVE, error_tests[idx].rhs, JJS_MOVE);
    TEST_ASSERT (jjs_value_is_exception (ctx (), result) == error_tests[idx].expected);
    jjs_value_free (ctx (), result);
  }

  jjs_value_free (ctx (), base_obj);
  jjs_value_free (ctx (), constructor);
  jjs_value_free (ctx (), error);
  jjs_value_free (ctx (), instance_val);
  jjs_value_free (ctx (), no_proto_instance_val);

  ctx_close ();

  return 0;
} /* main */
