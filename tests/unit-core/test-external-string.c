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

static int free_count = 0;

static const char *external_1 = "External string! External string! External string! External string!";
static const char *external_2 = "Object";
static const char *external_3 = "x!?:s";
static const char *external_4 = "Object property external string! Object property external string!";

static void
external_string_free_callback_1 (jjs_char_t *string_p, /**< string pointer */
                                 jjs_size_t string_size, /**< size of the string */
                                 void *user_p) /**< user pointer */
{
  TEST_ASSERT ((const char *) string_p == external_1);
  TEST_ASSERT (string_size == strlen (external_1));
  TEST_ASSERT (user_p == NULL);
  free_count++;
} /* external_string_free_callback_1 */

static void
external_string_free_callback_2 (jjs_char_t *string_p, /**< string pointer */
                                 jjs_size_t string_size, /**< size of the string */
                                 void *user_p) /**< user pointer */
{
  TEST_ASSERT ((const char *) string_p == external_2);
  TEST_ASSERT (string_size == strlen (external_2));
  TEST_ASSERT (user_p == (void *) &free_count);
  free_count++;
} /* external_string_free_callback_2 */

static void
external_string_free_callback_3 (jjs_char_t *string_p, /**< string pointer */
                                 jjs_size_t string_size, /**< size of the string */
                                 void *user_p) /**< user pointer */
{
  TEST_ASSERT ((const char *) string_p == external_3);
  TEST_ASSERT (string_size == strlen (external_3));
  TEST_ASSERT (user_p == (void *) string_p);
  free_count++;
} /* external_string_free_callback_3 */

int
main (void)
{
  ctx_open (NULL);

  bool is_external;

  /* Test external callback calls. */
  jjs_string_external_on_free (ctx (), external_string_free_callback_1);
  jjs_value_t external_string = jjs_string_external_sz (ctx (), external_1, NULL);
  TEST_ASSERT (free_count == 0);
  TEST_ASSERT (jjs_string_user_ptr (ctx (), external_string, &is_external) == NULL);
  TEST_ASSERT (is_external);
  TEST_ASSERT (jjs_string_user_ptr (ctx (), external_string, NULL) == NULL);
  jjs_value_free (ctx (), external_string);
  TEST_ASSERT (free_count == 1);

  jjs_string_external_on_free (ctx (), NULL);
  external_string = jjs_string_external_sz (ctx (), external_1, (void *) &free_count);
  TEST_ASSERT (free_count == 1);
  TEST_ASSERT (jjs_string_user_ptr (ctx (), external_string, &is_external) == (void *) &free_count);
  TEST_ASSERT (is_external);
  TEST_ASSERT (jjs_string_user_ptr (ctx (), external_string, NULL) == (void *) &free_count);
  jjs_value_free (ctx (), external_string);
  TEST_ASSERT (free_count == 1);

  jjs_string_external_on_free (ctx (), external_string_free_callback_2);
  external_string = jjs_string_external_sz (ctx (), external_2, (void *) &free_count);
  TEST_ASSERT (free_count == 2);
  TEST_ASSERT (jjs_string_user_ptr (ctx (), external_string, &is_external) == NULL);
  TEST_ASSERT (!is_external);
  jjs_value_free (ctx (), external_string);
  TEST_ASSERT (free_count == 2);

  jjs_string_external_on_free (ctx (), NULL);
  external_string = jjs_string_external_sz (ctx (), external_2, (void *) &free_count);
  TEST_ASSERT (free_count == 2);
  TEST_ASSERT (jjs_string_user_ptr (ctx (), external_string, &is_external) == NULL);
  TEST_ASSERT (!is_external);
  jjs_value_free (ctx (), external_string);
  TEST_ASSERT (free_count == 2);

  jjs_string_external_on_free (ctx (), external_string_free_callback_3);
  external_string = jjs_string_external_sz (ctx (), external_3, (void *) external_3);
  TEST_ASSERT (free_count == 3);
  TEST_ASSERT (jjs_string_user_ptr (ctx (), external_string, &is_external) == NULL);
  TEST_ASSERT (!is_external);
  jjs_value_free (ctx (), external_string);
  TEST_ASSERT (free_count == 3);

  jjs_string_external_on_free (ctx (), NULL);
  external_string = jjs_string_external_sz (ctx (), external_3, (void *) external_3);
  TEST_ASSERT (free_count == 3);
  TEST_ASSERT (jjs_string_user_ptr (ctx (), external_string, &is_external) == NULL);
  TEST_ASSERT (!is_external);
  jjs_value_free (ctx (), external_string);
  TEST_ASSERT (free_count == 3);

  /* Test string comparison. */
  jjs_string_external_on_free (ctx (), external_string_free_callback_1);
  external_string = jjs_string_external_sz (ctx (), external_1, NULL);
  jjs_value_t other_string = jjs_string_sz (ctx (), external_1);

  jjs_value_t result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, external_string, other_string);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result));
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, external_string, external_string);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result));
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  TEST_ASSERT (free_count == 3);
  jjs_value_free (ctx (), external_string);
  TEST_ASSERT (free_count == 4);
  jjs_value_free (ctx (), other_string);

  /* Test getting string. */
  jjs_string_external_on_free (ctx (), external_string_free_callback_1);
  external_string = jjs_string_external_sz (ctx (), external_1, NULL);
  size_t length = strlen (external_1);

  TEST_ASSERT (jjs_value_is_string (ctx (), external_string));
  TEST_ASSERT (jjs_string_size (ctx (), external_string, JJS_ENCODING_CESU8) == length);
  TEST_ASSERT (jjs_string_length (ctx (), external_string) == length);

  jjs_char_t buf[128];
  jjs_string_to_buffer (ctx (), external_string, JJS_ENCODING_CESU8, buf, sizeof (buf));
  TEST_ASSERT (memcmp (buf, external_1, length) == 0);

  TEST_ASSERT (free_count == 4);
  jjs_value_free (ctx (), external_string);
  TEST_ASSERT (free_count == 5);

  /* Test property access. */
  jjs_string_external_on_free (ctx (), NULL);
  external_string = jjs_string_external_sz (ctx (), external_4, NULL);
  other_string = jjs_string_sz (ctx (), external_4);

  jjs_value_t obj = jjs_object (ctx ());
  result = jjs_object_set (ctx (), obj, external_string, other_string);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result));
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  jjs_value_t get_result = jjs_object_get (ctx (), obj, other_string);
  TEST_ASSERT (jjs_value_is_string (ctx (), get_result));

  result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, get_result, external_string);
  jjs_value_free (ctx (), get_result);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result));
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_object_set (ctx (), obj, other_string, external_string);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result));
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  get_result = jjs_object_get (ctx (), obj, external_string);
  TEST_ASSERT (jjs_value_is_string (ctx (), get_result));

  result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, get_result, other_string);
  jjs_value_free (ctx (), get_result);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result));
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), obj);
  jjs_value_free (ctx (), external_string);
  jjs_value_free (ctx (), other_string);

  external_string = jjs_boolean (ctx (), true);
  TEST_ASSERT (jjs_string_user_ptr (ctx (), external_string, &is_external) == NULL);
  TEST_ASSERT (!is_external);
  jjs_value_free (ctx (), external_string);

  external_string = jjs_object (ctx ());
  TEST_ASSERT (jjs_string_user_ptr (ctx (), external_string, &is_external) == NULL);
  TEST_ASSERT (!is_external);
  jjs_value_free (ctx (), external_string);

  ctx_close ();
  return 0;
} /* main */
