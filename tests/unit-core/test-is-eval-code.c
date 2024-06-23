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
check_eval (const jjs_call_info_t *call_info_p, /**< call information */
            const jjs_value_t args_p[], /**< arguments list */
            const jjs_length_t args_cnt) /**< arguments length */
{
  JJS_UNUSED (call_info_p);

  TEST_ASSERT (args_cnt == 2 && jjs_function_is_dynamic (ctx (), args_p[0]) == jjs_value_is_true (ctx (), args_p[1]));
  return jjs_boolean (ctx (), true);
} /* check_eval */

static void
test_parse (const char *source_p, /**< source code */
            jjs_parse_options_t *options_p) /**< options passed to jjs_parse */
{
  jjs_value_t parse_result = jjs_parse_sz (ctx (), source_p, options_p);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), parse_result));
  TEST_ASSERT (!jjs_function_is_dynamic (ctx (), parse_result));

  jjs_value_t result;

  if (options_p->argument_list.has_value)
  {
    result = jjs_call_noargs (ctx (), parse_result);
  }
  else if (options_p->parse_module)
  {
    result = jjs_module_link (ctx (), parse_result, NULL, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
    jjs_value_free (ctx (), result);
    result = jjs_module_evaluate (ctx (), parse_result);
  }
  else
  {
    result = jjs_run (ctx (), parse_result, JJS_KEEP);
  }

  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));

  jjs_value_free (ctx (), parse_result);
  jjs_value_free (ctx (), result);
} /* test_parse */

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t global_object_value = jjs_current_realm (ctx ());

  jjs_value_t function_value = jjs_function_external (ctx (), check_eval);
  jjs_value_t function_name_value = jjs_string_sz (ctx (), "check_eval");
  jjs_value_free (ctx (), jjs_object_set (ctx (), global_object_value, function_name_value, function_value, JJS_MOVE));

  jjs_value_free (ctx (), function_name_value);
  jjs_value_free (ctx (), global_object_value);

  jjs_parse_options_t parse_options = jjs_parse_options ();
  const char *source_p = TEST_STRING_LITERAL ("eval('check_eval(function() {}, true)')\n"
                                              "check_eval(function() {}, false)");

  test_parse (source_p, &parse_options);

  if (jjs_feature_enabled (JJS_FEATURE_MODULE))
  {
    parse_options = (jjs_parse_options_t) {
      .parse_module = true,
    };
    test_parse (source_p, &parse_options);
  }

  parse_options = (jjs_parse_options_t) {
    .argument_list = jjs_optional_value (jjs_string_sz (ctx (), "")),
  };

  test_parse (source_p, &parse_options);
  jjs_value_free (ctx (), parse_options.argument_list.value);

  parse_options = jjs_parse_options ();
  source_p = TEST_STRING_LITERAL ("check_eval(new Function('a', 'return a'), true)");
  test_parse (source_p, &parse_options);

  source_p = TEST_STRING_LITERAL ("check_eval(function() {}, true)");
  jjs_value_free (ctx (), jjs_eval_sz (ctx (), source_p, JJS_PARSE_NO_OPTS));

  ctx_close ();
  return 0;
} /* main */
