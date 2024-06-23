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

static const char *prop_names[] = { "val1", "val2", "val3", "val4", "val5", "37", "symbol" };

static jjs_char_t buffer[256] = { 0 };

static void
create_and_set_property (const jjs_value_t object, const char *prop_name)
{
  jjs_value_free (ctx (), jjs_object_set_sz (ctx (), object, prop_name, jjs_undefined (ctx ()), JJS_MOVE));
} /* create_and_set_property */

static void
compare_prop_name (const jjs_value_t object, const char *prop_name, uint32_t idx)
{
  jjs_value_t name = jjs_object_get_index (ctx (), object, idx);
  TEST_ASSERT (jjs_value_is_string (ctx (), name) || jjs_value_is_number (ctx (), name));
  if (jjs_value_is_string (ctx (), name))
  {
    jjs_size_t name_size = jjs_string_size (ctx (), name, JJS_ENCODING_CESU8);
    TEST_ASSERT (name_size < sizeof (buffer));
    jjs_size_t ret_size = jjs_string_to_buffer (ctx (), name, JJS_ENCODING_CESU8, buffer, sizeof (buffer));
    TEST_ASSERT (name_size == ret_size);
    buffer[name_size] = '\0';
    TEST_ASSERT (strcmp ((const char *) buffer, prop_name) == 0);
  }
  else
  {
    TEST_ASSERT ((int) jjs_value_as_number (ctx (), name) == atoi (prop_name));
  }

  jjs_value_free (ctx (), name);
} /* compare_prop_name */

static void
define_property (const jjs_value_t object,
                 const char *prop_name,
                 jjs_property_descriptor_t *prop_desc_p,
                 bool is_symbol)
{
  jjs_value_t jname = jjs_string_sz (ctx (), prop_name);
  jjs_value_t ret_val;
  if (is_symbol)
  {
    jjs_value_t symbol = jjs_symbol_with_description (ctx (), jname, JJS_KEEP);
    ret_val = jjs_object_define_own_prop (ctx (), object, symbol, prop_desc_p);
    jjs_value_free (ctx (), symbol);
  }
  else
  {
    ret_val = jjs_object_define_own_prop (ctx (), object, jname, prop_desc_p);
  }

  jjs_value_free (ctx (), jname);
  jjs_value_free (ctx (), ret_val);
} /* define_property */

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t error_value = jjs_object_property_names (ctx (), jjs_undefined (ctx ()), JJS_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jjs_value_is_exception (ctx (), error_value) && jjs_error_type (ctx (), error_value) == JJS_ERROR_TYPE);
  jjs_value_free (ctx (), error_value);

  jjs_value_t test_object = jjs_object (ctx ());
  create_and_set_property (test_object, prop_names[0]);
  create_and_set_property (test_object, prop_names[1]);

  jjs_value_t names;

  jjs_property_descriptor_t prop_desc = jjs_property_descriptor ();
  prop_desc.flags |= (JJS_PROP_IS_CONFIGURABLE_DEFINED | JJS_PROP_IS_CONFIGURABLE | JJS_PROP_IS_WRITABLE_DEFINED
                      | JJS_PROP_IS_WRITABLE | JJS_PROP_IS_ENUMERABLE_DEFINED);

  // Test enumerable - non-enumerable filter
  define_property (test_object, prop_names[2], &prop_desc, false);
  names =
    jjs_object_property_names (ctx (), test_object, JJS_PROPERTY_FILTER_ALL | JJS_PROPERTY_FILTER_EXCLUDE_NON_ENUMERABLE);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 2);
  jjs_value_free (ctx (), names);
  names = jjs_object_property_names (ctx (), test_object, JJS_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 3);
  compare_prop_name (names, prop_names[2], 2);
  jjs_value_free (ctx (), names);
  prop_desc.flags |= JJS_PROP_IS_ENUMERABLE;

  // Test configurable - non-configurable filter
  prop_desc.flags &= (uint16_t) ~JJS_PROP_IS_CONFIGURABLE;
  define_property (test_object, prop_names[3], &prop_desc, false);
  names = jjs_object_property_names (ctx (), test_object,
                                       JJS_PROPERTY_FILTER_ALL | JJS_PROPERTY_FILTER_EXCLUDE_NON_CONFIGURABLE);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 3);
  jjs_value_free (ctx (), names);
  names = jjs_object_property_names (ctx (), test_object, JJS_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 4);
  compare_prop_name (names, prop_names[3], 3);
  jjs_value_free (ctx (), names);
  prop_desc.flags |= JJS_PROP_IS_CONFIGURABLE;

  // Test writable - non-writable filter
  prop_desc.flags &= (uint16_t) ~JJS_PROP_IS_WRITABLE;
  define_property (test_object, prop_names[4], &prop_desc, false);
  names =
    jjs_object_property_names (ctx (), test_object, JJS_PROPERTY_FILTER_ALL | JJS_PROPERTY_FILTER_EXCLUDE_NON_WRITABLE);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 4);
  jjs_value_free (ctx (), names);
  names = jjs_object_property_names (ctx (), test_object, JJS_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 5);
  compare_prop_name (names, prop_names[4], 4);
  jjs_value_free (ctx (), names);
  prop_desc.flags |= JJS_PROP_IS_WRITABLE;

  // Test all property filter
  names = jjs_object_property_names (ctx (), test_object, JJS_PROPERTY_FILTER_ALL);
  jjs_length_t array_len = jjs_array_length (ctx (), names);
  TEST_ASSERT (array_len == (uint32_t) 5);

  for (uint32_t i = 0; i < array_len; i++)
  {
    compare_prop_name (names, prop_names[i], i);
  }

  jjs_value_free (ctx (), names);

  // Test number and string index exclusion
  define_property (test_object, prop_names[5], &prop_desc, false);
  names = jjs_object_property_names (ctx (), test_object,
                                       JJS_PROPERTY_FILTER_ALL | JJS_PROPERTY_FILTER_EXCLUDE_STRINGS
                                         | JJS_PROPERTY_FILTER_INTEGER_INDICES_AS_NUMBER);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 1);
  compare_prop_name (names, prop_names[5], 0);
  jjs_value_free (ctx (), names);
  names = jjs_object_property_names (ctx (), test_object,
                                       JJS_PROPERTY_FILTER_ALL | JJS_PROPERTY_FILTER_EXCLUDE_INTEGER_INDICES);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 5);
  jjs_value_free (ctx (), names);

  // Test prototype chain traversion
  names = jjs_object_property_names (ctx (), test_object, JJS_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 6);
  jjs_value_free (ctx (), names);
  names = jjs_object_property_names (ctx (), test_object,
                                       JJS_PROPERTY_FILTER_ALL | JJS_PROPERTY_FILTER_TRAVERSE_PROTOTYPE_CHAIN);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 18);
  jjs_value_free (ctx (), names);

  // Test symbol exclusion
  define_property (test_object, prop_names[6], &prop_desc, true);
  names = jjs_object_property_names (ctx (), test_object, JJS_PROPERTY_FILTER_ALL | JJS_PROPERTY_FILTER_EXCLUDE_SYMBOLS);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 6);
  jjs_value_free (ctx (), names);
  names = jjs_object_property_names (ctx (), test_object, JJS_PROPERTY_FILTER_ALL);
  TEST_ASSERT (jjs_array_length (ctx (), names) == (uint32_t) 7);
  jjs_value_free (ctx (), names);

  jjs_property_descriptor_free (ctx (), &prop_desc);
  jjs_value_free (ctx (), test_object);
  
  ctx_close ();
  
  return 0;
} /* main */
