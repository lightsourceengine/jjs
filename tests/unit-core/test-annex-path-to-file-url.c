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
#include "test-common.h"
#include "annex.h"

static bool
strict_equals (jjs_value_t a, jjs_value_t b)
{
  jjs_value_t op_result = jjs_binary_op (JJS_BIN_OP_STRICT_EQUAL, a, b);
  bool result = jjs_value_is_true (op_result);

  jjs_value_free (op_result);

  return result;
}

static bool
strict_equals_cstr (jjs_value_t a, const char* b)
{
  jjs_value_t b_value = jjs_string_sz (b);
  bool result = strict_equals (a, b_value);

  jjs_value_free (b_value);

  return result;
}

void
try_annex_path_to_file_url (const char* input, const char* expected_output)
{
  jjs_value_t input_value = annex_util_create_string_utf8_sz(input);
  jjs_value_t output_value = annex_path_to_file_url (input_value);

  TEST_ASSERT (jjs_value_is_string (output_value));

  bool result = strict_equals_cstr (output_value, expected_output);

  jjs_value_free (input_value);
  jjs_value_free (output_value);

  TEST_ASSERT (result);
}

int
main (void)
{
  TEST_INIT ();

  jjs_init (JJS_INIT_EMPTY);

#ifdef _WIN32
  // Lowercase ascii alpha
  try_annex_path_to_file_url ("C:\\foo", "file:///C:/foo");
  // Uppercase ascii alpha
  try_annex_path_to_file_url ("C:\\FOO", "file:///C:/FOO");
  // dir
  try_annex_path_to_file_url ("C:\\dir\\foo", "file:///C:/dir/foo");
  // trailing separator
  try_annex_path_to_file_url ("C:\\dir\\", "file:///C:/dir/");
  // dot
  try_annex_path_to_file_url ("C:\\foo.mjs", "file:///C:/foo.mjs");
  // space
  try_annex_path_to_file_url ("C:\\foo bar", "file:///C:/foo%20bar");
  // question mark
//  try_annex_path_to_file_url ("C:\\foo?bar", "file:///C:/foo%3Fbar");
  // number sign
//  try_annex_path_to_file_url ("C:\\foo#bar", "file:///C:/foo%23bar");
  // ampersand
  try_annex_path_to_file_url ("C:\\foo&bar", "file:///C:/foo&bar");
  // equals
  try_annex_path_to_file_url ("C:\\foo=bar", "file:///C:/foo=bar");
  // colon
  try_annex_path_to_file_url ("C:\\foo:bar", "file:///C:/foo:bar");
  // semicolon
  try_annex_path_to_file_url ("C:\\foo;bar", "file:///C:/foo;bar");
  // percent
  try_annex_path_to_file_url ("C:\\foo%bar", "file:///C:/foo%25bar");
  // backslash
  try_annex_path_to_file_url ("C:\\foo\\bar", "file:///C:/foo/bar");
  // backspace
  try_annex_path_to_file_url ("C:\\foo\bbar", "file:///C:/foo%08bar");
  // tab
  try_annex_path_to_file_url ("C:\\foo\tbar", "file:///C:/foo%09bar");
  // newline
  try_annex_path_to_file_url ("C:\\foo\nbar", "file:///C:/foo%0Abar");
  // carriage return
  try_annex_path_to_file_url ("C:\\foo\rbar", "file:///C:/foo%0Dbar");
  // latin1
  try_annex_path_to_file_url ("C:\\fóóbàr", "file:///C:/f%C3%B3%C3%B3b%C3%A0r");
  // Euro sign (BMP code point)
  try_annex_path_to_file_url ("C:\\€", "file:///C:/%E2%82%AC");
  // Rocket emoji (non-BMP code point)
  try_annex_path_to_file_url ("C:\\🚀", "file:///C:/%F0%9F%9A%80");
  // UNC path (see https://docs.microsoft.com/en-us/archive/blogs/ie/file-uris-in-windows)
  try_annex_path_to_file_url ("\\\\nas\\My Docs\\File.doc", "file://nas/My%20Docs/File.doc");
#else // !_WIN32
  // Lowercase ascii alpha
  try_annex_path_to_file_url ("/foo", "file:///foo");
  // Uppercase ascii alpha
  try_annex_path_to_file_url ("/FOO", "file:///FOO");
  // dir
  try_annex_path_to_file_url ("/dir/foo", "file:///dir/foo");
  // trailing separator
  try_annex_path_to_file_url ("/dir/", "file:///dir/");
  // dot
  try_annex_path_to_file_url ("/foo.mjs", "file:///foo.mjs");
  // space
  try_annex_path_to_file_url ("/foo bar", "file:///foo%20bar");
  // question mark
//  try_annex_path_to_file_url ("/foo?bar", "file:///foo%3Fbar");
  // number sign
//  try_annex_path_to_file_url ("/foo#bar", "file:///foo%23bar");
  // ampersand
  try_annex_path_to_file_url ("/foo&bar", "file:///foo&bar");
  // equals
  try_annex_path_to_file_url ("/foo=bar", "file:///foo=bar");
  // colon
  try_annex_path_to_file_url ("/foo:bar", "file:///foo:bar");
  // semicolon
  try_annex_path_to_file_url ("/foo;bar", "file:///foo;bar");
  // percent
  try_annex_path_to_file_url ("/foo%bar", "file:///foo%25bar");
  // backslash
  try_annex_path_to_file_url ("/foo\\bar", "file:///foo%5Cbar");
  // backspace
  try_annex_path_to_file_url ("/foo\bbar", "file:///foo%08bar");
  // tab
  try_annex_path_to_file_url ("/foo\tbar", "file:///foo%09bar");
  // newline
  try_annex_path_to_file_url ("/foo\nbar", "file:///foo%0Abar");
  // carriage return
  try_annex_path_to_file_url ("/foo\rbar", "file:///foo%0Dbar");
  // latin1
  try_annex_path_to_file_url ("/fóóbàr", "file:///f%C3%B3%C3%B3b%C3%A0r");
  // Euro sign (BMP code point)
  try_annex_path_to_file_url ("/€", "file:///%E2%82%AC");
  // Rocket emoji (non-BMP code point)
  try_annex_path_to_file_url ("/🚀", "file:///%F0%9F%9A%80");
#endif
  
  jjs_cleanup ();
  return 0;
} /* main */
