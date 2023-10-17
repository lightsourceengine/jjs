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
  JJS_CONTEXT(commonjs_args) = ecma_string_ascii_sz ("module,exports,require,__filename,__dirname");
#endif /* JJS_COMMONJS */
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
  global_p->esm_cache = ECMA_VALUE_EMPTY;
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_annex_init_realm */

/**
 * Cleanup context for annex apis.
 */
void jjs_annex_finalize (void)
{
#if JJS_PMAP
  jjs_value_free (JJS_CONTEXT (pmap));
  jjs_value_free (JJS_CONTEXT (pmap_root));
#endif /* JJS_PMAP */
#if JJS_COMMONJS
  jjs_value_free (JJS_CONTEXT (commonjs_args));
#endif /* JJS_COMMONJS */
} /* jjs_annex_finalize */
