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

#include "jjs-annex.h"
#include "jjs-core.h"
#include "jjs-platform.h"

#include "ecma-builtin-helpers.h"
#include "ecma-helpers.h"

#include "annex.h"
#include "jcontext.h"

#if JJS_ANNEX_PMAP
static jjs_value_t validate_pmap (jjs_value_t pmap);
static ecma_value_t get_path_type (ecma_value_t object, lit_magic_string_id_t type, jjs_module_type_t module_type);
static ecma_value_t find_nearest_package_path (ecma_value_t packages, ecma_value_t root, ecma_value_t specifier, jjs_module_type_t module_type);
static bool is_object (ecma_value_t value);
static bool starts_with_dot_slash (ecma_value_t value);
static jjs_value_t expect_path_like_string (ecma_value_t value);
#endif /* JJS_ANNEX_PMAP */

/**
 * Load a pmap (Package Map).
 *
 * A pmap the way JJS translates ESM and CommonJS package names to an absolute module
 * file name. The pmap format is json and borrows from source maps and package.json
 * syntax.
 *
 * Here is a general example.
 *
 * {
 *   "packages": {
 *     "a": "./index.js",
 *     "b": {
 *       "main": "./index.js"
 *     },
 *     "c": {
 *       "module": "./index.mjs",
 *       "commonjs": "./index.cjs"
 *     },
 *     "d": {
 *       "path": "./path"
 *     },
 *     "@jjs/subpath": {
 *       "main": "index.js",
 *       "path": "./jjs_subpath"
 *     }
 *   }
 * }
 *
 * When the pmap is set, a root directory is set. At resolution, the package name and
 * commonjs or module is specified. The result is an absolute path. Here are examples
 * of how a root of /home/jjs would be resolved:
 *
 * resolve ("a") -> /home/jjs/index.js
 * resolve ("b") -> /home/jjs/index.js
 * resolve ("c", "module") -> /home/jjs/index.mjs
 * resolve ("c", "commonjs") -> /home/jjs/index.cjs
 * resolve ("d/file.js") -> /home/jjs/path/file.js
 * resolve ("@jjs/subpath") -> /home/jjs/subpath/index.js
 * resolve ("@jjs/subpath/specific.js") -> /home/jjs/subpath/specific.js
 *
 * With pmaps, most of the common package patterns are supported in a relatively
 * simple format.
 *
 * If a pmap has already been set in the current context, it will be replaced
 * and cleaned up if and only if the filename can be loaded and the pmap validated.
 * Otherwise, on error, the current pmap will remain unchanged.
 *
 * @param pmap pmap format JS object
 * @param pmap_o pmap reference ownership
 * @param filename JSON pmap file to load
 * @param filename_o filename reference ownership
 *
 * @return on success, undefined is returned. on failure, an exception is thrown. return
 * value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_pmap (jjs_value_t pmap, jjs_value_ownership_t pmap_o, jjs_value_t dirname, jjs_value_ownership_t dirname_o)
{
  jjs_assert_api_enabled ();
#if JJS_ANNEX_PMAP
  jjs_value_t result = validate_pmap (pmap);

  if (jjs_value_is_exception (result))
  {
    JJS_DISOWN (pmap, pmap_o);
    JJS_DISOWN (dirname, dirname_o);
    return result;
  }

  jjs_value_free (result);

  if (jjs_value_is_undefined (dirname))
  {
    JJS_DISOWN (dirname, dirname_o);
    dirname = jjs_platform_cwd ();
  }
  else if (!jjs_value_is_string (dirname))
  {
    JJS_DISOWN (pmap, pmap_o);
    JJS_DISOWN (dirname, dirname_o);
    return jjs_throw_sz (JJS_ERROR_TYPE, "");
  }
  else
  {
    dirname = jjs_platform_realpath (dirname, dirname_o);
  }

  if (jjs_value_is_exception (dirname))
  {
    JJS_DISOWN (pmap, pmap_o);
    return dirname;
  }

  // set the context pmap variables
  jjs_value_free (JJS_CONTEXT (pmap));
  JJS_CONTEXT (pmap) = pmap_o == JJS_MOVE ? pmap : jjs_value_copy (pmap);

  jjs_value_free (JJS_CONTEXT (pmap_root));
  JJS_CONTEXT (pmap_root) = dirname;

  return jjs_undefined ();
#else /* !JJS_ANNEX_PMAP */
  JJS_DISOWN (pmap, pmap_o);
  JJS_DISOWN (dirname, dirname_o);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PMAP_NOT_SUPPORTED));
#endif /* JJS_ANNEX_PMAP */
} /* jjs_pmap */

/**
 * Load a pmap (Package Map) from a file.
 *
 * The file must exist and be a valid pmap.json file format.
 *
 * The dirname of the file will be used as the pmap root. The pmap root
 * is used when resolving package specifiers with a pmap.
 *
 * If a pmap has already been set in the current context, it will be replaced
 * and cleaned up if and only if the filename can be loaded and the pmap validated.
 * Otherwise, on error, the current pmap will remain unchanged.
 *
 * @param filename JSON pmap file to load
 * @param filename_o filename reference ownership
 * @return on success, undefined is returned. on failure, an exception is thrown. return
 * value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_pmap_from_file (jjs_value_t filename, jjs_value_ownership_t filename_o)
{
  jjs_assert_api_enabled ();
#if JJS_ANNEX_PMAP
  // TODO: dirname does not work with relative paths, use realpath for now
  jjs_value_t resolved_filename = jjs_platform_realpath (filename, filename_o);

  if (jjs_value_is_exception (resolved_filename))
  {
    return resolved_filename;
  }

  jjs_value_t dirname = annex_path_dirname (resolved_filename);

  if (ecma_is_value_empty (dirname))
  {
    jjs_value_free (dirname);
    dirname = ecma_string_ascii_sz (".");
  }

  if (jjs_value_is_exception (dirname))
  {
    jjs_value_free (resolved_filename);
    return dirname;
  }

  jjs_value_t pmap = jjs_json_parse_file (resolved_filename, JJS_MOVE);

  if (jjs_value_is_exception (pmap))
  {
    jjs_value_free (dirname);
    return pmap;
  }

  return jjs_pmap (pmap, JJS_MOVE, dirname, JJS_MOVE);
#else /* !JJS_ANNEX_PMAP */
  JJS_DISOWN (filename, filename_o);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PMAP_NOT_SUPPORTED));
#endif /* JJS_ANNEX_PMAP */
} /* jjs_pmap_from_file */

/**
 * Version of jjs_pmap_from_file that takes a null-terminated string for the filename.
 *
 * @see jjs_pmap_from_file
 */
jjs_value_t jjs_pmap_from_file_sz (const char* filename_p)
{
  jjs_assert_api_enabled ();
  return jjs_pmap_from_file (annex_util_create_string_utf8_sz (filename_p), JJS_MOVE);
} /* jjs_pmap_from_file_sz */

/**
 * Resolve the absolute filename of a package specifier against the
 * currently set pmap and a module system.
 *
 * The specifier must be for a package. Filename specifiers (relative or
 * absolute) will throw an exception.
 *
 * The resolved file must exist on the filesystem. If it does not, an
 * exception will be thrown.
 *
 * If the package is not matched in the pmap, an exception will be thrown.
 *
 * If the pmap or pmap_root is not set, an exception will be thrown.
 *
 * If module_type is JJS_MODULE_TYPE_NONE, resolution via commonjs or
 * module systems will be excluded. Resolution will only happen if the
 * package is a string or the package object contains main or path. For
 * the nominal use case, a module system should be specified.
 *
 * @param specifier package name to resolve
 * @param specifier_o specifier reference ownership
 * @param module_type the module system to resolve against
 * @return absolute file path to the module. on error, an exception will be
 * thrown. return value must be freed with jjs_value_free().
 */
jjs_value_t jjs_pmap_resolve (jjs_value_t specifier, jjs_value_ownership_t specifier_o, jjs_module_type_t module_type)
{
  jjs_assert_api_enabled ();
#if JJS_ANNEX_PMAP
  jjs_value_t result = jjs_annex_pmap_resolve (specifier, module_type);
  JJS_DISOWN (specifier, specifier_o);
  return result;
#else /* !JJS_ANNEX_PMAP */
  JJS_UNUSED_ALL (module_type);
  JJS_DISOWN (specifier, specifier_o);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PMAP_NOT_SUPPORTED));
#endif /* JJS_ANNEX_PMAP */
} /* jjs_pmap_resolve */

/**
 * Version of jjs_pmap_resolve that takes a null-terminated string for the specifier.
 *
 * @see jjs_pmap_resolve
 */
jjs_value_t jjs_pmap_resolve_sz (const char* specifier_p, jjs_module_type_t module_type)
{
  jjs_assert_api_enabled ();
  return jjs_pmap_resolve (annex_util_create_string_utf8_sz (specifier_p), JJS_MOVE, module_type);
} /* jjs_pmap_resolve_sz */

#if JJS_ANNEX_PMAP

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
    ecma_value_t path = find_nearest_package_path (packages, pmap_root, specifier, module_type);

    ecma_free_value (packages);
    ecma_free_value (package_info);

    if (ecma_is_value_string (path))
    {
      return path;
    }

    ecma_free_value(path);

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
    result = jjs_throw_sz (JJS_ERROR_TYPE, "failed to resolve specifier");
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
 * Checks that package_info is a string or an object a main or path property.
 */
static jjs_value_t validate_path_or_main (ecma_value_t package_info, jjs_value_t key, jjs_module_type_t module_type)
{
  JJS_UNUSED(key);
  JJS_UNUSED(module_type);
  if (ecma_is_value_string (package_info))
  {
    return expect_path_like_string (package_info);
  }

  jjs_value_t result;
  ecma_value_t main_value = ecma_find_own_m (package_info, LIT_MAGIC_STRING_MAIN);
  ecma_value_t path_value = ecma_find_own_m (package_info, LIT_MAGIC_STRING_PATH);

  if (main_value == ECMA_VALUE_NOT_FOUND && path_value == ECMA_VALUE_NOT_FOUND)
  {
    result = jjs_throw_sz (JJS_ERROR_TYPE, "pmap package_info must have either a main or path property");
    goto done;
  }

  if (main_value != ECMA_VALUE_NOT_FOUND)
  {
    if (!ecma_is_value_string (main_value))
    {
      result = jjs_throw_sz (JJS_ERROR_TYPE, "pmap package_info main property must be a string");
      goto done;
    }

    result = expect_path_like_string (main_value);

    if (jjs_value_is_exception (result))
    {
      goto done;
    }

    jjs_value_free (result);
    result = ECMA_VALUE_TRUE;
  }

  if (path_value != ECMA_VALUE_NOT_FOUND)
  {
    if (!ecma_is_value_string (path_value))
    {
      result = jjs_throw_sz (JJS_ERROR_TYPE, "pmap package_info path property must be a string");
      goto done;
    }

    result = expect_path_like_string (path_value);

    if (jjs_value_is_exception (result))
    {
      goto done;
    }

    jjs_value_free (result);
    result = ECMA_VALUE_TRUE;
  }

done:
  ecma_free_value (main_value);
  ecma_free_value (path_value);
  return result;
} /* validate_path_or_main */

/**
 * Validate a module specific package info object.
 */
static jjs_value_t validate_module_type (ecma_value_t package_info, jjs_value_t key, jjs_module_type_t module_type)
{
  // Convert module type to magic string key.
  lit_magic_string_id_t module_type_key;

  switch (module_type)
  {
    case JJS_MODULE_TYPE_MODULE:
    {
      module_type_key = LIT_MAGIC_STRING_MODULE;
      break;
    }
    case JJS_MODULE_TYPE_COMMONJS:
    {
      module_type_key = LIT_MAGIC_STRING_COMMONJS;
      break;
    }
    default:
    {
      JJS_ASSERT (false && module_type == JJS_MODULE_TYPE_NONE);
      return ECMA_VALUE_NOT_FOUND;
    }
  }

  // Get module value.
  ecma_value_t module_type_value = ecma_find_own_m (package_info, module_type_key);

  if (module_type_value == ECMA_VALUE_NOT_FOUND)
  {
    return ECMA_VALUE_NOT_FOUND;
  }

  // Validate module value is a string or an object containing path and/or main.
  jjs_value_t result = validate_path_or_main (module_type_value, key, module_type);

  ecma_free_value (module_type_value);

  return result;
} /* validate_module_type */

/**
 * Validate a pacakge info object from a pmap (package map) object.
 */
static jjs_value_t validate_package_info (ecma_value_t package_info, jjs_value_t key)
{
  // Validate pkg.commonjs if it exists.
  jjs_value_t result = validate_module_type (package_info, key, JJS_MODULE_TYPE_COMMONJS);

  if (jjs_value_is_exception(result))
  {
    return result;
  }

  bool commonjs_found = jjs_value_is_true (result);

  jjs_value_free (result);
  result = validate_module_type (package_info, key, JJS_MODULE_TYPE_MODULE);

  if (jjs_value_is_exception(result))
  {
    return result;
  }

  // Validate pkg.module if it exists.
  bool module_found = jjs_value_is_true (result);

  jjs_value_free (result);

  if (commonjs_found || module_found)
  {
    // If a module type is present, ensure the package_info does not contain path or main.
    if (ecma_has_own_m (package_info, LIT_MAGIC_STRING_PATH) || ecma_has_own_m (package_info, LIT_MAGIC_STRING_MAIN))
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, "pmap package_info cannot have a path or main property if it has a module or commonjs property");
    }

    return ECMA_VALUE_TRUE;
  }
  else
  {
    // Validate package_info can be a string (shorthand for pkg.main) or an object containing main or path.
    return validate_path_or_main(package_info, key, JJS_MODULE_TYPE_NONE);
  }
} /* validate_package_info */

/**
 * Validate a pmap.
 */
static jjs_value_t validate_pmap (jjs_value_t pmap)
{
  if (!is_object (pmap))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "pmap must be an object");
  }

  ecma_value_t packages = ecma_find_own_m (pmap, LIT_MAGIC_STRING_PACKAGES);

  if (packages == ECMA_VALUE_NOT_FOUND)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "pmap contains no 'packages' property");
  }

  if (!is_object (packages))
  {
    ecma_free_value (packages);
    return jjs_throw_sz (JJS_ERROR_TYPE, "pmap 'packages' property must be an object");
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

    if (!annex_util_is_valid_package_name (key))
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
  lit_utf8_size_t specifier_length = ecma_string_get_length (specifier_p);
  lit_utf8_size_t last_slash_index = specifier_length;
  ecma_value_t slash = ecma_make_magic_string_value (LIT_MAGIC_STRING_SLASH_CHAR);
  ecma_value_t result = ECMA_VALUE_NOT_FOUND;

  while ((last_slash_index = last_index_of (specifier, slash, last_slash_index)) != UINT32_MAX)
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
          ecma_value_t trailing = substr (specifier, last_slash_index + 1, specifier_length);

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

    if (result != ECMA_VALUE_NOT_FOUND)
    {
      break;
    }

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

/**
 * Checks if the value is an object, assuming it came from an object from JSON.parse().
 *
 * In the context of validating and reading pmaps, this is faster than ecma_is_value_array().
 */
static bool
is_object (ecma_value_t value)
{
  return (ecma_is_value_object (value)
          && ecma_get_object_base_type (ecma_get_object_from_value (value)) != ECMA_OBJECT_BASE_TYPE_ARRAY);
} /* is_object */

/**
 * Checks if a value is a string that starts with "./".
 */
static bool
starts_with_dot_slash (ecma_value_t value)
{
  if (!ecma_is_value_string(value))
  {
    return false;
  }

  lit_utf8_byte_t path[2];
  lit_utf8_byte_t len = sizeof (path) / sizeof (lit_utf8_byte_t);
  lit_utf8_size_t written = ecma_string_copy_to_buffer (
    ecma_get_string_from_value (value), path, len, JJS_ENCODING_CESU8);

  if (written != len) {
    return false;
  }

  return path[0] == '.' && path[1] == '/';
} /* starts_with_dot_slash */

/**
 * Checks if a value is a string that starts with "./". If not, throws an error.
 */
static jjs_value_t
expect_path_like_string (ecma_value_t value)
{
  if (!starts_with_dot_slash (value))
  {
    return jjs_throw_sz(JJS_ERROR_TYPE, "pmap: fs path values must start with './'");
  }

  return ECMA_VALUE_TRUE;
} /* expect_path_like_string */

#endif /* JJS_ANNEX_PMAP */
