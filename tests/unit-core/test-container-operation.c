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
  TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

  if (!jjs_feature_enabled (JJS_FEATURE_MAP) || !jjs_feature_enabled (JJS_FEATURE_SET)
      || !jjs_feature_enabled (JJS_FEATURE_WEAKMAP) || !jjs_feature_enabled (JJS_FEATURE_WEAKSET))
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Containers are disabled!\n");
    jjs_cleanup ();
    return 0;
  }

  // Map container tests
  jjs_value_t map = jjs_container (JJS_CONTAINER_TYPE_MAP, NULL, 0);
  TEST_ASSERT (jjs_container_type (map) == JJS_CONTAINER_TYPE_MAP);

  jjs_value_t key_str = jjs_string_sz ("number");
  jjs_value_t number = jjs_number (10);
  jjs_value_t args[2] = { key_str, number };
  jjs_value_t result = jjs_container_op (JJS_CONTAINER_OP_SET, map, args, 2);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_GET, map, &key_str, 1);
  TEST_ASSERT (jjs_value_as_number (result) == 10);
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_HAS, map, &key_str, 1);
  TEST_ASSERT (jjs_value_is_true (result));
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_SIZE, map, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (result) == 1);
  jjs_value_free (result);

  key_str = jjs_string_sz ("number2");
  number = jjs_number (11);
  jjs_value_t args2[2] = { key_str, number };
  result = jjs_container_op (JJS_CONTAINER_OP_SET, map, args2, 2);
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_SIZE, map, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (result) == 2);
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_DELETE, map, &key_str, 1);
  TEST_ASSERT (jjs_value_is_true (result));
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_SIZE, map, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (result) == 1);
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_CLEAR, map, NULL, 0);
  TEST_ASSERT (jjs_value_is_undefined (result));
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_SIZE, map, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (result) == 0);
  jjs_value_free (result);

  // Set container tests
  number = jjs_number (10);
  jjs_value_t set = jjs_container (JJS_CONTAINER_TYPE_SET, NULL, 0);
  TEST_ASSERT (jjs_container_type (set) == JJS_CONTAINER_TYPE_SET);
  result = jjs_container_op (JJS_CONTAINER_OP_ADD, set, &number, 1);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_HAS, set, &number, 1);
  TEST_ASSERT (jjs_value_is_true (result));
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_SIZE, set, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (result) == 1);
  jjs_value_free (result);

  number = jjs_number (11);
  result = jjs_container_op (JJS_CONTAINER_OP_ADD, set, &number, 1);
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_SIZE, set, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (result) == 2);
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_DELETE, set, &number, 1);
  TEST_ASSERT (jjs_value_is_true (result));
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_SIZE, set, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (result) == 1);
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_CLEAR, set, NULL, 0);
  TEST_ASSERT (jjs_value_is_undefined (result));
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_SIZE, set, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (result) == 0);
  jjs_value_free (result);
  jjs_value_free (set);

  // WeakMap contanier tests
  number = jjs_number (10);
  jjs_value_t weak_map = jjs_container (JJS_CONTAINER_TYPE_WEAKMAP, NULL, 0);
  TEST_ASSERT (jjs_container_type (weak_map) == JJS_CONTAINER_TYPE_WEAKMAP);

  jjs_value_t obj = jjs_object ();
  number = jjs_number (10);
  jjs_value_t args4[2] = { obj, number };
  result = jjs_container_op (JJS_CONTAINER_OP_SET, weak_map, args4, 2);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_HAS, weak_map, &obj, 1);
  TEST_ASSERT (jjs_value_is_true (result));
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_DELETE, weak_map, &obj, 1);
  TEST_ASSERT (jjs_value_is_true (result));
  jjs_value_free (result);
  jjs_value_free (weak_map);

  // WeakSet contanier tests,
  jjs_value_t weak_set = jjs_container (JJS_CONTAINER_TYPE_WEAKSET, NULL, 0);
  TEST_ASSERT (jjs_container_type (weak_set) == JJS_CONTAINER_TYPE_WEAKSET);

  result = jjs_container_op (JJS_CONTAINER_OP_ADD, weak_set, &obj, 1);
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_HAS, weak_set, &obj, 1);
  TEST_ASSERT (jjs_value_is_true (result));
  jjs_value_free (result);

  result = jjs_container_op (JJS_CONTAINER_OP_DELETE, weak_set, &obj, 1);
  TEST_ASSERT (jjs_value_is_true (result));
  jjs_value_free (result);
  jjs_value_free (weak_set);

  // container is not a object
  jjs_value_t empty_val = jjs_undefined ();
  result = jjs_container_op (JJS_CONTAINER_OP_SET, empty_val, args, 2);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  // arguments is a error
  const char *const error_message_p = "Random error.";
  jjs_value_t error_val = jjs_throw_sz (JJS_ERROR_RANGE, error_message_p);
  jjs_value_t args3[2] = { error_val, error_val };
  result = jjs_container_op (JJS_CONTAINER_OP_SET, map, args3, 2);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);
  jjs_value_free (error_val);
  jjs_value_free (map);

  jjs_value_free (key_str);
  jjs_value_free (number);
  jjs_value_free (obj);
  jjs_cleanup ();
  return 0;

} /* main */
