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

static int mode = 0;
static int counter = 0;

static void
vm_throw_callback (jjs_context_t *context_p, /**< context */
                   const jjs_value_t error_value, /**< captured error */
                   void *user_p) /**< user pointer */
{
  TEST_ASSERT (user_p == (void *) &mode);
  counter++;

  switch (mode)
  {
    case 0:
    {
      TEST_ASSERT (counter == 1);
      TEST_ASSERT (jjs_value_is_number (context_p, error_value));
      TEST_ASSERT_DOUBLE_EQUALS (jjs_value_as_number (context_p, error_value), -5.6);
      break;
    }
    case 1:
    {
      TEST_ASSERT (counter == 1);
      TEST_ASSERT (jjs_value_is_null (context_p, error_value));
      break;
    }
    case 2:
    {
      jjs_char_t string_buf[2];
      jjs_size_t size = sizeof (string_buf);

      string_buf[0] = '\0';
      string_buf[1] = '\0';

      TEST_ASSERT (counter >= 1 && counter <= 3);
      TEST_ASSERT (jjs_value_is_string (context_p, error_value));
      TEST_ASSERT (jjs_string_size (context_p, error_value, JJS_ENCODING_CESU8) == size);
      TEST_ASSERT (jjs_string_to_buffer (context_p, error_value, JJS_ENCODING_CESU8, string_buf, size) == size);
      TEST_ASSERT (string_buf[0] == 'e' && string_buf[1] == (char) ('0' + counter));
      break;
    }
    case 3:
    {
      TEST_ASSERT (counter == 1);
      TEST_ASSERT (jjs_error_type (context_p, error_value) == JJS_ERROR_RANGE);
      break;
    }
    case 4:
    {
      TEST_ASSERT (mode == 4);
      TEST_ASSERT (counter >= 1 && counter <= 2);

      jjs_error_t error = (counter == 1) ? JJS_ERROR_REFERENCE : JJS_ERROR_TYPE;
      TEST_ASSERT (jjs_error_type (context_p, error_value) == error);
      break;
    }
    case 5:
    case 6:
    {
      TEST_ASSERT (counter >= 1 && counter <= 2);
      TEST_ASSERT (jjs_value_is_false (context_p, error_value));
      break;
    }
    default:
    {
      TEST_ASSERT (mode == 8 || mode == 9);
      TEST_ASSERT (counter == 1);
      TEST_ASSERT (jjs_value_is_true (context_p, error_value));
      break;
    }
  }
} /* vm_throw_callback */

static jjs_value_t
native_handler (const jjs_call_info_t *call_info_p, /**< call info */
                const jjs_value_t args_p[], /**< arguments */
                const jjs_length_t args_count) /**< arguments length */
{
  (void) args_p;
  jjs_context_t *context_p = call_info_p->context_p;
  TEST_ASSERT (args_count == 0);

  if (mode == 7)
  {
    jjs_value_t result = jjs_throw_sz (context_p, JJS_ERROR_COMMON, "Error!");

    TEST_ASSERT (!jjs_exception_is_captured (context_p, result));
    jjs_exception_allow_capture (context_p, result, false);
    TEST_ASSERT (jjs_exception_is_captured (context_p, result));
    return result;
  }

  jjs_char_t source[] = TEST_STRING_LITERAL ("throw false");
  jjs_value_t result = jjs_eval (context_p, source, sizeof (source) - 1, JJS_PARSE_NO_OPTS);

  TEST_ASSERT (jjs_exception_is_captured (context_p, result));

  if (mode == 6)
  {
    jjs_exception_allow_capture (context_p, result, true);
    TEST_ASSERT (!jjs_exception_is_captured (context_p, result));
  }
  return result;
} /* native_handler */

static void
do_eval (const char *script_p, /**< script to evaluate */
         bool should_throw) /**< script throws an error */
{
  jjs_value_t result = jjs_eval_sz (ctx (), script_p, JJS_PARSE_NO_OPTS);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result) == should_throw);
  jjs_value_free (ctx (), result);
} /* do_eval */

int
main (void)
{
  /* Test stopping an infinite loop. */
  if (!jjs_feature_enabled (JJS_FEATURE_VM_THROW))
  {
    return 0;
  }

  ctx_open (NULL);

  jjs_on_throw (ctx (), vm_throw_callback, (void *) &mode);

  mode = 0;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("throw -5.6"), true);
  TEST_ASSERT (counter == 1);

  mode = 1;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { throw null }\n"
                                "function g() { f() }\n"
                                "g()\n"),
           true);
  TEST_ASSERT (counter == 1);

  mode = 2;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { throw 'e1' }\n"
                                "function g() { try { f() } catch (e) { throw 'e2' } }\n"
                                "try { g() } catch (e) { throw 'e3' }\n"),
           true);
  TEST_ASSERT (counter == 3);

  mode = 3;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { throw new RangeError() }\n"
                                "function g() { try { f() } finally { } }\n"
                                "try { g() } finally { }\n"),
           true);
  TEST_ASSERT (counter == 1);

  mode = 4;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { unresolved }\n"
                                "function g() { try { f() } finally { null.member } }\n"
                                "try { g() } finally { }\n"),
           true);
  TEST_ASSERT (counter == 2);

  /* Native functions may trigger the call twice: */
  jjs_value_t global_object_value = jjs_current_realm (ctx ());
  jjs_value_t function_value = jjs_function_external (ctx (), native_handler);

  jjs_value_free (ctx (), jjs_object_set_sz (ctx (), global_object_value, "native", function_value, JJS_MOVE));
  jjs_value_free (ctx (), global_object_value);

  mode = 5;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("native()\n"), true);
  TEST_ASSERT (counter == 1);

  mode = 6;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("native()\n"), true);
  TEST_ASSERT (counter == 2);

  mode = 7;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("native()\n"), true);
  TEST_ASSERT (counter == 0);

  /* Built-in functions should not trigger the call twice: */
  mode = 8;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { eval('eval(\\'throw true\\')') }\n"
                                "f()\n"),
           true);
  TEST_ASSERT (counter == 1);

  mode = 9;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { [1].map(function() { throw true }) }\n"
                                "f()\n"),
           true);
  TEST_ASSERT (counter == 1);

  jjs_value_t value = jjs_object (ctx ());
  TEST_ASSERT (!jjs_exception_is_captured (ctx (), value));
  jjs_exception_allow_capture (ctx (), value, false);
  TEST_ASSERT (!jjs_exception_is_captured (ctx (), value));
  jjs_value_free (ctx (), value);

  ctx_close ();
  return 0;
} /* main */
