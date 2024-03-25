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

#include "jjs-context-init.h"

#include "jcontext.h"

/**
 * Computes the gc_limit when JJS_DEFAULT_GC_LIMIT is 0.
 *
 * See: JJS_DEFAULT_GC_LIMIT
 */
#define JJS_COMPUTE_GC_LIMIT(HEAP_SIZE) \
  JJS_MIN ((HEAP_SIZE) / JJS_DEFAULT_MAX_GC_LIMIT_DIVISOR, JJS_DEFAULT_MAX_GC_LIMIT)

#if JJS_VM_HEAP_STATIC
/**
 * Static heap.
 */
uint8_t jjs_static_heap [JJS_DEFAULT_VM_HEAP_SIZE * 1024] JJS_ATTR_ALIGNED (JMEM_ALIGNMENT) JJS_ATTR_STATIC_HEAP;
#endif /* JJS_VM_HEAP_STATIC */

/*
 * Setup the global context object.
 *
 * Any code that accesses the context needs to set this up. In core, jjs_init() makes
 * the call. If the tests are not using the full vm, they must call this method to
 * initialize the context.
 */
jjs_context_status_t
jjs_context_init (const jjs_context_options_t* options_p)
{
  jjs_context_options_t default_options;

  if (options_p == NULL)
  {
    options_p = jjs_context_options_init (&default_options);
  }

  if (JJS_CONTEXT (context_flags) & JJS_CONTEXT_INITIALIZED)
  {
    return JJS_CONTEXT_STATUS_ALREADY_INITIALIZED;
  }

  // context should be zero'd out at program init or jjs_cleanup
  JJS_ASSERT (JJS_CONTEXT (heap_p) == NULL);

  if (JJS_CONTEXT (heap_p) != NULL)
  {
    return JJS_CONTEXT_STATUS_CHAOS;
  }

  uint32_t context_flags = options_p->context_flags;

  if (options_p->external_heap)
  {
    context_flags |= JJS_CONTEXT_USING_EXTERNAL_HEAP;
  }

  if ((context_flags & JJS_CONTEXT_USING_EXTERNAL_HEAP) && options_p->external_heap == NULL)
  {
    return JJS_CONTEXT_STATUS_INVALID_EXTERNAL_HEAP;
  }

#if JJS_VM_STACK_STATIC
  // user cannot change the default
  if (options_p->vm_stack_limit != JJS_DEFAULT_VM_STACK_LIMIT)
  {
    return JJS_CONTEXT_STATUS_IMMUTABLE_STACK_LIMIT;
  }
#endif /* JJS_VM_STACK_STATIC */

#if JJS_VM_HEAP_STATIC
  // user cannot change the default
  if (options_p->vm_heap_size != JJS_DEFAULT_VM_HEAP_SIZE || context_flags & JJS_CONTEXT_USING_EXTERNAL_HEAP)
  {
    return JJS_CONTEXT_STATUS_IMMUTABLE_HEAP_SIZE;
  }
#endif /* JJS_VM_HEAP_STATIC */

  JJS_CONTEXT (vm_stack_limit) = options_p->vm_stack_limit * 1024;
  // TODO: check aligned?
  JJS_CONTEXT (vm_heap_size) = options_p->vm_heap_size * 1024;
  JJS_CONTEXT (gc_limit) =
    (options_p->gc_limit == 0) ? JJS_COMPUTE_GC_LIMIT (JJS_CONTEXT (vm_heap_size)) : (options_p->gc_limit * 1024);
  JJS_CONTEXT (gc_mark_limit) = options_p->gc_mark_limit;
  JJS_CONTEXT (gc_new_objects_fraction) = options_p->gc_new_objects_fraction;

  void* block;
#if JJS_VM_HEAP_STATIC
  block = &jjs_static_heap;
#else /* !JJS_VM_HEAP_STATIC */
  if (context_flags & JJS_CONTEXT_USING_EXTERNAL_HEAP)
  {
    block = options_p->external_heap;
    JJS_CONTEXT (heap_free_cb) = options_p->external_heap_free_cb;
  }
  else
  {
    block = calloc (JJS_CONTEXT (vm_heap_size), 1);
  }
#endif /* JJS_VM_HEAP_STATIC */
  JJS_CONTEXT (heap_p) = block;

  context_flags |= JJS_CONTEXT_INITIALIZED;
  JJS_CONTEXT (context_flags) = context_flags;

  return JJS_CONTEXT_STATUS_OK;
}

/**
 * Cleanup the context.
 */
void
jjs_context_cleanup (void)
{
#if JJS_VM_HEAP_STATIC
  memset (&jjs_static_heap, 0, sizeof (jjs_static_heap));
#else /* !JJS_VM_HEAP_STATIC */
  if ((JJS_CONTEXT (context_flags) & JJS_CONTEXT_USING_EXTERNAL_HEAP))
  {
    if (JJS_CONTEXT (heap_free_cb))
    {
      JJS_CONTEXT (heap_free_cb) (JJS_CONTEXT (heap_p), JJS_CONTEXT (heap_free_user_p));
    }
  }
  else
  {
    free (JJS_CONTEXT (heap_p));
  }
#endif /* JJS_VM_HEAP_STATIC */
  memset (&JJS_CONTEXT_STRUCT, 0, sizeof (JJS_CONTEXT_STRUCT));
}
