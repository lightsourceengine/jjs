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
  ctx_open (NULL);

  jjs_value_t object = jjs_object (ctx ());

  jjs_value_t prop_name_1 = jjs_string_sz (ctx (), "foo");
  jjs_value_t prop_name_2 = jjs_string_sz (ctx (), "non_hidden_prop");
  jjs_value_t prop_name_3;

  prop_name_3 = jjs_symbol_with_description_sz (ctx (), "bar");

  jjs_value_t internal_prop_name_1 = jjs_string_sz (ctx (), "hidden_foo");
  jjs_value_t internal_prop_name_2 = jjs_string_sz (ctx (), "hidden_prop");
  jjs_value_t internal_prop_name_3;

  internal_prop_name_3 = jjs_symbol_with_description_sz (ctx (), "bar");

  jjs_value_t prop_value_1 = jjs_number (ctx (), 5.5);
  jjs_value_t prop_value_2 = jjs_number (ctx (), 6.5);
  jjs_value_t prop_value_3 = jjs_number (ctx (), 7.5);

  jjs_value_t internal_prop_value_1 = jjs_number (ctx (), 8.5);
  jjs_value_t internal_prop_value_2 = jjs_number (ctx (), 9.5);
  jjs_value_t internal_prop_value_3 = jjs_number (ctx (), 10.5);

  /* Test the normal [[Set]] method */
  bool set_result_1 = jjs_object_set (ctx (), object, prop_name_1, prop_value_1);
  bool set_result_2 = jjs_object_set (ctx (), object, prop_name_2, prop_value_2);
  bool set_result_3 = jjs_object_set (ctx (), object, prop_name_3, prop_value_3);

  TEST_ASSERT (set_result_1);
  TEST_ASSERT (set_result_2);
  TEST_ASSERT (set_result_3);

  /* Test the internal [[Set]] method */
  bool set_internal_result_1 = jjs_object_set_internal (ctx (), object, internal_prop_name_1, internal_prop_value_1);
  bool set_internal_result_2 = jjs_object_set_internal (ctx (), object, internal_prop_name_2, internal_prop_value_2);
  bool set_internal_result_3 = jjs_object_set_internal (ctx (), object, internal_prop_name_3, internal_prop_value_3);

  TEST_ASSERT (set_internal_result_1);
  TEST_ASSERT (set_internal_result_2);
  TEST_ASSERT (set_internal_result_3);

  /* Test the normal [[Has]] method. */
  jjs_value_t has_result_1 = jjs_object_has (ctx (), object, prop_name_1);
  jjs_value_t has_result_2 = jjs_object_has (ctx (), object, prop_name_2);
  jjs_value_t has_result_3 = jjs_object_has (ctx (), object, prop_name_3);
  jjs_value_t has_result_4 = jjs_object_has (ctx (), object, internal_prop_name_1);
  jjs_value_t has_result_5 = jjs_object_has (ctx (), object, internal_prop_name_2);
  jjs_value_t has_result_6 = jjs_object_has (ctx (), object, internal_prop_name_3);

  TEST_ASSERT (jjs_value_is_boolean (ctx (), has_result_1) && jjs_value_is_true (ctx (), has_result_1));
  TEST_ASSERT (jjs_value_is_boolean (ctx (), has_result_2) && jjs_value_is_true (ctx (), has_result_2));
  TEST_ASSERT (jjs_value_is_boolean (ctx (), has_result_3) && jjs_value_is_true (ctx (), has_result_3));
  TEST_ASSERT (jjs_value_is_boolean (ctx (), has_result_4) && !jjs_value_is_true (ctx (), has_result_4));
  TEST_ASSERT (jjs_value_is_boolean (ctx (), has_result_5) && !jjs_value_is_true (ctx (), has_result_5));
  TEST_ASSERT (jjs_value_is_boolean (ctx (), has_result_6) && !jjs_value_is_true (ctx (), has_result_6));

  jjs_value_free (ctx (), has_result_1);
  jjs_value_free (ctx (), has_result_2);
  jjs_value_free (ctx (), has_result_3);
  jjs_value_free (ctx (), has_result_4);
  jjs_value_free (ctx (), has_result_5);
  jjs_value_free (ctx (), has_result_6);

  /* Test the internal [[Has]] method. */
  bool has_internal_result_1 = jjs_object_has_internal (ctx (), object, prop_name_1);
  bool has_internal_result_2 = jjs_object_has_internal (ctx (), object, prop_name_2);
  bool has_internal_result_3 = jjs_object_has_internal (ctx (), object, prop_name_3);
  bool has_internal_result_4 = jjs_object_has_internal (ctx (), object, internal_prop_name_1);
  bool has_internal_result_5 = jjs_object_has_internal (ctx (), object, internal_prop_name_2);
  bool has_internal_result_6 = jjs_object_has_internal (ctx (), object, internal_prop_name_3);

  TEST_ASSERT (!has_internal_result_1);
  TEST_ASSERT (!has_internal_result_2);
  TEST_ASSERT (!has_internal_result_3);
  TEST_ASSERT (has_internal_result_4);
  TEST_ASSERT (has_internal_result_5);
  TEST_ASSERT (has_internal_result_6);

  /* Test the normal [[Get]] method. */
  jjs_value_t get_result_1 = jjs_object_get (ctx (), object, prop_name_1);
  jjs_value_t get_result_2 = jjs_object_get (ctx (), object, prop_name_2);
  jjs_value_t get_result_3 = jjs_object_get (ctx (), object, prop_name_3);
  jjs_value_t get_result_4 = jjs_object_get (ctx (), object, internal_prop_name_1);
  jjs_value_t get_result_5 = jjs_object_get (ctx (), object, internal_prop_name_2);
  jjs_value_t get_result_6 = jjs_object_get (ctx (), object, internal_prop_name_3);

  TEST_ASSERT (jjs_value_is_number (ctx (), get_result_1) && jjs_value_as_number (ctx (), get_result_1) == 5.5);
  TEST_ASSERT (jjs_value_is_number (ctx (), get_result_2) && jjs_value_as_number (ctx (), get_result_2) == 6.5);
  TEST_ASSERT (jjs_value_is_number (ctx (), get_result_3) && jjs_value_as_number (ctx (), get_result_3) == 7.5);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), get_result_4));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), get_result_5));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), get_result_6));

  jjs_value_free (ctx (), get_result_1);
  jjs_value_free (ctx (), get_result_2);
  jjs_value_free (ctx (), get_result_3);
  jjs_value_free (ctx (), get_result_4);
  jjs_value_free (ctx (), get_result_5);
  jjs_value_free (ctx (), get_result_6);

  /* Test the internal [[Get]] method. */
  jjs_value_t get_internal_result_1 = jjs_object_get_internal (ctx (), object, prop_name_1);
  jjs_value_t get_internal_result_2 = jjs_object_get_internal (ctx (), object, prop_name_2);
  jjs_value_t get_internal_result_3 = jjs_object_get_internal (ctx (), object, prop_name_3);
  jjs_value_t get_internal_result_4 = jjs_object_get_internal (ctx (), object, internal_prop_name_1);
  jjs_value_t get_internal_result_5 = jjs_object_get_internal (ctx (), object, internal_prop_name_2);
  jjs_value_t get_internal_result_6 = jjs_object_get_internal (ctx (), object, internal_prop_name_3);

  TEST_ASSERT (jjs_value_is_undefined (ctx (), get_internal_result_1));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), get_internal_result_2));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), get_internal_result_3));
  TEST_ASSERT (jjs_value_is_number (ctx (), get_internal_result_4) && jjs_value_as_number (ctx (), get_internal_result_4) == 8.5);
  TEST_ASSERT (jjs_value_is_number (ctx (), get_internal_result_5) && jjs_value_as_number (ctx (), get_internal_result_5) == 9.5);
  TEST_ASSERT (jjs_value_is_number (ctx (), get_internal_result_6) && jjs_value_as_number (ctx (), get_internal_result_6) == 10.5);

  jjs_value_free (ctx (), get_internal_result_1);
  jjs_value_free (ctx (), get_internal_result_2);
  jjs_value_free (ctx (), get_internal_result_3);
  jjs_value_free (ctx (), get_internal_result_4);
  jjs_value_free (ctx (), get_internal_result_5);
  jjs_value_free (ctx (), get_internal_result_6);

  /* Test the normal [[Delete]] method. */
  jjs_value_t delete_result_1 = jjs_object_delete (ctx (), object, prop_name_1);
  jjs_value_t delete_result_2 = jjs_object_delete (ctx (), object, prop_name_2);
  jjs_value_t delete_result_3 = jjs_object_delete (ctx (), object, prop_name_3);
  jjs_value_t delete_result_4 = jjs_object_delete (ctx (), object, internal_prop_name_1);
  jjs_value_t delete_result_5 = jjs_object_delete (ctx (), object, internal_prop_name_2);
  jjs_value_t delete_result_6 = jjs_object_delete (ctx (), object, internal_prop_name_3);

  TEST_ASSERT (jjs_value_is_true (ctx (), delete_result_1));
  TEST_ASSERT (jjs_value_is_true (ctx (), delete_result_2));
  TEST_ASSERT (jjs_value_is_true (ctx (), delete_result_3));
  TEST_ASSERT (jjs_value_is_true (ctx (), delete_result_4));
  TEST_ASSERT (jjs_value_is_true (ctx (), delete_result_5));
  TEST_ASSERT (jjs_value_is_true (ctx (), delete_result_6));

  jjs_value_t has_after_delete_result_1 = jjs_object_has (ctx (), object, prop_name_1);
  jjs_value_t has_after_delete_result_2 = jjs_object_has (ctx (), object, prop_name_2);
  jjs_value_t has_after_delete_result_3 = jjs_object_has (ctx (), object, prop_name_3);
  bool has_after_delete_result_4 = jjs_object_has_internal (ctx (), object, internal_prop_name_1);
  bool has_after_delete_result_5 = jjs_object_has_internal (ctx (), object, internal_prop_name_2);
  bool has_after_delete_result_6 = jjs_object_has_internal (ctx (), object, internal_prop_name_3);

  TEST_ASSERT (jjs_value_is_boolean (ctx (), has_after_delete_result_1) && !jjs_value_is_true (ctx (), has_after_delete_result_1));
  TEST_ASSERT (jjs_value_is_boolean (ctx (), has_after_delete_result_2) && !jjs_value_is_true (ctx (), has_after_delete_result_2));
  TEST_ASSERT (jjs_value_is_boolean (ctx (), has_after_delete_result_3) && !jjs_value_is_true (ctx (), has_after_delete_result_3));
  TEST_ASSERT (has_after_delete_result_4);
  TEST_ASSERT (has_after_delete_result_5);
  TEST_ASSERT (has_after_delete_result_6);

  jjs_value_free (ctx (), has_after_delete_result_1);
  jjs_value_free (ctx (), has_after_delete_result_2);
  jjs_value_free (ctx (), has_after_delete_result_3);

  /* Test the internal [[Delete]] method. */
  bool delete_internal_result_4 = jjs_object_delete_internal (ctx (), object, internal_prop_name_1);
  bool delete_internal_result_5 = jjs_object_delete_internal (ctx (), object, internal_prop_name_2);
  bool delete_internal_result_6 = jjs_object_delete_internal (ctx (), object, internal_prop_name_3);

  TEST_ASSERT (delete_internal_result_4);
  TEST_ASSERT (delete_internal_result_5);
  TEST_ASSERT (delete_internal_result_6);

  bool has_after_internal_delete_result_1 = jjs_object_has_internal (ctx (), object, internal_prop_name_1);
  bool has_after_internal_delete_result_2 = jjs_object_has_internal (ctx (), object, internal_prop_name_2);
  bool has_after_internal_delete_result_3 = jjs_object_has_internal (ctx (), object, internal_prop_name_3);

  TEST_ASSERT (!has_after_internal_delete_result_1);
  TEST_ASSERT (!has_after_internal_delete_result_2);
  TEST_ASSERT (!has_after_internal_delete_result_3);

  /* Cleanup */
  jjs_value_free (ctx (), prop_value_3);
  jjs_value_free (ctx (), prop_value_2);
  jjs_value_free (ctx (), prop_value_1);

  jjs_value_free (ctx (), prop_name_3);
  jjs_value_free (ctx (), prop_name_2);
  jjs_value_free (ctx (), prop_name_1);

  jjs_value_free (ctx (), internal_prop_value_3);
  jjs_value_free (ctx (), internal_prop_value_2);
  jjs_value_free (ctx (), internal_prop_value_1);

  jjs_value_free (ctx (), internal_prop_name_3);
  jjs_value_free (ctx (), internal_prop_name_2);
  jjs_value_free (ctx (), internal_prop_name_1);

  jjs_value_free (ctx (), object);

  ctx_close ();

  return 0;
} /* main */
