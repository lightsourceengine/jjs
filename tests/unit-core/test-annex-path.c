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

#include "jjs-test.h"

#include "annex.h"

static void
check_annex_path_basename (jjs_value_t input, jjs_value_t expected)
{
  ctx_assert_strict_equals (ctx_value (annex_path_basename (ctx (), input)), expected);
}

static void
check_annex_path_dirname (jjs_value_t input, jjs_value_t expected)
{
  ctx_assert_strict_equals (ctx_value (annex_path_dirname (ctx (), input)), expected);
}

static void
test_annex_path_basename (void)
{
  check_annex_path_basename (ctx_cstr (""), ECMA_VALUE_EMPTY);
  check_annex_path_basename (ctx_cstr ("/"), ECMA_VALUE_EMPTY);
  check_annex_path_basename (ctx_cstr ("."), ECMA_VALUE_EMPTY);
  check_annex_path_basename (ctx_cstr (".."), ECMA_VALUE_EMPTY);
  check_annex_path_basename (ctx_cstr ("./"), ECMA_VALUE_EMPTY);
  check_annex_path_basename (ctx_cstr ("../"), ECMA_VALUE_EMPTY);
  check_annex_path_basename (ctx_cstr ("dir/"), ECMA_VALUE_EMPTY);

  check_annex_path_basename (ctx_cstr ("filename.js"), ctx_cstr ("filename.js"));
  check_annex_path_basename (ctx_cstr ("./filename.js"), ctx_cstr ("filename.js"));
  check_annex_path_basename (ctx_cstr ("../filename.js"), ctx_cstr ("filename.js"));
  check_annex_path_basename (ctx_cstr ("/path/filename.js"), ctx_cstr ("filename.js"));
  check_annex_path_basename (ctx_cstr ("///path//filename.js"), ctx_cstr ("filename.js"));
}

static void
test_annex_path_dirname (void)
{
  check_annex_path_dirname (ctx_cstr ("/"), ctx_cstr ("/"));
  check_annex_path_dirname (ctx_cstr ("/a"), ctx_cstr ("/"));
  check_annex_path_dirname (ctx_cstr ("///////a"), ctx_cstr ("///////"));
  check_annex_path_dirname (ctx_cstr ("/a/"), ctx_cstr ("/"));
  check_annex_path_dirname (ctx_cstr ("///////a/"), ctx_cstr ("///////"));
  check_annex_path_dirname (ctx_cstr ("/a//b"), ctx_cstr ("/a"));
  check_annex_path_dirname (ctx_cstr ("/aa//bb"), ctx_cstr ("/aa"));
  check_annex_path_dirname (ctx_cstr ("/aa//bb/////"), ctx_cstr ("/aa"));

  check_annex_path_dirname (ctx_cstr (""), ECMA_VALUE_EMPTY);
  check_annex_path_dirname (ctx_cstr ("a"), ECMA_VALUE_EMPTY);
  check_annex_path_dirname (ctx_cstr ("a/b/c"), ECMA_VALUE_EMPTY);

  check_annex_path_dirname (ctx_number (1), ECMA_VALUE_EMPTY);
  check_annex_path_dirname (ctx_null (), ECMA_VALUE_EMPTY);
  check_annex_path_dirname (ctx_undefined (), ECMA_VALUE_EMPTY);
  check_annex_path_dirname (ctx_object (), ECMA_VALUE_EMPTY);
  check_annex_path_dirname (ctx_array (0), ECMA_VALUE_EMPTY);

#ifdef JJS_OS_IS_WINDOWS
  // drive letter
  check_annex_path_dirname (ctx_cstr ("C:path"), ctx_cstr ("C:"));
  check_annex_path_dirname (ctx_cstr ("C:\\path"), ctx_cstr ("C:\\"));
  check_annex_path_dirname (ctx_cstr ("C:/path"), ctx_cstr ("C:/"));

  // driver letter nested path
  check_annex_path_dirname (ctx_cstr ("C:path/a"), ctx_cstr ("C:path"));
  check_annex_path_dirname (ctx_cstr ("C:\\path/a"), ctx_cstr ("C:\\path"));
  check_annex_path_dirname (ctx_cstr ("C:/path/a"), ctx_cstr ("C:/path"));

  // driver letter long path prefix
  check_annex_path_dirname (ctx_cstr ("\\\\?\\C:\\a\\b"), ctx_cstr ("\\\\?\\C:\\a"));
  check_annex_path_dirname (ctx_cstr ("\\\\?\\C:a\\b"), ctx_cstr ("\\\\?\\C:a"));

  // volume long path prefix
  check_annex_path_dirname (ctx_cstr ("\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\a"),
                            ctx_cstr ("\\\\?\\Volume{00000000-0000-0000-0000-000000000000}"));
  check_annex_path_dirname (ctx_cstr ("\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\a\\b"),
                            ctx_cstr ("\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\a"));

  // unc long path prefix
  check_annex_path_dirname ("\\\\?\\UNC\\a", "\\\\?\\UNC\\");
  check_annex_path_dirname ("\\\\?\\UNC\\a\\b", "\\\\?\\UNC\\a");

  check_annex_path_dirname (ctx_cstr ("\\\\?\\X\\a"), ECMA_VALUE_EMPTY);
  check_annex_path_dirname (ctx_cstr ("\\\\?\\unc\\blah"), ECMA_VALUE_EMPTY);
  check_annex_path_dirname (ctx_cstr ("\\\\?\\volume\\bb"), ECMA_VALUE_EMPTY);
  check_annex_path_dirname (ctx_cstr ("\\\\?\\Volume{0#000000-0000-0000-0000-000000000000}\\xx"), ECMA_VALUE_EMPTY);
#endif /* JJS_OS_IS_WINDOWS */
}

TEST_MAIN ({
  test_annex_path_basename ();
  test_annex_path_dirname ();
})
