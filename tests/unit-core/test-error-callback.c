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

static bool error_object_created_callback_is_running = false;
static int error_object_created_callback_count = 0;

static void
error_object_created_callback (const jjs_value_t error_object_t, /**< new error object */
                               void *user_p) /**< user pointer */
{
  TEST_ASSERT (!error_object_created_callback_is_running);
  TEST_ASSERT (user_p == (void *) &error_object_created_callback_count);

  error_object_created_callback_is_running = true;
  error_object_created_callback_count++;

  jjs_value_t name = jjs_string_sz ("message");
  jjs_value_t message = jjs_string_sz ("Replaced message!");

  jjs_value_t result = jjs_object_set (error_object_t, name, message);
  TEST_ASSERT (jjs_value_is_boolean (result) && jjs_value_is_true (result));
  jjs_value_free (result);

  /* This SyntaxError must not trigger a recusrsive call of the this callback. */
  const char *source_p = "Syntax Error in JS!";
  result = jjs_eval ((const jjs_char_t *) source_p, strlen (source_p), 0);
  TEST_ASSERT (jjs_value_is_exception (result));

  jjs_value_free (result);
  jjs_value_free (message);
  jjs_value_free (name);

  error_object_created_callback_is_running = false;
} /* error_object_created_callback */

static void
run_test (const char *source_p)
{
  /* Run the code 5 times. */
  for (int i = 0; i < 5; i++)
  {
    jjs_value_t result = jjs_eval ((const jjs_char_t *) source_p, strlen (source_p), 0);
    TEST_ASSERT (jjs_value_is_boolean (result) && jjs_value_is_true (result));
    jjs_value_free (result);
  }
} /* run_test */

/**
 * Unit test's main function.
 */
int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  jjs_error_on_created (error_object_created_callback, (void *) &error_object_created_callback_count);

  run_test ("var result = false\n"
            "try {\n"
            "  ref_error;\n"
            "} catch(e) {\n"
            "  result = (e.message === 'Replaced message!')\n"
            "}\n"
            "result\n");

  run_test ("var error = new Error()\n"
            "error.message === 'Replaced message!'\n");

  jjs_value_free (jjs_error_sz (JJS_ERROR_COMMON, "Message", jjs_undefined()));

  TEST_ASSERT (error_object_created_callback_count == 11);

  jjs_cleanup ();
  return 0;
} /* main */
