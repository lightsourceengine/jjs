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

static jjs_value_t
create_property_descriptor (const char *script_p) /**< source code */
{
  jjs_value_t result = jjs_eval_sz (ctx (), script_p, 0);
  TEST_ASSERT (jjs_value_is_object (ctx (), result));
  return result;
} /* create_property_descriptor */

static void
check_attribute (jjs_value_t attribute, /**< attribute to be checked */
                 jjs_value_t object, /**< original object */
                 const char *name_p) /**< name of the attribute */
{
  jjs_value_t prop_name = jjs_string_sz (ctx (), name_p);
  jjs_value_t value = jjs_object_get (ctx (), object, prop_name);

  if (jjs_value_is_undefined (ctx (), value))
  {
    TEST_ASSERT (jjs_value_is_null (ctx (), attribute));
  }
  else
  {
    jjs_value_t result = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, attribute, JJS_KEEP, value, JJS_KEEP);
    TEST_ASSERT (jjs_value_is_true (ctx (), result));
    jjs_value_free (ctx (), result);
  }

  jjs_value_free (ctx (), value);
  jjs_value_free (ctx (), prop_name);
} /* check_attribute */

static jjs_property_descriptor_t
to_property_descriptor (jjs_value_t object /**< object */)
{
  jjs_property_descriptor_t prop_desc = jjs_property_descriptor ();

  jjs_value_t result = jjs_property_descriptor_from_object (ctx (), object, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result) && jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  return prop_desc;
} /* to_property_descriptor */

int
main (void)
{
  ctx_open (NULL);

  /* Next test. */
  const char *source_p = "({ value:'X', writable:true, enumerable:true, configurable:true })";
  jjs_value_t object = create_property_descriptor (source_p);

  jjs_property_descriptor_t prop_desc = to_property_descriptor (object);

  check_attribute (prop_desc.value, object, "value");

  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_VALUE_DEFINED);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_GET_DEFINED));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_SET_DEFINED));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_WRITABLE_DEFINED);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_WRITABLE);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_ENUMERABLE_DEFINED);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_ENUMERABLE);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_CONFIGURABLE_DEFINED);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_CONFIGURABLE);

  jjs_value_free (ctx (), object);
  jjs_property_descriptor_free (ctx (), &prop_desc);

  /* Next test. */
  source_p = "({ writable:false, configurable:true })";
  object = create_property_descriptor (source_p);

  prop_desc = to_property_descriptor (object);

  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_VALUE_DEFINED));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_GET_DEFINED));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_SET_DEFINED));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_WRITABLE_DEFINED);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_WRITABLE));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_ENUMERABLE_DEFINED));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_CONFIGURABLE_DEFINED);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_CONFIGURABLE);

  jjs_value_free (ctx (), object);
  jjs_property_descriptor_free (ctx (), &prop_desc);

  /* Next test. */
  /* Note: the 'set' property is defined, and it has a value of undefined.
   *       This is different from not having a 'set' property. */
  source_p = "({ get: function() {}, set:undefined, configurable:true })";
  object = create_property_descriptor (source_p);

  prop_desc = to_property_descriptor (object);

  check_attribute (prop_desc.getter, object, "get");
  check_attribute (prop_desc.setter, object, "set");

  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_VALUE_DEFINED));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_WRITABLE_DEFINED));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_GET_DEFINED);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_SET_DEFINED);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_ENUMERABLE_DEFINED));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_CONFIGURABLE_DEFINED);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_CONFIGURABLE);

  jjs_value_free (ctx (), object);
  jjs_property_descriptor_free (ctx (), &prop_desc);

  /* Next test. */
  source_p = "({ get: undefined, enumerable:false })";
  object = create_property_descriptor (source_p);

  prop_desc = to_property_descriptor (object);

  check_attribute (prop_desc.getter, object, "get");

  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_VALUE_DEFINED));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_WRITABLE_DEFINED));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_GET_DEFINED);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_SET_DEFINED));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_ENUMERABLE_DEFINED);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_ENUMERABLE));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_CONFIGURABLE_DEFINED));

  jjs_value_free (ctx (), object);
  jjs_property_descriptor_free (ctx (), &prop_desc);

  /* Next test. */
  source_p = "({ set: function(v) {}, enumerable:true, configurable:false })";
  object = create_property_descriptor (source_p);

  prop_desc = to_property_descriptor (object);

  check_attribute (prop_desc.setter, object, "set");

  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_VALUE_DEFINED));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_WRITABLE_DEFINED));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_GET_DEFINED));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_SET_DEFINED);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_ENUMERABLE_DEFINED);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_ENUMERABLE);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_CONFIGURABLE_DEFINED);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_CONFIGURABLE));

  jjs_value_free (ctx (), object);
  jjs_property_descriptor_free (ctx (), &prop_desc);

  /* Next test. */
  source_p = "({ get: function(v) {}, writable:true })";
  object = create_property_descriptor (source_p);
  jjs_value_t result = jjs_property_descriptor_from_object (ctx (), object, &prop_desc);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);
  jjs_value_free (ctx (), object);

  /* Next test. */
  object = jjs_null (ctx ());
  result = jjs_property_descriptor_from_object (ctx (), object, &prop_desc);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);
  jjs_value_free (ctx (), object);

  ctx_close ();
  return 0;
} /* main */
