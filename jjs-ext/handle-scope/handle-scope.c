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

JJSX_STATIC_ASSERT (JJSX_SCOPE_PRELIST_SIZE < 32, JJSX_SCOPE_PRELIST_SIZE_MUST_BE_LESS_THAN_SIZE_OF_UINT8_T);

/**
 * Opens a new handle scope and attach it to current global scope as a child scope.
 *
 * @param result - [out value] opened scope.
 * @return status code, jjsx_handle_scope_ok if success.
 */
jjsx_handle_scope_status
jjsx_open_handle_scope (jjsx_handle_scope *result)
{
  *result = jjsx_handle_scope_alloc ();
  return jjsx_handle_scope_ok;
} /* jjsx_open_handle_scope */

/**
 * Release all JJS values attached to given scope
 *
 * @param scope - the scope of handles to be released.
 */
void
jjsx_handle_scope_release_handles (jjsx_handle_scope scope)
{
  size_t prelist_handle_count = scope->prelist_handle_count;
  if (prelist_handle_count == JJSX_HANDLE_PRELIST_SIZE && scope->handle_ptr != NULL)
  {
    jjsx_handle_t *a_handle = scope->handle_ptr;
    while (a_handle != NULL)
    {
      jjs_value_free (a_handle->jval);
      jjsx_handle_t *sibling = a_handle->sibling;
      jjs_heap_free (a_handle, sizeof (jjsx_handle_t));
      a_handle = sibling;
    }
    scope->handle_ptr = NULL;
    prelist_handle_count = JJSX_HANDLE_PRELIST_SIZE;
  }

  for (size_t idx = 0; idx < prelist_handle_count; idx++)
  {
    jjs_value_free (scope->handle_prelist[idx]);
  }
  scope->prelist_handle_count = 0;
} /* jjsx_handle_scope_release_handles */

/**
 * Close the scope and its child scopes and release all JJS values that
 * resides in the scopes.
 * Scopes must be closed in the reverse order from which they were created.
 *
 * @param scope - the scope closed.
 * @return status code, jjsx_handle_scope_ok if success.
 */
jjsx_handle_scope_status
jjsx_close_handle_scope (jjsx_handle_scope scope)
{
  /**
   * Release all handles related to given scope and its child scopes
   */
  jjsx_handle_scope a_scope = scope;
  do
  {
    jjsx_handle_scope_release_handles (a_scope);
    jjsx_handle_scope child = jjsx_handle_scope_get_child (a_scope);
    jjsx_handle_scope_free (a_scope);
    a_scope = child;
  } while (a_scope != NULL);

  return jjsx_handle_scope_ok;
} /* jjsx_close_handle_scope */

/**
 * Opens a new handle scope from which one object can be promoted to the outer scope
 * and attach it to current global scope as a child scope.
 *
 * @param result - [out value] opened escapable handle scope.
 * @return status code, jjsx_handle_scope_ok if success.
 */
jjsx_handle_scope_status
jjsx_open_escapable_handle_scope (jjsx_handle_scope *result)
{
  return jjsx_open_handle_scope (result);
} /* jjsx_open_escapable_handle_scope */

/**
 * Close the scope and its child scopes and release all JJS values that
 * resides in the scopes.
 * Scopes must be closed in the reverse order from which they were created.
 *
 * @param scope - the one to be closed.
 * @return status code, jjsx_handle_scope_ok if success.
 */
jjsx_handle_scope_status
jjsx_close_escapable_handle_scope (jjsx_handle_scope scope)
{
  return jjsx_close_handle_scope (scope);
} /* jjsx_close_escapable_handle_scope */

/**
 * Internal helper.
 * Escape a JJS value from the scope, yet did not promote it to outer scope.
 * An assertion of if parent exists shall be made before invoking this function.
 *
 * @param scope - scope of the handle added to.
 * @param idx - expected index of the handle in the scope's prelist.
 * @returns escaped JJS value id
 */
jjs_value_t
jjsx_hand_scope_escape_handle_from_prelist (jjsx_handle_scope scope, size_t idx)
{
  jjs_value_t jval = scope->handle_prelist[idx];
  if (scope->prelist_handle_count == JJSX_HANDLE_PRELIST_SIZE && scope->handle_ptr != NULL)
  {
    jjsx_handle_t *handle = scope->handle_ptr;
    scope->handle_ptr = handle->sibling;
    scope->handle_prelist[idx] = handle->jval;
    jjs_heap_free (handle, sizeof (jjsx_handle_t));
    return jval;
  }

  if (idx < JJSX_HANDLE_PRELIST_SIZE - 1)
  {
    scope->handle_prelist[idx] = scope->handle_prelist[scope->prelist_handle_count - 1];
  }
  return jval;
} /* jjsx_hand_scope_escape_handle_from_prelist */

/**
 * Internal helper.
 * Escape a JJS value from the given escapable handle scope.
 *
 * @param scope - the expected scope to be escaped from.
 * @param escapee - the JJS value to be escaped.
 * @param result - [out value] escaped JJS value result.
 * @param should_promote - true if the escaped value should be added to parent
 *                         scope after escaped from the given handle scope.
 * @return status code, jjsx_handle_scope_ok if success.
 */
jjsx_handle_scope_status
jjsx_escape_handle_internal (jjsx_escapable_handle_scope scope,
                               jjs_value_t escapee,
                               jjs_value_t *result,
                               bool should_promote)
{
  if (scope->escaped)
  {
    return jjsx_escape_called_twice;
  }

  jjsx_handle_scope parent = jjsx_handle_scope_get_parent (scope);
  if (parent == NULL)
  {
    return jjsx_handle_scope_mismatch;
  }

  bool found = false;
  {
    size_t found_idx = 0;
    size_t prelist_count = scope->prelist_handle_count;
    /**
     * Search prelist in a reversed order since last added handle
     * is possible the one to be escaped
     */
    for (size_t idx_plus_1 = prelist_count; idx_plus_1 > 0; --idx_plus_1)
    {
      if (escapee == scope->handle_prelist[idx_plus_1 - 1])
      {
        found = true;
        found_idx = idx_plus_1 - 1;
        break;
      }
    }

    if (found)
    {
      *result = jjsx_hand_scope_escape_handle_from_prelist (scope, found_idx);

      --scope->prelist_handle_count;
      if (should_promote)
      {
        scope->escaped = true;
        /**
         * Escape handle to parent scope
         */
        jjsx_create_handle_in_scope (*result, jjsx_handle_scope_get_parent (scope));
      }
      return jjsx_handle_scope_ok;
    }
  };

  if (scope->prelist_handle_count <= JJSX_HANDLE_PRELIST_SIZE && scope->handle_ptr == NULL)
  {
    return jjsx_handle_scope_mismatch;
  }

  /**
   * Handle chain list is already in an reversed order,
   * search through it as it is
   */
  jjsx_handle_t *handle = scope->handle_ptr;
  jjsx_handle_t *memo_handle = NULL;
  jjsx_handle_t *found_handle = NULL;
  while (!found)
  {
    if (handle == NULL)
    {
      return jjsx_handle_scope_mismatch;
    }
    if (handle->jval != escapee)
    {
      memo_handle = handle;
      handle = handle->sibling;
      continue;
    }
    /**
     * Remove found handle from current scope's handle chain
     */
    found = true;
    found_handle = handle;
    if (memo_handle == NULL)
    {
      // found handle is the first handle in heap
      scope->handle_ptr = found_handle->sibling;
    }
    else
    {
      memo_handle->sibling = found_handle->sibling;
    }
  }

  if (should_promote)
  {
    /**
     * Escape handle to parent scope
     */
    *result = jjsx_handle_scope_add_handle_to (found_handle, parent);
  }

  if (should_promote)
  {
    scope->escaped = true;
  }
  return jjsx_handle_scope_ok;
} /* jjsx_escape_handle_internal */

/**
 * Promotes the handle to the JavaScript object so that it is valid for the lifetime of
 * the outer scope. It can only be called once per scope. If it is called more than
 * once an error will be returned.
 *
 * @param scope - the expected scope to be escaped from.
 * @param escapee - the JJS value to be escaped.
 * @param result - [out value] escaped JJS value result.
 * @return status code, jjsx_handle_scope_ok if success.
 */
jjsx_handle_scope_status
jjsx_escape_handle (jjsx_escapable_handle_scope scope, jjs_value_t escapee, jjs_value_t *result)
{
  return jjsx_escape_handle_internal (scope, escapee, result, true);
} /* jjsx_escape_handle */

/**
 * Escape a handle from scope yet do not promote it to the outer scope.
 * Leave the handle's life time management up to user.
 *
 * @param scope - the expected scope to be removed from.
 * @param escapee - the JJS value to be removed.
 * @param result - [out value] removed JJS value result.
 * @return status code, jjsx_handle_scope_ok if success.
 */
jjsx_handle_scope_status
jjsx_remove_handle (jjsx_escapable_handle_scope scope, jjs_value_t escapee, jjs_value_t *result)
{
  return jjsx_escape_handle_internal (scope, escapee, result, false);
} /* jjsx_remove_handle */

/**
 * Try to reuse given handle if possible while adding to the scope.
 *
 * @param handle - the one to be added to the scope.
 * @param scope - the scope of handle to be added to.
 * @returns the JJS value id wrapped by given handle.
 */
jjs_value_t
jjsx_handle_scope_add_handle_to (jjsx_handle_t *handle, jjsx_handle_scope scope)
{
  size_t prelist_handle_count = scope->prelist_handle_count;
  if (prelist_handle_count < JJSX_HANDLE_PRELIST_SIZE)
  {
    ++scope->prelist_handle_count;
    jjs_value_t jval = handle->jval;
    jjs_heap_free (handle, sizeof (jjsx_handle_t));
    scope->handle_prelist[prelist_handle_count] = jval;
    return jval;
  }

  handle->sibling = scope->handle_ptr;
  scope->handle_ptr = handle;
  return handle->jval;
} /* jjsx_handle_scope_add_handle_to */

/**
 * Add given JJS value to the scope.
 *
 * @param jval - JJS value to be added to scope.
 * @param scope - the scope of the JJS value been expected to be added to.
 * @return JJS value that added to scope.
 */
jjs_value_t
jjsx_create_handle_in_scope (jjs_value_t jval, jjsx_handle_scope scope)
{
  size_t prelist_handle_count = scope->prelist_handle_count;
  if (prelist_handle_count < JJSX_HANDLE_PRELIST_SIZE)
  {
    scope->handle_prelist[prelist_handle_count] = jval;

    ++scope->prelist_handle_count;
    return jval;
  }
  jjsx_handle_t *handle = (jjsx_handle_t *) jjs_heap_alloc (sizeof (jjsx_handle_t));
  JJSX_ASSERT (handle != NULL);
  handle->jval = jval;

  handle->sibling = scope->handle_ptr;
  scope->handle_ptr = handle;

  return jval;
} /* jjsx_create_handle_in_scope */

/**
 * Add given JJS value to current top scope.
 *
 * @param jval - JJS value to be added to scope.
 * @return JJS value that added to scope.
 */
jjs_value_t
jjsx_create_handle (jjs_value_t jval)
{
  return jjsx_create_handle_in_scope (jval, jjsx_handle_scope_get_current ());
} /* jjsx_create_handle */
