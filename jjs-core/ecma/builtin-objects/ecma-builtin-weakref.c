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

#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"

#include "jcontext.h"

#if JJS_BUILTIN_WEAKREF

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-weakref.inc.h"
#define BUILTIN_UNDERSCORED_ID  weakref
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup weakref ECMA WeakRef object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in WeakRef object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_weakref_dispatch_call (ecma_context_t *context_p, /**< JJS context */
                                    const ecma_value_t *arguments_list_p, /**< arguments list */
                                    uint32_t arguments_list_len) /**< number of arguments */
{
  JJS_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_raise_type_error (context_p, ECMA_ERR_CONSTRUCTOR_WEAKREF_REQUIRES_NEW);
} /* ecma_builtin_weakref_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in WeakRef object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_weakref_dispatch_construct (ecma_context_t *context_p, /**< JJS context */
                                         const ecma_value_t *arguments_list_p, /**< arguments list */
                                         uint32_t arguments_list_len) /**< number of arguments */
{
  if (arguments_list_len == 0 || !ecma_is_value_object (arguments_list_p[0]))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_WEAKREF_TARGET_MUST_BE_AN_OBJECT);
  }

  JJS_ASSERT (context_p->current_new_target_p != NULL);

  ecma_object_t *proto_p =
    ecma_op_get_prototype_from_constructor (context_p, context_p->current_new_target_p, ECMA_BUILTIN_ID_WEAKREF_PROTOTYPE);

  if (JJS_UNLIKELY (proto_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_object_t *object_p = ecma_create_object (context_p, proto_p, sizeof (ecma_extended_object_t), ECMA_OBJECT_TYPE_CLASS);
  ecma_deref_object (proto_p);
  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;
  ext_obj_p->u.cls.type = ECMA_OBJECT_CLASS_WEAKREF;
  ext_obj_p->u.cls.u3.target = arguments_list_p[0];
  ecma_op_object_set_weak (context_p, ecma_get_object_from_value (context_p, arguments_list_p[0]), object_p);

  return ecma_make_object_value (context_p, object_p);
} /* ecma_builtin_weakref_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* JJS_BUILTIN_WEAKREF */
