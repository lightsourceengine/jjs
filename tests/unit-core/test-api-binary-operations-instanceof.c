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
  return jjs_undefined ();
} /* my_constructor */

int
main (void)
{
  TEST_INIT ();

  TEST_CONTEXT_NEW (context_p);

  jjs_value_t base_obj = jjs_object ();
  jjs_value_t constructor = jjs_function_external (my_constructor);

  jjs_value_t no_proto_instance_val = jjs_construct (constructor, NULL, 0);

  jjs_value_t prototype_str = jjs_string_sz ("prototype");
  jjs_value_t res = jjs_object_set (constructor, prototype_str, base_obj);
  jjs_value_free (prototype_str);
  TEST_ASSERT (!jjs_value_is_exception (res));
  jjs_value_free (res);

  jjs_value_t instance_val = jjs_construct (constructor, NULL, 0);

  jjs_value_t error = jjs_throw_value (base_obj, false);

  test_entry_t bool_tests[] = { T (jjs_value_copy (instance_val), jjs_value_copy (constructor), true),
                                T (jjs_value_copy (no_proto_instance_val), jjs_value_copy (constructor), false),
                                T (jjs_value_copy (base_obj), jjs_value_copy (constructor), false) };

  for (uint32_t idx = 0; idx < sizeof (bool_tests) / sizeof (test_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (JJS_BIN_OP_INSTANCEOF, bool_tests[idx].lhs, bool_tests[idx].rhs);
    TEST_ASSERT (!jjs_value_is_exception (result));
    TEST_ASSERT (jjs_value_is_true (result) == bool_tests[idx].expected);
    jjs_value_free (bool_tests[idx].lhs);
    jjs_value_free (bool_tests[idx].rhs);
    jjs_value_free (result);
  }

  test_entry_t error_tests[] = { T (jjs_value_copy (constructor), jjs_value_copy (instance_val), true),
                                 T (jjs_undefined (), jjs_value_copy (constructor), true),
                                 T (jjs_value_copy (instance_val), jjs_undefined (), true),
                                 T (jjs_value_copy (instance_val), jjs_value_copy (base_obj), true),
                                 T (jjs_value_copy (error), jjs_value_copy (constructor), true),
                                 T (jjs_value_copy (instance_val), jjs_value_copy (error), true),
                                 T (jjs_string_sz (""), jjs_string_sz (""), true),
                                 T (jjs_string_sz (""), jjs_number (5.0), true),
                                 T (jjs_number (5.0), jjs_string_sz (""), true),
                                 T (jjs_array (1), jjs_array (1), true),
                                 T (jjs_array (1), jjs_object (), true),
                                 T (jjs_object (), jjs_array (1), true),
                                 T (jjs_null (), jjs_object (), true),
                                 T (jjs_object (), jjs_string_sz (""), true) };

  for (uint32_t idx = 0; idx < sizeof (error_tests) / sizeof (test_entry_t); idx++)
  {
    jjs_value_t result = jjs_binary_op (JJS_BIN_OP_INSTANCEOF, error_tests[idx].lhs, error_tests[idx].rhs);
    TEST_ASSERT (jjs_value_is_exception (result) == error_tests[idx].expected);
    jjs_value_free (error_tests[idx].lhs);
    jjs_value_free (error_tests[idx].rhs);
    jjs_value_free (result);
  }

  jjs_value_free (base_obj);
  jjs_value_free (constructor);
  jjs_value_free (error);
  jjs_value_free (instance_val);
  jjs_value_free (no_proto_instance_val);

  TEST_CONTEXT_FREE (context_p);

  return 0;
} /* main */
