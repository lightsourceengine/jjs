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
assert_boolean_and_release (jjs_value_t result, bool expected)
{
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result));
  TEST_ASSERT (jjs_value_is_true (ctx (), result) == expected);
  jjs_value_free (ctx (), result);
} /* assert_boolean_and_release */

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t object = jjs_object (ctx ());
  jjs_value_t prop_name = jjs_string_sz (ctx (), "something");
  jjs_value_t prop_value = jjs_boolean (ctx (), true);
  jjs_value_t proto_object = jjs_object (ctx ());

  /* Assert that an empty object does not have the property in question */
  assert_boolean_and_release (jjs_object_has (ctx (), object, prop_name), false);
  assert_boolean_and_release (jjs_object_has_own (ctx (), object, prop_name), false);

  assert_boolean_and_release (jjs_object_set_proto (ctx (), object, proto_object, JJS_KEEP), true);

  /* If the object has a prototype, that still means it doesn't have the property */
  assert_boolean_and_release (jjs_object_has (ctx (), object, prop_name), false);
  assert_boolean_and_release (jjs_object_has_own (ctx (), object, prop_name), false);

  assert_boolean_and_release (jjs_object_set (ctx (), proto_object, prop_name, prop_value, JJS_KEEP), true);

  /* After setting the property on the prototype, it must be there, but not on the object */
  assert_boolean_and_release (jjs_object_has (ctx (), object, prop_name), true);
  assert_boolean_and_release (jjs_object_has_own (ctx (), object, prop_name), false);

  TEST_ASSERT (jjs_value_is_true (ctx (), jjs_object_delete (ctx (), proto_object, prop_name)));
  assert_boolean_and_release (jjs_object_set (ctx (), object, prop_name, prop_value, JJS_KEEP), true);

  /* After relocating the property onto the object, it must be there */
  assert_boolean_and_release (jjs_object_has (ctx (), object, prop_name), true);
  assert_boolean_and_release (jjs_object_has_own (ctx (), object, prop_name), true);

  jjs_value_free (ctx (), object);
  jjs_value_free (ctx (), prop_name);
  jjs_value_free (ctx (), prop_value);
  jjs_value_free (ctx (), proto_object);

  ctx_close ();

  return 0;
} /* main */
