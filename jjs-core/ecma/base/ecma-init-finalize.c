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

#include "ecma-init-finalize.h"

#include "ecma-builtins.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-literal-storage.h"

#include "jcontext.h"
#include "jmem.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmainitfinalize Initialization and finalization of ECMA components
 * @{
 */

/**
 * Maximum number of GC loops on cleanup.
 */
#define JJS_GC_LOOP_LIMIT 100

/**
 * Initialize ECMA components
 */
void
ecma_init (void)
{
  if (JJS_CONTEXT(gc_mark_limit) != 0)
  {
    JJS_CONTEXT (ecma_gc_mark_recursion_limit) = JJS_CONTEXT(gc_mark_limit);
  }
  ecma_init_global_environment ();

#if JJS_PROPERTY_HASHMAP
  JJS_CONTEXT (ecma_prop_hashmap_alloc_state) = ECMA_PROP_HASHMAP_ALLOC_ON;
  JJS_CONTEXT (status_flags) &= (uint32_t) ~ECMA_STATUS_HIGH_PRESSURE_GC;
#endif /* JJS_PROPERTY_HASHMAP */

  if (JJS_CONTEXT(vm_stack_limit) != 0)
  {
    volatile int sp;
    JJS_CONTEXT (stack_base) = (uintptr_t) &sp;
  }

  ecma_job_queue_init ();

  JJS_CONTEXT (current_new_target_p) = NULL;

#if JJS_BUILTIN_TYPEDARRAY
  JJS_CONTEXT (arraybuffer_compact_allocation_limit) = 256;
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* ecma_init */

/**
 * Finalize ECMA components
 */
void
ecma_finalize (void)
{
  JJS_ASSERT (JJS_CONTEXT (current_new_target_p) == NULL);

  ecma_finalize_global_environment ();
  uint8_t runs = 0;

  do
  {
    ecma_gc_run ();
    if (++runs >= JJS_GC_LOOP_LIMIT)
    {
      jjs_fatal (JJS_FATAL_UNTERMINATED_GC_LOOPS);
    }
  } while (JJS_CONTEXT (ecma_gc_new_objects) != 0);

  jmem_cpointer_t *global_symbols_cp = JJS_CONTEXT (global_symbols_cp);

  for (uint32_t i = 0; i < ECMA_BUILTIN_GLOBAL_SYMBOL_COUNT; i++)
  {
    if (global_symbols_cp[i] != JMEM_CP_NULL)
    {
      ecma_deref_ecma_string (ECMA_GET_NON_NULL_POINTER (ecma_string_t, global_symbols_cp[i]));
    }
  }

  ecma_finalize_lit_storage ();
} /* ecma_finalize */

/**
 * @}
 * @}
 */
