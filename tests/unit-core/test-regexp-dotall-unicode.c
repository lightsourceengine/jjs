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

int
main (void)
{
  TEST_INIT ();
  TEST_CONTEXT_NEW (context_p);

  jjs_value_t undefined_this_arg = jjs_undefined ();
  char pattern2[] = "\\u{61}.\\u{62}";

  uint16_t flags = JJS_REGEXP_FLAG_DOTALL | JJS_REGEXP_FLAG_UNICODE | JJS_REGEXP_FLAG_STICKY;
  jjs_value_t regex_obj = jjs_regexp_sz (pattern2, flags);
  TEST_ASSERT (jjs_value_is_object (regex_obj));

  const jjs_char_t func_src2[] = "return [regex.exec('a\\nb'), regex.dotAll, regex.sticky, regex.unicode ];";

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_HAS_ARGUMENT_LIST;
  parse_options.argument_list = jjs_string_sz ("regex");

  jjs_value_t func_val = jjs_parse (func_src2, sizeof (func_src2) - 1, &parse_options);
  jjs_value_free (parse_options.argument_list);

  jjs_value_t res = jjs_call (func_val, undefined_this_arg, &regex_obj, 1);
  jjs_value_t regex_res = jjs_object_get_index (res, 0);
  jjs_value_t regex_res_str = jjs_object_get_index (regex_res, 0);
  jjs_value_t is_dotall = jjs_object_get_index (res, 1);
  jjs_value_t is_sticky = jjs_object_get_index (res, 2);
  jjs_value_t is_unicode = jjs_object_get_index (res, 3);

  jjs_size_t str_size = jjs_string_size (regex_res_str, JJS_ENCODING_CESU8);
  JJS_VLA (jjs_char_t, res_buff, str_size);
  jjs_size_t res_size = jjs_string_to_buffer (regex_res_str, JJS_ENCODING_CESU8, res_buff, str_size);

  const char expected_result[] = "a\nb";
  TEST_ASSERT (res_size == (sizeof (expected_result) - 1));
  TEST_ASSERT (strncmp (expected_result, (const char *) res_buff, res_size) == 0);
  TEST_ASSERT (jjs_value_is_true (is_dotall));
  TEST_ASSERT (jjs_value_is_true (is_sticky));
  TEST_ASSERT (jjs_value_is_true (is_unicode));

  jjs_value_free (regex_obj);
  jjs_value_free (res);
  jjs_value_free (func_val);
  jjs_value_free (regex_res);
  jjs_value_free (regex_res_str);
  jjs_value_free (is_dotall);
  jjs_value_free (is_sticky);
  jjs_value_free (is_unicode);

  TEST_CONTEXT_FREE (context_p);
  return 0;
} /* main */
