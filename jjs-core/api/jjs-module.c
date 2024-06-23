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
#include "jjs.h"

#include "jjs-util.h"

#include "ecma-builtin-helpers.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-lex-env.h"

#include "jcontext.h"
#include "lit-char-helpers.h"

#if JJS_MODULE_SYSTEM
/**
 * Resolve callback used by jjs_module_link() when the user specifies a NULL callback.
 *
 * If the callback is NULL, the user's module contains no import statements, thus nothing
 * for the module link to resolve. In this situation, this callback is never called. If it
 * is, the user has import statements that they need to resolve by supplying their own
 * custom callback.
 */
static jjs_value_t
jjs_module_link_default_callback (jjs_context_t* context_p, const jjs_value_t specifier, const jjs_value_t referrer, void *user_p)
{
  JJS_UNUSED_ALL (specifier, referrer, user_p);
  return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "Provide a callback to jjs_module_link to resolve import specifiers.");
} /* jjs_module_link_default_callback */
#endif /* JJS_MODULE_SYSTEM */

/**
 * Link modules to their dependencies. The dependencies are resolved by a user callback.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return true - if linking is successful, error - otherwise
 */
jjs_value_t
jjs_module_link (jjs_context_t* context_p, /**< JJS context */
                 const jjs_value_t module, /**< root module */
                 jjs_module_link_cb_t callback, /**< resolve module callback */
                 void *user_p) /**< pointer passed to the resolve callback */
{
  jjs_assert_api_enabled (context_p);

#if JJS_MODULE_SYSTEM
  if (callback == NULL)
  {
    callback = jjs_module_link_default_callback;
    user_p = NULL;
  }

  ecma_module_t *module_p = ecma_module_get_resolved_module (context_p, module);

  if (module_p == NULL)
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_NOT_MODULE));
  }

  return jjs_return (context_p, ecma_module_link (context_p, module_p, callback, user_p));
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED_ALL (module, callback, user_p);

  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_link */

/**
 * Evaluate a module and its dependencies. The module must be in linked state.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return result of module bytecode execution - if evaluation was successful
 *         error - otherwise
 */
jjs_value_t
jjs_module_evaluate (jjs_context_t* context_p, /**< JJS context */
                     const jjs_value_t module) /**< root module */
{
  jjs_assert_api_enabled (context_p);

#if JJS_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (context_p, module);

  if (module_p == NULL)
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_NOT_MODULE));
  }

  if (module_p->header.u.cls.u1.module_state != JJS_MODULE_STATE_LINKED)
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_MUST_BE_IN_LINKED_STATE));
  }

  return jjs_return (context_p, ecma_module_evaluate (context_p, module_p));
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED (module);

  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_evaluate */

/**
 * Returns the current status of a module
 *
 * @return current status - if module is a module,
 *         JJS_MODULE_STATE_INVALID - otherwise
 */
jjs_module_state_t
jjs_module_state (jjs_context_t* context_p, /**< JJS context */
                  const jjs_value_t module) /**< module object */
{
  jjs_assert_api_enabled (context_p);

#if JJS_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (context_p, module);

  if (module_p == NULL)
  {
    return JJS_MODULE_STATE_INVALID;
  }

  return (jjs_module_state_t) module_p->header.u.cls.u1.module_state;
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED (module);

  return JJS_MODULE_STATE_INVALID;
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_state */

/**
 * Sets a callback which is called after a module state is changed to linked, evaluated, or error.
 */
void
jjs_module_on_state_changed (jjs_context_t* context_p, /**< JJS context */
                             jjs_module_state_changed_cb_t callback, /**< callback */
                             void *user_p) /**< pointer passed to the callback */
{
  jjs_assert_api_enabled (context_p);

#if JJS_MODULE_SYSTEM
  context_p->module_state_changed_callback_p = callback;
  context_p->module_state_changed_callback_user_p = user_p;
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED_ALL (callback, user_p);
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_on_state_changed */

/**
 * Sets a callback which is called when an import.meta expression of a module is evaluated the first time.
 */
void
jjs_module_on_import_meta (jjs_context_t* context_p, /**< JJS context */
                           jjs_module_import_meta_cb_t callback, /**< callback */
                           void *user_p) /**< pointer passed to the callback */
{
  jjs_assert_api_enabled (context_p);

#if JJS_MODULE_SYSTEM
  context_p->module_import_meta_callback_p = callback;
  context_p->module_import_meta_callback_user_p = user_p;
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED_ALL (callback, user_p);
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_on_import_meta */

/**
 * Returns the number of import/export requests of a module
 *
 * @return number of import/export requests of a module
 */
size_t
jjs_module_request_count (jjs_context_t* context_p, /**< JJS context */
                          const jjs_value_t module) /**< module */
{
  jjs_assert_api_enabled (context_p);

#if JJS_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (context_p, module);

  if (module_p == NULL)
  {
    return 0;
  }

  size_t number_of_requests = 0;

  ecma_module_node_t *node_p = module_p->imports_p;

  while (node_p != NULL)
  {
    number_of_requests++;
    node_p = node_p->next_p;
  }

  return number_of_requests;
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED (module);

  return 0;
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_request_count */

/**
 * Returns the module request specified by the request_index argument
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return string - if the request has not been resolved yet,
 *         module object - if the request has been resolved successfully,
 *         error - otherwise
 */
jjs_value_t
jjs_module_request (jjs_context_t* context_p, /**< JJS context */
                    const jjs_value_t module, /**< module */
                    size_t request_index) /**< request index */
{
  jjs_assert_api_enabled (context_p);

#if JJS_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (context_p, module);

  if (module_p == NULL)
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_NOT_MODULE));
  }

  ecma_module_node_t *node_p = module_p->imports_p;

  while (node_p != NULL)
  {
    if (request_index == 0)
    {
      return ecma_copy_value (context_p, node_p->u.path_or_module);
    }

    --request_index;
    node_p = node_p->next_p;
  }

  return jjs_throw_sz (context_p, JJS_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_REQUEST_IS_NOT_AVAILABLE));
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED_ALL (module, request_index);
  return jjs_throw_sz (context_p, JJS_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_request */

/**
 * Returns the namespace object of a module
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return object - if namespace object is available,
 *         error - otherwise
 */
jjs_value_t
jjs_module_namespace (jjs_context_t* context_p, /**< JJS context */
                      const jjs_value_t module) /**< module */
{
  jjs_assert_api_enabled (context_p);

#if JJS_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (context_p, module);

  if (module_p == NULL)
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_NOT_MODULE));
  }

  if (module_p->header.u.cls.u1.module_state < JJS_MODULE_STATE_LINKED
      || module_p->header.u.cls.u1.module_state > JJS_MODULE_STATE_EVALUATED)
  {
    return jjs_throw_sz (context_p, JJS_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_NAMESPACE_OBJECT_IS_NOT_AVAILABLE));
  }

  JJS_ASSERT (module_p->namespace_object_p != NULL);

  ecma_ref_object (module_p->namespace_object_p);
  return ecma_make_object_value (context_p, module_p->namespace_object_p);
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED (module);

  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_namespace */

/**
 * Sets the callback which is called when dynamic imports are resolved
 */
void
jjs_module_on_import (jjs_context_t* context_p, /**< JJS context */
                      jjs_module_import_cb_t callback_p, /**< callback which handles
                                                          *   dynamic import calls */
                      void *user_p) /**< user pointer passed to the callback */
{
  jjs_assert_api_enabled (context_p);

#if JJS_MODULE_SYSTEM
  context_p->module_import_callback_p = callback_p;
  context_p->module_import_callback_user_p = user_p;
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED_ALL (callback_p, user_p);
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_on_import */

/**
 * Creates a native module with a list of exports. The initial state of the module is linked.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return native module - if the module is successfully created,
 *         error - otherwise
 */
jjs_value_t
jjs_synthetic_module (jjs_context_t* context_p, /**< JJS context */
                      jjs_synthetic_module_evaluate_cb_t callback, /**< evaluation callback for
                                                                    *   native modules */
                      const jjs_value_t *const exports_p, /**< list of the exported bindings of the module,
                                                            *   must be valid string identifiers */
                      jjs_size_t export_count, /**< number of exports in the exports_p list */
                      jjs_own_t exports_o) /**< exports array resource ownership */
{
  jjs_assert_api_enabled (context_p);

#if JJS_MODULE_SYSTEM
  ecma_object_t *global_object_p = ecma_builtin_get_global (context_p);
  ecma_object_t *scope_p = ecma_create_decl_lex_env (context_p, ecma_get_global_environment (context_p, global_object_p));
  ecma_module_names_t *local_exports_p = NULL;

  for (size_t i = 0; i < export_count; i++)
  {
    if (!ecma_is_value_string (exports_p[i]))
    {
      ecma_deref_object (scope_p);
      ecma_module_release_module_names (context_p, local_exports_p);
      jjs_disown_value_array (context_p, exports_p, export_count, exports_o);
      return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_EXPORTS_MUST_BE_STRING_VALUES));
    }

    ecma_string_t *name_str_p = ecma_get_string_from_value (context_p, exports_p[i]);

    bool valid_identifier = false;

    ECMA_STRING_TO_UTF8_STRING (context_p, name_str_p, name_start_p, name_size);

    if (name_size > 0)
    {
      const lit_utf8_byte_t *name_p = name_start_p;
      const lit_utf8_byte_t *name_end_p = name_start_p + name_size;
      lit_code_point_t code_point;

      lit_utf8_size_t size = lit_read_code_point_from_cesu8 (name_p, name_end_p, &code_point);

      if (lit_code_point_is_identifier_start (code_point))
      {
        name_p += size;

        valid_identifier = true;

        while (name_p < name_end_p)
        {
          size = lit_read_code_point_from_cesu8 (name_p, name_end_p, &code_point);

          if (!lit_code_point_is_identifier_part (code_point))
          {
            valid_identifier = false;
            break;
          }

          name_p += size;
        }
      }
    }

    ECMA_FINALIZE_UTF8_STRING (context_p, name_start_p, name_size);

    if (!valid_identifier)
    {
      ecma_deref_object (scope_p);
      ecma_module_release_module_names (context_p, local_exports_p);
      jjs_disown_value_array (context_p, exports_p, export_count, exports_o);
      return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_EXPORTS_MUST_BE_VALID_IDENTIFIERS));
    }

    if (ecma_find_named_property (context_p, scope_p, name_str_p) != NULL)
    {
      continue;
    }

    ecma_create_named_data_property (context_p, scope_p, name_str_p, ECMA_PROPERTY_FLAG_WRITABLE, NULL);

    ecma_module_names_t *new_export_p;
    new_export_p = (ecma_module_names_t *) jmem_heap_alloc_block (context_p, sizeof (ecma_module_names_t));

    new_export_p->next_p = local_exports_p;
    local_exports_p = new_export_p;

    ecma_ref_ecma_string (name_str_p);
    new_export_p->imex_name_p = name_str_p;

    ecma_ref_ecma_string (name_str_p);
    new_export_p->local_name_p = name_str_p;
  }

  ecma_module_t *module_p = ecma_module_create (context_p);

  module_p->header.u.cls.u2.module_flags |= ECMA_MODULE_IS_SYNTHETIC;
  module_p->scope_p = scope_p;
  module_p->local_exports_p = local_exports_p;
  module_p->u.callback = callback;

  ecma_deref_object (scope_p);

  jjs_disown_value_array (context_p, exports_p, export_count, exports_o);
  return ecma_make_object_value (context_p, &module_p->header.object);

#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED_ALL (callback, exports_p, export_count);

  jjs_disown_value_array (context_p, exports_p, export_count, exports_o);
  return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_synthetic_module */

/**
 * Sets the value of an export which belongs to a native module.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         error - otherwise
 */
jjs_value_t
jjs_synthetic_module_set_export (jjs_context_t* context_p, /**< JJS context */
                                 jjs_value_t module, /**< a synthetic module object */
                                 const jjs_value_t export_name, /**< string identifier of the export */
                                 const jjs_value_t value, /**< new value of the export */
                                 jjs_own_t value_o) /**< value resource ownership */
{
  jjs_assert_api_enabled (context_p);
  jjs_value_t result;
#if JJS_MODULE_SYSTEM
  ecma_property_t *property_p;
  ecma_module_t *module_p = ecma_module_get_resolved_module (context_p, module);

  if (module_p == NULL)
  {
    result = jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_NOT_MODULE));
  }
  else if (!(module_p->header.u.cls.u2.module_flags & ECMA_MODULE_IS_SYNTHETIC) || !ecma_is_value_string (export_name)
      || ecma_is_value_exception (value))
  {
    result = jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }
  else if (module_p->header.u.cls.u1.module_state == JJS_MODULE_STATE_EVALUATED
      || module_p->header.u.cls.u1.module_state == JJS_MODULE_STATE_ERROR)
  {
    result = jjs_throw_sz (context_p, JJS_ERROR_TYPE, "Cannot set exports on a module in evaluated or error state.");
  }
  else
  {
    property_p =
      ecma_find_named_property (context_p, module_p->scope_p, ecma_get_string_from_value (context_p, export_name));

    if (property_p == NULL)
    {
      result = jjs_throw_sz (context_p, JJS_ERROR_REFERENCE, ecma_get_error_msg (ECMA_ERR_UNKNOWN_EXPORT));
    }
    else
    {
      ecma_named_data_property_assign_value (context_p, module_p->scope_p, ECMA_PROPERTY_VALUE_PTR (property_p), value);
      result = ECMA_VALUE_TRUE;
    }
  }
#else /* !JJS_MODULE_SYSTEM */
  JJS_UNUSED_ALL (module, export_name, value);
  result = jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JJS_MODULE_SYSTEM */

  jjs_disown_value (context_p, value, value_o);
  return result;
} /* jjs_synthetic_module_set_export */

jjs_value_t
jjs_synthetic_module_set_export_sz (jjs_context_t* context_p,
                                    jjs_value_t module,
                                    const char *export_name,
                                    const jjs_value_t value,
                                    jjs_own_t value_o)
{
  jjs_assert_api_enabled (context_p);
  jjs_value_t key = jjs_string_utf8_sz (context_p, export_name);
  return jjs_util_escape (context_p, jjs_synthetic_module_set_export (context_p, module, key, value, value_o), key);
}
