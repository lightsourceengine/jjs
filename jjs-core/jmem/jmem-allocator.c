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
 * Allocator implementation
 */
#include "ecma-globals.h"

#include "jcontext.h"
#include "jmem.h"
#include "jrt-libc-includes.h"

#define JMEM_ALLOCATOR_INTERNAL
#include "jmem-allocator-internal.h"

#if JJS_MEM_STATS
/**
 * Register byte code allocation.
 */
void
jmem_stats_allocate_byte_code_bytes (jjs_context_t *context_p, size_t byte_code_size)
{
  jmem_heap_stats_t *heap_stats = &context_p->jmem_heap_stats;

  heap_stats->byte_code_bytes += byte_code_size;

  if (heap_stats->byte_code_bytes >= heap_stats->peak_byte_code_bytes)
  {
    heap_stats->peak_byte_code_bytes = heap_stats->byte_code_bytes;
  }
} /* jmem_stats_allocate_byte_code_bytes */

/**
 * Register byte code free.
 */
void
jmem_stats_free_byte_code_bytes (jjs_context_t *context_p, size_t byte_code_size)
{
  jmem_heap_stats_t *heap_stats = &context_p->jmem_heap_stats;

  JJS_ASSERT (heap_stats->byte_code_bytes >= byte_code_size);

  heap_stats->byte_code_bytes -= byte_code_size;
} /* jmem_stats_free_byte_code_bytes */

/**
 * Register string allocation.
 */
void
jmem_stats_allocate_string_bytes (jjs_context_t *context_p, size_t string_size)
{
  jmem_heap_stats_t *heap_stats = &context_p->jmem_heap_stats;

  heap_stats->string_bytes += string_size;

  if (heap_stats->string_bytes >= heap_stats->peak_string_bytes)
  {
    heap_stats->peak_string_bytes = heap_stats->string_bytes;
  }
} /* jmem_stats_allocate_string_bytes */

/**
 * Register string free.
 */
void
jmem_stats_free_string_bytes (jjs_context_t *context_p, size_t string_size)
{
  jmem_heap_stats_t *heap_stats = &context_p->jmem_heap_stats;

  JJS_ASSERT (heap_stats->string_bytes >= string_size);

  heap_stats->string_bytes -= string_size;
} /* jmem_stats_free_string_bytes */

/**
 * Register object allocation.
 */
void
jmem_stats_allocate_object_bytes (jjs_context_t *context_p, size_t object_size)
{
  jmem_heap_stats_t *heap_stats = &context_p->jmem_heap_stats;

  heap_stats->object_bytes += object_size;

  if (heap_stats->object_bytes >= heap_stats->peak_object_bytes)
  {
    heap_stats->peak_object_bytes = heap_stats->object_bytes;
  }
} /* jmem_stats_allocate_object_bytes */

/**
 * Register object free.
 */
void
jmem_stats_free_object_bytes (jjs_context_t *context_p, size_t object_size)
{
  jmem_heap_stats_t *heap_stats = &context_p->jmem_heap_stats;

  JJS_ASSERT (heap_stats->object_bytes >= object_size);

  heap_stats->object_bytes -= object_size;
} /* jmem_stats_free_object_bytes */

/**
 * Register property allocation.
 */
void
jmem_stats_allocate_property_bytes (jjs_context_t *context_p, size_t property_size)
{
  jmem_heap_stats_t *heap_stats = &context_p->jmem_heap_stats;

  heap_stats->property_bytes += property_size;

  if (heap_stats->property_bytes >= heap_stats->peak_property_bytes)
  {
    heap_stats->peak_property_bytes = heap_stats->property_bytes;
  }
} /* jmem_stats_allocate_property_bytes */

/**
 * Register property free.
 */
void
jmem_stats_free_property_bytes (jjs_context_t *context_p, size_t property_size)
{
  jmem_heap_stats_t *heap_stats = &context_p->jmem_heap_stats;

  JJS_ASSERT (heap_stats->property_bytes >= property_size);

  heap_stats->property_bytes -= property_size;
} /* jmem_stats_free_property_bytes */

#endif /* JJS_MEM_STATS */

/**
 * Initialize memory allocators.
 */
void
jmem_init (jjs_context_t *context_p)
{
  jmem_heap_init (context_p);
} /* jmem_init */

/**
 * Finalize memory allocators.
 */
void
jmem_finalize (jjs_context_t *context_p)
{
#if JJS_MEM_STATS
  if (context_p->context_flags & JJS_CONTEXT_FLAG_MEM_STATS)
  {
    jmem_heap_stats_print (context_p);
  }
#endif /* JJS_MEM_STATS */

  jmem_heap_finalize (context_p);
} /* jmem_finalize */

/**
 * Compress pointer
 *
 * @return packed pointer
 */
extern inline jmem_cpointer_t JJS_ATTR_PURE JJS_ATTR_ALWAYS_INLINE
jmem_compress_pointer (jjs_context_t *context_p, /**< JJS context */
                       const void *pointer_p) /**< pointer to compress */
{
  JJS_ASSERT (pointer_p != NULL);
  JJS_ASSERT (jmem_is_heap_pointer (context_p, pointer_p));

  uintptr_t uint_ptr = (uintptr_t) pointer_p;

  JJS_ASSERT (uint_ptr % JMEM_ALIGNMENT == 0);

#if defined(ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY) && JJS_CPOINTER_32_BIT
  JJS_ASSERT (((jmem_cpointer_t) uint_ptr) == uint_ptr);
#else /* !ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY || !JJS_CPOINTER_32_BIT */
  const uintptr_t heap_start = (uintptr_t) &context_p->heap_p->first;

  uint_ptr -= heap_start;
  uint_ptr >>= JMEM_ALIGNMENT_LOG;

#if JJS_CPOINTER_32_BIT
  JJS_ASSERT (uint_ptr <= UINT32_MAX);
#else /* !JJS_CPOINTER_32_BIT */
  JJS_ASSERT (uint_ptr <= UINT16_MAX);
#endif /* JJS_CPOINTER_32_BIT */
  JJS_ASSERT (uint_ptr != JMEM_CP_NULL);
#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY && JJS_CPOINTER_32_BIT */

  return (jmem_cpointer_t) uint_ptr;
} /* jmem_compress_pointer */

/**
 * Decompress pointer
 *
 * @return unpacked pointer
 */
extern inline void *JJS_ATTR_PURE JJS_ATTR_ALWAYS_INLINE
jmem_decompress_pointer (jjs_context_t *context_p, /**< JJS context */
                         uintptr_t compressed_pointer) /**< pointer to decompress */
{
  JJS_ASSERT (compressed_pointer != JMEM_CP_NULL);

  uintptr_t uint_ptr = compressed_pointer;

  JJS_ASSERT (((jmem_cpointer_t) uint_ptr) == uint_ptr);

#if defined(ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY) && JJS_CPOINTER_32_BIT
  JJS_UNUSED (context_p);
  JJS_ASSERT (uint_ptr % JMEM_ALIGNMENT == 0);
#else /* !ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY || !JJS_CPOINTER_32_BIT */
  const uintptr_t heap_start = (uintptr_t) &context_p->heap_p->first;

  uint_ptr <<= JMEM_ALIGNMENT_LOG;
  uint_ptr += heap_start;

  JJS_ASSERT (jmem_is_heap_pointer (context_p, (void *) uint_ptr));
#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY && JJS_CPOINTER_32_BIT */

  return (void *) uint_ptr;
} /* jmem_decompress_pointer */
