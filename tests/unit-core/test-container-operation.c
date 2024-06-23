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
  if (!jjs_feature_enabled (JJS_FEATURE_MAP) || !jjs_feature_enabled (JJS_FEATURE_SET)
      || !jjs_feature_enabled (JJS_FEATURE_WEAKMAP) || !jjs_feature_enabled (JJS_FEATURE_WEAKSET))
  {
    jjs_log (ctx (), JJS_LOG_LEVEL_ERROR, "Containers are disabled!\n");
    return 0;
  }

  ctx_open (NULL);

  // Map container tests
  jjs_value_t map = jjs_container_noargs (ctx (), JJS_CONTAINER_TYPE_MAP);
  TEST_ASSERT (jjs_container_type (ctx (), map) == JJS_CONTAINER_TYPE_MAP);

  jjs_value_t key_str = jjs_string_sz (ctx (), "number");
  jjs_value_t number = jjs_number (ctx (), 10);
  jjs_value_t args[2] = { key_str, number };
  jjs_value_t result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SET, map, args, 2);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_GET, map, &key_str, 1);
  TEST_ASSERT (jjs_value_as_number (ctx (), result) == 10);
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_HAS, map, &key_str, 1);
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SIZE, map, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (ctx (), result) == 1);
  jjs_value_free (ctx (), result);

  key_str = jjs_string_sz (ctx (), "number2");
  number = jjs_number (ctx (), 11);
  jjs_value_t args2[2] = { key_str, number };
  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SET, map, args2, 2);
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SIZE, map, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (ctx (), result) == 2);
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_DELETE, map, &key_str, 1);
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SIZE, map, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (ctx (), result) == 1);
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_CLEAR, map, NULL, 0);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SIZE, map, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (ctx (), result) == 0);
  jjs_value_free (ctx (), result);

  // Set container tests
  number = jjs_number (ctx (), 10);
  jjs_value_t set = jjs_container_noargs (ctx (), JJS_CONTAINER_TYPE_SET);
  TEST_ASSERT (jjs_container_type (ctx (), set) == JJS_CONTAINER_TYPE_SET);
  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_ADD, set, &number, 1);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_HAS, set, &number, 1);
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SIZE, set, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (ctx (), result) == 1);
  jjs_value_free (ctx (), result);

  number = jjs_number (ctx (), 11);
  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_ADD, set, &number, 1);
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SIZE, set, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (ctx (), result) == 2);
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_DELETE, set, &number, 1);
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SIZE, set, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (ctx (), result) == 1);
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_CLEAR, set, NULL, 0);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SIZE, set, NULL, 0);
  TEST_ASSERT (jjs_value_as_number (ctx (), result) == 0);
  jjs_value_free (ctx (), result);
  jjs_value_free (ctx (), set);

  // WeakMap contanier tests
  number = jjs_number (ctx (), 10);
  jjs_value_t weak_map = jjs_container_noargs (ctx (), JJS_CONTAINER_TYPE_WEAKMAP);
  TEST_ASSERT (jjs_container_type (ctx (), weak_map) == JJS_CONTAINER_TYPE_WEAKMAP);

  jjs_value_t obj = jjs_object (ctx ());
  number = jjs_number (ctx (), 10);
  jjs_value_t args4[2] = { obj, number };
  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SET, weak_map, args4, 2);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_HAS, weak_map, &obj, 1);
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_DELETE, weak_map, &obj, 1);
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);
  jjs_value_free (ctx (), weak_map);

  // WeakSet contanier tests,
  jjs_value_t weak_set = jjs_container_noargs (ctx (), JJS_CONTAINER_TYPE_WEAKSET);
  TEST_ASSERT (jjs_container_type (ctx (), weak_set) == JJS_CONTAINER_TYPE_WEAKSET);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_ADD, weak_set, &obj, 1);
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_HAS, weak_set, &obj, 1);
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_DELETE, weak_set, &obj, 1);
  TEST_ASSERT (jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);
  jjs_value_free (ctx (), weak_set);

  // container is not a object
  jjs_value_t empty_val = jjs_undefined (ctx ());
  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SET, empty_val, args, 2);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  // arguments is a error
  const char *const error_message_p = "Random error.";
  jjs_value_t error_val = jjs_throw_sz (ctx (), JJS_ERROR_RANGE, error_message_p);
  jjs_value_t args3[2] = { error_val, error_val };
  result = jjs_container_op (ctx (), JJS_CONTAINER_OP_SET, map, args3, 2);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);
  jjs_value_free (ctx (), error_val);
  jjs_value_free (ctx (), map);

  jjs_value_free (ctx (), key_str);
  jjs_value_free (ctx (), number);
  jjs_value_free (ctx (), obj);
  ctx_close ();
  return 0;

} /* main */
