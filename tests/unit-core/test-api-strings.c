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
  jjs_size_t sz, utf8_sz, cesu8_sz;
  jjs_value_t args[2];

  ctx_open (NULL);

  /* Test corner case for jjs_string_to_char_buffer */
  args[0] = jjs_string_sz (ctx (), "");
  sz = jjs_string_size (ctx (), args[0], JJS_ENCODING_CESU8);
  TEST_ASSERT (sz == 0);
  jjs_value_free (ctx (), args[0]);

  /* Test create_jjs_string_from_utf8 with 4-byte long unicode sequences,
   * test string: 'str: {DESERET CAPITAL LETTER LONG I}'
   */
  char *utf8_bytes_p = "\x73\x74\x72\x3a \xf0\x90\x90\x80";
  char *cesu8_bytes_p = "\x73\x74\x72\x3a \xed\xa0\x81\xed\xb0\x80";
  args[0] = jjs_string (ctx (), (jjs_char_t *) utf8_bytes_p, (jjs_size_t) strlen (utf8_bytes_p), JJS_ENCODING_UTF8);
  args[1] = jjs_string (ctx (), (jjs_char_t *) cesu8_bytes_p, (jjs_size_t) strlen (cesu8_bytes_p), JJS_ENCODING_CESU8);

  /* These sizes must be equal */
  utf8_sz = jjs_string_size (ctx (), args[0], JJS_ENCODING_CESU8);
  cesu8_sz = jjs_string_size (ctx (), args[1], JJS_ENCODING_CESU8);

  JJS_VLA (jjs_char_t, string_from_utf8, utf8_sz);
  JJS_VLA (jjs_char_t, string_from_cesu8, cesu8_sz);

  jjs_string_to_buffer (ctx (), args[0], JJS_ENCODING_CESU8, string_from_utf8, utf8_sz);
  jjs_string_to_buffer (ctx (), args[1], JJS_ENCODING_CESU8, string_from_cesu8, cesu8_sz);

  TEST_ASSERT (utf8_sz == cesu8_sz);

  TEST_ASSERT (!memcmp (string_from_utf8, string_from_cesu8, utf8_sz));
  jjs_value_free (ctx (), args[0]);
  jjs_value_free (ctx (), args[1]);

  /* Test jjs_string_to_buffer, test string: 'str: {DESERET CAPITAL LETTER LONG I}' */
  utf8_bytes_p = "\x73\x74\x72\x3a \xf0\x90\x90\x80";
  cesu8_bytes_p = "\x73\x74\x72\x3a \xed\xa0\x81\xed\xb0\x80";
  args[0] = jjs_string (ctx (), (jjs_char_t *) utf8_bytes_p, (jjs_size_t) strlen (utf8_bytes_p), JJS_ENCODING_UTF8);
  args[1] = jjs_string (ctx (), (jjs_char_t *) cesu8_bytes_p, (jjs_size_t) strlen (cesu8_bytes_p), JJS_ENCODING_CESU8);

  /* Test that the strings are equal / ensure hashes are equal */
  TEST_ASSERT (strict_equals (ctx (), args[0], args[1]));

  /* These sizes must be equal */
  utf8_sz = jjs_string_size (ctx (), args[0], JJS_ENCODING_UTF8);
  cesu8_sz = jjs_string_size (ctx (), args[1], JJS_ENCODING_UTF8);

  TEST_ASSERT (utf8_sz == cesu8_sz && utf8_sz > 0);

  JJS_VLA (jjs_char_t, string_from_utf8_string, utf8_sz);
  JJS_VLA (jjs_char_t, string_from_cesu8_string, cesu8_sz);

  jjs_string_to_buffer (ctx (), args[0], JJS_ENCODING_UTF8, string_from_utf8_string, utf8_sz);
  jjs_string_to_buffer (ctx (), args[1], JJS_ENCODING_UTF8, string_from_cesu8_string, cesu8_sz);

  TEST_ASSERT (!memcmp (string_from_utf8_string, string_from_cesu8_string, utf8_sz));
  jjs_value_free (ctx (), args[0]);
  jjs_value_free (ctx (), args[1]);

  /* Test string: 'str: {MATHEMATICAL FRAKTUR SMALL F}{MATHEMATICAL FRAKTUR SMALL G}' */
  utf8_bytes_p = "\x73\x74\x72\x3a \xf0\x9d\x94\xa3 \xf0\x9d\x94\xa4";
  args[0] = jjs_string (ctx (), (jjs_char_t *) utf8_bytes_p, (jjs_size_t) strlen (utf8_bytes_p), JJS_ENCODING_UTF8);

  cesu8_sz = jjs_string_size (ctx (), args[0], JJS_ENCODING_CESU8);
  utf8_sz = jjs_string_size (ctx (), args[0], JJS_ENCODING_UTF8);

  TEST_ASSERT (jjs_string_length (ctx (), args[0]) == 10);
  TEST_ASSERT (cesu8_sz != utf8_sz);
  TEST_ASSERT (utf8_sz == 14 && cesu8_sz == 18);

  JJS_VLA (char, test_string, utf8_sz);

  TEST_ASSERT (jjs_string_to_buffer (ctx (), args[0], JJS_ENCODING_UTF8, (jjs_char_t *) test_string, utf8_sz) == 14);
  TEST_ASSERT (!strncmp (test_string, utf8_bytes_p, utf8_sz));

  jjs_value_free (ctx (), args[0]);

  /* Test string: 'str: {DESERET CAPITAL LETTER LONG I}' */
  cesu8_bytes_p = "\x73\x74\x72\x3a \xed\xa0\x81\xed\xb0\x80";
  args[0] = jjs_string (ctx (), (jjs_char_t *) cesu8_bytes_p, (jjs_size_t) strlen (cesu8_bytes_p), JJS_ENCODING_CESU8);

  cesu8_sz = jjs_string_size (ctx (), args[0], JJS_ENCODING_CESU8);
  utf8_sz = jjs_string_size (ctx (), args[0], JJS_ENCODING_UTF8);

  TEST_ASSERT (jjs_string_length (ctx (), args[0]) == 7);
  TEST_ASSERT (cesu8_sz != utf8_sz);
  TEST_ASSERT (utf8_sz == 9 && cesu8_sz == 11);

  jjs_value_free (ctx (), args[0]);

  /* Test string: 'price: 10{EURO SIGN}' */
  utf8_bytes_p = "\x70\x72\x69\x63\x65\x3a \x31\x30\xe2\x82\xac";
  args[0] = jjs_string (ctx (), (jjs_char_t *) utf8_bytes_p, (jjs_size_t) strlen (utf8_bytes_p), JJS_ENCODING_UTF8);

  cesu8_sz = jjs_string_size (ctx (), args[0], JJS_ENCODING_CESU8);
  utf8_sz = jjs_string_size (ctx (), args[0], JJS_ENCODING_UTF8);

  TEST_ASSERT (jjs_string_length (ctx (), args[0]) == 10);
  TEST_ASSERT (cesu8_sz == utf8_sz);
  TEST_ASSERT (utf8_sz == 12);
  jjs_value_free (ctx (), args[0]);

  /* Test string: '3' */
  {
    jjs_value_t test_str = jjs_string_sz (ctx (), "3");
    char result_string[1] = { 'E' };
    jjs_size_t copied_utf8 =
      jjs_string_to_buffer (ctx (), test_str, JJS_ENCODING_UTF8, (jjs_char_t *) result_string, sizeof (result_string));
    TEST_ASSERT (copied_utf8 == 1);
    TEST_ASSERT (result_string[0] == '3');

    result_string[0] = 'E';
    jjs_size_t copied =
      jjs_string_to_buffer (ctx (), test_str, JJS_ENCODING_CESU8, (jjs_char_t *) result_string, sizeof (result_string));
    TEST_ASSERT (copied == 1);
    TEST_ASSERT (result_string[0] == '3');

    jjs_value_free (ctx (), test_str);
  }

  ctx_close ();

  return 0;
} /* main */
