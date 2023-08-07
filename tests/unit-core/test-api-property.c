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
  TEST_INIT ();

  jjs_init (JJS_INIT_EMPTY);

  /* Test: init property descriptor */
  jjs_property_descriptor_t prop_desc = jjs_property_descriptor ();
  TEST_ASSERT (prop_desc.flags == JJS_PROP_NO_OPTS);
  TEST_ASSERT (jjs_value_is_undefined (prop_desc.value));
  TEST_ASSERT (jjs_value_is_undefined (prop_desc.getter));
  TEST_ASSERT (jjs_value_is_undefined (prop_desc.setter));

  /* Test: define own properties */
  jjs_value_t global_obj_val = jjs_current_realm ();
  jjs_value_t prop_name = jjs_string_sz ("my_defined_property");
  prop_desc.flags |= JJS_PROP_IS_VALUE_DEFINED;
  prop_desc.value = jjs_value_copy (prop_name);
  jjs_value_t res = jjs_object_define_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (res) && jjs_value_is_true (res));
  jjs_value_free (res);
  jjs_property_descriptor_free (&prop_desc);

  /* Test: define own property with error */
  prop_desc = jjs_property_descriptor ();
  prop_desc.flags |= JJS_PROP_IS_VALUE_DEFINED | JJS_PROP_SHOULD_THROW;
  prop_desc.value = jjs_number (3.14);
  res = jjs_object_define_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_exception (res));
  jjs_value_free (res);
  jjs_property_descriptor_free (&prop_desc);

  /* Test: test define own property failure without throw twice */
  prop_desc = jjs_property_descriptor ();
  prop_desc.flags |= JJS_PROP_IS_VALUE_DEFINED | JJS_PROP_IS_GET_DEFINED;
  res = jjs_object_define_own_prop (prop_name, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (res) && !jjs_value_is_true (res));
  jjs_value_free (res);
  res = jjs_object_define_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (res) && !jjs_value_is_true (res));
  jjs_value_free (res);
  jjs_property_descriptor_free (&prop_desc);

  /* Test: get own property descriptor */
  prop_desc = jjs_property_descriptor ();
  jjs_value_t is_ok = jjs_object_get_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (is_ok) && jjs_value_is_true (is_ok));
  jjs_value_free (is_ok);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_VALUE_DEFINED);
  TEST_ASSERT (jjs_value_is_string (prop_desc.value));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_WRITABLE));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_ENUMERABLE));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_CONFIGURABLE));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_GET_DEFINED));
  TEST_ASSERT (jjs_value_is_undefined (prop_desc.getter));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_SET_DEFINED));
  TEST_ASSERT (jjs_value_is_undefined (prop_desc.setter));
  jjs_property_descriptor_free (&prop_desc);

  if (jjs_feature_enabled (JJS_FEATURE_PROXY))
  {
    /* Note: update this test when the internal method is implemented */
    jjs_value_t target = jjs_object ();
    jjs_value_t handler = jjs_object ();
    jjs_value_t proxy = jjs_proxy (target, handler);

    jjs_value_free (target);
    jjs_value_free (handler);
    is_ok = jjs_object_get_own_prop (proxy, prop_name, &prop_desc);
    TEST_ASSERT (jjs_value_is_boolean (is_ok) && !jjs_value_is_true (is_ok));
    jjs_value_free (is_ok);
    jjs_value_free (proxy);
  }

  jjs_value_free (prop_name);

  /* Test: define and get own property descriptor */
  prop_desc.flags |= JJS_PROP_IS_ENUMERABLE;
  prop_name = jjs_string_sz ("enumerable-property");
  res = jjs_object_define_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_boolean (res));
  TEST_ASSERT (jjs_value_is_true (res));
  jjs_value_free (res);
  jjs_property_descriptor_free (&prop_desc);
  is_ok = jjs_object_get_own_prop (global_obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (is_ok) && jjs_value_is_true (is_ok));
  jjs_value_free (is_ok);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_WRITABLE));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_ENUMERABLE);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_CONFIGURABLE));

  jjs_value_free (prop_name);
  jjs_value_free (global_obj_val);

  /* Test: define own property descriptor error */
  prop_desc = jjs_property_descriptor ();
  prop_desc.flags |= JJS_PROP_IS_VALUE_DEFINED;
  prop_desc.value = jjs_number (11);

  jjs_value_t obj_val = jjs_object ();
  prop_name = jjs_string_sz ("property_key");
  res = jjs_object_define_own_prop (obj_val, prop_name, &prop_desc);
  TEST_ASSERT (!jjs_value_is_exception (res));
  jjs_value_free (res);

  jjs_value_free (prop_desc.value);
  prop_desc.value = jjs_number (22);
  res = jjs_object_define_own_prop (obj_val, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_exception (res));
  jjs_value_free (res);

  jjs_value_free (prop_name);
  jjs_value_free (obj_val);

  jjs_cleanup ();

  return 0;
} /* main */
