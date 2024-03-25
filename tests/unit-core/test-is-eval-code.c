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
check_eval (const jjs_call_info_t *call_info_p, /**< call information */
            const jjs_value_t args_p[], /**< arguments list */
            const jjs_length_t args_cnt) /**< arguments length */
{
  JJS_UNUSED (call_info_p);

  TEST_ASSERT (args_cnt == 2 && jjs_function_is_dynamic (args_p[0]) == jjs_value_is_true (args_p[1]));
  return jjs_boolean (true);
} /* check_eval */

static void
test_parse (const char *source_p, /**< source code */
            jjs_parse_options_t *options_p) /**< options passed to jjs_parse */
{
  jjs_value_t parse_result = jjs_parse ((const jjs_char_t *) source_p, strlen (source_p), options_p);
  TEST_ASSERT (!jjs_value_is_exception (parse_result));
  TEST_ASSERT (!jjs_function_is_dynamic (parse_result));

  jjs_value_t result;

  if (options_p->options & JJS_PARSE_HAS_ARGUMENT_LIST)
  {
    jjs_value_t this_value = jjs_undefined ();
    result = jjs_call (parse_result, this_value, NULL, 0);
    jjs_value_free (this_value);
  }
  else if (options_p->options & JJS_PARSE_MODULE)
  {
    result = jjs_module_link (parse_result, NULL, NULL);
    TEST_ASSERT (!jjs_value_is_exception (result));
    jjs_value_free (result);
    result = jjs_module_evaluate (parse_result);
  }
  else
  {
    result = jjs_run (parse_result);
  }

  TEST_ASSERT (!jjs_value_is_exception (result));

  jjs_value_free (parse_result);
  jjs_value_free (result);
} /* test_parse */

int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  jjs_value_t global_object_value = jjs_current_realm ();

  jjs_value_t function_value = jjs_function_external (check_eval);
  jjs_value_t function_name_value = jjs_string_sz ("check_eval");
  jjs_value_free (jjs_object_set (global_object_value, function_name_value, function_value));

  jjs_value_free (function_name_value);
  jjs_value_free (function_value);
  jjs_value_free (global_object_value);

  jjs_parse_options_t parse_options;
  const char *source_p = TEST_STRING_LITERAL ("eval('check_eval(function() {}, true)')\n"
                                              "check_eval(function() {}, false)");

  parse_options.options = JJS_PARSE_NO_OPTS;
  test_parse (source_p, &parse_options);

  if (jjs_feature_enabled (JJS_FEATURE_MODULE))
  {
    parse_options.options = JJS_PARSE_MODULE;
    test_parse (source_p, &parse_options);
  }

  parse_options.options = JJS_PARSE_HAS_ARGUMENT_LIST;
  parse_options.argument_list = jjs_string_sz ("");
  test_parse (source_p, &parse_options);
  jjs_value_free (parse_options.argument_list);

  parse_options.options = JJS_PARSE_NO_OPTS;
  source_p = TEST_STRING_LITERAL ("check_eval(new Function('a', 'return a'), true)");
  test_parse (source_p, &parse_options);

  source_p = TEST_STRING_LITERAL ("check_eval(function() {}, true)");
  jjs_value_free (jjs_eval ((const jjs_char_t *) source_p, strlen (source_p), JJS_PARSE_NO_OPTS));

  jjs_cleanup ();
  return 0;
} /* main */
