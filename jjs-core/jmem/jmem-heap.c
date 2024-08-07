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

/**
 * Heap implementation
 */

#include "ecma-gc.h"

#include "jcontext.h"
#include "jmem.h"
#include "jrt.h"
#include "jrt-bit-fields.h"
#include "jrt-libc-includes.h"

#define JMEM_ALLOCATOR_INTERNAL
#include "jmem-allocator-internal.h"

/** \addtogroup mem Memory allocation
 * @{
 *
 * \addtogroup heap Heap
 * @{
 */

/**
 * End of list marker.
 */
#define JMEM_HEAP_END_OF_LIST ((uint32_t) 0xffffffff)

#define JMEM_HEAP_AREA_SIZE(ctx) ((ctx)->vm_heap_size - JMEM_ALIGNMENT)

/**
 * @{
 */
#ifdef ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY
/* In this case we simply store the pointer, since it fits anyway. */
#define JMEM_HEAP_GET_OFFSET_FROM_ADDR(ctx, p) ((uint32_t) (p))
#define JMEM_HEAP_GET_ADDR_FROM_OFFSET(ctx, u) ((jmem_heap_free_t *) (u))
#else /* !ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */
#define JMEM_HEAP_GET_OFFSET_FROM_ADDR(ctx, p) ((uint32_t) ((uint8_t *) (p) -(ctx)->heap_p->area))
#define JMEM_HEAP_GET_ADDR_FROM_OFFSET(ctx, u) ((jmem_heap_free_t *) ((ctx)->heap_p->area + (u)))
#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */
/**
 * @}
 */

/**
 * Get end of region
 *
 * @return pointer to the end of the region
 */
static inline jmem_heap_free_t *JJS_ATTR_ALWAYS_INLINE JJS_ATTR_PURE
jmem_heap_get_region_end (jmem_heap_free_t *curr_p) /**< current region */
{
  return (jmem_heap_free_t *) ((uint8_t *) curr_p + curr_p->size);
} /* jmem_heap_get_region_end */

/**
 * Startup initialization of heap
 */
void
jmem_heap_init (jjs_context_t *context_p)
{
#if !JJS_CPOINTER_32_BIT
  /* the maximum heap size for 16bit compressed pointers should be 512K */
  JJS_ASSERT (((UINT16_MAX + 1) << JMEM_ALIGNMENT_LOG) >= context_p->vm_heap_size);
#endif /* !JJS_CPOINTER_32_BIT */
  JJS_ASSERT ((uintptr_t) (context_p->heap_p->area) % JMEM_ALIGNMENT == 0);

  context_p->jmem_heap_limit = context_p->gc_limit;

  jmem_heap_free_t *const region_p = (jmem_heap_free_t *) context_p->heap_p->area;
  const uint32_t heap_area_size = JMEM_HEAP_AREA_SIZE (context_p);

  context_p->jmem_area_end = context_p->heap_p->area + heap_area_size;

  region_p->size = heap_area_size;
  region_p->next_offset = JMEM_HEAP_END_OF_LIST;

  context_p->heap_p->first.size = 0;
  context_p->heap_p->first.next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (context_p, region_p);

  context_p->jmem_heap_list_skip_p = &context_p->heap_p->first;

  jmem_cellocator_init (context_p);

  JMEM_VALGRIND_NOACCESS_SPACE (&context_p->heap_p->first, sizeof (jmem_heap_free_t));
  JMEM_VALGRIND_NOACCESS_SPACE (context_p->heap_p->area, heap_area_size);

  JMEM_HEAP_STAT_INIT (context_p);
} /* jmem_heap_init */

/**
 * Finalize heap
 */
void
jmem_heap_finalize (jjs_context_t *context_p)
{
  jmem_cellocator_finalize (context_p);

  JJS_ASSERT (context_p->jmem_heap_allocated_size == 0);
  if (context_p->jmem_heap_allocated_size > 0)
  {
    jjs_fatal (JJS_FATAL_FAILED_ASSERTION);
  }
  JMEM_VALGRIND_NOACCESS_SPACE (&context_p->heap_p->first, context_p->vm_heap_size);
} /* jmem_heap_finalize */

/**
 * Allocation of memory region.
 *
 * See also:
 *          jmem_heap_alloc_block
 *
 * @return pointer to allocated memory block - if allocation is successful,
 *         NULL - if there is not enough memory.
 */
static void *JJS_ATTR_HOT
jmem_heap_alloc (jjs_context_t *context_p, /**< JJS context */
                 const size_t size) /**< size of requested block */
{
  /* Align size. */
  const size_t required_size = ((size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT) * JMEM_ALIGNMENT;
  jmem_heap_free_t *data_space_p = NULL;

  JMEM_VALGRIND_DEFINED_SPACE (&context_p->heap_p->first, sizeof (jmem_heap_free_t));

  if (required_size <= JMEM_CELLOCATOR_CELL_SIZE)
  {
    void *chunk_p = jmem_cellocator_alloc (&context_p->jmem_cellocator_32);

    if (chunk_p)
    {
      return chunk_p;
    }
    else
    {
      ecma_free_unused_memory (context_p, JMEM_PRESSURE_LOW);
      chunk_p = jmem_cellocator_alloc (&context_p->jmem_cellocator_32);

      if (chunk_p)
      {
        return chunk_p;
      }
    }

    if (jmem_cellocator_add_page (context_p, &context_p->jmem_cellocator_32))
    {
      return jmem_cellocator_alloc (&context_p->jmem_cellocator_32);
    }

    return NULL;
  }

  /* Fast path for 8 byte chunks, first region is guaranteed to be sufficient. */
  if (required_size == JMEM_ALIGNMENT && JJS_LIKELY (context_p->heap_p->first.next_offset != JMEM_HEAP_END_OF_LIST))
  {
    data_space_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (context_p, context_p->heap_p->first.next_offset);
    JJS_ASSERT (jmem_is_heap_pointer (context_p, data_space_p));

    JMEM_VALGRIND_DEFINED_SPACE (data_space_p, sizeof (jmem_heap_free_t));
    context_p->jmem_heap_allocated_size += JMEM_ALIGNMENT;

    if (context_p->jmem_heap_allocated_size >= context_p->jmem_heap_limit)
    {
      context_p->jmem_heap_limit += context_p->gc_limit;
    }

    if (data_space_p->size == JMEM_ALIGNMENT)
    {
      context_p->heap_p->first.next_offset = data_space_p->next_offset;
    }
    else
    {
      JJS_ASSERT (data_space_p->size > JMEM_ALIGNMENT);

      jmem_heap_free_t *remaining_p;
      remaining_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (context_p, context_p->heap_p->first.next_offset) + 1;

      JMEM_VALGRIND_DEFINED_SPACE (remaining_p, sizeof (jmem_heap_free_t));
      remaining_p->size = data_space_p->size - JMEM_ALIGNMENT;
      remaining_p->next_offset = data_space_p->next_offset;
      JMEM_VALGRIND_NOACCESS_SPACE (remaining_p, sizeof (jmem_heap_free_t));

      context_p->heap_p->first.next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (context_p, remaining_p);
    }

    JMEM_VALGRIND_NOACCESS_SPACE (data_space_p, sizeof (jmem_heap_free_t));

    if (JJS_UNLIKELY (data_space_p == context_p->jmem_heap_list_skip_p))
    {
      context_p->jmem_heap_list_skip_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (context_p, context_p->heap_p->first.next_offset);
    }
  }
  /* Slow path for larger regions. */
  else
  {
    uint32_t current_offset = context_p->heap_p->first.next_offset;
    jmem_heap_free_t *prev_p = &context_p->heap_p->first;

    while (JJS_LIKELY (current_offset != JMEM_HEAP_END_OF_LIST))
    {
      jmem_heap_free_t *current_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (context_p, current_offset);
      JJS_ASSERT (jmem_is_heap_pointer (context_p, current_p));
      JMEM_VALGRIND_DEFINED_SPACE (current_p, sizeof (jmem_heap_free_t));

      const uint32_t next_offset = current_p->next_offset;
      JJS_ASSERT (next_offset == JMEM_HEAP_END_OF_LIST
                    || jmem_is_heap_pointer (context_p, JMEM_HEAP_GET_ADDR_FROM_OFFSET (context_p, next_offset)));

      if (current_p->size >= required_size)
      {
        /* Region is sufficiently big, store address. */
        data_space_p = current_p;

        /* Region was larger than necessary. */
        if (current_p->size > required_size)
        {
          /* Get address of remaining space. */
          jmem_heap_free_t *const remaining_p = (jmem_heap_free_t *) ((uint8_t *) current_p + required_size);

          /* Update metadata. */
          JMEM_VALGRIND_DEFINED_SPACE (remaining_p, sizeof (jmem_heap_free_t));
          remaining_p->size = current_p->size - (uint32_t) required_size;
          remaining_p->next_offset = next_offset;
          JMEM_VALGRIND_NOACCESS_SPACE (remaining_p, sizeof (jmem_heap_free_t));

          /* Update list. */
          JMEM_VALGRIND_DEFINED_SPACE (prev_p, sizeof (jmem_heap_free_t));
          prev_p->next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (context_p, remaining_p);
          JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
        }
        /* Block is an exact fit. */
        else
        {
          /* Remove the region from the list. */
          JMEM_VALGRIND_DEFINED_SPACE (prev_p, sizeof (jmem_heap_free_t));
          prev_p->next_offset = next_offset;
          JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
        }

        context_p->jmem_heap_list_skip_p = prev_p;

        /* Found enough space. */
        context_p->jmem_heap_allocated_size += required_size;

        while (context_p->jmem_heap_allocated_size >= context_p->jmem_heap_limit)
        {
          context_p->jmem_heap_limit += context_p->gc_limit;
        }

        break;
      }

      JMEM_VALGRIND_NOACCESS_SPACE (current_p, sizeof (jmem_heap_free_t));
      /* Next in list. */
      prev_p = current_p;
      current_offset = next_offset;
    }
  }

  JMEM_VALGRIND_NOACCESS_SPACE (&context_p->heap_p->first, sizeof (jmem_heap_free_t));

  JJS_ASSERT ((uintptr_t) data_space_p % JMEM_ALIGNMENT == 0);
  JMEM_VALGRIND_MALLOCLIKE_SPACE (data_space_p, size);

  return (void *) data_space_p;
} /* jmem_heap_alloc */

/**
 * Allocation of memory block, reclaiming memory if the request cannot be fulfilled.
 *
 * Note:
 *    Each failed allocation attempt tries to reclaim memory with an increasing pressure,
 *    up to 'max_pressure', or until a sufficient memory block is found. When JMEM_PRESSURE_FULL
 *    is reached, the engine is terminated with JJS_FATAL_OUT_OF_MEMORY. The `max_pressure` argument
 *    can be used to limit the maximum pressure, and prevent the engine from terminating.
 *
 * @return NULL, if the required memory size is 0 or not enough memory
 *         pointer to the allocated memory block, if allocation is successful
 */
static void *
jmem_heap_gc_and_alloc_block (jjs_context_t *context_p, /**< JJS context */
                              const size_t size, /**< required memory size */
                              jmem_pressure_t max_pressure) /**< pressure limit */
{
  if (JJS_UNLIKELY (size == 0))
  {
    return NULL;
  }

  jmem_pressure_t pressure = JMEM_PRESSURE_NONE;

#if !JJS_MEM_GC_BEFORE_EACH_ALLOC
  if (context_p->jmem_heap_allocated_size + size >= context_p->jmem_heap_limit)
  {
    pressure = JMEM_PRESSURE_LOW;
    ecma_free_unused_memory (context_p, pressure);
  }
#else /* !JJS_MEM_GC_BEFORE_EACH_ALLOC */
  ecma_gc_run (context_p);
#endif /* JJS_MEM_GC_BEFORE_EACH_ALLOC */

  void *data_space_p = jmem_heap_alloc (context_p, size);

  while (JJS_UNLIKELY (data_space_p == NULL) && JJS_LIKELY (pressure < max_pressure))
  {
    pressure++;
    ecma_free_unused_memory (context_p, pressure);
    data_space_p = jmem_heap_alloc (context_p, size);
  }

  return data_space_p;
} /* jmem_heap_gc_and_alloc_block */

/**
 * Internal method for allocating a memory block.
 */
extern inline void *JJS_ATTR_HOT JJS_ATTR_ALWAYS_INLINE
jmem_heap_alloc_block_internal (jjs_context_t *context_p, /**< JJS context */
                                const size_t size) /**< required memory size */
{
  return jmem_heap_gc_and_alloc_block (context_p, size, JMEM_PRESSURE_FULL);
} /* jmem_heap_alloc_block_internal */

/**
 * Allocation of memory block, reclaiming unused memory if there is not enough.
 *
 * Note:
 *      If a sufficiently sized block can't be found, the engine will be terminated with JJS_FATAL_OUT_OF_MEMORY.
 *
 * @return NULL, if the required memory is 0
 *         pointer to allocated memory block, otherwise
 */
extern inline void *JJS_ATTR_HOT JJS_ATTR_ALWAYS_INLINE
jmem_heap_alloc_block (jjs_context_t *context_p, /**< JJS context */
                       const size_t size) /**< required memory size */
{
  void *block_p = jmem_heap_gc_and_alloc_block (context_p, size, JMEM_PRESSURE_FULL);
  JMEM_HEAP_STAT_ALLOC (context_p, size);
  return block_p;
} /* jmem_heap_alloc_block */

/**
 * Allocation of memory block, reclaiming unused memory if there is not enough.
 *
 * Note:
 *      If a sufficiently sized block can't be found, NULL will be returned.
 *
 * @return NULL, if the required memory size is 0
 *         also NULL, if the allocation has failed
 *         pointer to the allocated memory block, otherwise
 */
extern inline void *JJS_ATTR_HOT JJS_ATTR_ALWAYS_INLINE
jmem_heap_alloc_block_null_on_error (jjs_context_t *context_p, /**< JJS context */
                                     const size_t size) /**< required memory size */
{
  void *block_p = jmem_heap_gc_and_alloc_block (context_p, size, JMEM_PRESSURE_HIGH);

#if JJS_MEM_STATS
  if (block_p != NULL)
  {
    JMEM_HEAP_STAT_ALLOC (context_p, size);
  }
#endif /* JJS_MEM_STATS */

  return block_p;
} /* jmem_heap_alloc_block_null_on_error */

/**
 * Finds the block in the free block list which preceeds the argument block
 *
 * @return pointer to the preceeding block
 */
static jmem_heap_free_t *
jmem_heap_find_prev (jjs_context_t *context_p, /**< JJS context */
                     const jmem_heap_free_t *const block_p) /**< which memory block's predecessor we're looking for */
{
  const jmem_heap_free_t *prev_p;

  if (block_p > context_p->jmem_heap_list_skip_p)
  {
    prev_p = context_p->jmem_heap_list_skip_p;
  }
  else
  {
    prev_p = &context_p->heap_p->first;
  }

  JJS_ASSERT (jmem_is_heap_pointer (context_p, block_p));
  const uint32_t block_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (context_p, block_p);

  JMEM_VALGRIND_DEFINED_SPACE (prev_p, sizeof (jmem_heap_free_t));
  /* Find position of region in the list. */
  while (prev_p->next_offset < block_offset)
  {
    const jmem_heap_free_t *const next_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (context_p, prev_p->next_offset);
    JJS_ASSERT (jmem_is_heap_pointer (context_p, next_p));

    JMEM_VALGRIND_DEFINED_SPACE (next_p, sizeof (jmem_heap_free_t));
    JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
    prev_p = next_p;
  }

  JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
  return (jmem_heap_free_t *) prev_p;
} /* jmem_heap_find_prev */

/**
 * Inserts the block into the free chain after a specified block.
 *
 * Note:
 *     'jmem_heap_find_prev' can and should be used to find the previous free block
 */
static void
jmem_heap_insert_block (jjs_context_t *context_p, /**< JJS context */
                        jmem_heap_free_t *block_p, /**< block to insert */
                        jmem_heap_free_t *prev_p, /**< the free block after which to insert 'block_p' */
                        const size_t size) /**< size of the inserted block */
{
  JJS_ASSERT ((uintptr_t) block_p % JMEM_ALIGNMENT == 0);
  JJS_ASSERT (size % JMEM_ALIGNMENT == 0);

  JMEM_VALGRIND_NOACCESS_SPACE (block_p, size);

  JMEM_VALGRIND_DEFINED_SPACE (prev_p, sizeof (jmem_heap_free_t));
  jmem_heap_free_t *next_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (context_p, prev_p->next_offset);
  JMEM_VALGRIND_DEFINED_SPACE (block_p, sizeof (jmem_heap_free_t));
  JMEM_VALGRIND_DEFINED_SPACE (next_p, sizeof (jmem_heap_free_t));

  const uint32_t block_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (context_p, block_p);

  /* Update prev. */
  if (jmem_heap_get_region_end (prev_p) == block_p)
  {
    /* Can be merged. */
    prev_p->size += (uint32_t) size;
    JMEM_VALGRIND_NOACCESS_SPACE (block_p, sizeof (jmem_heap_free_t));
    block_p = prev_p;
  }
  else
  {
    block_p->size = (uint32_t) size;
    prev_p->next_offset = block_offset;
  }

  /* Update next. */
  if (jmem_heap_get_region_end (block_p) == next_p)
  {
    /* Can be merged. */
    block_p->size += next_p->size;
    block_p->next_offset = next_p->next_offset;
  }
  else
  {
    block_p->next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (context_p, next_p);
  }

  context_p->jmem_heap_list_skip_p = prev_p;

  JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
  JMEM_VALGRIND_NOACCESS_SPACE (block_p, sizeof (jmem_heap_free_t));
  JMEM_VALGRIND_NOACCESS_SPACE (next_p, sizeof (jmem_heap_free_t));
} /* jmem_heap_insert_block */

/**
 * Internal method for freeing a memory block.
 */
void JJS_ATTR_HOT
jmem_heap_free_block_internal (jjs_context_t *context_p, /**< JJS context */
                               void *ptr, /**< pointer to beginning of data space of the block */
                               const size_t size) /**< size of allocated region */
{
  JJS_ASSERT (size > 0);
  JJS_ASSERT (context_p->jmem_heap_limit >= context_p->jmem_heap_allocated_size);
  JJS_ASSERT (context_p->jmem_heap_allocated_size > 0);

  /* checking that ptr points to the heap */
  JJS_ASSERT (jmem_is_heap_pointer (context_p, ptr));
  JJS_ASSERT ((uintptr_t) ptr % JMEM_ALIGNMENT == 0);

  /* TODO: can't use size because it will be removed. is there a better way to do this? */
  /* find the page associated with this buffer. if not page, this is not a cell free. */
  jmem_cellocator_page_t *page_p = jmem_cellocator_find (&context_p->jmem_cellocator_32, ptr);

  if (page_p)
  {
    jmem_cellocator_cell_free (&context_p->jmem_cellocator_32, page_p, ptr);
    return;
  }

  const size_t aligned_size = (size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;

  jmem_heap_free_t *const block_p = (jmem_heap_free_t *) ptr;
  jmem_heap_free_t *const prev_p = jmem_heap_find_prev (context_p, block_p);
  jmem_heap_insert_block (context_p, block_p, prev_p, aligned_size);

  context_p->jmem_heap_allocated_size -= aligned_size;

  JMEM_VALGRIND_FREELIKE_SPACE (ptr);

  const uint32_t gc_limit = context_p->gc_limit;

  while (context_p->jmem_heap_allocated_size + gc_limit <= context_p->jmem_heap_limit)
  {
    context_p->jmem_heap_limit -= gc_limit;
  }

  JJS_ASSERT (context_p->jmem_heap_limit >= context_p->jmem_heap_allocated_size);
} /* jmem_heap_free_block_internal */

/**
 * Reallocates the memory region pointed to by 'ptr', changing the size of the allocated region.
 *
 * @return pointer to the reallocated region
 */
void *JJS_ATTR_HOT
jmem_heap_realloc_block (jjs_context_t *context_p, /**< JJS context */
                         void *ptr, /**< memory region to reallocate */
                         const size_t old_size, /**< current size of the region */
                         const size_t new_size) /**< desired new size */
{
  JJS_ASSERT (jmem_is_heap_pointer (context_p, ptr));
  JJS_ASSERT ((uintptr_t) ptr % JMEM_ALIGNMENT == 0);
  JJS_ASSERT (old_size != 0);
  JJS_ASSERT (new_size != 0);

  jmem_heap_free_t *const block_p = (jmem_heap_free_t *) ptr;
  const size_t aligned_new_size = (new_size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;
  const size_t aligned_old_size = (old_size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;

  /* search for the page of the ptr. if null, the ptr is not a cell */
  jmem_cellocator_page_t *page_p = jmem_cellocator_find (&context_p->jmem_cellocator_32, ptr);

  if (page_p)
  {
    if (aligned_new_size <= JMEM_CELLOCATOR_CELL_SIZE)
    {
      /* cell has extra space to accommodate the realloc */
      return ptr;
    }
    else
    {
      /* new size is bigger than a cell, transfer to the main heap. */
      void *chunk_p = jmem_heap_alloc_block_internal (context_p, aligned_new_size);

      if (chunk_p)
      {
        memcpy (chunk_p, ptr, aligned_old_size);
        jmem_cellocator_cell_free (&context_p->jmem_cellocator_32, page_p, ptr);
      }

      /* if null, pass it on */
      return chunk_p;
    }
  }

  if (aligned_old_size == aligned_new_size)
  {
    JMEM_VALGRIND_RESIZE_SPACE (block_p, old_size, new_size);
    JMEM_HEAP_STAT_FREE (context_p, old_size);
    JMEM_HEAP_STAT_ALLOC (context_p, new_size);
    return block_p;
  }

  if (aligned_new_size < aligned_old_size)
  {
    /* handle downsize from main heap to cellocator */
    if (aligned_new_size <= JMEM_CELLOCATOR_CELL_SIZE)
    {
      /* jmem_heap_alloc will go through cellocator for this size */
      void* new_buffer = jmem_heap_alloc (context_p, aligned_new_size);
      JJS_ASSERT (jmem_cellocator_find (&context_p->jmem_cellocator_32, new_buffer));

      if (new_buffer)
      {
        memcpy (new_buffer, ptr, aligned_new_size);
      }

      /* free the old block! */
      jmem_heap_free_block (context_p, ptr, aligned_old_size);

      return new_buffer;
    }

    JMEM_VALGRIND_RESIZE_SPACE (block_p, old_size, new_size);
    JMEM_HEAP_STAT_FREE (context_p, old_size);
    JMEM_HEAP_STAT_ALLOC (context_p, new_size);
    jmem_heap_insert_block (context_p,
                            (jmem_heap_free_t *) ((uint8_t *) block_p + aligned_new_size),
                            jmem_heap_find_prev (context_p, block_p),
                            aligned_old_size - aligned_new_size);

    context_p->jmem_heap_allocated_size -= (aligned_old_size - aligned_new_size);

    const uint32_t gc_limit = context_p->gc_limit;

    while (context_p->jmem_heap_allocated_size + gc_limit <= context_p->jmem_heap_limit)
    {
      context_p->jmem_heap_limit -= gc_limit;
    }

    return block_p;
  }

  void *ret_block_p = NULL;
  const size_t required_size = aligned_new_size - aligned_old_size;

#if !JJS_MEM_GC_BEFORE_EACH_ALLOC
  if (context_p->jmem_heap_allocated_size + required_size >= context_p->jmem_heap_limit)
  {
    ecma_free_unused_memory (context_p, JMEM_PRESSURE_LOW);
  }
#else /* !JJS_MEM_GC_BEFORE_EACH_ALLOC */
  ecma_gc_run (context_p);
#endif /* JJS_MEM_GC_BEFORE_EACH_ALLOC */

  jmem_heap_free_t *prev_p = jmem_heap_find_prev (context_p, block_p);
  JMEM_VALGRIND_DEFINED_SPACE (prev_p, sizeof (jmem_heap_free_t));
  jmem_heap_free_t *const next_p = JMEM_HEAP_GET_ADDR_FROM_OFFSET (context_p, prev_p->next_offset);

  /* Check if block can be extended at the end */
  if (((jmem_heap_free_t *) ((uint8_t *) block_p + aligned_old_size)) == next_p)
  {
    JMEM_VALGRIND_DEFINED_SPACE (next_p, sizeof (jmem_heap_free_t));

    if (required_size <= next_p->size)
    {
      /* Block can be extended, update the list. */
      if (required_size == next_p->size)
      {
        prev_p->next_offset = next_p->next_offset;
      }
      else
      {
        jmem_heap_free_t *const new_next_p = (jmem_heap_free_t *) ((uint8_t *) next_p + required_size);
        JMEM_VALGRIND_DEFINED_SPACE (new_next_p, sizeof (jmem_heap_free_t));
        new_next_p->next_offset = next_p->next_offset;
        new_next_p->size = (uint32_t) (next_p->size - required_size);
        JMEM_VALGRIND_NOACCESS_SPACE (new_next_p, sizeof (jmem_heap_free_t));
        prev_p->next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (context_p, new_next_p);
      }

      /* next_p will be marked as undefined space. */
      JMEM_VALGRIND_RESIZE_SPACE (block_p, old_size, new_size);
      ret_block_p = block_p;
    }
    else
    {
      JMEM_VALGRIND_NOACCESS_SPACE (next_p, sizeof (jmem_heap_free_t));
    }

    JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
  }
  /*
   * Check if block can be extended at the front.
   * This is less optimal because we need to copy the data, but still better than allocting a new block.
   */
  else if (jmem_heap_get_region_end (prev_p) == block_p)
  {
    if (required_size <= prev_p->size)
    {
      if (required_size == prev_p->size)
      {
        JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
        prev_p = jmem_heap_find_prev (context_p, prev_p);
        JMEM_VALGRIND_DEFINED_SPACE (prev_p, sizeof (jmem_heap_free_t));
        prev_p->next_offset = JMEM_HEAP_GET_OFFSET_FROM_ADDR (context_p, next_p);
      }
      else
      {
        prev_p->size = (uint32_t) (prev_p->size - required_size);
      }

      JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));

      ret_block_p = (uint8_t *) block_p - required_size;

      /* Mark the the new block as undefined so that we are able to write to it. */
      JMEM_VALGRIND_UNDEFINED_SPACE (ret_block_p, old_size);
      /* The blocks are likely to overlap, so mark the old block as defined memory again. */
      JMEM_VALGRIND_DEFINED_SPACE (block_p, old_size);
      memmove (ret_block_p, block_p, old_size);

      JMEM_VALGRIND_FREELIKE_SPACE (block_p);
      JMEM_VALGRIND_MALLOCLIKE_SPACE (ret_block_p, new_size);
      JMEM_VALGRIND_DEFINED_SPACE (ret_block_p, old_size);
    }
    else
    {
      JMEM_VALGRIND_NOACCESS_SPACE (prev_p, sizeof (jmem_heap_free_t));
    }
  }

  if (ret_block_p != NULL)
  {
    /* Managed to extend the block. Update memory usage and the skip pointer. */
    context_p->jmem_heap_list_skip_p = prev_p;
    context_p->jmem_heap_allocated_size += required_size;

    while (context_p->jmem_heap_allocated_size >= context_p->jmem_heap_limit)
    {
      context_p->jmem_heap_limit += context_p->gc_limit;
    }
  }
  else
  {
    /* Could not extend block. Allocate new region and copy the data. */
    /* jmem_heap_alloc_block_internal will adjust the allocated_size, but insert_block will not,
       so we reduce it here first, so that the limit calculation remains consistent. */
    context_p->jmem_heap_allocated_size -= aligned_old_size;
    ret_block_p = jmem_heap_alloc_block_internal (context_p, new_size);

    /* jmem_heap_alloc_block_internal may trigger garbage collection, which can create new free blocks
     * in the heap structure, so we need to look up the previous block again. */
    prev_p = jmem_heap_find_prev (context_p, block_p);

    memcpy (ret_block_p, block_p, old_size);
    jmem_heap_insert_block (context_p, block_p, prev_p, aligned_old_size);
    /* jmem_heap_alloc_block_internal will call JMEM_VALGRIND_MALLOCLIKE_SPACE */
    JMEM_VALGRIND_FREELIKE_SPACE (block_p);
  }

  JMEM_HEAP_STAT_FREE (context_p, old_size);
  JMEM_HEAP_STAT_ALLOC (context_p, new_size);
  return ret_block_p;
} /* jmem_heap_realloc_block */

/**
 * Free memory block
 */
extern inline void JJS_ATTR_HOT JJS_ATTR_ALWAYS_INLINE
jmem_heap_free_block (jjs_context_t *context_p, /**< JJS context */
                      void *ptr, /**< pointer to beginning of data space of the block */
                      const size_t size) /**< size of allocated region */
{
  jmem_heap_free_block_internal (context_p, ptr, size);
  JMEM_HEAP_STAT_FREE (context_p, size);
} /* jmem_heap_free_block */

#ifndef JJS_NDEBUG
/**
 * Check whether the pointer points to the heap
 *
 * Note:
 *      the routine should be used only for assertion checks
 *
 * @return true - if pointer points to the heap,
 *         false - otherwise
 */
bool
jmem_is_heap_pointer (jjs_context_t *context_p, /**< JJS context */
                      const void *pointer) /**< pointer */
{
  return ((uint8_t *) pointer >= context_p->heap_p->area && (uint8_t *) pointer <= context_p->jmem_area_end);
} /* jmem_is_heap_pointer */
#endif /* !JJS_NDEBUG */

#if JJS_MEM_STATS
/**
 * Get heap memory usage statistics
 */
void
jmem_heap_get_stats (jjs_context_t *context_p, /**< JJS context */
                     jmem_heap_stats_t *out_heap_stats_p) /**< [out] heap stats */
{
  JJS_ASSERT (out_heap_stats_p != NULL);

  *out_heap_stats_p = context_p->jmem_heap_stats;
} /* jmem_heap_get_stats */

/**
 * Print heap memory usage statistics
 */
void
jmem_heap_stats_print (jjs_context_t *context_p)
{
  jmem_heap_stats_t *heap_stats = &context_p->jmem_heap_stats;

  JJS_DEBUG_MSG (context_p, "Heap stats:\n");
  JJS_DEBUG_MSG (context_p, "  Heap size = %u bytes\n", (unsigned) heap_stats->size);
  JJS_DEBUG_MSG (context_p, "  Allocated = %u bytes\n", (unsigned) heap_stats->allocated_bytes);
  JJS_DEBUG_MSG (context_p, "  Peak allocated = %u bytes\n", (unsigned) heap_stats->peak_allocated_bytes);
  JJS_DEBUG_MSG (context_p, "  Waste = %u bytes\n", (unsigned) heap_stats->waste_bytes);
  JJS_DEBUG_MSG (context_p, "  Peak waste = %u bytes\n", (unsigned) heap_stats->peak_waste_bytes);
  JJS_DEBUG_MSG (context_p, "  Allocated byte code data = %u bytes\n", (unsigned) heap_stats->byte_code_bytes);
  JJS_DEBUG_MSG (context_p, "  Peak allocated byte code data = %u bytes\n", (unsigned) heap_stats->peak_byte_code_bytes);
  JJS_DEBUG_MSG (context_p, "  Allocated string data = %u bytes\n", (unsigned) heap_stats->string_bytes);
  JJS_DEBUG_MSG (context_p, "  Peak allocated string data = %u bytes\n", (unsigned) heap_stats->peak_string_bytes);
  JJS_DEBUG_MSG (context_p, "  Allocated object data = %u bytes\n", (unsigned) heap_stats->object_bytes);
  JJS_DEBUG_MSG (context_p, "  Peak allocated object data = %u bytes\n", (unsigned) heap_stats->peak_object_bytes);
  JJS_DEBUG_MSG (context_p, "  Allocated property data = %u bytes\n", (unsigned) heap_stats->property_bytes);
  JJS_DEBUG_MSG (context_p, "  Peak allocated property data = %u bytes\n", (unsigned) heap_stats->peak_property_bytes);
} /* jmem_heap_stats_print */

/**
 * Initalize heap memory usage statistics account structure
 */
void
jmem_heap_stat_init (jjs_context_t *context_p)
{
  context_p->jmem_heap_stats.size = JMEM_HEAP_AREA_SIZE (context_p);
} /* jmem_heap_stat_init */

/**
 * Account allocation
 */
void
jmem_heap_stat_alloc (jjs_context_t *context_p, size_t size) /**< Size of allocated block */
{
  const size_t aligned_size = (size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;
  const size_t waste_bytes = aligned_size - size;

  jmem_heap_stats_t *heap_stats = &context_p->jmem_heap_stats;

  heap_stats->allocated_bytes += aligned_size;
  heap_stats->waste_bytes += waste_bytes;

  if (heap_stats->allocated_bytes > heap_stats->peak_allocated_bytes)
  {
    heap_stats->peak_allocated_bytes = heap_stats->allocated_bytes;
  }

  if (heap_stats->waste_bytes > heap_stats->peak_waste_bytes)
  {
    heap_stats->peak_waste_bytes = heap_stats->waste_bytes;
  }
} /* jmem_heap_stat_alloc */

/**
 * Account freeing
 */
void
jmem_heap_stat_free (jjs_context_t *context_p, size_t size) /**< Size of freed block */
{
  const size_t aligned_size = (size + JMEM_ALIGNMENT - 1) / JMEM_ALIGNMENT * JMEM_ALIGNMENT;
  const size_t waste_bytes = aligned_size - size;

  jmem_heap_stats_t *heap_stats = &context_p->jmem_heap_stats;

  heap_stats->allocated_bytes -= aligned_size;
  heap_stats->waste_bytes -= waste_bytes;
} /* jmem_heap_stat_free */

#endif /* JJS_MEM_STATS */

/**
 * @}
 * @}
 */
