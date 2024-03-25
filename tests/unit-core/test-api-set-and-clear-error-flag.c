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

static void
compare_str (jjs_value_t value, const char *str_p, size_t str_len)
{
  jjs_size_t size = jjs_string_size (value, JJS_ENCODING_CESU8);
  TEST_ASSERT (str_len == size);
  JJS_VLA (jjs_char_t, str_buff, size);
  jjs_string_to_buffer (value, JJS_ENCODING_CESU8, str_buff, size);
  TEST_ASSERT (!memcmp (str_p, str_buff, str_len));
} /* compare_str */

int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  jjs_value_t obj_val = jjs_object ();
  obj_val = jjs_throw_value (obj_val, true);
  jjs_value_t err_val = jjs_value_copy (obj_val);

  obj_val = jjs_exception_value (err_val, true);

  TEST_ASSERT (obj_val != err_val);
  jjs_value_free (err_val);
  jjs_value_free (obj_val);

  const char pterodactylus[] = "Pterodactylus";
  const size_t pterodactylus_size = sizeof (pterodactylus) - 1;

  jjs_value_t str = jjs_string_sz (pterodactylus);
  jjs_value_t error = jjs_throw_value (str, true);
  str = jjs_exception_value (error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (str);

  str = jjs_string_sz (pterodactylus);
  error = jjs_throw_value (str, false);
  jjs_value_free (str);
  str = jjs_exception_value (error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (str);

  str = jjs_string_sz (pterodactylus);
  error = jjs_throw_abort (str, true);
  str = jjs_exception_value (error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (str);

  str = jjs_string_sz (pterodactylus);
  error = jjs_throw_abort (str, false);
  jjs_value_free (str);
  str = jjs_exception_value (error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (str);

  str = jjs_string_sz (pterodactylus);
  error = jjs_throw_value (str, true);
  error = jjs_throw_abort (error, true);
  TEST_ASSERT (jjs_value_is_abort (error));
  str = jjs_exception_value (error, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (str);

  str = jjs_string_sz (pterodactylus);
  error = jjs_throw_value (str, true);
  jjs_value_t error2 = jjs_throw_abort (error, false);
  TEST_ASSERT (jjs_value_is_abort (error2));
  jjs_value_free (error);
  str = jjs_exception_value (error2, true);

  compare_str (str, pterodactylus, pterodactylus_size);
  jjs_value_free (str);

  double test_num = 3.1415926;
  jjs_value_t num = jjs_number (test_num);
  jjs_value_t num2 = jjs_throw_value (num, false);
  TEST_ASSERT (jjs_value_is_exception (num2));
  jjs_value_free (num);
  num2 = jjs_exception_value (num2, true);
  TEST_ASSERT (jjs_value_as_number (num2) == test_num);
  jjs_value_free (num2);

  num = jjs_number (test_num);
  num2 = jjs_throw_value (num, true);
  TEST_ASSERT (jjs_value_is_exception (num2));
  num2 = jjs_exception_value (num2, true);
  TEST_ASSERT (jjs_value_as_number (num2) == test_num);
  jjs_value_free (num2);

  num = jjs_number (test_num);
  num2 = jjs_throw_value (num, false);
  TEST_ASSERT (jjs_value_is_exception (num2));
  jjs_value_free (num);
  jjs_value_t num3 = jjs_throw_value (num2, false);
  TEST_ASSERT (jjs_value_is_exception (num3));
  jjs_value_free (num2);
  num2 = jjs_exception_value (num3, true);
  TEST_ASSERT (jjs_value_as_number (num2) == test_num);
  jjs_value_free (num2);

  num = jjs_number (test_num);
  num2 = jjs_throw_value (num, true);
  TEST_ASSERT (jjs_value_is_exception (num2));
  num3 = jjs_throw_value (num2, true);
  TEST_ASSERT (jjs_value_is_exception (num3));
  num2 = jjs_exception_value (num3, true);
  TEST_ASSERT (jjs_value_as_number (num2) == test_num);
  jjs_value_free (num2);

  num = jjs_number (test_num);
  error = jjs_throw_abort (num, true);
  TEST_ASSERT (jjs_value_is_abort (error));
  num2 = jjs_throw_value (error, true);
  TEST_ASSERT (jjs_value_is_exception (num2));
  num = jjs_exception_value (num2, true);
  TEST_ASSERT (jjs_value_as_number (num) == test_num);
  jjs_value_free (num);

  num = jjs_number (test_num);
  error = jjs_throw_abort (num, false);
  jjs_value_free (num);
  TEST_ASSERT (jjs_value_is_abort (error));
  num2 = jjs_throw_value (error, true);
  TEST_ASSERT (jjs_value_is_exception (num2));
  num = jjs_exception_value (num2, true);
  TEST_ASSERT (jjs_value_as_number (num) == test_num);
  jjs_value_free (num);

  num = jjs_number (test_num);
  error = jjs_throw_abort (num, true);
  TEST_ASSERT (jjs_value_is_abort (error));
  num2 = jjs_throw_value (error, false);
  jjs_value_free (error);
  TEST_ASSERT (jjs_value_is_exception (num2));
  num = jjs_exception_value (num2, true);
  TEST_ASSERT (jjs_value_as_number (num) == test_num);
  jjs_value_free (num);

  num = jjs_number (test_num);
  error = jjs_throw_abort (num, false);
  jjs_value_free (num);
  TEST_ASSERT (jjs_value_is_abort (error));
  num2 = jjs_throw_value (error, false);
  jjs_value_free (error);
  TEST_ASSERT (jjs_value_is_exception (num2));
  num = jjs_exception_value (num2, true);
  TEST_ASSERT (jjs_value_as_number (num) == test_num);
  jjs_value_free (num);

  jjs_value_t value = jjs_number (42);
  value = jjs_exception_value (value, true);
  jjs_value_free (value);

  value = jjs_number (42);
  jjs_value_t value2 = jjs_exception_value (value, false);
  jjs_value_free (value);
  jjs_value_free (value2);

  value = jjs_number (42);
  error = jjs_throw_value (value, true);
  error = jjs_throw_value (error, true);
  jjs_value_free (error);

  value = jjs_number (42);
  error = jjs_throw_abort (value, true);
  error = jjs_throw_abort (error, true);
  jjs_value_free (error);

  value = jjs_number (42);
  error = jjs_throw_value (value, true);
  error2 = jjs_throw_value (error, false);
  jjs_value_free (error);
  jjs_value_free (error2);

  value = jjs_number (42);
  error = jjs_throw_abort (value, true);
  error2 = jjs_throw_abort (error, false);
  jjs_value_free (error);
  jjs_value_free (error2);

  jjs_cleanup ();
} /* main */
