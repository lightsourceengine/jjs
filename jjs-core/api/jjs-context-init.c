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
#define JJS_COMPUTE_GC_LIMIT(HEAP_SIZE) \
  JJS_MIN ((HEAP_SIZE) / JJS_DEFAULT_MAX_GC_LIMIT_DIVISOR, JJS_DEFAULT_MAX_GC_LIMIT)

/**
 * Default implementation of unhandled rejection callback.
 */
static void
unhandled_rejection_default (jjs_context_t* context_p, jjs_value_t promise, jjs_value_t reason, void *user_p)
{
  JJS_UNUSED_ALL (context_p, promise, reason, user_p);
#if JJS_LOGGING
  jjs_log_fmt (context_p, JJS_LOG_LEVEL_ERROR, "Uncaught:\n{}\n", reason);
#endif
}

/*
 * Setup the global context object.
 *
 * Any code that accesses the context needs to set this up. In core, jjs_init() makes
 * the call. If the tests are not using the full vm, they must call this method to
 * initialize the context.
 */
jjs_status_t
jjs_context_init (const jjs_context_options_t* options_p)
{
  jjs_context_t *context_p = &JJS_CONTEXT_STRUCT;

  if (context_p->status_flags & ECMA_STATUS_CONTEXT_INITIALIZED)
  {
    return JJS_STATUS_CONTEXT_ALREADY_INITIALIZED;
  }

  /* context should be zero'd out at program init or jjs_cleanup */
  JJS_ASSERT (context_p->heap_p == NULL);

  if (context_p->heap_p != NULL)
  {
    return JJS_STATUS_CONTEXT_CHAOS;
  }

  jjs_context_options_t default_options = {0};

  if (options_p == NULL)
  {
    options_p = &default_options;
  }

  jjs_allocator_t *allocator = options_p->allocator ? options_p->allocator : jjs_util_system_allocator_ptr ();
  jjs_platform_t *platform;

  if (options_p->platform)
  {
    platform = options_p->platform;
    platform->refs++;
  }
  else
  {
    jjs_platform_new (NULL, &platform);
  }

  context_p->platform_p = platform;

  /* javascript jjs namespace exclusions */
  context_p->jjs_namespace_exclusions = options_p->jjs_namespace_exclusions;

  /* allocators */
  if (!jjs_util_context_allocator_init (context_p, jjs_util_system_allocator_ptr ()))
  {
    return JJS_STATUS_CONTEXT_CHAOS;
  }

  context_p->vm_heap_size = (options_p->vm_heap_size_kb > 0 ? options_p->vm_heap_size_kb : JJS_DEFAULT_VM_HEAP_SIZE) * 1024;
  context_p->vm_stack_limit = (options_p->vm_stack_limit_kb > 0 ? options_p->vm_stack_limit_kb : JJS_DEFAULT_VM_STACK_LIMIT) * 1024;
  context_p->gc_limit = (options_p->gc_limit_kb == 0) ? JJS_COMPUTE_GC_LIMIT (context_p->vm_heap_size) : (options_p->gc_limit_kb * 1024);
  context_p->gc_mark_limit = options_p->gc_mark_limit > 0 ? options_p->gc_mark_limit : JJS_DEFAULT_GC_MARK_LIMIT;
  context_p->gc_new_objects_fraction = options_p->gc_new_objects_fraction > 0 ? options_p->gc_new_objects_fraction : JJS_DEFAULT_GC_NEW_OBJECTS_FRACTION;

  if (options_p->unhandled_rejection_cb)
  {
    context_p->unhandled_rejection_cb = options_p->unhandled_rejection_cb;
    context_p->unhandled_rejection_user_p = options_p->unhandled_rejection_user_p;
  }
  else
  {
    context_p->unhandled_rejection_cb = &unhandled_rejection_default;
  }

  context_p->heap_p = allocator->alloc (allocator, context_p->vm_heap_size);
  context_p->heap_allocator_p = allocator;
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

  return JJS_STATUS_OK;
}

/**
 * Cleanup the context.
 */
void
jjs_context_cleanup (jjs_context_t* context_p)
{
  // TODO: check initialized?
  // TODO: delete context?
  jjs_allocator_t *allocator = context_p->heap_allocator_p;

  allocator->free (allocator, context_p->heap_p, context_p->vm_heap_size);

  jjs_platform_free (context_p->platform_p);

  memset (&JJS_CONTEXT_STRUCT, 0, sizeof (JJS_CONTEXT_STRUCT));
}
