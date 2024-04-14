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
callback_func (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_count)
{
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_p);
  JJS_UNUSED (args_count);

  jjs_value_t value = jjs_string_sz ("Abort run!");
  value = jjs_throw_abort (value, true);
  return value;
} /* callback_func */

int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

  jjs_value_t global = jjs_current_realm ();
  jjs_value_t callback_name = jjs_string_sz ("callback");
  jjs_value_t func = jjs_function_external (callback_func);
  jjs_value_t res = jjs_object_set (global, callback_name, func);
  TEST_ASSERT (!jjs_value_is_exception (res));

  jjs_value_free (res);
  jjs_value_free (func);
  jjs_value_free (callback_name);
  jjs_value_free (global);

  const jjs_char_t inf_loop_code_src1[] = TEST_STRING_LITERAL ("while(true) {\n"
                                                                 "  with ({}) {\n"
                                                                 "    try {\n"
                                                                 "      callback();\n"
                                                                 "    } catch (e) {\n"
                                                                 "    } finally {\n"
                                                                 "    }\n"
                                                                 "  }\n"
                                                                 "}");

  jjs_value_t parsed_code_val = jjs_parse (inf_loop_code_src1, sizeof (inf_loop_code_src1) - 1, NULL);

  TEST_ASSERT (!jjs_value_is_exception (parsed_code_val));
  res = jjs_run (parsed_code_val);

  TEST_ASSERT (jjs_value_is_abort (res));

  jjs_value_free (res);
  jjs_value_free (parsed_code_val);

  const jjs_char_t inf_loop_code_src2[] = TEST_STRING_LITERAL ("function f() {"
                                                                 "  while(true) {\n"
                                                                 "    with ({}) {\n"
                                                                 "      try {\n"
                                                                 "        callback();\n"
                                                                 "      } catch (e) {\n"
                                                                 "      } finally {\n"
                                                                 "      }\n"
                                                                 "    }\n"
                                                                 "  }"
                                                                 "}\n"
                                                                 "function g() {\n"
                                                                 "  for (a in { x:5 })\n"
                                                                 "    f();\n"
                                                                 "}\n"
                                                                 "\n"
                                                                 "with({})\n"
                                                                 " f();\n");

  parsed_code_val = jjs_parse (inf_loop_code_src2, sizeof (inf_loop_code_src2) - 1, NULL);

  TEST_ASSERT (!jjs_value_is_exception (parsed_code_val));
  res = jjs_run (parsed_code_val);

  TEST_ASSERT (jjs_value_is_abort (res));

  jjs_value_free (res);
  jjs_value_free (parsed_code_val);

  /* Test flag overwrites. */
  jjs_value_t value = jjs_string_sz ("Error description");
  TEST_ASSERT (!jjs_value_is_abort (value));
  TEST_ASSERT (!jjs_value_is_exception (value));

  value = jjs_throw_abort (value, true);
  TEST_ASSERT (jjs_value_is_abort (value));
  TEST_ASSERT (jjs_value_is_exception (value));

  value = jjs_throw_value (value, true);
  TEST_ASSERT (!jjs_value_is_abort (value));
  TEST_ASSERT (jjs_value_is_exception (value));

  value = jjs_throw_abort (value, true);
  TEST_ASSERT (jjs_value_is_abort (value));
  TEST_ASSERT (jjs_value_is_exception (value));

  jjs_value_free (value);

  jjs_cleanup ();
  return 0;
} /* main */
