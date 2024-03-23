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

/*
 * Setup the global context object.
 * 
 * Any code that accesses the context needs to set this up. In core, jjs_init() makes
 * the call. If the tests are not using the full vm, they must call this method to
 * initialize the context.
 */
void
jjs_context_init (jjs_init_flag_t flags, jjs_init_options_t* options_p)
{
  jjs_init_options_t clean_options = { .flags = JJS_INIT_OPTION_ALL };
  uint32_t options_flags = options_p ? options_p->flags : JJS_INIT_OPTION_NO_OPTS;

  if (options_p == NULL)
  {
    options_p = &clean_options;
  }

  if (options_flags & JJS_INIT_OPTION_HAS_HEAP_SIZE)
  {
    clean_options.heap_size = options_p->heap_size;
  }
  else
  {
    clean_options.heap_size = JJS_GLOBAL_HEAP_SIZE;
  }

  if (options_flags & JJS_INIT_OPTION_HAS_GC_LIMIT)
  {
    clean_options.gc_limit = options_p->gc_limit * 1024;
  }
  else
  {
    clean_options.gc_limit = JJS_GC_LIMIT * 1024;
  }

  if (clean_options.gc_limit == 0)
  {
    clean_options.gc_limit = (clean_options.heap_size * 1024) / 32;
  }

  if (options_flags & JJS_INIT_OPTION_HAS_GC_MARK_LIMIT)
  {
    clean_options.gc_mark_limit = options_p->gc_mark_limit;
  }
  else
  {
    clean_options.gc_mark_limit = JJS_GC_MARK_LIMIT;
  }

  if (options_flags & JJS_INIT_OPTION_HAS_STACK_LIMIT)
  {
    clean_options.stack_limit = options_p->stack_limit;
  }
  else
  {
    clean_options.stack_limit = JJS_STACK_LIMIT;
  }

  if (options_flags & JJS_INIT_OPTION_HAS_GC_NEW_OBJECTS_FRACTION)
  {
    clean_options.gc_new_objects_fraction = options_p->gc_new_objects_fraction;
  }

  if (clean_options.gc_new_objects_fraction == 0)
  {
    clean_options.gc_new_objects_fraction = 16;
  }

#if JJS_EXTERNAL_CONTEXT
  size_t total_size = jjs_port_context_alloc (sizeof (jjs_context_t), options_p);
  JJS_UNUSED (total_size);
#endif /* JJS_EXTERNAL_CONTEXT */

  jjs_context_t *context_p = &JJS_CONTEXT_STRUCT;
  memset (context_p, 0, sizeof (jjs_context_t));

  context_p->cfg_global_heap_size = options_p->heap_size;
  context_p->cfg_gc_limit = options_p->gc_limit;
  context_p->cfg_gc_mark_limit = options_p->gc_mark_limit;
  context_p->cfg_stack_limit = options_p->stack_limit;
  context_p->cfg_gc_new_objects_fraction = options_p->gc_new_objects_fraction;

#if JJS_EXTERNAL_CONTEXT && !JJS_SYSTEM_ALLOCATOR
  uint32_t heap_start_offset = JJS_ALIGNUP (sizeof (jjs_context_t), JMEM_ALIGNMENT);
  uint8_t *heap_p = ((uint8_t *) context_p) + heap_start_offset;
  uint32_t heap_size = JJS_ALIGNDOWN (total_size - heap_start_offset, JMEM_ALIGNMENT);

  JJS_ASSERT (heap_p + heap_size <= ((uint8_t *) context_p) + total_size);

  context_p->heap_p = (jmem_heap_t *) heap_p;
  context_p->heap_size = heap_size;
#endif /* JJS_EXTERNAL_CONTEXT && !JJS_SYSTEM_ALLOCATOR */

  JJS_CONTEXT (jjs_init_flags) = flags;
}

/**
 * Cleanup the context.
 */
void
jjs_context_cleanup (void)
{
#if JJS_EXTERNAL_CONTEXT
  jjs_port_context_free ();
#endif /* JJS_EXTERNAL_CONTEXT */
}
