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

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t global_obj_val = jjs_current_realm (ctx ());

  char pattern[] = "[^.]+";
  uint16_t flags = JJS_REGEXP_FLAG_GLOBAL | JJS_REGEXP_FLAG_MULTILINE;
  jjs_value_t regex_obj = jjs_regexp_sz (ctx (), pattern, flags);
  TEST_ASSERT (jjs_value_is_object (ctx (), regex_obj));

  const jjs_char_t func_src[] = "return [regex.exec('something.domain.com'), regex.multiline, regex.global];";

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_HAS_ARGUMENT_LIST;
  parse_options.argument_list = jjs_string_sz (ctx (), "regex");

  jjs_value_t func_val = jjs_parse (ctx (), func_src, sizeof (func_src) - 1, &parse_options);
  jjs_value_free (ctx (), parse_options.argument_list);

  jjs_value_t res = jjs_call (ctx (), func_val, global_obj_val, &regex_obj, 1);
  jjs_value_t regex_res = jjs_object_get_index (ctx (), res, 0);
  jjs_value_t regex_res_str = jjs_object_get_index (ctx (), regex_res, 0);
  jjs_value_t is_multiline = jjs_object_get_index (ctx (), res, 1);
  jjs_value_t is_global = jjs_object_get_index (ctx (), res, 2);

  const char expected_result[] = "something";
  jjs_size_t str_size = jjs_string_size (ctx (), regex_res_str, JJS_ENCODING_CESU8);
  TEST_ASSERT (str_size == (sizeof (expected_result) - 1));

  JJS_VLA (jjs_char_t, res_buff, str_size);
  jjs_size_t res_size = jjs_string_to_buffer (ctx (), regex_res_str, JJS_ENCODING_CESU8, res_buff, str_size);

  TEST_ASSERT (res_size == str_size);
  TEST_ASSERT (strncmp (expected_result, (const char *) res_buff, res_size) == 0);
  TEST_ASSERT (jjs_value_is_true (ctx (), is_multiline));
  TEST_ASSERT (jjs_value_is_true (ctx (), is_global));

  jjs_value_free (ctx (), regex_obj);
  jjs_value_free (ctx (), res);
  jjs_value_free (ctx (), func_val);
  jjs_value_free (ctx (), regex_res);
  jjs_value_free (ctx (), regex_res_str);
  jjs_value_free (ctx (), is_multiline);
  jjs_value_free (ctx (), is_global);
  jjs_value_free (ctx (), global_obj_val);

  ctx_close ();
  return 0;
} /* main */
