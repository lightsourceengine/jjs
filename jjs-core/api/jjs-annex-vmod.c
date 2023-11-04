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
#include "jjs-port.h"

#include "ecma-function-object.h"

#include "annex.h"
#include "jcontext.h"

#if JJS_VMOD
static jjs_value_t
vmod_create_handler (const jjs_call_info_t* call_info_p, const jjs_value_t args_p[], jjs_length_t args_count);
static jjs_object_native_info_t vmod_create_id = { 0 };
static jjs_object_native_info_t vmod_create_context_id = { 0 };
#endif /* JJS_VMOD */

/**
 * Register a virtual module.
 *
 * After registration, the virtual module name can be used with require,
 * static import, dynamic import and require.resolve.
 *
 * The create function will be called when the virtual module is first
 * loaded. If successful, the result is cached for future load requests.
 *
 * CommonJS and ES Module systems will look at the registered virtual
 * modules first. If the specifier is registered, resolve() and load()
 * will not be called dur the module load.
 *
 * The virtual module cache is per realm, just like CommonJS and ES Module
 * systems. The realm is consulted during require() of CommonJS and the
 * evaluate() step of ES module loading. There is no API to manipulate or
 * clear the cache.
 *
 * @param name name of the virtual module. non-string values will return an
 * exception
 * @param create_function function to call when the virtual module is loaded.
 * if the value is not a callable function, an exception will be returned.
 * @return true on success. exception on failure. return value must be freed
 * with jj_value_free.
 */
jjs_value_t
jjs_vmod (jjs_value_t name, jjs_value_t create_function)
{
  jjs_assert_api_enabled ();
#if JJS_VMOD
  if (!jjs_value_is_string (name) || jjs_string_length (name) == 0)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "name arg must be a non-empty string");
  }

  if (!ecma_op_is_callable (create_function))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "create arg must be callable");
  }

  ecma_value_t vmods = JJS_CONTEXT (vmods);

  if (ecma_has_own_v (vmods, name))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "vmod already registered");
  }

  ecma_set_v (vmods, name, create_function);

  return ECMA_VALUE_TRUE;
#else /* !JJS_VMOD */
  JJS_UNUSED (name);
  JJS_UNUSED (create_function);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VMOD_NOT_SUPPORTED));
#endif /* jjs_vmod */
} /* jjs_vmod_function */

/**
 * Register a virtual module with a native create callback.
 *
 * @see jjs_vmod
 *
 * @param name virtual module name. non-string values will return an exception.
 * @param create_cb native create callback. NULL will return an exception.
 * @param user_p pointer passed to the native create callback when the
 * virtual module is loaded.
 * @return true on success. exception on failure. return value must be freed
 * with jj_value_free.
 */
jjs_value_t
jjs_vmod_native (jjs_value_t name, jjs_vmod_create_cb_t create_cb, void* user_p)
{
  jjs_assert_api_enabled ();
#if JJS_VMOD
  if (create_cb == NULL)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "create_cb arg cannot be NULL");
  }

  jjs_value_t fn = jjs_function_external (vmod_create_handler);

  jjs_object_set_native_ptr (fn, &vmod_create_id, *((void**) &create_cb));

  if (user_p)
  {
    jjs_object_set_native_ptr (fn, &vmod_create_context_id, user_p);
  }

  jjs_value_t result = jjs_vmod (name, fn);

  jjs_value_free (fn);

  return result;
#else /* !JJS_VMOD */
  JJS_UNUSED (name);
  JJS_UNUSED (create_cb);
  JJS_UNUSED (user_p);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VMOD_NOT_SUPPORTED));
#endif /* JJS_VMOD */
} /* jjs_vmod_native */

/**
 * Register a virtual module with a native create callback.
 *
 * @see jjs_vmod
 *
 * @param name_p virtual module name. NULL will return an exception.
 * @param create_cb native create callback. NULL will return an exception.
 * @param user_p pointer passed to the native create callback when the
 * virtual module is loaded.
 * @return true on success. exception on failure. return value must be freed
 * with jj_value_free.
 */
jjs_value_t
jjs_vmod_native_sz (const char* name_p, jjs_vmod_create_cb_t create_cb, void* user_p)
{
  jjs_assert_api_enabled ();
#if JJS_VMOD
  jjs_value_t name = annex_util_create_string_utf8_sz (name_p);
  jjs_value_t result = jjs_vmod_native (name, create_cb, user_p);

  jjs_value_free (name);

  return result;
#else /* !JJS_VMOD */
  JJS_UNUSED (name_p);
  JJS_UNUSED (create_cb);
  JJS_UNUSED (user_p);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VMOD_NOT_SUPPORTED));
#endif /* JJS_VMOD */
} /* jjs_vmod_native_sz */

/**
 * Check if a virtual module is registered.
 *
 * @param name virtual module name. non-string values will return false.
 * @return true if the virtual module is registered. false otherwise.
 */
bool
jjs_vmod_exists (jjs_value_t name)
{
  jjs_assert_api_enabled ();
#if JJS_VMOD
  return ecma_is_value_string (name) ? jjs_annex_vmod_is_registered (name) : false;
#else /* !JJS_VMOD */
  JJS_UNUSED (name);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VMOD_NOT_SUPPORTED));
#endif /* JJS_VMOD */
} /* jjs_vmod_exists */

/**
 * Check if a virtual module is registered.
 *
 * @param name_p virtual module name. NULL or empty string values will return false.
 * @return true if the virtual module is registered. false otherwise.
 */
bool
jjs_vmod_exists_sz (const char* name_p)
{
  jjs_assert_api_enabled ();
#if JJS_VMOD
  jjs_value_t name = annex_util_create_string_utf8_sz (name_p);
  bool result = jjs_vmod_exists (name);

  jjs_value_free (name);

  return result;
#else /* !JJS_VMOD */
  JJS_UNUSED (name_p);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VMOD_NOT_SUPPORTED));
#endif /* JJS_VMOD */
} /* jjs_vmod_exists_sz */

#if JJS_VMOD

/**
 * Check if a module name has a virtual module registration in the
 * current context.
 *
 * This function does not check the realm cache.
 */
bool
jjs_annex_vmod_is_registered (ecma_value_t name)
{
  return ecma_has_own_v (JJS_CONTEXT (vmods), name);
} /* jjs_annex_vmod_is_registered */

/**
 * Get the exports of a virtual module.
 *
 * If the module is in the realm cache, the cached value is returned. Otherwise,
 * the module is created and cached.
 *
 * Note: this function assumes jjs_annex_vmod_is_registered() has been called
 */
ecma_value_t
jjs_annex_vmod_exports (ecma_value_t name)
{
  JJS_ASSERT (jjs_annex_vmod_is_registered (name));

  ecma_global_object_t* realm = ecma_get_global_object ();
  ecma_value_t cached = ecma_find_own_v (realm->vmod_cache, name);

  if (ecma_is_value_found (cached))
  {
    return cached;
  }

  ecma_free_value (cached);

  ecma_value_t create_function = ecma_find_own_v (JJS_CONTEXT (vmods), name);
  jjs_value_t result = jjs_call (create_function, ECMA_VALUE_UNDEFINED, &name, 1);

  ecma_free_value (create_function);

  if (jjs_value_is_exception (result))
  {
    return result;
  }

  ecma_value_t exports = ecma_find_own_m (result, LIT_MAGIC_STRING_EXPORTS);

  jjs_value_free (result);

  if (ecma_is_value_found (exports))
  {
    ecma_set_v (realm->vmod_cache, name, exports);
    return exports;
  }

  ecma_free_value (exports);

  return jjs_throw_sz (JJS_ERROR_TYPE, "vmod create function must return an object with an exports property");
} /* jjs_annex_vmod_exports */

/**
 * Binding for the native version of the virtual module create callback.
 *
 * The native callback and it's user pointer context is stored in the function
 * object instance's native pointer.
 */
static jjs_value_t
vmod_create_handler (const jjs_call_info_t* call_info_p, const jjs_value_t args_p[], jjs_length_t args_count)
{
  JJS_ASSERT (args_count > 0);
  JJS_ASSERT (jjs_value_is_string (args_p[0]));

  jjs_vmod_create_cb_t create;

  *(void **) (&create) = jjs_object_get_native_ptr (call_info_p->function, &vmod_create_id);

  JJS_ASSERT (create != NULL);

  if (create == NULL)
  {
    return jjs_throw_sz (JJS_ERROR_COMMON, "missing vmod create callback");
  }

  void* user_p = jjs_object_get_native_ptr (call_info_p->function, &vmod_create_context_id);

  return create (args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED, user_p);
} /* vmod_create_handler */

#endif /* JJS_VMOD */
