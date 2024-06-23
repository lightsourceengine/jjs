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

static bool
test_syntax_error (char *script_p) /**< script */
{
  jjs_value_t parse_result = jjs_parse_sz (ctx (), script_p, NULL);

  bool result = false;

  if (jjs_value_is_exception (ctx (), parse_result))
  {
    result = true;
    TEST_ASSERT (jjs_error_type (ctx (), parse_result) == JJS_ERROR_SYNTAX);
  }

  jjs_value_free (ctx (), parse_result);
  return result;
} /* test_syntax_error */

int
main (void)
{
  ctx_open (NULL);

  if (!test_syntax_error ("\\u{61}"))
  {
    TEST_ASSERT (!test_syntax_error ("\xF0\x90\xB2\x80: break \\u{10C80}"));
    /* The \u surrogate pairs are ignored. The \u{hex} form must be used. */
    TEST_ASSERT (test_syntax_error ("\xF0\x90\xB2\x80: break \\ud803\\udc80"));
    /* The utf8 code point and the cesu8 surrogate pair must match. */
    TEST_ASSERT (!test_syntax_error ("\xF0\x90\xB2\x80: break \xed\xa0\x83\xed\xb2\x80"));

    TEST_ASSERT (!test_syntax_error ("$\xF0\x90\xB2\x80$: break $\\u{10C80}$"));
    TEST_ASSERT (test_syntax_error ("$\xF0\x90\xB2\x80$: break $\\ud803\\udc80$"));
    TEST_ASSERT (!test_syntax_error ("$\xF0\x90\xB2\x80$: break $\xed\xa0\x83\xed\xb2\x80$"));
  }

  ctx_close ();

  return 0;
} /* main */
