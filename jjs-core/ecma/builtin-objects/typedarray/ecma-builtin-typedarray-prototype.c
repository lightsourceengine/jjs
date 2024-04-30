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

#include <math.h>

#include "ecma-arraybuffer-object.h"
#include "ecma-bigint.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtin-typedarray-helpers.h"
#include "ecma-builtins.h"
#include "ecma-comparison.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-number-object.h"
#include "ecma-objects.h"
#include "ecma-typedarray-object.h"

#include "jcontext.h"
#include "jmem.h"
#include "jrt-libc-includes.h"
#include "jrt.h"
#include "lit-char-helpers.h"

#if JJS_BUILTIN_TYPEDARRAY

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
  /* These routines must be in this order */
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_START = 0,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_MAP,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_REDUCE,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_REDUCE_RIGHT,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_EVERY,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_SOME,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FOR_EACH,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FILTER,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FIND,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FIND_INDEX,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FIND_LAST,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FIND_LAST_INDEX,

  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_INDEX_OF,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_AT,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_LAST_INDEX_OF,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_INCLUDES,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FILL,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_SORT,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_REVERSE,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_COPY_WITHIN,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_SLICE,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_SUBARRAY,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_TO_LOCALE_STRING,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_JOIN,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_KEYS,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_ENTRIES,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_TO_REVERSED,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_TO_SORTED,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_WITH,

  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_BUFFER_GETTER,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_BYTELENGTH_GETTER,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_BYTEOFFSET_GETTER,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_LENGTH_GETTER,

  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_SET,
  ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_TO_STRING_TAG_GETTER,
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-typedarray-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID  typedarray_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup typedarrayprototype ECMA %TypedArray%.prototype object built-in
 * @{
 */

/**
 * Type of routine.
 */
typedef enum
{
  TYPEDARRAY_ROUTINE_EVERY, /**< routine: every ES2015, 22.2.3.7 */
  TYPEDARRAY_ROUTINE_SOME, /**< routine: some ES2015, 22.2.3.9 */
  TYPEDARRAY_ROUTINE_FOREACH, /**< routine: forEach ES2015, 15.4.4.18 */
  TYPEDARRAY_ROUTINE__COUNT /**< count of the modes */
} typedarray_routine_mode;

/**
 * The common function for 'every', 'some' and 'forEach'
 * because they have a similar structure.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_exec_routine (ecma_context_t *context_p, /**< JJS context */
                                                ecma_value_t this_arg, /**< this argument */
                                                ecma_typedarray_info_t *info_p, /**< object info */
                                                ecma_value_t cb_func_val, /**< callback function */
                                                ecma_value_t cb_this_arg, /**< 'this' of the callback function */
                                                typedarray_routine_mode mode) /**< mode: which routine */
{
  JJS_ASSERT (mode < TYPEDARRAY_ROUTINE__COUNT);

  ecma_object_t *typedarray_p = ecma_get_object_from_value (context_p, this_arg);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (context_p, typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, info.array_buffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, info.array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  ecma_typedarray_getter_fn_t typedarray_getter_cb = ecma_get_typedarray_getter_fn (info_p->id);
  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, cb_func_val);
  uint8_t *buffer_p = ecma_arraybuffer_get_buffer (context_p, info_p->array_buffer_p) + info_p->offset;
  uint32_t byte_pos = 0;
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  for (uint32_t index = 0; index < info_p->length && ecma_is_value_empty (ret_value); index++)
  {
    ecma_value_t current_index = ecma_make_uint32_value (context_p, index);
    ecma_value_t element = typedarray_getter_cb (context_p, buffer_p + byte_pos);

    ecma_value_t call_args[] = { element, current_index, this_arg };

    ecma_value_t call_value = ecma_op_function_call (context_p, func_object_p, cb_this_arg, call_args, 3);

    ecma_fast_free_value (context_p, current_index);
    ecma_fast_free_value (context_p, element);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      return call_value;
    }

    if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
    {
      ecma_free_value (context_p, call_value);
      return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
    }

    bool to_bool_result = ecma_op_to_boolean (context_p, call_value);
    ecma_free_value (context_p, call_value);

    if (mode == TYPEDARRAY_ROUTINE_EVERY)
    {
      if (!to_bool_result)
      {
        return ECMA_VALUE_FALSE;
      }
    }
    else if (mode == TYPEDARRAY_ROUTINE_SOME && to_bool_result)
    {
      return ECMA_VALUE_TRUE;
    }

    byte_pos += info_p->element_size;
  }

  if (mode == TYPEDARRAY_ROUTINE_EVERY)
  {
    ret_value = ECMA_VALUE_TRUE;
  }
  else if (mode == TYPEDARRAY_ROUTINE_SOME)
  {
    ret_value = ECMA_VALUE_FALSE;
  }
  else
  {
    ret_value = ECMA_VALUE_UNDEFINED;
  }

  return ret_value;
} /* ecma_builtin_typedarray_prototype_exec_routine */

/**
 * The %TypedArray%.prototype object's 'map' routine
 *
 * See also:
 *          ES2015, 22.2.3.8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_map (ecma_context_t *context_p, /**< JJS context */
                                       ecma_value_t this_arg, /**< this object */
                                       ecma_typedarray_info_t *src_info_p, /**< object info */
                                       ecma_value_t cb_func_val, /**< callback function */
                                       ecma_value_t cb_this_arg) /**< this' of the callback function */
{
  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, cb_func_val);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, src_info_p->array_buffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, src_info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  // TODO: 22.2.3.18, 7-8.
  ecma_value_t len = ecma_make_number_value (context_p, src_info_p->length);
  ecma_value_t new_typedarray = ecma_typedarray_species_create (context_p, this_arg, &len, 1);
  ecma_free_value (context_p, len);

  if (ECMA_IS_VALUE_ERROR (new_typedarray))
  {
    return new_typedarray;
  }

  ecma_object_t *target_obj_p = ecma_get_object_from_value (context_p, new_typedarray);

  uint8_t *src_buffer_p = ecma_typedarray_get_buffer (context_p, src_info_p);

  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (context_p, target_obj_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, target_info.array_buffer_p))
  {
    ecma_deref_object (target_obj_p);
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, target_info.array_buffer_p))
  {
    ecma_deref_object (target_obj_p);
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  uint8_t *target_buffer_p = ecma_typedarray_get_buffer (context_p, &target_info);

  ecma_typedarray_getter_fn_t src_typedarray_getter_cb = ecma_get_typedarray_getter_fn (src_info_p->id);
  ecma_typedarray_setter_fn_t target_typedarray_setter_cb = ecma_get_typedarray_setter_fn (target_info.id);

  for (uint32_t index = 0; index < src_info_p->length; index++)
  {
    ecma_value_t current_index = ecma_make_uint32_value (context_p, index);
    ecma_value_t element = src_typedarray_getter_cb (context_p, src_buffer_p);
    src_buffer_p += src_info_p->element_size;

    ecma_value_t call_args[] = { element, current_index, this_arg };
    ecma_value_t mapped_value = ecma_op_function_call (context_p, func_object_p, cb_this_arg, call_args, 3);

    ecma_free_value (context_p, current_index);
    ecma_free_value (context_p, element);

    if (ECMA_IS_VALUE_ERROR (mapped_value))
    {
      ecma_free_value (context_p, new_typedarray);
      return mapped_value;
    }

    if (ecma_arraybuffer_is_detached (context_p, src_info_p->array_buffer_p))
    {
      ecma_free_value (context_p, mapped_value);
      ecma_free_value (context_p, new_typedarray);
      return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
    }

    ecma_value_t set_element = target_typedarray_setter_cb (context_p, target_buffer_p, mapped_value);
    target_buffer_p += target_info.element_size;
    ecma_free_value (context_p, mapped_value);

    if (ECMA_IS_VALUE_ERROR (set_element))
    {
      ecma_free_value (context_p, new_typedarray);
      return set_element;
    }
  }

  return new_typedarray;
} /* ecma_builtin_typedarray_prototype_map */

/**
 * Reduce and reduceRight routines share a similar structure.
 * And we use 'is_right' to distinguish between them.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_reduce_with_direction (ecma_context_t *context_p, /**< JJS context */
                                                         ecma_value_t this_arg, /**< this object */
                                                         ecma_typedarray_info_t *info_p, /**< object info */
                                                         const ecma_value_t arguments_list_p[], /**arg_list*/
                                                         uint32_t arguments_number, /**< length of arguments' list*/
                                                         bool is_right) /**< choose order, true is reduceRight */
{
  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  ecma_typedarray_getter_fn_t getter_cb = ecma_get_typedarray_getter_fn (info_p->id);
  uint32_t byte_pos;

  if (info_p->length == 0)
  {
    if (arguments_number < 2)
    {
      return ecma_raise_type_error (context_p, ECMA_ERR_INITIAL_VALUE_CANNOT_BE_UNDEFINED);
    }

    return ecma_copy_value (context_p, arguments_list_p[1]);
  }

  JJS_ASSERT (info_p->length > 0);

  ecma_value_t accumulator = ECMA_VALUE_UNDEFINED;
  uint32_t index = is_right ? (info_p->length - 1) : 0;
  uint8_t *buffer_p = ecma_arraybuffer_get_buffer (context_p, info_p->array_buffer_p) + info_p->offset;

  if (ecma_is_value_undefined (arguments_list_p[1]))
  {
    byte_pos = index << info_p->shift;
    accumulator = getter_cb (context_p, buffer_p + byte_pos);

    if (is_right)
    {
      if (index == 0)
      {
        return accumulator;
      }

      index--;
    }
    else
    {
      index++;

      if (index == info_p->length)
      {
        return accumulator;
      }
    }
  }
  else
  {
    accumulator = ecma_copy_value (context_p, arguments_list_p[1]);
  }

  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, arguments_list_p[0]);

  while (true)
  {
    ecma_value_t current_index = ecma_make_uint32_value (context_p, index);
    byte_pos = index << info_p->shift;
    ecma_value_t get_value = getter_cb (context_p, buffer_p + byte_pos);

    ecma_value_t call_args[] = { accumulator, get_value, current_index, this_arg };

    JJS_ASSERT (ecma_is_value_number (get_value) || ecma_is_value_bigint (get_value));

    ecma_value_t call_value = ecma_op_function_call (context_p, func_object_p, ECMA_VALUE_UNDEFINED, call_args, 4);

    ecma_fast_free_value (context_p, accumulator);
    ecma_fast_free_value (context_p, get_value);
    ecma_fast_free_value (context_p, current_index);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      return call_value;
    }

    if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
    {
      ecma_free_value (context_p, call_value);
      return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
    }

    accumulator = call_value;

    if (is_right)
    {
      if (index == 0)
      {
        break;
      }

      index--;
    }
    else
    {
      index++;

      if (index == info_p->length)
      {
        break;
      }
    }
  }

  return accumulator;
} /* ecma_builtin_typedarray_prototype_reduce_with_direction */

/**
 * The %TypedArray%.prototype object's 'filter' routine
 *
 * See also:
 *          ES2015, 22.2.3.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_filter (ecma_context_t *context_p, /**< JJS context */
                                          ecma_value_t this_arg, /**< this object */
                                          ecma_typedarray_info_t *info_p, /**< object info */
                                          ecma_value_t cb_func_val, /**< callback function */
                                          ecma_value_t cb_this_arg) /**< 'this' of the callback function */
{
  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  ecma_typedarray_getter_fn_t getter_cb = ecma_get_typedarray_getter_fn (info_p->id);

  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, cb_func_val);
  ecma_value_t ret_value = ECMA_VALUE_ERROR;

  // TODO: 22.2.3.9, 7-8.
  if (info_p->length == 0)
  {
    return ecma_op_create_typedarray_with_type_and_length (context_p, info_p->id, 0);
  }

  ecma_collection_t *collected_p = ecma_new_collection (context_p);
  uint8_t *buffer_p = ecma_arraybuffer_get_buffer (context_p, info_p->array_buffer_p) + info_p->offset;

  for (uint32_t index = 0; index < info_p->length; index++)
  {
    ecma_value_t current_index = ecma_make_uint32_value (context_p, index);
    ecma_value_t get_value = getter_cb (context_p, buffer_p);

    JJS_ASSERT (ecma_is_value_number (get_value) || ecma_is_value_bigint (get_value));

    ecma_value_t call_args[] = { get_value, current_index, this_arg };

    ecma_value_t call_value = ecma_op_function_call (context_p, func_object_p, cb_this_arg, call_args, 3);

    ecma_fast_free_value (context_p, current_index);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      ecma_fast_free_value (context_p, get_value);
      goto cleanup;
    }

    if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
    {
      ecma_free_value (context_p, call_value);
      ecma_fast_free_value (context_p, get_value);
      ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
      goto cleanup;
    }

    if (ecma_op_to_boolean (context_p, call_value))
    {
      ecma_collection_push_back (context_p, collected_p, get_value);
    }
    else
    {
      ecma_fast_free_value (context_p, get_value);
    }

    buffer_p += info_p->element_size;
    ecma_fast_free_value (context_p, call_value);
  }

  ecma_value_t collected = ecma_make_number_value (context_p, collected_p->item_count);
  ret_value = ecma_typedarray_species_create (context_p, this_arg, &collected, 1);
  ecma_free_value (context_p, collected);

  if (!ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_object_t *new_typedarray_p = ecma_get_object_from_value (context_p, ret_value);
    ecma_typedarray_info_t target_info = ecma_typedarray_get_info (context_p, new_typedarray_p);

    JJS_ASSERT (target_info.offset == 0);

    uint8_t *target_buffer_p = ecma_typedarray_get_buffer (context_p, &target_info);

    ecma_typedarray_setter_fn_t target_typedarray_setter_cb = ecma_get_typedarray_setter_fn (target_info.id);

    for (uint32_t idx = 0; idx < collected_p->item_count; idx++)
    {
      ecma_value_t set_element = target_typedarray_setter_cb (context_p, target_buffer_p, collected_p->buffer_p[idx]);

      if (ECMA_IS_VALUE_ERROR (set_element))
      {
        ecma_deref_object (new_typedarray_p);
        ret_value = ECMA_VALUE_ERROR;
        goto cleanup;
      }

      target_buffer_p += target_info.element_size;
    }
  }

cleanup:
  ecma_collection_free (context_p, collected_p);

  return ret_value;
} /* ecma_builtin_typedarray_prototype_filter */

/**
 * The %TypedArray%.prototype object's 'reverse' routine
 *
 * See also:
 *          ES2015, 22.2.3.21
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_reverse (ecma_context_t *context_p, /**< JJS context */
                                           ecma_value_t this_arg, /**< this argument */
                                           ecma_typedarray_info_t *info_p) /**< object info */
{
  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  uint8_t *buffer_p = ecma_arraybuffer_get_buffer (context_p, info_p->array_buffer_p) + info_p->offset;
  uint32_t middle = (info_p->length / 2) << info_p->shift;
  uint32_t buffer_last = (info_p->length << info_p->shift) - info_p->element_size;

  for (uint32_t lower = 0; lower < middle; lower += info_p->element_size)
  {
    uint32_t upper = buffer_last - lower;
    uint8_t *lower_p = buffer_p + lower;
    uint8_t *upper_p = buffer_p + upper;

    uint8_t tmp[8];
    memcpy (&tmp[0], lower_p, info_p->element_size);
    memcpy (lower_p, upper_p, info_p->element_size);
    memcpy (upper_p, &tmp[0], info_p->element_size);
  }

  return ecma_copy_value (context_p, this_arg);
} /* ecma_builtin_typedarray_prototype_reverse */

/**
 * The %TypedArray%.prototype object's 'set' routine for a typedArray source
 *
 * See also:
 *          ES2015, 22.2.3.22, 22.2.3.22.2
 *
 * @return ecma value of undefined if success, error otherwise.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_op_typedarray_set_with_typedarray (ecma_context_t *context_p, /**< JJS context */
                                        ecma_value_t this_arg, /**< this argument */
                                        ecma_value_t arr_val, /**< typedarray object */
                                        ecma_value_t offset_val) /**< offset value */
{
  /* 6.~ 8. targetOffset */
  ecma_number_t target_offset_num;
  if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, offset_val, &target_offset_num)))
  {
    return ECMA_VALUE_ERROR;
  }

  if (target_offset_num <= -1.0 || target_offset_num >= (ecma_number_t) UINT32_MAX + 0.5)
  {
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_OFFSET);
  }

  ecma_object_t *target_typedarray_p = ecma_get_object_from_value (context_p, this_arg);
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (context_p, target_typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, target_info.array_buffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, target_info.array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  uint8_t *target_buffer_p = ecma_typedarray_get_buffer (context_p, &target_info);

  ecma_object_t *src_typedarray_p = ecma_get_object_from_value (context_p, arr_val);
  ecma_typedarray_info_t src_info = ecma_typedarray_get_info (context_p, src_typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, src_info.array_buffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, src_info.array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  uint8_t *src_buffer_p = ecma_typedarray_get_buffer (context_p, &src_info);

  uint32_t target_offset_uint32 = ecma_number_to_uint32 (target_offset_num);

  if ((int64_t) src_info.length + target_offset_uint32 > target_info.length)
  {
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_RANGE_OF_INDEX);
  }

  /* Fast path first. If the source and target arrays are the same we do not need to copy anything. */
  if (this_arg == arr_val)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  /* 26. targetByteIndex */
  target_buffer_p += target_offset_uint32 << target_info.shift;

  /* 27. limit */
  uint32_t limit = src_info.length << target_info.shift;

  if (src_info.id == target_info.id)
  {
    memmove (target_buffer_p, src_buffer_p, limit);
  }
  else
  {
    uint8_t *target_limit_p = target_buffer_p + limit;
    ecma_typedarray_getter_fn_t src_typedarray_getter_cb = ecma_get_typedarray_getter_fn (src_info.id);
    ecma_typedarray_setter_fn_t target_typedarray_setter_cb = ecma_get_typedarray_setter_fn (target_info.id);

    while (target_buffer_p < target_limit_p)
    {
      ecma_value_t element = src_typedarray_getter_cb (context_p, src_buffer_p);
      ecma_value_t set_element = target_typedarray_setter_cb (context_p, target_buffer_p, element);
      ecma_free_value (context_p, element);

      if (ECMA_IS_VALUE_ERROR (set_element))
      {
        return set_element;
      }

      src_buffer_p += src_info.element_size;
      target_buffer_p += target_info.element_size;
    }
  }

  return ECMA_VALUE_UNDEFINED;
} /* ecma_op_typedarray_set_with_typedarray */

/**
 * The %TypedArray%.prototype object's 'set' routine
 *
 * See also:
 *          ES2015, 22.2.3.22, 22.2.3.22.1
 *
 * @return ecma value of undefined if success, error otherwise.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_set (ecma_context_t *context_p, /**< JJS context */
                                       ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t arr_val, /**< array object */
                                       ecma_value_t offset_val) /**< offset value */
{
  /* 1. */
  if (ecma_is_typedarray (context_p, arr_val))
  {
    /* 22.2.3.22.2 */
    return ecma_op_typedarray_set_with_typedarray (context_p, this_arg, arr_val, offset_val);
  }

  /* 6.~ 8. targetOffset */
  ecma_number_t target_offset_num;

  if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, offset_val, &target_offset_num)))
  {
    return ECMA_VALUE_ERROR;
  }

  if (target_offset_num <= -1.0 || target_offset_num >= (ecma_number_t) UINT32_MAX + 0.5)
  {
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_OFFSET);
  }
  uint32_t target_offset_uint32 = ecma_number_to_uint32 (target_offset_num);

  /* 11. ~ 15. */
  ecma_object_t *typedarray_p = ecma_get_object_from_value (context_p, this_arg);
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (context_p, typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, target_info.array_buffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, target_info.array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  uint8_t *target_buffer_p = ecma_typedarray_get_buffer (context_p, &target_info);

  /* 16.~ 17. */
  ecma_value_t source_obj = ecma_op_to_object (context_p, arr_val);

  if (ECMA_IS_VALUE_ERROR (source_obj))
  {
    return source_obj;
  }

  /* 18.~ 19. */
  ecma_object_t *source_obj_p = ecma_get_object_from_value (context_p, source_obj);

  ecma_length_t source_length;

  if (ECMA_IS_VALUE_ERROR (ecma_op_object_get_length (context_p, source_obj_p, &source_length)))
  {
    ecma_deref_object (source_obj_p);
    return ECMA_VALUE_ERROR;
  }

  /* 20. if srcLength + targetOffset > targetLength, throw a RangeError */
  if ((int64_t) source_length + target_offset_uint32 > target_info.length)
  {
    ecma_deref_object (source_obj_p);
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_RANGE_OF_INDEX);
  }
  JJS_ASSERT (source_length <= UINT32_MAX);
  uint32_t source_length_uint32 = (uint32_t) source_length;

  /* 21.~ 25. */
  target_buffer_p += target_offset_uint32 << target_info.shift;

  ecma_typedarray_setter_fn_t target_typedarray_setter_cb = ecma_get_typedarray_setter_fn (target_info.id);
  uint32_t k = 0;

  while (k < source_length_uint32)
  {
    ecma_value_t elem = ecma_op_object_get_by_index (context_p, source_obj_p, k);

    if (ECMA_IS_VALUE_ERROR (elem))
    {
      ecma_deref_object (source_obj_p);
      return elem;
    }

    ecma_value_t value_to_set;

#if JJS_BUILTIN_BIGINT
    if (ECMA_TYPEDARRAY_IS_BIGINT_TYPE (target_info.id))
    {
      value_to_set = ecma_bigint_to_bigint (context_p, elem, false);

      if (ECMA_IS_VALUE_ERROR (value_to_set))
      {
        ecma_deref_object (source_obj_p);
        ecma_free_value (context_p, elem);
        return value_to_set;
      }
    }
    else
#endif /* JJS_BUILTIN_BIGINT */
    {
      ecma_number_t elem_num;
      if (ECMA_IS_VALUE_ERROR (ecma_op_to_numeric (context_p, elem, &elem_num, ECMA_TO_NUMERIC_NO_OPTS)))
      {
        ecma_free_value (context_p, elem);
        ecma_deref_object (source_obj_p);
        return ECMA_VALUE_ERROR;
      }

      value_to_set = ecma_make_number_value (context_p, elem_num);
    }

    ecma_free_value (context_p, elem);

    if (ecma_arraybuffer_is_detached (context_p, target_info.array_buffer_p))
    {
      ecma_deref_object (source_obj_p);
      ecma_free_value (context_p, value_to_set);
      return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
    }

    ecma_value_t set_element = target_typedarray_setter_cb (context_p, target_buffer_p, value_to_set);

    ecma_free_value (context_p, value_to_set);

    if (ECMA_IS_VALUE_ERROR (set_element))
    {
      ecma_deref_object (source_obj_p);
      return set_element;
    }

    k++;
    target_buffer_p += target_info.element_size;
  }

  ecma_deref_object (source_obj_p);

  return ECMA_VALUE_UNDEFINED;
} /* ecma_builtin_typedarray_prototype_set */

/**
 * TypedArray.prototype's 'toString' single element operation routine based
 * on the Array.prototype's 'toString' single element operation routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2
 *
 * @return NULL - if the converison fails
 *         ecma_string_t * - otherwise
 */
static ecma_string_t *
ecma_op_typedarray_get_to_string_at_index (ecma_context_t *context_p, /**< JJS context */
                                           ecma_object_t *obj_p, /**< this object */
                                           uint32_t index) /**< array index */
{
  ecma_value_t index_value = ecma_op_object_get_by_index (context_p, obj_p, index);

  if (ECMA_IS_VALUE_ERROR (index_value))
  {
    return NULL;
  }

  if (ecma_is_value_undefined (index_value) || ecma_is_value_null (index_value))
  {
    ecma_free_value (context_p, index_value);
    return ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_string_t *ret_str_p = ecma_op_to_string (context_p, index_value);

  ecma_free_value (context_p, index_value);

  return ret_str_p;
} /* ecma_op_typedarray_get_to_string_at_index */

/**
 * The TypedArray.prototype.toString's separator creation routine based on
 * the Array.prototype.toString's separator routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2 4th step
 *
 * @return NULL - if the conversion fails
 *         ecma_string_t * - otherwise
 */
static ecma_string_t *
ecma_op_typedarray_get_separator_string (ecma_context_t *context_p, /**< JJS context */
                                         ecma_value_t separator) /**< possible separator */
{
  if (ecma_is_value_undefined (separator))
  {
    return ecma_get_magic_string (LIT_MAGIC_STRING_COMMA_CHAR);
  }

  return ecma_op_to_string (context_p, separator);
} /* ecma_op_typedarray_get_separator_string */

/**
 * The TypedArray.prototype object's 'join' routine basen on
 * the Array.porottype object's 'join'
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_join (ecma_context_t *context_p, /**< JJS context */
                                        ecma_object_t *obj_p, /**< this object */
                                        ecma_value_t separator_arg) /**< separator argument */
{
  ecma_typedarray_info_t info = ecma_typedarray_get_info (context_p, obj_p);

  if (ecma_arraybuffer_is_detached (context_p, info.array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  /* 2. */
  uint32_t length = ecma_typedarray_get_length (context_p, obj_p);
  ecma_string_t *separator_string_p = ecma_op_typedarray_get_separator_string (context_p, separator_arg);

  ecma_value_t ret_value = ECMA_VALUE_ERROR;
  if (JJS_UNLIKELY (separator_string_p == NULL))
  {
    return ret_value;
  }

  /* 7-8. */
  ecma_string_t *first_string_p = ecma_op_typedarray_get_to_string_at_index (context_p, obj_p, 0);

  if (JJS_UNLIKELY (first_string_p == NULL))
  {
    ecma_deref_ecma_string (context_p, separator_string_p);
    return ret_value;
  }

  ecma_stringbuilder_t builder = ecma_stringbuilder_create_from (context_p, first_string_p);

  ecma_deref_ecma_string (context_p, first_string_p);

  /* 9-10. */
  for (uint32_t k = 1; k < length; k++)
  {
    /* 10.a */
    ecma_stringbuilder_append (&builder, separator_string_p);

    /* 10.d */
    ecma_string_t *next_string_p = ecma_op_typedarray_get_to_string_at_index (context_p, obj_p, k);

    if (JJS_UNLIKELY (next_string_p == NULL))
    {
      ecma_stringbuilder_destroy (&builder);
      ecma_deref_ecma_string (context_p, separator_string_p);
      return ret_value;
    }

    ecma_stringbuilder_append (&builder, next_string_p);

    ecma_deref_ecma_string (context_p, next_string_p);
  }

  ecma_deref_ecma_string (context_p, separator_string_p);
  ret_value = ecma_make_string_value (context_p, ecma_stringbuilder_finalize (&builder));

  return ret_value;
} /* ecma_builtin_typedarray_prototype_join */

/**
 * The %TypedArray%.prototype object's 'subarray' routine.
 *
 * See also:
 *          ES2015, 22.2.3.26
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_subarray (ecma_context_t *context_p, /**< JJS context */
                                            ecma_value_t this_arg, /**< this object */
                                            ecma_typedarray_info_t *info_p, /**< object info */
                                            ecma_value_t begin, /**< begin */
                                            ecma_value_t end) /**< end */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 9. beginIndex, 12. endIndex */
  uint32_t begin_index_uint32 = 0, end_index_uint32 = 0;

  /* 7. relativeBegin */
  if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, begin, info_p->length, &begin_index_uint32)))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_is_value_undefined (end))
  {
    end_index_uint32 = (uint32_t) info_p->length;
  }
  else
  {
    /* 10. relativeEnd */
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, end, info_p->length, &end_index_uint32)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  /* 13. newLength */
  uint32_t subarray_length = 0;

  if (end_index_uint32 > begin_index_uint32)
  {
    subarray_length = end_index_uint32 - begin_index_uint32;
  }

  /* 17. beginByteOffset */
  uint32_t begin_byte_offset = info_p->offset + (begin_index_uint32 << info_p->shift);

  ecma_value_t arguments_p[3] = { ecma_make_object_value (context_p, info_p->array_buffer_p),
                                  ecma_make_uint32_value (context_p, begin_byte_offset),
                                  ecma_make_uint32_value (context_p, subarray_length) };

  ret_value = ecma_typedarray_species_create (context_p, this_arg, arguments_p, 3);

  ecma_free_value (context_p, arguments_p[1]);
  ecma_free_value (context_p, arguments_p[2]);
  return ret_value;
} /* ecma_builtin_typedarray_prototype_subarray */

/**
 * The %TypedArray%.prototype object's 'fill' routine.
 *
 * See also:
 *          ES2015, 22.2.3.8, 22.1.3.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_fill (ecma_context_t *context_p, /**< JJS context */
                                        ecma_value_t this_arg, /**< this object */
                                        ecma_typedarray_info_t *info_p, /**< object info */
                                        ecma_value_t value, /**< value */
                                        ecma_value_t begin, /**< begin */
                                        ecma_value_t end) /**< end */
{
  ecma_value_t value_to_set;

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

#if JJS_BUILTIN_BIGINT
  if (ECMA_TYPEDARRAY_IS_BIGINT_TYPE (info_p->id))
  {
    value_to_set = ecma_bigint_to_bigint (context_p, value, true);

    if (ECMA_IS_VALUE_ERROR (value_to_set))
    {
      return value_to_set;
    }
  }
  else
#endif /* JJS_BUILTIN_BIGINT */
  {
    ecma_number_t value_num;
    ecma_value_t ret_value = ecma_op_to_numeric (context_p, value, &value_num, ECMA_TO_NUMERIC_NO_OPTS);

    if (!ecma_is_value_empty (ret_value))
    {
      return ret_value;
    }

    value_to_set = ecma_make_number_value (context_p, value_num);
  }

  uint32_t begin_index_uint32 = 0, end_index_uint32 = 0;

  if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, begin, info_p->length, &begin_index_uint32)))
  {
    ecma_free_value (context_p, value_to_set);
    return ECMA_VALUE_ERROR;
  }

  if (ecma_is_value_undefined (end))
  {
    end_index_uint32 = (uint32_t) info_p->length;
  }
  else
  {
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, end, info_p->length, &end_index_uint32)))
    {
      ecma_free_value (context_p, value_to_set);
      return ECMA_VALUE_ERROR;
    }
  }

  uint32_t subarray_length = 0;

  if (end_index_uint32 > begin_index_uint32)
  {
    subarray_length = end_index_uint32 - begin_index_uint32;
  }

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  uint8_t *buffer_p = ecma_typedarray_get_buffer (context_p, info_p);

  buffer_p += begin_index_uint32 << info_p->shift;

  uint8_t *limit_p = buffer_p + (subarray_length << info_p->shift);
  ecma_typedarray_setter_fn_t typedarray_setter_cb = ecma_get_typedarray_setter_fn (info_p->id);

  while (buffer_p < limit_p)
  {
    ecma_value_t set_element = typedarray_setter_cb (context_p, buffer_p, value_to_set);

    if (ECMA_IS_VALUE_ERROR (set_element))
    {
      ecma_free_value (context_p, value_to_set);
      return set_element;
    }

    buffer_p += info_p->element_size;
  }

  ecma_free_value (context_p, value_to_set);

  return ecma_copy_value (context_p, this_arg);
} /* ecma_builtin_typedarray_prototype_fill */

/**
 * SortCompare abstract method
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_sort_compare_helper (ecma_context_t *context_p, /**< JJS context */
                                                       ecma_value_t lhs, /**< left value */
                                                       ecma_value_t rhs, /**< right value */
                                                       ecma_value_t compare_func, /**< compare function */
                                                       ecma_object_t *array_buffer_p) /**< array buffer */
{
  if (ecma_is_value_undefined (compare_func))
  {
    /* Default comparison when no comparefn is passed. */
#if JJS_BUILTIN_BIGINT
    if (ecma_is_value_bigint (lhs) && ecma_is_value_bigint (rhs))
    {
      return ecma_make_number_value (context_p, ecma_bigint_compare_to_bigint (context_p, lhs, rhs));
    }
#endif /* JJS_BUILTIN_BIGINT */

    ecma_number_t result = ECMA_NUMBER_ZERO;

    double lhs_value = (double) ecma_get_number_from_value (context_p, lhs);
    double rhs_value = (double) ecma_get_number_from_value (context_p, rhs);

    if (ecma_number_is_nan (lhs_value))
    {
      // Keep NaNs at the end of the array.
      result = ECMA_NUMBER_ONE;
    }
    else if (ecma_number_is_nan (rhs_value))
    {
      // Keep NaNs at the end of the array.
      result = ECMA_NUMBER_MINUS_ONE;
    }
    else if (lhs_value < rhs_value)
    {
      result = ECMA_NUMBER_MINUS_ONE;
    }
    else if (lhs_value > rhs_value || (ecma_number_is_zero (rhs_value) && ecma_number_is_negative (rhs_value)))
    {
      result = ECMA_NUMBER_ONE;
    }
    else
    {
      result = ECMA_NUMBER_ZERO;
    }

    return ecma_make_number_value (context_p, result);
  }

  /*
   * compare_func, if not undefined, will always contain a callable function object.
   * We checked this previously, before this function was called.
   */
  JJS_ASSERT (ecma_op_is_callable (context_p, compare_func));
  ecma_object_t *comparefn_obj_p = ecma_get_object_from_value (context_p, compare_func);

  ecma_value_t compare_args[] = { lhs, rhs };

  ecma_value_t call_value = ecma_op_function_call (context_p, comparefn_obj_p, ECMA_VALUE_UNDEFINED, compare_args, 2);

  if (ECMA_IS_VALUE_ERROR (call_value) || ecma_is_value_number (call_value))
  {
    return call_value;
  }

  ecma_number_t ret_num;
  ecma_value_t number_result = ecma_op_to_number (context_p, call_value, &ret_num);

  ecma_free_value (context_p, call_value);

  if (ECMA_IS_VALUE_ERROR (number_result))
  {
    return number_result;
  }

  if (ecma_arraybuffer_is_detached (context_p, array_buffer_p))
  {
    ecma_free_value (context_p, number_result);
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  // If the coerced value can't be represented as a Number, compare them as equals.
  if (ecma_number_is_nan (ret_num))
  {
    return ecma_make_number_value (context_p, ECMA_NUMBER_ZERO);
  }

  return ecma_make_number_value (context_p, ret_num);
} /* ecma_builtin_typedarray_prototype_sort_compare_helper */

/**
 * The %TypedArray%.prototype object's 'sort' routine.
 *
 * See also:
 *          ES2015, 22.2.3.25, 22.1.3.24
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_sort (ecma_context_t *context_p, /**< JJS context */
                                        ecma_value_t this_arg, /**< this argument */
                                        ecma_typedarray_info_t *info_p, /**< object info */
                                        ecma_value_t compare_func) /**< comparator fn */
{
  JJS_ASSERT (ecma_is_typedarray (context_p, this_arg));
  JJS_ASSERT (ecma_is_value_undefined (compare_func) || ecma_op_is_callable (context_p, compare_func));

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  if (!info_p->length)
  {
    return ecma_copy_value (context_p, this_arg);
  }

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  JMEM_DEFINE_LOCAL_ARRAY (context_p, values_buffer, info_p->length, ecma_value_t);

  uint32_t buffer_index = 0;

  ecma_typedarray_getter_fn_t typedarray_getter_cb = ecma_get_typedarray_getter_fn (info_p->id);
  uint8_t *buffer_p = ecma_arraybuffer_get_buffer (context_p, info_p->array_buffer_p) + info_p->offset;
  uint8_t *limit_p = buffer_p + (info_p->length << info_p->shift);

  /* Copy unsorted array into a native c array. */
  while (buffer_p < limit_p)
  {
    JJS_ASSERT (buffer_index < info_p->length);
    ecma_value_t element_value = typedarray_getter_cb (context_p, buffer_p);
    values_buffer[buffer_index++] = element_value;
    buffer_p += info_p->element_size;
  }

  JJS_ASSERT (buffer_index == info_p->length);

  const ecma_builtin_helper_sort_compare_fn_t sort_cb = &ecma_builtin_typedarray_prototype_sort_compare_helper;

  ecma_value_t sort_value = ecma_builtin_helper_array_merge_sort_helper (context_p,
                                                                         values_buffer,
                                                                         (uint32_t) (info_p->length),
                                                                         compare_func,
                                                                         sort_cb,
                                                                         info_p->array_buffer_p);

  if (ECMA_IS_VALUE_ERROR (sort_value))
  {
    ret_value = sort_value;
    goto free_values;
  }

  JJS_ASSERT (sort_value == ECMA_VALUE_EMPTY);

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  ecma_typedarray_setter_fn_t typedarray_setter_cb = ecma_get_typedarray_setter_fn (info_p->id);

  buffer_p = limit_p - (info_p->length << info_p->shift);
  buffer_index = 0;

  /* Put sorted values from the native array back into the typedarray buffer. */
  while (buffer_p < limit_p)
  {
    JJS_ASSERT (buffer_index < info_p->length);
    ecma_value_t element_value = values_buffer[buffer_index++];
    ecma_value_t set_element = typedarray_setter_cb (context_p, buffer_p, element_value);

    if (ECMA_IS_VALUE_ERROR (set_element))
    {
      ret_value = set_element;
      goto free_values;
    }

    buffer_p += info_p->element_size;
  }

  JJS_ASSERT (buffer_index == info_p->length);

  ret_value = ecma_copy_value (context_p, this_arg);

free_values:
  /* Free values that were copied to the local array. */
  for (uint32_t index = 0; index < info_p->length; index++)
  {
    ecma_free_value (context_p, values_buffer[index]);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (context_p, values_buffer);

  return ret_value;
} /* ecma_builtin_typedarray_prototype_sort */

/**
 * The %TypedArray%.prototype object's 'find' and 'findIndex' routine helper
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_find_helper (ecma_context_t *context_p, /**< JJS context */
                                               ecma_value_t this_arg, /**< this argument */
                                               ecma_typedarray_info_t *info_p, /**< object info */
                                               ecma_value_t predicate, /**< callback function */
                                               ecma_value_t predicate_this_arg, /**< this argument for
                                                                                 *   invoke predicate */
                                               bool is_find) /**< true - find routine
                                                              *   false - findIndex routine */
{
  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  JJS_ASSERT (ecma_is_value_object (predicate));
  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, predicate);
  uint8_t *buffer_p = ecma_arraybuffer_get_buffer (context_p, info_p->array_buffer_p) + info_p->offset;
  uint8_t *limit_p = buffer_p + (info_p->length << info_p->shift);
  ecma_typedarray_getter_fn_t typedarray_getter_cb = ecma_get_typedarray_getter_fn (info_p->id);
  uint32_t buffer_index = 0;

  while (buffer_p < limit_p)
  {
    JJS_ASSERT (buffer_index < info_p->length);
    ecma_value_t element_value = typedarray_getter_cb (context_p, buffer_p);
    buffer_p += info_p->element_size;

    ecma_value_t call_args[] = { element_value, ecma_make_uint32_value (context_p, buffer_index), this_arg };
    ecma_value_t call_value = ecma_op_function_call (context_p, func_object_p, predicate_this_arg, call_args, 3);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      ecma_free_value (context_p, element_value);
      return call_value;
    }

    if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
    {
      ecma_free_value (context_p, element_value);
      ecma_free_value (context_p, call_value);
      return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
    }

    bool call_result = ecma_op_to_boolean (context_p, call_value);
    ecma_free_value (context_p, call_value);

    if (call_result)
    {
      if (is_find)
      {
        return element_value;
      }

      ecma_free_value (context_p, element_value);
      return ecma_make_uint32_value (context_p, buffer_index);
    }

    buffer_index++;
    ecma_free_value (context_p, element_value);
  }

  return is_find ? ECMA_VALUE_UNDEFINED : ecma_make_integer_value (-1);
} /* ecma_builtin_typedarray_prototype_find_helper */

/**
 * The %TypedArray%.prototype object's 'findLast' and 'findLastIndex' routine helper
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_find_last_helper (ecma_context_t *context_p, /**< JJS context */
                                                    ecma_value_t this_arg, /**< this argument */
                                                    ecma_typedarray_info_t *info_p, /**< object info */
                                                    ecma_value_t predicate, /**< callback function */
                                                    ecma_value_t predicate_this_arg, /**< this argument for
                                                                                      *   invoke predicate */
                                                    bool is_find_last) /**< true - findLast routine
                                                                        *   false - findLastIndex routine */
{
  if (!ecma_op_is_callable (context_p, predicate))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_CALLBACK_IS_NOT_CALLABLE);
  }

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  if (info_p->length == 0)
  {
    return is_find_last ? ECMA_VALUE_UNDEFINED : ecma_make_integer_value (-1);
  }

  JJS_ASSERT (ecma_is_value_object (predicate));
  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, predicate);
  uint8_t *buffer_p = ecma_arraybuffer_get_buffer (context_p, info_p->array_buffer_p) + info_p->offset;
  ecma_typedarray_getter_fn_t typedarray_getter_cb = ecma_get_typedarray_getter_fn (info_p->id);

  for (uint32_t buffer_index = info_p->length; buffer_index-- > 0;)
  {
    ecma_value_t element_value = typedarray_getter_cb (context_p, buffer_p + (buffer_index * info_p->element_size));

    ecma_value_t call_args[] = { element_value, ecma_make_uint32_value (context_p, buffer_index), this_arg };
    ecma_value_t call_value = ecma_op_function_call (context_p, func_object_p, predicate_this_arg, call_args, 3);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      ecma_free_value (context_p, element_value);
      return call_value;
    }

    if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
    {
      ecma_free_value (context_p, element_value);
      ecma_free_value (context_p, call_value);
      return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
    }

    bool call_result = ecma_op_to_boolean (context_p, call_value);
    ecma_free_value (context_p, call_value);

    if (call_result)
    {
      if (is_find_last)
      {
        return element_value;
      }

      ecma_free_value (context_p, element_value);
      return ecma_make_uint32_value (context_p, buffer_index);
    }

    ecma_free_value (context_p, element_value);
  }

  return is_find_last ? ECMA_VALUE_UNDEFINED : ecma_make_integer_value (-1);
} /* ecma_builtin_typedarray_prototype_find_helper */

/**
 * The %TypedArray%.prototype object's 'at' routine
 *
 * See also:
 *          ECMA-262 Stage 3 Draft Relative Indexing Method proposal
 *          from: https://tc39.es/proposal-relative-indexing-method
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_at (ecma_context_t *context_p, /**< JJS context */
                                      ecma_typedarray_info_t *info_p, /**< object info */
                                      const ecma_value_t index) /**< index argument */
{
  ecma_length_t len = info_p->length;
  ecma_length_t res_index;
  ecma_value_t return_value = ecma_builtin_helper_calculate_index (context_p, index, len, &res_index);

  if (return_value != ECMA_VALUE_EMPTY)
  {
    return return_value;
  }

  if (res_index >= UINT32_MAX)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  return ecma_get_typedarray_element (context_p, info_p, (uint32_t) res_index);
} /* ecma_builtin_typedarray_prototype_at */

/**
 * The %TypedArray%.prototype object's 'indexOf' routine
 *
 * See also:
 *         ECMA-262 v6, 22.2.3.13
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_index_of (ecma_context_t *context_p, /**< JJS context */
                                            ecma_typedarray_info_t *info_p, /**< object info */
                                            const ecma_value_t args[], /**< arguments list */
                                            uint32_t args_number) /**< number of arguments */
{
  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

#if JJS_BUILTIN_BIGINT
  bool is_bigint = ECMA_TYPEDARRAY_IS_BIGINT_TYPE (info_p->id);
#else /* !JJS_BUILTIN_BIGINT */
  bool is_bigint = false;
#endif /* JJS_BUILTIN_BIGINT */

  uint32_t from_index;

  /* 5. */
  if (args_number == 0 || (!ecma_is_value_number (args[0]) && !is_bigint) || info_p->length == 0)
  {
    return ecma_make_integer_value (-1);
  }
  if (args_number == 1)
  {
    from_index = 0;
  }
  else
  {
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, args[1], info_p->length, &from_index)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  uint8_t *buffer_p = ecma_typedarray_get_buffer (context_p, info_p);

  uint8_t *limit_p = buffer_p + (info_p->length << info_p->shift);
  ecma_typedarray_getter_fn_t getter_cb = ecma_get_typedarray_getter_fn (info_p->id);

  buffer_p += from_index << info_p->shift;

  /* 11. */
  while (buffer_p < limit_p)
  {
    ecma_value_t element = getter_cb (context_p, buffer_p);

    if (ecma_op_same_value_zero (context_p, args[0], element, true))
    {
      ecma_free_value (context_p, element);
      return ecma_make_number_value (context_p, from_index);
    }

    ecma_free_value (context_p, element);
    buffer_p += info_p->element_size;
    from_index++;
  }

  /* 12. */
  return ecma_make_integer_value (-1);
} /* ecma_builtin_typedarray_prototype_index_of */

/**
 * The %TypedArray%.prototype object's 'lastIndexOf' routine
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.16
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_last_index_of (ecma_context_t *context_p, /**< JJS context */
                                                 ecma_typedarray_info_t *info_p, /**< object info */
                                                 const ecma_value_t args[], /**< arguments list */
                                                 uint32_t args_number) /**< number of arguments */
{
  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

#if JJS_BUILTIN_BIGINT
  bool is_bigint = ECMA_TYPEDARRAY_IS_BIGINT_TYPE (info_p->id);
#else /* !JJS_BUILTIN_BIGINT */
  bool is_bigint = false;
#endif /* JJS_BUILTIN_BIGINT */

  uint32_t from_index;

  /* 5. */
  if (args_number == 0 || (!ecma_is_value_number (args[0]) && !is_bigint) || info_p->length == 0)
  {
    return ecma_make_integer_value (-1);
  }

  if (args_number == 1)
  {
    from_index = info_p->length - 1;
  }
  else
  {
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, args[1], info_p->length, &from_index)))
    {
      return ECMA_VALUE_ERROR;
    }

    ecma_number_t to_int;

    if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, args[1], &to_int)))
    {
      return ECMA_VALUE_ERROR;
    }

    if (info_p->length + to_int < 0)
    {
      return ecma_make_integer_value (-1);
    }

    from_index = JJS_MIN (from_index, info_p->length - 1);
  }

  ecma_typedarray_getter_fn_t getter_cb = ecma_get_typedarray_getter_fn (info_p->id);
  uint8_t *buffer_p = ecma_arraybuffer_get_buffer (context_p, info_p->array_buffer_p) + info_p->offset;
  uint8_t *current_element_p = buffer_p + (from_index << info_p->shift);

  /* 10. */
  while (current_element_p >= buffer_p)
  {
    ecma_value_t element = getter_cb (context_p, current_element_p);

    if (ecma_op_same_value_zero (context_p, args[0], element, true))
    {
      ecma_free_value (context_p, element);
      return ecma_make_number_value (context_p, (ecma_number_t) from_index);
    }

    ecma_free_value (context_p, element);
    current_element_p -= info_p->element_size;
    from_index--;
  }

  /* 11. */
  return ecma_make_integer_value (-1);
} /* ecma_builtin_typedarray_prototype_last_index_of */

/**
 * The %TypedArray%.prototype object's 'copyWithin' routine
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_copy_within (ecma_context_t *context_p, /**< JJS context */
                                               ecma_value_t this_arg, /**< this argument */
                                               ecma_typedarray_info_t *info_p, /**< object info */
                                               const ecma_value_t args[], /**< arguments list */
                                               uint32_t args_number) /**< number of arguments */
{
  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  uint32_t relative_target = 0;
  uint32_t relative_start = 0;
  uint32_t relative_end = info_p->length;

  if (args_number > 0)
  {
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, args[0], info_p->length, &relative_target)))
    {
      return ECMA_VALUE_ERROR;
    }

    if (args_number > 1)
    {
      if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, args[1], info_p->length, &relative_start)))
      {
        return ECMA_VALUE_ERROR;
      }

      if (args_number > 2 && args[2] != ECMA_VALUE_UNDEFINED)
      {
        if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, args[2], info_p->length, &relative_end)))
        {
          return ECMA_VALUE_ERROR;
        }
      }
    }
  }

  if (relative_target >= info_p->length || relative_start >= relative_end || relative_end == 0)
  {
    return ecma_copy_value (context_p, this_arg);
  }

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  uint8_t *buffer_p = ecma_typedarray_get_buffer (context_p, info_p);

  uint32_t distance = relative_end - relative_start;
  uint32_t offset = info_p->length - relative_target;
  uint32_t count = JJS_MIN (distance, offset);

  memmove (buffer_p + (relative_target << info_p->shift),
           buffer_p + (relative_start << info_p->shift),
           (size_t) (count << info_p->shift));

  return ecma_copy_value (context_p, this_arg);
} /* ecma_builtin_typedarray_prototype_copy_within */

/**
 * The %TypedArray%.prototype object's 'slice' routine
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.23
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_slice (ecma_context_t *context_p, /**< JJS context */
                                         ecma_value_t this_arg, /**< this argument */
                                         ecma_typedarray_info_t *info_p, /**< object info */
                                         const ecma_value_t args[], /**< arguments list */
                                         uint32_t args_number) /**< number of arguments */
{
  uint32_t relative_start = 0;
  uint32_t relative_end = info_p->length;

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  if (args_number > 0)
  {
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, args[0], info_p->length, &relative_start)))
    {
      return ECMA_VALUE_ERROR;
    }

    if (args_number > 1 && args[1] != ECMA_VALUE_UNDEFINED
        && ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, args[1], info_p->length, &relative_end)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  uint8_t *src_buffer_p = ecma_typedarray_get_buffer (context_p, info_p);

  int32_t distance = (int32_t) (relative_end - relative_start);
  uint32_t count = distance > 0 ? (uint32_t) distance : 0;

  ecma_value_t len = ecma_make_number_value (context_p, count);
  // TODO: 22.2.3.23, 12-13.
  ecma_value_t new_typedarray = ecma_typedarray_species_create (context_p, this_arg, &len, 1);
  ecma_free_value (context_p, len);

  if (ECMA_IS_VALUE_ERROR (new_typedarray) || count == 0)
  {
    return new_typedarray;
  }

  ecma_object_t *new_typedarray_p = ecma_get_object_from_value (context_p, new_typedarray);
  ecma_typedarray_info_t new_typedarray_info = ecma_typedarray_get_info (context_p, new_typedarray_p);

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    ecma_deref_object (new_typedarray_p);
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  uint8_t *dst_buffer_p = ecma_typedarray_get_buffer (context_p, &new_typedarray_info);

  JJS_ASSERT (new_typedarray_info.offset == 0);

  src_buffer_p += relative_start << info_p->shift;

  if (info_p->id == new_typedarray_info.id)
  {
    // 22.2.3.23. Step 22. h-i.
    memcpy (dst_buffer_p, src_buffer_p, count << info_p->shift);
  }
  else
  {
    // 22.2.3.23. Step 21. b.
    ecma_typedarray_getter_fn_t src_typedarray_getter_cb = ecma_get_typedarray_getter_fn (info_p->id);
    ecma_typedarray_setter_fn_t new_typedarray_setter_cb = ecma_get_typedarray_setter_fn (new_typedarray_info.id);

    for (uint32_t idx = 0; idx < count; idx++)
    {
      ecma_value_t element = src_typedarray_getter_cb (context_p, src_buffer_p);
      ecma_value_t set_element = new_typedarray_setter_cb (context_p, dst_buffer_p, element);
      ecma_free_value (context_p, element);

      if (ECMA_IS_VALUE_ERROR (set_element))
      {
        ecma_deref_object (new_typedarray_p);
        return set_element;
      }

      src_buffer_p += info_p->element_size;
      dst_buffer_p += new_typedarray_info.element_size;
    }
  }

  return new_typedarray;
} /* ecma_builtin_typedarray_prototype_slice */

/**
 * The TypedArray.prototype's 'toLocaleString' single element operation routine.
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.26 steps 7-10 and 12.b-e
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_to_locale_string_helper (ecma_context_t *context_p, /**< JJS context */
                                                           ecma_typedarray_info_t *info_p, /**< object info */
                                                           uint32_t index) /** array index */
{
  ecma_value_t element_value = ecma_get_typedarray_element (context_p, info_p, index);

  if (ECMA_IS_VALUE_ERROR (element_value))
  {
    return element_value;
  }

  ecma_value_t call_value = ecma_op_invoke_by_magic_id (context_p, element_value, LIT_MAGIC_STRING_TO_LOCALE_STRING_UL, NULL, 0);

  ecma_free_value (context_p, element_value);

  if (ECMA_IS_VALUE_ERROR (call_value))
  {
    return call_value;
  }

  ecma_string_t *str_p = ecma_op_to_string (context_p, call_value);

  ecma_free_value (context_p, call_value);

  if (JJS_UNLIKELY (str_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  return ecma_make_string_value (context_p, str_p);
} /* ecma_builtin_typedarray_prototype_to_locale_string_helper */

/**
 * The %TypedArray%.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.27
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_to_locale_string (ecma_context_t *context_p, /**< JJS context */
                                                    ecma_typedarray_info_t *info_p) /**< object info */
{
  if (info_p->length == 0)
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_value_t first_element = ecma_builtin_typedarray_prototype_to_locale_string_helper (context_p, info_p, 0);

  if (ECMA_IS_VALUE_ERROR (first_element))
  {
    return first_element;
  }

  ecma_string_t *return_string_p = ecma_get_string_from_value (context_p, first_element);
  ecma_stringbuilder_t builder = ecma_stringbuilder_create_from (context_p, return_string_p);
  ecma_deref_ecma_string (context_p, return_string_p);

  for (uint32_t k = 1; k < info_p->length; k++)
  {
    ecma_stringbuilder_append_byte (&builder, LIT_CHAR_COMMA);
    ecma_value_t next_element = ecma_builtin_typedarray_prototype_to_locale_string_helper (context_p, info_p, k);

    if (ECMA_IS_VALUE_ERROR (next_element))
    {
      ecma_stringbuilder_destroy (&builder);
      return next_element;
    }

    ecma_string_t *next_element_p = ecma_get_string_from_value (context_p, next_element);
    ecma_stringbuilder_append (&builder, next_element_p);
    ecma_deref_ecma_string (context_p, next_element_p);
  }

  return ecma_make_string_value (context_p, ecma_stringbuilder_finalize (&builder));
} /* ecma_builtin_typedarray_prototype_to_locale_string */

/**
 * The %TypedArray%.prototype object's 'includes' routine
 *
 * See also:
 *          ECMA-262 v11, 22.2.3.13.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_includes (ecma_context_t *context_p, /**< JJS context */
                                            ecma_typedarray_info_t *info_p, /**< object info */
                                            const ecma_value_t args[], /**< arguments list */
                                            uint32_t args_number) /**< number of arguments */
{
#if JJS_BUILTIN_BIGINT
  bool is_bigint = ECMA_TYPEDARRAY_IS_BIGINT_TYPE (info_p->id);
#else /* !JJS_BUILTIN_BIGINT */
  bool is_bigint = false;
#endif /* JJS_BUILTIN_BIGINT */

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  if (args_number == 0 || (!ecma_is_value_number (args[0]) && !is_bigint) || info_p->length == 0)
  {
    return ECMA_VALUE_FALSE;
  }

  uint32_t from_index = 0;

  if (args_number > 1)
  {
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, args[1], info_p->length, &from_index)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  uint8_t *buffer_p = ecma_typedarray_get_buffer (context_p, info_p);

  ecma_typedarray_getter_fn_t getter_cb = ecma_get_typedarray_getter_fn (info_p->id);
  uint8_t *limit_p = buffer_p + (info_p->length << info_p->shift);

  buffer_p += from_index << info_p->shift;

  while (buffer_p < limit_p)
  {
    ecma_value_t element = getter_cb (context_p, buffer_p);

    if (ecma_op_same_value_zero (context_p, args[0], element, false))
    {
      ecma_free_value (context_p, element);
      return ECMA_VALUE_TRUE;
    }

    ecma_free_value (context_p, element);
    buffer_p += info_p->element_size;
  }

  return ECMA_VALUE_FALSE;
} /* ecma_builtin_typedarray_prototype_includes */

/**
 * The %TypedArray%.prototype object's 'with' routine
 *
 * See also:
 *          ECMA-262 v14, 23.2.3.36.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_with (ecma_context_t *context_p, /**< JJS context */
                                        ecma_value_t this_arg, /**< this argument */
                                        const ecma_value_t args[], /**< arguments list */
                                        uint32_t args_number, /**< argument count */
                                        ecma_typedarray_info_t *info_p) /**< object info */
{
  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  ecma_value_t len = ecma_make_number_value (context_p, info_p->length);
  ecma_value_t new_typedarray = ecma_op_typedarray_create_same_type (context_p, this_arg, &len, 1);
  ecma_free_value (context_p, len);

  ecma_object_t *new_typedarray_p = ecma_get_object_from_value (context_p, new_typedarray);
  ecma_typedarray_info_t new_typedarray_info = ecma_typedarray_get_info (context_p, new_typedarray_p);

  JJS_ASSERT (info_p->length == new_typedarray_info.length);

  ecma_typedarray_getter_fn_t src_typedarray_getter_cb = ecma_get_typedarray_getter_fn (info_p->id);
  ecma_typedarray_setter_fn_t new_typedarray_setter_cb = ecma_get_typedarray_setter_fn (new_typedarray_info.id);
  uint8_t *src_buffer_p = ecma_typedarray_get_buffer (context_p, info_p);
  uint8_t *dst_buffer_p = ecma_typedarray_get_buffer (context_p, &new_typedarray_info);

  ecma_number_t relative_index = ECMA_NUMBER_ZERO;

  ecma_value_t tioi_result = ecma_op_to_integer_or_infinity (context_p, args_number > 0 ? args[0] : ECMA_VALUE_UNDEFINED,
                                                             &relative_index);

  if(ECMA_IS_VALUE_ERROR (tioi_result))
  {
    return tioi_result;
  }

  ecma_free_value (context_p, tioi_result);

  ecma_number_t len_n = (ecma_number_t)info_p->length;
  ecma_number_t actual_index_n = (relative_index >= 0) ? relative_index : len_n + relative_index;

  if (actual_index_n >= len_n || actual_index_n < 0)
  {
    ecma_free_value (context_p, new_typedarray);
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_RANGE_OF_INDEX);
  }

  ecma_value_t value;
  bool free_value = false;

  if (args_number > 1)
  {
#if JJS_BUILTIN_BIGINT
    if (ecma_is_value_undefined (args[1]))
    {
      value = ECMA_BIGINT_ZERO;
    }
    else if (ECMA_TYPEDARRAY_IS_BIGINT_TYPE (info_p->id))
    {
      value = ecma_bigint_to_bigint (context_p, args[1], true);

      if (ECMA_IS_VALUE_ERROR (value))
      {
        return value;
      }

      free_value = true;
    }
    else
#endif /* JJS_BUILTIN_BIGINT */
    {
      value = args[1];
    }
  }
  else
  {
#if JJS_BUILTIN_BIGINT
    if (ECMA_TYPEDARRAY_IS_BIGINT_TYPE (info_p->id))
    {
      value = ECMA_BIGINT_ZERO;
    }
    else
#endif /* JJS_BUILTIN_BIGINT */
    {
      value = ECMA_VALUE_UNDEFINED;
    }
  }

  ecma_length_t k = 0;
  ecma_length_t actual_index = (ecma_length_t)actual_index_n;
  uint8_t element_size = info_p->element_size;
  ecma_value_t set_element;

  while (k < info_p->length)
  {
    if (k == actual_index)
    {
      set_element = new_typedarray_setter_cb (context_p, dst_buffer_p, value);
    }
    else
    {
      ecma_value_t element = src_typedarray_getter_cb (context_p, src_buffer_p);
      set_element = new_typedarray_setter_cb (context_p, dst_buffer_p, element);
      ecma_free_value (context_p, element);
    }

    if (ECMA_IS_VALUE_ERROR (set_element))
    {
      if (free_value)
      {
        ecma_free_value (context_p, value);
      }

      ecma_free_value (context_p, new_typedarray);
      return set_element;
    }

    ecma_free_value (context_p, set_element);
    src_buffer_p += element_size;
    dst_buffer_p += element_size;
    k++;
  }

  if (free_value)
  {
    ecma_free_value (context_p, value);
  }

  return new_typedarray;
} /* ecma_builtin_typedarray_prototype_with */

/**
 * The %TypedArray%.prototype object's 'toReversed' routine
 *
 * See also:
 *          ECMA-262 v14, 23.2.3.32.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_to_reversed (ecma_context_t *context_p, /**< JJS context */
                                               ecma_value_t this_arg, /**< this argument */
                                               ecma_typedarray_info_t *info_p) /**< object info */
{
  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  ecma_value_t len = ecma_make_number_value (context_p, info_p->length);
  ecma_value_t new_typedarray = ecma_op_typedarray_create_same_type (context_p, this_arg, &len, 1);
  ecma_free_value (context_p, len);

  if (ECMA_IS_VALUE_ERROR (new_typedarray) || info_p->length == 0)
  {
    return new_typedarray;
  }

  ecma_object_t *new_typedarray_p = ecma_get_object_from_value (context_p, new_typedarray);
  ecma_typedarray_info_t new_typedarray_info = ecma_typedarray_get_info (context_p, new_typedarray_p);

  JJS_ASSERT (info_p->length == new_typedarray_info.length);

  ecma_typedarray_getter_fn_t src_typedarray_getter_cb = ecma_get_typedarray_getter_fn (info_p->id);
  ecma_typedarray_setter_fn_t new_typedarray_setter_cb = ecma_get_typedarray_setter_fn (new_typedarray_info.id);
  uint8_t *src_buffer_p = ecma_typedarray_get_buffer (context_p, info_p) + ((info_p->length - 1) * info_p->element_size);
  uint8_t *dst_buffer_p = ecma_typedarray_get_buffer (context_p, &new_typedarray_info);

  for (uint32_t k = 0; k < info_p->length; k++)
  {
    ecma_value_t element = src_typedarray_getter_cb (context_p, src_buffer_p);
    ecma_value_t set_element = new_typedarray_setter_cb (context_p, dst_buffer_p, element);
    ecma_free_value (context_p, element);

    if (ECMA_IS_VALUE_ERROR (set_element))
    {
      ecma_free_value (context_p, new_typedarray);
      return set_element;
    }

    ecma_free_value (context_p, set_element);
    src_buffer_p -= info_p->element_size;
    dst_buffer_p += new_typedarray_info.element_size;
  }

  return new_typedarray;
} /* ecma_builtin_typedarray_prototype_to_reversed */

static ecma_value_t
ecma_builtin_typedarray_prototype_to_sorted (ecma_context_t *context_p, /**< JJS context */
                                             ecma_value_t this_arg, /**< this argument */
                                             const ecma_value_t args[], /**< arguments list */
                                             uint32_t args_number, /**< argument count */
                                             ecma_typedarray_info_t *info_p) /**< object info */
{
  JJS_ASSERT (ecma_is_typedarray (context_p, this_arg));

  ecma_value_t compare_fn = args_number > 0 ? args[0] : ECMA_VALUE_UNDEFINED;

  if (!ecma_is_value_undefined (compare_fn) && !ecma_op_is_callable (context_p, compare_fn))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_COMPARE_FUNC_NOT_CALLABLE);
  }

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  ecma_value_t len = ecma_make_number_value (context_p, info_p->length);
  ecma_value_t new_typedarray = ecma_op_typedarray_create_same_type (context_p, this_arg, &len, 1);
  ecma_free_value (context_p, len);

  if (ECMA_IS_VALUE_ERROR (new_typedarray) || info_p->length == 0)
  {
    return new_typedarray;
  }

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  JMEM_DEFINE_LOCAL_ARRAY (context_p, values_buffer, info_p->length, ecma_value_t);

  uint32_t buffer_index = 0;
  ecma_typedarray_getter_fn_t typedarray_getter_cb = ecma_get_typedarray_getter_fn (info_p->id);
  uint8_t *buffer_p = ecma_arraybuffer_get_buffer (context_p, info_p->array_buffer_p) + info_p->offset;
  uint8_t *limit_p = buffer_p + (info_p->length << info_p->shift);

  /* Copy unsorted array into a native c array. */
  while (buffer_p < limit_p)
  {
    JJS_ASSERT (buffer_index < info_p->length);
    ecma_value_t element_value = typedarray_getter_cb (context_p, buffer_p);
    values_buffer[buffer_index++] = element_value;
    buffer_p += info_p->element_size;
  }

  JJS_ASSERT (buffer_index == info_p->length);

  const ecma_builtin_helper_sort_compare_fn_t sort_cb = &ecma_builtin_typedarray_prototype_sort_compare_helper;

  ecma_value_t sort_value = ecma_builtin_helper_array_merge_sort_helper (context_p,
                                                                         values_buffer,
                                                                         (uint32_t) (info_p->length),
                                                                         compare_fn,
                                                                         sort_cb,
                                                                         info_p->array_buffer_p);

  if (ECMA_IS_VALUE_ERROR (sort_value))
  {
    ret_value = sort_value;
    goto free_values;
  }

  JJS_ASSERT (sort_value == ECMA_VALUE_EMPTY);

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  ecma_object_t *new_typedarray_p = ecma_get_object_from_value (context_p, new_typedarray);
  ecma_typedarray_info_t new_typedarray_info = ecma_typedarray_get_info (context_p, new_typedarray_p);
  ecma_typedarray_setter_fn_t new_typedarray_setter_cb = ecma_get_typedarray_setter_fn (new_typedarray_info.id);

  buffer_p = ecma_arraybuffer_get_buffer (context_p, new_typedarray_info.array_buffer_p) + new_typedarray_info.offset;
  limit_p = buffer_p + (new_typedarray_info.length << new_typedarray_info.shift);
  buffer_index = 0;

  /* Put sorted values from the native array back into the typedarray buffer. */
  while (buffer_p < limit_p)
  {
    JJS_ASSERT (buffer_index < new_typedarray_info.length);
    ecma_value_t element_value = values_buffer[buffer_index++];
    ecma_value_t set_element = new_typedarray_setter_cb (context_p, buffer_p, element_value);

    if (ECMA_IS_VALUE_ERROR (set_element))
    {
      ret_value = set_element;
      goto free_values;
    }

    buffer_p += new_typedarray_info.element_size;
  }

  JJS_ASSERT (buffer_index == new_typedarray_info.length);

free_values:
  /* Free values that were copied to the local array. */
  for (uint32_t index = 0; index < info_p->length; index++)
  {
    ecma_free_value (context_p, values_buffer[index]);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (context_p, values_buffer);

  if (ECMA_IS_VALUE_ERROR(ret_value))
  {
    ecma_free_value (context_p, new_typedarray);
    return ret_value;
  }

  JJS_ASSERT(ret_value == ECMA_VALUE_EMPTY);

  return new_typedarray;
} /* ecma_builtin_typedarray_prototype_to_sorted */

/**
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_typedarray_prototype_dispatch_routine (ecma_context_t *context_p, /**< JJS context */
                                                    uint8_t builtin_routine_id, /**< built-in wide
                                                                                 *   routine identifier */
                                                    ecma_value_t this_arg, /**< 'this' argument value */
                                                    const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                                            *   passed to routine */
                                                    uint32_t arguments_number) /**< length of arguments' list */
{
  if (!ecma_is_typedarray (context_p, this_arg))
  {
    if (builtin_routine_id == ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_TO_STRING_TAG_GETTER)
    {
      return ECMA_VALUE_UNDEFINED;
    }

    return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_THIS_NOT_TYPED_ARRAY);
  }

  ecma_object_t *typedarray_p = ecma_get_object_from_value (context_p, this_arg);
  ecma_typedarray_info_t info = { 0 };

  if (builtin_routine_id < ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_BUFFER_GETTER)
  {
    info = ecma_typedarray_get_info (context_p, typedarray_p);

    if (builtin_routine_id != ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_SUBARRAY
        && ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, info.array_buffer_p))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  if (builtin_routine_id < ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_INDEX_OF && !ecma_op_is_callable (context_p, arguments_list_p[0]))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_CALLBACK_IS_NOT_CALLABLE);
  }

  switch (builtin_routine_id)
  {
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_INCLUDES:
    {
      return ecma_builtin_typedarray_prototype_includes (context_p, &info, arguments_list_p, arguments_number);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_JOIN:
    {
      return ecma_builtin_typedarray_prototype_join (context_p, typedarray_p, arguments_list_p[0]);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_EVERY:
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_SOME:
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FOR_EACH:
    {
      uint8_t offset = (uint8_t) (builtin_routine_id - ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_EVERY);

      return ecma_builtin_typedarray_prototype_exec_routine (context_p,
                                                             this_arg,
                                                             &info,
                                                             arguments_list_p[0],
                                                             arguments_list_p[1],
                                                             offset);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_MAP:
    {
      return ecma_builtin_typedarray_prototype_map (context_p, this_arg, &info, arguments_list_p[0], arguments_list_p[1]);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_REDUCE:
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_REDUCE_RIGHT:
    {
      bool is_reduce = builtin_routine_id == ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_REDUCE_RIGHT;
      return ecma_builtin_typedarray_prototype_reduce_with_direction (context_p,
                                                                      this_arg,
                                                                      &info,
                                                                      arguments_list_p,
                                                                      arguments_number,
                                                                      is_reduce);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FILTER:
    {
      return ecma_builtin_typedarray_prototype_filter (context_p, this_arg, &info, arguments_list_p[0], arguments_list_p[1]);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_REVERSE:
    {
      return ecma_builtin_typedarray_prototype_reverse (context_p, this_arg, &info);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_SET:
    {
      return ecma_builtin_typedarray_prototype_set (context_p, this_arg, arguments_list_p[0], arguments_list_p[1]);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_SUBARRAY:
    {
      return ecma_builtin_typedarray_prototype_subarray (context_p, this_arg, &info, arguments_list_p[0], arguments_list_p[1]);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FILL:
    {
      return ecma_builtin_typedarray_prototype_fill (context_p,
                                                     this_arg,
                                                     &info,
                                                     arguments_list_p[0],
                                                     arguments_list_p[1],
                                                     arguments_list_p[2]);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_SORT:
    {
      if (!ecma_is_value_undefined (arguments_list_p[0]) && !ecma_op_is_callable (context_p, arguments_list_p[0]))
      {
        return ecma_raise_type_error (context_p, ECMA_ERR_CALLBACK_IS_NOT_CALLABLE);
      }

      return ecma_builtin_typedarray_prototype_sort (context_p, this_arg, &info, arguments_list_p[0]);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FIND:
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FIND_INDEX:
    {
      bool is_find = builtin_routine_id == ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FIND;
      return ecma_builtin_typedarray_prototype_find_helper (context_p,
                                                            this_arg,
                                                            &info,
                                                            arguments_list_p[0],
                                                            arguments_list_p[1],
                                                            is_find);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FIND_LAST:
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FIND_LAST_INDEX:
    {
      bool is_find_last = builtin_routine_id == ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_FIND_LAST;
      return ecma_builtin_typedarray_prototype_find_last_helper (context_p,
                                                                 this_arg,
                                                                 &info,
                                                                 arguments_list_p[0],
                                                                 arguments_list_p[1],
                                                                 is_find_last);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_AT:
    {
      return ecma_builtin_typedarray_prototype_at (context_p, &info, arguments_list_p[0]);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_INDEX_OF:
    {
      return ecma_builtin_typedarray_prototype_index_of (context_p, &info, arguments_list_p, arguments_number);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_LAST_INDEX_OF:
    {
      return ecma_builtin_typedarray_prototype_last_index_of (context_p, &info, arguments_list_p, arguments_number);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_COPY_WITHIN:
    {
      return ecma_builtin_typedarray_prototype_copy_within (context_p, this_arg, &info, arguments_list_p, arguments_number);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_SLICE:
    {
      return ecma_builtin_typedarray_prototype_slice (context_p, this_arg, &info, arguments_list_p, arguments_number);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_TO_LOCALE_STRING:
    {
      return ecma_builtin_typedarray_prototype_to_locale_string (context_p, &info);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_KEYS:
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_ENTRIES:
    {
      ecma_iterator_kind_t iter_id =
        (builtin_routine_id == ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_KEYS) ? ECMA_ITERATOR_KEYS : ECMA_ITERATOR_ENTRIES;

      return ecma_typedarray_iterators_helper (context_p, this_arg, iter_id);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_BUFFER_GETTER:
    {
      ecma_object_t *buffer_p = ecma_typedarray_get_arraybuffer (context_p, typedarray_p);
      ecma_ref_object (buffer_p);

      return ecma_make_object_value (context_p, buffer_p);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_BYTELENGTH_GETTER:
    {
      ecma_object_t *buffer_p = ecma_typedarray_get_arraybuffer (context_p, typedarray_p);

      if (ecma_arraybuffer_is_detached (context_p, buffer_p))
      {
        return ecma_make_uint32_value (context_p, 0);
      }

      uint32_t length = ecma_typedarray_get_length (context_p, typedarray_p);
      uint8_t shift = ecma_typedarray_get_element_size_shift (context_p, typedarray_p);
      return ecma_make_uint32_value (context_p, length << shift);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_BYTEOFFSET_GETTER:
    {
      return ecma_make_uint32_value (context_p, ecma_typedarray_get_offset (context_p, typedarray_p));
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_LENGTH_GETTER:
    {
      ecma_object_t *buffer_p = ecma_typedarray_get_arraybuffer (context_p, typedarray_p);

      if (ecma_arraybuffer_is_detached (context_p, buffer_p))
      {
        return ecma_make_uint32_value (context_p, 0);
      }

      return ecma_make_uint32_value (context_p, ecma_typedarray_get_length (context_p, typedarray_p));
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_TO_STRING_TAG_GETTER:
    {
      ecma_extended_object_t *object_p = (ecma_extended_object_t *) typedarray_p;
      return ecma_make_magic_string_value (ecma_get_typedarray_magic_string_id (object_p->u.cls.u1.typedarray_type));
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_WITH:
    {
      return ecma_builtin_typedarray_prototype_with (context_p, this_arg, arguments_list_p, arguments_number, &info);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_TO_REVERSED:
    {
      return ecma_builtin_typedarray_prototype_to_reversed (context_p, this_arg, &info);
    }
    case ECMA_TYPEDARRAY_PROTOTYPE_ROUTINE_TO_SORTED:
    {
      return ecma_builtin_typedarray_prototype_to_sorted (context_p, this_arg, arguments_list_p, arguments_number, &info);
    }
    default:
    {
      JJS_UNREACHABLE ();
    }
  }
} /* ecma_builtin_typedarray_prototype_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* JJS_BUILTIN_TYPEDARRAY */
