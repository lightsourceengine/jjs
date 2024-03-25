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
  if (!jjs_feature_enabled (JJS_FEATURE_BIGINT))
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Bigint support is disabled!\n");
    return 0;
  }

  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  jjs_value_t string = jjs_string_sz ("0xfffffff1fffffff2fffffff3");
  TEST_ASSERT (!jjs_value_is_exception (string));

  jjs_value_t bigint = jjs_value_to_bigint (string);
  jjs_value_free (string);

  TEST_ASSERT (!jjs_value_is_exception (bigint));
  TEST_ASSERT (jjs_value_is_bigint (bigint));

  string = jjs_value_to_string (bigint);
  TEST_ASSERT (!jjs_value_is_exception (string));

  static jjs_char_t str_buffer[64];
  const char *expected_string_p = "79228162256009920505775652851";

  jjs_size_t size = jjs_string_to_buffer (string, JJS_ENCODING_CESU8, str_buffer, sizeof (str_buffer));
  TEST_ASSERT (size == strlen (expected_string_p));
  TEST_ASSERT (memcmp (str_buffer, expected_string_p, size) == 0);
  jjs_value_free (string);

  TEST_ASSERT (jjs_bigint_digit_count (bigint) == 2);

  uint64_t digits_buffer[4];
  bool sign;

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = true;
  jjs_bigint_to_digits (bigint, digits_buffer, 0, &sign);
  TEST_ASSERT (sign == false);
  TEST_ASSERT (digits_buffer[0] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[1] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = true;
  jjs_bigint_to_digits (bigint, digits_buffer, 1, &sign);
  TEST_ASSERT (sign == false);
  TEST_ASSERT (digits_buffer[0] == 0xfffffff2fffffff3ull);
  TEST_ASSERT (digits_buffer[1] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = true;
  jjs_bigint_to_digits (bigint, digits_buffer, 2, &sign);
  TEST_ASSERT (sign == false);
  TEST_ASSERT (digits_buffer[0] == 0xfffffff2fffffff3ull);
  TEST_ASSERT (digits_buffer[1] == 0xfffffff1ull);
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = true;
  jjs_bigint_to_digits (bigint, digits_buffer, 3, &sign);
  TEST_ASSERT (sign == false);
  TEST_ASSERT (digits_buffer[0] == 0xfffffff2fffffff3ull);
  TEST_ASSERT (digits_buffer[1] == 0xfffffff1ull);
  TEST_ASSERT (digits_buffer[2] == 0);
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  jjs_bigint_to_digits (bigint, digits_buffer, 4, NULL);
  TEST_ASSERT (digits_buffer[0] == 0xfffffff2fffffff3ull);
  TEST_ASSERT (digits_buffer[1] == 0xfffffff1ull);
  TEST_ASSERT (digits_buffer[2] == 0);
  TEST_ASSERT (digits_buffer[3] == 0);

  jjs_value_free (bigint);

  digits_buffer[0] = 0;
  digits_buffer[1] = 0;
  digits_buffer[2] = 0;
  /* Sign of zero value is always positive, even if we set negative. */
  bigint = jjs_bigint (digits_buffer, 3, true);
  TEST_ASSERT (jjs_value_is_bigint (bigint));
  TEST_ASSERT (jjs_bigint_digit_count (bigint) == 0);

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = true;
  jjs_bigint_to_digits (bigint, digits_buffer, 2, &sign);
  TEST_ASSERT (sign == false);
  TEST_ASSERT (digits_buffer[0] == 0);
  TEST_ASSERT (digits_buffer[1] == 0);
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  jjs_value_free (bigint);

  digits_buffer[0] = 1;
  digits_buffer[1] = 0;
  digits_buffer[2] = 0;
  digits_buffer[3] = 0;
  bigint = jjs_bigint (digits_buffer, 4, true);
  TEST_ASSERT (jjs_value_is_bigint (bigint));
  TEST_ASSERT (jjs_bigint_digit_count (bigint) == 1);

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = false;
  jjs_bigint_to_digits (bigint, digits_buffer, 1, &sign);
  TEST_ASSERT (sign == true);
  TEST_ASSERT (digits_buffer[0] == 1);
  TEST_ASSERT (digits_buffer[1] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = false;
  jjs_bigint_to_digits (bigint, digits_buffer, 2, &sign);
  TEST_ASSERT (sign == true);
  TEST_ASSERT (digits_buffer[0] == 1);
  TEST_ASSERT (digits_buffer[1] == 0);
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  jjs_value_free (bigint);

  digits_buffer[0] = 0;
  digits_buffer[1] = 1;
  digits_buffer[2] = 0;
  digits_buffer[3] = 0;
  bigint = jjs_bigint (digits_buffer, 4, true);
  TEST_ASSERT (jjs_value_is_bigint (bigint));
  TEST_ASSERT (jjs_bigint_digit_count (bigint) == 2);

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = false;
  jjs_bigint_to_digits (bigint, digits_buffer, 1, &sign);
  TEST_ASSERT (sign == true);
  TEST_ASSERT (digits_buffer[0] == 0);
  TEST_ASSERT (digits_buffer[1] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = false;
  jjs_bigint_to_digits (bigint, digits_buffer, 2, &sign);
  TEST_ASSERT (sign == true);
  TEST_ASSERT (digits_buffer[0] == 0);
  TEST_ASSERT (digits_buffer[1] == 1);
  TEST_ASSERT (digits_buffer[2] == ~((uint64_t) 0));
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  memset (digits_buffer, 0xff, sizeof (digits_buffer));
  sign = false;
  jjs_bigint_to_digits (bigint, digits_buffer, 3, &sign);
  TEST_ASSERT (sign == true);
  TEST_ASSERT (digits_buffer[0] == 0);
  TEST_ASSERT (digits_buffer[1] == 1);
  TEST_ASSERT (digits_buffer[2] == 0);
  TEST_ASSERT (digits_buffer[3] == ~((uint64_t) 0));

  jjs_value_free (bigint);

  jjs_cleanup ();
  return 0;
} /* main */
