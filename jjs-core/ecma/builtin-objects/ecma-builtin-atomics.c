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

#include "ecma-atomics-object.h"
#include "ecma-builtins.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"

#include "jrt.h"

#if JJS_BUILTIN_ATOMICS

#define ECMA_BUILTINS_INTERNAL

#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
#define BUILTIN_CUSTOM_DISPATCH

/**
 * List of built-in routine identifiers.
 */
enum
{
  ECMA_ATOMICS_ROUTINE_START = 0, /**< Special value, should be ignored */
  ECMA_ATOMICS_ROUTINE_ADD, /**< Atomics add routine */
  ECMA_ATOMICS_ROUTINE_AND, /**< Atomics and routine */
  ECMA_ATOMICS_ROUTINE_COMPAREEXCHANGE, /**< Atomics compare exchange routine */
  ECMA_ATOMICS_ROUTINE_EXCHANGE, /**< Atomics exchange routine */
  ECMA_ATOMICS_ROUTINE_ISLOCKFREE, /**< Atomics is lock free routine */
  ECMA_ATOMICS_ROUTINE_LOAD, /**< Atomics load routine */
  ECMA_ATOMICS_ROUTINE_OR, /**< Atomics or routine */
  ECMA_ATOMICS_ROUTINE_STORE, /**< Atomics store routine */
  ECMA_ATOMICS_ROUTINE_SUB, /**< Atomics sub routine */
  ECMA_ATOMICS_ROUTINE_WAIT, /**< Atomics wait routine */
  ECMA_ATOMICS_ROUTINE_NOTIFY, /**< Atomics notify routine */
  ECMA_ATOMICS_ROUTINE_XOR, /**< Atomics xor routine */
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-atomics.inc.h"
#define BUILTIN_UNDERSCORED_ID  atomics

#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup atomics ECMA Atomics object built-in
 * @{
 */

/**
 * The Atomics object's 'compareExchange' routine
 *
 * See also: ES11 24.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_atomics_compare_exchange (ecma_context_t *context_p, /**< JJS context */
                                       ecma_value_t typedarray, /**< typedArray argument */
                                       ecma_value_t index, /**< index argument */
                                       ecma_value_t expected_value, /**< expectedValue argument */
                                       ecma_value_t replacement_value) /**< replacementValue argument*/
{
  JJS_UNUSED (typedarray);
  JJS_UNUSED (index);
  JJS_UNUSED (expected_value);
  JJS_UNUSED (replacement_value);

  return ecma_make_uint32_value (context_p, 0);
} /* ecma_builtin_atomics_compare_exchange */

/**
 * The Atomics object's 'isLockFree' routine
 *
 * See also: ES11 24.4.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_atomics_is_lock_free (ecma_context_t *context_p, /**< JJS context */
                                   ecma_value_t size) /**< size argument */
{
  JJS_UNUSED_ALL (context_p, size);

  return ECMA_VALUE_FALSE;
} /* ecma_builtin_atomics_is_lock_free */

/**
 * The Atomics object's 'store' routine
 *
 * See also: ES11 24.4.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_atomics_store (ecma_context_t *context_p, /**< JJS context */
                            ecma_value_t typedarray, /**< typedArray argument */
                            ecma_value_t index, /**< index argument */
                            ecma_value_t value) /**< value argument */
{
  JJS_UNUSED (typedarray);
  JJS_UNUSED (index);
  JJS_UNUSED (value);

  return ecma_make_uint32_value (context_p, 0);
} /* ecma_builtin_atomics_store */

/**
 * The Atomics object's 'wait' routine
 *
 * See also: ES11 24.4.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_atomics_wait (ecma_context_t *context_p, /**< JJS context */
                           ecma_value_t typedarray, /**< typedArray argument */
                           ecma_value_t index, /**< index argument */
                           ecma_value_t value, /**< value argument */
                           ecma_value_t timeout) /**< timeout argument */
{
  JJS_UNUSED (typedarray);
  JJS_UNUSED (index);
  JJS_UNUSED (value);
  JJS_UNUSED (timeout);

  return ecma_make_uint32_value (context_p, 0);
} /* ecma_builtin_atomics_wait */

/**
 * The Atomics object's 'notify' routine
 *
 * See also: ES11 24.4.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_atomics_notify (ecma_context_t *context_p, /**< JJS context */
                             ecma_value_t typedarray, /**< typedArray argument */
                             ecma_value_t index, /**< index argument */
                             ecma_value_t count) /**< count argument */
{
  JJS_UNUSED (typedarray);
  JJS_UNUSED (index);
  JJS_UNUSED (count);

  return ecma_make_uint32_value (context_p, 0);
} /* ecma_builtin_atomics_notify */

/**
 * Dispatcher of the built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_atomics_dispatch_routine (ecma_context_t *context_p, /**< JJS context */
                                       uint8_t builtin_routine_id, /**< built-in wide routine identifier */
                                       ecma_value_t this_arg, /**< 'this' argument value */
                                       const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                               *   passed to routine */
                                       uint32_t arguments_number) /**< length of arguments' list */
{
  JJS_UNUSED (this_arg);
  ecma_value_t arg1 = arguments_list_p[0];
  ecma_value_t arg2 = arguments_list_p[1];
  ecma_value_t arg3 = arguments_list_p[2];
  ecma_value_t arg4 = (arguments_number > 3) ? arguments_list_p[3] : ECMA_VALUE_UNDEFINED;

  ecma_atomics_op_t type;

  switch (builtin_routine_id)
  {
    case ECMA_ATOMICS_ROUTINE_ADD:
    {
      type = ECMA_ATOMICS_ADD;
      break;
    }
    case ECMA_ATOMICS_ROUTINE_AND:
    {
      type = ECMA_ATOMICS_AND;
      break;
    }
    case ECMA_ATOMICS_ROUTINE_COMPAREEXCHANGE:
    {
      return ecma_builtin_atomics_compare_exchange (context_p, arg1, arg2, arg3, arg4);
    }
    case ECMA_ATOMICS_ROUTINE_EXCHANGE:
    {
      type = ECMA_ATOMICS_EXCHANGE;
      break;
    }
    case ECMA_ATOMICS_ROUTINE_ISLOCKFREE:
    {
      return ecma_builtin_atomics_is_lock_free (context_p, arg1);
    }
    case ECMA_ATOMICS_ROUTINE_LOAD:
    {
      return ecma_atomic_load (context_p, arg1, arg2);
    }
    case ECMA_ATOMICS_ROUTINE_OR:
    {
      type = ECMA_ATOMICS_OR;
      break;
    }
    case ECMA_ATOMICS_ROUTINE_STORE:
    {
      return ecma_builtin_atomics_store (context_p, arg1, arg2, arg3);
    }
    case ECMA_ATOMICS_ROUTINE_SUB:
    {
      type = ECMA_ATOMICS_SUBTRACT;
      break;
    }
    case ECMA_ATOMICS_ROUTINE_WAIT:
    {
      return ecma_builtin_atomics_wait (context_p, arg1, arg2, arg3, arg4);
    }
    case ECMA_ATOMICS_ROUTINE_NOTIFY:
    {
      return ecma_builtin_atomics_notify (context_p, arg1, arg2, arg3);
    }
    case ECMA_ATOMICS_ROUTINE_XOR:
    {
      type = ECMA_ATOMICS_XOR;
      break;
    }
    default:
    {
      JJS_UNREACHABLE ();
    }
  }
  return ecma_atomic_read_modify_write (context_p, arg1, arg2, arg3, type);
} /* ecma_builtin_atomics_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* JJS_BUILTIN_ATOMICS */
