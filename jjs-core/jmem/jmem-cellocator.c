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

#include "jcontext.h"

void
jmem_cellocator_init (jjs_context_t *context_p)
{
  /* defer page creation until it is needed. alloc'ing is not possible here because gc and heap are not ready! */
  (void) context_p;
}

void
jmem_cellocator_finalize (jjs_context_t *context_p)
{
  jmem_cellocator_page_t *iter_p = context_p->jmem_cellocator_32.pages;
  jmem_cellocator_page_t *next_p;

  while (iter_p)
  {
    next_p = iter_p->next_p;
    jmem_heap_free_block (context_p, iter_p, JMEM_CELLOCATOR_PAGE_SIZE (context_p->vm_cell_count));
    iter_p = next_p;
  }
}

bool
jmem_cellocator_add_page (jjs_context_t *context_p, jmem_cellocator_t *cellocator_p)
{
  uint8_t *chunk_p = jmem_heap_alloc_block_null_on_error (context_p, JMEM_CELLOCATOR_PAGE_SIZE(context_p->vm_cell_count));
  JJS_ASSERT (chunk_p);

  if (!chunk_p)
  {
    return false;
  }

  /* TODO: need a way to cleanup empty pages. gc mark? */
  jmem_cellocator_page_t *page_p = (jmem_cellocator_page_t *) chunk_p;

  *page_p = (jmem_cellocator_page_t) {
    .start_p = chunk_p + JMEM_CELLOCATOR_PAGE_HEADER_SIZE,
    .end_p = (chunk_p + JMEM_CELLOCATOR_PAGE_HEADER_SIZE)
             + (JMEM_CELLOCATOR_CELL_SIZE * (context_p->vm_cell_count - 1)),
    .next_p = cellocator_p->pages,
  };

  uint8_t *iter_p = page_p->start_p;
  jmem_cellocator_free_cell_t * cell_p;

  while (iter_p <= page_p->end_p)
  {
    cell_p = (jmem_cellocator_free_cell_t *) iter_p;
    cell_p->next_p = cellocator_p->free_cells;
    cellocator_p->free_cells = cell_p;

    iter_p += JMEM_CELLOCATOR_CELL_SIZE;
  }

  page_p->next_p = cellocator_p->pages;
  cellocator_p->pages = page_p;

  return true;
}

void *
jmem_cellocator_alloc (jmem_cellocator_t *cellocator_p)
{
  jmem_cellocator_free_cell_t *cell_p = cellocator_p->free_cells;

  if (cell_p)
  {
    cellocator_p->free_cells = cell_p->next_p;
  }

  return cell_p;
}

void
jmem_cellocator_cell_free (jmem_cellocator_t *cellocator_p, jmem_cellocator_page_t *page_p, void *chunk_p)
{
  (void) page_p;
  jmem_cellocator_free_cell_t *item_p = chunk_p;

  item_p->next_p = cellocator_p->free_cells;
  cellocator_p->free_cells = item_p;
}

jmem_cellocator_page_t *
jmem_cellocator_find (jmem_cellocator_t *cellocator_p, void *chunk_p)
{
  jmem_cellocator_page_t *iter_p = cellocator_p->pages;

  while (iter_p)
  {
    if ((uint8_t *) chunk_p >= iter_p->start_p && (uint8_t *) chunk_p <= iter_p->end_p)
    {
      return iter_p;
    }

    iter_p = iter_p->next_p;
  }

  return NULL;
}
