/* Copyright Light Source Software, LLC and other contributors.
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
#include "annex.h"
#define TEST_COMMON_IMPLEMENTATION
#include "test-common.h"

static void
check_basename_result (const char* filename, ecma_value_t expected)
{
  ecma_value_t filename_value = ecma_string_ascii_sz (filename);
  ecma_value_t actual = annex_path_basename (filename_value);

  if (expected == ECMA_VALUE_EMPTY)
  {
    TEST_ASSERT (actual == expected);
  }
  else
  {
    TEST_ASSERT (strict_equals (actual, expected));
  }

  ecma_free_value (actual);
  ecma_free_value (filename_value);
}

static void
check_basename_result_sz (const char* filename, const char* expected)
{
  ecma_value_t expected_value = ecma_string_ascii_sz (expected);

  check_basename_result (filename, expected_value);
  ecma_free_value (expected_value);
}

static void
test_annex_path_basename (void)
{
  check_basename_result ("", ECMA_VALUE_EMPTY);
  check_basename_result ("/", ECMA_VALUE_EMPTY);
  check_basename_result (".", ECMA_VALUE_EMPTY);
  check_basename_result ("..", ECMA_VALUE_EMPTY);
  check_basename_result ("./", ECMA_VALUE_EMPTY);
  check_basename_result ("../", ECMA_VALUE_EMPTY);
  check_basename_result ("dir/", ECMA_VALUE_EMPTY);

  check_basename_result_sz ("filename.js", "filename.js");
  check_basename_result_sz ("./filename.js", "filename.js");
  check_basename_result_sz ("../filename.js", "filename.js");
  check_basename_result_sz ("/path/filename.js", "filename.js");
  check_basename_result_sz ("///path//filename.js", "filename.js");
}

int
main (void)
{
  TEST_INIT ();

  jjs_init (JJS_INIT_EMPTY);

  test_annex_path_basename ();
  
  jjs_cleanup ();
  return 0;
} /* main */
