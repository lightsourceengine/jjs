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

#include <ctype.h>

#include "ecma-function-object.h"

#include "annex.h"
#include "jjs-util.h"

#define JJS_CONFIGURABLE_ENUMERABLE_WRITABLE_VALUE                                                        \
  (JJS_PROP_IS_CONFIGURABLE_DEFINED | JJS_PROP_IS_ENUMERABLE_DEFINED | JJS_PROP_IS_WRITABLE_DEFINED       \
   | JJS_PROP_IS_CONFIGURABLE | JJS_PROP_IS_ENUMERABLE | JJS_PROP_IS_WRITABLE | JJS_PROP_IS_VALUE_DEFINED)

#define JJS_CONFIGURABLE_ENUMERABLE_READONLY_VALUE                                  \
  (JJS_PROP_IS_CONFIGURABLE_DEFINED | JJS_PROP_IS_ENUMERABLE_DEFINED                \
   | JJS_PROP_IS_CONFIGURABLE | JJS_PROP_IS_ENUMERABLE | JJS_PROP_IS_VALUE_DEFINED)

#define NPM_PACKAGE_NAME_LENGTH_LIMIT 214

/**
 * Define a function property on an object.
 *
 * @param context_p JJS context
 * @param object_p target object
 * @param name_id magic string id of the property name
 * @param handler_p native function
 */
void
annex_util_define_function (jjs_context_t* context_p,
                            ecma_object_t* object_p,
                            lit_magic_string_id_t name_id,
                            ecma_native_handler_t handler_p)
{
  ecma_value_t fn = ecma_make_object_value (context_p, ecma_op_create_external_function_object (context_p, handler_p));

  annex_util_define_value (context_p, object_p, name_id, fn, JJS_MOVE);
} /* annex_util_define_function */

/**
 * Define a value property on an object.
 *
 * @param context_p JJS context
 * @param object_p target object
 * @param name_id magic string id of the property name
 * @param value property value
 * @param value_o value reference ownership
 */
void
annex_util_define_value (jjs_context_t* context_p,
                         ecma_object_t* object_p,
                         lit_magic_string_id_t name_id,
                         ecma_value_t value,
                         jjs_own_t value_o)
{
  ecma_property_descriptor_t prop_desc = {
    .flags = JJS_CONFIGURABLE_ENUMERABLE_WRITABLE_VALUE,
    .value = value,
  };

  ecma_value_t result = ecma_op_object_define_own_property (
    context_p,
    object_p,
    ecma_get_magic_string (name_id),
    &prop_desc);

  ecma_free_value (context_p, result);

  jjs_disown_value (context_p, value, value_o);
} /* annex_util_define_value */

/**
 * Define a readonly value property on an object.
 *
 * @param context_p JJS context
 * @param object_p target object
 * @param name_id magic string id of the property name
 * @param value property value
 * @param value_o value reference ownership
 */
void
annex_util_define_ro_value (jjs_context_t* context_p,
                            ecma_object_t* object_p,
                            lit_magic_string_id_t name_id,
                            ecma_value_t value,
                            jjs_own_t value_o)
{
  ecma_property_descriptor_t prop_desc = {
    .flags = JJS_CONFIGURABLE_ENUMERABLE_READONLY_VALUE,
    .value = value,
  };

  ecma_value_t result = ecma_op_object_define_own_property (
    context_p,
    object_p,
    ecma_get_magic_string (name_id),
    &prop_desc);

  ecma_free_value (context_p, result);

  jjs_disown_value (context_p, value, value_o);
} /* annex_util_define_value_ro */

/**
 * Set a property on an object with a magic string as the key.
 *
 * @param object target object
 * @param name_id property name
 * @param value property value
 */
void
ecma_set_m (ecma_context_t* context_p, ecma_value_t object, lit_magic_string_id_t name_id, ecma_value_t value)
{
  ecma_value_t result = ecma_op_object_put (context_p,
                                            ecma_get_object_from_value (context_p, object),
                                            ecma_get_magic_string (name_id),
                                            value,
                                            false);
  ecma_free_value (context_p, result);
} /* ecma_set_m */

/**
 * Set a property on an object with a string as the key.
 *
 * @param object target object
 * @param key property name
 * @param value property value
 */
void
ecma_set_v (ecma_value_t object, ecma_context_t* context_p, ecma_value_t key, ecma_value_t value)
{
  JJS_ASSERT (ecma_is_value_string (key));

  if (ecma_is_value_string (key))
  {
    ecma_value_t result =
      ecma_op_object_put (context_p,
                          ecma_get_object_from_value (context_p, object),
                          ecma_get_prop_name_from_value (context_p, key),
                          value,
                          false);
    ecma_free_value (context_p, result);
  }
} /* ecma_set_v */

/**
 * Set a property on an object with an index as the key.
 *
 * @param object target object
 * @param index integer index
 * @param value value to set
 */
void
ecma_set_index_v (ecma_context_t* context_p, ecma_value_t object, ecma_length_t index, ecma_value_t value)
{
  // note: index is converted to string and then converted back to index. need a more efficient set index
  ecma_free_value (context_p, ecma_op_object_put_by_index (context_p, ecma_get_object_from_value (context_p, object), index, value, false));
}

/**
 * Create an ecma string value from a null-terminated ascii string.
 *
 * @param string_p c-string value
 * @return ecma string value
 */
ecma_value_t
ecma_string_ascii_sz (ecma_context_t* context_p, const char* string_p)
{
  const lit_utf8_size_t size = (lit_utf8_size_t)strlen (string_p);
  ecma_string_t* ecma_string_p = ecma_new_ecma_string_from_ascii (context_p, (const lit_utf8_byte_t*)string_p, size);

  return ecma_make_string_value (context_p, ecma_string_p);
} /* ecma_string_sz */

/**
 * Find own property on an object with a magic string as the key.
 *
 * @param object target object
 * @param key magic string id of the property name
 * @return value if found; otherwise, ECMA_VALUE_NOT_FOUND
 */
ecma_value_t
ecma_find_own_m (ecma_context_t* context_p, ecma_value_t object, lit_magic_string_id_t key)
{
  if (!ecma_is_value_object (object))
  {
    return ECMA_VALUE_NOT_FOUND;
  }

  return ecma_op_object_find_own (context_p,
                                  object,
                                  ecma_get_object_from_value (context_p, object),
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
ecma_find_own_v (ecma_context_t* context_p, ecma_value_t object, ecma_value_t key)
{
  if (!ecma_is_value_object (object) || !ecma_is_value_string (key))
  {
    return ECMA_VALUE_NOT_FOUND;
  }

  return ecma_op_object_find_own (context_p,
                                  object,
                                  ecma_get_object_from_value (context_p, object),
                                  ecma_get_string_from_value (context_p, key));
} /* ecma_find_own_v */

/**
 * Checks if object has own property.
 *
 * @param object target object
 * @param key magic string id of the property name
 * @return true if key exists, false otherwise
 */
bool
ecma_has_own_m (ecma_context_t* context_p, ecma_value_t object, lit_magic_string_id_t key)
{
  ecma_value_t value = ecma_find_own_m (context_p, object, key);

  ecma_free_value (context_p, value);

  return ecma_is_value_found (value);
} /* ecma_has_own_m */

/**
 * Checks if object has own property.
 *
 * @param object target object
 * @param key property name
 * @return true if key exists, false otherwise
 */
bool
ecma_has_own_v (ecma_context_t* context_p, ecma_value_t object, ecma_value_t key)
{
  ecma_value_t value = ecma_find_own_v (context_p, object, key);

  ecma_free_value (context_p, value);

  return ecma_is_value_found (value);
} /* ecma_has_own_v */

/**
 * Add a value to an objects internal property map.
 *
 * @param context_p JJS context
 * @param object target
 * @param key magic string id
 * @param value JS value to set
 */
void annex_util_set_internal_m (jjs_context_t* context_p, jjs_value_t object, lit_magic_string_id_t key, jjs_value_t value)
{
  jjs_object_set_internal (context_p, object, ecma_make_magic_string_value (key), value);
}

/**
 * Get a value from an object's internal property map.
 *
 * @param context_p JJS context
 * @param object target
 * @param key magic string id
 * @return the requested value. if the value is not found, undefined is returned. the return
 * value must be free'd
 */
ecma_value_t annex_util_get_internal_m (jjs_context_t* context_p, ecma_value_t object, lit_magic_string_id_t key)
{
  jjs_value_t value = jjs_object_get_internal (context_p, object, ecma_make_magic_string_value (key));

  if (jjs_value_is_exception (context_p, value))
  {
    jjs_value_free (context_p, value);
    return ECMA_VALUE_UNDEFINED;
  }

  return value;
}
/**
 * Create a JS string from a UTF-8 encoded, null-terminated string.
 *
 * @param str_p cstring value. if NULL or empty, an empty string will be returned.
 * @return JS string. caller must free with ecma_free_value.
 */
jjs_value_t
annex_util_create_string_utf8_sz (jjs_context_t* context_p, const char* str_p)
{
  if (str_p == NULL || *str_p == '\0')
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  return jjs_string (context_p, (const jjs_char_t*) str_p, (jjs_size_t) strlen (str_p), JJS_ENCODING_UTF8);
} /* annex_util_create_string_utf8_sz */

/**
 * Checks if a package name specifier is valid.
 *
 * The validation goal is for module package names, used by vmod and pmap, to be consistent
 * with the current JS ecosystem. The validation more or less follows the validation strategy
 * used by npm (https://github.com/npm/validate-npm-package-name).
 *
 * Valid Package Name Rules:
 * - Length [0,214]
 * - No leading . or _
 * - Valid characters: a–z 0–9 - _ . @ /
 *
 * @param name string package name
 * @return true if the package name is valid; otherwise, false
 */
bool
annex_util_is_valid_package_name (jjs_context_t *context_p, /**< JJS context */
                                  ecma_value_t name)
{
  if (!ecma_is_value_string (name))
  {
    return false;
  }

  bool result = false;
  lit_utf8_byte_t c;
  lit_utf8_size_t i;
  ecma_string_t* name_p = ecma_get_string_from_value (context_p, name);

  ECMA_STRING_TO_UTF8_STRING (context_p, name_p, name_bytes_p, name_bytes_len);

  // empty strings are invalid
  // strings that are too long are invalid
  if (name_bytes_len == 0 || name_bytes_len > NPM_PACKAGE_NAME_LENGTH_LIMIT)
  {
    goto done;
  }

  // packages cannot begin with . or _
  if (name_bytes_p[0] == '.' || name_bytes_p[0] == '_')
  {
    goto done;
  }

  // only these characters are valid: a–z 0–9 - _ . @ / :
  for (i = 0; i < name_bytes_len; i++)
  {
    c = name_bytes_p[i];

    if (isalnum (c))
    {
      if (c >= 'A' && c <= 'Z')
      {
        goto done;
      }

      continue;
    }

    switch (c)
    {
      case '-':
      case '_':
      case '@':
      case '/':
      case ':':
      case '.':
        continue;
      default:
        goto done;
    }
  }

  result = true;

done:
  ECMA_FINALIZE_UTF8_STRING (context_p, name_bytes_p, name_bytes_len);
  return result;
} /* annex_util_is_valid_package_name */
