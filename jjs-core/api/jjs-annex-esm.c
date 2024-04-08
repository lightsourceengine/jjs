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

#include "jjs-annex-module-util.h"
#include "jjs-annex-vmod.h"
#include "jjs-annex.h"
#include "jjs-platform.h"

#include "annex.h"
#include "jcontext.h"

#if JJS_ANNEX_ESM

static void
jjs_module_copy_string_property (jjs_value_t target, jjs_value_t source, lit_magic_string_id_t key)
{
  ecma_value_t value = ecma_find_own_m (source, key);

  if (ecma_is_value_string (value))
  {
    ecma_set_m (target, key, value);
  }

  ecma_free_value (value);
} /* jjs_module_copy_string_property */

typedef enum
{
  ESM_RUN_RESULT_EVALUATE,
  ESM_RUN_RESULT_NAMESPACE,
  ESM_RUN_RESULT_NONE,
} esm_result_type_t;

static JJS_HANDLER (esm_resolve_handler);
static jjs_value_t esm_read (jjs_value_t specifier, jjs_value_t referrer_path);
static jjs_value_t esm_import (jjs_value_t specifier, jjs_value_t referrer_path);
static jjs_value_t esm_link_and_evaluate (jjs_value_t module, bool move_module, esm_result_type_t result_type);
static jjs_value_t user_value_to_path (jjs_value_t user_value);
static jjs_value_t esm_link_cb (jjs_value_t specifier, jjs_value_t referrer, void *user_p);
static jjs_value_t
esm_run_source (jjs_esm_source_t *source_p, jjs_value_ownership_t source_values_o, esm_result_type_t result_type);
static void set_module_properties (jjs_value_t module, jjs_value_t filename, jjs_value_t url);
#if JJS_ANNEX_COMMONJS
static jjs_value_t commonjs_module_evaluate_cb (jjs_value_t native_module);
#endif /* JJS_ANNEX_COMMONJS */
#if JJS_ANNEX_VMOD
static jjs_value_t vmod_module_evaluate_cb (jjs_value_t native_module);
static jjs_value_t vmod_get_or_load_module (jjs_value_t specifier, ecma_value_t esm_cache);
#endif /* JJS_ANNEX_VMOD */

static void
esm_source_init (jjs_esm_source_t* esm_source_p)
{
  memset(esm_source_p, 0, sizeof(*esm_source_p));

  esm_source_p->filename = ECMA_VALUE_UNDEFINED;
  esm_source_p->dirname = ECMA_VALUE_UNDEFINED;
  esm_source_p->meta_extension = ECMA_VALUE_UNDEFINED;
  esm_source_p->source_value = ECMA_VALUE_UNDEFINED;
} /* esm_source_init */

#endif /* JJS_ESM */

/**
 * Constructs an empty jjs_esm_source_t object.
 *
 * The resulting object does NOT hold any JS value references that need to be
 * free'd. After this function call, if the caller adds a JS value reference,
 * then the caller will need to clean up manually or via jjs_esm_source_free_values.
 *
 * @return an empty jjs_esm_source_t object
 */
jjs_esm_source_t
jjs_esm_source (void)
{
  jjs_assert_api_enabled ();
#if JJS_ANNEX_ESM
  jjs_esm_source_t source;

  esm_source_init (&source);

  return source;
#else /* !(JJS_ANNEX_ESM) */
  JJS_ASSERT (false);
  jjs_esm_source_t source;

  memset (&source, 0, sizeof (source));

  return source;
#endif /* (JJS_ANNEX_ESM) */
}

/**
 * Fills in a jjs_esm_source object with default values.
 *
 * It is assumes source_p points to garbage memory on the stack or
 * heap. This method will not attempt to free any JS references source_p.
 *
 * The resulting object state does NOT hold any JS value references that need to be
 * free'd. After this function call, if the caller adds a JS value reference,
 * then the caller will need to clean up manually or via jjs_esm_source_free_values.
 *
 * @param source_p pointer to a jjs_esm_source_t object to initialize
 * @return the given source_p pointer
 */
jjs_esm_source_t *
jjs_esm_source_init (jjs_esm_source_t *source_p)
{
  jjs_assert_api_enabled ();
#if JJS_ANNEX_ESM
  JJS_ASSERT (source_p != NULL);
  esm_source_init (source_p);
  return source_p;
#else /* !(JJS_ANNEX_ESM) */
  JJS_UNUSED (source_p);
  JJS_ASSERT (false);
  return source_p;
#endif /* (JJS_ANNEX_ESM) */
}

void
jjs_esm_on_load (jjs_esm_load_cb_t callback_p, void *user_p)
{
  jjs_assert_api_enabled ();

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM
  JJS_CONTEXT (module_on_load_cb) = callback_p;
  JJS_CONTEXT (module_on_load_user_p) = user_p;
#else /* !(JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM) */
  JJS_UNUSED (callback_p);
  JJS_UNUSED (user_p);
#endif /* (JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM) */
} /* jjs_esm_on_load */

void
jjs_esm_on_resolve (jjs_esm_resolve_cb_t callback_p, void *user_p)
{
  jjs_assert_api_enabled ();

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM
  JJS_CONTEXT (module_on_resolve_cb) = callback_p;
  JJS_CONTEXT (module_on_resolve_user_p) = user_p;
#else /* !(JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM) */
  JJS_UNUSED (callback_p);
  JJS_UNUSED (user_p);
#endif /* (JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM) */
} /* jjs_esm_on_resolve */

/**
 * Load hook for CommonJS and ES modules.
 *
 * This hook is responsible for loading a module given a resolved path.
 *
 * @param path resolved path
 * @param context_p load context
 * @param user_p user pointer
 * @return object containing 'source' and 'format'; otherwise, an exception. return value must be freed.
 */
jjs_value_t
jjs_esm_default_on_load_cb (jjs_value_t path, jjs_esm_load_context_t *context_p, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled ();

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM
  ecma_value_t source;
  ecma_string_t *format_p = ecma_get_string_from_value (context_p->format);

  if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_SNAPSHOT))
  {
    source = jjsp_read_file (path, JJS_ENCODING_NONE);
  }
  else if (!ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_NONE))
  {
    source = jjsp_read_file (path, JJS_ENCODING_UTF8);
  }
  else
  {
    source = jjs_throw_sz(JJS_ERROR_TYPE, "load context contains an unsupported format field");
  }

  if (jjs_value_is_exception(source))
  {
    return source;
  }

  ecma_value_t result = ecma_create_object_with_null_proto ();

  ecma_set_m (result, LIT_MAGIC_STRING_SOURCE, source);
  ecma_free_value (source);

  ecma_set_m (result, LIT_MAGIC_STRING_FORMAT, context_p->format);

  return result;
#else /* !(JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM) */
  JJS_UNUSED (path);
  JJS_UNUSED (context_p);

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM */
} /* jjs_esm_default_on_load_cb */

/**
 * Resolve hook for CommonJS and ES modules.
 *
 * This hook resolves a specifier to an absolute path to a module file to load and determines
 * the format of the module.
 *
 * The return object will be passed to on_load hook, which will do the work
 * of reading (maybe transpiling, etc) the module file.
 *
 * The formats supported by the default on_load are 'js', 'commonjs', 'module' and 'snapshot'.
 * If you have a custom on_load hook, you can have custom formats.
 *
 * TODO: information about overloading
 *
 * @param specifier CommonJS request or ESM specifier
 * @param context_p context of the resolve operation
 * @param user_p user pointer passed to jjs_esm_on_resolve
 * @return on success, an object containing 'path' to a module and 'format' of the module
 */
jjs_value_t
jjs_esm_default_on_resolve_cb (jjs_value_t specifier, jjs_esm_resolve_context_t *context_p, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled ();

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM
  ecma_value_t path;

  switch (annex_path_specifier_type (specifier))
  {
    case ANNEX_SPECIFIER_TYPE_RELATIVE:
    {
      path = annex_path_join (context_p->referrer_path, specifier, true);
      break;
    }
    case ANNEX_SPECIFIER_TYPE_ABSOLUTE:
    {
      path = annex_path_normalize (specifier);
      break;
    }
#if JJS_ANNEX_PMAP
    case ANNEX_SPECIFIER_TYPE_PACKAGE:
    {
      path = jjs_annex_pmap_resolve (specifier, context_p->type);
      break;
    }
#endif /* JJS_ANNEX_PMAP */
    default:
    {
      path = ECMA_VALUE_EMPTY;
      break;
    }
  }

  if (jjs_value_is_exception (path))
  {
    return path;
  }

  if (!ecma_is_value_string (path))
  {
    ecma_free_value (path);
    return jjs_throw_sz (JJS_ERROR_COMMON, "failed to resolve path");
  }

  ecma_value_t format = annex_path_format (path);
  ecma_value_t result = ecma_create_object_with_null_proto ();

  ecma_set_m (result, LIT_MAGIC_STRING_PATH, path);
  ecma_free_value (path);

  ecma_set_m (result, LIT_MAGIC_STRING_FORMAT, format);
  ecma_free_value (format);

  return result;
#else /* !(JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM) */
  JJS_UNUSED (specifier);
  JJS_UNUSED (context_p);

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM */
} /* jjs_esm_default_on_resolve_cb */

/**
 * Releases JS reference values inside a jjs_esm_source_t object.
 *
 * This function will zero out the memory of esm_source_p. Any
 * non-jjs_value_t, such as source code buffers, field cleanup
 * should be done before calling this function.
 *
 * @param esm_source_p source object
 */
void
jjs_esm_source_free_values (jjs_esm_source_t *esm_source_p)
{
  JJS_ASSERT (esm_source_p != NULL);

  if (esm_source_p->source_value != 0)
  {
    jjs_value_free (esm_source_p->source_value);
  }

  if (esm_source_p->dirname != 0)
  {
    jjs_value_free (esm_source_p->dirname);
  }

  if (esm_source_p->filename != 0)
  {
    jjs_value_free (esm_source_p->filename);
  }

  if (esm_source_p->meta_extension != 0)
  {
    jjs_value_free (esm_source_p->meta_extension);
  }

  memset (esm_source_p, 0, sizeof (jjs_esm_source_t));
} /* jjs_esm_source_free_values */

/**
 * Import an ES module.
 *
 * The specifier can be a package name, relative path (qualified with
 * ./ or ../) or absolute path. Package names are resolved by the currently
 * set pmap.
 *
 * Note: This import call is synchronous, which is not to the ECMA spec. In the
 *       future, this method may be changed to be asynchronous or deprecated.
 *
 * @param specifier string value of the specifier
 * @param specifier_o specifier reference ownership
 * @return the namespace object of the module. on error, an exception is
 * returned. return value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_esm_import (jjs_value_t specifier, jjs_value_ownership_t specifier_o)
{
  jjs_assert_api_enabled ();
#if JJS_ANNEX_ESM
  jjs_value_t referrer_path = annex_path_cwd ();

  if (!jjs_value_is_string (referrer_path))
  {
    JJS_DISOWN (specifier, specifier_o);
    return jjs_throw_sz (JJS_ERROR_COMMON, "Failed to get current working directory");
  }

  jjs_value_t module = esm_import (specifier, referrer_path);

  jjs_value_free (referrer_path);

  JJS_DISOWN (specifier, specifier_o);

  if (jjs_value_is_exception (module))
  {
    return module;
  }

  jjs_value_t namespace = jjs_module_namespace (module);

  jjs_value_free (module);

  return namespace;
#else /* !JJS_ANNEX_ESM */
  JJS_DISOWN (specifier, specifier_o);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif
} /* jjs_esm_import */

/**
 * Import an ES module.
 *
 * @see jjs_esm_import
 *
 * @param specifier_sz string value of the specifier. if NULL, an empty string is used.
 * @return the namespace object of the module. on error, an exception is returned. return
 * value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_esm_import_sz (const char *specifier_p)
{
  jjs_assert_api_enabled ();
  return jjs_esm_import (annex_util_create_string_utf8_sz (specifier_p), JJS_MOVE);
} /* jjs_esm_import_sz */

/**
 * import a module from in-memory source.
 *
 * @param source_p UTF-8 encoded module source
 * @param source_len size of source_p in bytes (not encoded characters)
 * @param options_p configuration options for the new module
 * @return the namespace of the imported module or an exception on failure to import the
 * module. the return value must be release with jjs_value_free
 */
jjs_value_t
jjs_esm_import_source (jjs_esm_source_t *source_p, jjs_value_ownership_t source_values_o)
{
  jjs_assert_api_enabled ();
#if JJS_ANNEX_ESM
  return esm_run_source (source_p, source_values_o, ESM_RUN_RESULT_NAMESPACE);
#else /* !JJS_ANNEX_ESM */
  if (source_values_o == JJS_MOVE)
  {
    jjs_esm_source_free_values (source_p);
  }
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_ESM */
} /* jjs_esm_import_source */

/**
 * Evaluate an ES module.
 *
 * Imports a module, but instead of returning the namespace object, it returns
 * the evaluation result of the module itself. This should not be generally
 * used. It exists to support the cmd line program use case.
 *
 * The specifier can be a package name, relative path (qualified with
 * ./ or ../) or absolute path. Package names are resolved by the currently
 * set pmap.
 *
 * Note: This import call is synchronous, which is not to the ECMA spec. In the
 *       future, this method may be changed to be asynchronous or deprecated.
 *
 * Note: This method will not work with cached modules. A module can only be
 *       evaluated once!
 *
 * @param specifier string value of the specifier
 * @param specifier_o specifier reference ownership
 * @return the evaluation result of the module. on error, an exception is
 * returned. return value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_esm_evaluate (jjs_value_t specifier, jjs_value_ownership_t specifier_o)
{
  jjs_assert_api_enabled ();
#if JJS_ANNEX_ESM
  jjs_value_t referrer_path = annex_path_cwd ();

  if (!jjs_value_is_string (referrer_path))
  {
    JJS_DISOWN (specifier, specifier_o);
    return jjs_throw_sz (JJS_ERROR_COMMON, "Failed to get current working directory");
  }

  jjs_value_t module = esm_read (specifier, referrer_path);

  jjs_value_free (referrer_path);

  JJS_DISOWN (specifier, specifier_o);

  return esm_link_and_evaluate (module, true, ESM_RUN_RESULT_EVALUATE);
#else /* !JJS_ANNEX_ESM */
  JJS_DISOWN (specifier, specifier_o);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_ESM */
} /* jjs_esm_evaluate */

/**
 * Evaluate an ES module.
 *
 * @see jjs_esm_evaluate
 *
 * @param specifier string value of the specifier. if NULL, an empty string is used.
 * @return the evaluation result of the module. on error, an exception is returned. return
 * value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_esm_evaluate_sz (const char *specifier_p)
{
  jjs_assert_api_enabled ();
  return jjs_esm_evaluate (annex_util_create_string_utf8_sz (specifier_p), JJS_MOVE);
} /* jjs_esm_evaluate_sz */

/**
 * Evaluate a module from source.
 *
 * @param source_p UTF-8 encoded module source
 * @param source_len size of source_p in bytes (not encoded characters)
 * @param options_p configuration options for the new module
 * @return the evaluation result of the module or an exception on failure to evaluate the
 * module. the return value must be release with jjs_value_free
 */
jjs_value_t
jjs_esm_evaluate_source (jjs_esm_source_t *source_p, jjs_value_ownership_t source_values_o)
{
  jjs_assert_api_enabled ();
#if JJS_ANNEX_ESM
  return esm_run_source (source_p, source_values_o, ESM_RUN_RESULT_EVALUATE);
#else /* !JJS_ANNEX_ESM */
  if (source_values_o == JJS_MOVE)
  {
    jjs_esm_source_free_values (source_p);
  }
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_ESM */
} /* jjs_esm_evaluate_source */

jjs_value_t
jjs_esm_default_on_import_cb (jjs_value_t specifier, jjs_value_t user_value, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled ();
#if JJS_ANNEX_ESM
  jjs_value_t referrer_path = user_value_to_path (user_value);

  if (!jjs_value_is_string (referrer_path))
  {
    jjs_value_free (referrer_path);
    return jjs_throw_sz (JJS_ERROR_COMMON, "Failed to get referrer path from user_value");
  }

  jjs_value_t module = esm_import (specifier, referrer_path);

  jjs_value_free (referrer_path);

  return module;
#else /* !JJS_ANNEX_ESM */
  JJS_UNUSED (specifier);
  JJS_UNUSED (user_value);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_ESM */
} /* jjs_esm_default_on_import_cb */

void
jjs_esm_default_on_import_meta_cb (jjs_value_t module, jjs_value_t meta_object, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled ();
#if JJS_ANNEX_ESM
  jjs_module_copy_string_property (meta_object, module, LIT_MAGIC_STRING_URL);
  jjs_module_copy_string_property (meta_object, module, LIT_MAGIC_STRING_FILENAME);
  jjs_module_copy_string_property (meta_object, module, LIT_MAGIC_STRING_DIRNAME);

  jjs_value_t resolve = jjs_function_external (esm_resolve_handler);
  jjs_value_t dirname = ecma_find_own_m (module, LIT_MAGIC_STRING_DIRNAME);
  jjs_value_t path = ecma_make_magic_string_value(LIT_MAGIC_STRING_PATH);

  jjs_object_set_internal (resolve, path, dirname);
  ecma_set_m (meta_object, LIT_MAGIC_STRING_RESOLVE, resolve);

  jjs_value_free (path);
  jjs_value_free (dirname);
  jjs_value_free (resolve);

  ecma_value_t extension = ecma_find_own_m (module, LIT_MAGIC_STRING_EXTENSION);

  if (extension != ECMA_VALUE_NOT_FOUND)
  {
    ecma_set_m (meta_object, LIT_MAGIC_STRING_EXTENSION, extension);
    ecma_free_value (extension);
  }
#else /* !JJS_ANNEX_ESM */
  JJS_UNUSED (module);
  JJS_UNUSED (meta_object);
#endif /* JJS_ANNEX_ESM */
} /* jjs_module_default_import_meta */

#if JJS_ANNEX_ESM

static JJS_HANDLER (esm_resolve_handler)
{
  jjs_value_t specifier = args_count > 0 ? args_p[0] : jjs_undefined ();

  if (!jjs_value_is_string (specifier))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Invalid argument");
  }

#if JJS_ANNEX_VMOD
  if (jjs_vmod_exists (specifier, JJS_KEEP))
  {
    return ecma_copy_value (specifier);
  }
#endif

  ecma_value_t referrer_path = annex_util_get_internal_m (call_info_p->function, LIT_MAGIC_STRING_PATH);

  if (!jjs_value_is_string (referrer_path))
  {
    jjs_value_free (referrer_path);
    return jjs_throw_sz (JJS_ERROR_COMMON, "resolve is missing referrer path");
  }

  jjs_annex_module_resolve_t resolved = jjs_annex_module_resolve (specifier, referrer_path, JJS_MODULE_TYPE_MODULE);
  jjs_value_t result;

  if (jjs_value_is_exception (resolved.result))
  {
    result = jjs_value_copy (resolved.result);
  }
  else
  {
    result = jjs_value_copy (resolved.path);
  }

  jjs_value_free (referrer_path);
  jjs_annex_module_resolve_free (&resolved);

  return result;
}

static jjs_value_t
esm_import (jjs_value_t specifier, jjs_value_t referrer_path)
{
  jjs_value_t module = esm_read (specifier, referrer_path);
  jjs_value_t result = esm_link_and_evaluate (module, false, ESM_RUN_RESULT_NONE);

  if (jjs_value_is_exception (result))
  {
    jjs_value_free (module);
    return result;
  }

  jjs_value_free (result);

  return module;
} /* esm_import */

static jjs_value_t
esm_realpath_dirname (jjs_value_t dirname_value)
{
  if (dirname_value == 0 || ecma_is_value_undefined (dirname_value))
  {
    return annex_path_cwd ();
  }

  return annex_path_normalize (dirname_value);
} /* esm_realpath_dirname */

static jjs_value_t
esm_basename_or_default (jjs_value_t filename_value)
{
  if (filename_value == 0 || ecma_is_value_undefined (filename_value))
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING_ESM_FILENAME_DEFAULT);
  }

  return annex_path_basename (filename_value);
} /* esm_basename_or_default */

static jjs_value_t
esm_link_and_evaluate (jjs_value_t module, bool move_module, esm_result_type_t result_type)
{
  jjs_module_state_t state;
  jjs_value_t result;

  if (jjs_value_is_exception (module))
  {
    return move_module ? module : jjs_value_copy (module);
  }

  state = jjs_module_state (module);

  if (state == JJS_MODULE_STATE_UNLINKED)
  {
    jjs_value_t link_result = jjs_module_link (module, esm_link_cb, NULL);

    if (jjs_value_is_exception (link_result))
    {
      result = link_result;
      goto done;
    }

    JJS_ASSERT (jjs_value_is_true (link_result));
    jjs_value_free (link_result);
  }

  state = jjs_module_state (module);

  if (state == JJS_MODULE_STATE_LINKED)
  {
    result = jjs_module_evaluate (module);
  }
  else if (state == JJS_MODULE_STATE_EVALUATED)
  {
    result = ECMA_VALUE_UNDEFINED;
  }
  else
  {
    result = jjs_throw_sz (JJS_ERROR_COMMON, "module must be in linked state to evaluate");
  }

done:
  if (!jjs_value_is_exception (result))
  {
    switch (result_type)
    {
      case ESM_RUN_RESULT_NAMESPACE:
        jjs_value_free (result);
        result = jjs_module_namespace (module);
        break;
      case ESM_RUN_RESULT_EVALUATE:
        break;
      case ESM_RUN_RESULT_NONE:
        jjs_value_free (result);
        result = ECMA_VALUE_UNDEFINED;
        break;
      default:
        jjs_value_free (result);
        result = jjs_throw_sz (JJS_ERROR_COMMON, "invalid result_type");
        break;
    }
  }

  if (move_module)
  {
    jjs_value_free (module);
  }

  return result;
} /* esm_link_and_evaluate */

static jjs_value_t
esm_run_source (jjs_esm_source_t *source_p, jjs_value_ownership_t source_values_o, esm_result_type_t result_type)
{
  if (source_p == NULL)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "source_p must not be NULL");
  }

  bool has_source_sz = source_p->source_sz != NULL;
  bool has_source_value = source_p->source_value != 0 ? jjs_value_is_string (source_p->source_value) : false;
  bool has_source_buffer = source_p->source_buffer_size != 0;
  jjs_value_t validation;

  if ((has_source_sz && !has_source_value && !has_source_buffer)
      || (has_source_value && !has_source_sz && !has_source_buffer)
      || (has_source_buffer && !has_source_value && !has_source_sz))
  {
    validation = jjs_undefined ();
  }
  else if (!has_source_sz && !has_source_value && !has_source_buffer)
  {
    validation = jjs_throw_sz (JJS_ERROR_TYPE, "source_p contains no sources");
  }
  else
  {
    validation =
      jjs_throw_sz (JJS_ERROR_TYPE,
                    "source_p contains multiple sources. Use only one of source_value, source_sz or source_buffer.");
  }

  if (jjs_value_is_exception (validation))
  {
    if (source_values_o == JJS_MOVE)
    {
      jjs_esm_source_free_values (source_p);
    }

    return validation;
  }

  jjs_value_free (validation);

  ecma_value_t esm_cache = ecma_get_global_object ()->esm_cache;
  jjs_parse_options_t parse_options;
  jjs_value_t module;
  jjs_value_t basename_value = ECMA_VALUE_UNDEFINED;
  jjs_value_t filename_value = ECMA_VALUE_UNDEFINED;
  jjs_value_t dirname_value;

  dirname_value = esm_realpath_dirname (source_p->dirname);

  if (!jjs_value_is_string (dirname_value))
  {
    module = jjs_throw_sz (JJS_ERROR_TYPE, "jjs_source_options_t.dirname must be a path to an fs directory");
    goto after_parse;
  }

  basename_value = esm_basename_or_default (source_p->filename);

  if (!jjs_value_is_string (basename_value))
  {
    module = jjs_throw_sz (JJS_ERROR_TYPE, "jjs_source_options_t.filename must be a normal filename");
    goto after_parse;
  }

  filename_value = annex_path_join (dirname_value, basename_value, false);

  if (!jjs_value_is_string (filename_value))
  {
    module = jjs_throw_sz (JJS_ERROR_TYPE, "Failed to create filename path to source module.");
    goto after_parse;
  }

  if (ecma_has_own_v (esm_cache, filename_value))
  {
    module = jjs_throw_sz (JJS_ERROR_TYPE, "A module with this filename has already been loaded.");
    goto after_parse;
  }

  parse_options = (jjs_parse_options_t){
    .options = JJS_PARSE_MODULE | JJS_PARSE_HAS_USER_VALUE | JJS_PARSE_HAS_SOURCE_NAME | JJS_PARSE_HAS_START,
    .start_column = source_p->start_column,
    .start_line = source_p->start_line,
    .user_value = filename_value,
    .source_name = basename_value,
  };

  if (has_source_buffer)
  {
    module = jjs_parse (source_p->source_buffer_p, source_p->source_buffer_size, &parse_options);
  }
  else if (has_source_sz)
  {
    module = jjs_parse ((const jjs_char_t *) source_p->source_sz, strlen (source_p->source_sz), &parse_options);
  }
  else
  {
    module = jjs_parse_value (source_p->source_value, &parse_options);
  }

  if (!jjs_value_is_exception (module))
  {
    jjs_value_t file_url = annex_path_to_file_url (filename_value);

    JJS_ASSERT (ecma_is_value_string (file_url));

    if (!ecma_is_value_string (file_url))
    {
      file_url = ECMA_VALUE_UNDEFINED;
    }

    ecma_set_m (module, LIT_MAGIC_STRING_DIRNAME, dirname_value);
    ecma_set_m (module, LIT_MAGIC_STRING_URL, file_url);
    ecma_set_m (module, LIT_MAGIC_STRING_FILENAME, filename_value);

    if (!ecma_is_value_undefined (source_p->meta_extension))
    {
      ecma_set_m (module, LIT_MAGIC_STRING_EXTENSION, source_p->meta_extension);
    }

    if (source_p->cache)
    {
      ecma_set_v (esm_cache, filename_value, module);
    }

    jjs_value_free (file_url);
  }

after_parse:
  jjs_value_free (filename_value);
  jjs_value_free (basename_value);
  jjs_value_free (dirname_value);

  if (source_values_o == JJS_MOVE)
  {
    jjs_esm_source_free_values (source_p);
  }

  return esm_link_and_evaluate (module, true, result_type);
} /* esm_run_source */

static jjs_value_t
esm_read (jjs_value_t specifier, jjs_value_t referrer_path)
{
  ecma_value_t esm_cache = ecma_get_global_object ()->esm_cache;

#if JJS_ANNEX_VMOD
  if (jjs_annex_vmod_exists (specifier))
  {
    return vmod_get_or_load_module (specifier, esm_cache);
  }
#endif /* JJS_ANNEX_VMOD */

  // resolve specifier
  jjs_annex_module_resolve_t resolved = jjs_annex_module_resolve (specifier, referrer_path, JJS_MODULE_TYPE_MODULE);

  if (jjs_value_is_exception (resolved.result))
  {
    jjs_value_t resolved_exception = jjs_value_copy (resolved.result);
    jjs_annex_module_resolve_free (&resolved);
    return resolved_exception;
  }

  ecma_value_t cached_module = ecma_find_own_v (esm_cache, resolved.path);

  if (cached_module != ECMA_VALUE_NOT_FOUND)
  {
    jjs_annex_module_resolve_free (&resolved);
    return cached_module;
  }

  ecma_free_value (cached_module);

  // load source
  jjs_annex_module_load_t loaded = jjs_annex_module_load (resolved.path, resolved.format, JJS_MODULE_TYPE_MODULE);

  if (jjs_value_is_exception (loaded.result))
  {
    jjs_annex_module_resolve_free (&resolved);
    return loaded.result;
  }

  ecma_string_t *format_p = ecma_get_string_from_value (loaded.format);
  jjs_value_t module;

  if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_JS)
      || ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_MODULE))
  {
    jjs_parse_options_t opts = {
      .options = JJS_PARSE_MODULE | JJS_PARSE_HAS_USER_VALUE | JJS_PARSE_HAS_SOURCE_NAME,
      .user_value = resolved.path,
      .source_name = resolved.path,
    };

    module = jjs_parse_value (loaded.source, &opts);

    if (!jjs_value_is_exception (module))
    {
      jjs_value_t file_url = annex_path_to_file_url (resolved.path);

      if (jjs_value_is_string (file_url))
      {
        set_module_properties (module, resolved.path, file_url);
      }
      else
      {
        jjs_value_free (module);
        module = jjs_throw_sz (JJS_ERROR_COMMON, "failed to convert path to file url");
      }

      jjs_value_free (file_url);
    }
  }
#if JJS_ANNEX_COMMONJS
  else if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_COMMONJS))
  {
    jjs_value_t default_name = ecma_make_magic_string_value (LIT_MAGIC_STRING_DEFAULT);
    jjs_value_t file_url = annex_path_to_file_url (resolved.path);

    // TODO: get export names from pmap?

    JJS_ASSERT (ecma_is_value_string (file_url));

    if (!ecma_is_value_string (file_url))
    {
      file_url = ECMA_VALUE_UNDEFINED;
    }

    module = jjs_synthetic_module (commonjs_module_evaluate_cb, &default_name, 1);
    set_module_properties (module, resolved.path, file_url);

    jjs_value_free (default_name);
    jjs_value_free (file_url);
  }
#endif /* JJS_ANNEX_COMMONJS */
  else
  {
    module = jjs_throw_sz (JJS_ERROR_TYPE, "Invalid format");
  }

  if (!jjs_value_is_exception (module))
  {
    ecma_set_v (esm_cache, resolved.path, module);
  }

  jjs_annex_module_resolve_free (&resolved);
  jjs_annex_module_load_free (&loaded);

  return module;
} /* esm_read */

static jjs_value_t
esm_link_cb (jjs_value_t specifier, jjs_value_t referrer, void *user_p)
{
  JJS_UNUSED (user_p);

  jjs_value_t path = ecma_find_own_m (referrer, LIT_MAGIC_STRING_DIRNAME);
  jjs_value_t module = esm_read (specifier, path);

  jjs_value_free (path);

  return module;
} /* esm_link_cb */

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_VMOD

/**
 * Sets the default export of a synthetic/native ES module.
 *
 * If exports contains a 'default' key, exports.default will be used as
 * default. Otherwise, exports will be used as default.
 *
 * @param native_module synthetic/native ES module
 * @param exports CommonJS or Virtual Module exports object
 * @return true if successful, exception otherwise. the return value must be
 * freed with jjs_value_free.
 */
static jjs_value_t
module_native_set_default (jjs_value_t native_module, jjs_value_t exports)
{
  jjs_value_t default_name = ecma_make_magic_string_value (LIT_MAGIC_STRING_DEFAULT);
  ecma_value_t default_value = ecma_find_own_v (exports, default_name);
  jjs_value_t result = jjs_synthetic_module_set_export (native_module,
                                                        default_name,
                                                        ecma_is_value_found (default_value) ? default_value : exports);

  jjs_value_free (default_name);
  ecma_free_value (default_value);

  return result;
} /* module_native_set_default */

#endif /* JJS_ANNEX_COMMONJS || JJS_ANNEX_VMOD */

#if JJS_ANNEX_COMMONJS

static jjs_value_t
commonjs_module_evaluate_cb (jjs_value_t native_module)
{
  jjs_value_t filename = ecma_find_own_m (native_module, LIT_MAGIC_STRING_FILENAME);
  JJS_ASSERT (jjs_value_is_string (filename));
  jjs_value_t referrer_path = ecma_find_own_m (native_module, LIT_MAGIC_STRING_DIRNAME);
  JJS_ASSERT (jjs_value_is_string (referrer_path));

  jjs_value_t exports = jjs_annex_require (filename, referrer_path);

  jjs_value_free (filename);
  jjs_value_free (referrer_path);

  if (jjs_value_is_exception (exports))
  {
    return exports;
  }

  jjs_value_t result = module_native_set_default (native_module, exports);

  jjs_value_free (exports);

  return result;
} /* commonjs_module_evaluate_cb */

#endif /* JJS_ANNEX_COMMONJS */

#if JJS_ANNEX_VMOD

static jjs_value_t
vmod_module_evaluate_cb (jjs_value_t native_module)
{
  ecma_value_t exports = ecma_find_own_m (native_module, LIT_MAGIC_STRING_EXPORTS);

  JJS_ASSERT (ecma_is_value_found (exports));

  if (!ecma_is_value_found (exports))
  {
    return jjs_throw_sz (JJS_ERROR_COMMON, "vmod esm module missing exports property");
  }

  ecma_value_t delete_result = ecma_op_object_delete (ecma_get_object_from_value (native_module),
                                                      ecma_get_magic_string (LIT_MAGIC_STRING_EXPORTS),
                                                      false);

  ecma_free_value (delete_result);

  jjs_value_t result = module_native_set_default (native_module, exports);

  jjs_value_free (exports);

  return result;
} /* vmod_module_evaluate_cb */

static jjs_value_t
vmod_link (jjs_value_t module, jjs_value_t exports, ecma_collection_t *keys_p, bool was_default_appended)
{
  uint32_t count = keys_p->item_count - (was_default_appended ? 1 : 0);

  for (uint32_t i = 0; i < count; i++)
  {
    ecma_value_t value = ecma_find_own_v (exports, keys_p->buffer_p[i]);

    JJS_ASSERT (value != ECMA_VALUE_NOT_FOUND);

    if (value == ECMA_VALUE_NOT_FOUND)
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, "failed to get export value while linking vmod module");
    }

    jjs_value_t result = jjs_synthetic_module_set_export (module, keys_p->buffer_p[i], value);

    ecma_free_value (value);

    if (jjs_value_is_exception (result))
    {
      return result;
    }

    jjs_value_free (result);
  }

  if (was_default_appended)
  {
    ecma_value_t default_key = ecma_make_magic_string_value (LIT_MAGIC_STRING_DEFAULT);
    jjs_value_t result = jjs_synthetic_module_set_export (module, default_key, exports);

    ecma_free_value (default_key);

    if (jjs_value_is_exception (result))
    {
      return result;
    }

    jjs_value_free (result);
  }

  return jjs_module_link (module, esm_link_cb, NULL);
} /* vmod_link */

static jjs_value_t
vmod_get_or_load_module (jjs_value_t specifier, ecma_value_t esm_cache)
{
  ecma_value_t cached = ecma_find_own_v (esm_cache, specifier);

  if (ecma_is_value_found (cached))
  {
    return cached;
  }

  ecma_free_value (cached);

  jjs_value_t exports = jjs_annex_vmod_resolve (specifier);

  if (jjs_value_is_exception (exports))
  {
    return exports;
  }

  ecma_collection_t *keys_p;

  if (ecma_is_value_object (exports))
  {
    keys_p = ecma_op_object_get_enumerable_property_names (ecma_get_object_from_value (exports),
                                                           ECMA_ENUMERABLE_PROPERTY_KEYS);

#if JJS_BUILTIN_PROXY
    if (JJS_UNLIKELY (keys_p == NULL))
    {
      return ecma_create_exception_from_context ();
    }
#endif /* JJS_BUILTIN_PROXY */
  }
  else
  {
    keys_p = ecma_new_collection ();

    if (JJS_UNLIKELY (keys_p == NULL))
    {
      return jjs_throw_sz (JJS_ERROR_COMMON, "failed to allocate collection for vmod keys");
    }
  }

  bool was_default_appended;

  if (keys_p->item_count == 0 || !ecma_has_own_m (exports, LIT_MAGIC_STRING_DEFAULT))
  {
    ecma_collection_push_back (keys_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_DEFAULT));
    was_default_appended = true;
  }
  else
  {
    was_default_appended = false;
  }

  jjs_value_t native_module = jjs_synthetic_module (vmod_module_evaluate_cb, keys_p->buffer_p, keys_p->item_count);

  if (!jjs_value_is_exception (native_module))
  {
    jjs_value_t linked = vmod_link (native_module, exports, keys_p, was_default_appended);

    if (!jjs_value_is_exception (linked))
    {
      jjs_value_free (linked);
      ecma_set_m (native_module, LIT_MAGIC_STRING_EXPORTS, exports);
      ecma_set_v (esm_cache, specifier, native_module);
    }
    else
    {
      jjs_value_free (native_module);
      native_module = linked;
    }
  }

  ecma_collection_free (keys_p);
  jjs_value_free (exports);

  return native_module;
} /* vmod_get_or_load_module */

#endif /* JJS_ANNEX_VMOD */

static jjs_value_t
user_value_to_path (jjs_value_t user_value)
{
  annex_specifier_type_t path_type = annex_path_specifier_type (user_value);
  jjs_value_t result;

  if (path_type == ANNEX_SPECIFIER_TYPE_ABSOLUTE)
  {
    jjs_value_t module = ecma_find_own_v (ecma_get_global_object ()->esm_cache, user_value);

    result = ecma_is_value_found (module) ? ecma_find_own_m (module, LIT_MAGIC_STRING_DIRNAME)
                                          : annex_path_dirname (user_value);

    jjs_value_free (module);
  }
  else if (path_type == ANNEX_SPECIFIER_TYPE_FILE_URL)
  {
    result = jjs_throw_sz (JJS_ERROR_COMMON, "user_value cannot be a file url");
  }
  else
  {
    // if no absolute path, ignore user_value contents and use the cwd.
    //
    // when using jjs_parse, the caller may forget to set user_value, they need to contrive a fake absolute
    // path (for parsing an in mem string) or the absolute path needs to be built. if user_value is not set,
    // cwd is a reasonable default value for most use cases.
    result = annex_path_cwd ();
  }

  return result;
} /* user_value_to_path */

static void
set_module_properties (jjs_value_t module, jjs_value_t filename, jjs_value_t url)
{
  if (jjs_value_is_exception (module))
  {
    return;
  }

  jjs_value_t path_dirname = annex_path_dirname (filename);

  JJS_ASSERT (jjs_value_is_string (path_dirname));

  ecma_set_m (module, LIT_MAGIC_STRING_DIRNAME, path_dirname);
  ecma_set_m (module, LIT_MAGIC_STRING_URL, url);
  ecma_set_m (module, LIT_MAGIC_STRING_FILENAME, filename);

  jjs_value_free (path_dirname);
} /* set_module_properties */

#endif /* JJS_ANNEX_ESM */
