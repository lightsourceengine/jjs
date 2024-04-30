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

#include "jjs-annex-vmod.h"
#include "jjs-core.h"

#include "annex.h"
#include "jcontext.h"

#if JJS_MODULE_SYSTEM
/**
 * Module scope initialization hook.
 */
static void module_on_init_scope (jjs_context_t* context_p, ecma_module_t* module_p)
{
#if JJS_ANNEX_COMMONJS
 // For a non-native ES module, a require function (resolving relative to
 // the module's user_value or URL) is added to the module scope. If the
 // module is native, does not have a user_value or the require function
 // cannot otherwise be created, the require function is not added.

 if (module_p->header.u.cls.u2.module_flags & ECMA_MODULE_IS_SYNTHETIC)
 {
   return;
 }

 ecma_value_t script_value = ((cbc_uint8_arguments_t *) module_p->u.compiled_code_p)->script_value;
 const cbc_script_t *script_p = ECMA_GET_INTERNAL_VALUE_POINTER (context_p, cbc_script_t, script_value);

 if (!(script_p->refs_and_type & CBC_SCRIPT_HAS_USER_VALUE))
 {
   return;
 }

 jjs_value_t require = jjs_annex_create_require (context_p, CBC_SCRIPT_GET_USER_VALUE (script_p));

 if (!jjs_value_is_exception (context_p, require))
 {
   ecma_property_value_t *value_p =
     ecma_create_named_data_property (context_p,
                                      module_p->scope_p,
                                      ecma_get_magic_string (LIT_MAGIC_STRING_REQUIRE),
                                      ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                      NULL);

   value_p->value = require;
 }

 jjs_value_free (context_p, require);
#else /* !JJS_ANNEX_COMMONJS */
 JJS_UNUSED_ALL (context_p, module_p);
#endif /* JJS_ANNEX_COMMONJS */
} /* module_on_init_scope */

#endif /* JJS_MODULE_SYSTEM */

/**
 * Initialize context for annex apis.
 */
void jjs_annex_init (jjs_context_t* context_p)
{
  JJS_UNUSED (context_p);
#if JJS_ANNEX_PMAP
  context_p->pmap = ECMA_VALUE_UNDEFINED;
  context_p->pmap_root = ECMA_VALUE_UNDEFINED;
#endif /* JJS_ANNEX_PMAP */

#if JJS_ANNEX_COMMONJS
  context_p->commonjs_args = ecma_string_ascii_sz (context_p, "module,exports,require,__filename,__dirname");
#endif /* JJS_ANNEX_COMMONJS */

#if JJS_MODULE_SYSTEM
  context_p->module_on_init_scope_p = module_on_init_scope;
#endif /* JJS_MODULE_SYSTEM */

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM
  context_p->module_on_load_cb = jjs_esm_default_on_load_cb;
  context_p->module_on_load_user_p = NULL;

  context_p->module_on_resolve_cb = jjs_esm_default_on_resolve_cb;
  context_p->module_on_resolve_user_p = NULL;
#endif /* JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM */

#if JJS_ANNEX_ESM
  context_p->module_import_meta_callback_p = jjs_esm_default_on_import_meta_cb;
  context_p->module_import_meta_callback_user_p = NULL;
  context_p->module_import_callback_p = jjs_esm_default_on_import_cb;
  context_p->module_import_callback_user_p = NULL;
#endif /* JJS_ANNEX_ESM */
} /* jjs_annex_init */

/**
 * Initialize realm for annex apis.
 *
 * @param context_p JJS context
 * @param global_p the realm object
 */
void jjs_annex_init_realm (jjs_context_t* context_p, ecma_global_object_t* global_p)
{
  JJS_UNUSED (context_p);

#if JJS_ANNEX_QUEUE_MICROTASK || JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM || JJS_ANNEX_VMOD
  ecma_object_t* global_object_p = (ecma_object_t*) global_p;
#else /* !(JJS_ANNEX_QUEUE_MICROTASK || JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM || JJS_ANNEX_VMOD) */
  JJS_UNUSED (global_p);
#endif /* JJS_ANNEX_QUEUE_MICROTASK || JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM || JJS_ANNEX_VMOD */

#if JJS_ANNEX_QUEUE_MICROTASK
  annex_util_define_function (context_p, global_object_p, LIT_MAGIC_STRING_QUEUE_MICROTASK, queue_microtask_handler);
#endif /* JJS_ANNEX_QUEUE_MICROTASK */

#if JJS_ANNEX_COMMONJS
  global_p->commonjs_cache = ecma_create_object_with_null_proto (context_p);
  ecma_free_value (context_p, global_p->commonjs_cache);

  ecma_value_t fn = jjs_annex_create_require (context_p, ECMA_VALUE_UNDEFINED);

  if (jjs_value_is_exception (context_p, fn))
  {
    jjs_log (context_p, JJS_LOG_LEVEL_ERROR, "failed to create global require");
    jjs_fatal (JJS_FATAL_FAILED_ASSERTION);
  }

  annex_util_define_value (context_p, global_object_p, LIT_MAGIC_STRING_REQUIRE, fn, JJS_MOVE);
#endif /* JJS_ANNEX_COMMONJS */

#if JJS_ANNEX_ESM
  global_p->esm_cache = ecma_create_object_with_null_proto (context_p);
  ecma_free_value (context_p, global_p->esm_cache);
#endif /* JJS_ANNEX_ESM */

#if JJS_ANNEX_VMOD
  global_p->vmod_cache = ecma_create_object_with_null_proto (context_p);
  ecma_free_value (context_p, global_p->vmod_cache);
#endif /* JJS_ANNEX_VMOD */
} /* jjs_annex_init_realm */

/**
 * Cleanup context for annex apis.
 */
void jjs_annex_finalize (jjs_context_t* context_p)
{
  JJS_UNUSED (context_p);

#if JJS_ANNEX_ESM
  // the esm modules lifetime of the vm. in some cases, that I don't fully understand,
  // the module gc does not occur during the final memory cleanup and debug builds
  // assert. the problem has only been observed on windows for an import capi call
  // with relative paths (??). clearing the cache to a non-object, so ecma-gc does not
  // mark esm_cache, fixes the issue.
  //
  // in the future, jjs_esm_cleanup(realm) should be exposed for realm users.
  ecma_get_global_object (context_p)->esm_cache = ECMA_VALUE_UNDEFINED;
#endif /* JJS_MODULE_SYSTEM */

#if JJS_ANNEX_PMAP
  jjs_value_free (context_p, context_p->pmap);
  jjs_value_free (context_p, context_p->pmap_root);
#endif /* JJS_ANNEX_PMAP */

#if JJS_ANNEX_COMMONJS
  jjs_value_free (context_p, context_p->commonjs_args);
#endif /* JJS_ANNEX_COMMONJS */
} /* jjs_annex_finalize */
