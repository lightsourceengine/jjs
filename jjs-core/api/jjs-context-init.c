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
#include "jjs-stream.h"
#include "jjs-util.h"

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

/**
 * Default implementation of unhandled rejection callback.
 */
static void
unhandled_rejection_default (jjs_value_t promise, jjs_value_t reason, void *user_p)
{
  JJS_UNUSED_ALL (promise, reason, user_p);
#if JJS_LOGGING
  jjs_log_fmt (JJS_LOG_LEVEL_ERROR, "Uncaught:\n{}\n", reason);
#endif
}

static bool
check_stream_encoding (void* stream_p, jjs_encoding_t default_encoding)
{
  if (stream_p != NULL)
  {
    return (default_encoding == JJS_ENCODING_ASCII || default_encoding == JJS_ENCODING_UTF8 || default_encoding == JJS_ENCODING_CESU8);
  }

  return true;
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
  jjs_context_options_t default_options;

  if (options_p == NULL)
  {
    options_p = jjs_context_options_init (&default_options);
  }

  if (JJS_CONTEXT (status_flags) & ECMA_STATUS_CONTEXT_INITIALIZED)
  {
    return JJS_STATUS_CONTEXT_ALREADY_INITIALIZED;
  }

  /* context should be zero'd out at program init or jjs_cleanup */
  JJS_ASSERT (JJS_CONTEXT (heap_p) == NULL);

  if (JJS_CONTEXT (heap_p) != NULL)
  {
    return JJS_STATUS_CONTEXT_CHAOS;
  }

  uint32_t context_flags = options_p->context_flags;

  if ((context_flags & JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP) && options_p->external_heap.buffer_p == NULL)
  {
    return JJS_STATUS_CONTEXT_INVALID_EXTERNAL_HEAP;
  }

  if (options_p->platform.fatal == NULL)
  {
    return JJS_STATUS_CONTEXT_REQUIRES_API_FATAL;
  }

  if (!check_stream_encoding (options_p->platform.io_stdout, options_p->platform.io_stdout_encoding))
  {
    return JJS_STATUS_CONTEXT_STDOUT_INVALID_ENCODING;
  }

  if (!check_stream_encoding (options_p->platform.io_stderr, options_p->platform.io_stderr_encoding))
  {
    return JJS_STATUS_CONTEXT_STDERR_INVALID_ENCODING;
  }

#if JJS_DEBUGGER
  if (options_p->platform.time_sleep == NULL)
  {
    return JJS_STATUS_CONTEXT_REQUIRES_API_TIME_SLEEP;
  }
#endif

#if JJS_BUILTIN_DATE
  if (options_p->platform.time_local_tza == NULL)
  {
    return JJS_STATUS_CONTEXT_REQUIRES_API_TIME_LOCAL_TZA;
  }

  if (options_p->platform.time_now_ms == NULL)
  {
    return JJS_STATUS_CONTEXT_REQUIRES_API_TIME_NOW_MS;
  }
#endif

#if JJS_VM_STACK_STATIC
  // user cannot change the default
  if (options_p->vm_stack_limit_kb != JJS_DEFAULT_VM_STACK_LIMIT)
  {
    return JJS_STATUS_CONTEXT_IMMUTABLE_STACK_LIMIT;
  }
#endif /* JJS_VM_STACK_STATIC */

#if JJS_VM_HEAP_STATIC
  // user cannot change the default
  if (options_p->vm_heap_size_kb != JJS_DEFAULT_VM_HEAP_SIZE || context_flags & JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP)
  {
    return JJS_STATUS_CONTEXT_IMMUTABLE_HEAP_SIZE;
  }
#endif /* JJS_VM_HEAP_STATIC */

  /* allocators */
  if (!jjs_util_context_allocator_init (&options_p->scratch_allocator))
  {
    return JJS_STATUS_CONTEXT_CHAOS;
  }

  if (context_flags & JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP)
  {
    // TODO: check alignment
    if (options_p->external_heap.buffer_p == NULL)
    {
      return JJS_STATUS_CONTEXT_INVALID_EXTERNAL_HEAP;
    }
    JJS_CONTEXT (vm_heap_size) = options_p->external_heap.buffer_size_in_bytes;
  }
  else
  {
    JJS_CONTEXT (vm_heap_size) = options_p->vm_heap_size_kb * 1024;
  }

  JJS_CONTEXT (vm_stack_limit) = options_p->vm_stack_limit_kb * 1024;
  JJS_CONTEXT (gc_limit) =
    (options_p->gc_limit_kb == 0) ? JJS_COMPUTE_GC_LIMIT (JJS_CONTEXT (vm_heap_size)) : (options_p->gc_limit_kb * 1024);
  JJS_CONTEXT (gc_mark_limit) = options_p->gc_mark_limit;
  JJS_CONTEXT (gc_new_objects_fraction) = options_p->gc_new_objects_fraction;

  void* block;
#if JJS_VM_HEAP_STATIC
  block = &jjs_static_heap;
#else /* !JJS_VM_HEAP_STATIC */
  if (context_flags & JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP)
  {
    block = options_p->external_heap.buffer_p;
    JJS_CONTEXT (heap_free_cb) = options_p->external_heap.free_cb;
    JJS_CONTEXT (heap_free_user_p) = options_p->external_heap.free_user_p;
  }
  else
  {
    block = calloc (JJS_CONTEXT (vm_heap_size), 1);
  }
#endif /* JJS_VM_HEAP_STATIC */
  JJS_CONTEXT (heap_p) = block;

  if (options_p->unhandled_rejection_cb)
  {
    JJS_CONTEXT (unhandled_rejection_cb) = options_p->unhandled_rejection_cb;
    JJS_CONTEXT (unhandled_rejection_user_p) = options_p->unhandled_rejection_user_p;
  }
  else
  {
    JJS_CONTEXT (unhandled_rejection_cb) = &unhandled_rejection_default;
  }

  context_flags |= ECMA_STATUS_CONTEXT_INITIALIZED;
  JJS_CONTEXT (context_flags) = context_flags;
  JJS_CONTEXT (platform_api) = options_p->platform;

  /* install streams iff they are non-null and a write function exists */
  if (options_p->platform.io_write)
  {
    if (options_p->platform.io_stdout)
    {
      JJS_CONTEXT (streams)[JJS_STDOUT] = options_p->platform.io_stdout;
    }

    if (options_p->platform.io_stderr)
    {
      JJS_CONTEXT (streams)[JJS_STDERR] = options_p->platform.io_stderr;
    }
  }

  return JJS_STATUS_OK;
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
  if ((JJS_CONTEXT (context_flags) & JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP))
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
