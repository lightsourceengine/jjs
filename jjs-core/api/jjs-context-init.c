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

#include "jjs-platform.h"
#include "jjs-stream.h"
#include "jjs-util.h"

#include "jcontext.h"

/**
 * Computes the gc_limit when JJS_DEFAULT_GC_LIMIT is 0.
 *
 * See: JJS_DEFAULT_GC_LIMIT
 */
#define JJS_COMPUTE_GC_LIMIT(HEAP_SIZE) JJS_MIN ((HEAP_SIZE) / JJS_DEFAULT_MAX_GC_LIMIT_DIVISOR, 8192)

static uint32_t
get_context_option_u32 (const jjs_optional_u32_t * optional, uint32_t default_value)
{
  return optional->has_value && optional->value > 0 ? optional->value : default_value;
}

/**
 * Initialize the allocators in the context.
 */
static void
context_set_scratch_allocator (jjs_context_t* context_p, uint8_t *scratch_block, uint32_t scratch_block_size_b, uint32_t flags)
{
  /* TODO: support a custom fallback allocator */
  jjs_allocator_t fallback_allocator = (flags & JJS_CONTEXT_FLAG_SCRATCH_ALLOCATOR_VM) ? jjs_util_vm_allocator (context_p) : jjs_util_system_allocator ();

  if (scratch_block_size_b > 0)
  {
    context_p->scratch_arena_allocator = jjs_util_arena_allocator (scratch_block, scratch_block_size_b);
    context_p->scratch_fallback_allocator = fallback_allocator;
    context_p->scratch_allocator = jjs_util_scratch_allocator (context_p);
    context_p->scratch_arena_allocator_enabled = true;
  }
  else
  {
    context_p->scratch_allocator = fallback_allocator;
    context_p->scratch_arena_allocator_enabled = false;
  }
}

/*
 * Setup the global context object.
 *
 * Any code that accesses the context needs to set this up. In core, jjs_init() makes
 * the call. If the tests are not using the full vm, they must call this method to
 * initialize the context.
 */
jjs_status_t
jjs_context_init (const jjs_context_options_t* options_p, jjs_context_t** out_p)
{
  jjs_status_t status;
  jjs_context_options_t default_options = {0};
  jjs_platform_t *platform;

  if (options_p == NULL)
  {
    options_p = &default_options;
  }

  if (options_p->platform)
  {
    platform = options_p->platform;
    platform->refs++;
  }
  else if ((status = jjs_platform_new (NULL, &platform)) != JJS_STATUS_OK)
  {
    return status;
  }

#if !JJS_VM_STACK_LIMIT
  if (options_p->vm_stack_limit_kb.has_value)
  {
    return JJS_STATUS_CONTEXT_VM_STACK_LIMIT_DISABLED;
  }
#endif /* !JJS_VM_STACK_LIMIT */

  jjs_allocator_t allocator = options_p->allocator ? *options_p->allocator : jjs_util_system_allocator ();
  jjs_size_t context_aligned_size_b = JJS_ALIGNUP (sizeof (jjs_context_t), JMEM_ALIGNMENT);
  jjs_size_t vm_heap_size_b = get_context_option_u32 (&options_p->vm_heap_size_kb, JJS_DEFAULT_VM_HEAP_SIZE_KB) * 1024;
  jjs_size_t scratch_size_b = get_context_option_u32 (&options_p->scratch_arena_kb, JJS_DEFAULT_SCRATCH_ARENA_KB) * 1024;
  uint8_t* block_p;
  jjs_size_t block_size;
  uint8_t *scratch_block_p;
  jjs_context_t *context_p;

  if (options_p->context_flags & JJS_CONTEXT_FLAG_STRICT_MEMORY_LAYOUT)
  {
    block_size = vm_heap_size_b + scratch_size_b;
    vm_heap_size_b = vm_heap_size_b - context_aligned_size_b;
  }
  else
  {
    block_size = context_aligned_size_b + vm_heap_size_b + scratch_size_b;
  }

  block_p = jjs_allocator_alloc (&allocator, block_size);

  if (!block_p)
  {
    return JJS_STATUS_BAD_ALLOC;
  }

  context_p = (jjs_context_t *) block_p;

  memset (block_p, 0, context_aligned_size_b + vm_heap_size_b + scratch_size_b);
  context_p->context_block_size_b = context_aligned_size_b + vm_heap_size_b + scratch_size_b;

  context_p->platform_p = platform;

  /* javascript jjs namespace exclusions */
  context_p->jjs_namespace_exclusions = options_p->jjs_namespace_exclusions;

  context_p->vm_heap_size = vm_heap_size_b;
  context_p->vm_stack_limit = get_context_option_u32 (&options_p->vm_stack_limit_kb, JJS_DEFAULT_VM_STACK_LIMIT) * 1024;
  context_p->gc_mark_limit = get_context_option_u32 (&options_p->gc_mark_limit, JJS_DEFAULT_GC_MARK_LIMIT);
  context_p->gc_new_objects_fraction = get_context_option_u32 (&options_p->gc_new_objects_fraction, JJS_DEFAULT_GC_NEW_OBJECTS_FRACTION);
  context_p->gc_limit = get_context_option_u32 (&options_p->gc_limit_kb, JJS_DEFAULT_MAX_GC_LIMIT) * 1024;

  if (context_p->gc_limit == 0)
  {
    context_p->gc_limit = JJS_COMPUTE_GC_LIMIT (vm_heap_size_b);
  }

  context_p->unhandled_rejection_cb = &jjs_util_promise_unhandled_rejection_default;
  context_p->heap_p = (jmem_heap_t *) (block_p + context_aligned_size_b);
  context_p->context_flags = options_p->context_flags | ECMA_STATUS_CONTEXT_INITIALIZED;

  /* install streams iff they are non-null and a write function exists */
  if (platform->io_write)
  {
    if (platform->io_stdout)
    {
      context_p->streams[JJS_STDOUT] = platform->io_stdout;
    }

    if (platform->io_stderr)
    {
      context_p->streams[JJS_STDERR] = platform->io_stderr;
    }
  }

  /* allocators */
  context_p->context_allocator = allocator;
  scratch_block_p = scratch_size_b > 0 ? block_p + (context_aligned_size_b + vm_heap_size_b) : NULL;
  context_set_scratch_allocator (context_p, scratch_block_p, scratch_size_b, options_p->context_flags);

  *out_p = context_p;

  return JJS_STATUS_OK;
}

/**
 * Cleanup the context.
 */
void
jjs_context_cleanup (jjs_context_t* context_p)
{
  jjs_allocator_t allocator = context_p->context_allocator;
  jjs_allocator_t scratch_allocator = context_p->scratch_allocator;

  jjs_platform_free (context_p->platform_p);
  jjs_allocator_free (&allocator, context_p, context_p->context_block_size_b);

  jjs_allocator_free_self (&scratch_allocator);
  jjs_allocator_free_self (&allocator);
}
