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

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-objects.h"

#include "jcontext.h"
#include "jrt.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-aggregateerror.inc.h"
#define BUILTIN_UNDERSCORED_ID  aggregate_error
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup aggregateerror ECMA AggregateError object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in AggregateError object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_aggregate_error_dispatch_call (ecma_context_t *context_p, /**< JJS context */
                                            const ecma_value_t *arguments_list_p, /**< arguments list */
                                            uint32_t arguments_list_len) /**< number of arguments */
{
  JJS_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);
  ecma_value_t message_val = ECMA_VALUE_UNDEFINED;
  ecma_value_t error_val = ECMA_VALUE_UNDEFINED;
  ecma_value_t options_val = ECMA_VALUE_UNDEFINED;

  if (arguments_list_len > 0)
  {
    error_val = arguments_list_p[0];

    if (arguments_list_len > 1)
    {
      message_val = arguments_list_p[1];
    }

    if (arguments_list_len > 2) {
      options_val = arguments_list_p[2];
    }
  }

  return ecma_new_aggregate_error (context_p, error_val, message_val, options_val);
} /* ecma_builtin_aggregate_error_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in AggregateError object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_aggregate_error_dispatch_construct (ecma_context_t *context_p, /**< JJS context */
                                                 const ecma_value_t *arguments_list_p, /**< arguments list */
                                                 uint32_t arguments_list_len) /**< number of arguments */
{
  ecma_object_t *proto_p = ecma_op_get_prototype_from_constructor (context_p,
                                                                   context_p->current_new_target_p,
                                                                   ECMA_BUILTIN_ID_AGGREGATE_ERROR_PROTOTYPE);

  if (proto_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t result = ecma_builtin_aggregate_error_dispatch_call (context_p, arguments_list_p, arguments_list_len);

  if (!ECMA_IS_VALUE_ERROR (result))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (context_p, result);
    ECMA_SET_NON_NULL_POINTER (context_p, object_p->u2.prototype_cp, proto_p);
  }

  ecma_deref_object (proto_p);

  return result;
} /* ecma_builtin_aggregate_error_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */
