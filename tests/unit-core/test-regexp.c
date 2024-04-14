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
  TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

  jjs_value_t global_obj_val = jjs_current_realm ();

  char pattern[] = "[^.]+";
  uint16_t flags = JJS_REGEXP_FLAG_GLOBAL | JJS_REGEXP_FLAG_MULTILINE;
  jjs_value_t regex_obj = jjs_regexp_sz (pattern, flags);
  TEST_ASSERT (jjs_value_is_object (regex_obj));

  const jjs_char_t func_src[] = "return [regex.exec('something.domain.com'), regex.multiline, regex.global];";

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_HAS_ARGUMENT_LIST;
  parse_options.argument_list = jjs_string_sz ("regex");

  jjs_value_t func_val = jjs_parse (func_src, sizeof (func_src) - 1, &parse_options);
  jjs_value_free (parse_options.argument_list);

  jjs_value_t res = jjs_call (func_val, global_obj_val, &regex_obj, 1);
  jjs_value_t regex_res = jjs_object_get_index (res, 0);
  jjs_value_t regex_res_str = jjs_object_get_index (regex_res, 0);
  jjs_value_t is_multiline = jjs_object_get_index (res, 1);
  jjs_value_t is_global = jjs_object_get_index (res, 2);

  const char expected_result[] = "something";
  jjs_size_t str_size = jjs_string_size (regex_res_str, JJS_ENCODING_CESU8);
  TEST_ASSERT (str_size == (sizeof (expected_result) - 1));

  JJS_VLA (jjs_char_t, res_buff, str_size);
  jjs_size_t res_size = jjs_string_to_buffer (regex_res_str, JJS_ENCODING_CESU8, res_buff, str_size);

  TEST_ASSERT (res_size == str_size);
  TEST_ASSERT (strncmp (expected_result, (const char *) res_buff, res_size) == 0);
  TEST_ASSERT (jjs_value_is_true (is_multiline));
  TEST_ASSERT (jjs_value_is_true (is_global));

  jjs_value_free (regex_obj);
  jjs_value_free (res);
  jjs_value_free (func_val);
  jjs_value_free (regex_res);
  jjs_value_free (regex_res_str);
  jjs_value_free (is_multiline);
  jjs_value_free (is_global);
  jjs_value_free (global_obj_val);

  jjs_cleanup ();
  return 0;
} /* main */
