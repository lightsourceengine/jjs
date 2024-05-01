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
#include "jjs-annex-vmod.h"

#include "jjs-annex.h"
#include "jjs-core.h"
#include "jjs-util.h"

#include "ecma-function-object.h"

#include "annex.h"
#include "jcontext.h"

#if JJS_ANNEX_VMOD
static jjs_value_t annex_vmod_new (jjs_context_t* context_p, jjs_value_t name, jjs_value_t value);
static void annex_vmod_remove (jjs_context_t* context_p, jjs_value_t name);
static jjs_value_t
annex_vmod_handler (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_count);
static jjs_value_t
annex_vmod_resolve_handler (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_count);
static jjs_value_t
annex_vmod_exists_handler (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_count);
static jjs_value_t
annex_vmod_remove_handler (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_count);
static jjs_value_t annex_vmod_entry_exports (jjs_context_t* context_p, jjs_value_t entry);
static void annex_vmod_entry_update (jjs_context_t *context_p, jjs_value_t entry, jjs_value_t exports);
static bool annex_vmod_entry_is_ready (jjs_context_t *context_p, jjs_value_t entry);
static jjs_value_t annex_vmod_entry_new (jjs_context_t* context_p, bool ready, jjs_value_t exports);
static jjs_value_t annex_vmod_get_exports_from_config (jjs_context_t* context_p, jjs_value_t config);
#endif /* JJS_ANNEX_VMOD */

/**
 * Register a virtual module.
 *
 * After registration, the virtual module name can be used with require,
 * static import, dynamic import and require.resolve.
 *
 * The package name must be a string conforming to NPM's package name rules
 * and the package name must not already be registered. package,
 * \@scope/package and my-pack_age.xyz are valid package names.
 *
 * If the value is an object, it must be a vmod config object. It must contain
 * a format property of string "object". If not set, "object" is the default.
 * The "object" format expects an "exports" property containing the exports
 * of the package (object, function, primitive, etc). If using a config, the
 * vmod will be loaded in the scope of the register function call.
 *
 * If the value is a function, the module is registered, but not loaded. On the
 * first import or require of the package, the registered callback is invoked
 * with the expectation that a vmod config will be returned. The package will
 * be loaded based on that vmod config.
 *
 * The virtual module cache is per realm, just like CommonJS and ES Module
 * systems. The realm is consulted during require() of CommonJS and the
 * evaluate() step of ES module loading. There is no API to manipulate or
 * clear the cache.
 *
 * @param context_p JJS context
 * @param name name of the virtual module. non-string values will return an
 * exception
 * @param name_o name reference ownership
 * @param value callback function or vmod config object
 * @param value_o value reference ownership
 * @return undefined on success. exception on failure. return value must be freed
 * with jj_value_free.
 */
jjs_value_t
jjs_vmod (jjs_context_t* context_p, jjs_value_t name, jjs_value_ownership_t name_o, jjs_value_t value, jjs_value_ownership_t value_o)
{
  jjs_assert_api_enabled (context_p);
#if JJS_ANNEX_VMOD
  jjs_value_t result = annex_vmod_new (context_p, name, value);

  JJS_DISOWN (context_p, name, name_o);
  JJS_DISOWN (context_p, value, value_o);

  return result;
#else /* !JJS_ANNEX_VMOD */
  JJS_DISOWN (context_p, name, name_o);
  JJS_DISOWN (context_p, value, value_o);
  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VMOD_NOT_SUPPORTED));
#endif /* JJS_ANNEX_VMOD */
} /* jjs_vmod */

/**
 * @see jjs_vmod
 */
jjs_value_t
jjs_vmod_sz (jjs_context_t* context_p, const char *name, jjs_value_t value, jjs_value_ownership_t value_o)
{
  jjs_assert_api_enabled (context_p);
  return jjs_vmod (context_p, annex_util_create_string_utf8_sz (context_p, name), JJS_MOVE, value, value_o);
} /* jjs_vmod_sz */

/**
 * Returns the exports of the given package. This is the functional equivalent of calling
 * require on the package.
 *
 * If the package is loaded, the package exports have been cached and the exports will be
 * returned.
 *
 * If the package is not loaded, the user registered callback will be invoked and the
 * exports generated and cached. Then, the exports will be returned.
 *
 * @param context_p JJS context
 * @param name package name to resolve
 * @param name_o name reference ownership
 * @return on success, package exports; otherwise, an exception is thrown
 */
jjs_value_t
jjs_vmod_resolve (jjs_context_t* context_p, jjs_value_t name, jjs_value_ownership_t name_o)
{
  jjs_assert_api_enabled (context_p);
#if JJS_ANNEX_VMOD
  jjs_value_t result = jjs_annex_vmod_resolve (context_p, name);

  JJS_DISOWN (context_p, name, name_o);

  return result;
#else /* !JJS_ANNEX_VMOD */
  JJS_DISOWN (context_p, name, name_o);
  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VMOD_NOT_SUPPORTED));
#endif /* JJS_ANNEX_VMOD */
} /* jjs_vmod_resolve */

/**
 * @see jjs_vmod_resolve
 */
jjs_value_t
jjs_vmod_resolve_sz (jjs_context_t* context_p, const char *name)
{
  jjs_assert_api_enabled (context_p);
#if JJS_ANNEX_VMOD
  return jjs_vmod_resolve (context_p, annex_util_create_string_utf8_sz (context_p, name), JJS_MOVE);
#else /* !JJS_ANNEX_VMOD */
  JJS_UNUSED (name);
  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VMOD_NOT_SUPPORTED));
#endif /* JJS_ANNEX_VMOD */
} /* jjs_vmod_resolve_sz */

/**
 * Checks if a vmod package has been registered with vmod() or jjs_vmod().
 *
 * Note: a registered package is available for require() and import(), but the package
 * exports may or may not have been loaded into the vmod cache.
 *
 * @param context_p JJS context
 * @param name string package name
 * @param name_o name reference ownership
 * @return true if package is registered; otherwise, false
 */
bool
jjs_vmod_exists (jjs_context_t* context_p, jjs_value_t name, jjs_value_ownership_t name_o)
{
  jjs_assert_api_enabled (context_p);
#if JJS_ANNEX_VMOD
  jjs_value_t result = jjs_annex_vmod_exists (context_p, name);

  JJS_DISOWN(context_p, name, name_o);

  return result;
#else /* !JJS_ANNEX_VMOD */
  JJS_DISOWN (context_p, name, name_o);
  return false;
#endif /* JJS_ANNEX_VMOD */
} /* jjs_vmod_exists */

/**
 * @see jjs_vmod_exists
 */
bool
jjs_vmod_exists_sz (jjs_context_t* context_p, const char *name)
{
  jjs_assert_api_enabled (context_p);
  return jjs_vmod_exists (context_p, annex_util_create_string_utf8_sz (context_p, name), JJS_MOVE);
} /* jjs_vmod_exists */

/**
 * Unregister a vmod package.
 *
 * It is not recommended to remove package entries. If future requires or imports
 * try to use the package, their calls will fail.
 *
 * @param context_p JJS context
 * @param name string package name
 * @param name_o name reference ownership
 */
void
jjs_vmod_remove (jjs_context_t* context_p, jjs_value_t name, jjs_value_ownership_t name_o)
{
  jjs_assert_api_enabled (context_p);
#if JJS_ANNEX_VMOD
  annex_vmod_remove (context_p, name);

  JJS_DISOWN (context_p, name, name_o);
#else /* !JJS_ANNEX_VMOD */
  JJS_DISOWN (context_p, name, name_o);
#endif /* JJS_ANNEX_VMOD */
} /* jjs_vmod_remove */

/**
 * @see jjs_vmod_remove
 */
void
jjs_vmod_remove_sz (jjs_context_t* context_p, const char *name)
{
  jjs_assert_api_enabled (context_p);
  jjs_vmod_remove (context_p, annex_util_create_string_utf8_sz(context_p, name), JJS_MOVE);
} /* jjs_vmod_remove */

#if JJS_ANNEX_VMOD

/**
 * Create the vmod api to expose to JS.
 */
ecma_value_t
jjs_annex_vmod_create_api (jjs_context_t* context_p)
{
  JJS_UNUSED (context_p);
  ecma_object_t* vmod_p = ecma_op_create_external_function_object (context_p, annex_vmod_handler);

  annex_util_define_function (context_p, vmod_p, LIT_MAGIC_STRING_EXISTS, annex_vmod_exists_handler);
  annex_util_define_function (context_p, vmod_p, LIT_MAGIC_STRING_RESOLVE, annex_vmod_resolve_handler);
  annex_util_define_function (context_p, vmod_p, LIT_MAGIC_STRING_REMOVE, annex_vmod_remove_handler);

  return ecma_make_object_value (context_p, vmod_p);
} /* jjs_annex_vmod_create_api */

jjs_value_t
jjs_annex_vmod_resolve (jjs_context_t* context_p, jjs_value_t name)
{
  ecma_value_t entry = ecma_find_own_v (context_p, ecma_get_global_object (context_p)->vmod_cache, name);

  if (!ecma_is_value_found (entry))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "vmod is not registered");
  }

  if (annex_vmod_entry_is_ready (context_p, entry))
  {
    jjs_value_t result = annex_vmod_entry_exports (context_p, entry);

    ecma_free_value (context_p, entry);

    return result;
  }

  jjs_value_t realm = ecma_make_object_value (context_p, ecma_builtin_get_global (context_p));
  jjs_value_t function = annex_vmod_entry_exports (context_p, entry);
  JJS_ASSERT (jjs_value_is_function (context_p, function));
  jjs_value_t config = jjs_call (context_p, function, realm, NULL, 0);
  jjs_value_t exports = annex_vmod_get_exports_from_config (context_p, config);

  if (!jjs_value_is_exception (context_p, exports))
  {
    annex_vmod_entry_update (context_p, entry, exports);
  }

  jjs_value_free (context_p, function);
  jjs_value_free (context_p, config);
  ecma_free_value (context_p, entry);

  return exports;
} /* jjs_annex_vmod_resolve */

bool
jjs_annex_vmod_exists (jjs_context_t* context_p, jjs_value_t name)
{
  return jjs_value_is_string (context_p, name) && ecma_has_own_v (context_p, ecma_get_global_object (context_p)->vmod_cache, name);
} /* jjs_annex_vmod_exists */

static jjs_value_t
annex_vmod_entry_new (jjs_context_t* context_p, bool ready, jjs_value_t exports)
{
  jjs_value_t array = jjs_array (context_p, 2);

  ecma_set_index_v (context_p, array, 0, ecma_make_boolean_value (ready));
  ecma_set_index_v (context_p, array, 1, exports);

  return array;
} /* annex_vmod_entry_new */

static bool
annex_vmod_entry_is_ready (jjs_context_t *context_p, jjs_value_t entry)
{
  ecma_value_t ready = ecma_op_object_find_by_index (context_p, ecma_get_object_from_value (context_p, entry), 0);
  ecma_free_value (context_p, ready);

  return ecma_is_value_true (ready);
} /* annex_vmod_entry_is_ready */

static jjs_value_t
annex_vmod_entry_exports (jjs_context_t* context_p, jjs_value_t entry)
{
  ecma_value_t exports = ecma_op_object_find_by_index (context_p, ecma_get_object_from_value (context_p, entry), 1);

  if (ecma_is_value_found (exports))
  {
    return exports;
  }

  ecma_free_value (context_p, exports);

  return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "failed to get vmod entry exports");
} /* annex_vmod_entry_exports */

static void
annex_vmod_entry_update (jjs_context_t *context_p, jjs_value_t entry, jjs_value_t exports)
{
  ecma_set_index_v (context_p, entry, 0, ECMA_VALUE_TRUE);
  ecma_set_index_v (context_p, entry, 1, exports);
} /* annex_vmod_entry_update */

static void
annex_vmod_remove (jjs_context_t* context_p, jjs_value_t name)
{
  ecma_value_t vmod_cache = ecma_get_global_object (context_p)->vmod_cache;

  jjs_value_free (context_p, jjs_object_delete (context_p, vmod_cache, name));
} /* annex_vmod_remove */

static jjs_value_t
annex_vmod_new (jjs_context_t* context_p, jjs_value_t name, jjs_value_t value)
{
  ecma_value_t vmod_cache = ecma_get_global_object (context_p)->vmod_cache;

  if (!annex_util_is_valid_package_name (context_p, name))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "vmod name arg must be a valid package name");
  }

  if (ecma_has_own_v (context_p, vmod_cache, name))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "vmod name has already been registered");
  }

  if (ecma_op_is_callable (context_p, value))
  {
    jjs_value_t entry = annex_vmod_entry_new (context_p, false, value);

    ecma_set_v (vmod_cache, context_p, name, entry);
    jjs_value_free (context_p, entry);
  }
  else if (ecma_is_value_object (value))
  {
    jjs_value_t exports = annex_vmod_get_exports_from_config (context_p, value);

    if (jjs_value_is_exception (context_p, exports))
    {
      return exports;
    }

    jjs_value_t entry = annex_vmod_entry_new (context_p, true, exports);

    ecma_set_v (vmod_cache, context_p, name, entry);

    jjs_value_free (context_p, entry);
    jjs_value_free (context_p, exports);
  }
  else
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "expected value to be a function or vmod config object");
  }

  return ECMA_VALUE_UNDEFINED;
} /* annex_vmod_new */

static jjs_value_t
annex_vmod_handler (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_count)
{
  jjs_context_t *context_p = call_info_p->context_p;
  return annex_vmod_new (context_p, ecma_arg0 (args_p, args_count), ecma_arg1 (args_p, args_count));
} /* annex_vmod_handler */

static jjs_value_t
annex_vmod_resolve_handler (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_count)
{
  jjs_context_t *context_p = call_info_p->context_p;
  return jjs_annex_vmod_resolve (context_p, ecma_arg0 (args_p, args_count));
} /* annex_vmod_resolve_handler */

static jjs_value_t
annex_vmod_exists_handler (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_count)
{
  jjs_context_t *context_p = call_info_p->context_p;
  return jjs_boolean (context_p, jjs_annex_vmod_exists (context_p, ecma_arg0 (args_p, args_count)));
} /* annex_vmod_exists_handler */

static jjs_value_t
annex_vmod_remove_handler (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_count)
{
  jjs_context_t *context_p = call_info_p->context_p;
  annex_vmod_remove (context_p, ecma_arg0 (args_p, args_count));
  return ECMA_VALUE_UNDEFINED;
} /* annex_vmod_exists_handler */

static jjs_value_t
annex_vmod_get_exports_from_config (jjs_context_t* context_p, jjs_value_t config)
{
  if (!jjs_value_is_object (context_p, config))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "vmod callback return value must return an Object");
  }

  // format string -> format enum
  ecma_value_t format = ecma_find_own_m (context_p, config, LIT_MAGIC_STRING_FORMAT);
  jjs_annex_vmod_format_t format_type;

  if (format == ECMA_VALUE_NOT_FOUND)
  {
    format_type = JJS_ANNEX_VMOD_FORMAT_OBJECT;
  }
  else if (ecma_is_value_string (format))
  {
    ecma_string_t *format_p = ecma_get_string_from_value (context_p, format);

    if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_OBJECT))
    {
      format_type = JJS_ANNEX_VMOD_FORMAT_OBJECT;
    }
    else if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_COMMONJS))
    {
      format_type = JJS_ANNEX_VMOD_FORMAT_COMMONJS;
    }
    else if (ecma_compare_ecma_string_to_magic_id (format_p, LIT_MAGIC_STRING_MODULE))
    {
      format_type = JJS_ANNEX_VMOD_FORMAT_MODULE;
    }
    else
    {
      format_type = JJS_ANNEX_VMOD_FORMAT_UNKNOWN;
    }
  }
  else
  {
    format_type = JJS_ANNEX_VMOD_FORMAT_UNKNOWN;
  }

  ecma_free_value (context_p, format);

  // validate / return exports
  switch (format_type)
  {
    case JJS_ANNEX_VMOD_FORMAT_OBJECT:
    {
      ecma_value_t exports = ecma_find_own_m (context_p, config, LIT_MAGIC_STRING_EXPORTS);

      if (exports == ECMA_VALUE_NOT_FOUND)
      {
        return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "vmod config of type 'object' missing 'exports' property");
      }

      return exports;
    }
    case JJS_ANNEX_VMOD_FORMAT_MODULE:
      // TODO: implement module source
    case JJS_ANNEX_VMOD_FORMAT_COMMONJS:
      // TODO: implement commonjs source
    default:
      return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "vmod config contains an invalid 'format' property");
  }
} /* annex_vmod_get_exports_from_config */

#endif /* JJS_ANNEX_VMOD */
