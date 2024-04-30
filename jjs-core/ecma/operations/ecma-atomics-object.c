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

#include "ecma-arraybuffer-object.h"
#include "ecma-bigint.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-shared-arraybuffer-object.h"
#include "ecma-typedarray-object.h"

#include "jcontext.h"
#include "jmem.h"

#if JJS_BUILTIN_ATOMICS

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaatomicsobject ECMA builtin Atomics helper functions
 * @{
 */

/**
 * Atomics validate Shared integer typedArray
 *
 * See also: ES11 24.4.1.1
 *
 * @return ecma value
 */
ecma_value_t
ecma_validate_shared_integer_typedarray (ecma_context_t *context_p, /**< JJS context */
                                         ecma_value_t typedarray, /**< typedArray argument */
                                         bool waitable) /**< waitable argument */
{
  /* 2. */
  if (!ecma_is_typedarray (context_p, typedarray))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_THIS_NOT_TYPED_ARRAY);
  }

  /* 3-4. */
  ecma_object_t *typedarray_p = ecma_get_object_from_value (context_p, typedarray);
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (context_p, typedarray_p);

  /* 5-6. */
  if (waitable)
  {
    if (!(target_info.id == ECMA_BIGINT64_ARRAY || target_info.id == ECMA_INT32_ARRAY))
    {
      return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_NOT_SUPPORTED);
    }
  }
  else
  {
    if (target_info.id == ECMA_UINT8_CLAMPED_ARRAY || target_info.id == ECMA_FLOAT32_ARRAY
        || target_info.id == ECMA_FLOAT64_ARRAY)
    {
      return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_NOT_SUPPORTED);
    }
  }

  /* 7. */
  JJS_ASSERT (target_info.array_buffer_p != NULL);

  /* 8-10. */
  ecma_object_t *buffer = ecma_typedarray_get_arraybuffer (context_p, typedarray_p);

  if (!ecma_object_class_is (buffer, ECMA_OBJECT_CLASS_SHARED_ARRAY_BUFFER))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_NOT_SHARED_ARRAY_BUFFER);
  }

  return ecma_make_object_value (context_p, buffer);
} /* ecma_validate_shared_integer_typedarray */

/**
 * Atomics validate Atomic Access
 *
 * See also: ES11 24.4.1.2
 *
 * @return ecma value
 */
ecma_value_t
ecma_validate_atomic_access (ecma_context_t *context_p, /**< JJS context */
                             ecma_value_t typedarray, /**< typedArray argument */
                             ecma_value_t request_index) /**< request_index argument */
{
  /* 1. */
  JJS_ASSERT (ecma_is_value_object (typedarray)
                && ecma_typedarray_get_arraybuffer (context_p, ecma_get_object_from_value (context_p, typedarray)) != NULL);

  ecma_object_t *typedarray_p = ecma_get_object_from_value (context_p, typedarray);

  /* 2. */
  ecma_number_t access_index;
  if (ECMA_IS_VALUE_ERROR (ecma_op_to_index (context_p, request_index, &access_index)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 3. */
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (context_p, typedarray_p);

  /* 4. */
  JJS_ASSERT (access_index >= 0);

  /* 5-6. */
  if (JJS_UNLIKELY (access_index >= target_info.length))
  {
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_LENGTH);
  }

  return ecma_make_number_value (context_p, access_index);
} /* ecma_validate_atomic_access */

/**
 * Atomics read, modify, write
 *
 * See also: ES11 24.4.1.11
 *
 * @return ecma value
 */
ecma_value_t
ecma_atomic_read_modify_write (ecma_context_t *context_p, /**< JJS context */
                               ecma_value_t typedarray, /**< typedArray argument */
                               ecma_value_t index, /**< index argument */
                               ecma_value_t value, /**< value argument */
                               ecma_atomics_op_t op) /**< operation argument */
{
  /* 1. */
  ecma_value_t buffer = ecma_validate_shared_integer_typedarray (context_p, typedarray, false);

  if (ECMA_IS_VALUE_ERROR (buffer))
  {
    return buffer;
  }

  /* 2. */
  ecma_value_t idx = ecma_validate_atomic_access (context_p, typedarray, index);

  if (ECMA_IS_VALUE_ERROR (idx))
  {
    return idx;
  }

  /* 3. */
  ecma_object_t *typedarray_p = ecma_get_object_from_value (context_p, typedarray);
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (context_p, typedarray_p);

  /* 4-5. */
  ecma_value_t val = ECMA_VALUE_ERROR;
  ecma_number_t tmp;
  if (target_info.id == ECMA_BIGINT64_ARRAY || target_info.id == ECMA_BIGUINT64_ARRAY)
  {
    val = ecma_bigint_to_bigint (context_p, value, true);
  }
  else if (!ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, value, &tmp)))
  {
    val = ecma_make_number_value (context_p, tmp);
  }

  if (ECMA_IS_VALUE_ERROR (val))
  {
    return val;
  }

  /* 6. */
  uint8_t element_size = target_info.element_size;

  /* 7. */
  ecma_typedarray_type_t element_type = target_info.id;

  /* 8. */
  uint32_t offset = target_info.offset;

  /* 9. */
  uint32_t indexed_position = ecma_number_to_uint32 (idx) * element_size + offset;

  ecma_free_value (context_p, idx);

  JJS_UNUSED (indexed_position);
  JJS_UNUSED (element_type);
  JJS_UNUSED (val);
  JJS_UNUSED (buffer);
  JJS_UNUSED (op);

  ecma_free_value (context_p, val);

  /* 10. */
  return ecma_make_uint32_value (context_p, 0);
} /* ecma_atomic_read_modify_write */

/**
 * Atomics load
 *
 * See also: ES11 24.4.1.12
 *
 * @return ecma value
 */
ecma_value_t
ecma_atomic_load (ecma_context_t *context_p, /**< JJS context */
                  ecma_value_t typedarray, /**< typedArray argument */
                  ecma_value_t index) /**< index argument */
{
  ecma_value_t buffer = ecma_validate_shared_integer_typedarray (context_p, typedarray, false);

  if (ECMA_IS_VALUE_ERROR (buffer))
  {
    return buffer;
  }

  /* 2. */
  ecma_value_t idx = ecma_validate_atomic_access (context_p, typedarray, index);

  if (ECMA_IS_VALUE_ERROR (idx))
  {
    return idx;
  }

  /* 3. */
  ecma_object_t *typedarray_p = ecma_get_object_from_value (context_p, typedarray);
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (context_p, typedarray_p);

  /* 4. */
  uint8_t element_size = target_info.element_size;

  /* 5. */
  ecma_typedarray_type_t element_type = target_info.id;

  /* 6. */
  uint32_t offset = target_info.offset;

  /* 7. */
  uint32_t indexed_position = ecma_number_to_uint32 (idx) * element_size + offset;

  JJS_UNUSED (indexed_position);
  JJS_UNUSED (element_type);
  JJS_UNUSED (buffer);

  /* 8. */
  return ecma_make_uint32_value (context_p, 0);
} /* ecma_atomic_load */

/**
 * @}
 * @}
 */
#endif /* JJS_BUILTIN_ATOMICS */
