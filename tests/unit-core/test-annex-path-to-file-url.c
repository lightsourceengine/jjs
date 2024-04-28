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
try_annex_path_to_file_url (const char* input, const char* expected_output)
{
  ecma_value_t input_value = annex_util_create_string_utf8_sz(input);
  ecma_value_t output_value = annex_path_to_file_url (input_value);

  TEST_ASSERT (ecma_is_value_string(output_value));

  bool result = strict_equals_cstr (output_value, expected_output);

  ecma_free_value (input_value);
  ecma_free_value (output_value);

  TEST_ASSERT (result);
}

static void
try_annex_path_to_file_url_bad_input (ecma_value_t input)
{
  TEST_ASSERT (ecma_is_value_empty(annex_path_to_file_url (input)));
  ecma_free_value (input);
}

static void
test_annex_path_to_file_url(void)
{
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
  try_annex_path_to_file_url ("C:\\foo?bar", "file:///C:/foo%3Fbar");
  // number sign
  try_annex_path_to_file_url ("C:\\foo#bar", "file:///C:/foo%23bar");
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
  try_annex_path_to_file_url ("/foo?bar", "file:///foo%3Fbar");
  // number sign
  try_annex_path_to_file_url ("/foo#bar", "file:///foo%23bar");
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
}

static void
test_annex_path_to_file_url_bad_input (void)
{
  try_annex_path_to_file_url_bad_input (ecma_make_boolean_value (true));
  try_annex_path_to_file_url_bad_input (ecma_make_int32_value(123));
  try_annex_path_to_file_url_bad_input (jjs_object());
  try_annex_path_to_file_url_bad_input (jjs_array(1));
  try_annex_path_to_file_url_bad_input (jjs_symbol (JJS_SYMBOL_MATCH));
  try_annex_path_to_file_url_bad_input (ecma_string_ascii_sz (""));
  try_annex_path_to_file_url_bad_input (ecma_string_ascii_sz ("./relative-path"));
  try_annex_path_to_file_url_bad_input (ecma_string_ascii_sz ("../relative-path"));
  try_annex_path_to_file_url_bad_input (ecma_string_ascii_sz ("relative-path"));
}

int
main (void)
{
  TEST_INIT ();

  TEST_CONTEXT_NEW (context_p);

  test_annex_path_to_file_url ();
  test_annex_path_to_file_url_bad_input ();
  
  TEST_CONTEXT_FREE (context_p);
  return 0;
} /* main */
