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

#include "annex.h"
#include "jcontext.h"

#if JJS_MODULE_SYSTEM
/**
 * Module scope initialization hook.
 */
static void module_on_init_scope (ecma_module_t* module_p)
{
#if JJS_COMMONJS
 // For a non-native ES module, a require function (resolving relative to
 // the module's user_value or URL) is added to the module scope. If the
 // module is native, does not have a user_value or the require function
 // cannot otherwise be created, the require function is not added.

 if (module_p->header.u.cls.u2.module_flags & ECMA_MODULE_IS_NATIVE)
 {
   return;
 }

 ecma_value_t script_value = ((cbc_uint8_arguments_t *) module_p->u.compiled_code_p)->script_value;
 const cbc_script_t *script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);

 if (!(script_p->refs_and_type & CBC_SCRIPT_HAS_USER_VALUE))
 {
   return;
 }

 jjs_value_t require = jjs_annex_create_require (CBC_SCRIPT_GET_USER_VALUE (script_p));

 if (!jjs_value_is_exception (require))
 {
   ecma_property_value_t *value_p =
     ecma_create_named_data_property (module_p->scope_p,
                                      ecma_get_magic_string (LIT_MAGIC_STRING_REQUIRE),
                                      ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                      NULL);

   value_p->value = require;
 }

 jjs_value_free (require);
#else /* !JJS_COMMONJS */
 JJS_UNUSED (module_p);
#endif /* JJS_COMMONJS */
} /* module_on_init_scope */

#endif /* JJS_MODULE_SYSTEM */

/**
 * Initialize context for annex apis.
 */
void jjs_annex_init (void)
{
#if JJS_PMAP
  JJS_CONTEXT (pmap) = ECMA_VALUE_UNDEFINED;
  JJS_CONTEXT (pmap_root) = ECMA_VALUE_UNDEFINED;
#endif /* JJS_PMAP */

#if JJS_COMMONJS
  JJS_CONTEXT (commonjs_args) = ecma_string_ascii_sz ("module,exports,require,__filename,__dirname");
#endif /* JJS_COMMONJS */

#if JJS_MODULE_SYSTEM
  JJS_CONTEXT (module_on_init_scope_p) = module_on_init_scope;
#endif /* JJS_MODULE_SYSTEM */

#if JJS_VMOD
  JJS_CONTEXT (vmods) = ecma_create_object_with_null_proto ();
#endif /* JJS_VMOD */
} /* jjs_annex_init */

/**
 * Initialize realm for annex apis.
 *
 * @param global_p the realm object
 */
void jjs_annex_init_realm (ecma_global_object_t* global_p)
{
#if JJS_QUEUE_MICROTASK
  annex_util_define_function ((ecma_object_t*)global_p,
                              LIT_MAGIC_STRING_QUEUE_MICROTASK,
                              queue_microtask_handler);
#endif /* JJS_QUEUE_MICROTASK */

#if JJS_COMMONJS
  global_p->commonjs_cache = ecma_create_object_with_null_proto ();
  ecma_free_value (global_p->commonjs_cache);

  ecma_value_t fn = jjs_annex_create_require (ECMA_VALUE_UNDEFINED);

  annex_util_define_value ((ecma_object_t*)global_p, LIT_MAGIC_STRING_REQUIRE, fn);

  ecma_free_value (fn);
#endif /* JJS_COMMONJS */

#if JJS_MODULE_SYSTEM
  global_p->esm_cache = ecma_create_object_with_null_proto ();
  ecma_free_value (global_p->esm_cache);
#endif /* JJS_MODULE_SYSTEM */

#if JJS_VMOD
  global_p->vmod_cache = ecma_create_object_with_null_proto ();
  ecma_free_value (global_p->vmod_cache);
#endif /* JJS_VMOD */
} /* jjs_annex_init_realm */

/**
 * Cleanup context for annex apis.
 */
void jjs_annex_finalize (void)
{
#if JJS_MODULE_SYSTEM
  // the esm modules lifetime of the vm. in some cases, that I don't fully understand,
  // the module gc does not occur during the final memory cleanup and debug builds
  // assert. the problem has only been observed on windows for an import capi call
  // with relative paths (??). clearing the cache to a non-object, so ecma-gc does not
  // mark esm_cache, fixes the issue.
  //
  // in the future, jjs_esm_cleanup(realm) should be exposed for realm users.
  ecma_get_global_object ()->esm_cache = ECMA_VALUE_UNDEFINED;
#endif /* JJS_MODULE_SYSTEM */

#if JJS_PMAP
  jjs_value_free (JJS_CONTEXT (pmap));
  jjs_value_free (JJS_CONTEXT (pmap_root));
#endif /* JJS_PMAP */

#if JJS_COMMONJS
  jjs_value_free (JJS_CONTEXT (commonjs_args));
#endif /* JJS_COMMONJS */

#if JJS_VMOD
  jjs_value_free (JJS_CONTEXT (vmods));
#endif /* JJS_VMOD */
} /* jjs_annex_finalize */
