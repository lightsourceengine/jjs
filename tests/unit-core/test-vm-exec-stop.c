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

#include "config.h"
#include "test-common.h"

static jjs_value_t
vm_exec_stop_callback (void *user_p)
{
  int *int_p = (int *) user_p;

  if (*int_p > 0)
  {
    (*int_p)--;

    return jjs_undefined ();
  }

  return jjs_string_sz ("Abort script");
} /* vm_exec_stop_callback */

int
main (void)
{
  TEST_INIT ();

  /* Test stopping an infinite loop. */
  if (!jjs_feature_enabled (JJS_FEATURE_VM_EXEC_STOP))
  {
    return 0;
  }

  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  int countdown = 6;
  jjs_halt_handler (16, vm_exec_stop_callback, &countdown);

  const jjs_char_t inf_loop_code_src1[] = "while(true) {}";
  jjs_value_t parsed_code_val = jjs_parse (inf_loop_code_src1, sizeof (inf_loop_code_src1) - 1, NULL);

  TEST_ASSERT (!jjs_value_is_exception (parsed_code_val));
  jjs_value_t res = jjs_run (parsed_code_val);
  TEST_ASSERT (countdown == 0);

  TEST_ASSERT (jjs_value_is_exception (res));

  jjs_value_free (res);
  jjs_value_free (parsed_code_val);

  /* A more complex example. Although the callback error is captured
   * by the catch block, it is automatically thrown again. */

  /* We keep the callback function, only the countdown is reset. */
  countdown = 6;

  const jjs_char_t inf_loop_code_src2[] = TEST_STRING_LITERAL ("function f() { while (true) ; }\n"
                                                                 "try { f(); } catch(e) {}");

  parsed_code_val = jjs_parse (inf_loop_code_src2, sizeof (inf_loop_code_src2) - 1, NULL);

  TEST_ASSERT (!jjs_value_is_exception (parsed_code_val));
  res = jjs_run (parsed_code_val);
  TEST_ASSERT (countdown == 0);

  /* The result must have an error flag which proves that
   * the error is thrown again inside the catch block. */
  TEST_ASSERT (jjs_value_is_exception (res));

  jjs_value_free (res);
  jjs_value_free (parsed_code_val);

  jjs_cleanup ();
  return 0;
} /* main */
