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

  jjs_error_t errors[] = { JJS_ERROR_COMMON, JJS_ERROR_EVAL, JJS_ERROR_RANGE, JJS_ERROR_REFERENCE,
                             JJS_ERROR_SYNTAX, JJS_ERROR_TYPE, JJS_ERROR_URI };

  for (size_t idx = 0; idx < sizeof (errors) / sizeof (errors[0]); idx++)
  {
    jjs_value_t error_obj = jjs_throw_sz (ctx (), errors[idx], "test");
    TEST_ASSERT (jjs_value_is_exception (ctx (), error_obj));
    TEST_ASSERT (jjs_error_type (ctx (), error_obj) == errors[idx]);

    error_obj = jjs_exception_value (ctx (), error_obj, true);

    TEST_ASSERT (jjs_error_type (ctx (), error_obj) == errors[idx]);

    jjs_value_free (ctx (), error_obj);
  }

  jjs_value_t test_values[] = {
    jjs_number (ctx (), 11),
    jjs_string_sz (ctx (), "message"),
    jjs_boolean (ctx (), true),
    jjs_object (ctx ()),
  };

  for (size_t idx = 0; idx < sizeof (test_values) / sizeof (test_values[0]); idx++)
  {
    jjs_error_t error_type = jjs_error_type (ctx (), test_values[idx]);
    TEST_ASSERT (error_type == JJS_ERROR_NONE);
    jjs_value_free (ctx (), test_values[idx]);
  }

  char test_source[] = "\xF0\x9D\x84\x9E";

  jjs_value_t result = jjs_parse_sz (ctx (), test_source, NULL);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  TEST_ASSERT (jjs_error_type (ctx (), result) == JJS_ERROR_SYNTAX);

  jjs_value_free (ctx (), result);

  char test_invalid_error[] = "Object.create(Error.prototype)";
  result = jjs_eval (ctx (), (const jjs_char_t *) test_invalid_error, sizeof (test_invalid_error) - 1, JJS_PARSE_NO_OPTS);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result) && jjs_value_is_object (ctx (), result));
  TEST_ASSERT (jjs_error_type (ctx (), result) == JJS_ERROR_NONE);

  jjs_value_free (ctx (), result);

  ctx_close ();
} /* main */
