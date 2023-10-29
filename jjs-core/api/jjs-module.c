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

#include "lit-magic-strings.h"
#include "lit-strings.h"

#include "jjs-core.h"
#include "jjs-port.h"
#include "jjs-annex.h"
#include "jjs-annex-module-util.h"
#include "jcontext.h"

#include "ecma-errors.h"
#include "ecma-builtins.h"
#include "annex.h"

#if JJS_MODULE_SYSTEM

/**
 * A module descriptor.
 */
typedef struct jjs_module_t
{
  struct jjs_module_t *next_p; /**< next_module */
  jjs_char_t *path_p; /**< path to the module */
  jjs_size_t basename_offset; /**< offset of the basename in the module path*/
  jjs_value_t realm; /**< the realm of the module */
  jjs_value_t module; /**< the module itself */
} jjs_module_t;

/**
 * Native info descriptor for modules.
 */
static const jjs_object_native_info_t jjs_module_native_info JJS_ATTR_CONST_DATA = {
  .free_cb = NULL,
};

/**
 * Default module manager.
 */
typedef struct
{
  jjs_module_t *module_head_p; /**< first module */
} jjs_module_manager_t;

/**
 * Release known modules.
 */
static void
jjs_module_free (jjs_module_manager_t *manager_p, /**< module manager */
                   const jjs_value_t realm) /**< if this argument is object, release only those modules,
                                               *   which realm value is equal to this argument. */
{
  jjs_module_t *module_p = manager_p->module_head_p;

  bool release_all = !jjs_value_is_object (realm);

  jjs_module_t *prev_p = NULL;

  while (module_p != NULL)
  {
    jjs_module_t *next_p = module_p->next_p;

    if (release_all || module_p->realm == realm)
    {
      jjs_port_path_free (module_p->path_p);
      jjs_value_free (module_p->realm);
      jjs_value_free (module_p->module);

      jjs_heap_free (module_p, sizeof (jjs_module_t));

      if (prev_p == NULL)
      {
        manager_p->module_head_p = next_p;
      }
      else
      {
        prev_p->next_p = next_p;
      }
    }
    else
    {
      prev_p = module_p;
    }

    module_p = next_p;
  }
} /* jjs_module_free */

/**
 * Initialize the default module manager.
 */
static void
jjs_module_manager_init (void *user_data_p)
{
  ((jjs_module_manager_t *) user_data_p)->module_head_p = NULL;
} /* jjs_module_manager_init */

/**
 * Deinitialize the default module manager.
 */
static void
jjs_module_manager_deinit (void *user_data_p) /**< context pointer to deinitialize */
{
  jjs_value_t undef = jjs_undefined ();
  jjs_module_free ((jjs_module_manager_t *) user_data_p, undef);
  jjs_value_free (undef);
} /* jjs_module_manager_deinit */

/**
 * Declare the context data manager for modules.
 */
static const jjs_context_data_manager_t jjs_module_manager JJS_ATTR_CONST_DATA = {
  .init_cb = jjs_module_manager_init,
  .deinit_cb = jjs_module_manager_deinit,
  .bytes_needed = sizeof (jjs_module_manager_t)
};

#endif /* JJS_MODULE_SYSTEM */

/**
 * Default module resolver.
 *
 * @return a module object if resolving is successful, an error otherwise
 */
jjs_value_t
jjs_module_resolve (const jjs_value_t specifier, /**< module specifier string */
                      const jjs_value_t referrer, /**< parent module */
                      void *user_p) /**< user data */
{
#if JJS_MODULE_SYSTEM
  JJS_UNUSED (user_p);

  const jjs_char_t *directory_p = lit_get_magic_string_utf8 (LIT_MAGIC_STRING__EMPTY);
  jjs_size_t directory_size = lit_get_magic_string_size (LIT_MAGIC_STRING__EMPTY);

  jjs_module_t *module_p = jjs_object_get_native_ptr (referrer, &jjs_module_native_info);

  if (module_p != NULL)
  {
    directory_p = module_p->path_p;
    directory_size = module_p->basename_offset;
  }

  jjs_size_t specifier_size = jjs_string_size (specifier, JJS_ENCODING_UTF8);
  jjs_size_t reference_size = directory_size + specifier_size;
  jjs_char_t *reference_path_p = jjs_heap_alloc (reference_size + 1);

  memcpy (reference_path_p, directory_p, directory_size);
  jjs_string_to_buffer (specifier, JJS_ENCODING_UTF8, reference_path_p + directory_size, specifier_size);
  reference_path_p[reference_size] = '\0';

  jjs_char_t *path_p = jjs_port_path_normalize (reference_path_p, reference_size);
  jjs_heap_free (reference_path_p, reference_size + 1);

  if (path_p == NULL)
  {
    return jjs_throw_sz (JJS_ERROR_SYNTAX, "Failed to resolve module");
  }

  jjs_value_t realm = jjs_current_realm ();

  jjs_module_manager_t *manager_p;
  manager_p = (jjs_module_manager_t *) jjs_context_data (&jjs_module_manager);

  module_p = manager_p->module_head_p;

  while (module_p != NULL)
  {
    if (module_p->realm == realm && strcmp ((const char *) module_p->path_p, (const char *) path_p) == 0)
    {
      jjs_value_free (realm);
      jjs_port_path_free (path_p);
      return jjs_value_copy (module_p->module);
    }

    module_p = module_p->next_p;
  }

  jjs_size_t source_size;
  jjs_char_t *source_p = jjs_port_source_read ((const char *) path_p, &source_size);

  if (source_p == NULL)
  {
    jjs_value_free (realm);
    jjs_port_path_free (path_p);

    return jjs_throw_sz (JJS_ERROR_SYNTAX, "Module file not found");
  }

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_MODULE | JJS_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jjs_value_copy (specifier);

  jjs_value_t ret_value = jjs_parse (source_p, source_size, &parse_options);
  jjs_value_free (parse_options.source_name);

  jjs_port_source_free (source_p);

  if (jjs_value_is_exception (ret_value))
  {
    jjs_port_path_free (path_p);
    jjs_value_free (realm);
    return ret_value;
  }

  module_p = (jjs_module_t *) jjs_heap_alloc (sizeof (jjs_module_t));

  module_p->next_p = manager_p->module_head_p;
  module_p->path_p = path_p;
  module_p->basename_offset = jjs_port_path_base (module_p->path_p);
  module_p->realm = realm;
  module_p->module = jjs_value_copy (ret_value);

  jjs_object_set_native_ptr (ret_value, &jjs_module_native_info, module_p);
  manager_p->module_head_p = module_p;

  return ret_value;
#else /* JJS_MODULE_SYSTEM */
  JJS_UNUSED (specifier);
  JJS_UNUSED (referrer);
  JJS_UNUSED (user_p);

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_resolve */

/**
 * Release known modules.
 */
void
jjs_module_cleanup (const jjs_value_t realm) /**< if this argument is object, release only those modules,
                                                  *   which realm value is equal to this argument. */
{
#if JJS_MODULE_SYSTEM
  jjs_module_free ((jjs_module_manager_t *) jjs_context_data (&jjs_module_manager), realm);
#else /* JJS_MODULE_SYSTEM */
  JJS_UNUSED (realm);
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_cleanup */

#if JJS_MODULE_SYSTEM

static jjs_value_t esm_read (jjs_value_t specifier, jjs_value_t referrer_path);
static jjs_value_t esm_import (jjs_value_t specifier, jjs_value_t referrer_path);
static jjs_value_t path_to_file_url (jjs_value_t path);
static jjs_value_t user_value_to_path (jjs_value_t user_value);
static jjs_value_t esm_link_cb (jjs_value_t specifier, jjs_value_t referrer, void *user_p);
static jjs_value_t esm_link (jjs_value_t module);
static jjs_value_t esm_evaluate (jjs_value_t module);
static void set_module_properties (jjs_value_t module, jjs_value_t filename, jjs_value_t url);
#if JJS_COMMONJS
static jjs_value_t commonjs_module_evaluate_cb (jjs_value_t native_module);
#endif /* JJS_COMMONJS */

#endif /* JJS_MODULE_SYSTEM */

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
jjs_module_default_load (jjs_value_t path, jjs_module_load_context_t* context_p, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled ();

#if JJS_MODULE_SYSTEM || JJS_COMMONJS
  ecma_cstr_t path_cstr = ecma_string_to_cstr (path);
  jjs_size_t source_size;
  jjs_char_t *source_raw = jjs_port_source_read (path_cstr.str_p, &source_size);

  ecma_free_cstr (&path_cstr);

  if (!source_raw)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Failed to read source file");
  }

  ecma_value_t source;
  ecma_string_t* format_p = ecma_get_string_from_value (context_p->format);

  if (ecma_compare_ecma_string_to_magic_id(format_p, LIT_MAGIC_STRING_SNAPSHOT))
  {
    source = jjs_arraybuffer(source_size);

    if (jjs_value_is_exception(source))
    {
      source = ECMA_VALUE_EMPTY;
    }
    else
    {
      uint8_t* buffer_p = jjs_arraybuffer_data (source);
      JJS_ASSERT(buffer_p != NULL);
      memcpy (buffer_p, source_raw, source_size);
    }
  }
  else if (!ecma_compare_ecma_string_to_magic_id(format_p, LIT_MAGIC_STRING_NONE))
  {
    source = jjs_string ((const jjs_char_t*) source_raw, source_size, JJS_ENCODING_UTF8);
  }
  else
  {
    source = ECMA_VALUE_EMPTY;
  }

  jjs_port_source_free (source_raw);

  if (source == ECMA_VALUE_EMPTY)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Failed to create source");
  }

  ecma_value_t result = ecma_create_object_with_null_proto ();

  ecma_set_m (result, LIT_MAGIC_STRING_SOURCE, source);
  ecma_free_value (source);

  ecma_set_m (result, LIT_MAGIC_STRING_FORMAT, context_p->format);

  return result;
#else /* !(JJS_MODULE_SYSTEM || JJS_COMMONJS) */
  JJS_UNUSED (path);
  JJS_UNUSED (context_p);

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JJS_MODULE_SYSTEM || JJS_COMMONJS */
} /* jjs_module_default_load */

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
 * @param user_p user pointer passed to jjs_module_on_resolve
 * @return on success, an object containing 'path' to a module and 'format' of the module
 */
jjs_value_t
jjs_module_default_resolve (jjs_value_t specifier, jjs_module_resolve_context_t* context_p, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled ();

#if JJS_MODULE_SYSTEM || JJS_COMMONJS
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
#if JJS_PMAP
    case ANNEX_SPECIFIER_TYPE_PACKAGE:
    {
      path = jjs_annex_pmap_resolve (specifier, context_p->type);
      break;
    }
#endif /* JJS_PMAP */
    default:
    {
      path = ECMA_VALUE_EMPTY;
      break;
    }
  }

  if (jjs_value_is_exception(path))
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
#else /* !(JJS_MODULE_SYSTEM || JJS_COMMONJS) */
  JJS_UNUSED (specifier);
  JJS_UNUSED (context_p);

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JJS_MODULE_SYSTEM || JJS_COMMONJS */
} /* jjs_module_default_resolve */

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
 * @return the namespace object of the module. on error, an exception is
 * returned. return value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_esm_import (jjs_value_t specifier)
{
  jjs_assert_api_enabled ();
#if JJS_MODULE_SYSTEM
  jjs_value_t referrer_path = annex_path_cwd ();

  if (!jjs_value_is_string (referrer_path))
  {
    return jjs_throw_sz(JJS_ERROR_COMMON, "Failed to get current working directory");
  }

  jjs_value_t module = esm_import (specifier, referrer_path);

  jjs_value_free (referrer_path);

  if (jjs_value_is_exception (module))
  {
    return module;
  }

  jjs_value_t namespace = jjs_module_namespace (module);

  jjs_value_free (module);

  return namespace;
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED (specifier);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
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
jjs_value_t jjs_esm_import_sz (const char* specifier_p)
{
  jjs_assert_api_enabled ();
#if JJS_MODULE_SYSTEM
  jjs_value_t specifier = annex_util_create_string_utf8_sz (specifier_p);
  jjs_value_t result = jjs_esm_import (specifier);

  jjs_value_free (specifier);

  return result;
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED (specifier_p);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif
} /* jjs_esm_import_sz */

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
 * @return the evaluation result of the module. on error, an exception is
 * returned. return value must be freed with jjs_value_free.
 */
jjs_value_t jjs_esm_evaluate (jjs_value_t specifier)
{
  jjs_assert_api_enabled ();
#if JJS_MODULE_SYSTEM
  jjs_value_t referrer_path = annex_path_cwd ();

  if (!jjs_value_is_string (referrer_path))
  {
    return jjs_throw_sz (JJS_ERROR_COMMON, "Failed to get current working directory");
  }

  // parse module
  jjs_value_t module = esm_read (specifier, referrer_path);

  jjs_value_free (referrer_path);

  if (jjs_value_is_exception (module))
  {
    return module;
  }

  // link
  jjs_value_t linked = esm_link (module);

  if (jjs_value_is_exception (linked))
  {
    jjs_value_free (module);
    return linked;
  }

  // evaluate
  jjs_value_t result = esm_evaluate (module);

  jjs_value_free (module);

  return result;
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED (specifier);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif
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
jjs_value_t jjs_esm_evaluate_sz (const char* specifier_p)
{
  jjs_assert_api_enabled ();
#if JJS_MODULE_SYSTEM
  jjs_value_t specifier = annex_util_create_string_utf8_sz (specifier_p);
  jjs_value_t result = jjs_esm_evaluate (specifier);

  jjs_value_free (specifier);

  return result;
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED (specifier_p);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif
} /* jjs_esm_evaluate_sz */

/**
 * Import a CommonJS module.
 *
 * The specifier can be a package name, relative path (qualified with
 * ./ or ../) or absolute path. Package names are resolved by the currently
 * set pmap.
 *
 * @param specifier string value of the specifier
 * @return module export object. on error, an exception is returned. return
 * value must be freed with jjs_value_free.
 */
jjs_value_t
jjs_commonjs_require (jjs_value_t specifier)
{
  jjs_assert_api_enabled ();
#if JJS_COMMONJS
  jjs_value_t referrer_path = annex_path_cwd ();
  jjs_value_t result = jjs_annex_require (specifier, referrer_path);

  jjs_value_free (referrer_path);

  return result;
#else /* !JJS_COMMONJS */
  JJS_UNUSED (specifier);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_COMMONJS_NOT_SUPPORTED));
#endif
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
jjs_value_t jjs_commonjs_require_sz (const char* specifier_p)
{
  jjs_assert_api_enabled ();
#if JJS_COMMONJS
  jjs_value_t specifier = annex_util_create_string_utf8_sz (specifier_p);
  jjs_value_t result = jjs_commonjs_require (specifier);

  jjs_value_free (specifier);

  return result;
#else /* !JJS_COMMONJS */
  JJS_UNUSED (specifier_p);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_COMMONJS_NOT_SUPPORTED));
#endif
} /* jjs_commonjs_require_sz */

jjs_value_t
jjs_module_default_import (jjs_value_t specifier, jjs_value_t user_value, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled ();
#if JJS_MODULE_SYSTEM
  jjs_value_t referrer_path = user_value_to_path (user_value);

  if (!jjs_value_is_string (referrer_path))
  {
    jjs_value_free (referrer_path);
    return jjs_throw_sz (JJS_ERROR_COMMON, "Failed to get referrer path from user_value");
  }

  jjs_value_t module = esm_import (specifier, referrer_path);

  jjs_value_free (referrer_path);

  return module;
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED (specifier);
  JJS_UNUSED (user_value);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif
} /* jjs_module_default_import */

void
jjs_module_default_import_meta (jjs_value_t module, jjs_value_t meta_object, void *user_p)
{
  JJS_UNUSED (user_p);
  jjs_assert_api_enabled ();
#if JJS_MODULE_SYSTEM
  ecma_value_t url = ecma_find_own_m (module, LIT_MAGIC_STRING_URL);

  if (ecma_is_value_string (url))
  {
    ecma_set_m (meta_object, LIT_MAGIC_STRING_URL, url);
  }

  ecma_free_value (url);
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED (module);
  JJS_UNUSED (meta_object);
#endif
} /* jjs_module_default_import_meta */

#if JJS_MODULE_SYSTEM

static jjs_value_t
esm_import (jjs_value_t specifier, jjs_value_t referrer_path)
{
  // parse module
  jjs_value_t module = esm_read (specifier, referrer_path);

  if (jjs_value_is_exception (module))
  {
    return module;
  }

  // link
  jjs_value_t linked = esm_link (module);

  if (jjs_value_is_exception (linked))
  {
    jjs_value_free (module);
    return linked;
  }

  // evaluate
  jjs_value_t evaluated = esm_evaluate (module);

  if (jjs_value_is_exception (evaluated))
  {
    jjs_value_free (module);
    return evaluated;
  }

  jjs_value_free (evaluated);

  // ensure the imported module is in the evaluated state
  jjs_module_state_t state = jjs_module_state (module);

  JJS_ASSERT(state == JJS_MODULE_STATE_EVALUATED);

  if (state != JJS_MODULE_STATE_EVALUATED)
  {
    jjs_value_free (module);
    module = jjs_throw_sz (JJS_ERROR_COMMON, "imported module in unknown state");
  }

  return module;
} /* esm_import */

static jjs_value_t
esm_link (jjs_value_t module)
{
  jjs_module_state_t state = jjs_module_state (module);

  if (state == JJS_MODULE_STATE_UNLINKED)
  {
    jjs_value_t link_result = jjs_module_link (module, esm_link_cb, NULL);

    if (jjs_value_is_exception (link_result))
    {
      return link_result;
    }

    bool linked = jjs_value_is_true (link_result);

    jjs_value_free (link_result);

    if (!linked)
    {
      return jjs_throw_sz (JJS_ERROR_COMMON, "failed to link module");
    }
  }

  return ECMA_VALUE_UNDEFINED;
} /* esm_link */

static jjs_value_t
esm_evaluate (jjs_value_t module)
{
  jjs_module_state_t state = jjs_module_state (module);

  if (state == JJS_MODULE_STATE_LINKED || state == JJS_MODULE_STATE_UNLINKED)
  {
    return jjs_module_evaluate (module);
  }

  return ECMA_VALUE_UNDEFINED;
} /* esm_evaluate */

static jjs_value_t
esm_read (jjs_value_t specifier, jjs_value_t referrer_path)
{
  // resolve specifier
  jjs_annex_module_resolve_t resolved = jjs_annex_module_resolve (specifier, referrer_path, JJS_MODULE_TYPE_MODULE);

  if (jjs_value_is_exception (resolved.result))
  {
    return resolved.result;
  }

  ecma_value_t esm_cache = ecma_get_global_object ()->esm_cache;
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

  ecma_string_t* format_p = ecma_get_string_from_value (loaded.format);
  jjs_value_t module;

  if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_JS)
      || ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_MODULE))
  {
    jjs_value_t file_url = path_to_file_url (resolved.path);
    jjs_parse_options_t opts = {
      .options = JJS_PARSE_MODULE | JJS_PARSE_HAS_USER_VALUE | JJS_PARSE_HAS_SOURCE_NAME,
      .user_value = file_url,
      .source_name = resolved.path,
    };

    if (!jjs_value_is_exception (file_url))
    {
      module = jjs_parse_value (loaded.source, &opts);
      set_module_properties (module, resolved.path, file_url);
    }
    else
    {
      module = jjs_throw_sz (JJS_ERROR_COMMON, "failed to convert path to file url");
    }

    jjs_value_free (file_url);
  }
#if JJS_COMMONJS
  else if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_COMMONJS))
  {
    jjs_value_t default_name = ecma_make_magic_string_value (LIT_MAGIC_STRING_DEFAULT);
    jjs_value_t file_url = path_to_file_url (resolved.path);

    // TODO: get export names from pmap?

    if (!jjs_value_is_exception (file_url))
    {
      module = jjs_native_module (commonjs_module_evaluate_cb, &default_name, 1);
      set_module_properties (module, resolved.path, file_url);
    }
    else
    {
      module = jjs_throw_sz (JJS_ERROR_COMMON, "failed to convert path to file url");
    }

    jjs_value_free (default_name);
    jjs_value_free (file_url);
  }
#endif /* JJS_COMMONJS */
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
path_to_file_url (jjs_value_t path)
{
  jjs_value_t result;
  jjs_value_t prefix = ecma_make_magic_string_value (LIT_MAGIC_STRING_FILE_URL_PREFIX);
  jjs_value_t encoded = ecma_builtin_global_encode_uri (path);

  if (jjs_value_is_exception (encoded))
  {
    result = encoded;
    encoded = ECMA_VALUE_UNDEFINED;
  }
  else
  {
    result = jjs_binary_op (JJS_BIN_OP_ADD, prefix, encoded);
  }

  jjs_value_free (prefix);
  jjs_value_free (encoded);

  return result;
} /* path_to_file_url */

static jjs_value_t
esm_link_cb (jjs_value_t specifier, jjs_value_t referrer, void *user_p)
{
  JJS_UNUSED (user_p);

  jjs_value_t path = ecma_find_own_m (referrer, LIT_MAGIC_STRING_PATH);
  jjs_value_t module = esm_read (specifier, path);

  jjs_value_free (path);

  return module;
} /* esm_link_cb */

#if JJS_COMMONJS

static jjs_value_t
commonjs_module_evaluate_cb (jjs_value_t native_module)
{
  jjs_value_t filename = ecma_find_own_m (native_module, LIT_MAGIC_STRING_FILENAME);
  JJS_ASSERT (jjs_value_is_string (filename));
  jjs_value_t path = ecma_find_own_m (native_module, LIT_MAGIC_STRING_PATH);
  JJS_ASSERT (jjs_value_is_string (path));
  jjs_value_t exports = jjs_annex_require(filename, path);

  jjs_value_free (filename);
  jjs_value_free (path);

  if (jjs_value_is_exception (exports))
  {
    return exports;
  }

  jjs_value_t default_name = ecma_make_magic_string_value (LIT_MAGIC_STRING_DEFAULT);
  jjs_value_t result = jjs_native_module_set (native_module, default_name, exports);

  jjs_value_free (default_name);
  jjs_value_free (exports);

  if (jjs_value_is_exception (result))
  {
    return result;
  }

  jjs_value_free (result);

  return ECMA_VALUE_UNDEFINED;
} /* commonjs_module_evaluate_cb */

#endif /* JJS_COMMONJS */

static jjs_value_t
user_value_to_path (jjs_value_t user_value)
{
  annex_specifier_type_t path_type = annex_path_specifier_type (user_value);
  jjs_value_t filename;

  if (path_type == ANNEX_SPECIFIER_TYPE_ABSOLUTE)
  {
    filename = annex_path_normalize (user_value);
  }
  else if (path_type == ANNEX_SPECIFIER_TYPE_FILE_URL)
  {
    filename = annex_path_from_file_url (user_value);
  }
  else
  {
    filename = ECMA_VALUE_EMPTY;
  }

  jjs_value_t result = annex_path_dirname (filename);

  jjs_value_free (filename);

  if (!jjs_value_is_string(result))
  {
    jjs_value_free (result);
    return jjs_throw_sz (JJS_ERROR_TYPE, "Invalid module/source user value.");
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

  jjs_value_t path = annex_path_dirname (filename);

  JJS_ASSERT(jjs_value_is_string (path));

  ecma_set_m (module, LIT_MAGIC_STRING_PATH, path);
  ecma_set_m (module, LIT_MAGIC_STRING_URL, url);
  ecma_set_m (module, LIT_MAGIC_STRING_FILENAME, filename);

  jjs_value_free (path);
} /* set_module_properties */

#endif /* JJS_MODULE_SYSTEM */
