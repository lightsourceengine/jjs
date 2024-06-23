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

/**
 * Register a JavaScript function in the global object.
 */
static jjs_value_t
register_js_function (const char *name_p, /**< name of the function */
                      jjs_external_handler_t handler_p) /**< function callback */
{
  jjs_value_t global_obj_val = jjs_current_realm (ctx ());

  jjs_value_t function_val = jjs_function_external (ctx (), handler_p);
  jjs_value_t function_name_val = jjs_string_sz (ctx (), name_p);
  jjs_value_t result_val = jjs_object_set (ctx (), global_obj_val, function_name_val, function_val);

  jjs_value_free (ctx (), function_name_val);
  jjs_value_free (ctx (), global_obj_val);

  jjs_value_free (ctx (), result_val);

  return function_val;
} /* register_js_function */

enum
{
  TEST_ID_SIMPLE_CONSTRUCT = 1,
  TEST_ID_SIMPLE_CALL = 2,
  TEST_ID_CONSTRUCT_AND_CALL_SUB = 3,
};

static jjs_value_t
construct_handler (const jjs_call_info_t *call_info_p, /**< call information */
                   const jjs_value_t args_p[], /**< function arguments */
                   const jjs_length_t args_cnt) /**< number of function arguments */
{
  JJS_UNUSED (args_p);

  if (args_cnt != 1 || !jjs_value_is_number (ctx (), args_p[0]))
  {
    TEST_ASSERT (0 && "Invalid arguments for demo method");
  }

  int test_id = (int) jjs_value_as_number (ctx (), args_p[0]);

  switch (test_id)
  {
    case TEST_ID_SIMPLE_CONSTRUCT:
    {
      /* Method was called with "new": new.target should be equal to the function object. */
      jjs_value_t target = call_info_p->new_target;
      TEST_ASSERT (!jjs_value_is_undefined (ctx (), target));
      TEST_ASSERT (target == call_info_p->function);
      break;
    }
    case TEST_ID_SIMPLE_CALL:
    {
      /* Method was called directly without "new": new.target should be equal undefined. */
      jjs_value_t target = call_info_p->new_target;
      TEST_ASSERT (jjs_value_is_undefined (ctx (), target));
      TEST_ASSERT (target != call_info_p->function);
      break;
    }
    case TEST_ID_CONSTRUCT_AND_CALL_SUB:
    {
      /* Method was called with "new": new.target should be equal to the function object. */
      jjs_value_t target = call_info_p->new_target;
      TEST_ASSERT (!jjs_value_is_undefined (ctx (), target));
      TEST_ASSERT (target == call_info_p->function);

      /* Calling a function should hide the old "new.target". */
      jjs_value_t sub_arg = jjs_number (ctx (), TEST_ID_SIMPLE_CALL);
      jjs_value_t func_call_result;

      func_call_result = jjs_call (ctx (), call_info_p->function, call_info_p->this_value, &sub_arg, 1);
      TEST_ASSERT (!jjs_value_is_exception (ctx (), func_call_result));
      TEST_ASSERT (jjs_value_is_undefined (ctx (), func_call_result));
      break;
    }

    default:
    {
      TEST_ASSERT (0 && "Incorrect test ID");
      break;
    }
  }

  return jjs_undefined (ctx ());
} /* construct_handler */

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t demo_func = register_js_function ("Demo", construct_handler);

  {
    jjs_value_t test_arg = jjs_number (ctx (), TEST_ID_SIMPLE_CONSTRUCT);
    jjs_value_t constructed = jjs_construct (ctx (), demo_func, &test_arg, 1);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), constructed));
    TEST_ASSERT (jjs_value_is_object (ctx (), constructed));
    jjs_value_free (ctx (), test_arg);
    jjs_value_free (ctx (), constructed);
  }

  {
    jjs_value_t test_arg = jjs_number (ctx (), TEST_ID_SIMPLE_CALL);
    jjs_value_t this_arg = jjs_undefined (ctx ());
    jjs_value_t constructed = jjs_call (ctx (), demo_func, this_arg, &test_arg, 1);
    TEST_ASSERT (jjs_value_is_undefined (ctx (), constructed));
    jjs_value_free (ctx (), constructed);
    jjs_value_free (ctx (), constructed);
    jjs_value_free (ctx (), test_arg);
  }

  {
    jjs_value_t test_arg = jjs_number (ctx (), TEST_ID_CONSTRUCT_AND_CALL_SUB);
    jjs_value_t constructed = jjs_construct (ctx (), demo_func, &test_arg, 1);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), constructed));
    TEST_ASSERT (jjs_value_is_object (ctx (), constructed));
    jjs_value_free (ctx (), test_arg);
    jjs_value_free (ctx (), constructed);
  }

  {
    static const char test_source[] = TEST_STRING_LITERAL ("new Demo (1)");

    jjs_value_t parsed_code_val = jjs_parse_sz (ctx (), test_source, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), parsed_code_val));

    jjs_value_t res = jjs_run (ctx (), parsed_code_val, JJS_MOVE);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), res));

    jjs_value_free (ctx (), res);
  }

  {
    static const char test_source[] = TEST_STRING_LITERAL ("Demo (2)");

    jjs_value_t parsed_code_val = jjs_parse_sz (ctx (), test_source, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), parsed_code_val));

    jjs_value_t res = jjs_run (ctx (), parsed_code_val, JJS_MOVE);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), res));

    jjs_value_free (ctx (), res);
  }

  {
    static const char test_source[] = TEST_STRING_LITERAL ("function base(arg) { new Demo (arg); };"
                                                                   "base (1);"
                                                                   "new base(1);"
                                                                   "new base(3);");

    jjs_value_t parsed_code_val = jjs_parse_sz (ctx (), test_source, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), parsed_code_val));

    jjs_value_t res = jjs_run (ctx (), parsed_code_val, JJS_MOVE);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), res));

    jjs_value_free (ctx (), res);
  }

  jjs_value_free (ctx (), demo_func);
  ctx_close ();
  return 0;
} /* main */
