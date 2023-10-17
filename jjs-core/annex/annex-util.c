/* Copyright Light Source Software, LLC and other contributors.
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

#include "annex.h"

#include "ecma-function-object.h"

#define JJS_CONFIGURABLE_ENUMERABLE_WRITABLE_VALUE (JJS_PROP_IS_CONFIGURABLE_DEFINED \
                                                    | JJS_PROP_IS_ENUMERABLE_DEFINED \
                                                    | JJS_PROP_IS_WRITABLE_DEFINED \
                                                    | JJS_PROP_IS_CONFIGURABLE \
                                                    | JJS_PROP_IS_ENUMERABLE \
                                                    | JJS_PROP_IS_WRITABLE \
                                                    | JJS_PROP_IS_VALUE_DEFINED)

/**
 * Define a function property on an object.
 *
 * @param object_p target object
 * @param name_id magic string id of the property name
 * @param handler_p native function
 */
void
annex_util_define_function (ecma_object_t* object_p,
                            lit_magic_string_id_t name_id,
                            ecma_native_handler_t handler_p)
{
  ecma_value_t fn = ecma_make_object_value (ecma_op_create_external_function_object (handler_p));

  annex_util_define_value (object_p, name_id, fn);

  ecma_free_value (fn);
} /* annex_util_define_function */

/**
 * Define a value property on an object.
 *
 * @param object_p target object
 * @param name_id magic string id of the property name
 * @param value property value
 */
void
annex_util_define_value (ecma_object_t* object_p,
                         lit_magic_string_id_t name_id,
                         ecma_value_t value)
{
  ecma_property_descriptor_t prop_desc = {
    .flags = JJS_CONFIGURABLE_ENUMERABLE_WRITABLE_VALUE,
    .value = value,
  };

  ecma_value_t result = ecma_op_object_define_own_property (
    object_p,
    ecma_get_magic_string (name_id),
    &prop_desc);

  ecma_free_value (result);
} /* annex_util_define_value */

/**
 * Set a property on an object with a magic string as the key.
 *
 * @param object target object
 * @param name_id property name
 * @param value property value
 */
void
ecma_set_m (ecma_value_t object, lit_magic_string_id_t name_id, ecma_value_t value)
{
  ecma_value_t result = ecma_op_object_put (ecma_get_object_from_value (object),
                                            ecma_get_magic_string (name_id),
                                            value,
                                            false);
  ecma_free_value (result);
} /* ecma_set_m */

/**
 * Set a property on an object with a string as the key.
 *
 * @param object target object
 * @param key property name
 * @param value property value
 */
void
ecma_set_v (ecma_value_t object, ecma_value_t key, ecma_value_t value)
{
  JJS_ASSERT (ecma_is_value_string (key));

  if (ecma_is_value_string (key))
  {
    ecma_value_t result = ecma_op_object_put (ecma_get_object_from_value (object),
                                              ecma_get_prop_name_from_value (key),
                                              value,
                                              false);
    ecma_free_value (result);
  }
} /* ecma_set_v */

/**
 * Create an ecma string value from a null-terminated ascii string.
 *
 * @param string_p c-string value
 * @return ecma string value
 */
ecma_value_t
ecma_string_ascii_sz (const char* string_p)
{
  const lit_utf8_size_t size = (lit_utf8_size_t)strlen (string_p);
  ecma_string_t* ecma_string_p = ecma_new_ecma_string_from_ascii ((const lit_utf8_byte_t*)string_p, size);

  return ecma_make_string_value (ecma_string_p);
} /* ecma_string_sz */

/**
 * Convert an ecma string value to a null-terminated c-string.
 *
 * @param value ecma string value
 * @return ecma_cstr_t containing the converted string and size. if the conversion
 *         fails, the string will be empty. caller must use ecma_string_cstr_free
 *         to cleanup the return value.
 */
ecma_cstr_t
ecma_string_to_cstr (ecma_value_t value)
{
  ecma_cstr_t cstr = { "", 0 };

  if (ecma_is_value_string (value))
  {
    ecma_string_t* string_p = ecma_get_string_from_value (value);
    lit_utf8_size_t size = ecma_string_get_size (string_p);
    lit_utf8_byte_t* buffer_p = (lit_utf8_byte_t*)jmem_heap_alloc_block_null_on_error (size + 1);

    if (buffer_p == NULL)
    {
      return cstr;
    }

    lit_utf8_size_t written = ecma_string_copy_to_buffer (string_p,
                                                          buffer_p,
                                                          size,
                                                          JJS_ENCODING_UTF8);

    if (written != size)
    {
      jmem_heap_free_block (buffer_p, size + 1);
    }
    else
    {
      buffer_p[size] = '\0';
      cstr.str_p = (char*)buffer_p;
      cstr.size = size;
    }
  }

  return cstr;
} /* ecma_string_to_cstr */

/**
 * Free a string converted with ecma_string_to_cstr.
 *
 * @param cstr_p pointer to cstr struct
 */
void
ecma_free_cstr (ecma_cstr_t* cstr_p)
{
  if (cstr_p->size > 0)
  {
    jmem_heap_free_block (cstr_p->str_p, cstr_p->size + 1);
  }
} /* ecma_free_cstr */

/**
 * Find own property on an object with a magic string as the key.
 *
 * @param object target object
 * @param key magic string id of the property name
 * @return value if found; otherwise, ECMA_VALUE_NOT_FOUND
 */
ecma_value_t
ecma_find_own_m (ecma_value_t object, lit_magic_string_id_t key)
{
  if (!ecma_is_value_object (object))
  {
    return ECMA_VALUE_NOT_FOUND;
  }

  return ecma_op_object_find_own (object,
                                  ecma_get_object_from_value (object),
                                  ecma_get_magic_string (key));
} /* ecma_find_own_m */

/**
 * Find own property on an object with a string as the key.
 *
 * @param object target object
 * @param key property name
 * @return value if found; otherwise, ECMA_VALUE_NOT_FOUND
 */
ecma_value_t
ecma_find_own_v (ecma_value_t object, ecma_value_t key)
{
  if (!ecma_is_value_object (object) || !ecma_is_value_string (key))
  {
    return ECMA_VALUE_NOT_FOUND;
  }

  return ecma_op_object_find_own (object,
                                  ecma_get_object_from_value (object),
                                  ecma_get_string_from_value (key));
} /* ecma_find_own_v */

