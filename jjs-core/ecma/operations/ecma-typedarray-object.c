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

#include "ecma-typedarray-object.h"

#include <math.h>

#include "ecma-arraybuffer-object.h"
#include "ecma-big-uint.h"
#include "ecma-bigint.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers-number.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-objects-general.h"
#include "ecma-objects.h"
#include "ecma-shared-arraybuffer-object.h"

#include "jcontext.h"

#if JJS_BUILTIN_TYPEDARRAY

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmatypedarrayobject ECMA TypedArray object related routines
 * @{
 */

/**
 * Read and copy a number from a given buffer to a value.
 **/
#define ECMA_TYPEDARRAY_GET_ELEMENT(src_p, num, type)                      \
  do                                                                       \
  {                                                                        \
    if (JJS_LIKELY ((((uintptr_t) (src_p)) & (sizeof (type) - 1)) == 0)) \
    {                                                                      \
      num = *(type *) ((void *) src_p);                                    \
    }                                                                      \
    else                                                                   \
    {                                                                      \
      memcpy (&num, (void *) src_p, sizeof (type));                        \
    }                                                                      \
  } while (0)

/**
 * Copy a number from a value to the given buffer
 **/
#define ECMA_TYPEDARRAY_SET_ELEMENT(src_p, num, type)                      \
  do                                                                       \
  {                                                                        \
    if (JJS_LIKELY ((((uintptr_t) (src_p)) & (sizeof (type) - 1)) == 0)) \
    {                                                                      \
      *(type *) ((void *) src_p) = num;                                    \
    }                                                                      \
    else                                                                   \
    {                                                                      \
      memcpy ((void *) src_p, &num, sizeof (type));                        \
    }                                                                      \
  } while (0)

/**
 * Read an int8_t value from the given arraybuffer
 */
static ecma_value_t
ecma_typedarray_get_int8_element (ecma_context_t *context_p, /**< JJS context */
                                  lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  JJS_UNUSED (context_p);
  int8_t num = (int8_t) *src;
  return ecma_make_integer_value (num);
} /* ecma_typedarray_get_int8_element */

/**
 * Read an uint8_t value from the given arraybuffer
 */
static ecma_value_t
ecma_typedarray_get_uint8_element (ecma_context_t *context_p, /**< JJS context */
                                   lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  JJS_UNUSED (context_p);
  uint8_t num = (uint8_t) *src;
  return ecma_make_integer_value (num);
} /* ecma_typedarray_get_uint8_element */

/**
 * Read an int16_t value from the given arraybuffer
 */
static ecma_value_t
ecma_typedarray_get_int16_element (ecma_context_t *context_p, /**< JJS context */
                                   lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  JJS_UNUSED (context_p);
  int16_t num;
  ECMA_TYPEDARRAY_GET_ELEMENT (src, num, int16_t);
  return ecma_make_integer_value (num);
} /* ecma_typedarray_get_int16_element */

/**
 * Read an uint16_t value from the given arraybuffer
 */
static ecma_value_t
ecma_typedarray_get_uint16_element (ecma_context_t *context_p, /**< JJS context */
                                    lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  JJS_UNUSED (context_p);
  uint16_t num;
  ECMA_TYPEDARRAY_GET_ELEMENT (src, num, uint16_t);
  return ecma_make_integer_value (num);
} /* ecma_typedarray_get_uint16_element */

/**
 * Read an int32_t value from the given arraybuffer
 */
static ecma_value_t
ecma_typedarray_get_int32_element (ecma_context_t *context_p, /**< JJS context */
                                   lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  int32_t num;
  ECMA_TYPEDARRAY_GET_ELEMENT (src, num, int32_t);
  return ecma_make_number_value (context_p, num);
} /* ecma_typedarray_get_int32_element */

/**
 * Read an uint32_t value from the given arraybuffer
 */
static ecma_value_t
ecma_typedarray_get_uint32_element (ecma_context_t *context_p, /**< JJS context */
                                    lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  uint32_t num;
  ECMA_TYPEDARRAY_GET_ELEMENT (src, num, uint32_t);
  return ecma_make_number_value (context_p, num);
} /* ecma_typedarray_get_uint32_element */

/**
 * Read a float value from the given arraybuffer
 */
static ecma_value_t
ecma_typedarray_get_float_element (ecma_context_t *context_p, /**< JJS context */
                                   lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  float num;
  ECMA_TYPEDARRAY_GET_ELEMENT (src, num, float);
  return ecma_make_number_value (context_p, num);
} /* ecma_typedarray_get_float_element */

/**
 * Read a double value from the given arraybuffer
 */
static ecma_value_t
ecma_typedarray_get_double_element (ecma_context_t *context_p, /**< JJS context */
                                    lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  double num;
  ECMA_TYPEDARRAY_GET_ELEMENT (src, num, double);
  return ecma_make_number_value (context_p, num);
} /* ecma_typedarray_get_double_element */

#if JJS_BUILTIN_BIGINT
/**
 * Read a bigint64 value from the given arraybuffer
 */
static ecma_value_t
ecma_typedarray_get_bigint64_element (ecma_context_t *context_p, /**< JJS context */
                                      lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  uint64_t num;
  ECMA_TYPEDARRAY_GET_ELEMENT (src, num, uint64_t);
  bool sign = (num >> 63) != 0;

  if (sign)
  {
    num = (uint64_t) (-(int64_t) num);
  }

  return ecma_bigint_create_from_digits (context_p, &num, 1, sign);
} /* ecma_typedarray_get_bigint64_element */

/**
 * Read a biguint64 value from the given arraybuffer
 */
static ecma_value_t
ecma_typedarray_get_biguint64_element (ecma_context_t *context_p, /**< JJS context */
                                       lit_utf8_byte_t *src) /**< the location in the internal arraybuffer */
{
  uint64_t num;
  ECMA_TYPEDARRAY_GET_ELEMENT (src, num, uint64_t);
  return ecma_bigint_create_from_digits (context_p, &num, 1, false);
} /* ecma_typedarray_get_biguint64_element */
#endif /* JJS_BUILTIN_BIGINT */

/**
 * Normalize the given ecma_number_t to an uint32_t value
 */
static uint32_t
ecma_typedarray_setter_number_to_uint32 (ecma_context_t *context_p, /**< JJS context */
                                         ecma_number_t value) /**< the number value to normalize */
{
  JJS_UNUSED (context_p);
  uint32_t uint32_value = 0;

  if (!ecma_number_is_nan (value) && !ecma_number_is_infinity (value))
  {
    bool is_negative = false;

    if (value < 0)
    {
      is_negative = true;
      value = -value;
    }

    if (value > ((ecma_number_t) 0xffffffff))
    {
      value = (ecma_number_t) (fmod (value, (ecma_number_t) 0x100000000));
    }

    uint32_value = (uint32_t) value;

    if (is_negative)
    {
      uint32_value = (uint32_t) (-(int32_t) uint32_value);
    }
  }

  return uint32_value;
} /* ecma_typedarray_setter_number_to_uint32 */

/**
 * Write an int8_t value into the given arraybuffer
 *
 * @return ECMA_VALUE_ERROR - if the ToNumber operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
static ecma_value_t
ecma_typedarray_set_int8_element (ecma_context_t *context_p, /**< JJS context */
                                  lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                  ecma_value_t value) /**< the number value to set */
{
  ecma_number_t result_num;
  ecma_value_t to_num = ecma_op_to_numeric (context_p, value, &result_num, ECMA_TO_NUMERIC_NO_OPTS);

  if (ECMA_IS_VALUE_ERROR (to_num))
  {
    return to_num;
  }

  int8_t num = (int8_t) ecma_typedarray_setter_number_to_uint32 (context_p, result_num);
  *dst_p = (lit_utf8_byte_t) num;
  return ECMA_VALUE_TRUE;
} /* ecma_typedarray_set_int8_element */

/**
 * Write an uint8_t value into the given arraybuffer
 *
 * @return ECMA_VALUE_ERROR - if the ToNumber operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
static ecma_value_t
ecma_typedarray_set_uint8_element (ecma_context_t *context_p, /**< JJS context */
                                   lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                   ecma_value_t value) /**< the number value to set */
{
  ecma_number_t result_num;
  ecma_value_t to_num = ecma_op_to_numeric (context_p, value, &result_num, ECMA_TO_NUMERIC_NO_OPTS);

  if (ECMA_IS_VALUE_ERROR (to_num))
  {
    return to_num;
  }

  uint8_t num = (uint8_t) ecma_typedarray_setter_number_to_uint32 (context_p, result_num);
  *dst_p = (lit_utf8_byte_t) num;
  return ECMA_VALUE_TRUE;
} /* ecma_typedarray_set_uint8_element */

/**
 * Write an uint8_t clamped value into the given arraybuffer
 *
 * @return ECMA_VALUE_ERROR - if the ToNumber operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
static ecma_value_t
ecma_typedarray_set_uint8_clamped_element (ecma_context_t *context_p, /**< JJS context */
                                           lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                           ecma_value_t value) /**< the number value to set */
{
  ecma_number_t result_num;
  ecma_value_t to_num = ecma_op_to_numeric (context_p, value, &result_num, ECMA_TO_NUMERIC_NO_OPTS);

  if (ECMA_IS_VALUE_ERROR (to_num))
  {
    return to_num;
  }

  uint8_t clamped;

  if (result_num > 255)
  {
    clamped = 255;
  }
  else if (result_num <= 0)
  {
    clamped = 0;
  }
  else
  {
    clamped = (uint8_t) result_num;

    if (clamped + 0.5 < result_num || (clamped + 0.5 == result_num && (clamped % 2) == 1))
    {
      clamped++;
    }
  }

  *dst_p = (lit_utf8_byte_t) clamped;
  return ECMA_VALUE_TRUE;
} /* ecma_typedarray_set_uint8_clamped_element */

/**
 * Write an int16_t value into the given arraybuffer
 *
 * @return ECMA_VALUE_ERROR - if the ToNumber operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
static ecma_value_t
ecma_typedarray_set_int16_element (ecma_context_t *context_p, /**< JJS context */
                                   lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                   ecma_value_t value) /**< the number value to set */
{
  ecma_number_t resut_num;
  ecma_value_t to_num = ecma_op_to_numeric (context_p, value, &resut_num, ECMA_TO_NUMERIC_NO_OPTS);

  if (ECMA_IS_VALUE_ERROR (to_num))
  {
    return to_num;
  }

  int16_t num = (int16_t) ecma_typedarray_setter_number_to_uint32 (context_p, resut_num);
  ECMA_TYPEDARRAY_SET_ELEMENT (dst_p, num, int16_t);
  return ECMA_VALUE_TRUE;
} /* ecma_typedarray_set_int16_element */

/**
 * Write an uint8_t value into the given arraybuffer
 *
 * @return ECMA_VALUE_ERROR - if the ToNumber operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
static ecma_value_t
ecma_typedarray_set_uint16_element (ecma_context_t *context_p, /**< JJS context */
                                    lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                    ecma_value_t value) /**< the number value to set */
{
  ecma_number_t resut_num;
  ecma_value_t to_num = ecma_op_to_numeric (context_p, value, &resut_num, ECMA_TO_NUMERIC_NO_OPTS);

  if (ECMA_IS_VALUE_ERROR (to_num))
  {
    return to_num;
  }

  uint16_t num = (uint16_t) ecma_typedarray_setter_number_to_uint32 (context_p, resut_num);
  ECMA_TYPEDARRAY_SET_ELEMENT (dst_p, num, uint16_t);
  return ECMA_VALUE_TRUE;
} /* ecma_typedarray_set_uint16_element */

/**
 * Write an int32_t value into the given arraybuffer
 *
 * @return ECMA_VALUE_ERROR - if the ToNumber operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
static ecma_value_t
ecma_typedarray_set_int32_element (ecma_context_t *context_p, /**< JJS context */
                                   lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                   ecma_value_t value) /**< the number value to set */
{
  ecma_number_t resut_num;
  ecma_value_t to_num = ecma_op_to_numeric (context_p, value, &resut_num, ECMA_TO_NUMERIC_NO_OPTS);

  if (ECMA_IS_VALUE_ERROR (to_num))
  {
    return to_num;
  }

  int32_t num = (int32_t) ecma_typedarray_setter_number_to_uint32 (context_p, resut_num);
  ECMA_TYPEDARRAY_SET_ELEMENT (dst_p, num, int32_t);
  return ECMA_VALUE_TRUE;
} /* ecma_typedarray_set_int32_element */

/**
 * Write an uint32_t value into the given arraybuffer
 *
 * @return ECMA_VALUE_ERROR - if the ToNumber operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
static ecma_value_t
ecma_typedarray_set_uint32_element (ecma_context_t *context_p, /**< JJS context */
                                    lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                    ecma_value_t value) /**< the number value to set */
{
  ecma_number_t resut_num;
  ecma_value_t to_num = ecma_op_to_numeric (context_p, value, &resut_num, ECMA_TO_NUMERIC_NO_OPTS);

  if (ECMA_IS_VALUE_ERROR (to_num))
  {
    return to_num;
  }

  uint32_t num = (uint32_t) ecma_typedarray_setter_number_to_uint32 (context_p, resut_num);
  ECMA_TYPEDARRAY_SET_ELEMENT (dst_p, num, uint32_t);
  return ECMA_VALUE_TRUE;
} /* ecma_typedarray_set_uint32_element */

/**
 * Write a float value into the given arraybuffer
 *
 * @return ECMA_VALUE_ERROR - if the ToNumber operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
static ecma_value_t
ecma_typedarray_set_float_element (ecma_context_t *context_p, /**< JJS context */
                                   lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                   ecma_value_t value) /**< the number value to set */
{
  ecma_number_t result_num;
  ecma_value_t to_num = ecma_op_to_numeric (context_p, value, &result_num, ECMA_TO_NUMERIC_NO_OPTS);

  if (ECMA_IS_VALUE_ERROR (to_num))
  {
    return to_num;
  }

  float num = (float) result_num;
  ECMA_TYPEDARRAY_SET_ELEMENT (dst_p, num, float);
  return ECMA_VALUE_TRUE;
} /* ecma_typedarray_set_float_element */

#if JJS_NUMBER_TYPE_FLOAT64
/**
 * Write a double value into the given arraybuffer
 *
 * @return ECMA_VALUE_ERROR - if the ToNumber operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
static ecma_value_t
ecma_typedarray_set_double_element (ecma_context_t *context_p, /**< JJS context */
                                    lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                    ecma_value_t value) /**< the number value to set */
{
  ecma_number_t result_num;
  ecma_value_t to_num = ecma_op_to_numeric (context_p, value, &result_num, ECMA_TO_NUMERIC_NO_OPTS);

  if (ECMA_IS_VALUE_ERROR (to_num))
  {
    return to_num;
  }

  double num = (double) result_num;
  ECMA_TYPEDARRAY_SET_ELEMENT (dst_p, num, double);
  return ECMA_VALUE_TRUE;
} /* ecma_typedarray_set_double_element */
#endif /* JJS_NUMBER_TYPE_FLOAT64 */

#if JJS_BUILTIN_BIGINT
/**
 * Write a bigint64/biguint64 value into the given arraybuffer
 *
 * @return ECMA_VALUE_ERROR - if the ToBigInt operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
static ecma_value_t
ecma_typedarray_set_bigint_element (ecma_context_t *context_p, /**< JJS context */
                                    lit_utf8_byte_t *dst_p, /**< the location in the internal arraybuffer */
                                    ecma_value_t value) /**< the bigint value to set */
{
  ecma_value_t bigint = ecma_bigint_to_bigint (context_p, value, false);

  if (ECMA_IS_VALUE_ERROR (bigint))
  {
    return bigint;
  }

  uint64_t num;
  bool sign;
  ecma_bigint_get_digits_and_sign (context_p, bigint, &num, 1, &sign);

  if (sign)
  {
    num = (uint64_t) (-(int64_t) num);
  }

  ECMA_TYPEDARRAY_SET_ELEMENT (dst_p, num, uint64_t);

  ecma_free_value (context_p, bigint);

  return ECMA_VALUE_TRUE;
} /* ecma_typedarray_set_bigint_element */
#endif /* JJS_BUILTIN_BIGINT */

/**
 * Builtin id of the first %TypedArray% builtin routine intrinsic object
 */
#define ECMA_FIRST_TYPEDARRAY_BUILTIN_ROUTINE_ID ECMA_BUILTIN_ID_INT8ARRAY

#if JJS_BUILTIN_BIGINT
/**
 * Builtin id of the last %TypedArray% builtin routine intrinsic object
 */
#define ECMA_LAST_TYPEDARRAY_BUILTIN_ROUTINE_ID ECMA_BUILTIN_ID_BIGUINT64ARRAY
#elif !JJS_BUILTIN_BIGINT && JJS_NUMBER_TYPE_FLOAT64
/**
 * Builtin id of the last %TypedArray% builtin routine intrinsic object
 */
#define ECMA_LAST_TYPEDARRAY_BUILTIN_ROUTINE_ID ECMA_BUILTIN_ID_FLOAT64ARRAY
#else /* !JJS_NUMBER_TYPE_FLOAT64 */
/**
 * Builtin id of the last %TypedArray% builtin routine intrinsic object
 */
#define ECMA_LAST_TYPEDARRAY_BUILTIN_ROUTINE_ID ECMA_BUILTIN_ID_FLOAT32ARRAY
#endif /* JJS_NUMBER_TYPE_FLOAT64 */

/**
 * Builtin id of the first %TypedArray% builtin prototype intrinsic object
 */
#define ECMA_FIRST_TYPEDARRAY_BUILTIN_PROTOTYPE_ID ECMA_BUILTIN_ID_INT8ARRAY_PROTOTYPE

/**
 * List of typedarray getters based on their builtin id
 */
static const ecma_typedarray_getter_fn_t ecma_typedarray_getters[] = {
  ecma_typedarray_get_int8_element, /**< Int8Array */
  ecma_typedarray_get_uint8_element, /**< Uint8Array */
  ecma_typedarray_get_uint8_element, /**< Uint8ClampedArray */
  ecma_typedarray_get_int16_element, /**< Int16Array */
  ecma_typedarray_get_uint16_element, /**< Int32Array */
  ecma_typedarray_get_int32_element, /**< Uint32Array */
  ecma_typedarray_get_uint32_element, /**< Uint32Array */
  ecma_typedarray_get_float_element, /**< Float32Array */
#if JJS_NUMBER_TYPE_FLOAT64
  ecma_typedarray_get_double_element, /**< Float64Array */
#endif /* JJS_NUMBER_TYPE_FLOAT64 */
#if JJS_BUILTIN_BIGINT
  ecma_typedarray_get_bigint64_element, /**< BigInt64Array*/
  ecma_typedarray_get_biguint64_element, /**< BigUint64Array */
#endif /* JJS_BUILTIN_BIGINT */
};

/**
 * List of typedarray setters based on their builtin id
 */
static const ecma_typedarray_setter_fn_t ecma_typedarray_setters[] = {
  ecma_typedarray_set_int8_element, /**< Int8Array */
  ecma_typedarray_set_uint8_element, /**< Uint8Array */
  ecma_typedarray_set_uint8_clamped_element, /**< Uint8ClampedArray */
  ecma_typedarray_set_int16_element, /**< Int16Array */
  ecma_typedarray_set_uint16_element, /**< Int32Array */
  ecma_typedarray_set_int32_element, /**< Uint32Array */
  ecma_typedarray_set_uint32_element, /**< Uint32Array */
  ecma_typedarray_set_float_element, /**< Float32Array */
#if JJS_NUMBER_TYPE_FLOAT64
  ecma_typedarray_set_double_element, /**< Float64Array */
#endif /* JJS_NUMBER_TYPE_FLOAT64 */
#if JJS_BUILTIN_BIGINT
  ecma_typedarray_set_bigint_element, /**< BigInt64Array */
  ecma_typedarray_set_bigint_element, /**< BigUInt64Array */
#endif /* JJS_BUILTIN_BIGINT */
};

/**
 * List of typedarray element shift sizes based on their builtin id
 */
static const uint8_t ecma_typedarray_element_shift_sizes[] = {
  0, /**< Int8Array */
  0, /**< Uint8Array */
  0, /**< Uint8ClampedArray */
  1, /**< Int16Array */
  1, /**< Uint16Array */
  2, /**< Int32Array */
  2, /**< Uint32Array */
  2, /**< Float32Array */
#if JJS_NUMBER_TYPE_FLOAT64
  3, /**< Float64Array */
#endif /* JJS_NUMBER_TYPE_FLOAT64 */
#if JJS_BUILTIN_BIGINT
  3, /**< BigInt64Array */
  3, /**< BigUInt64Array */
#endif /* JJS_BUILTIN_BIGINT */
};

/**
 * List of typedarray class magic strings based on their builtin id
 */
static const uint16_t ecma_typedarray_magic_string_list[] = {
  (uint16_t) LIT_MAGIC_STRING_INT8_ARRAY_UL, /**< Int8Array */
  (uint16_t) LIT_MAGIC_STRING_UINT8_ARRAY_UL, /**< Uint8Array */
  (uint16_t) LIT_MAGIC_STRING_UINT8_CLAMPED_ARRAY_UL, /**< Uint8ClampedArray */
  (uint16_t) LIT_MAGIC_STRING_INT16_ARRAY_UL, /**< Int16Array */
  (uint16_t) LIT_MAGIC_STRING_UINT16_ARRAY_UL, /**< Uint16Array */
  (uint16_t) LIT_MAGIC_STRING_INT32_ARRAY_UL, /**< Int32Array */
  (uint16_t) LIT_MAGIC_STRING_UINT32_ARRAY_UL, /**< Uint32Array */
  (uint16_t) LIT_MAGIC_STRING_FLOAT32_ARRAY_UL, /**< Float32Array */
#if JJS_NUMBER_TYPE_FLOAT64
  (uint16_t) LIT_MAGIC_STRING_FLOAT64_ARRAY_UL, /**< Float64Array */
#endif /* JJS_NUMBER_TYPE_FLOAT64 */
#if JJS_BUILTIN_BIGINT
  (uint16_t) LIT_MAGIC_STRING_BIGINT64_ARRAY_UL, /**< BigInt64Array */
  (uint16_t) LIT_MAGIC_STRING_BIGUINT64_ARRAY_UL, /**< BigUInt64Array */
#endif /* JJS_BUILTIN_BIGINT */
};

/**
 * Get the magic string id of a typedarray
 *
 * @return magic string
 */
extern inline lit_magic_string_id_t JJS_ATTR_ALWAYS_INLINE
ecma_get_typedarray_magic_string_id (ecma_typedarray_type_t typedarray_id)
{
  return (lit_magic_string_id_t) ecma_typedarray_magic_string_list[typedarray_id];
} /* ecma_get_typedarray_magic_string_id */

/**
 * Get typedarray's getter function callback
 *
 * @return ecma_typedarray_getter_fn_t: the getter function for the given builtin TypedArray id
 */
extern inline ecma_typedarray_getter_fn_t JJS_ATTR_ALWAYS_INLINE
ecma_get_typedarray_getter_fn (ecma_typedarray_type_t typedarray_id) /**< typedarray id */
{
  return ecma_typedarray_getters[typedarray_id];
} /* ecma_get_typedarray_getter_fn */

/**
 * Get element from a TypedArray
 *
 * @return the value of the element
 */
extern inline ecma_value_t JJS_ATTR_ALWAYS_INLINE
ecma_get_typedarray_element (ecma_context_t *context_p, /**< JJS context */
                             ecma_typedarray_info_t *info_p, /**< typedarray info */
                             uint32_t index) /**< element index */
{
  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, info_p->array_buffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p) || index >= info_p->length)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  uint8_t *buffer_p = ecma_typedarray_get_buffer (context_p, info_p);

  return ecma_typedarray_getters[info_p->id](context_p, buffer_p + (index << info_p->shift));
} /* ecma_get_typedarray_element */

/**
 * Get typedarray's setter function callback
 *
 * @return ecma_typedarray_setter_fn_t: the setter function for the given builtin TypedArray id
 */
extern inline ecma_typedarray_setter_fn_t JJS_ATTR_ALWAYS_INLINE
ecma_get_typedarray_setter_fn (ecma_typedarray_type_t typedarray_id) /**< typedarray id */
{
  return ecma_typedarray_setters[typedarray_id];
} /* ecma_get_typedarray_setter_fn */

/**
 * set typedarray's element value
 */
extern inline ecma_value_t JJS_ATTR_ALWAYS_INLINE
ecma_set_typedarray_element (ecma_context_t *context_p, /**< JJS context */
                             ecma_typedarray_info_t *info_p, /**< typedarray info */
                             ecma_value_t value, /**< value to be set */
                             uint32_t index) /**< element index */
{
  ecma_value_t to_num;
  if (ECMA_TYPEDARRAY_IS_BIGINT_TYPE (info_p->id))
  {
    to_num = ecma_bigint_to_bigint (context_p, value, false);

    if (ECMA_IS_VALUE_ERROR (to_num))
    {
      return to_num;
    }
  }
  else
  {
    ecma_number_t result_num;
    to_num = ecma_op_to_numeric (context_p, value, &result_num, ECMA_TO_NUMERIC_NO_OPTS);

    if (ECMA_IS_VALUE_ERROR (to_num))
    {
      return to_num;
    }
  }

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, info_p->array_buffer_p))
  {
    ecma_free_value (context_p, to_num);
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, info_p->array_buffer_p) || index >= info_p->length)
  {
    ecma_free_value (context_p, to_num);
    return ECMA_VALUE_FALSE;
  }

  uint8_t *buffer_p = ecma_typedarray_get_buffer (context_p, info_p);

  ecma_free_value (context_p, to_num);

  return ecma_typedarray_setters[info_p->id](context_p, buffer_p + (index << info_p->shift), value);
} /* ecma_set_typedarray_element */

/**
 * Get the element shift size of a TypedArray type.
 *
 * @return uint8_t
 */
extern inline uint8_t JJS_ATTR_ALWAYS_INLINE
ecma_typedarray_helper_get_shift_size (ecma_typedarray_type_t typedarray_id)
{
  return ecma_typedarray_element_shift_sizes[typedarray_id];
} /* ecma_typedarray_helper_get_shift_size */

/**
 * Check if the builtin is a TypedArray type.
 *
 * @return bool: - true if based on the given id it is a TypedArray
 *               - false if based on the given id it is not a TypedArray
 */
bool
ecma_typedarray_helper_is_typedarray (ecma_builtin_id_t builtin_id) /**< the builtin id of a type **/
{
  return ((builtin_id >= ECMA_FIRST_TYPEDARRAY_BUILTIN_ROUTINE_ID)
          && (builtin_id <= ECMA_LAST_TYPEDARRAY_BUILTIN_ROUTINE_ID));
} /* ecma_typedarray_helper_is_typedarray */

/**
 * Get the prototype ID of a TypedArray type.
 *
 * @return ecma_builtin_id_t
 */
ecma_builtin_id_t
ecma_typedarray_helper_get_prototype_id (ecma_typedarray_type_t typedarray_id) /**< the id of the typedarray **/
{
  return (ecma_builtin_id_t) (ECMA_FIRST_TYPEDARRAY_BUILTIN_PROTOTYPE_ID + typedarray_id);
} /* ecma_typedarray_helper_get_prototype_id */

/**
 * Get the constructor ID of a TypedArray type.
 *
 * @return ecma_builtin_id_t
 */
ecma_builtin_id_t
ecma_typedarray_helper_get_constructor_id (ecma_typedarray_type_t typedarray_id) /**< the id of the typedarray **/
{
  return (ecma_builtin_id_t) (ECMA_FIRST_TYPEDARRAY_BUILTIN_ROUTINE_ID + typedarray_id);
} /* ecma_typedarray_helper_get_constructor_id */

/**
 * Get the built-in TypedArray type of the given object.
 *
 * @return ecma_typedarray_type_t
 */
ecma_typedarray_type_t
ecma_get_typedarray_id (ecma_context_t *context_p, /**< JJS context */
                        ecma_object_t *obj_p) /**< typedarray object **/
{
  JJS_UNUSED (context_p);
  JJS_ASSERT (ecma_object_is_typedarray (context_p, obj_p));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

  return (ecma_typedarray_type_t) ext_object_p->u.cls.u1.typedarray_type;
} /* ecma_get_typedarray_id */

/**
 * Get the built-in TypedArray type of the given object.
 *
 * @return ecma_typedarray_type_t
 */
ecma_typedarray_type_t
ecma_typedarray_helper_builtin_to_typedarray_id (ecma_builtin_id_t builtin_id)
{
  JJS_ASSERT (ecma_typedarray_helper_is_typedarray (builtin_id));

  return (ecma_typedarray_type_t) (builtin_id - ECMA_FIRST_TYPEDARRAY_BUILTIN_ROUTINE_ID);
} /* ecma_typedarray_helper_builtin_to_typedarray_id */

/**
 * Create a TypedArray object by given array_length
 *
 * See also: ES2015 22.2.1.2.1
 *
 * @return ecma value of the new typedarray object
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_typedarray_create_object_with_length (ecma_context_t *context_p, /**< JJS context */
                                           uint32_t array_length, /**< length of the typedarray */
                                           ecma_object_t *src_buffer_p, /**< source buffer */
                                           ecma_object_t *proto_p, /**< prototype object */
                                           uint8_t element_size_shift, /**< the size shift of the element length */
                                           ecma_typedarray_type_t typedarray_id) /**< id of the typedarray */
{
  uint32_t byte_length = UINT32_MAX;

  if (array_length <= (UINT32_MAX >> element_size_shift))
  {
    byte_length = array_length << element_size_shift;
  }

  if (byte_length > UINT32_MAX - sizeof (ecma_extended_object_t) - JMEM_ALIGNMENT + 1)
  {
#if JJS_ERROR_MESSAGES
    ecma_value_t array_length_value = ecma_make_number_value (context_p, array_length);

    ecma_value_t result =
      ecma_raise_standard_error_with_format (context_p, JJS_ERROR_RANGE, "Invalid typed array length: %", array_length_value);
    ecma_free_value (context_p, array_length_value);
    return result;
#else /* !JJS_ERROR_MESSAGES */
    return ecma_raise_range_error (context_p, ECMA_ERR_EMPTY);
#endif /* JJS_ERROR_MESSAGES */
  }

  ecma_object_t *new_arraybuffer_p = NULL;
  if (src_buffer_p == NULL)
  {
    new_arraybuffer_p = ecma_arraybuffer_new_object (context_p, byte_length);
  }
  else
  {
    ecma_value_t ctor_proto = ecma_op_species_constructor (context_p, src_buffer_p, ECMA_BUILTIN_ID_ARRAYBUFFER);

    if (ECMA_IS_VALUE_ERROR (ctor_proto))
    {
      return ctor_proto;
    }

    ecma_object_t *ctor_proto_p = ecma_get_object_from_value (context_p, ctor_proto);

    ecma_object_t *prototype_p =
      ecma_op_get_prototype_from_constructor (context_p, ctor_proto_p, ECMA_BUILTIN_ID_ARRAYBUFFER_PROTOTYPE);

    ecma_deref_object (ctor_proto_p);

    if (JJS_UNLIKELY (prototype_p == NULL))
    {
      return ECMA_VALUE_ERROR;
    }

    new_arraybuffer_p = ecma_arraybuffer_new_object (context_p, byte_length);

    ECMA_SET_NON_NULL_POINTER (context_p, new_arraybuffer_p->u2.prototype_cp, prototype_p);

    ecma_deref_object (prototype_p);

    if (ecma_arraybuffer_is_detached (context_p, src_buffer_p))
    {
      ecma_deref_object (new_arraybuffer_p);
      return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
    }
  }

  ecma_object_t *object_p = ecma_create_object (context_p, proto_p, sizeof (ecma_extended_object_t), ECMA_OBJECT_TYPE_CLASS);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.cls.type = ECMA_OBJECT_CLASS_TYPEDARRAY;
  ext_object_p->u.cls.u1.typedarray_type = (uint8_t) typedarray_id;
  ext_object_p->u.cls.u2.typedarray_flags = 0;
  ext_object_p->u.cls.u3.arraybuffer = ecma_make_object_value (context_p, new_arraybuffer_p);

  ecma_deref_object (new_arraybuffer_p);

  return ecma_make_object_value (context_p, object_p);
} /* ecma_typedarray_create_object_with_length */

/**
 * Create a TypedArray object by given another TypedArray object
 *
 * See also: ES2015 22.2.1.3
 *
 * @return ecma value of the new typedarray object
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
ecma_typedarray_create_object_with_typedarray (ecma_context_t *context_p, /**< JJS context */
                                               ecma_object_t *typedarray_p, /**< a typedarray object */
                                               ecma_object_t *proto_p, /**< prototype object */
                                               uint8_t element_size_shift, /**< the size shift of the element length */
                                               ecma_typedarray_type_t typedarray_id) /**< id of the typedarray */
{
  uint32_t array_length = ecma_typedarray_get_length (context_p, typedarray_p);
  ecma_object_t *src_arraybuffer_p = ecma_typedarray_get_arraybuffer (context_p, typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, src_arraybuffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, src_arraybuffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  ecma_value_t new_typedarray = ecma_typedarray_create_object_with_length (context_p,
                                                                           array_length,
                                                                           src_arraybuffer_p,
                                                                           proto_p,
                                                                           element_size_shift,
                                                                           typedarray_id);

  if (ECMA_IS_VALUE_ERROR (new_typedarray))
  {
    return new_typedarray;
  }

  ecma_object_t *new_typedarray_p = ecma_get_object_from_value (context_p, new_typedarray);
  ecma_object_t *dst_arraybuffer_p = ecma_typedarray_get_arraybuffer (context_p, new_typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, dst_arraybuffer_p))
  {
    ecma_deref_object (new_typedarray_p);
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, dst_arraybuffer_p))
  {
    ecma_deref_object (new_typedarray_p);
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  lit_utf8_byte_t *src_buf_p = ecma_arraybuffer_get_buffer (context_p, src_arraybuffer_p);
  lit_utf8_byte_t *dst_buf_p = ecma_arraybuffer_get_buffer (context_p, dst_arraybuffer_p);

  src_buf_p += ecma_typedarray_get_offset (context_p, typedarray_p);

  ecma_typedarray_type_t src_id = ecma_get_typedarray_id (context_p, typedarray_p);

  if (src_id == typedarray_id)
  {
    memcpy (dst_buf_p, src_buf_p, array_length << element_size_shift);
  }
  else
  {
#if JJS_BUILTIN_BIGINT
    if ((ECMA_TYPEDARRAY_IS_BIGINT_TYPE (src_id) ^ ECMA_TYPEDARRAY_IS_BIGINT_TYPE (typedarray_id)) == 1)
    {
      ecma_deref_object (new_typedarray_p);
      return ecma_raise_type_error (context_p, ECMA_ERR_INCOMPATIBLE_TYPEDARRAY_TYPES);
    }
#endif /* JJS_BUILTIN_BIGINT */

    uint32_t src_element_size = 1u << ecma_typedarray_get_element_size_shift (context_p, typedarray_p);
    uint32_t dst_element_size = 1u << element_size_shift;
    ecma_typedarray_getter_fn_t src_typedarray_getter_cb = ecma_get_typedarray_getter_fn (src_id);
    ecma_typedarray_setter_fn_t target_typedarray_setter_cb = ecma_get_typedarray_setter_fn (typedarray_id);

    for (uint32_t i = 0; i < array_length; i++)
    {
      /* Convert values from source to destination format. */
      ecma_value_t tmp = src_typedarray_getter_cb (context_p, src_buf_p);
      ecma_value_t set_element = target_typedarray_setter_cb (context_p, dst_buf_p, tmp);

      ecma_free_value (context_p, tmp);

      if (ECMA_IS_VALUE_ERROR (set_element))
      {
        ecma_deref_object (new_typedarray_p);
        return set_element;
      }

      src_buf_p += src_element_size;
      dst_buf_p += dst_element_size;
    }
  }

  return new_typedarray;
} /* ecma_typedarray_create_object_with_typedarray */

/**
 * Helper method for ecma_op_typedarray_from
 *
 * @return ECMA_VALUE_TRUE - if setting the given value to the new typedarray was successful
 *         ECMA_VALUE_ERROR - otherwise
 */
static ecma_value_t
ecma_op_typedarray_from_helper (ecma_context_t *context_p, /**< JJS context */
                                ecma_value_t this_val, /**< this_arg for the above from function */
                                ecma_value_t current_value, /**< given value to set */
                                uint32_t index, /**< currrent index */
                                ecma_object_t *func_object_p, /**< map function object */
                                uint8_t *buffer_p, /**< target buffer */
                                ecma_typedarray_setter_fn_t setter_cb) /**< setter callback function */
{
  ecma_value_t mapped_value = current_value;

  if (func_object_p != NULL)
  {
    /* 17.d 17.f */
    ecma_value_t current_index = ecma_make_uint32_value (context_p, index);
    ecma_value_t call_args[] = { current_value, current_index };

    ecma_value_t cb_value = ecma_op_function_call (context_p, func_object_p, this_val, call_args, 2);

    ecma_free_value (context_p, current_value);
    ecma_free_value (context_p, current_index);

    if (ECMA_IS_VALUE_ERROR (cb_value))
    {
      return cb_value;
    }

    mapped_value = cb_value;
  }

  ecma_value_t set_element = setter_cb (context_p, buffer_p, mapped_value);
  ecma_free_value (context_p, mapped_value);

  if (ECMA_IS_VALUE_ERROR (set_element))
  {
    return set_element;
  }

  return ECMA_VALUE_TRUE;
} /* ecma_op_typedarray_from_helper */

/**
 * Create a TypedArray object by transforming from an array-like object or iterable object
 *
 * See also: ES11 22.2.4.4
 *
 * @return ecma value of the new typedarray object
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_typedarray_create_object_with_object (ecma_context_t *context_p, /**< JJS context */
                                           ecma_value_t items_val, /**< the source array-like object */
                                           ecma_object_t *proto_p, /**< prototype object */
                                           uint8_t element_size_shift, /**< the size shift of the element length */
                                           ecma_typedarray_type_t typedarray_id) /**< id of the typedarray */
{
  /* 5 */
  ecma_value_t using_iterator = ecma_op_get_method_by_symbol_id (context_p, items_val, LIT_GLOBAL_SYMBOL_ITERATOR);

  if (ECMA_IS_VALUE_ERROR (using_iterator))
  {
    return using_iterator;
  }

  /* 6 */
  if (!ecma_is_value_undefined (using_iterator))
  {
    /* 6.a */
    ecma_value_t next_method;
    ecma_value_t iterator = ecma_op_get_iterator (context_p, items_val, using_iterator, &next_method);
    ecma_free_value (context_p, using_iterator);

    if (ECMA_IS_VALUE_ERROR (iterator))
    {
      return iterator;
    }

    ecma_collection_t *values_p = ecma_new_collection (context_p);
    ecma_value_t ret_value = ECMA_VALUE_EMPTY;

    while (true)
    {
      ecma_value_t next = ecma_op_iterator_step (context_p, iterator, next_method);

      if (ECMA_IS_VALUE_ERROR (next))
      {
        ret_value = next;
        break;
      }

      if (next == ECMA_VALUE_FALSE)
      {
        break;
      }

      ecma_value_t next_value = ecma_op_iterator_value (context_p, next);
      ecma_free_value (context_p, next);

      if (ECMA_IS_VALUE_ERROR (next_value))
      {
        ret_value = next_value;
        break;
      }

      ecma_collection_push_back (context_p, values_p, next_value);
    }

    ecma_free_value (context_p, iterator);
    ecma_free_value (context_p, next_method);

    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      ecma_collection_free (context_p, values_p);
      return ret_value;
    }

    /* 8.c */
    ecma_value_t new_typedarray = ecma_typedarray_create_object_with_length (context_p,
                                                                             values_p->item_count,
                                                                             NULL,
                                                                             proto_p,
                                                                             element_size_shift,
                                                                             typedarray_id);

    if (ECMA_IS_VALUE_ERROR (new_typedarray))
    {
      ecma_collection_free (context_p, values_p);
      return new_typedarray;
    }

    ecma_object_t *new_typedarray_p = ecma_get_object_from_value (context_p, new_typedarray);
    ecma_typedarray_info_t info = ecma_typedarray_get_info (context_p, new_typedarray_p);
    ecma_value_t *next_value_p = values_p->buffer_p;

    ret_value = ECMA_VALUE_ERROR;

    if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, info.array_buffer_p))
    {
      goto free_collection;
    }

    if (ecma_arraybuffer_is_detached (context_p, info.array_buffer_p))
    {
      ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
      goto free_collection;
    }

    uint8_t *buffer_p = ecma_typedarray_get_buffer (context_p, &info);

    ecma_typedarray_setter_fn_t setter_cb = ecma_get_typedarray_setter_fn (info.id);
    uint8_t *limit_p = buffer_p + (values_p->item_count << info.shift);

    ret_value = ecma_make_object_value (context_p, new_typedarray_p);

    /* 8.e */
    while (buffer_p < limit_p)
    {
      ecma_value_t value = *next_value_p++;
      ecma_value_t set_value = setter_cb (context_p, buffer_p, value);
      ecma_free_value (context_p, value);

      if (ECMA_IS_VALUE_ERROR (set_value))
      {
        ret_value = set_value;
        break;
      }

      buffer_p += info.element_size;
    }

free_collection:
    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      ecma_value_t *last_value_p = values_p->buffer_p + values_p->item_count;

      while (next_value_p < last_value_p)
      {
        ecma_free_value (context_p, *next_value_p++);
      }

      ecma_deref_object (new_typedarray_p);
    }

    ecma_collection_destroy (context_p, values_p);
    return ret_value;
  }

  /* 8 */
  ecma_value_t arraylike_object_val = ecma_op_to_object (context_p, items_val);

  if (ECMA_IS_VALUE_ERROR (arraylike_object_val))
  {
    return arraylike_object_val;
  }

  ecma_object_t *arraylike_object_p = ecma_get_object_from_value (context_p, arraylike_object_val);

  /* 9 */
  ecma_length_t length_index;
  ecma_value_t len_value = ecma_op_object_get_length (context_p, arraylike_object_p, &length_index);

  if (ECMA_IS_VALUE_ERROR (len_value))
  {
    ecma_deref_object (arraylike_object_p);
    return len_value;
  }

  if (length_index >= UINT32_MAX)
  {
    ecma_deref_object (arraylike_object_p);
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_TYPEDARRAY_LENGTH);
  }

  uint32_t len = (uint32_t) length_index;

  /* 10 */
  ecma_value_t new_typedarray =
    ecma_typedarray_create_object_with_length (context_p, len, NULL, proto_p, element_size_shift, typedarray_id);

  if (ECMA_IS_VALUE_ERROR (new_typedarray))
  {
    ecma_deref_object (arraylike_object_p);
    return new_typedarray;
  }

  ecma_object_t *new_typedarray_p = ecma_get_object_from_value (context_p, new_typedarray);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (context_p, new_typedarray_p);

  ecma_value_t ret_value = ECMA_VALUE_ERROR;

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, info.array_buffer_p))
  {
    goto free_object;
  }

  if (ecma_arraybuffer_is_detached (context_p, info.array_buffer_p))
  {
    ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
    goto free_object;
  }

  uint8_t *buffer_p = ecma_typedarray_get_buffer (context_p, &info);

  ecma_typedarray_setter_fn_t setter_cb = ecma_get_typedarray_setter_fn (info.id);

  ret_value = ecma_make_object_value (context_p, new_typedarray_p);

  /* 12 */
  for (uint32_t index = 0; index < len; index++)
  {
    ecma_value_t value = ecma_op_object_find_by_index (context_p, arraylike_object_p, index);

    if (ECMA_IS_VALUE_ERROR (value))
    {
      ret_value = value;
      break;
    }

    if (!ecma_is_value_found (value))
    {
      value = ECMA_VALUE_UNDEFINED;
    }

    ecma_value_t set_value = setter_cb (context_p, buffer_p, value);
    ecma_free_value (context_p, value);

    if (ECMA_IS_VALUE_ERROR (set_value))
    {
      ret_value = set_value;
      break;
    }

    buffer_p += info.element_size;
  }

free_object:
  ecma_deref_object (arraylike_object_p);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_deref_object (new_typedarray_p);
  }

  return ret_value;
} /* ecma_typedarray_create_object_with_object */

/**
 * Create a TypedArray object by transforming from an array-like object or iterable object
 *
 * See also: ES11 22.2.2.1
 *
 * @return ecma value of the new typedarray object
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_typedarray_from (ecma_context_t *context_p, /**< JJS context */
                         ecma_value_t this_val, /**< this value */
                         ecma_value_t source_val, /**< source value */
                         ecma_value_t map_fn_val, /**< mapped function value */
                         ecma_value_t this_arg) /**< this argument */
{
  JJS_UNUSED (this_arg);
  /* 3 */
  JJS_ASSERT (ecma_op_is_callable (context_p, map_fn_val) || ecma_is_value_undefined (map_fn_val));

  /* 4 */
  ecma_object_t *func_object_p = NULL;

  if (!ecma_is_value_undefined (map_fn_val))
  {
    func_object_p = ecma_get_object_from_value (context_p, map_fn_val);
  }

  /* 5 */
  ecma_value_t using_iterator = ecma_op_get_method_by_symbol_id (context_p, source_val, LIT_GLOBAL_SYMBOL_ITERATOR);

  if (ECMA_IS_VALUE_ERROR (using_iterator))
  {
    return using_iterator;
  }

  /* 6 */
  if (!ecma_is_value_undefined (using_iterator))
  {
    /* 6.a */
    ecma_value_t next_method;
    ecma_value_t iterator = ecma_op_get_iterator (context_p, source_val, using_iterator, &next_method);
    ecma_free_value (context_p, using_iterator);

    /* 6.b */
    if (ECMA_IS_VALUE_ERROR (iterator))
    {
      return iterator;
    }

    /* 6.c */
    ecma_collection_t *values_p = ecma_new_collection (context_p);
    ecma_value_t ret_value = ECMA_VALUE_EMPTY;

    /* 6.e */
    while (true)
    {
      ecma_value_t next = ecma_op_iterator_step (context_p, iterator, next_method);

      if (ECMA_IS_VALUE_ERROR (next))
      {
        ret_value = next;
        break;
      }

      if (next == ECMA_VALUE_FALSE)
      {
        break;
      }

      ecma_value_t next_value = ecma_op_iterator_value (context_p, next);
      ecma_free_value (context_p, next);

      if (ECMA_IS_VALUE_ERROR (next_value))
      {
        ret_value = next_value;
        break;
      }

      ecma_collection_push_back (context_p, values_p, next_value);
    }

    ecma_free_value (context_p, iterator);
    ecma_free_value (context_p, next_method);

    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      ecma_collection_free (context_p, values_p);
      return ret_value;
    }

    /* 6.c */
    ecma_object_t *constructor_obj_p = ecma_get_object_from_value (context_p, this_val);
    ecma_value_t len_val = ecma_make_uint32_value (context_p, values_p->item_count);
    ecma_value_t new_typedarray = ecma_typedarray_create (context_p, constructor_obj_p, &len_val, 1);
    ecma_free_value (context_p, len_val);

    if (ECMA_IS_VALUE_ERROR (new_typedarray))
    {
      ecma_collection_free (context_p, values_p);
      return new_typedarray;
    }

    ecma_object_t *new_typedarray_p = ecma_get_object_from_value (context_p, new_typedarray);
    ecma_typedarray_info_t info = ecma_typedarray_get_info (context_p, new_typedarray_p);
    ecma_typedarray_setter_fn_t setter_cb = ecma_get_typedarray_setter_fn (info.id);
    ecma_value_t *next_value_p = values_p->buffer_p;

    ret_value = ECMA_VALUE_ERROR;

    if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, info.array_buffer_p))
    {
      goto free_collection;
    }

    if (ecma_arraybuffer_is_detached (context_p, info.array_buffer_p))
    {
      ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
      goto free_collection;
    }

    uint8_t *buffer_p = ecma_typedarray_get_buffer (context_p, &info);

    ret_value = ecma_make_object_value (context_p, new_typedarray_p);

    /* 6.e */
    for (uint32_t index = 0; index < values_p->item_count; index++)
    {
      ecma_value_t set_value =
        ecma_op_typedarray_from_helper (context_p, this_arg, *next_value_p++, index, func_object_p, buffer_p, setter_cb);

      if (ECMA_IS_VALUE_ERROR (set_value))
      {
        ret_value = set_value;
        break;
      }

      buffer_p += info.element_size;
    }

free_collection:
    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      ecma_value_t *last_value_p = values_p->buffer_p + values_p->item_count;

      while (next_value_p < last_value_p)
      {
        ecma_free_value (context_p, *next_value_p++);
      }

      ecma_deref_object (new_typedarray_p);
    }

    ecma_collection_destroy (context_p, values_p);
    return ret_value;
  }

  /* 8 */
  ecma_value_t arraylike_object_val = ecma_op_to_object (context_p, source_val);

  if (ECMA_IS_VALUE_ERROR (arraylike_object_val))
  {
    return arraylike_object_val;
  }

  ecma_object_t *arraylike_object_p = ecma_get_object_from_value (context_p, arraylike_object_val);

  /* 9 */
  ecma_length_t length_index;
  ecma_value_t len_value = ecma_op_object_get_length (context_p, arraylike_object_p, &length_index);

  if (ECMA_IS_VALUE_ERROR (len_value))
  {
    ecma_deref_object (arraylike_object_p);
    return len_value;
  }

  if (length_index >= UINT32_MAX)
  {
    ecma_deref_object (arraylike_object_p);
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_TYPEDARRAY_LENGTH);
  }

  uint32_t len = (uint32_t) length_index;

  /* 10 */
  ecma_object_t *constructor_obj_p = ecma_get_object_from_value (context_p, this_val);
  ecma_value_t len_val = ecma_make_uint32_value (context_p, len);
  ecma_value_t new_typedarray = ecma_typedarray_create (context_p, constructor_obj_p, &len_val, 1);
  ecma_free_value (context_p, len_val);

  if (ECMA_IS_VALUE_ERROR (new_typedarray))
  {
    ecma_deref_object (arraylike_object_p);
    return new_typedarray;
  }

  ecma_object_t *new_typedarray_p = ecma_get_object_from_value (context_p, new_typedarray);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (context_p, new_typedarray_p);

  ecma_value_t ret_value = ECMA_VALUE_ERROR;

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, info.array_buffer_p))
  {
    goto free_object;
  }

  if (ecma_arraybuffer_is_detached (context_p, info.array_buffer_p))
  {
    ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
    goto free_object;
  }

  uint8_t *buffer_p = ecma_typedarray_get_buffer (context_p, &info);

  ecma_typedarray_setter_fn_t setter_cb = ecma_get_typedarray_setter_fn (info.id);

  ret_value = ecma_make_object_value (context_p, new_typedarray_p);

  /* 12 */
  for (uint32_t index = 0; index < len; index++)
  {
    ecma_value_t value = ecma_op_object_find_by_index (context_p, arraylike_object_p, index);

    if (ECMA_IS_VALUE_ERROR (value))
    {
      ret_value = value;
      break;
    }

    if (!ecma_is_value_found (value))
    {
      value = ECMA_VALUE_UNDEFINED;
    }

    ecma_value_t set_value =
      ecma_op_typedarray_from_helper (context_p, this_arg, value, index, func_object_p, buffer_p, setter_cb);

    if (ECMA_IS_VALUE_ERROR (set_value))
    {
      ret_value = set_value;
      break;
    }

    buffer_p += info.element_size;
  }

free_object:
  ecma_deref_object (arraylike_object_p);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_deref_object (new_typedarray_p);
  }

  return ret_value;
} /* ecma_op_typedarray_from */

/**
 * Get the arraybuffer of the typedarray object
 *
 * @return the pointer to the internal arraybuffer
 */
extern inline ecma_object_t *JJS_ATTR_ALWAYS_INLINE
ecma_typedarray_get_arraybuffer (ecma_context_t *context_p, /**< JJS context */
                                 ecma_object_t *typedarray_p) /**< the pointer to the typedarray object */
{
  JJS_ASSERT (ecma_object_is_typedarray (context_p, typedarray_p));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) typedarray_p;

  return ecma_get_object_from_value (context_p, ext_object_p->u.cls.u3.arraybuffer);
} /* ecma_typedarray_get_arraybuffer */

/**
 * Get the element size shift in the typedarray object
 *
 * @return the size shift of the element, size is 1 << shift
 */
uint8_t
ecma_typedarray_get_element_size_shift (ecma_context_t *context_p, /**< JJS context */
                                        ecma_object_t *typedarray_p) /**< the pointer to the typedarray object */
{
  JJS_ASSERT (ecma_object_is_typedarray (context_p, typedarray_p));

  return ecma_typedarray_helper_get_shift_size (ecma_get_typedarray_id (context_p, typedarray_p));
} /* ecma_typedarray_get_element_size_shift */

/**
 * Get the array length of the typedarray object
 *
 * @return the array length
 */
uint32_t
ecma_typedarray_get_length (ecma_context_t *context_p, /**< JJS context */
                            ecma_object_t *typedarray_p) /**< the pointer to the typedarray object */
{
  JJS_ASSERT (ecma_object_is_typedarray (context_p, typedarray_p));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) typedarray_p;

  if (!(ext_object_p->u.cls.u2.typedarray_flags & ECMA_TYPEDARRAY_IS_EXTENDED))
  {
    ecma_object_t *arraybuffer_p = ecma_get_object_from_value (context_p, ext_object_p->u.cls.u3.arraybuffer);
    ecma_extended_object_t *arraybuffer_object_p = (ecma_extended_object_t *) arraybuffer_p;
    uint32_t buffer_length = arraybuffer_object_p->u.cls.u3.length;
    uint8_t shift = ecma_typedarray_get_element_size_shift (context_p, typedarray_p);

    return buffer_length >> shift;
  }

  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (context_p, typedarray_p);

  if (ecma_arraybuffer_is_detached (context_p, arraybuffer_p))
  {
    return 0;
  }

  ecma_extended_typedarray_object_t *info_p = (ecma_extended_typedarray_object_t *) ext_object_p;

  return info_p->array_length;
} /* ecma_typedarray_get_length */

/**
 * Get the offset of the internal arraybuffer
 *
 * @return the offset
 */
uint32_t
ecma_typedarray_get_offset (ecma_context_t *context_p, /**< JJS context */
                            ecma_object_t *typedarray_p) /**< the pointer to the typedarray object */
{
  JJS_ASSERT (ecma_object_is_typedarray (context_p, typedarray_p));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) typedarray_p;

  if (!(ext_object_p->u.cls.u2.typedarray_flags & ECMA_TYPEDARRAY_IS_EXTENDED))
  {
    return 0;
  }

  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (context_p, typedarray_p);

  if (ecma_arraybuffer_is_detached (context_p, arraybuffer_p))
  {
    return 0;
  }

  ecma_extended_typedarray_object_t *info_p = (ecma_extended_typedarray_object_t *) ext_object_p;

  return info_p->byte_offset;
} /* ecma_typedarray_get_offset */

/**
 * Utility function: return the pointer of the data buffer referenced by the typedarray info
 *
 * @return pointer to the data buffer if successfull,
 *         NULL otherwise
 */
extern inline uint8_t *
ecma_typedarray_get_buffer (ecma_context_t *context_p, /**< JJS context */
                            ecma_typedarray_info_t *info_p) /**< typedarray info */
{
  return ecma_arraybuffer_get_buffer (context_p, info_p->array_buffer_p) + info_p->offset;
} /* ecma_typedarray_get_buffer */

/**
 * Create a new typedarray object.
 *
 * The struct of the typedarray object
 *   ecma_object_t
 *   extend_part
 *   typedarray_info
 *
 * @return ecma value of the new typedarray object
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_typedarray (ecma_context_t *context_p, /**< JJS context */
                           const ecma_value_t *arguments_list_p, /**< the arg list passed to typedarray construct */
                           uint32_t arguments_list_len, /**< the length of the arguments_list_p */
                           ecma_object_t *proto_p, /**< prototype object */
                           uint8_t element_size_shift, /**< the size shift of the element length */
                           ecma_typedarray_type_t typedarray_id) /**< id of the typedarray */
{
  JJS_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  if (arguments_list_len == 0)
  {
    /* 22.2.1.1 */
    return ecma_typedarray_create_object_with_length (context_p, 0, NULL, proto_p, element_size_shift, typedarray_id);
  }

  if (!ecma_is_value_object (arguments_list_p[0]))
  {
    ecma_number_t num = 0;

    if (!ecma_is_value_undefined (arguments_list_p[0])
        && ECMA_IS_VALUE_ERROR (ecma_op_to_index (context_p, arguments_list_p[0], &num)))
    {
      return ECMA_VALUE_ERROR;
    }

    JJS_ASSERT (num >= 0 && num <= ECMA_NUMBER_MAX_SAFE_INTEGER);

    if (num > UINT32_MAX)
    {
#if JJS_ERROR_MESSAGES
      return ecma_raise_standard_error_with_format (context_p,
                                                    JJS_ERROR_RANGE,
                                                    "Invalid typed array length: %",
                                                    arguments_list_p[0]);
#else /* !JJS_ERROR_MESSAGES */
      return ecma_raise_range_error (context_p, ECMA_ERR_EMPTY);
#endif /* JJS_ERROR_MESSAGES */
    }

    return ecma_typedarray_create_object_with_length (context_p, (uint32_t) num, NULL, proto_p, element_size_shift, typedarray_id);
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (context_p, arguments_list_p[0]);

  if (ecma_object_is_typedarray (context_p, obj_p))
  {
    /* 22.2.1.3 */
    ecma_object_t *typedarray_p = obj_p;
    return ecma_typedarray_create_object_with_typedarray (context_p, typedarray_p, proto_p, element_size_shift, typedarray_id);
  }

  if (!ecma_object_class_is (obj_p, ECMA_OBJECT_CLASS_ARRAY_BUFFER) && !ecma_object_is_shared_arraybuffer (context_p, obj_p))
  {
    /* 22.2.1.4 */
    return ecma_typedarray_create_object_with_object (context_p, arguments_list_p[0], proto_p, element_size_shift, typedarray_id);
  }

  /* 22.2.1.5 */
  ecma_object_t *arraybuffer_p = obj_p;
  ecma_value_t byte_offset_value = ((arguments_list_len > 1) ? arguments_list_p[1] : ECMA_VALUE_UNDEFINED);

  ecma_value_t length_value = ((arguments_list_len > 2) ? arguments_list_p[2] : ECMA_VALUE_UNDEFINED);

  ecma_number_t offset;

  if (ECMA_IS_VALUE_ERROR (ecma_op_to_index (context_p, byte_offset_value, &offset)))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_number_is_negative (offset) || fmod (offset, (1 << element_size_shift)) != 0)
  {
    /* ES2015 22.2.1.5: 9 - 10. */
    if (ecma_number_is_zero (offset))
    {
      offset = 0;
    }
    else
    {
      return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_OFFSET);
    }
  }

  ecma_number_t new_length = 0;

  if (ECMA_IS_VALUE_ERROR (ecma_op_to_index (context_p, length_value, &new_length)))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, arraybuffer_p))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  if (offset > UINT32_MAX)
  {
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_LENGTH);
  }

  uint32_t byte_offset = (uint32_t) offset;

  uint32_t buf_byte_length = ecma_arraybuffer_get_length (context_p, arraybuffer_p);
  uint32_t new_byte_length = 0;

  if (ecma_is_value_undefined (length_value))
  {
    if ((buf_byte_length % (uint32_t) (1 << element_size_shift) != 0) || (buf_byte_length < byte_offset))
    {
      return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_LENGTH);
    }

    new_byte_length = (uint32_t) (buf_byte_length - byte_offset);
  }
  else
  {
    if (new_length > (UINT32_MAX >> element_size_shift))
    {
      return ecma_raise_range_error (context_p, ECMA_ERR_MAXIMUM_TYPEDARRAY_SIZE_IS_REACHED);
    }

    new_byte_length = (uint32_t) new_length << element_size_shift;

    if (byte_offset > buf_byte_length || new_byte_length > (buf_byte_length - byte_offset))
    {
      return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_LENGTH);
    }
  }

  bool needs_ext_typedarray_obj = (byte_offset != 0 || new_byte_length != buf_byte_length);

  size_t object_size =
    (needs_ext_typedarray_obj ? sizeof (ecma_extended_typedarray_object_t) : sizeof (ecma_extended_object_t));

  ecma_object_t *object_p = ecma_create_object (context_p, proto_p, object_size, ECMA_OBJECT_TYPE_CLASS);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.cls.type = ECMA_OBJECT_CLASS_TYPEDARRAY;
  ext_object_p->u.cls.u1.typedarray_type = (uint8_t) typedarray_id;
  ext_object_p->u.cls.u2.typedarray_flags = 0;
  ext_object_p->u.cls.u3.arraybuffer = ecma_make_object_value (context_p, arraybuffer_p);

  if (needs_ext_typedarray_obj)
  {
    ext_object_p->u.cls.u2.typedarray_flags |= ECMA_TYPEDARRAY_IS_EXTENDED;

    ecma_extended_typedarray_object_t *typedarray_info_p = (ecma_extended_typedarray_object_t *) object_p;
    typedarray_info_p->array_length = new_byte_length >> element_size_shift;
    typedarray_info_p->byte_offset = byte_offset;
  }

  return ecma_make_object_value (context_p, object_p);
} /* ecma_op_create_typedarray */

/**
 * Helper function for typedArray.prototype object's {'keys', 'values', 'entries', '@@iterator'}
 * routines common parts.
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.15
 *          ECMA-262 v6, 22.2.3.29
 *          ECMA-262 v6, 22.2.3.6
 *          ECMA-262 v6, 22.1.3.30
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator result object, if success
 *         error - otherwise
 */
ecma_value_t
ecma_typedarray_iterators_helper (ecma_context_t *context_p, /**< JJS context */
                                  ecma_value_t this_arg, /**< this argument */
                                  ecma_iterator_kind_t kind) /**< iterator kind */
{
  JJS_ASSERT (ecma_is_typedarray (context_p, this_arg));

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

  ecma_object_t *prototype_obj_p = ecma_builtin_get (context_p, ECMA_BUILTIN_ID_ARRAY_ITERATOR_PROTOTYPE);

  return ecma_op_create_iterator_object (context_p, this_arg, prototype_obj_p, ECMA_OBJECT_CLASS_ARRAY_ITERATOR, kind);
} /* ecma_typedarray_iterators_helper */

/**
 * Check if the object is typedarray
 *
 * @return true - if object is a TypedArray object
 *         false - otherwise
 */
bool
ecma_object_is_typedarray (ecma_context_t *context_p, /**< JJS context */
                           ecma_object_t *obj_p) /**< the target object need to be checked */
{
  JJS_UNUSED (context_p);
  JJS_ASSERT (!ecma_is_lexical_environment (obj_p));

  return ecma_object_class_is (obj_p, ECMA_OBJECT_CLASS_TYPEDARRAY);
} /* ecma_object_is_typedarray */

/**
 * Check if the value is typedarray
 *
 * @return true - if value is a TypedArray object
 *         false - otherwise
 */
bool
ecma_is_typedarray (ecma_context_t *context_p, /**< JJS context */
                    ecma_value_t value) /**< the target need to be checked */
{
  if (!ecma_is_value_object (value))
  {
    return false;
  }

  return ecma_object_is_typedarray (context_p, ecma_get_object_from_value (context_p, value));
} /* ecma_is_typedarray */

/**
 * Checks whether the property name is a valid element index
 *
 * @return true, if valid
 *         false, otherwise
 */
bool
ecma_typedarray_is_element_index (ecma_context_t *context_p, /**< JJS context */
                                  ecma_string_t *property_name_p) /**< property name */
{
  ecma_number_t num = ecma_string_to_number (context_p, property_name_p);

  if (num == 0)
  {
    return true;
  }

  ecma_string_t *num_to_str = ecma_new_ecma_string_from_number (context_p, num);
  bool is_same = ecma_compare_ecma_strings (property_name_p, num_to_str);
  ecma_deref_ecma_string (context_p, num_to_str);
  return is_same;
} /* ecma_typedarray_is_element_index */

/**
 * List names of a TypedArray object's integer indexed properties
 */
void
ecma_op_typedarray_list_lazy_property_names (ecma_context_t *context_p, /**< JJS context */
                                             ecma_object_t *obj_p, /**< a TypedArray object */
                                             ecma_collection_t *prop_names_p, /**< prop name collection */
                                             ecma_property_counter_t *prop_counter_p, /**< property counters */
                                             jjs_property_filter_t filter) /**< property name filter options */
{
  JJS_ASSERT (ecma_object_is_typedarray (context_p, obj_p));

  if (filter & JJS_PROPERTY_FILTER_EXCLUDE_INTEGER_INDICES)
  {
    return;
  }

  uint32_t array_length = ecma_typedarray_get_length (context_p, obj_p);

  for (uint32_t i = 0; i < array_length; i++)
  {
    ecma_string_t *name_p = ecma_new_ecma_string_from_uint32 (context_p, i);
    ecma_collection_push_back (context_p, prop_names_p, ecma_make_string_value (context_p, name_p));
  }

  prop_counter_p->array_index_named_props += array_length;
} /* ecma_op_typedarray_list_lazy_property_names */

/**
 * [[DefineOwnProperty]] operation for TypedArray objects
 *
 * See also: ES2015 9.4.5.3
 *
 * @return ECMA_VALUE_TRUE - if the property is successfully defined
 *         ECMA_VALUE_FALSE - if is JJS_PROP_SHOULD_THROW is not set
 *         raised TypeError - otherwise
 */
ecma_value_t
ecma_op_typedarray_define_own_property (ecma_context_t *context_p, /**< JJS context */
                                        ecma_object_t *obj_p, /**< TypedArray object */
                                        ecma_string_t *property_name_p, /**< property name */
                                        const ecma_property_descriptor_t *property_desc_p) /**< property descriptor */
{
  JJS_ASSERT (ecma_object_is_typedarray (context_p, obj_p));

  if (JJS_UNLIKELY (ecma_prop_name_is_symbol (property_name_p)))
  {
    return ecma_op_general_object_define_own_property (context_p, obj_p, property_name_p, property_desc_p);
  }

  uint32_t index = ecma_string_get_array_index (property_name_p);

  if (index == ECMA_STRING_NOT_ARRAY_INDEX && !ecma_typedarray_is_element_index (context_p, property_name_p))
  {
    return ecma_op_general_object_define_own_property (context_p, obj_p, property_name_p, property_desc_p);
  }

  if ((property_desc_p->flags & (JJS_PROP_IS_GET_DEFINED | JJS_PROP_IS_SET_DEFINED))
      || ((property_desc_p->flags & (JJS_PROP_IS_CONFIGURABLE_DEFINED | JJS_PROP_IS_CONFIGURABLE))
          == (JJS_PROP_IS_CONFIGURABLE_DEFINED | JJS_PROP_IS_CONFIGURABLE))
      || ((property_desc_p->flags & JJS_PROP_IS_ENUMERABLE_DEFINED)
          && !(property_desc_p->flags & JJS_PROP_IS_ENUMERABLE))
      || ((property_desc_p->flags & JJS_PROP_IS_WRITABLE_DEFINED)
          && !(property_desc_p->flags & JJS_PROP_IS_WRITABLE)))
  {
    return ecma_raise_property_redefinition (context_p, property_name_p, property_desc_p->flags);
  }

  ecma_typedarray_info_t info = ecma_typedarray_get_info (context_p, obj_p);

  if (index >= info.length || ecma_arraybuffer_is_detached (context_p, info.array_buffer_p))
  {
    return ECMA_VALUE_FALSE;
  }

  if (property_desc_p->flags & JJS_PROP_IS_VALUE_DEFINED)
  {
    ecma_value_t set_element = ecma_set_typedarray_element (context_p, &info, property_desc_p->value, index);

    if (ECMA_IS_VALUE_ERROR (set_element))
    {
      return set_element;
    }
  }

  return ECMA_VALUE_TRUE;
} /* ecma_op_typedarray_define_own_property */

/**
 * Specify the creation of a new TypedArray
 * object using a constructor function.
 *
 * See also: ES11 22.2.4.6
 *
 * Used by:
 *        - ecma_typedarray_species_create
 *
 * @return ecma_value_t function object from created from constructor_p argument
 */

ecma_value_t
ecma_typedarray_create (ecma_context_t *context_p, /**< JJS context */
                        ecma_object_t *constructor_p, /**< constructor function */
                        ecma_value_t *arguments_list_p, /**< argument list */
                        uint32_t arguments_list_len) /**< length of argument list */
{
  ecma_value_t ret_val =
    ecma_op_function_construct (context_p, constructor_p, constructor_p, arguments_list_p, arguments_list_len);
  if (ECMA_IS_VALUE_ERROR (ret_val))
  {
    return ret_val;
  }

  if (!ecma_is_typedarray (context_p, ret_val))
  {
    ecma_free_value (context_p, ret_val);
    return ecma_raise_type_error (context_p, ECMA_ERR_CONSTRUCTED_OBJECT_IS_NOT_TYPEDARRAY);
  }

  ecma_object_t *typedarray_p = ecma_get_object_from_value (context_p, ret_val);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (context_p, typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (context_p, info.array_buffer_p))
  {
    ecma_deref_object (typedarray_p);
    return ECMA_VALUE_ERROR;
  }

  if (ecma_arraybuffer_is_detached (context_p, info.array_buffer_p))
  {
    ecma_deref_object (typedarray_p);
    return ecma_raise_type_error (context_p, ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  if ((arguments_list_len == 1) && (ecma_is_value_number (arguments_list_p[0])))
  {
    ecma_number_t num = ecma_get_number_from_value (context_p, arguments_list_p[0]);

    if (info.length < num)
    {
      ecma_free_value (context_p, ret_val);
      return ecma_raise_type_error (context_p, ECMA_ERR_TYPEDARRAY_SMALLER_THAN_FILTER_CALL_RESULT);
    }
  }
  return ret_val;
} /* ecma_typedarray_create */

/* Specify the creation of a new TypedArray object
 * using a constructor function that is derived from this_arg.
 *
 * See also: ES11 22.2.4.7
 *
 * @return ecma value of the new typedarray object, constructed by default or species constructor
 */
ecma_value_t
ecma_typedarray_species_create (ecma_context_t *context_p, /**< JJS context */
                                ecma_value_t this_arg, /**< this argument */
                                ecma_value_t *arguments_list_p, /**< the arg list passed to typedarray construct */
                                uint32_t arguments_list_len) /**< length of the the arg list */
{
  ecma_object_t *typedarray_p = ecma_get_object_from_value (context_p, this_arg);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (context_p, typedarray_p);

  JJS_ASSERT (ecma_is_typedarray (context_p, this_arg));

  ecma_builtin_id_t default_constructor = ecma_typedarray_helper_get_constructor_id (info.id);

  ecma_value_t constructor = ecma_op_species_constructor (context_p, typedarray_p, default_constructor);

  if (ECMA_IS_VALUE_ERROR (constructor))
  {
    return constructor;
  }

  ecma_object_t *constructor_proto_p = ecma_get_object_from_value (context_p, constructor);

  ecma_value_t result = ecma_typedarray_create (context_p, constructor_proto_p, arguments_list_p, arguments_list_len);
  ecma_deref_object (constructor_proto_p);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    return result;
  }

#if JJS_BUILTIN_BIGINT
  ecma_object_t *result_p = ecma_get_object_from_value (context_p, result);
  ecma_typedarray_info_t result_info = ecma_typedarray_get_info (context_p, result_p);
  /*
   * Check result_info.id to to be either bigint type if info.id is one
   * or be neither of them is info.id is none of them as well.
   */
  if (ECMA_TYPEDARRAY_IS_BIGINT_TYPE (info.id) ^ ECMA_TYPEDARRAY_IS_BIGINT_TYPE (result_info.id))
  {
    ecma_free_value (context_p, result);
    return ecma_raise_type_error (context_p, ECMA_ERR_CONTENTTYPE_RETURNED_TYPEDARRAY_NOT_MATCH_SOURCE);
  }
#endif /* JJS_BUILTIN_BIGINT */

  return result;
} /* ecma_typedarray_species_create */

/**
 * Abstract Operation: TypedArrayCreateSameType ( exemplar, argumentList )
 *
 * See also:
 *   ECMA-262 v14, 23.2.4.3
 *
 * @param exemplar It is used to specify the creation of a new TypedArray using a
 *   constructor function that is derived from exemplar.
 * @param arguments_list_p arguments passed to new TypedArray constructor
 * @param arguments_list_len number of arguments in arguments_list_p
 * @return value of new TypedArray object or error object on failure. caller must free the value.
 */
ecma_value_t ecma_op_typedarray_create_same_type(ecma_context_t *context_p, /**< JJS context */
                                     ecma_value_t exemplar,
                                                 ecma_value_t *arguments_list_p,
                                                 uint32_t arguments_list_len) {
  ecma_object_t *exemplar_p = ecma_get_object_from_value (context_p, exemplar);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (context_p, exemplar_p);
  ecma_builtin_id_t c_id = ecma_typedarray_helper_get_constructor_id (info.id);

  ecma_object_t *ctor_p = ecma_builtin_get (context_p, c_id);

  ecma_value_t result = ecma_typedarray_create (context_p, ctor_p, arguments_list_p, arguments_list_len);

#if JJS_BUILTIN_BIGINT
  ecma_object_t *result_p = ecma_get_object_from_value (context_p, result);
  ecma_typedarray_info_t result_info = ecma_typedarray_get_info (context_p, result_p);
  /*
   * Check result_info.id to to be either bigint type if info.id is one
   * or be neither of them is info.id is none of them as well.
   */
  if (ECMA_TYPEDARRAY_IS_BIGINT_TYPE (info.id) ^ ECMA_TYPEDARRAY_IS_BIGINT_TYPE (result_info.id))
  {
    ecma_free_value (context_p, result);
    return ecma_raise_type_error (context_p, ECMA_ERR_CONTENTTYPE_RETURNED_TYPEDARRAY_NOT_MATCH_SOURCE);
  }
#endif /* JJS_BUILTIN_BIGINT */

  return result;
} /* ecma_op_typedarray_create_same_type */

/**
 * Create a typedarray object based on the "type" and arraylength
 * The "type" is same with arg1
 *
 * @return ecma_value_t
 */
ecma_value_t
ecma_op_create_typedarray_with_type_and_length (ecma_context_t *context_p, /**< JJS context */
                                                ecma_typedarray_type_t typedarray_id, /** TypedArray id  */
                                                uint32_t array_length) /**< length of the typedarray */
{
  // TODO: assert validate typedarray_id
  ecma_object_t *proto_p = ecma_builtin_get (context_p, ecma_typedarray_helper_get_prototype_id (typedarray_id));
  uint8_t element_size_shift = ecma_typedarray_helper_get_shift_size (typedarray_id);

  ecma_value_t new_obj =
    ecma_typedarray_create_object_with_length (context_p, array_length, NULL, proto_p, element_size_shift, typedarray_id);

  return new_obj;
} /* ecma_op_create_typedarray_with_type_and_length */

/**
 * Method for getting the additional typedArray informations.
 */
ecma_typedarray_info_t
ecma_typedarray_get_info (ecma_context_t *context_p, /**< JJS context */
                          ecma_object_t *typedarray_p)
{
  ecma_typedarray_info_t info;

  info.id = ecma_get_typedarray_id (context_p, typedarray_p);
  info.length = ecma_typedarray_get_length (context_p, typedarray_p);
  info.shift = ecma_typedarray_get_element_size_shift (context_p, typedarray_p);
  info.element_size = (uint8_t) (1 << info.shift);
  info.offset = ecma_typedarray_get_offset (context_p, typedarray_p);
  info.array_buffer_p = ecma_typedarray_get_arraybuffer (context_p, typedarray_p);

  return info;
} /* ecma_typedarray_get_info */

/**
 * @}
 * @}
 */
#endif /* JJS_BUILTIN_TYPEDARRAY */
