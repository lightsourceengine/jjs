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

#include <stdlib.h>

#include "handle-scope-internal.h"
#include "jext-common.h"

static jjsx_handle_scope_t jjsx_handle_scope_root = {
  .prelist_handle_count = 0,
  .handle_ptr = NULL,
};
static jjsx_handle_scope_t *jjsx_handle_scope_current = &jjsx_handle_scope_root;
static jjsx_handle_scope_pool_t jjsx_handle_scope_pool = {
  .count = 0,
  .start = NULL,
};

#define JJSX_HANDLE_SCOPE_POOL_PRELIST_LAST jjsx_handle_scope_pool.prelist + JJSX_SCOPE_PRELIST_SIZE - 1

#define JJSX_HANDLE_SCOPE_PRELIST_IDX(scope) (scope - jjsx_handle_scope_pool.prelist)

/**
 * Get current handle scope top of stack.
 */
jjsx_handle_scope_t *
jjsx_handle_scope_get_current (void)
{
  return jjsx_handle_scope_current;
} /* jjsx_handle_scope_get_current */

/**
 * Get root handle scope.
 */
jjsx_handle_scope_t *
jjsx_handle_scope_get_root (void)
{
  return &jjsx_handle_scope_root;
} /* jjsx_handle_scope_get_root */

/**
 * Determines if given handle scope is located in pre-allocated list.
 *
 * @param scope - the one to be determined.
 */
static bool
jjsx_handle_scope_is_in_prelist (jjsx_handle_scope_t *scope)
{
  return (jjsx_handle_scope_pool.prelist <= scope)
         && (scope <= (jjsx_handle_scope_pool.prelist + JJSX_SCOPE_PRELIST_SIZE - 1));
} /* jjsx_handle_scope_is_in_prelist */

/**
 * Get the parent of given handle scope.
 * If given handle scope is in prelist, the parent must be in prelist too;
 * if given is the first item of heap chain list, the parent must be the last one of prelist;
 * the parent must be in chain list otherwise.
 *
 * @param scope - the one to be permformed on.
 * @returns - the parent of the given scope.
 */
jjsx_handle_scope_t *
jjsx_handle_scope_get_parent (jjsx_handle_scope_t *scope)
{
  if (scope == &jjsx_handle_scope_root)
  {
    return NULL;
  }
  if (!jjsx_handle_scope_is_in_prelist (scope))
  {
    jjsx_handle_scope_dynamic_t *dy_scope = (jjsx_handle_scope_dynamic_t *) scope;
    if (dy_scope == jjsx_handle_scope_pool.start)
    {
      return JJSX_HANDLE_SCOPE_POOL_PRELIST_LAST;
    }
    jjsx_handle_scope_dynamic_t *parent = dy_scope->parent;
    return (jjsx_handle_scope_t *) parent;
  }
  if (scope == jjsx_handle_scope_pool.prelist)
  {
    return &jjsx_handle_scope_root;
  }
  return jjsx_handle_scope_pool.prelist + JJSX_HANDLE_SCOPE_PRELIST_IDX (scope) - 1;
} /* jjsx_handle_scope_get_parent */

/**
 * Get the child of given handle scope.
 * If the given handle scope is in heap chain list, its child must be in heap chain list too;
 * if the given handle scope is the last one of prelist, its child must be the first item of chain list;
 * the children are in prelist otherwise.
 *
 * @param scope - the one to be permformed on.
 * @returns the child of the given scope.
 */
jjsx_handle_scope_t *
jjsx_handle_scope_get_child (jjsx_handle_scope_t *scope)
{
  if (scope == &jjsx_handle_scope_root)
  {
    if (jjsx_handle_scope_pool.count > 0)
    {
      return jjsx_handle_scope_pool.prelist;
    }
    return NULL;
  }
  if (!jjsx_handle_scope_is_in_prelist (scope))
  {
    jjsx_handle_scope_dynamic_t *child = ((jjsx_handle_scope_dynamic_t *) scope)->child;
    return (jjsx_handle_scope_t *) child;
  }
  if (scope == JJSX_HANDLE_SCOPE_POOL_PRELIST_LAST)
  {
    return (jjsx_handle_scope_t *) jjsx_handle_scope_pool.start;
  }
  ptrdiff_t idx = JJSX_HANDLE_SCOPE_PRELIST_IDX (scope);
  if (idx < 0)
  {
    return NULL;
  }
  if ((unsigned long) idx >= jjsx_handle_scope_pool.count - 1)
  {
    return NULL;
  }
  return jjsx_handle_scope_pool.prelist + idx + 1;
} /* jjsx_handle_scope_get_child */

/**
 * Claims a handle scope either from prelist or allocating a new memory block,
 * and increment pool's scope count by 1, and set current scope to the newly claimed one.
 * If there are still available spaces in prelist, claims a block in prelist;
 * otherwise allocates a new memory block from heap and sets its fields to default values,
 * and link it to previously dynamically allocated scope, or link it to pool's start pointer.
 *
 * @returns the newly claimed handle scope pointer.
 */
jjsx_handle_scope_t *
jjsx_handle_scope_alloc (void)
{
  jjsx_handle_scope_t *scope;
  if (jjsx_handle_scope_pool.count < JJSX_SCOPE_PRELIST_SIZE)
  {
    scope = jjsx_handle_scope_pool.prelist + jjsx_handle_scope_pool.count;
  }
  else
  {
    jjsx_handle_scope_dynamic_t *dy_scope;
    dy_scope = (jjsx_handle_scope_dynamic_t *) jjs_heap_alloc (sizeof (jjsx_handle_scope_dynamic_t));
    JJSX_ASSERT (dy_scope != NULL);
    dy_scope->child = NULL;

    if (jjsx_handle_scope_pool.count != JJSX_SCOPE_PRELIST_SIZE)
    {
      jjsx_handle_scope_dynamic_t *dy_current = (jjsx_handle_scope_dynamic_t *) jjsx_handle_scope_current;
      dy_scope->parent = dy_current;
      dy_current->child = dy_scope;
    }
    else
    {
      jjsx_handle_scope_pool.start = dy_scope;
      dy_scope->parent = NULL;
    }

    scope = (jjsx_handle_scope_t *) dy_scope;
  }

  scope->prelist_handle_count = 0;
  scope->escaped = false;
  scope->handle_ptr = NULL;

  jjsx_handle_scope_current = scope;
  ++jjsx_handle_scope_pool.count;
  return (jjsx_handle_scope_t *) scope;
} /* jjsx_handle_scope_alloc */

/**
 * Deannounce a previously claimed handle scope, return it to pool
 * or free the allocated memory block.
 *
 * @param scope - the one to be freed.
 */
void
jjsx_handle_scope_free (jjsx_handle_scope_t *scope)
{
  if (scope == &jjsx_handle_scope_root)
  {
    return;
  }

  --jjsx_handle_scope_pool.count;
  if (scope == jjsx_handle_scope_current)
  {
    jjsx_handle_scope_current = jjsx_handle_scope_get_parent (scope);
  }

  if (!jjsx_handle_scope_is_in_prelist (scope))
  {
    jjsx_handle_scope_dynamic_t *dy_scope = (jjsx_handle_scope_dynamic_t *) scope;
    if (dy_scope == jjsx_handle_scope_pool.start)
    {
      jjsx_handle_scope_pool.start = dy_scope->child;
    }
    else if (dy_scope->parent != NULL)
    {
      dy_scope->parent->child = dy_scope->child;
    }
    jjs_heap_free (dy_scope, sizeof (jjsx_handle_scope_dynamic_t));
    return;
  }
  /**
   * Nothing to do with scopes in prelist
   */
} /* jjsx_handle_scope_free */
