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
context_set_scratch_allocator (jjs_context_t* context_p,
                               uint8_t *scratch_block,
                               uint32_t scratch_block_size_b,
                               const jjs_context_options_t *options_p)
{
  jjs_allocator_t fallback_allocator;

  switch (options_p->scratch_fallback_allocator_type)
  {
    case JJS_SCRATCH_ALLOCATOR_SYSTEM:
    {
      fallback_allocator = jjs_util_system_allocator ();
      break;
    }
    case JJS_SCRATCH_ALLOCATOR_VM:
    {
      fallback_allocator = jjs_util_vm_allocator (context_p);
      break;
    }
    case JJS_SCRATCH_ALLOCATOR_CUSTOM:
    {
      fallback_allocator = options_p->custom_scratch_fallback_allocator;
      break;
    }
    default:
    {
      abort ();
    }
  }

  jmem_scratch_allocator_init (scratch_block, scratch_block_size_b, fallback_allocator, &context_p->scratch_allocator);
}

/*
 * Setup the global context object.
 *
 * Any code that accesses the context needs to set this up. In core, jjs_init() makes
 * the call. If the tests are not using the full vm, they must call this method to
 * initialize the context.
 */
jjs_status_t
jjs_context_init (const jjs_context_options_t* options_p, const jjs_allocator_t *allocator_p, jjs_context_t** out_p)
{
  jjs_context_options_t default_options = {0};

  if (options_p == NULL)
  {
    options_p = &default_options;
  }

  if (allocator_p == NULL)
  {
    allocator_p = jjs_util_system_allocator_ptr ();
  }

#if !JJS_VM_STACK_LIMIT
  if (options_p->vm_stack_limit_kb.has_value)
  {
    return JJS_STATUS_CONTEXT_VM_STACK_LIMIT_DISABLED;
  }
#endif /* !JJS_VM_STACK_LIMIT */

  jjs_size_t context_aligned_size_b = JJS_ALIGNUP (sizeof (jjs_context_t), JMEM_ALIGNMENT);
  jjs_size_t vm_heap_size_b = get_context_option_u32 (&options_p->vm_heap_size_kb, JJS_DEFAULT_VM_HEAP_SIZE_KB) * 1024;
  jjs_size_t scratch_size_b = get_context_option_u32 (&options_p->scratch_size_kb, JJS_DEFAULT_SCRATCH_SIZE_KB) * 1024;
  uint8_t* block_p;
  jjs_size_t block_size;
  uint8_t *scratch_block_p;
  jjs_context_t *context_p;

  if (options_p->strict_memory_layout)
  {
    block_size = vm_heap_size_b + scratch_size_b;
    vm_heap_size_b = vm_heap_size_b - context_aligned_size_b;
  }
  else
  {
    block_size = context_aligned_size_b + vm_heap_size_b + scratch_size_b;
  }

  block_p = jjs_allocator_alloc (allocator_p, block_size);

  if (!block_p)
  {
    return JJS_STATUS_BAD_ALLOC;
  }

  context_p = (jjs_context_t *) block_p;
  memset (context_p, 0, sizeof (*context_p));

  context_p->context_block_size_b = context_aligned_size_b + vm_heap_size_b + scratch_size_b;

  context_p->vm_heap_size = vm_heap_size_b;
  context_p->vm_stack_limit = get_context_option_u32 (&options_p->vm_stack_limit_kb, JJS_DEFAULT_VM_STACK_LIMIT) * 1024;
  context_p->vm_cell_count = get_context_option_u32 (&options_p->vm_cell_count, JJS_DEFAULT_VM_CELL_COUNT);
  context_p->gc_mark_limit = get_context_option_u32 (&options_p->gc_mark_limit, JJS_DEFAULT_GC_MARK_LIMIT);
  context_p->gc_new_objects_fraction = get_context_option_u32 (&options_p->gc_new_objects_fraction, JJS_DEFAULT_GC_NEW_OBJECTS_FRACTION);
  context_p->gc_limit = get_context_option_u32 (&options_p->gc_limit_kb, JJS_DEFAULT_MAX_GC_LIMIT);

  if (context_p->gc_limit == 0)
  {
    context_p->gc_limit = JJS_COMPUTE_GC_LIMIT (vm_heap_size_b);
  }

  context_p->unhandled_rejection_cb = &jjs_util_promise_unhandled_rejection_default;
  context_p->heap_p = (jmem_heap_t *) (block_p + context_aligned_size_b);
  context_p->context_flags = 0;

  if (options_p->show_op_codes)
  {
    context_p->context_flags |= JJS_CONTEXT_FLAG_SHOW_OPCODES;
  }

  if (options_p->show_regexp_op_codes)
  {
    context_p->context_flags |= JJS_CONTEXT_FLAG_SHOW_REGEXP_OPCODES;
  }

  if (options_p->enable_mem_stats)
  {
    context_p->context_flags |= JJS_CONTEXT_FLAG_MEM_STATS;
  }

  /* default stream settings */
  context_p->io_target[JJS_STDOUT] = (void *) stdout;
  context_p->io_target_encoding[JJS_STDOUT] = JJS_ENCODING_UTF8;
  context_p->io_target[JJS_STDERR] = (void *) stderr;
  context_p->io_target_encoding[JJS_STDERR] = JJS_ENCODING_UTF8;

  /* allocators */
  context_p->context_allocator = *allocator_p;
  context_p->vm_allocator = jjs_util_vm_allocator (context_p);
  scratch_block_p = scratch_size_b > 0 ? block_p + (context_aligned_size_b + vm_heap_size_b) : NULL;
  context_set_scratch_allocator (context_p, scratch_block_p, scratch_size_b, options_p);

  *out_p = context_p;

  return JJS_STATUS_OK;
}

/**
 * Cleanup the context.
 */
void
jjs_context_cleanup (jjs_context_t* context_p)
{
  JJS_ASSERT (context_p->scratch_allocator.refs == 0);
  jmem_scratch_allocator_deinit (&context_p->scratch_allocator);
  jjs_allocator_free (&context_p->context_allocator, context_p, context_p->context_block_size_b);
}
