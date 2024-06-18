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
check_annex_path_to_file_url (jjs_value_t input, jjs_value_t expected)
{
  ctx_assert_strict_equals (ctx_defer_free (annex_path_to_file_url (ctx (), input)), expected);
}

static void
test_annex_path_to_file_url (void)
{
#ifdef _WIN32
  // Lowercase ascii alpha
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo"), ctx_cstr ("file:///C:/foo"));
  // Uppercase ascii alpha
  check_annex_path_to_file_url (ctx_cstr ("C:\\FOO"), ctx_cstr ("file:///C:/FOO"));
  // dir
  check_annex_path_to_file_url (ctx_cstr ("C:\\dir\\foo"), ctx_cstr ("file:///C:/dir/foo"));
  // trailing separator
  check_annex_path_to_file_url (ctx_cstr ("C:\\dir\\"), ctx_cstr ("file:///C:/dir/"));
  // dot
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo.mjs"), ctx_cstr ("file:///C:/foo.mjs"));
  // space
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo bar"), ctx_cstr ("file:///C:/foo%20bar"));
  // question mark
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo?bar"), ctx_cstr ("file:///C:/foo%3Fbar"));
  // number sign
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo#bar"), ctx_cstr ("file:///C:/foo%23bar"));
  // ampersand
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo&bar"), ctx_cstr ("file:///C:/foo&bar"));
  // equals
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo=bar"), ctx_cstr ("file:///C:/foo=bar"));
  // colon
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo:bar"), ctx_cstr ("file:///C:/foo:bar"));
  // semicolon
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo;bar"), ctx_cstr ("file:///C:/foo;bar"));
  // percent
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo%bar"), ctx_cstr ("file:///C:/foo%25bar"));
  // backslash
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo\\bar"), ctx_cstr ("file:///C:/foo/bar"));
  // backspace
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo\bbar"), ctx_cstr ("file:///C:/foo%08bar"));
  // tab
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo\tbar"), ctx_cstr ("file:///C:/foo%09bar"));
  // newline
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo\nbar"), ctx_cstr ("file:///C:/foo%0Abar"));
  // carriage return
  check_annex_path_to_file_url (ctx_cstr ("C:\\foo\rbar"), ctx_cstr ("file:///C:/foo%0Dbar"));
  // latin1
  check_annex_path_to_file_url (ctx_cstr ("C:\\fóóbàr"), ctx_cstr ("file:///C:/f%C3%B3%C3%B3b%C3%A0r"));
  // Euro sign (BMP code point)
  check_annex_path_to_file_url (ctx_cstr ("C:\\€"), ctx_cstr ("file:///C:/%E2%82%AC"));
  // Rocket emoji (non-BMP code point)
  check_annex_path_to_file_url (ctx_cstr ("C:\\🚀"), ctx_cstr ("file:///C:/%F0%9F%9A%80"));
  // UNC path (see https://docs.microsoft.com/en-us/archive/blogs/ie/file-uris-in-windows)
  check_annex_path_to_file_url (ctx_cstr ("\\\\nas\\My Docs\\File.doc"), ctx_cstr ("file://nas/My%20Docs/File.doc"));
#else // !_WIN32
  // Lowercase ascii alpha
  check_annex_path_to_file_url (ctx_cstr ("/foo"), ctx_cstr ("file:///foo"));
  // Uppercase ascii alpha
  check_annex_path_to_file_url (ctx_cstr ("/FOO"), ctx_cstr ("file:///FOO"));
  // dir
  check_annex_path_to_file_url (ctx_cstr ("/dir/foo"), ctx_cstr ("file:///dir/foo"));
  // trailing separator
  check_annex_path_to_file_url (ctx_cstr ("/dir/"), ctx_cstr ("file:///dir/"));
  // dot
  check_annex_path_to_file_url (ctx_cstr ("/foo.mjs"), ctx_cstr ("file:///foo.mjs"));
  // space
  check_annex_path_to_file_url (ctx_cstr ("/foo bar"), ctx_cstr ("file:///foo%20bar"));
  // question mark
  check_annex_path_to_file_url (ctx_cstr ("/foo?bar"), ctx_cstr ("file:///foo%3Fbar"));
  // number sign
  check_annex_path_to_file_url (ctx_cstr ("/foo#bar"), ctx_cstr ("file:///foo%23bar"));
  // ampersand
  check_annex_path_to_file_url (ctx_cstr ("/foo&bar"), ctx_cstr ("file:///foo&bar"));
  // equals
  check_annex_path_to_file_url (ctx_cstr ("/foo=bar"), ctx_cstr ("file:///foo=bar"));
  // colon
  check_annex_path_to_file_url (ctx_cstr ("/foo:bar"), ctx_cstr ("file:///foo:bar"));
  // semicolon
  check_annex_path_to_file_url (ctx_cstr ("/foo;bar"), ctx_cstr ("file:///foo;bar"));
  // percent
  check_annex_path_to_file_url (ctx_cstr ("/foo%bar"), ctx_cstr ("file:///foo%25bar"));
  // backslash
  check_annex_path_to_file_url (ctx_cstr ("/foo\\bar"), ctx_cstr ("file:///foo%5Cbar"));
  // backspace
  check_annex_path_to_file_url (ctx_cstr ("/foo\bbar"), ctx_cstr ("file:///foo%08bar"));
  // tab
  check_annex_path_to_file_url (ctx_cstr ("/foo\tbar"), ctx_cstr ("file:///foo%09bar"));
  // newline
  check_annex_path_to_file_url (ctx_cstr ("/foo\nbar"), ctx_cstr ("file:///foo%0Abar"));
  // carriage return
  check_annex_path_to_file_url (ctx_cstr ("/foo\rbar"), ctx_cstr ("file:///foo%0Dbar"));
  // latin1
  check_annex_path_to_file_url (ctx_cstr ("/fóóbàr"), ctx_cstr ("file:///f%C3%B3%C3%B3b%C3%A0r"));
  // Euro sign (BMP code point)
  check_annex_path_to_file_url (ctx_cstr ("/€"), ctx_cstr ("file:///%E2%82%AC"));
  // Rocket emoji (non-BMP code point)
  check_annex_path_to_file_url (ctx_cstr ("/🚀"), ctx_cstr ("file:///%F0%9F%9A%80"));
#endif
}

static void
test_annex_path_to_file_url_bad_input (void)
{
  check_annex_path_to_file_url (ctx_boolean (true), ECMA_VALUE_EMPTY);
  check_annex_path_to_file_url (ctx_number (123), ECMA_VALUE_EMPTY);
  check_annex_path_to_file_url (ctx_object (), ECMA_VALUE_EMPTY);
  check_annex_path_to_file_url (ctx_array (1), ECMA_VALUE_EMPTY);
  check_annex_path_to_file_url (ctx_symbol ("test"), ECMA_VALUE_EMPTY);
  check_annex_path_to_file_url (ctx_cstr (""), ECMA_VALUE_EMPTY);
  check_annex_path_to_file_url (ctx_cstr ("./relative-path"), ECMA_VALUE_EMPTY);
  check_annex_path_to_file_url (ctx_cstr ("../relative-path"), ECMA_VALUE_EMPTY);
  check_annex_path_to_file_url (ctx_cstr ("relative-path"), ECMA_VALUE_EMPTY);
}

TEST_MAIN ({
  test_annex_path_to_file_url ();
  test_annex_path_to_file_url_bad_input ();
})
