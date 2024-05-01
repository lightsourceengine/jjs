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

static jjs_value_t
vm_exec_stop_callback (void *user_p)
{
  int *int_p = (int *) user_p;

  if (*int_p > 0)
  {
    (*int_p)--;

    return jjs_undefined (ctx ());
  }

  return jjs_string_sz (ctx (), "Abort script");
} /* vm_exec_stop_callback */

int
main (void)
{
  /* Test stopping an infinite loop. */
  if (!jjs_feature_enabled (JJS_FEATURE_VM_EXEC_STOP))
  {
    return 0;
  }

  ctx_open (NULL);

  int countdown = 6;
  jjs_halt_handler (ctx (), 16, vm_exec_stop_callback, &countdown);

  const jjs_char_t inf_loop_code_src1[] = "while(true) {}";
  jjs_value_t parsed_code_val = jjs_parse (ctx (), inf_loop_code_src1, sizeof (inf_loop_code_src1) - 1, NULL);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), parsed_code_val));
  jjs_value_t res = jjs_run (ctx (), parsed_code_val);
  TEST_ASSERT (countdown == 0);

  TEST_ASSERT (jjs_value_is_exception (ctx (), res));

  jjs_value_free (ctx (), res);
  jjs_value_free (ctx (), parsed_code_val);

  /* A more complex example. Although the callback error is captured
   * by the catch block, it is automatically thrown again. */

  /* We keep the callback function, only the countdown is reset. */
  countdown = 6;

  const jjs_char_t inf_loop_code_src2[] = TEST_STRING_LITERAL ("function f() { while (true) ; }\n"
                                                                 "try { f(); } catch(e) {}");

  parsed_code_val = jjs_parse (ctx (), inf_loop_code_src2, sizeof (inf_loop_code_src2) - 1, NULL);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), parsed_code_val));
  res = jjs_run (ctx (), parsed_code_val);
  TEST_ASSERT (countdown == 0);

  /* The result must have an error flag which proves that
   * the error is thrown again inside the catch block. */
  TEST_ASSERT (jjs_value_is_exception (ctx (), res));

  jjs_value_free (ctx (), res);
  jjs_value_free (ctx (), parsed_code_val);

  ctx_close ();
  return 0;
} /* main */
