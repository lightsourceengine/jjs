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

#include "jjs-snapshot.h"

#include "annex.h"
#include "jcontext.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"

#if JJS_COMMONJS

typedef struct
{
  jjs_value_t path;
  jjs_value_t format;
  jjs_value_t result;
} resolve_result_t;

static jjs_value_t create_require_from_directory (jjs_value_t referrer_path);
static void referrer_path_free (void *native_p, struct jjs_object_native_info_t *info_p);
static void resolve_result_free (resolve_result_t* resolve_result_p);
static ecma_value_t create_module (ecma_value_t filename);
static ecma_value_t load_module (ecma_value_t module, ecma_value_t filename, ecma_value_t format);
static ecma_value_t load_module_exports_from_source (ecma_value_t module, ecma_value_t source);
static ecma_value_t load_module_exports_from_snapshot (ecma_value_t module, ecma_value_t source);
static ecma_value_t run_module (ecma_value_t module, ecma_value_t filename, jjs_value_t fn);
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
    ecma_value_t dot = ecma_string_ascii_sz (".");
    path = annex_path_normalize (dot);
    ecma_free_value (dot);
  }
  else if (jjs_value_is_string (referrer))
  {
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
  else
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_EXPECTED_STRING_OR_UNDEFINED));
  }

} /* jjs_annex_create_require */

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
static void referrer_path_free (void *native_p, struct jjs_object_native_info_t *info_p)
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
 * Call the module_on_resolve callback.
 *
 * @param request require request
 * @param referrer_path directory path of the referrer
 * @return resolve_result. must be freed by resolve_result_free.
 */
static resolve_result_t resolve_impl (ecma_value_t request, ecma_value_t referrer_path)
{
  jjs_module_resolve_context_t context = {
    .type = JJS_MODULE_TYPE_COMMONJS,
    .referrer_path = referrer_path,
  };

  jjs_module_resolve_cb_t module_on_resolve_cb = JJS_CONTEXT (module_on_resolve_cb);

  JJS_ASSERT (module_on_resolve_cb != NULL);

  ecma_value_t resolve_result;

  if (module_on_resolve_cb != NULL)
  {
    resolve_result = module_on_resolve_cb (request, &context, JJS_CONTEXT (module_on_resolve_user_p));
  }
  else
  {
    resolve_result = jjs_throw_sz(JJS_ERROR_COMMON, "module_on_resolve callback is not set");
  }

  if (jjs_value_is_exception(resolve_result))
  {
    return (resolve_result_t) {
      .path = ECMA_VALUE_UNDEFINED,
      .format = ECMA_VALUE_UNDEFINED,
      .result = resolve_result,
    };
  }
  // TODO: validate object returned from resolve callback

  return (resolve_result_t) {
    .path = ecma_find_own_m (resolve_result, LIT_MAGIC_STRING_PATH),
    .format = ecma_find_own_m (resolve_result, LIT_MAGIC_STRING_FORMAT),
    .result = resolve_result,
  };
} /* resolve_impl */

/**
 * Free the resolve_result.
 *
 * @param result_p pointer to a resolve result
 */
static void resolve_result_free (resolve_result_t* result_p)
{
  ecma_free_value (result_p->path);
  ecma_free_value (result_p->format);
  ecma_free_value (result_p->result);
} /* resolve_result_free */

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

  ecma_value_t referrer_path = get_referrer_path (call_info_p->function);

  if (ecma_is_value_exception (referrer_path))
  {
    return referrer_path;
  }

  resolve_result_t result = resolve_impl (request, referrer_path);

  ecma_free_value (referrer_path);

  if (jjs_value_is_exception (result.result))
  {
    return result.result;
  }

  ecma_value_t path = result.path;

  result.path = ECMA_VALUE_UNDEFINED;
  resolve_result_free (&result);

  return path;
} /* resolve_handler */

/**
 * Binding for the javascript require() function.
 */
static JJS_HANDLER (require_handler)
{
  /* validate request */
  ecma_value_t request = args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED;

  if (!ecma_is_value_string (request))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Invalid argument");
  }

  /* get referrer path */
  ecma_value_t referrer_path = get_referrer_path (call_info_p->function);

  if (ecma_is_value_exception (referrer_path))
  {
    return referrer_path;
  }

  /* resolve the request to an absolute path */
  resolve_result_t resolved = resolve_impl (request, referrer_path);

  ecma_free_value (referrer_path);

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
    resolve_result_free (&resolved);

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

  resolve_result_free (&resolved);

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

  ecma_free_value (load_module_result);
  ecma_free_value (module);

  return exports;
} /* require_handler */

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
  jjs_module_load_context_t context = {
    .type = JJS_MODULE_TYPE_COMMONJS,
    .format = format,
  };

  // TODO: handle null on_load_cb
  ecma_value_t load_result = JJS_CONTEXT (module_on_load_cb) (filename,
                                                              &context,
                                                              JJS_CONTEXT (module_on_load_user_p));

  if (jjs_value_is_exception (load_result))
  {
    return load_result;
  }

  format = ecma_find_own_m (load_result, LIT_MAGIC_STRING_FORMAT);

  if (format == ECMA_VALUE_NOT_FOUND)
  {
    ecma_free_value (load_result);
    return jjs_throw_sz (JJS_ERROR_TYPE, "Invalid format");
  }

  ecma_string_t* format_p = ecma_get_string_from_value (format);
  ecma_value_t source = ecma_find_own_m (load_result, LIT_MAGIC_STRING_SOURCE);
  ecma_value_t exports;

  if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_JS)
      || ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_COMMONJS))
  {
    exports = load_module_exports_from_source (module, source);
  }
  else if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_SNAPSHOT))
  {
    exports = load_module_exports_from_snapshot (module, source);
  }
  else
  {
    exports = jjs_throw_sz (JJS_ERROR_TYPE, "Invalid format");
  }

  ecma_free_value (source);
  ecma_free_value (format);
  ecma_free_value (load_result);

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
    .options = JJS_PARSE_HAS_ARGUMENT_LIST | JJS_PARSE_HAS_USER_VALUE,
    .argument_list = JJS_CONTEXT (commonjs_args),
    .user_value = filename,
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

  jjs_value_t fn = jjs_exec_snapshot(
    (const uint32_t*) buffer,
    size,
    0,
    JJS_SNAPSHOT_EXEC_ALLOW_STATIC | JJS_SNAPSHOT_EXEC_COPY_DATA | JJS_SNAPSHOT_EXEC_LOAD_AS_FUNCTION,
    NULL);

  if (jjs_value_is_exception (fn))
  {
    return fn;
  }

  ecma_value_t filename = ecma_find_own_m (module, LIT_MAGIC_STRING_FILENAME);
  JJS_ASSERT (ecma_is_value_string (filename));
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

  ecma_value_t module_dirname = annex_path_dirname (filename);

  if (!ecma_is_value_string (module_dirname))
  {
    ecma_free_value (require);
    return module_dirname;
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

#endif // JJS_COMMONJS
