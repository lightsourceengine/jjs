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
#include "jjs-util.h"

#include "annex.h"
#include "jcontext.h"

#if JJS_ANNEX_ESM

static void
jjs_module_copy_string_property (jjs_context_t *context_p, jjs_value_t target, jjs_value_t source, lit_magic_string_id_t key)
{
  ecma_value_t value = ecma_find_own_m (context_p, source, key);

  if (ecma_is_value_string (value))
  {
    ecma_set_m (context_p, target, key, value);
  }

  ecma_free_value (context_p, value);
} /* jjs_module_copy_string_property */

typedef enum
{
  ESM_RUN_RESULT_EVALUATE,
  ESM_RUN_RESULT_NAMESPACE,
  ESM_RUN_RESULT_NONE,
} esm_result_type_t;

static JJS_HANDLER (esm_resolve_handler);
static jjs_value_t esm_read (jjs_context_t* context_p, jjs_value_t specifier, jjs_value_t referrer_path);
static jjs_value_t esm_import (jjs_context_t* context_p, jjs_value_t specifier, jjs_value_t referrer_path);
static jjs_value_t esm_link_and_evaluate (jjs_context_t* context_p, jjs_value_t module, bool move_module, esm_result_type_t result_type);
static jjs_value_t user_value_to_path (jjs_context_t* context_p, jjs_value_t user_value);
static jjs_value_t esm_link_cb (jjs_context_t* context_p, jjs_value_t specifier, jjs_value_t referrer, void *user_p);
static jjs_value_t
esm_run_source (jjs_context_t* context_p,
                const jjs_esm_source_options_t *options_p,
                const jjs_char_t *source_p,
                jjs_size_t source_size,
                jjs_value_t source_value,
                esm_result_type_t result_type);
static void set_module_properties (jjs_context_t* context_p, jjs_value_t module, jjs_value_t filename, jjs_value_t url);
#if JJS_ANNEX_COMMONJS
static jjs_value_t commonjs_module_evaluate_cb (jjs_context_t* context_p, jjs_value_t native_module);
#endif /* JJS_ANNEX_COMMONJS */
#if JJS_ANNEX_VMOD
static jjs_value_t vmod_module_evaluate_cb (jjs_context_t* context_p, jjs_value_t native_module);
static jjs_value_t vmod_get_or_load_module (jjs_context_t* context_p, jjs_value_t specifier, ecma_value_t esm_cache);
#endif /* JJS_ANNEX_VMOD */

#endif /* JJS_ESM */

void
jjs_esm_on_load (jjs_context_t* context_p, jjs_esm_load_cb_t callback_p, void *user_p)
{
  jjs_assert_api_enabled (context_p);

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM
  context_p->module_on_load_cb = callback_p;
  context_p->module_on_load_user_p = user_p;
#else /* !(JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM) */
  JJS_UNUSED (callback_p);
  JJS_UNUSED (user_p);
#endif /* (JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM) */
} /* jjs_esm_on_load */

void
jjs_esm_on_resolve (jjs_context_t* context_p, jjs_esm_resolve_cb_t callback_p, void *user_p)
{
  jjs_assert_api_enabled (context_p);

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM
  context_p->module_on_resolve_cb = callback_p;
  context_p->module_on_resolve_user_p = user_p;
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
 * @param context_p JJS context
 * @param path resolved path
 * @param load_context_p load context
 * @param user_p user pointer
 * @return object containing 'source' and 'format'; otherwise, an exception. return value must be freed.
 */
jjs_value_t
jjs_esm_default_on_load_cb (jjs_context_t* context_p, jjs_value_t path, jjs_esm_load_context_t *load_context_p, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled (context_p);

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM
  ecma_value_t source;
  ecma_string_t *format_p = ecma_get_string_from_value (context_p, load_context_p->format);

  if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_SNAPSHOT))
  {
    jjs_platform_read_file_options_t options = {
      .encoding = JJS_ENCODING_NONE,
    };

    source = jjs_platform_read_file (context_p, path, JJS_KEEP, &options);
  }
  else if (!ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_NONE))
  {
    jjs_platform_read_file_options_t options = {
      .encoding = JJS_ENCODING_UTF8,
    };

    source = jjs_platform_read_file (context_p, path, JJS_KEEP, &options);
  }
  else
  {
    source = jjs_throw_sz (context_p, JJS_ERROR_TYPE, "load context contains an unsupported format field");
  }

  if (jjs_value_is_exception (context_p, source))
  {
    return source;
  }

  ecma_value_t result = ecma_create_object_with_null_proto (context_p);

  ecma_set_m (context_p, result, LIT_MAGIC_STRING_SOURCE, source);
  ecma_free_value (context_p, source);

  ecma_set_m (context_p, result, LIT_MAGIC_STRING_FORMAT, load_context_p->format);

  return result;
#else /* !(JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM) */
  JJS_UNUSED_ALL (path, load_context_p);

  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
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
 * @param context_p JJS context
 * @param specifier CommonJS request or ESM specifier
 * @param resolve_context_p context of the resolve operation
 * @param user_p user pointer passed to jjs_esm_on_resolve
 * @return on success, an object containing 'path' to a module and 'format' of the module
 */
jjs_value_t
jjs_esm_default_on_resolve_cb (jjs_context_t* context_p, jjs_value_t specifier, jjs_esm_resolve_context_t *resolve_context_p, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled (context_p);

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM
  ecma_value_t path;

  switch (annex_path_specifier_type (context_p, specifier))
  {
    case ANNEX_SPECIFIER_TYPE_RELATIVE:
    {
      path = annex_path_join (context_p, resolve_context_p->referrer_path, specifier, true);
      break;
    }
    case ANNEX_SPECIFIER_TYPE_ABSOLUTE:
    {
      path = annex_path_normalize (context_p, specifier);
      break;
    }
#if JJS_ANNEX_PMAP
    case ANNEX_SPECIFIER_TYPE_PACKAGE:
    {
      path = jjs_annex_pmap_resolve (context_p, specifier, resolve_context_p->type);
      break;
    }
#endif /* JJS_ANNEX_PMAP */
    default:
    {
      path = ECMA_VALUE_EMPTY;
      break;
    }
  }

  if (jjs_value_is_exception (context_p, path))
  {
    return path;
  }

  if (!ecma_is_value_string (path))
  {
    ecma_free_value (context_p, path);
    return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "failed to resolve path");
  }

  ecma_value_t format = annex_path_format (context_p, path);
  ecma_value_t result = ecma_create_object_with_null_proto (context_p);

  ecma_set_m (context_p, result, LIT_MAGIC_STRING_PATH, path);
  ecma_free_value (context_p, path);

  ecma_set_m (context_p, result, LIT_MAGIC_STRING_FORMAT, format);
  ecma_free_value (context_p, format);

  return result;
#else /* !(JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM) */
  JJS_UNUSED_ALL (specifier, context_p, resolve_context_p);

  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM */
} /* jjs_esm_default_on_resolve_cb */

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
 * @param context_p JJS context
 * @param specifier string value of the specifier
 * @param specifier_o specifier reference ownership
 * @return the namespace object of the module. on error, an exception is
 * returned. return value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_esm_import (jjs_context_t* context_p, jjs_value_t specifier, jjs_own_t specifier_o)
{
  jjs_assert_api_enabled (context_p);
#if JJS_ANNEX_ESM
  jjs_value_t referrer_path = annex_path_cwd (context_p);

  if (!jjs_value_is_string (context_p, referrer_path))
  {
    jjs_disown_value (context_p, specifier, specifier_o);
    return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "Failed to get current working directory");
  }

  jjs_value_t module = esm_import (context_p, specifier, referrer_path);

  jjs_value_free (context_p, referrer_path);

  jjs_disown_value (context_p, specifier, specifier_o);

  if (jjs_value_is_exception (context_p, module))
  {
    return module;
  }

  jjs_value_t namespace = jjs_module_namespace (context_p, module);

  jjs_value_free (context_p, module);

  return namespace;
#else /* !JJS_ANNEX_ESM */
  jjs_disown_value (context_p, specifier, specifier_o);
  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif
} /* jjs_esm_import */

/**
 * Import an ES module.
 *
 * @see jjs_esm_import
 *
 * @param context_p JJS context
 * @param specifier_sz string value of the specifier. if NULL, an empty string is used.
 * @return the namespace object of the module. on error, an exception is returned. return
 * value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_esm_import_sz (jjs_context_t* context_p, const char *specifier_p)
{
  jjs_assert_api_enabled (context_p);
  return jjs_esm_import (context_p, annex_util_create_string_utf8_sz (context_p, specifier_p), JJS_MOVE);
} /* jjs_esm_import_sz */

/**
 * Import a module from in-memory source.
 *
 * @return the namespace of the imported module or an exception on failure to import the
 * module. the return value must be release with jjs_value_free
 */
jjs_value_t
jjs_esm_import_source (jjs_context_t* context_p, /**< JJS context */
                       const jjs_char_t *buffer_p, /**< UTF8 encoded module source */
                       jjs_size_t buffer_size, /**< size of buffer_p in bytes */
                       const jjs_esm_source_options_t *options_p) /**< module load options */
{
  jjs_assert_api_enabled (context_p);
  jjs_value_t result;

#if JJS_ANNEX_ESM
  result = esm_run_source (context_p, options_p, buffer_p, buffer_size, ECMA_VALUE_EMPTY, ESM_RUN_RESULT_NAMESPACE);
#else /* !JJS_ANNEX_ESM */
  JJS_UNUSED_ALL (buffer_p, buffer_size);
  result = jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_ESM */

  jjs_esm_source_options_disown (context_p, options_p);
  return result;
} /* jjs_esm_import_source */

/**
 * Import a module from in-memory source.
 *
 * @return the namespace of the imported module or an exception on failure to import the
 * module. the return value must be release with jjs_value_free
 */
jjs_value_t
jjs_esm_import_source_sz (jjs_context_t* context_p, /**< JJS context */
                          const char *source_p, /**< UTF8 encoded and null-terminated module source */
                          const jjs_esm_source_options_t *options_p) /**< module load options */
{
  return jjs_esm_import_source (context_p, (const jjs_char_t *) source_p, (jjs_size_t) strlen (source_p), options_p);
} /* jjs_esm_import_source_sz */

/**
 * Import a module from in-memory source.
 *
 * @return the namespace of the imported module or an exception on failure to import the
 * module. the return value must be release with jjs_value_free
 */
jjs_value_t
jjs_esm_import_source_value (jjs_context_t* context_p, /**< JJS context */
                             jjs_value_t source, /**< source value: string or buffer like */
                             jjs_own_t source_o, /**< source value resource ownership */
                             const jjs_esm_source_options_t *options_p) /**< module load options */
{
  jjs_assert_api_enabled (context_p);
  jjs_value_t result;

#if JJS_ANNEX_ESM
  result = esm_run_source (context_p, options_p, NULL, 0, source, ESM_RUN_RESULT_NAMESPACE);
#else /* !JJS_ANNEX_ESM */
  result = jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_ESM */

  jjs_disown_value (context_p, source, source_o);
  jjs_esm_source_options_disown (context_p, options_p);
  return result;
} /* jjs_esm_import_source_value */

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
 * @param context_p JJS context
 * @param specifier string value of the specifier
 * @param specifier_o specifier reference ownership
 * @return the evaluation result of the module. on error, an exception is
 * returned. return value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_esm_evaluate (jjs_context_t* context_p, jjs_value_t specifier, jjs_own_t specifier_o)
{
  jjs_assert_api_enabled (context_p);
#if JJS_ANNEX_ESM
  jjs_value_t referrer_path = annex_path_cwd (context_p);

  if (!jjs_value_is_string (context_p, referrer_path))
  {
    jjs_disown_value (context_p, specifier, specifier_o);
    return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "Failed to get current working directory");
  }

  jjs_value_t module = esm_read (context_p, specifier, referrer_path);

  jjs_value_free (context_p, referrer_path);

  jjs_disown_value (context_p, specifier, specifier_o);

  return esm_link_and_evaluate (context_p, module, true, ESM_RUN_RESULT_EVALUATE);
#else /* !JJS_ANNEX_ESM */
  jjs_disown_value (context_p, specifier, specifier_o);
  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_ESM */
} /* jjs_esm_evaluate */

/**
 * Evaluate an ES module.
 *
 * @see jjs_esm_evaluate
 *
 * @param context_p JJS context
 * @param specifier string value of the specifier. if NULL, an empty string is used.
 * @return the evaluation result of the module. on error, an exception is returned. return
 * value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_esm_evaluate_sz (jjs_context_t* context_p, const char *specifier_p)
{
  jjs_assert_api_enabled (context_p);
  return jjs_esm_evaluate (context_p, annex_util_create_string_utf8_sz (context_p, specifier_p), JJS_MOVE);
} /* jjs_esm_evaluate_sz */

/**
 * Evaluate a module from in-memory source.
 *
 * @return the evaluation result of the module or an exception on failure to evaluate the
 * module. the return value must be release with jjs_value_free
 */
jjs_value_t
jjs_esm_evaluate_source (jjs_context_t* context_p, /**< JJS context */
                         const jjs_char_t *buffer_p, /**< UTF-8 encoded module source */
                         jjs_size_t buffer_size, /**< size of buffer_p in bytes */
                         const jjs_esm_source_options_t *options_p) /**< module load options */
{
  jjs_assert_api_enabled (context_p);
  jjs_value_t result;

#if JJS_ANNEX_ESM
  result = esm_run_source (context_p, options_p, buffer_p, buffer_size, ECMA_VALUE_EMPTY, ESM_RUN_RESULT_EVALUATE);
#else /* !JJS_ANNEX_ESM */
  JJS_UNUSED_ALL (buffer_p, buffer_size);
  result = jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_ESM */

  jjs_esm_source_options_disown (context_p, options_p);

  return result;
} /* jjs_esm_evaluate_source */

/**
 * Evaluate a module from in-memory source.
 *
 * @return the evaluation result of the module or an exception on failure to evaluate the
 * module. the return value must be release with jjs_value_free
 */
jjs_value_t
jjs_esm_evaluate_source_sz (jjs_context_t* context_p, /**< JJS context */
                            const char *source_p, /**< module source code as a null-terminated string */
                            const jjs_esm_source_options_t *options_p) /**< module load options */
{
  return jjs_esm_evaluate_source (context_p, (const jjs_char_t *) source_p, (jjs_size_t) strlen (source_p), options_p);
} /* jjs_esm_evaluate_source_sz */

/**
 * Evaluate a module from in-memory source.
 *
 * @return the evaluation result of the module or an exception on failure to evaluate the
 * module. the return value must be release with jjs_value_free
 */
jjs_value_t
jjs_esm_evaluate_source_value (jjs_context_t* context_p, /**< JJS context */
                               jjs_value_t source, /**< source value: string or buffer like */
                               jjs_own_t source_o, /**< source value resource ownership */
                               const jjs_esm_source_options_t *options_p) /**< module load options */
{
  jjs_assert_api_enabled (context_p);
  jjs_value_t result;

#if JJS_ANNEX_ESM
  result = esm_run_source (context_p, options_p, NULL, 0, source, ESM_RUN_RESULT_EVALUATE);
#else /* !JJS_ANNEX_ESM */
  result = jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_ESM */

  jjs_disown_value (context_p, source, source_o);
  jjs_esm_source_options_disown (context_p, options_p);

  return result;
} /* jjs_esm_evaluate_source_value */

/**
 * Init an empty source options object.
 *
 * Note: this function is only for convenience. It is ok to use memset(0) or C's default struct
 *       initialization on a jjs_esm_source_options_t object.
 *
 * @return empty object
 */
jjs_esm_source_options_t jjs_esm_source_options (void)
{
  return (jjs_esm_source_options_t) {0};
} /* jjs_esm_source_options */

/**
 * Free all jjs_value_t values from an jjs_esm_source_options_t object.
 *
 * This function is exposed for narrow use cases. The jjs_esm_source family of functions
 * will call this function for you on any jjs_esm_source_options_t passed to them.
 */
void jjs_esm_source_options_disown (jjs_context_t *context_p, /**< JJS context */
                                    const jjs_esm_source_options_t* options_p) /**< options to clean up */
{
  jjs_assert_api_enabled (context_p);
  if (options_p)
  {
    if (options_p->filename_o == JJS_MOVE && options_p->filename.has_value)
    {
      jjs_value_free (context_p, options_p->filename.value);
    }

    if (options_p->dirname_o == JJS_MOVE && options_p->dirname.has_value)
    {
      jjs_value_free (context_p, options_p->dirname.value);
    }

    if (options_p->meta_extension_o == JJS_MOVE && options_p->meta_extension.has_value)
    {
      jjs_value_free (context_p, options_p->meta_extension.value);
    }
  }
} /* jjs_esm_source_options_disown */

jjs_value_t
jjs_esm_default_on_import_cb (jjs_context_t* context_p, jjs_value_t specifier, jjs_value_t user_value, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled (context_p);
#if JJS_ANNEX_ESM
  jjs_value_t referrer_path = user_value_to_path (context_p, user_value);

  if (!jjs_value_is_string (context_p, referrer_path))
  {
    jjs_value_free (context_p, referrer_path);
    return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "Failed to get referrer path from user_value");
  }

  jjs_value_t module = esm_import (context_p, specifier, referrer_path);

  jjs_value_free (context_p, referrer_path);

  return module;
#else /* !JJS_ANNEX_ESM */
  JJS_UNUSED_ALL (specifier, user_value);
  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ESM_NOT_SUPPORTED));
#endif /* JJS_ANNEX_ESM */
} /* jjs_esm_default_on_import_cb */

void
jjs_esm_default_on_import_meta_cb (jjs_context_t* context_p, jjs_value_t module, jjs_value_t meta_object, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled (context_p);
#if JJS_ANNEX_ESM
  jjs_module_copy_string_property (context_p, meta_object, module, LIT_MAGIC_STRING_URL);
  jjs_module_copy_string_property (context_p, meta_object, module, LIT_MAGIC_STRING_FILENAME);
  jjs_module_copy_string_property (context_p, meta_object, module, LIT_MAGIC_STRING_DIRNAME);

  jjs_value_t resolve = jjs_function_external (context_p, esm_resolve_handler);
  jjs_value_t dirname = ecma_find_own_m (context_p, module, LIT_MAGIC_STRING_DIRNAME);
  jjs_value_t path = ecma_make_magic_string_value (LIT_MAGIC_STRING_PATH);

  jjs_object_set_internal (context_p, resolve, path, dirname, JJS_MOVE);
  ecma_set_m (context_p, meta_object, LIT_MAGIC_STRING_RESOLVE, resolve);

  jjs_value_free (context_p, path);
  jjs_value_free (context_p, resolve);

  ecma_value_t extension = ecma_find_own_m (context_p, module, LIT_MAGIC_STRING_EXTENSION);

  if (extension != ECMA_VALUE_NOT_FOUND)
  {
    ecma_set_m (context_p, meta_object, LIT_MAGIC_STRING_EXTENSION, extension);
    ecma_free_value (context_p, extension);
  }
#else /* !JJS_ANNEX_ESM */
  JJS_UNUSED_ALL (module, meta_object);
#endif /* JJS_ANNEX_ESM */
} /* jjs_module_default_import_meta */

#if JJS_ANNEX_ESM

static JJS_HANDLER (esm_resolve_handler)
{
  jjs_context_t* context_p = call_info_p->context_p;
  jjs_value_t specifier = args_count > 0 ? args_p[0] : jjs_undefined (context_p);

  if (!jjs_value_is_string (context_p, specifier))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "Invalid argument");
  }

#if JJS_ANNEX_VMOD
  if (jjs_vmod_exists (context_p, specifier, JJS_KEEP))
  {
    return ecma_copy_value (context_p, specifier);
  }
#endif

  ecma_value_t referrer_path = annex_util_get_internal_m (context_p, call_info_p->function, LIT_MAGIC_STRING_PATH);

  if (!jjs_value_is_string (context_p, referrer_path))
  {
    jjs_value_free (context_p, referrer_path);
    return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "resolve is missing referrer path");
  }

  jjs_annex_module_resolve_t resolved = jjs_annex_module_resolve (context_p, specifier, referrer_path, JJS_MODULE_TYPE_MODULE);
  jjs_value_t result;

  if (jjs_value_is_exception (context_p, resolved.result))
  {
    result = jjs_value_copy (context_p, resolved.result);
  }
  else
  {
    /* options = { path: boolean }. if path truthy, return file path; otherwise, return file url */
    jjs_value_t options = args_count > 1 ? args_p[1] : jjs_undefined (context_p);
    ecma_value_t options_path = ecma_find_own_m (context_p, options, LIT_MAGIC_STRING_PATH);
    bool use_path = ecma_is_value_found (options_path) ? ecma_op_to_boolean (context_p, options_path) : false;

    ecma_free_value (context_p, options_path);

    if (use_path)
    {
      result = jjs_value_copy (context_p, resolved.path);
    }
    else
    {
      result = annex_path_to_file_url (context_p, resolved.path);

      if (!jjs_value_is_string (context_p, result))
      {
        ecma_free_value (context_p, result);
        result = jjs_throw_sz (context_p, JJS_ERROR_COMMON, "Failed to convert path to url.");
      }
    }
  }

  jjs_value_free (context_p, referrer_path);
  jjs_annex_module_resolve_free (context_p, &resolved);

  return result;
}

static jjs_value_t
esm_import (jjs_context_t* context_p, jjs_value_t specifier, jjs_value_t referrer_path)
{
  jjs_value_t module = esm_read (context_p, specifier, referrer_path);
  jjs_value_t result = esm_link_and_evaluate (context_p, module, false, ESM_RUN_RESULT_NONE);

  if (jjs_value_is_exception (context_p, result))
  {
    jjs_value_free (context_p, module);
    return result;
  }

  jjs_value_free (context_p, result);

  return module;
} /* esm_import */

static jjs_value_t
esm_realpath_dirname (jjs_context_t* context_p, jjs_value_t dirname_value)
{
  if (dirname_value == 0 || ecma_is_value_undefined (dirname_value))
  {
    return annex_path_cwd (context_p);
  }

  return annex_path_normalize (context_p, dirname_value);
} /* esm_realpath_dirname */

static jjs_value_t
esm_basename_or_default (jjs_context_t* context_p, jjs_value_t filename_value)
{
  if (filename_value == 0 || ecma_is_value_undefined (filename_value))
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING_ESM_FILENAME_DEFAULT);
  }

  return annex_path_basename (context_p, filename_value);
} /* esm_basename_or_default */

static jjs_value_t
esm_link_and_evaluate (jjs_context_t* context_p, jjs_value_t module, bool move_module, esm_result_type_t result_type)
{
  jjs_module_state_t state;
  jjs_value_t result;

  if (jjs_value_is_exception (context_p, module))
  {
    return move_module ? module : jjs_value_copy (context_p, module);
  }

  state = jjs_module_state (context_p, module);

  if (state == JJS_MODULE_STATE_UNLINKED)
  {
    jjs_value_t link_result = jjs_module_link (context_p, module, esm_link_cb, NULL);

    if (jjs_value_is_exception (context_p, link_result))
    {
      result = link_result;
      goto done;
    }

    JJS_ASSERT (jjs_value_is_true (context_p, link_result));
    jjs_value_free (context_p, link_result);
  }

  state = jjs_module_state (context_p, module);

  if (state == JJS_MODULE_STATE_LINKED)
  {
    result = jjs_module_evaluate (context_p, module);
  }
  else if (state == JJS_MODULE_STATE_EVALUATED)
  {
    result = ECMA_VALUE_UNDEFINED;
  }
  else
  {
    result = jjs_throw_sz (context_p, JJS_ERROR_COMMON, "module must be in linked state to evaluate");
  }

done:
  if (!jjs_value_is_exception (context_p, result))
  {
    switch (result_type)
    {
      case ESM_RUN_RESULT_NAMESPACE:
        jjs_value_free (context_p, result);
        result = jjs_module_namespace (context_p, module);
        break;
      case ESM_RUN_RESULT_EVALUATE:
        break;
      case ESM_RUN_RESULT_NONE:
        jjs_value_free (context_p, result);
        result = ECMA_VALUE_UNDEFINED;
        break;
      default:
        jjs_value_free (context_p, result);
        result = jjs_throw_sz (context_p, JJS_ERROR_COMMON, "invalid result_type");
        break;
    }
  }

  if (move_module)
  {
    jjs_value_free (context_p, module);
  }

  return result;
} /* esm_link_and_evaluate */

static jjs_esm_source_options_t DEFAULT_ESM_SOURCE_OPTIONS = {0};

static jjs_value_t
esm_run_source (jjs_context_t* context_p,
                const jjs_esm_source_options_t *options_p,
                const jjs_char_t *source_p,
                jjs_size_t source_size,
                jjs_value_t source_value,
                esm_result_type_t result_type)
{
  if (!options_p)
  {
    options_p = &DEFAULT_ESM_SOURCE_OPTIONS;
  }

  bool parse_from_source_buffer = ecma_is_value_empty (source_value);

  if (parse_from_source_buffer && (source_p == NULL || source_size == 0))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "source buffer is empty");
  }

  ecma_value_t esm_cache = ecma_get_global_object (context_p)->esm_cache;
  jjs_parse_options_t parse_options;
  jjs_value_t module;
  jjs_value_t basename_value = ECMA_VALUE_UNDEFINED;
  jjs_value_t filename_value = ECMA_VALUE_UNDEFINED;
  jjs_value_t dirname_value;

  dirname_value = esm_realpath_dirname (context_p, jjs_optional_value_or_undefined (&options_p->dirname));

  if (!jjs_value_is_string (context_p, dirname_value))
  {
    module = jjs_throw_sz (context_p, JJS_ERROR_TYPE, "jjs_source_options_t.dirname must be a path to an fs directory");
    goto after_parse;
  }

  basename_value = esm_basename_or_default (context_p, jjs_optional_value_or_undefined (&options_p->filename));

  if (!jjs_value_is_string (context_p, basename_value))
  {
    module = jjs_throw_sz (context_p, JJS_ERROR_TYPE, "jjs_source_options_t.filename must be a normal filename");
    goto after_parse;
  }

  filename_value = annex_path_join (context_p, dirname_value, basename_value, false);

  if (!jjs_value_is_string (context_p, filename_value))
  {
    module = jjs_throw_sz (context_p, JJS_ERROR_TYPE, "Failed to create filename path to source module.");
    goto after_parse;
  }

  if (ecma_has_own_v (context_p, esm_cache, filename_value))
  {
    module = jjs_throw_sz (context_p, JJS_ERROR_TYPE, "A module with this filename has already been loaded.");
    goto after_parse;
  }

  parse_options = (jjs_parse_options_t){
    .parse_module = true,
    .start_column = options_p->start_column,
    .start_line = options_p->start_line,
    .user_value = jjs_optional_value (filename_value),
    .source_name = jjs_optional_value (basename_value),
  };

  if (parse_from_source_buffer)
  {
    module = jjs_parse (context_p, source_p, source_size, &parse_options);
  }
  else
  {
    module = jjs_parse_value (context_p, source_value, JJS_KEEP, &parse_options);
  }

  if (!jjs_value_is_exception (context_p, module))
  {
    jjs_value_t file_url = annex_path_to_file_url (context_p, filename_value);

    JJS_ASSERT (ecma_is_value_string (file_url));

    if (!ecma_is_value_string (file_url))
    {
      file_url = ECMA_VALUE_UNDEFINED;
    }

    ecma_set_m (context_p, module, LIT_MAGIC_STRING_DIRNAME, dirname_value);
    ecma_set_m (context_p, module, LIT_MAGIC_STRING_URL, file_url);
    ecma_set_m (context_p, module, LIT_MAGIC_STRING_FILENAME, filename_value);

    if (options_p->meta_extension.has_value)
    {
      ecma_set_m (context_p, module, LIT_MAGIC_STRING_EXTENSION, options_p->meta_extension.value);
    }

    if (options_p->cache)
    {
      ecma_set_v (esm_cache, context_p, filename_value, module);
    }

    jjs_value_free (context_p, file_url);
  }

after_parse:
  jjs_value_free (context_p, filename_value);
  jjs_value_free (context_p, basename_value);
  jjs_value_free (context_p, dirname_value);

  return esm_link_and_evaluate (context_p, module, true, result_type);
} /* esm_run_source */

static jjs_value_t
esm_read (jjs_context_t* context_p, jjs_value_t specifier, jjs_value_t referrer_path)
{
  ecma_value_t esm_cache = ecma_get_global_object (context_p)->esm_cache;

#if JJS_ANNEX_VMOD
  if (jjs_annex_vmod_exists (context_p, specifier))
  {
    return vmod_get_or_load_module (context_p, specifier, esm_cache);
  }
#endif /* JJS_ANNEX_VMOD */

  // resolve specifier
  jjs_annex_module_resolve_t resolved = jjs_annex_module_resolve (context_p, specifier, referrer_path, JJS_MODULE_TYPE_MODULE);

  if (jjs_value_is_exception (context_p, resolved.result))
  {
    jjs_value_t resolved_exception = jjs_value_copy (context_p, resolved.result);
    jjs_annex_module_resolve_free (context_p, &resolved);
    return resolved_exception;
  }

  ecma_value_t cached_module = ecma_find_own_v (context_p, esm_cache, resolved.path);

  if (cached_module != ECMA_VALUE_NOT_FOUND)
  {
    jjs_annex_module_resolve_free (context_p, &resolved);
    return cached_module;
  }

  ecma_free_value (context_p, cached_module);

  // load source
  jjs_annex_module_load_t loaded = jjs_annex_module_load (context_p, resolved.path, resolved.format, JJS_MODULE_TYPE_MODULE);

  if (jjs_value_is_exception (context_p, loaded.result))
  {
    jjs_annex_module_resolve_free (context_p, &resolved);
    return loaded.result;
  }

  ecma_string_t *format_p = ecma_get_string_from_value (context_p, loaded.format);
  jjs_value_t module;

  if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_JS)
      || ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_MODULE))
  {
    jjs_parse_options_t opts = {
      .parse_module = true,
      .user_value = jjs_optional_value (resolved.path),
      .source_name = jjs_optional_value (resolved.path),
    };

    module = jjs_parse_value (context_p, loaded.source, JJS_KEEP, &opts);

    if (!jjs_value_is_exception (context_p, module))
    {
      jjs_value_t file_url = annex_path_to_file_url (context_p, resolved.path);

      if (jjs_value_is_string (context_p, file_url))
      {
        set_module_properties (context_p, module, resolved.path, file_url);
      }
      else
      {
        jjs_value_free (context_p, module);
        module = jjs_throw_sz (context_p, JJS_ERROR_COMMON, "failed to convert path to file url");
      }

      jjs_value_free (context_p, file_url);
    }
  }
#if JJS_ANNEX_COMMONJS
  else if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_COMMONJS))
  {
    jjs_value_t default_name = ecma_make_magic_string_value (LIT_MAGIC_STRING_DEFAULT);
    jjs_value_t file_url = annex_path_to_file_url (context_p, resolved.path);

    // TODO: get export names from pmap?

    JJS_ASSERT (ecma_is_value_string (file_url));

    if (!ecma_is_value_string (file_url))
    {
      file_url = ECMA_VALUE_UNDEFINED;
    }

    module = jjs_synthetic_module (context_p, commonjs_module_evaluate_cb, &default_name, 1, JJS_MOVE);
    set_module_properties (context_p, module, resolved.path, file_url);

    jjs_value_free (context_p, file_url);
  }
#endif /* JJS_ANNEX_COMMONJS */
  else
  {
    module = jjs_throw_sz (context_p, JJS_ERROR_TYPE, "Invalid format");
  }

  if (!jjs_value_is_exception (context_p, module))
  {
    ecma_set_v (esm_cache, context_p, resolved.path, module);
  }

  jjs_annex_module_resolve_free (context_p, &resolved);
  jjs_annex_module_load_free (context_p, &loaded);

  return module;
} /* esm_read */

static jjs_value_t
esm_link_cb (jjs_context_t* context_p, jjs_value_t specifier, jjs_value_t referrer, void *user_p)
{
  JJS_UNUSED (user_p);

  jjs_value_t path = ecma_find_own_m (context_p, referrer, LIT_MAGIC_STRING_DIRNAME);
  jjs_value_t module = esm_read (context_p, specifier, path);

  jjs_value_free (context_p, path);

  return module;
} /* esm_link_cb */

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_VMOD

/**
 * Sets the default export of a synthetic/native ES module.
 *
 * If exports contains a 'default' key, exports.default will be used as
 * default. Otherwise, exports will be used as default.
 *
 * @param context_p JJS context
 * @param native_module synthetic/native ES module
 * @param exports CommonJS or Virtual Module exports object
 * @return true if successful, exception otherwise. the return value must be
 * freed with jjs_value_free.
 */
static jjs_value_t
module_native_set_default (jjs_context_t* context_p, jjs_value_t native_module, jjs_value_t exports)
{
  jjs_value_t default_name = ecma_make_magic_string_value (LIT_MAGIC_STRING_DEFAULT);
  ecma_value_t default_value = ecma_find_own_v (context_p, exports, default_name);
  jjs_value_t result = jjs_synthetic_module_set_export (context_p,
                                                        native_module,
                                                        default_name,
                                                        ecma_is_value_found (default_value) ? default_value : exports,
                                                        JJS_KEEP);

  jjs_value_free (context_p, default_name);
  ecma_free_value (context_p, default_value);

  return result;
} /* module_native_set_default */

#endif /* JJS_ANNEX_COMMONJS || JJS_ANNEX_VMOD */

#if JJS_ANNEX_COMMONJS

static jjs_value_t
commonjs_module_evaluate_cb (jjs_context_t* context_p, jjs_value_t native_module)
{
  jjs_value_t filename = ecma_find_own_m (context_p, native_module, LIT_MAGIC_STRING_FILENAME);
  JJS_ASSERT (jjs_value_is_string (context_p, filename));
  jjs_value_t referrer_path = ecma_find_own_m (context_p, native_module, LIT_MAGIC_STRING_DIRNAME);
  JJS_ASSERT (jjs_value_is_string (context_p, referrer_path));

  jjs_value_t exports = jjs_annex_require (context_p, filename, referrer_path);

  jjs_value_free (context_p, filename);
  jjs_value_free (context_p, referrer_path);

  if (jjs_value_is_exception (context_p, exports))
  {
    return exports;
  }

  jjs_value_t result = module_native_set_default (context_p, native_module, exports);

  jjs_value_free (context_p, exports);

  return result;
} /* commonjs_module_evaluate_cb */

#endif /* JJS_ANNEX_COMMONJS */

#if JJS_ANNEX_VMOD

static jjs_value_t
vmod_module_evaluate_cb (jjs_context_t* context_p, jjs_value_t native_module)
{
  ecma_value_t exports = ecma_find_own_m (context_p, native_module, LIT_MAGIC_STRING_EXPORTS);

  JJS_ASSERT (ecma_is_value_found (exports));

  if (!ecma_is_value_found (exports))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "vmod esm module missing exports property");
  }

  ecma_value_t delete_result = ecma_op_object_delete (context_p,
                                                      ecma_get_object_from_value (context_p, native_module),
                                                      ecma_get_magic_string (LIT_MAGIC_STRING_EXPORTS),
                                                      false);

  ecma_free_value (context_p, delete_result);

  jjs_value_t result = module_native_set_default (context_p, native_module, exports);

  jjs_value_free (context_p, exports);

  return result;
} /* vmod_module_evaluate_cb */

static jjs_value_t
vmod_link (jjs_context_t* context_p, jjs_value_t module, jjs_value_t exports, ecma_collection_t *keys_p, bool was_default_appended)
{
  uint32_t count = keys_p->item_count - (was_default_appended ? 1 : 0);

  for (uint32_t i = 0; i < count; i++)
  {
    ecma_value_t value = ecma_find_own_v (context_p, exports, keys_p->buffer_p[i]);

    JJS_ASSERT (value != ECMA_VALUE_NOT_FOUND);

    if (value == ECMA_VALUE_NOT_FOUND)
    {
      return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "failed to get export value while linking vmod module");
    }

    jjs_value_t result = jjs_synthetic_module_set_export (context_p, module, keys_p->buffer_p[i], value, JJS_MOVE);

    if (jjs_value_is_exception (context_p, result))
    {
      return result;
    }

    jjs_value_free (context_p, result);
  }

  if (was_default_appended)
  {
    ecma_value_t default_key = ecma_make_magic_string_value (LIT_MAGIC_STRING_DEFAULT);
    jjs_value_t result = jjs_synthetic_module_set_export (context_p, module, default_key, exports, JJS_KEEP);

    ecma_free_value (context_p, default_key);

    if (jjs_value_is_exception (context_p, result))
    {
      return result;
    }

    jjs_value_free (context_p, result);
  }

  return jjs_module_link (context_p, module, esm_link_cb, NULL);
} /* vmod_link */

static jjs_value_t
vmod_get_or_load_module (jjs_context_t* context_p, jjs_value_t specifier, ecma_value_t esm_cache)
{
  ecma_value_t cached = ecma_find_own_v (context_p, esm_cache, specifier);

  if (ecma_is_value_found (cached))
  {
    return cached;
  }

  ecma_free_value (context_p, cached);

  jjs_value_t exports = jjs_annex_vmod_resolve (context_p, specifier);

  if (jjs_value_is_exception (context_p, exports))
  {
    return exports;
  }

  ecma_collection_t *keys_p;

  if (ecma_is_value_object (exports))
  {
    keys_p = ecma_op_object_get_enumerable_property_names (context_p,
                                                           ecma_get_object_from_value (context_p, exports),
                                                           ECMA_ENUMERABLE_PROPERTY_KEYS);

#if JJS_BUILTIN_PROXY
    if (JJS_UNLIKELY (keys_p == NULL))
    {
      return ecma_create_exception_from_context (context_p);
    }
#endif /* JJS_BUILTIN_PROXY */
  }
  else
  {
    keys_p = ecma_new_collection (context_p);

    if (JJS_UNLIKELY (keys_p == NULL))
    {
      return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "failed to allocate collection for vmod keys");
    }
  }

  bool was_default_appended;

  if (keys_p->item_count == 0 || !ecma_has_own_m (context_p, exports, LIT_MAGIC_STRING_DEFAULT))
  {
    ecma_collection_push_back (context_p, keys_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_DEFAULT));
    was_default_appended = true;
  }
  else
  {
    was_default_appended = false;
  }

  jjs_value_t native_module = jjs_synthetic_module (context_p, vmod_module_evaluate_cb, keys_p->buffer_p, keys_p->item_count, JJS_KEEP);

  if (!jjs_value_is_exception (context_p, native_module))
  {
    jjs_value_t linked = vmod_link (context_p, native_module, exports, keys_p, was_default_appended);

    if (!jjs_value_is_exception (context_p, linked))
    {
      jjs_value_free (context_p, linked);
      ecma_set_m (context_p, native_module, LIT_MAGIC_STRING_EXPORTS, exports);
      ecma_set_v (esm_cache, context_p, specifier, native_module);
    }
    else
    {
      jjs_value_free (context_p, native_module);
      native_module = linked;
    }
  }

  ecma_collection_free (context_p, keys_p);
  jjs_value_free (context_p, exports);

  return native_module;
} /* vmod_get_or_load_module */

#endif /* JJS_ANNEX_VMOD */

static jjs_value_t
user_value_to_path (jjs_context_t* context_p, jjs_value_t user_value)
{
  annex_specifier_type_t path_type = annex_path_specifier_type (context_p, user_value);
  jjs_value_t result;

  if (path_type == ANNEX_SPECIFIER_TYPE_ABSOLUTE)
  {
    jjs_value_t module = ecma_find_own_v (context_p, ecma_get_global_object (context_p)->esm_cache, user_value);

    result = ecma_is_value_found (module) ? ecma_find_own_m (context_p, module, LIT_MAGIC_STRING_DIRNAME)
                                          : annex_path_dirname (context_p, user_value);

    jjs_value_free (context_p, module);
  }
  else if (path_type == ANNEX_SPECIFIER_TYPE_FILE_URL)
  {
    result = jjs_throw_sz (context_p, JJS_ERROR_COMMON, "user_value cannot be a file url");
  }
  else
  {
    // if no absolute path, ignore user_value contents and use the cwd.
    //
    // when using jjs_parse, the caller may forget to set user_value, they need to contrive a fake absolute
    // path (for parsing an in mem string) or the absolute path needs to be built. if user_value is not set,
    // cwd is a reasonable default value for most use cases.
    result = annex_path_cwd (context_p);
  }

  return result;
} /* user_value_to_path */

static void
set_module_properties (jjs_context_t* context_p, jjs_value_t module, jjs_value_t filename, jjs_value_t url)
{
  if (jjs_value_is_exception (context_p, module))
  {
    return;
  }

  jjs_value_t path_dirname = annex_path_dirname (context_p, filename);

  JJS_ASSERT (jjs_value_is_string (context_p, path_dirname));

  ecma_set_m (context_p, module, LIT_MAGIC_STRING_DIRNAME, path_dirname);
  ecma_set_m (context_p, module, LIT_MAGIC_STRING_URL, url);
  ecma_set_m (context_p, module, LIT_MAGIC_STRING_FILENAME, filename);

  jjs_value_free (context_p, path_dirname);
} /* set_module_properties */

#endif /* JJS_ANNEX_ESM */
