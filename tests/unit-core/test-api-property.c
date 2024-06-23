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

  /* Test: init property descriptor */
  jjs_property_descriptor_t prop_desc = jjs_property_descriptor ();
  TEST_ASSERT (prop_desc.flags == JJS_PROP_NO_OPTS);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), prop_desc.value));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), prop_desc.getter));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), prop_desc.setter));

  /* Test: define own properties */
  jjs_value_t global_obj_val = jjs_current_realm (ctx ());
  jjs_value_t prop_name = jjs_string_sz (ctx (), "my_defined_property");
  prop_desc.flags |= JJS_PROP_IS_VALUE_DEFINED;
  prop_desc.value = jjs_value_copy (ctx (), prop_name);
  jjs_value_t res = jjs_object_define_own_prop (ctx (), global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), res) && jjs_value_is_true (ctx (), res));
  jjs_value_free (ctx (), res);
  jjs_property_descriptor_free (ctx (), &prop_desc);

  /* Test: define own property with error */
  prop_desc = jjs_property_descriptor ();
  prop_desc.flags |= JJS_PROP_IS_VALUE_DEFINED | JJS_PROP_SHOULD_THROW;
  prop_desc.value = jjs_number (ctx (), 3.14);
  res = jjs_object_define_own_prop (ctx (), global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_exception (ctx (), res));
  jjs_value_free (ctx (), res);
  jjs_property_descriptor_free (ctx (), &prop_desc);

  /* Test: test define own property failure without throw twice */
  prop_desc = jjs_property_descriptor ();
  prop_desc.flags |= JJS_PROP_IS_VALUE_DEFINED | JJS_PROP_IS_GET_DEFINED;
  res = jjs_object_define_own_prop (ctx (), prop_name, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), res) && !jjs_value_is_true (ctx (), res));
  jjs_value_free (ctx (), res);
  res = jjs_object_define_own_prop (ctx (), global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), res) && !jjs_value_is_true (ctx (), res));
  jjs_value_free (ctx (), res);
  jjs_property_descriptor_free (ctx (), &prop_desc);

  /* Test: get own property descriptor */
  prop_desc = jjs_property_descriptor ();
  jjs_value_t is_ok = jjs_object_get_own_prop (ctx (), global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), is_ok) && jjs_value_is_true (ctx (), is_ok));
  jjs_value_free (ctx (), is_ok);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_VALUE_DEFINED);
  TEST_ASSERT (jjs_value_is_string (ctx (), prop_desc.value));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_WRITABLE));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_ENUMERABLE));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_CONFIGURABLE));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_GET_DEFINED));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), prop_desc.getter));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_SET_DEFINED));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), prop_desc.setter));
  jjs_property_descriptor_free (ctx (), &prop_desc);

  if (jjs_feature_enabled (JJS_FEATURE_PROXY))
  {
    /* Note: update this test when the internal method is implemented */
    jjs_value_t proxy = jjs_proxy (ctx (), jjs_object (ctx ()), JJS_MOVE, jjs_object (ctx ()), JJS_MOVE);

    is_ok = jjs_object_get_own_prop (ctx (), proxy, prop_name, &prop_desc);
    TEST_ASSERT (jjs_value_is_boolean (ctx (), is_ok) && !jjs_value_is_true (ctx (), is_ok));
    jjs_value_free (ctx (), is_ok);
    jjs_value_free (ctx (), proxy);
  }

  jjs_value_free (ctx (), prop_name);

  /* Test: define and get own property descriptor */
  prop_desc.flags |= JJS_PROP_IS_ENUMERABLE;
  prop_name = jjs_string_sz (ctx (), "enumerable-property");
  res = jjs_object_define_own_prop (ctx (), global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), res));
  TEST_ASSERT (jjs_value_is_boolean (ctx (), res));
  TEST_ASSERT (jjs_value_is_true (ctx (), res));
  jjs_value_free (ctx (), res);
  jjs_property_descriptor_free (ctx (), &prop_desc);
  is_ok = jjs_object_get_own_prop (ctx (), global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), is_ok) && jjs_value_is_true (ctx (), is_ok));
  jjs_value_free (ctx (), is_ok);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_WRITABLE));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_ENUMERABLE);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_CONFIGURABLE));

  jjs_value_free (ctx (), prop_name);
  jjs_value_free (ctx (), global_obj_val);

  /* Test: define own property descriptor error */
  prop_desc = jjs_property_descriptor ();
  prop_desc.flags |= JJS_PROP_IS_VALUE_DEFINED;
  prop_desc.value = jjs_number (ctx (), 11);

  jjs_value_t obj_val = jjs_object (ctx ());
  prop_name = jjs_string_sz (ctx (), "property_key");
  res = jjs_object_define_own_prop (ctx (), obj_val, prop_name, &prop_desc);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), res));
  jjs_value_free (ctx (), res);

  jjs_value_free (ctx (), prop_desc.value);
  prop_desc.value = jjs_number (ctx (), 22);
  res = jjs_object_define_own_prop (ctx (), obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_exception (ctx (), res));
  jjs_value_free (ctx (), res);

  jjs_value_free (ctx (), prop_name);
  jjs_value_free (ctx (), obj_val);

  ctx_close ();

  return 0;
} /* main */
