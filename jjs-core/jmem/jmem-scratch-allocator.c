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

#include "jmem.h"

static const jjs_size_t
JMEM_FALLBACK_ALLOCATION_SIZE = (jjs_size_t) JJS_ALIGNUP (sizeof (jmem_fallback_allocation_t), JMEM_ALIGNMENT);

static void*
jmem_scratch_allocator_alloc (void *impl_p, uint32_t size)
{
  jmem_scratch_allocator_t *allocator_p = impl_p;

  if (size == 0)
  {
    return NULL;
  }

  void *p;
  jjs_size_t request_size = JJS_ALIGNUP (size, JMEM_ALIGNMENT);

  if (request_size <= allocator_p->fixed_buffer_remaining_size)
  {
    p = allocator_p->fixed_buffer_next_p;

    allocator_p->fixed_buffer_next_p = ((uint8_t *) p) + request_size;
    allocator_p->fixed_buffer_remaining_size -= request_size;

    return p;
  }

  request_size = JMEM_FALLBACK_ALLOCATION_SIZE + size;

  p = jjs_allocator_alloc (&allocator_p->fallback_allocator, request_size);

  if (p)
  {
    jmem_fallback_allocation_t *header = p;

    header->size = request_size;
    header->next_p = allocator_p->fallback_allocations;
    allocator_p->fallback_allocations = header;

    return ((uint8_t *) p) + JMEM_FALLBACK_ALLOCATION_SIZE;
  }

  return p;
}

static void
jmem_scratch_allocator_free (void *impl_p, void* p, uint32_t size)
{
  JJS_UNUSED_ALL (impl_p, p, size);
}

/**
 * Init a new scratch allocator.
 *
 * The scratch allocator is for temporary allocations at a VM operation level for
 * manipulating path string during module operations, loading source code from disk,
 * merging strings, etc. Going to the VM heap for these operations can add GC pressure
 * and lead to fragmentation. The primary motivation was module loading, as that requires
 * the manipulation of filesystem paths. The scratch allocator is generally useful for
 * source code loading and ECMA operations.
 *
 * The scratch allocator is similar to an arena. Allocations occur continuously. When the
 * scratch is no longer in use, a reset will free all allocated memory. Internally, the
 * scratch allocator is fronted by an optional fixed block of memory. If the block is not
 * big enough to handle an allocation, a fallback allocator exists to handle it. The
 * fallback is usually a system or VM heap allocator.
 *
 * The fixed buffer allocations have no header overhead. The allocator implementation
 * keeps track of the remaining space and next available pointer. Fallback allocation
 * require a 2 pointer sized header to record allocations in a linked list. On reset,
 * the recorded allocations are free'd through the fallback allocator.
 *
 * The scratch allocator and it's design are for performance. Another consideration is
 * that JJS can run without dynamic allocations. A custom fallback allocator let's the
 * scratch be configurable for static and dynamic environments.
 *
 * @return JJS_STATUS_OK on success; otherwise, an error code.
 */
jjs_status_t
jmem_scratch_allocator_init (uint8_t *fixed_buffer_p, /**< fixed buffer, can be NULL */
                             jjs_size_t fixed_buffer_size, /**< fixed buffer size, can be 0 */
                             jjs_allocator_t fallback_allocator, /**< required fallback allocator */
                             jmem_scratch_allocator_t *dest_p) /**< scratch allocator memory */
{
  static const jjs_allocator_vtab_t vtab = {
    .alloc = jmem_scratch_allocator_alloc,
    .free = jmem_scratch_allocator_free,
  };

  *dest_p = (jmem_scratch_allocator_t) {
    .fixed_buffer_p = fixed_buffer_p,
    .fixed_buffer_size = fixed_buffer_size,
    .fixed_buffer_next_p = fixed_buffer_size > 0 ? fixed_buffer_p : NULL,
    .fixed_buffer_remaining_size = fixed_buffer_size > 0 ? fixed_buffer_size : 0,
    .fallback_allocator = fallback_allocator,
    .fallback_allocations = NULL,
    .allocator = {
      .vtab_p = &vtab,
      .impl_p = dest_p,
    },
  };

  return JJS_STATUS_OK;
} /* jmem_scratch_allocator_init */

inline void JJS_ATTR_ALWAYS_INLINE
jmem_scratch_allocator_free_allocations (jmem_scratch_allocator_t *allocator_p)
{
  jmem_fallback_allocation_t *next = allocator_p->fallback_allocations;
  jmem_fallback_allocation_t *t;

  while (next)
  {
    t = next->next_p;
    jjs_allocator_free (&allocator_p->fallback_allocator, next, next->size);
    next = t;
  }

  allocator_p->fallback_allocations = NULL;
}

/**
 * Reset the scratch allocator.
 *
 * Frees any fallback allocator allocations and resets the fixed buffer pointers.
 */
void
jmem_scratch_allocator_reset (jmem_scratch_allocator_t *allocator_p) /**< scratch allocator */
{
  if (allocator_p->fixed_buffer_size)
  {
    allocator_p->fixed_buffer_next_p = allocator_p->fixed_buffer_p;
    allocator_p->fixed_buffer_remaining_size = allocator_p->fixed_buffer_size;
  }

  jmem_scratch_allocator_free_allocations (allocator_p);
} /* jmem_scratch_allocator_reset */

/**
 * Destroy the scratch allocator.
 *
 * After this call, the allocator should not longer be used.
 */
void
jmem_scratch_allocator_deinit (jmem_scratch_allocator_t *allocator_p) /**< scratch allocator */
{
  allocator_p->fixed_buffer_size = 0;
  jmem_scratch_allocator_free_allocations (allocator_p);
} /* jmem_scratch_allocator_deinit */
