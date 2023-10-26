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
#include "ecma-helpers.h"
#include "ecma-builtin-helpers.h"

#if JJS_PMAP
static jjs_value_t validate_pmap (jjs_value_t pmap);
static ecma_value_t get_path_type (ecma_value_t object, lit_magic_string_id_t type, jjs_module_type_t module_type);
static jjs_value_t set_pmap_from_json (const jjs_char_t* json_string_p, jjs_size_t json_string_size, jjs_value_t root);
static ecma_value_t find_nearest_package_path (ecma_value_t packages, ecma_value_t root, ecma_value_t specifier, jjs_module_type_t module_type);
#endif /* JJS_PMAP */

/**
 * Load a pmap (Package Map) from a file.
 *
 * The file must exist and be a valid JSON file in the pmap format. The pmap root
 * will be set to the root directory of the pmap file.
 *
 * @param filename JSON pmap file to load
 * @return on success, true is returned. on failure, an exception is thrown. return
 * value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_pmap (jjs_value_t filename)
{
#if JJS_PMAP
  jjs_assert_api_enabled ();
  // get filename and dirname
  jjs_value_t normalized = annex_path_normalize (filename);

  if (!jjs_value_is_string (normalized))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Invalid filename");
  }

  jjs_value_t root_path = annex_path_dirname (normalized);

  if (!jjs_value_is_string (root_path))
  {
    jjs_value_free (normalized);
    return jjs_throw_sz (JJS_ERROR_TYPE, "Failed to get dirname from normalized filename");
  }

  ecma_cstr_t filename_cstr = ecma_string_to_cstr (normalized);

  jjs_value_free (normalized);

  if (filename_cstr.size == 0)
  {
    jjs_value_free (root_path);
    return jjs_throw_sz (JJS_ERROR_TYPE, "Failed to parse normalized filename");
  }

  // read JSON file
  jjs_size_t filename_size;
  jjs_char_t* source = jjs_port_source_read (filename_cstr.str_p, &filename_size);

  ecma_free_cstr (&filename_cstr);

  if (!source)
  {
    jjs_value_free (root_path);
    jjs_port_source_free (source);
    return jjs_throw_sz (JJS_ERROR_TYPE, "Failed to read json source file.");
  }

  // set pmap variables from JSON source
  jjs_value_t result = set_pmap_from_json (source, filename_size, root_path);

  jjs_port_source_free (source);
  jjs_value_free (root_path);

  return result;
#else /* !JJS_PMAP */
  JJS_UNUSED (filename);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PMAP_NOT_SUPPORTED));
#endif /* JJS_PMAP */
} /* jjs_pmap */

/**
 * Load a pmap (Package Map) from a JSON string.
 *
 * The pmap root must be specified by the caller.
 *
 * @param json_string JSON pmap string
 * @param root pmap root directory
 * @return on success, true is returned. on failure, an exception is thrown. return
 *         value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_pmap_from_json (jjs_value_t json_string, jjs_value_t root)
{
#if JJS_PMAP
  if (!jjs_value_is_string (json_string))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "json_string must be a string");
  }

  ecma_string_t* source_p = ecma_get_string_from_value (json_string);

  ECMA_STRING_TO_UTF8_STRING (source_p, json_string_p, json_string_size);

  jjs_value_t result = set_pmap_from_json (json_string_p, json_string_size, root);

  ECMA_FINALIZE_UTF8_STRING (json_string_p, json_string_size);

  return result;
#else /* !JJS_PMAP */
  JJS_UNUSED (json_string);
  JJS_UNUSED (root);
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

  ecma_value_t file = get_path_type (package_info, LIT_MAGIC_STRING_MAIN, module_type);
  ecma_value_t result = ECMA_VALUE_EMPTY;

  if (jjs_value_is_string (file))
  {
    result = annex_path_join (pmap_root, file, true);
  }

  if (ecma_is_value_empty (result))
  {
    ecma_free_value(result);
    result = find_nearest_package_path (packages, pmap_root, specifier, module_type);

    if (!ecma_is_value_string (result))
    {
      ecma_free_value(result);
      result = jjs_throw_sz (JJS_ERROR_TYPE, "failed to resolve specifier");
    }
  }

  ecma_free_value (packages);
  ecma_free_value (package_info);
  ecma_free_value (file);

  return result;
} /* jjs_annex_pmap_resolve */

/**
 * Resolve main or path from a pmap object.
 */
static ecma_value_t get_path_type (ecma_value_t object, lit_magic_string_id_t type, jjs_module_type_t module_type)
{
  if (ecma_is_value_string (object))
  {
    return ecma_copy_value (object);
  }

  if (!ecma_is_value_object (object))
  {
    return ECMA_VALUE_EMPTY;
  }

  ecma_value_t m = ecma_find_own_m (object, type);

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

  ecma_value_t result = get_path_type (module, type, JJS_MODULE_TYPE_NONE);

  ecma_free_value (module);

  return result;
} /* get_path_type */

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
  char buffer[512];
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
    if (validate_package_info_contents (package_info, LIT_MAGIC_STRING_MAIN, true)
        || validate_package_info_contents(package_info, LIT_MAGIC_STRING_PATH, true))
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

/**
 * Set the pmap context variables from a JSON string.
 */
static jjs_value_t
set_pmap_from_json (const jjs_char_t* json_string_p, jjs_size_t json_string_size, jjs_value_t root)
{
  if (jjs_value_is_string (root))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "pmap root must be a string");
  }

  // parse JSON
  jjs_value_t pmap = jjs_json_parse (json_string_p, json_string_size);

  if (jjs_value_is_exception (pmap))
  {
    return pmap;
  }

  // validate pmap
  jjs_value_t result = validate_pmap (pmap);

  if (jjs_value_is_exception (result))
  {
    ecma_free_value (pmap);
    return result;
  }

  // set the context pmap variables
  jjs_value_free (JJS_CONTEXT (pmap));
  JJS_CONTEXT (pmap) = pmap;
  jjs_value_free (JJS_CONTEXT (pmap_root));
  JJS_CONTEXT (pmap_root) = jjs_value_copy (root);

  return ECMA_VALUE_TRUE;
}

/**
 * Call string.lastIndexOf() with the given arguments.
 */
static lit_utf8_size_t
last_index_of (ecma_value_t str, ecma_value_t search, lit_utf8_size_t position)
{
  ecma_string_t* path_string_p = ecma_get_string_from_value (str);
  ecma_value_t position_value = ecma_make_uint32_value (position);
  ecma_value_t value = ecma_builtin_helper_string_prototype_object_index_of (
    path_string_p, search, position_value, ECMA_STRING_LAST_INDEX_OF);

  int32_t result = ecma_is_value_integer_number (value) ? ecma_get_integer_from_value (value) : -1;

  ecma_free_value (value);
  ecma_free_value (position_value);

  return result < 0 ? UINT32_MAX : (lit_utf8_size_t) result;
} /* last_index_of */

/**
 * Call string.substr() with the given arguments.
 */
static ecma_value_t
substr (ecma_value_t str, lit_utf8_size_t start, lit_utf8_size_t len)
{
  return ecma_make_string_value (ecma_string_substr (ecma_get_string_from_value (str), start, len));
} /* substr */

/**
 * Find the nearest package path for the given specifier.
 *
 * The algorithm splits the specifier on the last slash. The first part is
 * the package name, the second part is the trailing basename. If the
 * package name exists in the pmap, the algorithm joins pmap_root, package
 * path and the trailing basename. If the package name does not exist, the
 * algorithm splits on the next slash and tries again until the specifier
 * has no more slashes.
 */
static ecma_value_t
find_nearest_package_path (ecma_value_t packages, ecma_value_t root, ecma_value_t specifier, jjs_module_type_t module_type)
{
  ecma_string_t* specifier_p = ecma_get_string_from_value (specifier);
  lit_utf8_size_t last_slash_index = ecma_string_get_length (specifier_p);
  ecma_value_t slash = ecma_make_magic_string_value (LIT_MAGIC_STRING_SLASH_CHAR);
  ecma_value_t result = ECMA_VALUE_NOT_FOUND;

  while (result == ECMA_VALUE_NOT_FOUND || (last_slash_index = last_index_of (specifier, slash, last_slash_index)) != UINT32_MAX)
  {
    ecma_value_t package = substr (specifier, 0, last_slash_index);
    ecma_value_t package_info = ecma_find_own_v (packages, package);

    if (package_info != ECMA_VALUE_NOT_FOUND)
    {
      ecma_value_t path = get_path_type (package_info, LIT_MAGIC_STRING_PATH, module_type);

      if (ecma_is_value_string (path))
      {
        ecma_value_t temp = annex_path_join (root, path, false);

        if (ecma_is_value_string(temp))
        {
          ecma_value_t trailing = substr (specifier, last_slash_index + 1, UINT32_MAX);

          result = annex_path_join(temp, trailing, true);
          ecma_free_value(trailing);
        }
        else
        {
          result = ECMA_VALUE_EMPTY;
        }

        ecma_free_value(temp);
      }
      else
      {
        result = ECMA_VALUE_EMPTY;
      }

      ecma_free_value (path);
    }

    ecma_fast_free_value (package);
    ecma_fast_free_value (package_info);

    if (last_slash_index > 0)
    {
      last_slash_index--;
    }
    else
    {
      result = ECMA_VALUE_EMPTY;
      break;
    }
  }

  ecma_fast_free_value (slash);

  return result;
} /* find_nearest_package_path */

#endif /* JJS_PMAP */
