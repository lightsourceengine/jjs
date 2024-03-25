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

  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  jjs_error_t errors[] = { JJS_ERROR_COMMON, JJS_ERROR_EVAL, JJS_ERROR_RANGE, JJS_ERROR_REFERENCE,
                             JJS_ERROR_SYNTAX, JJS_ERROR_TYPE, JJS_ERROR_URI };

  for (size_t idx = 0; idx < sizeof (errors) / sizeof (errors[0]); idx++)
  {
    jjs_value_t error_obj = jjs_throw_sz (errors[idx], "test");
    TEST_ASSERT (jjs_value_is_exception (error_obj));
    TEST_ASSERT (jjs_error_type (error_obj) == errors[idx]);

    error_obj = jjs_exception_value (error_obj, true);

    TEST_ASSERT (jjs_error_type (error_obj) == errors[idx]);

    jjs_value_free (error_obj);
  }

  jjs_value_t test_values[] = {
    jjs_number (11),
    jjs_string_sz ("message"),
    jjs_boolean (true),
    jjs_object (),
  };

  for (size_t idx = 0; idx < sizeof (test_values) / sizeof (test_values[0]); idx++)
  {
    jjs_error_t error_type = jjs_error_type (test_values[idx]);
    TEST_ASSERT (error_type == JJS_ERROR_NONE);
    jjs_value_free (test_values[idx]);
  }

  char test_source[] = "\xF0\x9D\x84\x9E";

  jjs_value_t result = jjs_parse ((const jjs_char_t *) test_source, sizeof (test_source) - 1, NULL);
  TEST_ASSERT (jjs_value_is_exception (result));
  TEST_ASSERT (jjs_error_type (result) == JJS_ERROR_SYNTAX);

  jjs_value_free (result);

  char test_invalid_error[] = "Object.create(Error.prototype)";
  result = jjs_eval ((const jjs_char_t *) test_invalid_error, sizeof (test_invalid_error) - 1, JJS_PARSE_NO_OPTS);
  TEST_ASSERT (!jjs_value_is_exception (result) && jjs_value_is_object (result));
  TEST_ASSERT (jjs_error_type (result) == JJS_ERROR_NONE);

  jjs_value_free (result);

  jjs_cleanup ();
} /* main */
