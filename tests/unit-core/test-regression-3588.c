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

/**
 * Empty constructor
 */
static jjs_value_t
construct_handler (const jjs_call_info_t *call_info_p, /**< call information */
                   const jjs_value_t args_p[], /**< function arguments */
                   const jjs_length_t args_cnt) /**< number of function arguments */
{
  JJS_UNUSED (call_info_p);

  TEST_ASSERT (args_cnt == 1);
  TEST_ASSERT (jjs_value_as_number (args_p[0]) == 1.0);

  return jjs_undefined ();
} /* construct_handler */

int
main (void)
{
  TEST_CONTEXT_NEW (context_p);

  {
    jjs_value_t global_obj_val = jjs_current_realm ();

    jjs_value_t function_val = jjs_function_external (construct_handler);
    jjs_value_t function_name_val = jjs_string_sz ("Demo");
    jjs_value_t result_val = jjs_object_set (global_obj_val, function_name_val, function_val);
    TEST_ASSERT (!jjs_value_is_exception (result_val));
    TEST_ASSERT (jjs_value_is_true (result_val));
    jjs_value_free (result_val);
    jjs_value_free (function_name_val);
    jjs_value_free (global_obj_val);
    jjs_value_free (function_val);
  }

  {
    static const jjs_char_t test_source[] =
      TEST_STRING_LITERAL ("class Sub1 extends Demo { constructor () { super (1); } };"
                           "new Sub1 ()");

    jjs_value_t parsed_code_val = jjs_parse (test_source, sizeof (test_source) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parsed_code_val));

    jjs_value_t result = jjs_run (parsed_code_val);
    TEST_ASSERT (!jjs_value_is_exception (result));

    jjs_value_free (result);
    jjs_value_free (parsed_code_val);
  }

  {
    static const jjs_char_t test_source[] = TEST_STRING_LITERAL ("class Sub2 extends Demo { };"
                                                                   "new Sub2 (1)");

    jjs_value_t parsed_code_val = jjs_parse (test_source, sizeof (test_source) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parsed_code_val));

    jjs_value_t result = jjs_run (parsed_code_val);
    TEST_ASSERT (!jjs_value_is_exception (result));

    jjs_value_free (result);
    jjs_value_free (parsed_code_val);
  }

  TEST_CONTEXT_FREE (context_p);
  return 0;
} /* main */
