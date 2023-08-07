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

#ifndef JJSX_HANDLE_SCOPE_H
#define JJSX_HANDLE_SCOPE_H

#include "jjs.h"

JJS_C_API_BEGIN

#ifndef JJSX_HANDLE_PRELIST_SIZE
#define JJSX_HANDLE_PRELIST_SIZE 20
#endif /* !defined(JJSX_HANDLE_PRELIST_SIZE) */

#ifndef JJSX_SCOPE_PRELIST_SIZE
#define JJSX_SCOPE_PRELIST_SIZE 20
#endif /* !defined(JJSX_SCOPE_PRELIST_SIZE) */

typedef struct jjsx_handle_t jjsx_handle_t;
/**
 * Dynamically allocated handle in the scopes.
 * Scopes has it's own size-limited linear storage of handles. Still there
 * might be not enough space left for new handles, dynamically allocated
 * `jjsx_handle_t` could ease the pre-allocated linear memory burden.
 */
struct jjsx_handle_t
{
  jjs_value_t jval; /**< JJS value of the handle bound to */
  jjsx_handle_t *sibling; /**< next sibling the the handle */
};

#define JJSX_HANDLE_SCOPE_FIELDS                          \
  jjs_value_t handle_prelist[JJSX_HANDLE_PRELIST_SIZE]; \
  uint8_t prelist_handle_count;                             \
  bool escaped;                                             \
  jjsx_handle_t *handle_ptr

typedef struct jjsx_handle_scope_s jjsx_handle_scope_t;
typedef jjsx_handle_scope_t *jjsx_handle_scope;
typedef jjsx_handle_scope_t *jjsx_escapable_handle_scope;
/**
 * Inlined simple handle scope type.
 */
struct jjsx_handle_scope_s
{
  JJSX_HANDLE_SCOPE_FIELDS; /**< common handle scope fields */
};

typedef struct jjsx_handle_scope_dynamic_s jjsx_handle_scope_dynamic_t;
/**
 * Dynamically allocated handle scope type.
 */
struct jjsx_handle_scope_dynamic_s
{
  JJSX_HANDLE_SCOPE_FIELDS; /**< common handle scope fields */
  jjsx_handle_scope_dynamic_t *child; /**< child dynamically allocated handle scope */
  jjsx_handle_scope_dynamic_t *parent; /**< parent dynamically allocated handle scope */
};

#undef JJSX_HANDLE_SCOPE_FIELDS

typedef enum
{
  jjsx_handle_scope_ok = 0,

  jjsx_escape_called_twice,
  jjsx_handle_scope_mismatch,
} jjsx_handle_scope_status;

jjsx_handle_scope_status jjsx_open_handle_scope (jjsx_handle_scope *result);

jjsx_handle_scope_status jjsx_close_handle_scope (jjsx_handle_scope scope);

jjsx_handle_scope_status jjsx_open_escapable_handle_scope (jjsx_handle_scope *result);

jjsx_handle_scope_status jjsx_close_escapable_handle_scope (jjsx_handle_scope scope);

jjsx_handle_scope_status
jjsx_escape_handle (jjsx_escapable_handle_scope scope, jjs_value_t escapee, jjs_value_t *result);

/**
 * Completely escape a handle from handle scope,
 * leave life time management totally up to user.
 */
jjsx_handle_scope_status
jjsx_remove_handle (jjsx_escapable_handle_scope scope, jjs_value_t escapee, jjs_value_t *result);

jjs_value_t jjsx_create_handle (jjs_value_t jval);

jjs_value_t jjsx_create_handle_in_scope (jjs_value_t jval, jjsx_handle_scope scope);

/** MARK: - handle-scope-allocator.c */
jjsx_handle_scope_t *jjsx_handle_scope_get_current (void);

jjsx_handle_scope_t *jjsx_handle_scope_get_root (void);
/** MARK: - END handle-scope-allocator.c */

JJS_C_API_END

#endif /* !JJSX_HANDLE_SCOPE_H */
