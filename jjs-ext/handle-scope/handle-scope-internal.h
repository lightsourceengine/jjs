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

#ifndef JJSX_HANDLE_SCOPE_INTERNAL_H
#define JJSX_HANDLE_SCOPE_INTERNAL_H

#include "jjs-port.h"
#include "jjs.h"

#include "jjs-ext/handle-scope.h"

JJS_C_API_BEGIN

/** MARK: - handle-scope-allocator.c */
typedef struct jjsx_handle_scope_pool_s jjsx_handle_scope_pool_t;
/**
 * A linear allocating memory pool for type jjsx_handle_scope_t,
 * in which allocated item shall be released in reversed order of allocation
 */
struct jjsx_handle_scope_pool_s
{
  jjsx_handle_scope_t prelist[JJSX_SCOPE_PRELIST_SIZE]; /**< inlined handle scopes in the pool */
  size_t count; /**< number of handle scopes in the pool */
  jjsx_handle_scope_dynamic_t *start; /**< start address of dynamically allocated handle scope list */
};

jjsx_handle_scope_t *jjsx_handle_scope_get_parent (jjsx_handle_scope_t *scope);

jjsx_handle_scope_t *jjsx_handle_scope_get_child (jjsx_handle_scope_t *scope);

jjsx_handle_scope_t *jjsx_handle_scope_alloc (void);

void jjsx_handle_scope_free (jjsx_handle_scope_t *scope);
/** MARK: - END handle-scope-allocator.c */

/** MARK: - handle-scope.c */
void jjsx_handle_scope_release_handles (jjsx_handle_scope scope);

jjs_value_t jjsx_hand_scope_escape_handle_from_prelist (jjsx_handle_scope scope, size_t idx);

jjs_value_t jjsx_handle_scope_add_handle_to (jjsx_handle_t *handle, jjsx_handle_scope scope);

jjsx_handle_scope_status jjsx_escape_handle_internal (jjsx_escapable_handle_scope scope,
                                                          jjs_value_t escapee,
                                                          jjs_value_t *result,
                                                          bool should_promote);
/** MARK: - END handle-scope.c */

JJS_C_API_END

#endif /* !JJSX_HANDLE_SCOPE_INTERNAL_H */
