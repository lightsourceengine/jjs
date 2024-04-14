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

#include "jjs-compiler.h"
#include "jjs.h"

#include "annex.h"
#include "config.h"
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
check_dirname_result_sz (const char* input, const char* expected)
{
  ecma_value_t actual = push (annex_path_dirname (push_sz (input)));

  TEST_ASSERT (ecma_is_value_string (actual));
  TEST_ASSERT (strict_equals (actual, push_sz (expected)));
}

static void
check_dirname_returns_empty_value (jjs_value_t value)
{
  ecma_value_t actual = push (annex_path_dirname (value));

  TEST_ASSERT (ecma_is_value_empty (actual));
}

static void
check_dirname_returns_empty_value_sz (const char* input)
{
  check_dirname_returns_empty_value (push_sz (input));
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

static void
test_annex_path_dirname (void)
{
  check_dirname_result_sz ("/", "/");
  check_dirname_result_sz ("/a", "/");
  check_dirname_result_sz ("///////a", "///////");
  check_dirname_result_sz ("/a/", "/");
  check_dirname_result_sz ("///////a/", "///////");
  check_dirname_result_sz ("/a//b", "/a");
  check_dirname_result_sz ("/aa//bb", "/aa");
  check_dirname_result_sz ("/aa//bb/////", "/aa");

  check_dirname_returns_empty_value_sz ("");
  check_dirname_returns_empty_value_sz ("a");
  check_dirname_returns_empty_value_sz ("a/b/c");

  check_dirname_returns_empty_value (push (jjs_number (1)));
  check_dirname_returns_empty_value (push (jjs_null ()));
  check_dirname_returns_empty_value (push (jjs_undefined ()));
  check_dirname_returns_empty_value (push (jjs_object ()));
  check_dirname_returns_empty_value (push (jjs_array (1)));

#ifdef JJS_OS_IS_WINDOWS
  // drive letter
  check_dirname_result_sz ("C:path", "C:");
  check_dirname_result_sz ("C:\\path", "C:\\");
  check_dirname_result_sz ("C:/path", "C:/");

  // driver letter nested path
  check_dirname_result_sz ("C:path/a", "C:path");
  check_dirname_result_sz ("C:\\path/a", "C:\\path");
  check_dirname_result_sz ("C:/path/a", "C:/path");

  // driver letter long path prefix
  check_dirname_result_sz ("\\\\?\\C:\\a\\b", "\\\\?\\C:\\a");
  check_dirname_result_sz ("\\\\?\\C:a\\b", "\\\\?\\C:a");

  // volume long path prefix
  check_dirname_result_sz ("\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\a",
                           "\\\\?\\Volume{00000000-0000-0000-0000-000000000000}");
  check_dirname_result_sz ("\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\a\\b",
                           "\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\a");

  // unc long path prefix
  check_dirname_result_sz ("\\\\?\\UNC\\a", "\\\\?\\UNC\\");
  check_dirname_result_sz ("\\\\?\\UNC\\a\\b", "\\\\?\\UNC\\a");

  check_dirname_returns_empty_value_sz ("\\\\?\\X\\a");
  check_dirname_returns_empty_value_sz ("\\\\?\\unc\\blah");
  check_dirname_returns_empty_value_sz ("\\\\?\\volume\\bb");
  check_dirname_returns_empty_value_sz ("\\\\?\\Volume{0#000000-0000-0000-0000-000000000000}\\xx");
#endif
}

int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

  test_annex_path_basename ();
  test_annex_path_dirname ();

  free_values ();

  jjs_cleanup ();
  return 0;
} /* main */
