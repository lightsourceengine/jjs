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

static void
compare_str (jjs_value_t value, const char *str_p, size_t str_len)
{
  jjs_size_t size = jjs_string_size (ctx (), value, JJS_ENCODING_CESU8);
  TEST_ASSERT (str_len == size);
  JJS_VLA (jjs_char_t, str_buff, size);
  jjs_string_to_buffer (ctx (), value, JJS_ENCODING_CESU8, str_buff, size);
  TEST_ASSERT (!memcmp (str_p, str_buff, str_len));
} /* compare_str */

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t obj_val = jjs_object (ctx ());
  obj_val = jjs_throw_value (ctx (), obj_val, true);
  jjs_value_t err_val = jjs_value_copy (ctx (), obj_val);

  obj_val = jjs_exception_value (ctx (), err_val, true);

  TEST_ASSERT (obj_val != err_val);
  jjs_value_free (ctx (), err_val);
  jjs_value_free (ctx (), obj_val);

  const char pterodactylus[] = "Pterodactylus";
  const size_t pterodactylus_size = sizeof (pterodactylus) - 1;

  jjs_value_t str = jjs_string_sz (ctx (), pterodactylus);
  jjs_value_t error = jjs_throw_value (ctx (), str, true);
  str = jjs_exception_value (ctx (), error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (ctx (), str);

  str = jjs_string_sz (ctx (), pterodactylus);
  error = jjs_throw_value (ctx (), str, false);
  jjs_value_free (ctx (), str);
  str = jjs_exception_value (ctx (), error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (ctx (), str);

  str = jjs_string_sz (ctx (), pterodactylus);
  error = jjs_throw_abort (ctx (), str, true);
  str = jjs_exception_value (ctx (), error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (ctx (), str);

  str = jjs_string_sz (ctx (), pterodactylus);
  error = jjs_throw_abort (ctx (), str, false);
  jjs_value_free (ctx (), str);
  str = jjs_exception_value (ctx (), error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (ctx (), str);

  str = jjs_string_sz (ctx (), pterodactylus);
  error = jjs_throw_value (ctx (), str, true);
  error = jjs_throw_abort (ctx (), error, true);
  TEST_ASSERT (jjs_value_is_abort (ctx (), error));
  str = jjs_exception_value (ctx (), error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (ctx (), str);

  str = jjs_string_sz (ctx (), pterodactylus);
  error = jjs_throw_value (ctx (), str, true);
  jjs_value_t error2 = jjs_throw_abort (ctx (), error, false);
  TEST_ASSERT (jjs_value_is_abort (ctx (), error2));
  jjs_value_free (ctx (), error);
  str = jjs_exception_value (ctx (), error2, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (ctx (), str);

  double test_num = 3.1415926;
  jjs_value_t num = jjs_number (ctx (), test_num);
  jjs_value_t num2 = jjs_throw_value (ctx (), num, false);
  TEST_ASSERT (jjs_value_is_exception (ctx (), num2));
  jjs_value_free (ctx (), num);
  num2 = jjs_exception_value (ctx (), num2, true);
  TEST_ASSERT (jjs_value_as_number (ctx (), num2) == test_num);
  jjs_value_free (ctx (), num2);

  num = jjs_number (ctx (), test_num);
  num2 = jjs_throw_value (ctx (), num, true);
  TEST_ASSERT (jjs_value_is_exception (ctx (), num2));
  num2 = jjs_exception_value (ctx (), num2, true);
  TEST_ASSERT (jjs_value_as_number (ctx (), num2) == test_num);
  jjs_value_free (ctx (), num2);

  num = jjs_number (ctx (), test_num);
  num2 = jjs_throw_value (ctx (), num, false);
  TEST_ASSERT (jjs_value_is_exception (ctx (), num2));
  jjs_value_free (ctx (), num);
  jjs_value_t num3 = jjs_throw_value (ctx (), num2, false);
  TEST_ASSERT (jjs_value_is_exception (ctx (), num3));
  jjs_value_free (ctx (), num2);
  num2 = jjs_exception_value (ctx (), num3, true);
  TEST_ASSERT (jjs_value_as_number (ctx (), num2) == test_num);
  jjs_value_free (ctx (), num2);

  num = jjs_number (ctx (), test_num);
  num2 = jjs_throw_value (ctx (), num, true);
  TEST_ASSERT (jjs_value_is_exception (ctx (), num2));
  num3 = jjs_throw_value (ctx (), num2, true);
  TEST_ASSERT (jjs_value_is_exception (ctx (), num3));
  num2 = jjs_exception_value (ctx (), num3, true);
  TEST_ASSERT (jjs_value_as_number (ctx (), num2) == test_num);
  jjs_value_free (ctx (), num2);

  num = jjs_number (ctx (), test_num);
  error = jjs_throw_abort (ctx (), num, true);
  TEST_ASSERT (jjs_value_is_abort (ctx (), error));
  num2 = jjs_throw_value (ctx (), error, true);
  TEST_ASSERT (jjs_value_is_exception (ctx (), num2));
  num = jjs_exception_value (ctx (), num2, true);
  TEST_ASSERT (jjs_value_as_number (ctx (), num) == test_num);
  jjs_value_free (ctx (), num);

  num = jjs_number (ctx (), test_num);
  error = jjs_throw_abort (ctx (), num, false);
  jjs_value_free (ctx (), num);
  TEST_ASSERT (jjs_value_is_abort (ctx (), error));
  num2 = jjs_throw_value (ctx (), error, true);
  TEST_ASSERT (jjs_value_is_exception (ctx (), num2));
  num = jjs_exception_value (ctx (), num2, true);
  TEST_ASSERT (jjs_value_as_number (ctx (), num) == test_num);
  jjs_value_free (ctx (), num);

  num = jjs_number (ctx (), test_num);
  error = jjs_throw_abort (ctx (), num, true);
  TEST_ASSERT (jjs_value_is_abort (ctx (), error));
  num2 = jjs_throw_value (ctx (), error, false);
  jjs_value_free (ctx (), error);
  TEST_ASSERT (jjs_value_is_exception (ctx (), num2));
  num = jjs_exception_value (ctx (), num2, true);
  TEST_ASSERT (jjs_value_as_number (ctx (), num) == test_num);
  jjs_value_free (ctx (), num);

  num = jjs_number (ctx (), test_num);
  error = jjs_throw_abort (ctx (), num, false);
  jjs_value_free (ctx (), num);
  TEST_ASSERT (jjs_value_is_abort (ctx (), error));
  num2 = jjs_throw_value (ctx (), error, false);
  jjs_value_free (ctx (), error);
  TEST_ASSERT (jjs_value_is_exception (ctx (), num2));
  num = jjs_exception_value (ctx (), num2, true);
  TEST_ASSERT (jjs_value_as_number (ctx (), num) == test_num);
  jjs_value_free (ctx (), num);

  jjs_value_t value = jjs_number (ctx (), 42);
  value = jjs_exception_value (ctx (), value, true);
  jjs_value_free (ctx (), value);

  value = jjs_number (ctx (), 42);
  jjs_value_t value2 = jjs_exception_value (ctx (), value, false);
  jjs_value_free (ctx (), value);
  jjs_value_free (ctx (), value2);

  value = jjs_number (ctx (), 42);
  error = jjs_throw_value (ctx (), value, true);
  error = jjs_throw_value (ctx (), error, true);
  jjs_value_free (ctx (), error);

  value = jjs_number (ctx (), 42);
  error = jjs_throw_abort (ctx (), value, true);
  error = jjs_throw_abort (ctx (), error, true);
  jjs_value_free (ctx (), error);

  value = jjs_number (ctx (), 42);
  error = jjs_throw_value (ctx (), value, true);
  error2 = jjs_throw_value (ctx (), error, false);
  jjs_value_free (ctx (), error);
  jjs_value_free (ctx (), error2);

  value = jjs_number (ctx (), 42);
  error = jjs_throw_abort (ctx (), value, true);
  error2 = jjs_throw_abort (ctx (), error, false);
  jjs_value_free (ctx (), error);
  jjs_value_free (ctx (), error2);

  ctx_close ();
} /* main */
