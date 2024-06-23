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
  jjs_value_t prop_name = jjs_string_sz (ctx (), "length");
  jjs_value_t value;

  TEST_ASSERT (jjs_object_set (ctx (), object, prop_name, prop_name, JJS_KEEP));
  TEST_ASSERT (jjs_object_has (ctx (), object, prop_name));
  TEST_ASSERT (jjs_object_has_own (ctx (), object, prop_name));

  jjs_property_descriptor_t prop_desc;
  TEST_ASSERT (jjs_object_get_own_prop (ctx (), object, prop_name, &prop_desc));

  jjs_value_t from_object = jjs_property_descriptor_to_object (ctx (), &prop_desc);

  prop_name = jjs_string_sz (ctx (), "value");
  value = jjs_object_get (ctx (), from_object, prop_name);
  TEST_ASSERT (value == prop_desc.value);

  prop_name = jjs_string_sz (ctx (), "writable");
  value = jjs_object_get (ctx (), from_object, prop_name);
  TEST_ASSERT (jjs_value_is_true (ctx (), value) == ((prop_desc.flags & JJS_PROP_IS_WRITABLE) != 0));

  prop_name = jjs_string_sz (ctx (), "enumerable");
  value = jjs_object_get (ctx (), from_object, prop_name);
  TEST_ASSERT (jjs_value_is_true (ctx (), value) == ((prop_desc.flags & JJS_PROP_IS_ENUMERABLE) != 0));

  prop_name = jjs_string_sz (ctx (), "configurable");
  value = jjs_object_get (ctx (), from_object, prop_name);
  TEST_ASSERT (jjs_value_is_true (ctx (), value) == ((prop_desc.flags & JJS_PROP_IS_CONFIGURABLE) != 0));

  jjs_value_free (ctx (), object);
  jjs_value_free (ctx (), prop_name);
  jjs_value_free (ctx (), value);
  jjs_value_free (ctx (), from_object);
  jjs_property_descriptor_free (ctx (), &prop_desc);

  prop_desc.flags = JJS_PROP_IS_CONFIGURABLE;
  from_object = jjs_property_descriptor_to_object (ctx (), &prop_desc);
  TEST_ASSERT (jjs_value_is_exception (ctx (), from_object));
  jjs_value_free (ctx (), from_object);

  prop_desc.flags = JJS_PROP_IS_ENUMERABLE;
  from_object = jjs_property_descriptor_to_object (ctx (), &prop_desc);
  TEST_ASSERT (jjs_value_is_exception (ctx (), from_object));
  jjs_value_free (ctx (), from_object);

  prop_desc.flags = JJS_PROP_IS_WRITABLE;
  from_object = jjs_property_descriptor_to_object (ctx (), &prop_desc);
  TEST_ASSERT (jjs_value_is_exception (ctx (), from_object));
  jjs_value_free (ctx (), from_object);

  ctx_close ();
  return 0;
} /* main */
