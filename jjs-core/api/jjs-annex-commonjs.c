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
#include "jjs-annex.h"
#include "jjs-annex-module-util.h"

#include "jjs-snapshot.h"
#include "jjs-annex-vmod.h"

#include "annex.h"
#include "jcontext.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"

/**
* Import a CommonJS module.
*
* The specifier can be a package name, relative path (qualified with
* ./ or ../) or absolute path. Package names are resolved by the currently
* set pmap.
*
* @param specifier string value of the specifier
* @param specifier_o specifier reference ownership
* @return module export object. on error, an exception is returned. return
* value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_commonjs_require (jjs_value_t specifier, jjs_value_ownership_t specifier_o)
{
  jjs_assert_api_enabled ();
#if JJS_ANNEX_COMMONJS
  jjs_value_t referrer_path = annex_path_cwd ();
  jjs_value_t result = jjs_annex_require (specifier, referrer_path);

  jjs_value_free (referrer_path);

  if (specifier_o == JJS_MOVE)
  {
    jjs_value_free (specifier);
  }

  return result;
#else /* !JJS_ANNEX_COMMONJS */
  JJS_UNUSED (specifier);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_COMMONJS_NOT_SUPPORTED));
#endif /* JJS_ANNEX_COMMONJS */
} /* jjs_commonjs_require */

/**
* Import a CommonJS module.
*
* @see jjs_commonjs_require
*
* @param specifier string value of the specifier
* @return module export object. on error, an exception is returned. return
* value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_commonjs_require_sz (const char *specifier_p)
{
  jjs_assert_api_enabled ();
#if JJS_ANNEX_COMMONJS
  return jjs_commonjs_require (annex_util_create_string_utf8_sz (specifier_p), JJS_MOVE);
#else /* !JJS_ANNEX_COMMONJS */
  JJS_UNUSED (specifier_p);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_COMMONJS_NOT_SUPPORTED));
#endif /* JJS_ANNEX_COMMONJS */
} /* jjs_commonjs_require_sz */

#if JJS_ANNEX_COMMONJS

static jjs_value_t create_require_from_directory (jjs_value_t referrer_path);
static void referrer_path_free (void *native_p, const jjs_object_native_info_t *info_p);
static ecma_value_t create_module (ecma_value_t filename);
static ecma_value_t load_module (ecma_value_t module, ecma_value_t filename, ecma_value_t format);
static ecma_value_t load_module_exports_from_source (ecma_value_t module, ecma_value_t source);
static ecma_value_t load_module_exports_from_snapshot (ecma_value_t module, ecma_value_t source);
static ecma_value_t run_module (ecma_value_t module, ecma_value_t filename, jjs_value_t fn);
static ecma_value_t require_impl (ecma_value_t specifier, ecma_value_t referrer_path);
static JJS_HANDLER(require_handler);
static JJS_HANDLER(resolve_handler);

static jjs_object_native_info_t referrer_path_id = {
  .free_cb = referrer_path_free,
  .number_of_references = 0,
  .offset_of_references = 0,
};

/**
 * Create a require function for a module filename.
 *
 * dirname of the filename will be taken as the base directory for resolving
 *
 * @param referrer filename of module
 * @return require function or exception on error. return value must be freed.
 */
jjs_value_t jjs_annex_create_require (jjs_value_t referrer)
{
  ecma_value_t path;

  if (jjs_value_is_undefined (referrer))
  {
    path = annex_path_cwd ();
  }
  else if (jjs_value_is_string (referrer))
  {
    annex_specifier_type_t type = annex_path_specifier_type (referrer);

    // this function is only called internally with an absolute filename
    JJS_ASSERT (type == ANNEX_SPECIFIER_TYPE_ABSOLUTE);

    if (type != ANNEX_SPECIFIER_TYPE_ABSOLUTE) {
      return jjs_throw_sz (JJS_ERROR_COMMON, "create_require expects an absolute filename");
    }

    path = annex_path_dirname (referrer);
  }
  else
  {
    path = ECMA_VALUE_EMPTY;
  }

  if (ecma_is_value_string (path))
  {
    ecma_value_t fn = create_require_from_directory (path);

    ecma_free_value(path);

    return fn;
  }

  ecma_free_value(path);

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_EXPECTED_STRING_OR_UNDEFINED));
} /* jjs_annex_create_require */

/**
 * Require a module from a specifier relative to a directory.
 *
 * @param specifier package name or file path string
 * @param referrer_path directory string to resolve relative file paths against
 * @return on success, module exports are returned. on error, exception is returned.
 */
jjs_value_t
jjs_annex_require (jjs_value_t specifier, jjs_value_t referrer_path)
{
  return require_impl (specifier, referrer_path);
} /* jjs_annex_require */

/**
 * Create a require function from a directory path.
 *
 * @param referrer_path directory path
 * @return require function or exception on error. return value must be freed.
 */
static jjs_value_t create_require_from_directory (jjs_value_t referrer_path)
{
  bool create_native_pointer_result;
  ecma_global_object_t* global_p = ecma_get_global_object ();
  ecma_object_t* require_p = ecma_op_create_external_function_object (require_handler);
  ecma_object_t* resolve_p = ecma_op_create_external_function_object (resolve_handler);
  ecma_value_t require = ecma_make_object_value (require_p);

  // put referrer path in native slot of require and resolve
  create_native_pointer_result = ecma_create_native_pointer_property (
    require_p, (void*)(uintptr_t)ecma_copy_value (referrer_path), &referrer_path_id);

  JJS_ASSERT (create_native_pointer_result);

  create_native_pointer_result = ecma_create_native_pointer_property (
    resolve_p, (void*)(uintptr_t)ecma_copy_value (referrer_path), &referrer_path_id);

  JJS_ASSERT (create_native_pointer_result);

  // set require.resolve
  ecma_set_m (require, LIT_MAGIC_STRING_RESOLVE, ecma_make_object_value (resolve_p));

  // set require.cache
  ecma_set_m (require, LIT_MAGIC_STRING_CACHE, global_p->commonjs_cache);

  // cleanup
  ecma_deref_object (resolve_p);

  return require;
} /* create_require_from_directory */

/**
 * Free the native pointer stored in the require or resolve function object.
 */
static void referrer_path_free (void *native_p, const jjs_object_native_info_t *info_p)
{
  (void) info_p;
  ecma_free_value ((ecma_value_t)(uintptr_t)native_p);
} /* referrer_path_free */

/**
 * Get the referrer path from the require or resolve function object.
 *
 * @param target require or resolve function object
 * @return directory path or exception on error. return value must be freed.
 */
static ecma_value_t get_referrer_path (ecma_value_t target)
{
  ecma_native_pointer_t* native_wrap_p = ecma_get_native_pointer_value (
    ecma_get_object_from_value (target), &referrer_path_id);

  JJS_ASSERT (native_wrap_p != NULL && native_wrap_p->native_p != NULL);

  if (native_wrap_p == NULL || native_wrap_p->native_p == NULL)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Invalid native pointer");
  }

  return ecma_copy_value ((ecma_value_t)(uintptr_t)native_wrap_p->native_p);
} /* get_referrer_path */

/**
 * Binding for the javascript resolve() function.
 */
static JJS_HANDLER(resolve_handler)
{
  ecma_value_t request = args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED;

  if (!ecma_is_value_string (request))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Invalid argument");
  }

#if JJS_ANNEX_VMOD
  if (jjs_vmod_exists (request))
  {
    return ecma_copy_value (request);
  }
#endif

  ecma_value_t referrer_path = get_referrer_path (call_info_p->function);

  if (ecma_is_value_exception (referrer_path))
  {
    return referrer_path;
  }

  jjs_annex_module_resolve_t result = jjs_annex_module_resolve (request, referrer_path, JJS_MODULE_TYPE_COMMONJS);

  ecma_free_value (referrer_path);

  if (jjs_value_is_exception (result.result))
  {
    return result.result;
  }

  ecma_value_t path = result.path;

  result.path = ECMA_VALUE_UNDEFINED;
  jjs_annex_module_resolve_free (&result);

  return path;
} /* resolve_handler */

/**
 * Binding for the javascript require() function.
 */
static JJS_HANDLER (require_handler)
{
  ecma_value_t specifier = args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED;
  ecma_value_t referrer_path = get_referrer_path (call_info_p->function);

  if (ecma_is_value_exception (referrer_path))
  {
    return referrer_path;
  }

  jjs_value_t result = require_impl (specifier, referrer_path);

  jjs_value_free (referrer_path);

  return result;
} /* require_handler */

/**
 * Shared require implementation.
 */
static ecma_value_t
require_impl (ecma_value_t specifier, ecma_value_t referrer_path)
{
  if (!ecma_is_value_string (specifier))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Invalid require specifier");
  }

  if (!ecma_is_value_string (referrer_path))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Invalid require referrer");
  }

#if JJS_ANNEX_VMOD
  if (jjs_annex_vmod_exists (specifier))
  {
    return jjs_annex_vmod_resolve (specifier);
  }
#endif /* JJS_ANNEX_VMOD */

  /* resolve the request to an absolute path */
  jjs_annex_module_resolve_t resolved = jjs_annex_module_resolve (specifier, referrer_path, JJS_MODULE_TYPE_COMMONJS);

  if (jjs_value_is_exception (resolved.result))
  {
    return resolved.result;
  }

  /* find the request in the module cache. */
  ecma_value_t commonjs_cache = ecma_get_global_object ()->commonjs_cache;
  ecma_value_t cached_module = ecma_find_own_v (commonjs_cache, resolved.path);

  if (cached_module != ECMA_VALUE_NOT_FOUND)
  {
    /* module.loaded must be true. if not, invalid module or circular dependency */
    ecma_value_t loaded = ecma_find_own_m (cached_module, LIT_MAGIC_STRING_LOADED);
    bool is_loaded = ecma_is_value_true (loaded);

    ecma_free_value (loaded);
    jjs_annex_module_resolve_free (&resolved);

    if (!is_loaded)
    {
      ecma_free_value (cached_module);

      return jjs_throw_sz (JJS_ERROR_TYPE, "Circular dependency");
    }

    /* return module.exports */
    ecma_value_t exports = ecma_find_own_m (cached_module, LIT_MAGIC_STRING_EXPORTS);

    ecma_free_value (cached_module);

    if (exports == ECMA_VALUE_NOT_FOUND)
    {
      ecma_free_value (exports);

      return jjs_throw_sz (JJS_ERROR_TYPE, "Invalid module");
    }

    return exports;
  }

  ecma_value_t module = create_module (resolved.path);

  ecma_set_v (commonjs_cache, resolved.path, module);

  ecma_value_t load_module_result = load_module (module, resolved.path, resolved.format);

  jjs_annex_module_resolve_free (&resolved);

  if (jjs_value_is_exception (load_module_result))
  {
    ecma_value_t delete_result = ecma_op_object_delete (ecma_get_object_from_value (commonjs_cache),
                                                        ecma_get_string_from_value (resolved.path),
                                                        false);
    ecma_free_value (delete_result);
    ecma_free_value (module);

    return load_module_result;
  }

  ecma_value_t exports = ecma_find_own_m (module, LIT_MAGIC_STRING_EXPORTS);

  ecma_set_m (module, LIT_MAGIC_STRING_LOADED, ECMA_VALUE_TRUE);

  ecma_free_value (load_module_result);
  ecma_free_value (module);

  return exports;
} /* require_impl */

/**
 * Create a CommonJS module object.
 */
static ecma_value_t create_module (ecma_value_t filename)
{
  ecma_value_t module = ecma_create_object_with_null_proto ();
  ecma_value_t exports = ecma_create_object_with_null_proto ();
  ecma_value_t path_dirname = annex_path_dirname (filename);

  ecma_set_m (module, LIT_MAGIC_STRING_ID, filename);
  ecma_set_m (module, LIT_MAGIC_STRING_FILENAME, filename);
  ecma_set_m (module, LIT_MAGIC_STRING_EXPORTS, exports);
  ecma_free_value (exports);
  ecma_set_m (module, LIT_MAGIC_STRING_PATH, path_dirname);
  ecma_free_value (path_dirname);
  ecma_set_m (module, LIT_MAGIC_STRING_LOADED, ECMA_VALUE_FALSE);

  ecma_op_ordinary_object_prevent_extensions (ecma_get_object_from_value (module));

  return module;
} /* create_module */

/**
 * Load a CommonJS module.
 */
static ecma_value_t load_module (ecma_value_t module, ecma_value_t filename, ecma_value_t format)
{
  jjs_annex_module_load_t loaded = jjs_annex_module_load (filename, format, JJS_MODULE_TYPE_COMMONJS);

  if (jjs_value_is_exception (loaded.result))
  {
    return loaded.result;
  }

  ecma_string_t* format_p = ecma_get_string_from_value (format);
  ecma_value_t exports;

  if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_JS)
      || ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_COMMONJS))
  {
    exports = load_module_exports_from_source (module, loaded.source);
  }
  else if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_SNAPSHOT))
  {
    exports = load_module_exports_from_snapshot (module, loaded.source);
  }
  else
  {
    exports = jjs_throw_sz (JJS_ERROR_TYPE, "Invalid format");
  }

  jjs_annex_module_load_free (&loaded);

  return exports;
} /* load_module_exports */

/**
 * Load a CommonJS module from javascript source code.
 */
static ecma_value_t load_module_exports_from_source (ecma_value_t module, ecma_value_t source)
{
  ecma_value_t filename = ecma_find_own_m (module, LIT_MAGIC_STRING_FILENAME);
  JJS_ASSERT (ecma_is_value_string (filename));

  jjs_parse_options_t parse_opts = {
    .options = JJS_PARSE_HAS_ARGUMENT_LIST | JJS_PARSE_HAS_USER_VALUE | JJS_PARSE_HAS_SOURCE_NAME,
    .argument_list = JJS_CONTEXT (commonjs_args),
    .user_value = filename,
    .source_name = filename,
  };

  jjs_value_t fn = jjs_parse_value (source, &parse_opts);

  if (jjs_value_is_exception(fn))
  {
    ecma_free_value(filename);
    return fn;
  }

  ecma_value_t exports = run_module (module, filename, fn);

  ecma_free_value(filename);
  jjs_value_free(fn);

  return exports;
} /* load_module_exports_from_source */

/**
 * Load module exports from snapshot.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t load_module_exports_from_snapshot (ecma_value_t module, ecma_value_t source)
{
  if (!jjs_value_is_arraybuffer(source))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Invalid snapshot");
  }

  jjs_size_t size = jjs_arraybuffer_size (source);
  uint8_t* buffer = jjs_arraybuffer_data (source);

  if (size == 0 || !buffer)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Empty ArrayBuffer for snapshot");
  }

  ecma_value_t filename = ecma_find_own_m (module, LIT_MAGIC_STRING_FILENAME);
  JJS_ASSERT (ecma_is_value_string (filename));

  jjs_exec_snapshot_option_values_t opts = {
    .source_name = filename,
    .user_value = filename,
  };

  jjs_value_t fn = jjs_exec_snapshot(
    (const uint32_t*) buffer,
    size,
    0,
    JJS_SNAPSHOT_EXEC_ALLOW_STATIC | JJS_SNAPSHOT_EXEC_COPY_DATA | JJS_SNAPSHOT_EXEC_LOAD_AS_FUNCTION
     | JJS_SNAPSHOT_EXEC_HAS_SOURCE_NAME | JJS_SNAPSHOT_EXEC_HAS_USER_VALUE,
    &opts);

  if (jjs_value_is_exception (fn))
  {
    ecma_free_value (filename);
    return fn;
  }

  ecma_value_t exports = run_module (module, filename, fn);

  ecma_free_value(filename);
  jjs_value_free(fn);

  return exports;
} /* load_module_exports_from_snapshot */

/**
 * Run module.
 *
 * @return ecma value
 */
static ecma_value_t run_module (ecma_value_t module, ecma_value_t filename, jjs_value_t fn)
{
  ecma_value_t require = jjs_annex_create_require (filename);

  if (jjs_value_is_exception (require))
  {
    return require;
  }

  ecma_value_t module_dirname = ecma_find_own_m (module, LIT_MAGIC_STRING_PATH);

  if (!ecma_is_value_found (module_dirname))
  {
    ecma_free_value (require);
    return jjs_throw_sz (JJS_ERROR_COMMON, "CommonJS module missing path");
  }

  ecma_value_t exports = jjs_object ();

  ecma_set_m (module, LIT_MAGIC_STRING_EXPORTS, exports);

  ecma_value_t argv[] = {
    module, exports, require, filename, module_dirname,
  };

  jjs_value_t result = jjs_call (fn, ECMA_VALUE_UNDEFINED, argv, sizeof (argv) / sizeof (ecma_value_t));

  ecma_free_value (module_dirname);
  ecma_free_value (exports);
  ecma_free_value (require);

  if (jjs_value_is_exception (result))
  {
    return result;
  }

  jjs_value_free (result);

  return ecma_find_own_m (module, LIT_MAGIC_STRING_EXPORTS);
} /* run_module */

#endif // JJS_ANNEX_COMMONJS
