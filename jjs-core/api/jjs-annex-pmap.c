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

#include "jjs-core.h"
#include "jjs-port.h"
#include "jjs-annex.h"

#include "annex.h"
#include "jcontext.h"

static jjs_value_t validate_pmap (jjs_value_t pmap);
static ecma_value_t get_main (ecma_value_t object, jjs_module_type_t module_type);

/**
 * Get the current pmap (pacakge map) root directory.
 *
 * @return string or undefined
 */
jjs_value_t jjs_pmap_root(void)
{
  jjs_assert_api_enabled ();

#if JJS_PMAP
  return jjs_value_copy (JJS_CONTEXT (pmap_root));
#else /* !JJS_PMAP */
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PMAP_NOT_SUPPORTED));
#endif /* JJS_PMAP */
} /* jjs_pmap_root */

/**
 * Set the current pmap (package map) root directory.
 *
 * @param path string or undefined
 * @return true on success, exception on error. return value must be freed by caller.
 */
jjs_value_t jjs_pmap_set_root (jjs_value_t path)
{
  jjs_assert_api_enabled ();

#if JJS_PMAP
  if (!ecma_is_value_string (path) && !ecma_is_value_undefined (path))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_EXPECTED_STRING_OR_UNDEFINED));
  }

  jjs_value_free (JJS_CONTEXT (pmap_root));
  JJS_CONTEXT (pmap_root) = jjs_value_copy (path);

  return ECMA_VALUE_TRUE;
#else /* !JJS_PMAP */
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PMAP_NOT_SUPPORTED));
#endif /* JJS_PMAP */
} /* jjs_pmap_set_root */

/**
 * Set the current pmap (package map) from a JSON string.
 *
 * @param json_string JSON
 * @return true on success, exception on error. return value must be freed by caller.
 */
jjs_value_t jjs_pmap_from_json (jjs_value_t json_string)
{
  jjs_assert_api_enabled ();

#if JJS_PMAP
  if (!jjs_value_is_string(json_string))
  {
    if (jjs_value_is_undefined(json_string))
    {
      JJS_CONTEXT (pmap) = jjs_undefined ();
      return ECMA_VALUE_TRUE;
    }

    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_EXPECTED_STRING_OR_UNDEFINED));
  }

  ECMA_STRING_TO_UTF8_STRING (ecma_get_string_from_value (json_string), json_string_p, json_string_size);

  jjs_value_t parsed = jjs_json_parse (json_string_p, json_string_size);

  ECMA_FINALIZE_UTF8_STRING (json_string_p, json_string_size);

  if (jjs_value_is_exception (parsed))
  {
    return parsed;
  }

  jjs_value_t result = validate_pmap (parsed);

  if (jjs_value_is_true(result))
  {
    JJS_CONTEXT (pmap) = parsed;
  }
  else
  {
    jjs_value_free(parsed);
  }

  return result;
#else /* !JJS_PMAP */
  (void) json_string;
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PMAP_NOT_SUPPORTED));
#endif /* JJS_PMAP */
} /* jjs_pmap_from_json */

#if JJS_PMAP

/**
 * Resolve a specifier or request against the current pmap (package map).
 *
 * @param specifier ESM specifier or CommonJS request
 * @param module_type ESM or CommonJS
 * @return absolute filename or exception if specifier could not be resolved
 */
jjs_value_t jjs_annex_pmap_resolve (jjs_value_t specifier, jjs_module_type_t module_type)
{
  jjs_assert_api_enabled ();

  if (!jjs_value_is_string (specifier))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "specifier must be a string");
  }

  ecma_value_t pmap = JJS_CONTEXT (pmap);

  if (!ecma_is_value_object (pmap))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "pmap has not been set");
  }

  ecma_value_t pmap_root = JJS_CONTEXT (pmap_root);

  if (!ecma_is_value_string (pmap_root))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "pmap root has not been set");
  }

  ecma_value_t packages = ecma_find_own_m (pmap, LIT_MAGIC_STRING_PACKAGES);
  ecma_value_t package_info = ecma_find_own_v (packages, specifier);

  if (package_info == ECMA_VALUE_NOT_FOUND)
  {
    ecma_free_value (packages);
    ecma_free_value (package_info);

    return jjs_throw_sz (JJS_ERROR_TYPE, "package not found");
  }

  ecma_value_t file = get_main (package_info, module_type);
  ecma_value_t result = ECMA_VALUE_EMPTY;

  if (jjs_value_is_string (file))
  {
    result = annex_path_join (pmap_root, file, true);
  }

  if (ecma_is_value_empty (result))
  {
    result = jjs_throw_sz (JJS_ERROR_TYPE, "failed to resolve specifier");
  }

  ecma_free_value (packages);
  ecma_free_value (package_info);
  ecma_free_value (file);

  return result;
} /* jjs_annex_pmap_resolve */

/**
 * Resolve main from a pmap object.
 */
static ecma_value_t get_main (ecma_value_t object, jjs_module_type_t module_type)
{
  if (ecma_is_value_string (object))
  {
    return ecma_copy_value (object);
  }

  if (!ecma_is_value_object (object))
  {
    return ECMA_VALUE_EMPTY;
  }

  ecma_value_t m = ecma_find_own_m (object, LIT_MAGIC_STRING_MAIN);

  if (ecma_is_value_string (m))
  {
    return m;
  }

  ecma_free_value (m);

  ecma_value_t module;

  if (module_type == JJS_MODULE_TYPE_MODULE)
  {
    module = ecma_find_own_m (object, LIT_MAGIC_STRING_MODULE);
  }
  else if (module_type == JJS_MODULE_TYPE_COMMONJS)
  {
    module = ecma_find_own_m (object, LIT_MAGIC_STRING_COMMONJS);
  }
  else
  {
    return ECMA_VALUE_EMPTY;
  }

  ecma_value_t result = get_main (module, JJS_MODULE_TYPE_NONE);

  ecma_free_value (module);

  return result;
} /* get_main */

/**
 * Resolve package_info from a pmap (package map) object
 */
static bool validate_package_info_contents (jjs_value_t package_info,
                                            lit_magic_string_id_t key,
                                            bool check_module_type)
{
  if (ecma_is_value_string (package_info))
  {
    ecma_free_value (package_info);
    return true;
  }
  else if (!ecma_is_value_object (package_info))
  {
    return false;
  }

  ecma_value_t value = ecma_find_own_m (package_info, key);

  if (ecma_is_value_string (value))
  {
    ecma_free_value (value);
    return true;
  }

  ecma_free_value (value);

  if (check_module_type)
  {
    ecma_value_t module_type = ecma_find_own_m (package_info, LIT_MAGIC_STRING_MODULE);
    bool result = validate_package_info_contents (module_type, key, false);

    ecma_free_value (module_type);

    if (result)
    {
      return true;
    }

    module_type = ecma_find_own_m (package_info, LIT_MAGIC_STRING_COMMONJS);

    ecma_free_value (module_type);

    return validate_package_info_contents (module_type, key, false);
  }

  return false;
} /* validate_package_info_contents */

/**
 * Throw a packages errors.
 */
static jjs_value_t throw_package_info_error (jjs_value_t key, const char* message)
{
  char buffer[256];
  char key_cstr[256];
  jjs_size_t written = jjs_string_to_buffer (key, JJS_ENCODING_UTF8, (jjs_char_t*) key_cstr, sizeof (key_cstr));

  key_cstr[written] = '\0';
  snprintf (buffer, sizeof (buffer), "pmap packages['%s'] %s", key_cstr, message);

  return jjs_throw_sz (JJS_ERROR_TYPE, buffer);
} /* throw_package_info_error */

/**
 * Validate a pacakge info object from a pmap (package map) object.
 */
static jjs_value_t validate_package_info (jjs_value_t package_info, jjs_value_t key)
{
  if (jjs_value_is_string (package_info))
  {
    return ECMA_VALUE_TRUE;
  }
  else if (jjs_value_is_object (package_info))
  {
    // TODO: validate_package_info_contents(package_info, LIT_MAGIC_STRING_PATH, true)
    if (validate_package_info_contents (package_info, LIT_MAGIC_STRING_MAIN, true))
    {
      return ECMA_VALUE_TRUE;
    }

    return throw_package_info_error (key, "value must be a string or contain a 'main' property");
  }
  else
  {
    return throw_package_info_error (key, "value must be a string or an object");
  }
}

/**
 * Validate a pmap.
 */
static jjs_value_t validate_pmap (jjs_value_t pmap)
{
  if (!ecma_is_value_object (pmap))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_EXPECTED_AN_OBJECT));
  }

  ecma_value_t packages = ecma_find_own_m (pmap, LIT_MAGIC_STRING_PACKAGES);

  if (packages == ECMA_VALUE_NOT_FOUND)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "pmap contains no 'packages' property");
  }

  jjs_value_t keys = jjs_object_keys (packages);

  if (jjs_value_is_exception (keys))
  {
    ecma_free_value (keys);
    ecma_free_value (packages);
    return jjs_throw_sz (JJS_ERROR_TYPE, "pmap 'packages' contains no keys");
  }

  jjs_size_t keys_count = jjs_array_length (keys);

  for (jjs_size_t i = 0; i < keys_count; i++)
  {
    jjs_value_t key = jjs_object_get_index (keys, i);

    if (!jjs_value_is_string (key))
    {
      jjs_value_free (key);
      continue;
    }

    ecma_value_t package_info = ecma_find_own_v (packages, key);
    jjs_value_t package_info_result = validate_package_info (package_info, key);

    jjs_value_free (key);
    ecma_free_value (package_info);

    if (jjs_value_is_exception (package_info_result))
    {
      ecma_free_value (keys);
      ecma_free_value (packages);
      return package_info_result;
    }

    jjs_value_free (package_info_result);
  }

  ecma_free_value (keys);
  ecma_free_value (packages);

  return ECMA_VALUE_TRUE;
} /* validate_pmap */

#endif /* JJS_PMAP */
