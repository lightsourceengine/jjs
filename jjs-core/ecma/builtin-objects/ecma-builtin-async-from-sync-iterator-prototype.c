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

#include "jjs-types.h"

#include "ecma-builtin-handlers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-objects.h"
#include "ecma-promise-object.h"

#include "jcontext.h"
#include "jrt.h"
#include "lit-magic-strings.h"
#include "lit-strings.h"
#include "opcodes.h"
#include "vm-defines.h"

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
  ECMA_ASYNC_FROM_SYNC_ITERATOR_PROTOTYPE_ROUTINE_START = 0, /**< buitlin routine start id */
  ECMA_ASYNC_FROM_SYNC_ITERATOR_PROTOTYPE_ROUTINE_NEXT, /**< 'next' routine v11, 25.1.4.2.1  */
  ECMA_ASYNC_FROM_SYNC_ITERATOR_PROTOTYPE_ROUTINE_RETURN, /**< 'return' routine v11, 25.1.4.2.2  */
  ECMA_ASYNC_FROM_SYNC_ITERATOR_PROTOTYPE_ROUTINE_THROW /**< 'throw' routine v11, 25.1.4.2.3  */
} ecma_async_from_sync_iterator_operation_type_t;

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-async-from-sync-iterator-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID  async_from_sync_iterator_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup asyncfromsynciteratorprototype ECMA %AsyncFromSyncIteratorPrototype% object built-in
 * @{
 */

/**
 * AsyncFromSyncIteratorContinuation operation
 *
 * See also:
 *         ECMAScript v11, 25.1.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_op_async_from_sync_iterator_prototype_continuation (ecma_context_t *context_p, /**< JJS context */
                                                         ecma_value_t result, /**< routine's 'result' argument */
                                                         ecma_object_t *capability_obj_p) /**< promise capability */
{
  /* 1. */
  ecma_value_t done = ecma_op_iterator_complete (context_p, result);

  /* 2. */
  if (ECMA_IS_VALUE_ERROR (ecma_op_if_abrupt_reject_promise (context_p, &done, capability_obj_p)))
  {
    return done;
  }

  uint16_t done_flag = ecma_is_value_false (done) ? 0 : (1 << ECMA_NATIVE_HANDLER_COMMON_FLAGS_SHIFT);
  ecma_free_value (context_p, done);

  /* 3. */
  ecma_value_t value = ecma_op_iterator_value (context_p, result);

  /* 4. */
  if (ECMA_IS_VALUE_ERROR (ecma_op_if_abrupt_reject_promise (context_p, &value, capability_obj_p)))
  {
    return value;
  }

  /* 5. */
  ecma_value_t builtin_promise = ecma_make_object_value (context_p, ecma_builtin_get (context_p, ECMA_BUILTIN_ID_PROMISE));
  ecma_value_t value_wrapper = ecma_promise_reject_or_resolve (context_p, builtin_promise, value, true);
  ecma_free_value (context_p, value);

  /* 6. */
  if (ECMA_IS_VALUE_ERROR (ecma_op_if_abrupt_reject_promise (context_p, &value_wrapper, capability_obj_p)))
  {
    return value_wrapper;
  }

  /* 8 - 9. */
  ecma_object_t *on_fullfilled = ecma_op_create_native_handler (context_p,
                                                                ECMA_NATIVE_HANDLER_ASYNC_FROM_SYNC_ITERATOR_UNWRAP,
                                                                sizeof (ecma_extended_object_t));
  ((ecma_extended_object_t *) on_fullfilled)->u.built_in.u2.routine_flags = (uint8_t) done_flag;

  /* 10. */
  ecma_value_t then_result = ecma_promise_perform_then (context_p,
                                                        value_wrapper,
                                                        ecma_make_object_value (context_p, on_fullfilled),
                                                        ECMA_VALUE_UNDEFINED,
                                                        capability_obj_p);

  JJS_ASSERT (!ECMA_IS_VALUE_ERROR (then_result));
  ecma_deref_object (on_fullfilled);
  ecma_free_value (context_p, value_wrapper);

  /* 11. */
  return then_result;
} /* ecma_op_async_from_sync_iterator_prototype_continuation */

/**
 * The %AsyncFromSyncIteratorPrototype% object's 'next' routine
 *
 * See also:
 *         ECMAScript v11, 25.1.4.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_async_from_sync_iterator_prototype_next (ecma_context_t *context_p, /**< JJS context */
                                                      ecma_async_from_sync_iterator_object_t *iter_p, /**< iterator
                                                                                                       *   record*/
                                                      ecma_object_t *capability_p, /**< promise capability */
                                                      ecma_value_t value) /**< routine's 'value' argument */
{
  /* 5. */
  ecma_value_t next_result =
    ecma_op_iterator_next (context_p, iter_p->header.u.cls.u3.sync_iterator, iter_p->sync_next_method, value);

  /* 6. */
  if (ECMA_IS_VALUE_ERROR (ecma_op_if_abrupt_reject_promise (context_p, &next_result, capability_p)))
  {
    return next_result;
  }

  /* 7. */
  ecma_value_t result = ecma_op_async_from_sync_iterator_prototype_continuation (context_p, next_result, capability_p);
  ecma_free_value (context_p, next_result);

  return result;
} /* ecma_builtin_async_from_sync_iterator_prototype_next */

/**
 * The %AsyncFromSyncIteratorPrototype% object's 'return' and 'throw' routines
 *
 * See also:
 *         ECMAScript v11, 25.1.4.2.2
 *         ECMAScript v11, 25.1.4.2.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_async_from_sync_iterator_prototype_do (ecma_context_t *context_p, /**< JJS context */
                                                    ecma_async_from_sync_iterator_object_t *iter_p, /**< iterator
                                                                                                     *   record*/
                                                    ecma_object_t *capability_obj_p, /**< promise capability */
                                                    ecma_value_t value, /**< routine's 'value' argument */
                                                    lit_magic_string_id_t method_id) /**< method id */
{
  /* 5. */
  ecma_value_t sync_iterator = iter_p->header.u.cls.u3.sync_iterator;
  ecma_value_t method = ecma_op_get_method_by_magic_id (context_p, sync_iterator, method_id);

  /* 6. */
  if (ECMA_IS_VALUE_ERROR (ecma_op_if_abrupt_reject_promise (context_p, &method, capability_obj_p)))
  {
    return method;
  }

  ecma_promise_capabality_t *capability_p = (ecma_promise_capabality_t *) capability_obj_p;

  ecma_value_t call_arg;
  uint32_t arg_size;

  if (ecma_is_value_empty (value))
  {
    arg_size = 0;
    call_arg = ECMA_VALUE_UNDEFINED;
  }
  else
  {
    arg_size = 1;
    call_arg = value;
  }

  /* 7. */
  if (ecma_is_value_undefined (method))
  {
    ecma_value_t func_obj;

    if (method_id == LIT_MAGIC_STRING_RETURN)
    {
      /* 7.a. */
      call_arg = ecma_create_iter_result_object (context_p, call_arg, ECMA_VALUE_TRUE);
      arg_size = 1;
      func_obj = capability_p->resolve;
    }
    else
    {
      func_obj = capability_p->reject;
    }

    /* 7.b. */
    ecma_value_t resolve =
      ecma_op_function_call (context_p, ecma_get_object_from_value (context_p, func_obj), ECMA_VALUE_UNDEFINED, &call_arg, arg_size);
    JJS_ASSERT (!ECMA_IS_VALUE_ERROR (resolve));
    ecma_free_value (context_p, resolve);

    if (method_id == LIT_MAGIC_STRING_RETURN)
    {
      ecma_free_value (context_p, call_arg);
    }

    /* 7.c. */
    return ecma_copy_value (context_p, capability_p->header.u.cls.u3.promise);
  }

  /* 8. */
  ecma_value_t call_result = ecma_op_function_validated_call (context_p, method, sync_iterator, &call_arg, arg_size);
  ecma_free_value (context_p, method);

  /* 9. */
  if (ECMA_IS_VALUE_ERROR (ecma_op_if_abrupt_reject_promise (context_p, &call_result, capability_obj_p)))
  {
    return call_result;
  }

  /* 10. */
  if (!ecma_is_value_object (call_result))
  {
    ecma_free_value (context_p, call_result);

#if JJS_ERROR_MESSAGES
    const lit_utf8_byte_t *msg_p = (lit_utf8_byte_t *) ecma_get_error_msg (ECMA_ERR_ARGUMENT_IS_NOT_AN_OBJECT);
    lit_utf8_size_t msg_size = ecma_get_error_size (ECMA_ERR_ARGUMENT_IS_NOT_AN_OBJECT);
    ecma_string_t *error_msg_p = ecma_new_ecma_string_from_ascii (context_p, msg_p, msg_size);
#else /* !JJS_ERROR_MESSAGES */
    ecma_string_t *error_msg_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
#endif /* JJS_ERROR_MESSAGES */

    ecma_object_t *type_error_obj_p = ecma_new_standard_error (context_p, JJS_ERROR_TYPE, error_msg_p);

#if JJS_ERROR_MESSAGES
    ecma_deref_ecma_string (context_p, error_msg_p);
#endif /* JJS_ERROR_MESSAGES */

    ecma_value_t type_error = ecma_make_object_value (context_p, type_error_obj_p);

    /* 10.a. */
    ecma_value_t reject =
      ecma_op_function_call (context_p, ecma_get_object_from_value (context_p, capability_p->reject), ECMA_VALUE_UNDEFINED, &type_error, 1);
    JJS_ASSERT (!ECMA_IS_VALUE_ERROR (reject));
    ecma_deref_object (type_error_obj_p);
    ecma_free_value (context_p, reject);

    /* 10.b. */
    return ecma_copy_value (context_p, capability_p->header.u.cls.u3.promise);
  }

  ecma_value_t result = ecma_op_async_from_sync_iterator_prototype_continuation (context_p, call_result, capability_obj_p);
  ecma_free_value (context_p, call_result);

  return result;
} /* ecma_builtin_async_from_sync_iterator_prototype_do */

/**
 * Dispatcher of the %AsyncFromSyncIteratorPrototype% built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_async_from_sync_iterator_prototype_dispatch_routine (ecma_context_t *context_p, /**< JJS context */
                                                                  uint8_t builtin_routine_id, /**< built-in wide
                                                                                               *   routine
                                                                                               *   identifier */
                                                                  ecma_value_t this_arg, /**< 'this' argument value */
                                                                  const ecma_value_t arguments_list_p[], /**< list of
                                                                                                          *   arguments
                                                                                                          *   passed to
                                                                                                          *   routine */
                                                                  uint32_t arguments_number) /**< length of
                                                                                              *   arguments' list */
{
  JJS_UNUSED (arguments_number);
  JJS_ASSERT (ecma_is_value_object (this_arg));

  ecma_object_t *this_obj_p = ecma_get_object_from_value (context_p, this_arg);

  JJS_ASSERT (ecma_object_class_is (this_obj_p, ECMA_OBJECT_CLASS_ASYNC_FROM_SYNC_ITERATOR));

  ecma_async_from_sync_iterator_object_t *iter_p = (ecma_async_from_sync_iterator_object_t *) this_obj_p;

  ecma_value_t builtin_promise = ecma_make_object_value (context_p, ecma_builtin_get (context_p, ECMA_BUILTIN_ID_PROMISE));
  ecma_object_t *capability_p = ecma_promise_new_capability (context_p, builtin_promise, ECMA_VALUE_UNDEFINED);
  JJS_ASSERT (capability_p != NULL);

  ecma_value_t result;
  ecma_value_t arg = (arguments_number == 0 ? ECMA_VALUE_EMPTY : arguments_list_p[0]);

  switch (builtin_routine_id)
  {
    case ECMA_ASYNC_FROM_SYNC_ITERATOR_PROTOTYPE_ROUTINE_NEXT:
    {
      result = ecma_builtin_async_from_sync_iterator_prototype_next (context_p, iter_p, capability_p, arg);
      break;
    }
    case ECMA_ASYNC_FROM_SYNC_ITERATOR_PROTOTYPE_ROUTINE_RETURN:
    {
      result = ecma_builtin_async_from_sync_iterator_prototype_do (context_p, iter_p, capability_p, arg, LIT_MAGIC_STRING_RETURN);
      break;
    }
    case ECMA_ASYNC_FROM_SYNC_ITERATOR_PROTOTYPE_ROUTINE_THROW:
    {
      result = ecma_builtin_async_from_sync_iterator_prototype_do (context_p, iter_p, capability_p, arg, LIT_MAGIC_STRING_THROW);
      break;
    }
    default:
    {
      JJS_UNREACHABLE ();
      break;
    }
  }

  ecma_deref_object (capability_p);

  return result;
} /* ecma_builtin_async_from_sync_iterator_prototype_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */
