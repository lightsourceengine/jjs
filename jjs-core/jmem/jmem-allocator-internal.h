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

#ifndef JMEM_ALLOCATOR_INTERNAL_H
#define JMEM_ALLOCATOR_INTERNAL_H

#ifndef JMEM_ALLOCATOR_INTERNAL
#error "The header is for internal routines of memory allocator component. Please, don't use the routines directly."
#endif /* !JMEM_ALLOCATOR_INTERNAL */

/** \addtogroup mem Memory allocation
 * @{
 */

/**
 * @{
 * Valgrind-related options and headers
 */
#if JJS_VALGRIND
#include "memcheck.h"

#define JMEM_VALGRIND_NOACCESS_SPACE(p, s)   VALGRIND_MAKE_MEM_NOACCESS ((p), (s))
#define JMEM_VALGRIND_UNDEFINED_SPACE(p, s)  VALGRIND_MAKE_MEM_UNDEFINED ((p), (s))
#define JMEM_VALGRIND_DEFINED_SPACE(p, s)    VALGRIND_MAKE_MEM_DEFINED ((p), (s))
#define JMEM_VALGRIND_MALLOCLIKE_SPACE(p, s) VALGRIND_MALLOCLIKE_BLOCK ((p), (s), 0, 0)
#define JMEM_VALGRIND_RESIZE_SPACE(p, o, n)  VALGRIND_RESIZEINPLACE_BLOCK ((p), (o), (n), 0)
#define JMEM_VALGRIND_FREELIKE_SPACE(p)      VALGRIND_FREELIKE_BLOCK ((p), 0)
#else /* !JJS_VALGRIND */
#define JMEM_VALGRIND_NOACCESS_SPACE(p, s)
#define JMEM_VALGRIND_UNDEFINED_SPACE(p, s)
#define JMEM_VALGRIND_DEFINED_SPACE(p, s)
#define JMEM_VALGRIND_MALLOCLIKE_SPACE(p, s)
#define JMEM_VALGRIND_RESIZE_SPACE(p, o, n)
#define JMEM_VALGRIND_FREELIKE_SPACE(p)
#endif /* JJS_VALGRIND */
/** @} */

void jmem_heap_init (jjs_context_t *context_p);
void jmem_heap_finalize (jjs_context_t *context_p);
bool jmem_is_heap_pointer (jjs_context_t *context_p, const void *pointer);
void *jmem_heap_alloc_block_internal (jjs_context_t *context_p, const size_t size);
void jmem_heap_free_block_internal (jjs_context_t *context_p, void *ptr, const size_t size);

/**
 * @{
 * JJS mem-stat definitions
 */
#if JJS_MEM_STATS
void jmem_heap_stat_init (jjs_context_t *context_p);
void jmem_heap_stat_alloc (jjs_context_t *context_p, size_t num);
void jmem_heap_stat_free (jjs_context_t *context_p, size_t num);

#define JMEM_HEAP_STAT_INIT(ctx)    jmem_heap_stat_init (ctx)
#define JMEM_HEAP_STAT_ALLOC(ctx, v1) jmem_heap_stat_alloc (ctx, v1)
#define JMEM_HEAP_STAT_FREE(ctx, v1)  jmem_heap_stat_free (ctx, v1)
#else /* !JJS_MEM_STATS */
#define JMEM_HEAP_STAT_INIT(ctx) JJS_UNUSED (ctx)
#define JMEM_HEAP_STAT_ALLOC(ctx, v1) JJS_UNUSED_ALL (ctx, v1)
#define JMEM_HEAP_STAT_FREE(ctx, v1)  JJS_UNUSED_ALL (ctx, v1)
#endif /* JJS_MEM_STATS */

/** @} */

#endif /* !JMEM_ALLOCATOR_INTERNAL_H */
