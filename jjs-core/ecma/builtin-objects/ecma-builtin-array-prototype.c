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
#include "ecma-comparison.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers-number.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"

#include "jcontext.h"
#include "jrt.h"
#include "lit-char-helpers.h"

#if JJS_BUILTIN_ARRAY

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
  ECMA_ARRAY_PROTOTYPE_ROUTINE_START = 0,
  /* Note: these 2 routine ids must be in this order */
  ECMA_ARRAY_PROTOTYPE_SORT,
  ECMA_ARRAY_PROTOTYPE_CONCAT,
  ECMA_ARRAY_PROTOTYPE_TO_LOCALE_STRING,
  ECMA_ARRAY_PROTOTYPE_JOIN,
  ECMA_ARRAY_PROTOTYPE_POP,
  ECMA_ARRAY_PROTOTYPE_PUSH,
  ECMA_ARRAY_PROTOTYPE_REVERSE,
  ECMA_ARRAY_PROTOTYPE_SHIFT,
  ECMA_ARRAY_PROTOTYPE_SLICE,
  ECMA_ARRAY_PROTOTYPE_SPLICE,
  ECMA_ARRAY_PROTOTYPE_UNSHIFT,
  ECMA_ARRAY_PROTOTYPE_AT,
  ECMA_ARRAY_PROTOTYPE_INDEX_OF,
  ECMA_ARRAY_PROTOTYPE_LAST_INDEX_OF,
  /* Note these 3 routines must be in this order */
  ECMA_ARRAY_PROTOTYPE_EVERY,
  ECMA_ARRAY_PROTOTYPE_SOME,
  ECMA_ARRAY_PROTOTYPE_FOR_EACH,
  ECMA_ARRAY_PROTOTYPE_MAP,
  ECMA_ARRAY_PROTOTYPE_FILTER,
  /* Note these 2 routines must be in this order */
  ECMA_ARRAY_PROTOTYPE_REDUCE,
  ECMA_ARRAY_PROTOTYPE_REDUCE_RIGHT,
  ECMA_ARRAY_PROTOTYPE_FIND,
  ECMA_ARRAY_PROTOTYPE_FIND_INDEX,
  ECMA_ARRAY_PROTOTYPE_ENTRIES,
  ECMA_ARRAY_PROTOTYPE_KEYS,
  ECMA_ARRAY_PROTOTYPE_SYMBOL_ITERATOR,
  ECMA_ARRAY_PROTOTYPE_FILL,
  ECMA_ARRAY_PROTOTYPE_COPY_WITHIN,
  ECMA_ARRAY_PROTOTYPE_INCLUDES,
  ECMA_ARRAY_PROTOTYPE_FLAT,
  ECMA_ARRAY_PROTOTYPE_FLATMAP,
  ECMA_ARRAY_PROTOTYPE_FIND_LAST,
  ECMA_ARRAY_PROTOTYPE_FIND_LAST_INDEX,
  ECMA_ARRAY_PROTOTYPE_WITH,
  ECMA_ARRAY_PROTOTYPE_TO_REVERSED,
  ECMA_ARRAY_PROTOTYPE_TO_SORTED,
  ECMA_ARRAY_PROTOTYPE_TO_SPLICED,
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-array-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID  array_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup arrayprototype ECMA Array.prototype object built-in
 * @{
 */

/**
 * Helper function to set an object's length property
 *
 * @return ecma value (return value of the [[Put]] method)
 *         Calling ecma_free_value on the returned value is optional if it is not abrupt completion.
 */
static ecma_value_t
ecma_builtin_array_prototype_helper_set_length (ecma_context_t *context_p, /**< JJS context */
                                                ecma_object_t *object, /**< object*/
                                                ecma_number_t length) /**< new length */
{
  ecma_value_t length_value = ecma_make_number_value (context_p, length);
  ecma_value_t ret_value =
    ecma_op_object_put (context_p, object, ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH), length_value, true);

  ecma_free_value (context_p, length_value);

  JJS_ASSERT (ecma_is_value_boolean (ret_value) || ecma_is_value_empty (ret_value)
                || ECMA_IS_VALUE_ERROR (ret_value));
  return ret_value;
} /* ecma_builtin_array_prototype_helper_set_length */

/**
 * The Array.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_to_locale_string (ecma_context_t *context_p, /**< JJS context */
                                                      ecma_object_t *obj_p, /**< object */
                                                      ecma_length_t length) /**< object's length */
{
  /* 5. */
  if (length == 0)
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  /* 7-8. */
  ecma_string_t *first_string_p = ecma_builtin_helper_get_to_locale_string_at_index (context_p, obj_p, 0);

  if (JJS_UNLIKELY (first_string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_stringbuilder_t builder = ecma_stringbuilder_create_from (context_p, first_string_p);
  ecma_deref_ecma_string (context_p, first_string_p);

  /* 9-10. */
  for (ecma_length_t k = 1; k < length; k++)
  {
    /* 4. Implementation-defined: set the separator to a single comma character. */
    ecma_stringbuilder_append_byte (&builder, LIT_CHAR_COMMA);

    ecma_string_t *next_string_p = ecma_builtin_helper_get_to_locale_string_at_index (context_p, obj_p, k);

    if (JJS_UNLIKELY (next_string_p == NULL))
    {
      ecma_stringbuilder_destroy (&builder);
      return ECMA_VALUE_ERROR;
    }

    ecma_stringbuilder_append (&builder, next_string_p);
    ecma_deref_ecma_string (context_p, next_string_p);
  }

  return ecma_make_string_value (context_p, ecma_stringbuilder_finalize (&builder));
} /* ecma_builtin_array_prototype_object_to_locale_string */

/**
 * The Array.prototype object's 'concat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_concat (ecma_context_t *context_p, /**< JJS context */
                                            const ecma_value_t args[], /**< arguments list */
                                            uint32_t args_number, /**< number of arguments */
                                            ecma_object_t *obj_p) /**< array object */
{
  /* 2. */
  ecma_object_t *new_array_p = ecma_op_array_species_create (context_p, obj_p, 0);

  if (JJS_UNLIKELY (new_array_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 3. */
  ecma_length_t new_length = 0;

  /* 5.b - 5.c for this_arg */
  ecma_value_t concat_this_value =
    ecma_builtin_helper_array_concat_value (context_p, new_array_p, &new_length, ecma_make_object_value (context_p, obj_p));
  if (ECMA_IS_VALUE_ERROR (concat_this_value))
  {
    ecma_deref_object (new_array_p);
    return concat_this_value;
  }

  JJS_ASSERT (ecma_is_value_empty (concat_this_value));

  /* 5. */
  for (uint32_t arg_index = 0; arg_index < args_number; arg_index++)
  {
    ecma_value_t concat_value = ecma_builtin_helper_array_concat_value (context_p, new_array_p, &new_length, args[arg_index]);

    if (ECMA_IS_VALUE_ERROR (concat_value))
    {
      ecma_deref_object (new_array_p);
      return concat_value;
    }

    JJS_ASSERT (ecma_is_value_empty (concat_value));
  }

  ecma_value_t set_length_value =
    ecma_builtin_array_prototype_helper_set_length (context_p, new_array_p, ((ecma_number_t) new_length));
  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    ecma_deref_object (new_array_p);
    return set_length_value;
  }

  return ecma_make_object_value (context_p, new_array_p);
} /* ecma_builtin_array_prototype_object_concat */

/**
 * The Array.prototype.toString's separator creation routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2 4th step
 *
 * @return NULL - if the conversion fails
 *         ecma_string_t * - otherwise
 */

static ecma_string_t *
ecma_op_array_get_separator_string (ecma_context_t *context_p, /**< JJS context */
                                    ecma_value_t separator) /**< possible separator */
{
  if (ecma_is_value_undefined (separator))
  {
    return ecma_get_magic_string (LIT_MAGIC_STRING_COMMA_CHAR);
  }

  return ecma_op_to_string (context_p, separator);
} /* ecma_op_array_get_separator_string */

/**
 * The Array.prototype's 'toString' single element operation routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2
 *
 * @return NULL - if the conversion fails
 *         ecma_string_t * - otherwise
 */
static ecma_string_t *
ecma_op_array_get_to_string_at_index (ecma_context_t *context_p, /**< JJS context */
                                      ecma_object_t *obj_p, /**< this object */
                                      ecma_length_t index) /**< array index */
{
  ecma_value_t index_value = ecma_op_object_get_by_index (context_p, obj_p, index);

  if (ECMA_IS_VALUE_ERROR (index_value))
  {
    return NULL;
  }

  if (ecma_is_value_undefined (index_value) || ecma_is_value_null (index_value))
  {
    return ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_string_t *ret_str_p = ecma_op_to_string (context_p, index_value);

  ecma_free_value (context_p, index_value);

  return ret_str_p;
} /* ecma_op_array_get_to_string_at_index */

/**
 * The Array.prototype object's 'join' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_join (ecma_context_t *context_p, /**< JJS context */
                                   ecma_value_t separator_arg, /**< separator argument */
                                   ecma_object_t *obj_p, /**< object */
                                   ecma_length_t length) /**< object's length */
{
  /* 4-5. */
  ecma_string_t *separator_string_p = ecma_op_array_get_separator_string (context_p, separator_arg);

  if (JJS_UNLIKELY (separator_string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  if (length == 0)
  {
    /* 6. */
    ecma_deref_ecma_string (context_p, separator_string_p);
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  /* 7-8. */
  ecma_string_t *first_string_p = ecma_op_array_get_to_string_at_index (context_p, obj_p, 0);

  if (JJS_UNLIKELY (first_string_p == NULL))
  {
    ecma_deref_ecma_string (context_p, separator_string_p);
    return ECMA_VALUE_ERROR;
  }

  ecma_stringbuilder_t builder = ecma_stringbuilder_create_from (context_p, first_string_p);
  ecma_deref_ecma_string (context_p, first_string_p);

  /* 9-10. */
  for (ecma_length_t k = 1; k < length; k++)
  {
    /* 10.a */
    ecma_stringbuilder_append (&builder, separator_string_p);

    /* 10.d */
    ecma_string_t *next_string_p = ecma_op_array_get_to_string_at_index (context_p, obj_p, k);

    if (JJS_UNLIKELY (next_string_p == NULL))
    {
      ecma_deref_ecma_string (context_p, separator_string_p);
      ecma_stringbuilder_destroy (&builder);
      return ECMA_VALUE_ERROR;
    }

    ecma_stringbuilder_append (&builder, next_string_p);
    ecma_deref_ecma_string (context_p, next_string_p);
  }

  ecma_deref_ecma_string (context_p, separator_string_p);
  return ecma_make_string_value (context_p, ecma_stringbuilder_finalize (&builder));
} /* ecma_builtin_array_prototype_join */

/**
 * The Array.prototype object's 'pop' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_pop (ecma_context_t *context_p, /**< JJS context */
                                         ecma_object_t *obj_p, /**< object */
                                         ecma_length_t len) /**< object's length */
{
  /* 4. */
  if (len == 0)
  {
    /* 4.a */
    ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (context_p, obj_p, ECMA_NUMBER_ZERO);

    /* 4.b */
    return ECMA_IS_VALUE_ERROR (set_length_value) ? set_length_value : ECMA_VALUE_UNDEFINED;
  }

  /* 5.b */
  len--;
  ecma_value_t get_value = ecma_op_object_get_by_index (context_p, obj_p, len);

  if (ECMA_IS_VALUE_ERROR (get_value))
  {
    return get_value;
  }

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_delete_fast_array_properties (context_p, obj_p, (uint32_t) len);

    return get_value;
  }

  /* 5.c */
  ecma_value_t del_value = ecma_op_object_delete_by_index (context_p, obj_p, len, true);

  if (ECMA_IS_VALUE_ERROR (del_value))
  {
    ecma_free_value (context_p, get_value);
    return del_value;
  }

  ecma_free_value (context_p, del_value);

  /* 5.d */
  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (context_p, obj_p, ((ecma_number_t) len));

  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    ecma_free_value (context_p, get_value);
    return set_length_value;
  }

  return get_value;
} /* ecma_builtin_array_prototype_object_pop */

/**
 * The Array.prototype object's 'push' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_push (ecma_context_t *context_p, /**< JJS context */
                                          const ecma_value_t *argument_list_p, /**< arguments list */
                                          uint32_t arguments_number, /**< number of arguments */
                                          ecma_object_t *obj_p, /**< object */
                                          ecma_length_t length) /**< object's length */
{
  if (ecma_op_object_is_fast_array (obj_p))
  {
    if ((ecma_number_t) (length + arguments_number) > UINT32_MAX)
    {
      return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_ARRAY_LENGTH);
    }

    if (arguments_number == 0)
    {
      return ecma_make_uint32_value (context_p, (uint32_t) length);
    }

    uint32_t new_length = ((uint32_t) length) + arguments_number;
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;
    ecma_value_t *buffer_p = ecma_fast_array_extend (context_p, obj_p, new_length) + length;

    for (uint32_t index = 0; index < arguments_number; index++)
    {
      buffer_p[index] = ecma_copy_value_if_not_object (context_p, argument_list_p[index]);
    }

    ext_obj_p->u.array.length_prop_and_hole_count -= ECMA_FAST_ARRAY_HOLE_ONE * arguments_number;

    return ecma_make_uint32_value (context_p, new_length);
  }

  /* 5. */
  if ((ecma_number_t) (length + arguments_number) > ECMA_NUMBER_MAX_SAFE_INTEGER)
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_PUSHING_TOO_HIGH_ELEMENT);
  }

  /* 6. */
  for (ecma_length_t index = 0; index < arguments_number; index++, length++)
  {
    /* 6.b */
    ecma_value_t put_value = ecma_op_object_put_by_index (context_p, obj_p, length, argument_list_p[index], true);

    if (ECMA_IS_VALUE_ERROR (put_value))
    {
      return put_value;
    }
  }

  /* 6 - 7. */
  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (context_p, obj_p, (ecma_number_t) length);

  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    return set_length_value;
  }

  return ecma_make_length_value (context_p, length);
} /* ecma_builtin_array_prototype_object_push */

/**
 * The Array.prototype object's 'reverse' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_reverse (ecma_context_t *context_p, /**< JJS context */
                                             ecma_value_t this_arg, /**< this argument */
                                             ecma_object_t *obj_p, /**< object */
                                             ecma_length_t len) /**< object's length */
{
  if (ecma_op_object_is_fast_array (obj_p))
  {
    uint32_t middle = (uint32_t) len / 2;
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.length_prop_and_hole_count < ECMA_FAST_ARRAY_HOLE_ONE && len != 0)
    {
      ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (context_p, ecma_value_t, obj_p->u1.property_list_cp);

      for (uint32_t i = 0; i < middle; i++)
      {
        ecma_value_t tmp = buffer_p[i];
        buffer_p[i] = buffer_p[len - 1 - i];
        buffer_p[len - 1 - i] = tmp;
      }

      return ecma_copy_value (context_p, this_arg);
    }
  }

  ecma_length_t middle = len / 2;
  for (ecma_length_t lower = 0; lower < middle; lower++)
  {
    ecma_length_t upper = len - lower - 1;
    ecma_value_t ret_value = ECMA_VALUE_ERROR;

    ecma_string_t *lower_str_p = ecma_new_ecma_string_from_length (context_p, lower);
    ecma_string_t *upper_str_p = ecma_new_ecma_string_from_length (context_p, upper);

    ecma_value_t lower_value = ECMA_VALUE_EMPTY;
    ecma_value_t upper_value = ECMA_VALUE_EMPTY;

    ecma_value_t has_lower = ecma_op_object_has_property (context_p, obj_p, lower_str_p);

#if JJS_BUILTIN_PROXY
    if (ECMA_IS_VALUE_ERROR (has_lower))
    {
      goto clean_up;
    }
#endif /* JJS_BUILTIN_PROXY */

    bool lower_exist = ecma_is_value_true (has_lower);

    if (lower_exist)
    {
      lower_value = ecma_op_object_get (context_p, obj_p, lower_str_p);

      if (ECMA_IS_VALUE_ERROR (lower_value))
      {
        goto clean_up;
      }
    }

    ecma_value_t has_upper = ecma_op_object_has_property (context_p, obj_p, upper_str_p);

#if JJS_BUILTIN_PROXY
    if (ECMA_IS_VALUE_ERROR (has_upper))
    {
      goto clean_up;
    }
#endif /* JJS_BUILTIN_PROXY */

    bool upper_exist = ecma_is_value_true (has_upper);

    if (upper_exist)
    {
      upper_value = ecma_op_object_get (context_p, obj_p, upper_str_p);

      if (ECMA_IS_VALUE_ERROR (upper_value))
      {
        goto clean_up;
      }
    }

    if (lower_exist && upper_exist)
    {
      ecma_value_t outer_put_value = ecma_op_object_put (context_p, obj_p, lower_str_p, upper_value, true);

      if (ECMA_IS_VALUE_ERROR (outer_put_value))
      {
        goto clean_up;
      }

      ecma_value_t inner_put_value = ecma_op_object_put (context_p, obj_p, upper_str_p, lower_value, true);

      if (ECMA_IS_VALUE_ERROR (inner_put_value))
      {
        goto clean_up;
      }
    }
    else if (!lower_exist && upper_exist)
    {
      ecma_value_t put_value = ecma_op_object_put (context_p, obj_p, lower_str_p, upper_value, true);

      if (ECMA_IS_VALUE_ERROR (put_value))
      {
        goto clean_up;
      }

      ecma_value_t del_value = ecma_op_object_delete (context_p, obj_p, upper_str_p, true);

      if (ECMA_IS_VALUE_ERROR (del_value))
      {
        goto clean_up;
      }
    }
    else if (lower_exist)
    {
      ecma_value_t del_value = ecma_op_object_delete (context_p, obj_p, lower_str_p, true);

      if (ECMA_IS_VALUE_ERROR (del_value))
      {
        goto clean_up;
      }

      ecma_value_t put_value = ecma_op_object_put (context_p, obj_p, upper_str_p, lower_value, true);

      if (ECMA_IS_VALUE_ERROR (put_value))
      {
        goto clean_up;
      }
    }

    ret_value = ECMA_VALUE_EMPTY;

clean_up:
    ecma_free_value (context_p, upper_value);
    ecma_free_value (context_p, lower_value);
    ecma_deref_ecma_string (context_p, lower_str_p);
    ecma_deref_ecma_string (context_p, upper_str_p);

    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      return ret_value;
    }
  }

  return ecma_copy_value (context_p, this_arg);
} /* ecma_builtin_array_prototype_object_reverse */

/**
 * The Array.prototype object's 'shift' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_shift (ecma_context_t *context_p, /**< JJS context */
                                           ecma_object_t *obj_p, /**< object */
                                           ecma_length_t len) /**< object's length */
{
  /* 4. */
  if (len == 0)
  {
    ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (context_p, obj_p, ECMA_NUMBER_ZERO);

    return ECMA_IS_VALUE_ERROR (set_length_value) ? set_length_value : ECMA_VALUE_UNDEFINED;
  }

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.length_prop_and_hole_count < ECMA_FAST_ARRAY_HOLE_ONE && len != 0)
    {
      ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (context_p, ecma_value_t, obj_p->u1.property_list_cp);
      ecma_value_t ret_value = buffer_p[0];

      if (ecma_is_value_object (ret_value))
      {
        ecma_ref_object (ecma_get_object_from_value (context_p, ret_value));
      }

      memmove (buffer_p, buffer_p + 1, (size_t) (sizeof (ecma_value_t) * (len - 1)));

      buffer_p[len - 1] = ECMA_VALUE_UNDEFINED;
      ecma_delete_fast_array_properties (context_p, obj_p, (uint32_t) (len - 1));

      return ret_value;
    }
  }

  /* 5. */
  ecma_value_t first_value = ecma_op_object_get_by_index (context_p, obj_p, 0);

  if (ECMA_IS_VALUE_ERROR (first_value))
  {
    return first_value;
  }

  /* 6. and 7. */
  for (ecma_length_t k = 1; k < len; k++)
  {
    /* 7.a - 7.c */
    ecma_value_t curr_value = ecma_op_object_find_by_index (context_p, obj_p, k);

    if (ECMA_IS_VALUE_ERROR (curr_value))
    {
      ecma_free_value (context_p, first_value);
      return curr_value;
    }

    /* 7.b */
    ecma_length_t to = k - 1;
    ecma_value_t operation_value;

    if (ecma_is_value_found (curr_value))
    {
      /* 7.d.i, 7.d.ii */
      operation_value = ecma_op_object_put_by_index (context_p, obj_p, to, curr_value, true);
      ecma_free_value (context_p, curr_value);
    }
    else
    {
      /* 7.e.i */
      operation_value = ecma_op_object_delete_by_index (context_p, obj_p, to, true);
    }

    if (ECMA_IS_VALUE_ERROR (operation_value))
    {
      ecma_free_value (context_p, first_value);
      return operation_value;
    }
  }

  /* 8. */
  ecma_value_t del_value = ecma_op_object_delete_by_index (context_p, obj_p, --len, true);

  if (ECMA_IS_VALUE_ERROR (del_value))
  {
    ecma_free_value (context_p, first_value);
    return del_value;
  }

  /* 9. */
  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (context_p, obj_p, ((ecma_number_t) len));

  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    ecma_free_value (context_p, first_value);
    return set_length_value;
  }

  /* 10. */
  return first_value;
} /* ecma_builtin_array_prototype_object_shift */

/**
 * The Array.prototype object's 'slice' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_slice (ecma_context_t *context_p, /**< JJS context */
                                           ecma_value_t arg1, /**< start */
                                           ecma_value_t arg2, /**< end */
                                           ecma_object_t *obj_p, /**< object */
                                           ecma_length_t len) /**< object's length */
{
  ecma_length_t start = 0, end = len;

  /* 5. 6.*/
  if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (context_p, arg1, len, &start)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 7. */
  if (ecma_is_value_undefined (arg2))
  {
    end = len;
  }
  else
  {
    /* 7. part 2, 8.*/
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (context_p, arg2, len, &end)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  JJS_ASSERT (start <= len && end <= len);

  bool use_fast_path = ecma_op_object_is_fast_array (obj_p);
  ecma_length_t copied_length = (end > start) ? end - start : 0;

  ecma_object_t *new_array_p = ecma_op_array_species_create (context_p, obj_p, copied_length);

  if (JJS_UNLIKELY (new_array_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  use_fast_path &= ecma_op_object_is_fast_array (new_array_p);

  if (use_fast_path && copied_length > 0)
  {
    ecma_extended_object_t *ext_from_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_from_obj_p->u.array.length_prop_and_hole_count < ECMA_FAST_ARRAY_HOLE_ONE)
    {
      if (JJS_UNLIKELY (obj_p->u1.property_list_cp == JMEM_CP_NULL))
      {
        /**
         * Very unlikely case: the buffer copied from is a fast buffer and the property list was deleted.
         * There is no need to do any copy.
         */
        return ecma_make_object_value (context_p, new_array_p);
      }

      /* Source array's length could be changed during the start/end normalization.
       * If the "end" value is greater than the current length, clamp the value to avoid buffer-overflow. */
      if (ext_from_obj_p->u.array.length < end)
      {
        end = ext_from_obj_p->u.array.length;
      }

      ecma_extended_object_t *ext_to_obj_p = (ecma_extended_object_t *) new_array_p;

      uint32_t target_length = ext_to_obj_p->u.array.length;
      ecma_value_t *to_buffer_p;
      JJS_ASSERT (copied_length <= UINT32_MAX);

      if (copied_length == target_length)
      {
        to_buffer_p = ECMA_GET_NON_NULL_POINTER (context_p, ecma_value_t, new_array_p->u1.property_list_cp);
      }
      else if (copied_length > target_length)
      {
        to_buffer_p = ecma_fast_array_extend (context_p, new_array_p, (uint32_t) copied_length);
      }
      else
      {
        ecma_delete_fast_array_properties (context_p, new_array_p, (uint32_t) copied_length);
        to_buffer_p = ECMA_GET_NON_NULL_POINTER (context_p, ecma_value_t, new_array_p->u1.property_list_cp);
      }

      ecma_value_t *from_buffer_p = ECMA_GET_NON_NULL_POINTER (context_p, ecma_value_t, obj_p->u1.property_list_cp);

      /* 9. */
      uint32_t n = 0;

      for (uint32_t k = (uint32_t) start; k < (uint32_t) end; k++, n++)
      {
        ecma_free_value_if_not_object (context_p, to_buffer_p[n]);
        to_buffer_p[n] = ecma_copy_value_if_not_object (context_p, from_buffer_p[k]);
      }

      ext_to_obj_p->u.array.length_prop_and_hole_count &= ECMA_FAST_ARRAY_HOLE_ONE - 1;

      return ecma_make_object_value (context_p, new_array_p);
    }
  }

  /* 9. */
  ecma_length_t n = 0;

  /* 10. */
  for (ecma_length_t k = start; k < end; k++, n++)
  {
    /* 10.c */
    ecma_value_t get_value = ecma_op_object_find_by_index (context_p, obj_p, k);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      ecma_deref_object (new_array_p);
      return get_value;
    }

    if (ecma_is_value_found (get_value))
    {
      /* 10.c.ii */
      ecma_value_t put_comp;
      put_comp = ecma_builtin_helper_def_prop_by_index (context_p,
                                                        new_array_p,
                                                        n,
                                                        get_value,
                                                        ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE
                                                          | JJS_PROP_SHOULD_THROW);
      ecma_free_value (context_p, get_value);

      if (ECMA_IS_VALUE_ERROR (put_comp))
      {
        ecma_deref_object (new_array_p);
        return put_comp;
      }
    }
  }

  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (context_p, new_array_p, ((ecma_number_t) n));

  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    ecma_deref_object (new_array_p);
    return set_length_value;
  }

  return ecma_make_object_value (context_p, new_array_p);
} /* ecma_builtin_array_prototype_object_slice */

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
ecma_builtin_array_prototype_object_sort_compare_helper (ecma_context_t *context_p, /**< JJS context */
                                                         ecma_value_t lhs, /**< left value */
                                                         ecma_value_t rhs, /**< right value */
                                                         ecma_value_t compare_func, /**< compare function */
                                                         ecma_object_t *array_buffer_p) /**< arrayBuffer */
{
  JJS_UNUSED (array_buffer_p);
  /*
   * ECMA-262 v5, 15.4.4.11 NOTE1: Because non-existent property values always
   * compare greater than undefined property values, and undefined always
   * compares greater than any other value, undefined property values always
   * sort to the end of the result, followed by non-existent property values.
   */
  bool lhs_is_undef = ecma_is_value_undefined (lhs);
  bool rhs_is_undef = ecma_is_value_undefined (rhs);

  if (lhs_is_undef)
  {
    return ecma_make_integer_value (rhs_is_undef ? 0 : 1);
  }

  if (rhs_is_undef)
  {
    return ecma_make_integer_value (-1);
  }

  ecma_number_t result = ECMA_NUMBER_ZERO;

  if (ecma_is_value_undefined (compare_func))
  {
    /* Default comparison when no compare_func is passed. */
    ecma_string_t *lhs_str_p = ecma_op_to_string (context_p, lhs);
    if (JJS_UNLIKELY (lhs_str_p == NULL))
    {
      return ECMA_VALUE_ERROR;
    }

    ecma_string_t *rhs_str_p = ecma_op_to_string (context_p, rhs);
    if (JJS_UNLIKELY (rhs_str_p == NULL))
    {
      ecma_deref_ecma_string (context_p, lhs_str_p);
      return ECMA_VALUE_ERROR;
    }

    if (ecma_compare_ecma_strings_relational (context_p, lhs_str_p, rhs_str_p))
    {
      result = ECMA_NUMBER_MINUS_ONE;
    }
    else if (!ecma_compare_ecma_strings (lhs_str_p, rhs_str_p))
    {
      result = ECMA_NUMBER_ONE;
    }
    else
    {
      result = ECMA_NUMBER_ZERO;
    }

    ecma_deref_ecma_string (context_p, rhs_str_p);
    ecma_deref_ecma_string (context_p, lhs_str_p);
  }
  else
  {
    /*
     * compare_func, if not undefined, will always contain a callable function object.
     * We checked this previously, before this function was called.
     */
    JJS_ASSERT (ecma_op_is_callable (context_p, compare_func));
    ecma_object_t *comparefn_obj_p = ecma_get_object_from_value (context_p, compare_func);

    ecma_value_t compare_args[] = { lhs, rhs };

    ecma_value_t call_value = ecma_op_function_call (context_p, comparefn_obj_p, ECMA_VALUE_UNDEFINED, compare_args, 2);
    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      return call_value;
    }

    if (!ecma_is_value_number (call_value))
    {
      ecma_number_t ret_num;

      if (ECMA_IS_VALUE_ERROR (ecma_op_to_number (context_p, call_value, &ret_num)))
      {
        ecma_free_value (context_p, call_value);
        return ECMA_VALUE_ERROR;
      }

      result = ret_num;
    }
    else
    {
      result = ecma_get_number_from_value (context_p, call_value);
    }

    ecma_free_value (context_p, call_value);
  }

  return ecma_make_number_value (context_p, result);
} /* ecma_builtin_array_prototype_object_sort_compare_helper */

/**
 * The Array.prototype object's 'sort' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_sort (ecma_context_t *context_p, /**< JJS context */
                                          ecma_value_t this_arg, /**< this argument */
                                          ecma_value_t arg1, /**< comparefn */
                                          ecma_object_t *obj_p) /**< object */
{
  /* Check if the provided compare function is callable. */
  if (!ecma_is_value_undefined (arg1) && !ecma_op_is_callable (context_p, arg1))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_COMPARE_FUNC_NOT_CALLABLE);
  }

  ecma_length_t len;
  ecma_value_t len_value = ecma_op_object_get_length (context_p, obj_p, &len);

  if (ECMA_IS_VALUE_ERROR (len_value))
  {
    return len_value;
  }
  ecma_collection_t *array_index_props_p = ecma_new_collection (context_p);

  for (uint32_t i = 0; i < len; i++)
  {
    ecma_string_t *prop_name_p = ecma_new_ecma_string_from_uint32 (context_p, i);

    ecma_property_descriptor_t prop_desc;
    ecma_value_t get_desc = ecma_op_object_get_own_property_descriptor (context_p, obj_p, prop_name_p, &prop_desc);

    if (ECMA_IS_VALUE_ERROR (get_desc))
    {
      ecma_collection_free (context_p, array_index_props_p);
      ecma_deref_ecma_string (context_p, prop_name_p);
      return get_desc;
    }

    if (ecma_is_value_true (get_desc))
    {
      ecma_ref_ecma_string (prop_name_p);
      ecma_collection_push_back (context_p, array_index_props_p, ecma_make_string_value (context_p, prop_name_p));
      ecma_free_property_descriptor (context_p, &prop_desc);
      continue;
    }
  }

  uint32_t defined_prop_count = array_index_props_p->item_count;

  ecma_value_t ret_value = ECMA_VALUE_ERROR;
  uint32_t copied_num = 0;
  JMEM_DEFINE_LOCAL_ARRAY (context_p, values_buffer, defined_prop_count, ecma_value_t);

  ecma_value_t *buffer_p = array_index_props_p->buffer_p;

  /* Copy unsorted array into a native c array. */
  for (uint32_t i = 0; i < array_index_props_p->item_count; i++)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (context_p, buffer_p[i]);

    uint32_t index = ecma_string_get_array_index (property_name_p);
    JJS_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);

    if (index >= len)
    {
      break;
    }

    ecma_value_t index_value = ecma_op_object_get (context_p, obj_p, property_name_p);

    if (ECMA_IS_VALUE_ERROR (index_value))
    {
      goto clean_up;
    }

    values_buffer[copied_num++] = index_value;
  }

  JJS_ASSERT (copied_num == defined_prop_count);

  /* Sorting. */
  if (copied_num > 1)
  {
    const ecma_builtin_helper_sort_compare_fn_t sort_cb = &ecma_builtin_array_prototype_object_sort_compare_helper;
    ecma_value_t sort_value =
      ecma_builtin_helper_array_merge_sort_helper (context_p, values_buffer, (uint32_t) (copied_num), arg1, sort_cb, NULL);
    if (ECMA_IS_VALUE_ERROR (sort_value))
    {
      goto clean_up;
    }

    ecma_free_value (context_p, sort_value);
  }

  /* Put sorted values to the front of the array. */
  for (uint32_t index = 0; index < copied_num; index++)
  {
    ecma_value_t put_value = ecma_op_object_put_by_index (context_p, obj_p, index, values_buffer[index], true);

    if (ECMA_IS_VALUE_ERROR (put_value))
    {
      goto clean_up;
    }
  }

  ret_value = ECMA_VALUE_EMPTY;

clean_up:
  /* Free values that were copied to the local array. */
  for (uint32_t index = 0; index < copied_num; index++)
  {
    ecma_free_value (context_p, values_buffer[index]);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (context_p, values_buffer);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_collection_free (context_p, array_index_props_p);
    return ret_value;
  }

  JJS_ASSERT (ecma_is_value_empty (ret_value));

  /* Undefined properties should be in the back of the array. */
  ecma_value_t *buffer_p = array_index_props_p->buffer_p;

  for (uint32_t i = 0; i < array_index_props_p->item_count; i++)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (context_p, buffer_p[i]);

    uint32_t index = ecma_string_get_array_index (property_name_p);
    JJS_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);

    if (index >= copied_num && index < len)
    {
      ecma_value_t del_value = ecma_op_object_delete (context_p, obj_p, property_name_p, true);

      if (ECMA_IS_VALUE_ERROR (del_value))
      {
        ecma_collection_free (context_p, array_index_props_p);
        return del_value;
      }
    }
  }

  ecma_collection_free (context_p, array_index_props_p);

  return ecma_copy_value (context_p, this_arg);
} /* ecma_builtin_array_prototype_object_sort */

/**
 * The Array.prototype object's 'splice' routine
 *
 * See also:
 *          ECMA-262 v11, 22.1.3.28
 *          ECMA-262 v5, 15.4.4.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_splice (ecma_context_t *context_p, /**< JJS context */
                                            const ecma_value_t args[], /**< arguments list */
                                            uint32_t args_number, /**< number of arguments */
                                            ecma_object_t *obj_p, /**< object */
                                            ecma_length_t len) /**< object's length */
{
  ecma_length_t actual_start = 0;
  ecma_length_t actual_delete_count = 0;
  ecma_length_t insert_count = 0;

  if (args_number > 0)
  {
    /* ES5.1: 6, ES11: 4. */
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (context_p, args[0], len, &actual_start)))
    {
      return ECMA_VALUE_ERROR;
    }

    /* ES11: 6. */
    if (args_number == 1)
    {
      actual_delete_count = len - actual_start;
    }
    /* ES11: 7. */
    else
    {
      insert_count = args_number - 2;

      ecma_number_t delete_num;
      if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, args[1], &delete_num)))
      {
        return ECMA_VALUE_ERROR;
      }

      /* ES5.1: 7 */
      actual_delete_count =
        (ecma_length_t) (JJS_MIN (JJS_MAX (delete_num, 0), (ecma_number_t) (len - actual_start)));
    }
  }

  ecma_length_t new_length = len + insert_count - actual_delete_count;

  /* ES11: 8. */
  if ((ecma_number_t) new_length > ECMA_NUMBER_MAX_SAFE_INTEGER)
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_INVALID_NEW_ARRAY_LENGTH);
  }

  /* ES11: 9. */
  ecma_object_t *new_array_p = ecma_op_array_species_create (context_p, obj_p, actual_delete_count);

  if (JJS_UNLIKELY (new_array_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  /* ES5.1: 8, ES11: 10. */
  ecma_length_t k = 0;

  /* ES5.1: 9, ES11: 11. */
  for (; k < actual_delete_count; k++)
  {
    ecma_length_t from = actual_start + k;
    ecma_value_t from_present = ecma_op_object_find_by_index (context_p, obj_p, from);

    if (ECMA_IS_VALUE_ERROR (from_present))
    {
      ecma_deref_object (new_array_p);
      return from_present;
    }

    if (ecma_is_value_found (from_present))
    {
      ecma_value_t put_comp = ecma_builtin_helper_def_prop_by_index (context_p,
                                                                     new_array_p,
                                                                     k,
                                                                     from_present,
                                                                     ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE
                                                                       | JJS_PROP_SHOULD_THROW);
      ecma_free_value (context_p, from_present);

      if (ECMA_IS_VALUE_ERROR (put_comp))
      {
        ecma_deref_object (new_array_p);
        return put_comp;
      }
    }
  }

  /* ES11: 12. */
  ecma_value_t set_length =
    ecma_builtin_array_prototype_helper_set_length (context_p, new_array_p, ((ecma_number_t) actual_delete_count));

  if (ECMA_IS_VALUE_ERROR (set_length))
  {
    ecma_deref_object (new_array_p);
    return set_length;
  }

  /* ES5.1: 12, ES11: 15. */
  if (insert_count < actual_delete_count)
  {
    for (k = actual_start; k < len - actual_delete_count; k++)
    {
      ecma_length_t from = k + actual_delete_count;
      ecma_length_t to = k + insert_count;

      ecma_value_t from_present = ecma_op_object_find_by_index (context_p, obj_p, from);

      if (ECMA_IS_VALUE_ERROR (from_present))
      {
        ecma_deref_object (new_array_p);
        return from_present;
      }

      ecma_value_t operation_value;

      if (ecma_is_value_found (from_present))
      {
        operation_value = ecma_op_object_put_by_index (context_p, obj_p, to, from_present, true);
        ecma_free_value (context_p, from_present);
      }
      else
      {
        operation_value = ecma_op_object_delete_by_index (context_p, obj_p, to, true);
      }

      if (ECMA_IS_VALUE_ERROR (operation_value))
      {
        ecma_deref_object (new_array_p);
        return operation_value;
      }
    }

    k = len;

    for (k = len; k > new_length; k--)
    {
      ecma_value_t del_value = ecma_op_object_delete_by_index (context_p, obj_p, k - 1, true);

      if (ECMA_IS_VALUE_ERROR (del_value))
      {
        ecma_deref_object (new_array_p);
        return del_value;
      }
    }
  }
  /* ES5.1: 13, ES11: 16. */
  else if (insert_count > actual_delete_count)
  {
    for (k = len - actual_delete_count; k > actual_start; k--)
    {
      ecma_length_t from = k + actual_delete_count - 1;
      ecma_length_t to = k + insert_count - 1;

      ecma_value_t from_present = ecma_op_object_find_by_index (context_p, obj_p, from);

      if (ECMA_IS_VALUE_ERROR (from_present))
      {
        ecma_deref_object (new_array_p);
        return from_present;
      }

      ecma_value_t operation_value;

      if (ecma_is_value_found (from_present))
      {
        operation_value = ecma_op_object_put_by_index (context_p, obj_p, to, from_present, true);
        ecma_free_value (context_p, from_present);
      }
      else
      {
        operation_value = ecma_op_object_delete_by_index (context_p, obj_p, to, true);
      }

      if (ECMA_IS_VALUE_ERROR (operation_value))
      {
        ecma_deref_object (new_array_p);
        return operation_value;
      }
    }
  }

  /* ES5.1: 14, ES11: 17. */
  k = actual_start;

  /* ES5.1: 15, ES11: 18. */
  uint32_t idx = 0;
  for (uint32_t arg_index = 2; arg_index < args_number; arg_index++, idx++)
  {
    ecma_value_t put_value = ecma_op_object_put_by_index (context_p, obj_p, actual_start + idx, args[arg_index], true);

    if (ECMA_IS_VALUE_ERROR (put_value))
    {
      ecma_deref_object (new_array_p);
      return put_value;
    }
  }

  /* ES5.1: 16, ES11: 19. */
  ecma_value_t set_new_length = ecma_builtin_array_prototype_helper_set_length (context_p, obj_p, ((ecma_number_t) new_length));

  if (ECMA_IS_VALUE_ERROR (set_new_length))
  {
    ecma_deref_object (new_array_p);
    return set_new_length;
  }

  /* ES5.1: 17, ES11: 20. */
  return ecma_make_object_value (context_p, new_array_p);
} /* ecma_builtin_array_prototype_object_splice */

/**
 * The Array.prototype object's 'unshift' routine
 *
 * See also:
 *          ECMA-262  v5, 15.4.4.13
 *          ECMA-262 v11, 22.1.3.31
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_unshift (ecma_context_t *context_p, /**< JJS context */
                                             const ecma_value_t args[], /**< arguments list */
                                             uint32_t args_number, /**< number of arguments */
                                             ecma_object_t *obj_p, /**< object */
                                             ecma_length_t len) /**< object's length */
{
  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.length_prop_and_hole_count < ECMA_FAST_ARRAY_HOLE_ONE && len != 0)
    {
      if (args_number > UINT32_MAX - len)
      {
        return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_ARRAY_LENGTH);
      }

      if (args_number == 0)
      {
        return ecma_make_uint32_value (context_p, (uint32_t) len);
      }

      uint32_t new_length = ((uint32_t) len) + args_number;
      ecma_value_t *buffer_p = ecma_fast_array_extend (context_p, obj_p, new_length);
      memmove (buffer_p + args_number, buffer_p, (size_t) (sizeof (ecma_value_t) * len));

      uint32_t index = 0;

      while (index < args_number)
      {
        buffer_p[index] = ecma_copy_value_if_not_object (context_p, args[index]);
        index++;
      }

      ext_obj_p->u.array.length_prop_and_hole_count -= args_number * ECMA_FAST_ARRAY_HOLE_ONE;

      return ecma_make_uint32_value (context_p, new_length);
    }
  }

  if (args_number > 0)
  {
    /* ES11:4.a. */
    if ((ecma_number_t) (len + args_number) > ECMA_NUMBER_MAX_SAFE_INTEGER)
    {
      return ecma_raise_type_error (context_p, ECMA_ERR_UNSHIFT_TOO_HIGH);
    }

    /* ES5.1:5.,6. ES11: 4.b, 4.c */
    for (ecma_length_t k = len; k > 0; k--)
    {
      /* ES5.1:6.a, 6.c, ES11:4.c.i., 4.c.iii.  */
      ecma_value_t get_value = ecma_op_object_find_by_index (context_p, obj_p, k - 1);

      if (ECMA_IS_VALUE_ERROR (get_value))
      {
        return get_value;
      }

      /* ES5.1:6.b, ES11:4.c.ii. */
      ecma_number_t new_idx = ((ecma_number_t) k) + ((ecma_number_t) args_number) - 1;
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_number (context_p, new_idx);
      ecma_value_t operation_value;

      if (ecma_is_value_found (get_value))
      {
        /* ES5.1:6.d.i, 6.d.ii, ES11:4.c.iv. */
        operation_value = ecma_op_object_put (context_p, obj_p, index_str_p, get_value, true);
        ecma_free_value (context_p, get_value);
      }
      else
      {
        /* ES5.1:6.e.i, ES11:4.c.v. */
        operation_value = ecma_op_object_delete (context_p, obj_p, index_str_p, true);
      }

      ecma_deref_ecma_string (context_p, index_str_p);

      if (ECMA_IS_VALUE_ERROR (operation_value))
      {
        return operation_value;
      }
    }

    for (uint32_t arg_index = 0; arg_index < args_number; arg_index++)
    {
      /* ES5.1:9.b, ES11:4.f.ii.  */
      ecma_value_t put_value = ecma_op_object_put_by_index (context_p, obj_p, arg_index, args[arg_index], true);

      if (ECMA_IS_VALUE_ERROR (put_value))
      {
        return put_value;
      }
    }
  }

  /* ES5.1:10., ES11:5. */
  ecma_number_t new_len = ((ecma_number_t) len) + ((ecma_number_t) args_number);
  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (context_p, obj_p, new_len);

  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    return set_length_value;
  }

  return ecma_make_number_value (context_p, new_len);
} /* ecma_builtin_array_prototype_object_unshift */

/**
 * The Array.prototype object's 'at' routine
 *
 * See also:
 *          ECMA-262 Stage 3 Draft Relative Indexing Method proposal
 *          from: https://tc39.es/proposal-relative-indexing-method
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_at (ecma_context_t *context_p, /**< JJS context */
                                        const ecma_value_t index, /**< index argument */
                                        ecma_object_t *obj_p, /**< object */
                                        ecma_length_t len) /**< object's length */
{
  ecma_length_t res_index;
  ecma_value_t return_value = ecma_builtin_helper_calculate_index (context_p, index, len, &res_index);

  if (return_value != ECMA_VALUE_EMPTY)
  {
    return return_value;
  }

  return ecma_op_object_get_by_index (context_p, obj_p, res_index);
} /* ecma_builtin_array_prototype_object_at */

/**
 * The Array.prototype object's 'indexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.14
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_index_of (ecma_context_t *context_p, /**< JJS context */
                                              const ecma_value_t args[], /**< arguments list */
                                              uint32_t args_number, /**< number of arguments */
                                              ecma_object_t *obj_p, /**< object */
                                              ecma_length_t len) /**< object's length */
{
  /* 4. */
  if (len == 0)
  {
    return ecma_make_integer_value (-1);
  }

  /* 5. */
  ecma_number_t idx = 0;
  if (args_number > 1)
  {
    if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, args[1], &idx)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  /* 6. */
  if (idx >= (ecma_number_t) len)
  {
    return ecma_make_number_value (context_p, -1);
  }

  /* 7. */
  ecma_length_t from_idx = (ecma_length_t) idx;

  /* 8. */
  if (idx < 0)
  {
    from_idx = (ecma_length_t) JJS_MAX ((ecma_number_t) len + idx, 0);
  }

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.length_prop_and_hole_count < ECMA_FAST_ARRAY_HOLE_ONE)
    {
      if (JJS_UNLIKELY (obj_p->u1.property_list_cp == JMEM_CP_NULL))
      {
        return ecma_make_integer_value (-1);
      }

      len = JJS_MIN (ext_obj_p->u.array.length, len);

      ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (context_p, ecma_value_t, obj_p->u1.property_list_cp);

      while (from_idx < len)
      {
        if (ecma_op_strict_equality_compare (context_p, args[0], buffer_p[from_idx]))
        {
          return ecma_make_uint32_value (context_p, (uint32_t) from_idx);
        }

        from_idx++;
      }

      return ecma_make_integer_value (-1);
    }
  }

  /* 6. */
  while (from_idx < len)
  {
    /* 9.a */
    ecma_value_t get_value = ecma_op_object_find_by_index (context_p, obj_p, from_idx);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    /* 9.b.i, 9.b.ii */
    if (ecma_is_value_found (get_value) && ecma_op_strict_equality_compare (context_p, args[0], get_value))
    {
      ecma_free_value (context_p, get_value);
      return ecma_make_length_value (context_p, from_idx);
    }

    from_idx++;

    ecma_free_value (context_p, get_value);
  }

  return ecma_make_integer_value (-1);
} /* ecma_builtin_array_prototype_object_index_of */

/**
 * The Array.prototype object's 'lastIndexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.15
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_last_index_of (ecma_context_t *context_p, /**< JJS context */
                                                   const ecma_value_t args[], /**< arguments list */
                                                   uint32_t args_number, /**< number of arguments */
                                                   ecma_object_t *obj_p, /**< object */
                                                   ecma_length_t len) /**< object's length */
{
  /* 4. */
  if (len == 0)
  {
    return ecma_make_integer_value (-1);
  }

  /* 5. */
  ecma_number_t idx = (ecma_number_t) len - 1;
  if (args_number > 1)
  {
    if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, args[1], &idx)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  ecma_length_t from_idx;

  /* 6 */
  if (idx >= 0)
  {
    from_idx = (ecma_length_t) (JJS_MIN (idx, (ecma_number_t) (len - 1)));
  }
  else
  {
    ecma_number_t k = (ecma_number_t) len + idx;
    if (k < 0)
    {
      return ecma_make_integer_value (-1);
    }
    from_idx = (ecma_length_t) k;
  }

  ecma_value_t search_element = (args_number > 0) ? args[0] : ECMA_VALUE_UNDEFINED;

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.length_prop_and_hole_count < ECMA_FAST_ARRAY_HOLE_ONE)
    {
      if (JJS_UNLIKELY (obj_p->u1.property_list_cp == JMEM_CP_NULL))
      {
        return ecma_make_integer_value (-1);
      }

      len = JJS_MIN (ext_obj_p->u.array.length, len);

      ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (context_p, ecma_value_t, obj_p->u1.property_list_cp);

      while (from_idx < len)
      {
        if (ecma_op_strict_equality_compare (context_p, search_element, buffer_p[from_idx]))
        {
          return ecma_make_uint32_value (context_p, (uint32_t) from_idx);
        }
        from_idx--;
      }
      return ecma_make_integer_value (-1);
    }
  }

  /* 8. */
  while (from_idx < len)
  {
    /* 8.a */
    ecma_value_t get_value = ecma_op_object_find_by_index (context_p, obj_p, from_idx);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    /* 8.b.i, 8.b.ii */
    if (ecma_is_value_found (get_value) && ecma_op_strict_equality_compare (context_p, search_element, get_value))
    {
      ecma_free_value (context_p, get_value);
      return ecma_make_length_value (context_p, from_idx);
    }

    from_idx--;

    ecma_free_value (context_p, get_value);
  }

  return ecma_make_integer_value (-1);
} /* ecma_builtin_array_prototype_object_last_index_of */

/**
 * Type of array routine.
 */
typedef enum
{
  ARRAY_ROUTINE_EVERY, /**< Array.every: ECMA-262 v5, 15.4.4.16 */
  ARRAY_ROUTINE_SOME, /**< Array.some: ECMA-262 v5, 15.4.4.17 */
  ARRAY_ROUTINE_FOREACH, /**< Array.forEach: ECMA-262 v5, 15.4.4.18 */
  ARRAY_ROUTINE__COUNT /**< count of the modes */
} array_routine_mode;

/**
 * Applies the provided function to each element of the array as long as
 * the return value stays empty. The common function for 'every', 'some'
 * and 'forEach' of the Array prototype.
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.16
 *          ECMA-262 v5, 15.4.4.17
 *          ECMA-262 v5, 15.4.4.18
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_apply (ecma_context_t *context_p, /**< JJS context */
                          ecma_value_t arg1, /**< callbackfn */
                          ecma_value_t arg2, /**< thisArg */
                          array_routine_mode mode, /**< array routine mode */
                          ecma_object_t *obj_p, /**< object */
                          ecma_length_t len) /**< object's length */

{
  JJS_ASSERT (mode < ARRAY_ROUTINE__COUNT);

  /* 4. */
  if (!ecma_op_is_callable (context_p, arg1))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_CALLBACK_IS_NOT_CALLABLE);
  }

  /* We already checked that arg1 is callable */
  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, arg1);
  ecma_value_t current_index;

  /* 7. */
  for (ecma_length_t index = 0; index < len; index++)
  {
    /* 7.a - 7.c */
    ecma_value_t get_value = ecma_op_object_find_by_index (context_p, obj_p, index);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    if (ecma_is_value_found (get_value))
    {
      /* 7.c.i */
      current_index = ecma_make_length_value (context_p, index);

      ecma_value_t call_args[] = { get_value, current_index, ecma_make_object_value (context_p, obj_p) };
      /* 7.c.ii */
      ecma_value_t call_value = ecma_op_function_call (context_p, func_object_p, arg2, call_args, 3);

      if (ECMA_IS_VALUE_ERROR (call_value))
      {
        ecma_free_value (context_p, get_value);
        return call_value;
      }

      bool to_boolean = ecma_op_to_boolean (context_p, call_value);

      ecma_free_value (context_p, call_value);
      ecma_free_value (context_p, get_value);

      /* 7.c.iii */
      if (mode == ARRAY_ROUTINE_EVERY && !to_boolean)
      {
        return ECMA_VALUE_FALSE;
      }
      else if (mode == ARRAY_ROUTINE_SOME && to_boolean)
      {
        return ECMA_VALUE_TRUE;
      }
    }
  }

  /* 8. */

  if (mode == ARRAY_ROUTINE_EVERY)
  {
    return ECMA_VALUE_TRUE;
  }
  else if (mode == ARRAY_ROUTINE_SOME)
  {
    return ECMA_VALUE_FALSE;
  }

  JJS_ASSERT (mode == ARRAY_ROUTINE_FOREACH);
  return ECMA_VALUE_UNDEFINED;
} /* ecma_builtin_array_apply */

/**
 * The Array.prototype object's 'map' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.19
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_map (ecma_context_t *context_p, /**< JJS context */
                                         ecma_value_t arg1, /**< callbackfn */
                                         ecma_value_t arg2, /**< thisArg */
                                         ecma_object_t *obj_p, /**< object */
                                         ecma_length_t len) /**< object's length */
{
  /* 4. */
  if (!ecma_op_is_callable (context_p, arg1))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_CALLBACK_IS_NOT_CALLABLE);
  }

  /* 6. */
  ecma_object_t *new_array_p = ecma_op_array_species_create (context_p, obj_p, len);

  if (JJS_UNLIKELY (new_array_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  JJS_ASSERT (ecma_is_value_object (arg1));
  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, arg1);

  /* 7-8. */
  ecma_value_t current_index;

  for (ecma_length_t index = 0; index < len; index++)
  {
    /* 8.a - 8.b */
    ecma_value_t current_value = ecma_op_object_find_by_index (context_p, obj_p, index);

    if (ECMA_IS_VALUE_ERROR (current_value))
    {
      ecma_deref_object (new_array_p);
      return current_value;
    }

    if (ecma_is_value_found (current_value))
    {
      /* 8.c.i, 8.c.ii */
      current_index = ecma_make_length_value (context_p, index);
      ecma_value_t call_args[] = { current_value, current_index, ecma_make_object_value (context_p, obj_p) };

      ecma_value_t mapped_value = ecma_op_function_call (context_p, func_object_p, arg2, call_args, 3);

      if (ECMA_IS_VALUE_ERROR (mapped_value))
      {
        ecma_free_value (context_p, current_value);
        ecma_deref_object (new_array_p);
        return mapped_value;
      }

      /* 8.c.iii */
      ecma_value_t put_comp;
      put_comp = ecma_builtin_helper_def_prop_by_index (context_p,
                                                        new_array_p,
                                                        index,
                                                        mapped_value,
                                                        ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE
                                                          | JJS_PROP_SHOULD_THROW);

      ecma_free_value (context_p, mapped_value);
      ecma_free_value (context_p, current_value);

      if (ECMA_IS_VALUE_ERROR (put_comp))
      {
        ecma_deref_object (new_array_p);
        return put_comp;
      }
    }
  }

  return ecma_make_object_value (context_p, new_array_p);
} /* ecma_builtin_array_prototype_object_map */

/**
 * The Array.prototype object's 'filter' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.20
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_filter (ecma_context_t *context_p, /**< JJS context */
                                            ecma_value_t arg1, /**< callbackfn */
                                            ecma_value_t arg2, /**< thisArg */
                                            ecma_object_t *obj_p, /**< object */
                                            ecma_length_t len) /**< object's length */
{
  /* 4. */
  if (!ecma_op_is_callable (context_p, arg1))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_CALLBACK_IS_NOT_CALLABLE);
  }

  /* 6. */
  ecma_object_t *new_array_p = ecma_op_array_species_create (context_p, obj_p, 0);

  if (JJS_UNLIKELY (new_array_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  /* ES11: 22.1.3.7. 7.c.iii.1 */
  const uint32_t prop_flags = ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE | JJS_PROP_SHOULD_THROW;

  /* We already checked that arg1 is callable, so it will always be an object. */
  JJS_ASSERT (ecma_is_value_object (arg1));
  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, arg1);

  /* 8. */
  ecma_length_t new_array_index = 0;
  ecma_value_t current_index;

  /* 9. */
  for (ecma_length_t index = 0; index < len; index++)
  {
    /* 9.a - 9.c */
    ecma_value_t get_value = ecma_op_object_find_by_index (context_p, obj_p, index);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      ecma_deref_object (new_array_p);
      return get_value;
    }

    if (ecma_is_value_found (get_value))
    {
      /* 9.c.i */
      current_index = ecma_make_length_value (context_p, index);

      ecma_value_t call_args[] = { get_value, current_index, ecma_make_object_value (context_p, obj_p) };
      /* 9.c.ii */
      ecma_value_t call_value = ecma_op_function_call (context_p, func_object_p, arg2, call_args, 3);

      if (ECMA_IS_VALUE_ERROR (call_value))
      {
        ecma_free_value (context_p, get_value);
        ecma_deref_object (new_array_p);
        return call_value;
      }

      /* 9.c.iii */
      if (ecma_op_to_boolean (context_p, call_value))
      {
        ecma_value_t put_comp;
        put_comp = ecma_builtin_helper_def_prop_by_index (context_p, new_array_p, new_array_index, get_value, prop_flags);

        if (ECMA_IS_VALUE_ERROR (put_comp))
        {
          ecma_free_value (context_p, call_value);
          ecma_free_value (context_p, get_value);
          ecma_deref_object (new_array_p);

          return put_comp;
        }

        new_array_index++;
      }

      ecma_free_value (context_p, call_value);
      ecma_free_value (context_p, get_value);
    }
  }

  return ecma_make_object_value (context_p, new_array_p);
} /* ecma_builtin_array_prototype_object_filter */

/**
 * The Array.prototype object's 'reduce' and 'reduceRight' routine
 *
 * See also:
 *         ECMA-262 v5, 15.4.4.21
 *         ECMA-262 v5, 15.4.4.22
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_reduce_from (ecma_context_t *context_p, /**< JJS context */
                                const ecma_value_t args_p[], /**< routine's arguments */
                                uint32_t args_number, /**< arguments list length */
                                bool start_from_left, /**< whether the reduce starts from left or right */
                                ecma_object_t *obj_p, /**< object */
                                ecma_length_t len) /**< object's length */
{
  /* 4. */
  if (!ecma_op_is_callable (context_p, args_p[0]))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_CALLBACK_IS_NOT_CALLABLE);
  }

  /* 5. */
  if (len == 0 && args_number == 1)
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_REDUCE_OF_EMPTY_ARRAY_WITH_NO_INITIAL_VALUE);
  }

  JJS_ASSERT (ecma_is_value_object (args_p[0]));
  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, args_p[0]);

  ecma_value_t accumulator = ECMA_VALUE_UNDEFINED;

  /* 6. */
  ecma_length_t index = 0;
  const ecma_length_t last_index = len - 1;

  /* 7.a */
  if (args_number > 1)
  {
    accumulator = ecma_copy_value (context_p, args_p[1]);
  }
  else
  {
    /* 8.a */
    bool k_present = false;

    /* 8.b */
    while (!k_present && index < len)
    {
      /* 8.b.i */
      k_present = true;

      /* 8.b.ii-iii */
      ecma_value_t current_value = ecma_op_object_find_by_index (context_p, obj_p, start_from_left ? index : last_index - index);

      if (ECMA_IS_VALUE_ERROR (current_value))
      {
        return current_value;
      }

      if (ecma_is_value_found (current_value))
      {
        accumulator = current_value;
      }
      else
      {
        k_present = false;
      }

      /* 8.b.iv */
      index++;
    }

    /* 8.c */
    if (!k_present)
    {
      return ecma_raise_type_error (context_p, ECMA_ERR_MISSING_ARRAY_ELEMENT);
    }
  }
  /* 9. */
  ecma_value_t current_index;

  for (; index < len; index++)
  {
    const ecma_length_t corrected_index = start_from_left ? index : last_index - index;

    /* 9.a - 9.b */
    ecma_value_t current_value = ecma_op_object_find_by_index (context_p, obj_p, corrected_index);

    if (ECMA_IS_VALUE_ERROR (current_value))
    {
      ecma_free_value (context_p, accumulator);
      return current_value;
    }

    if (ecma_is_value_found (current_value))
    {
      /* 9.c.i, 9.c.ii */
      current_index = ecma_make_length_value (context_p, corrected_index);
      ecma_value_t call_args[] = { accumulator, current_value, current_index, ecma_make_object_value (context_p, obj_p) };

      ecma_value_t call_value = ecma_op_function_call (context_p, func_object_p, ECMA_VALUE_UNDEFINED, call_args, 4);
      ecma_free_value (context_p, current_index);
      ecma_free_value (context_p, accumulator);
      ecma_free_value (context_p, current_value);

      if (ECMA_IS_VALUE_ERROR (call_value))
      {
        return call_value;
      }

      accumulator = call_value;
    }
  }

  return accumulator;
} /* ecma_builtin_array_reduce_from */

/**
 * The Array.prototype object's 'fill' routine
 *
 * Note: this method only supports length up to uint32, instead of max_safe_integer
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_fill (ecma_context_t *context_p, /**< JJS context */
                                   ecma_value_t value, /**< value */
                                   ecma_value_t start_val, /**< start value */
                                   ecma_value_t end_val, /**< end value */
                                   ecma_object_t *obj_p, /**< object */
                                   ecma_length_t len) /**< object's length */
{
  ecma_length_t k, final;

  /* 5. 6. 7. */
  if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (context_p, start_val, len, &k)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 8. */
  if (ecma_is_value_undefined (end_val))
  {
    final = len;
  }
  else
  {
    /* 8 part 2, 9, 10 */
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (context_p, end_val, len, &final)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.length_prop_and_hole_count < ECMA_FAST_ARRAY_HOLE_ONE)
    {
      if (JJS_UNLIKELY (obj_p->u1.property_list_cp == JMEM_CP_NULL))
      {
        ecma_ref_object (obj_p);
        return ecma_make_object_value (context_p, obj_p);
      }

      ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (context_p, ecma_value_t, obj_p->u1.property_list_cp);

      while (k < final)
      {
        ecma_free_value_if_not_object (context_p, buffer_p[k]);
        buffer_p[k] = ecma_copy_value_if_not_object (context_p, value);
        k++;
      }

      ecma_ref_object (obj_p);
      return ecma_make_object_value (context_p, obj_p);
    }
  }

  /* 11. */
  while (k < final)
  {
    /* 11.a - 11.b */
    ecma_value_t put_val = ecma_op_object_put_by_index (context_p, obj_p, k, value, true);

    /* 11. c */
    if (ECMA_IS_VALUE_ERROR (put_val))
    {
      return put_val;
    }

    /* 11.d */
    k++;
  }

  ecma_ref_object (obj_p);
  return ecma_make_object_value (context_p, obj_p);
} /* ecma_builtin_array_prototype_fill */

/**
 * The Array.prototype object's 'find' and 'findIndex' routine
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.8
 *          ECMA-262 v6, 22.1.3.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_find (ecma_context_t *context_p, /**< JJS context */
                                          ecma_value_t predicate, /**< callback function */
                                          ecma_value_t predicate_this_arg, /**< this argument for
                                                                            *   invoke predicate */
                                          bool is_find, /**< true - find routine
                                                         *   false - findIndex routine */
                                          ecma_object_t *obj_p, /**< object */
                                          ecma_length_t len) /**< object's length */
{
  /* 5. */
  if (!ecma_op_is_callable (context_p, predicate))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_CALLBACK_IS_NOT_CALLABLE);
  }

  /* We already checked that predicate is callable, so it will always be an object. */
  JJS_ASSERT (ecma_is_value_object (predicate));
  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, predicate);

  /* 7 - 8. */
  for (ecma_length_t index = 0; index < len; index++)
  {
    /* 8.a - 8.c */
    ecma_value_t get_value = ecma_op_object_get_by_index (context_p, obj_p, index);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    /* 8.d - 8.e */
    ecma_value_t current_index = ecma_make_length_value (context_p, index);

    ecma_value_t call_args[] = { get_value, current_index, ecma_make_object_value (context_p, obj_p) };

    ecma_value_t call_value = ecma_op_function_call (context_p, func_object_p, predicate_this_arg, call_args, 3);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      ecma_free_value (context_p, get_value);
      return call_value;
    }

    bool call_value_to_bool = ecma_op_to_boolean (context_p, call_value);

    ecma_free_value (context_p, call_value);

    if (call_value_to_bool)
    {
      /* 8.f */
      if (is_find)
      {
        ecma_free_value (context_p, current_index);
        return get_value;
      }

      ecma_free_value (context_p, get_value);
      return current_index;
    }

    ecma_free_value (context_p, get_value);
    ecma_free_value (context_p, current_index);
  }

  /* 9. */
  return is_find ? ECMA_VALUE_UNDEFINED : ecma_make_integer_value (-1);
} /* ecma_builtin_array_prototype_object_find */


/**
 * The Array.prototype object's 'findLast' and 'findLastIndex' routine
 *
 * See also:
 *          ECMA-262, 23.1.3.11
 *          ECMA-262, 23.1.3.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_find_last (ecma_context_t *context_p, /**< JJS context */
                                               ecma_value_t predicate, /**< callback function */
                                               ecma_value_t predicate_this_arg, /**< this argument for
                                                                                 *   invoke predicate */
                                               bool is_find_last, /**< true - find_last routine
                                                                   *   false - findLastIndex routine */
                                               ecma_object_t *obj_p, /**< object */
                                               ecma_length_t len) /**< object's length */
{
  if (!ecma_op_is_callable (context_p, predicate))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_CALLBACK_IS_NOT_CALLABLE);
  }

  if (len == 0)
  {
    return is_find_last ? ECMA_VALUE_UNDEFINED : ecma_make_integer_value (-1);
  }

  /* We already checked that predicate is callable, so it will always be an object. */
  JJS_ASSERT (ecma_is_value_object (predicate));
  ecma_object_t *func_object_p = ecma_get_object_from_value (context_p, predicate);

  for (ecma_length_t index = len; index-- > 0;)
  {
    ecma_value_t get_value = ecma_op_object_get_by_index (context_p, obj_p, index);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    ecma_value_t current_index = ecma_make_length_value (context_p, index);
    ecma_value_t call_args[] = { get_value, current_index, ecma_make_object_value (context_p, obj_p) };
    ecma_value_t call_value = ecma_op_function_call (context_p, func_object_p, predicate_this_arg, call_args, 3);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      ecma_free_value (context_p, get_value);
      return call_value;
    }

    bool call_value_to_bool = ecma_op_to_boolean (context_p, call_value);

    ecma_free_value (context_p, call_value);

    if (call_value_to_bool)
    {
      /* 8.f */
      if (is_find_last)
      {
        ecma_free_value (context_p, current_index);
        return get_value;
      }

      ecma_free_value (context_p, get_value);
      return current_index;
    }

    ecma_free_value (context_p, get_value);
    ecma_free_value (context_p, current_index);
  }

  return is_find_last ? ECMA_VALUE_UNDEFINED : ecma_make_integer_value (-1);
} /* ecma_builtin_array_prototype_object_find_last */

/**
 * The Array.prototype object's 'copyWithin' routine
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_copy_within (ecma_context_t *context_p, /**< JJS context */
                                                 const ecma_value_t args[], /**< arguments list */
                                                 uint32_t args_number, /**< number of arguments */
                                                 ecma_object_t *obj_p, /**< object */
                                                 ecma_length_t len) /**< object's length */
{
  if (args_number == 0)
  {
    return ecma_copy_value (context_p, ecma_make_object_value (context_p, obj_p));
  }

  /* 5 - 7 */
  ecma_length_t target;

  if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (context_p, args[0], len, &target)))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_length_t start = 0;
  ecma_length_t end = len;

  if (args_number > 1)
  {
    /* 8 - 10 */
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (context_p, args[1], len, &start)))
    {
      return ECMA_VALUE_ERROR;
    }

    if (args_number > 2)
    {
      /* 11 */
      if (ecma_is_value_undefined (args[2]))
      {
        end = len;
      }
      else
      {
        /* 11 part 2, 12, 13 */
        if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (context_p, args[2], len, &end)))
        {
          return ECMA_VALUE_ERROR;
        }
      }
    }
  }

  ecma_length_t count = JJS_MIN (end - start, len - target);
  if (end <= start || len <= target) /* count <= 0 check, but variables are unsigned */
  {
    ecma_ref_object (obj_p);
    return ecma_make_object_value (context_p, obj_p);
  }

  bool forward = true;

  if (start < target && target < start + count)
  {
    start = start + count - 1;
    target = target + count - 1;
    forward = false;
  }

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;
    const uint32_t actual_length = ext_obj_p->u.array.length;

    if (ext_obj_p->u.array.length_prop_and_hole_count < ECMA_FAST_ARRAY_HOLE_ONE
        && ((forward && (target + count - 1 < actual_length)) || (!forward && (target < actual_length))))
    {
      if (obj_p->u1.property_list_cp != JMEM_CP_NULL)
      {
        ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (context_p, ecma_value_t, obj_p->u1.property_list_cp);

        for (; count > 0; count--)
        {
          ecma_value_t copy_value = ecma_copy_value_if_not_object (context_p, buffer_p[start]);

          ecma_free_value_if_not_object (context_p, buffer_p[target]);

          buffer_p[target] = copy_value;

          if (forward)
          {
            start++;
            target++;
          }
          else
          {
            start--;
            target--;
          }
        }
      }

      ecma_ref_object (obj_p);
      return ecma_make_object_value (context_p, obj_p);
    }
  }

  while (count > 0)
  {
    ecma_value_t get_value = ecma_op_object_find_by_index (context_p, obj_p, start);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    ecma_value_t op_value;

    if (ecma_is_value_found (get_value))
    {
      op_value = ecma_op_object_put_by_index (context_p, obj_p, target, get_value, true);
    }
    else
    {
      op_value = ecma_op_object_delete_by_index (context_p, obj_p, target, true);
    }

    ecma_free_value (context_p, get_value);

    if (ECMA_IS_VALUE_ERROR (op_value))
    {
      return op_value;
    }

    ecma_free_value (context_p, op_value);

    if (forward)
    {
      start++;
      target++;
    }
    else
    {
      start--;
      target--;
    }

    count--;
  }

  return ecma_copy_value (context_p, ecma_make_object_value (context_p, obj_p));
} /* ecma_builtin_array_prototype_object_copy_within */

/**
 * The Array.prototype object's 'includes' routine
 *
 * See also:
 *          ECMA-262 v11, 22.1.3.13
 *
 * @return ECMA_VALUE_ERROR -if the operation fails
 *         ECMA_VALUE_{TRUE/FALSE} - depends on whether the search element is in the array or not
 */
static ecma_value_t
ecma_builtin_array_prototype_includes (ecma_context_t *context_p, /**< JJS context */
                                       const ecma_value_t args[], /**< arguments list */
                                       uint32_t args_number, /**< number of arguments */
                                       ecma_object_t *obj_p, /**< object */
                                       ecma_length_t len) /**< object's length */
{
  /* 3. */
  if (len == 0)
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_length_t from_index = 0;

  /* 4-7. */
  if (args_number > 1)
  {
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (context_p, args[1], len, &from_index)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  /* Fast array path */
  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.length_prop_and_hole_count < ECMA_FAST_ARRAY_HOLE_ONE)
    {
      if (obj_p->u1.property_list_cp != JMEM_CP_NULL)
      {
        len = JJS_MIN (ext_obj_p->u.array.length, len);

        ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (context_p, ecma_value_t, obj_p->u1.property_list_cp);

        while (from_index < len)
        {
          if (ecma_op_same_value_zero (context_p, buffer_p[from_index], args[0], false))
          {
            return ECMA_VALUE_TRUE;
          }

          from_index++;
        }
      }

      return ECMA_VALUE_FALSE;
    }
  }

  /* 8. */
  while (from_index < len)
  {
    ecma_value_t element = ecma_op_object_get_by_index (context_p, obj_p, from_index);

    if (ECMA_IS_VALUE_ERROR (element))
    {
      return element;
    }

    if (ecma_op_same_value_zero (context_p, element, args[0], false))
    {
      ecma_free_value (context_p, element);
      return ECMA_VALUE_TRUE;
    }

    ecma_free_value (context_p, element);
    from_index++;
  }

  /* 9. */
  return ECMA_VALUE_FALSE;
} /* ecma_builtin_array_prototype_includes */

/**
 * Abstract operation: FlattenIntoArray
 *
 * See also:
 *          ECMA-262 v10, 22.1.3.10.1
 *
 * @return  ECMA_VALUE_ERROR -if the operation fails
 *          ecma value which contains target_index
 */
static ecma_value_t
ecma_builtin_array_flatten_into_array (ecma_context_t *context_p, /**< JJS context */
                                       ecma_value_t target, /**< target will contains source's elements  */
                                       ecma_object_t *source, /**< source object */
                                       ecma_length_t source_len, /**< source object length */
                                       ecma_length_t start, /**< remaining recursion depth */
                                       ecma_number_t depth, /**< start index offset */
                                       ecma_value_t mapped_value, /**< mapped value  */
                                       ecma_value_t thisArg) /**< this arg */
{
  ECMA_CHECK_STACK_USAGE (context_p);

  /* 7. */
  ecma_length_t target_index = start;

  /* 9. */
  for (ecma_length_t source_index = 0; source_index < source_len; source_index++)
  {
    /* a. */
    ecma_value_t element = ecma_op_object_find_by_index (context_p, source, source_index);

    if (ECMA_IS_VALUE_ERROR (element))
    {
      return element;
    }

    if (!ecma_is_value_found (element))
    {
      continue;
    }

    /* b-c. */
    if (!ecma_is_value_undefined (mapped_value))
    {
      /* i-ii. */
      ecma_value_t source_val = ecma_make_length_value (context_p, source_index);
      ecma_value_t args[] = { element, source_val, ecma_make_object_value (context_p, source) };
      ecma_value_t temp_element = ecma_op_function_call (context_p, ecma_get_object_from_value (context_p, mapped_value), thisArg, args, 3);

      ecma_free_value (context_p, element);
      ecma_free_value (context_p, source_val);

      if (ECMA_IS_VALUE_ERROR (temp_element))
      {
        return temp_element;
      }

      element = temp_element;
    }

    /* iv-v. */
    if (depth > 0)
    {
      ecma_value_t is_array = ecma_is_value_array (context_p, element);

      if (ECMA_IS_VALUE_ERROR (is_array))
      {
        ecma_free_value (context_p, element);
        return is_array;
      }

      if (ecma_is_value_true (is_array))
      {
        ecma_object_t *element_obj = ecma_get_object_from_value (context_p, element);
        ecma_length_t element_len;
        ecma_value_t len_value = ecma_op_object_get_length (context_p, element_obj, &element_len);

        if (ECMA_IS_VALUE_ERROR (len_value))
        {
          ecma_deref_object (element_obj);
          return len_value;
        }

        ecma_value_t target_index_val = ecma_builtin_array_flatten_into_array (context_p,
                                                                               target,
                                                                               element_obj,
                                                                               element_len,
                                                                               target_index,
                                                                               depth - 1,
                                                                               ECMA_VALUE_UNDEFINED,
                                                                               ECMA_VALUE_UNDEFINED);

        ecma_deref_object (element_obj);

        if (ECMA_IS_VALUE_ERROR (target_index_val))
        {
          return target_index_val;
        }

        target_index = (ecma_length_t) ecma_get_number_from_value (context_p, target_index_val);
        continue;
      }
    }

    /* vi. */
    const uint32_t flags = ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE | JJS_PROP_SHOULD_THROW;
    ecma_value_t element_temp =
      ecma_builtin_helper_def_prop_by_index (context_p, ecma_get_object_from_value (context_p, target), target_index, element, flags);

    ecma_free_value (context_p, element);

    if (ECMA_IS_VALUE_ERROR (element_temp))
    {
      return element_temp;
    }

    target_index++;
  }
  /* 10. */
  return ecma_make_length_value (context_p, target_index);
} /* ecma_builtin_array_flatten_into_array */

/**
 * The Array.prototype object's 'flat' routine
 *
 * See also:
 *          ECMA-262 v10, 22.1.3.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_flat (ecma_context_t *context_p, /**< JJS context */
                                          const ecma_value_t args[], /**< arguments list */
                                          uint32_t args_number, /**< number of arguments */
                                          ecma_object_t *obj_p, /**< array object */
                                          ecma_length_t len) /**< array object's length */
{
  /* 3. */
  ecma_number_t depth_num = 1;

  /* 4. */
  if (args_number > 0 && ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, args[0], &depth_num)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 5. */
  ecma_object_t *new_array_p = ecma_op_array_species_create (context_p, obj_p, 0);

  if (JJS_UNLIKELY (new_array_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 6. */
  ecma_value_t flatten_val = ecma_builtin_array_flatten_into_array (context_p,
                                                                    ecma_make_object_value (context_p, new_array_p),
                                                                    obj_p,
                                                                    len,
                                                                    0,
                                                                    depth_num,
                                                                    ECMA_VALUE_UNDEFINED,
                                                                    ECMA_VALUE_UNDEFINED);

  if (ECMA_IS_VALUE_ERROR (flatten_val))
  {
    ecma_deref_object (new_array_p);
    return flatten_val;
  }

  /* 7. */
  return ecma_make_object_value (context_p, new_array_p);
} /* ecma_builtin_array_prototype_object_flat */

/**
 * The Array.prototype object's 'flatMap' routine
 *
 * See also:
 *          ECMA-262 v10, 22.1.3.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_flat_map (ecma_context_t *context_p, /**< JJS context */
                                              ecma_value_t callback, /**< callbackFn */
                                              ecma_value_t this_arg, /**< thisArg */
                                              ecma_object_t *obj_p, /**< array object */
                                              ecma_length_t len) /**< array object's length */
{
  if (!ecma_op_is_callable (context_p, callback))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_CALLBACK_IS_NOT_CALLABLE);
  }

  /* 4. */
  ecma_object_t *new_array_p = ecma_op_array_species_create (context_p, obj_p, 0);

  if (JJS_UNLIKELY (new_array_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 5. */
  ecma_value_t flatten_val =
    ecma_builtin_array_flatten_into_array (context_p, ecma_make_object_value (context_p, new_array_p), obj_p, len, 0, 1, callback, this_arg);
  if (ECMA_IS_VALUE_ERROR (flatten_val))
  {
    ecma_deref_object (new_array_p);
    return flatten_val;
  }

  /* 6. */
  return ecma_make_object_value (context_p, new_array_p);
} /* ecma_builtin_array_prototype_object_flat_map */

/**
 * The Array.prototype object's 'with' routine
 *
 * See also:
 *          ECMA-262 v14, 23.1.3.39
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_with (ecma_context_t *context_p, /**< JJS context */
                                          const ecma_value_t args[], /**< arguments list */
                                          uint32_t args_number, /**< argument count */
                                          ecma_object_t *obj_p, /**< array object */
                                          ecma_length_t len) /**< array object's length */
{
  ecma_number_t relative_index = ECMA_NUMBER_ZERO;

  // JJS limits an array size of 2^32 - 1, but the spec allows up to 2^53 - 1 and objects can have
  // up to 2^32 - 1 properties. obj_p can be a plain object with a length property, which exceeds the
  // array limit. Therefore, we need to check the length here.
  if (len > UINT32_MAX)
  {
    jjs_error_t e = len > (ecma_length_t)ECMA_NUMBER_MAX_SAFE_INTEGER ? JJS_ERROR_TYPE : JJS_ERROR_RANGE;
    return ecma_raise_standard_error (context_p, e, ECMA_ERR_ARRAY_CONSTRUCTOR_SIZE_EXCEEDED);
  }

  // 3
  ecma_value_t tioi_result = ecma_op_to_integer_or_infinity (context_p,
                                                             args_number > 0 ? args[0] : ECMA_VALUE_UNDEFINED,
                                                             &relative_index);

  if(ECMA_IS_VALUE_ERROR (tioi_result))
  {
    return tioi_result;
  }

  ecma_free_value (context_p, tioi_result);

  // 4, 5
  ecma_number_t len_n = (ecma_number_t)len;
  ecma_number_t actual_index_n = (relative_index >= 0) ? relative_index : len_n + relative_index;

  // 6
  if (actual_index_n >= len_n || actual_index_n < 0)
  {
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_RANGE_OF_INDEX);
  }

  // 7
  ecma_object_t* a = ecma_op_new_array_object (context_p, (uint32_t)len);

  // 8
  ecma_length_t k = 0;
  ecma_value_t value = args_number > 1 ? args[1] : ECMA_VALUE_UNDEFINED;
  ecma_length_t actual_index = (ecma_length_t)actual_index_n;

  // 9
  while (k < len) {
    if (k == actual_index)
    {
      // 9.b, 9.d
      ecma_value_t result = ecma_op_object_put_by_index (context_p, a, k, value, true);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        ecma_deref_object (a);
        return result;
      }

      ecma_free_value (context_p, result);
    }
    else
    {
      // 9.c
      ecma_value_t element = ecma_op_object_get_by_index (context_p, obj_p, k);

      if (ECMA_IS_VALUE_ERROR (element))
      {
        ecma_deref_object (a);
        return element;
      }

      // 9.d
      ecma_value_t result = ecma_op_object_put_by_index (context_p, a, k, element, true);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        ecma_deref_object (a);
        ecma_free_value (context_p, element);
        return result;
      }

      ecma_free_value (context_p, result);
      ecma_free_value (context_p, element);
    }

    // 9.e
    k++;
  }

  // 10
  return ecma_make_object_value (context_p, a);
} /* ecma_builtin_array_prototype_object_with */

/**
 * The Array.prototype object's 'toReversed' routine
 *
 * See also:
 *          ECMA-262 v14, 23.1.3.33
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_to_reversed (ecma_context_t *context_p, /**< JJS context */
                                                 ecma_object_t *obj_p, /**< array object */
                                                 ecma_length_t len)    /**< array object's length */
{
  // 23.1.3.33.3
  if (len > UINT32_MAX)
  {
    jjs_error_t e = len > (ecma_length_t)ECMA_NUMBER_MAX_SAFE_INTEGER ? JJS_ERROR_TYPE : JJS_ERROR_RANGE;
    return ecma_raise_standard_error (context_p, e, ECMA_ERR_ARRAY_CONSTRUCTOR_SIZE_EXCEEDED);
  }

  ecma_object_t* a = ecma_op_new_array_object (context_p, (uint32_t)len);

  // 23.1.3.33.4
  ecma_length_t k = 0;

  // 23.1.3.33.5
  while (k < len)
  {
    // 23.1.3.33.5.a-c
    ecma_value_t from_value = ecma_op_object_get_by_index (context_p, obj_p, len - k - 1);

    if (ECMA_IS_VALUE_ERROR (from_value))
    {
      ecma_deref_object (a);
      return from_value;
    }

    // 23.1.3.33.5.d
    ecma_value_t result = ecma_op_object_put_by_index (context_p, a, k, from_value, true);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      ecma_deref_object (a);
      ecma_free_value (context_p, from_value);
      return result;
    }

    ecma_free_value (context_p, from_value);
    ecma_free_value (context_p, result);

    // 23.1.3.33.5.e
    k++;
  }

  // 23.1.3.33.6
  return ecma_make_object_value (context_p, a);
} /* ecma_builtin_array_prototype_object_to_reversed */

/**
 * The Array.prototype object's 'toSorted' routine.
 *
 * See also:
 *          ECMA-262 v14, 23.1.3.34 Array.prototype.toSorted
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_to_sorted (ecma_context_t *context_p, /**< JJS context */
                                               ecma_value_t compare_fn, /**< sort compare function or undefined */
                                               ecma_object_t *obj_p, /**< array object */
                                               ecma_length_t len)    /**< array object's length */
{
  if (len > UINT32_MAX)
  {
    jjs_error_t e = len > (ecma_length_t)ECMA_NUMBER_MAX_SAFE_INTEGER ? JJS_ERROR_TYPE : JJS_ERROR_RANGE;
    return ecma_raise_standard_error (context_p, e, ECMA_ERR_ARRAY_CONSTRUCTOR_SIZE_EXCEEDED);
  }

  ecma_object_t* a = ecma_op_new_array_object (context_p, (uint32_t)len);
  ecma_collection_t *array_index_props_p = ecma_new_collection (context_p);

  for (uint32_t i = 0; i < len; i++)
  {
    ecma_string_t *prop_name_p = ecma_new_ecma_string_from_uint32 (context_p, i);
    ecma_property_descriptor_t prop_desc;
    ecma_value_t get_desc = ecma_op_object_get_own_property_descriptor (context_p, obj_p, prop_name_p, &prop_desc);

    if (ECMA_IS_VALUE_ERROR (get_desc))
    {
      ecma_deref_object (a);
      ecma_collection_free (context_p, array_index_props_p);
      ecma_deref_ecma_string (context_p, prop_name_p);
      return get_desc;
    }

    if (ecma_is_value_true (get_desc))
    {
      ecma_ref_ecma_string (prop_name_p);
      ecma_collection_push_back (context_p, array_index_props_p, ecma_make_string_value (context_p, prop_name_p));
      ecma_free_property_descriptor (context_p, &prop_desc);
      continue;
    }
  }

  uint32_t defined_prop_count = array_index_props_p->item_count;

  ecma_value_t ret_value = ECMA_VALUE_ERROR;
  uint32_t copied_num = 0;
  JMEM_DEFINE_LOCAL_ARRAY (context_p, values_buffer, defined_prop_count, ecma_value_t);

  ecma_value_t *buffer_p = array_index_props_p->buffer_p;

  /* Copy unsorted array into a native c array. */
  for (uint32_t i = 0; i < array_index_props_p->item_count; i++)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (context_p, buffer_p[i]);

    uint32_t index = ecma_string_get_array_index (property_name_p);
    JJS_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);

    if (index >= len)
    {
      break;
    }

    ecma_value_t index_value = ecma_op_object_get (context_p, obj_p, property_name_p);

    if (ECMA_IS_VALUE_ERROR (index_value))
    {
      goto clean_up;
    }

    values_buffer[copied_num++] = index_value;
  }

  JJS_ASSERT (copied_num == defined_prop_count);

  /* Sorting. */
  if (copied_num > 1)
  {
    const ecma_builtin_helper_sort_compare_fn_t sort_cb = &ecma_builtin_array_prototype_object_sort_compare_helper;
    ecma_value_t sort_value =
      ecma_builtin_helper_array_merge_sort_helper (context_p, values_buffer, (uint32_t) (copied_num), compare_fn, sort_cb, NULL);
    if (ECMA_IS_VALUE_ERROR (sort_value))
    {
      goto clean_up;
    }

    ecma_free_value (context_p, sort_value);
  }

  /* Put sorted values to the front of the array. */
  for (uint32_t index = 0; index < copied_num; index++)
  {
    ecma_value_t put_value = ecma_op_object_put_by_index (context_p, a, index, values_buffer[index], true);

    if (ECMA_IS_VALUE_ERROR (put_value))
    {
      goto clean_up;
    }
  }

  ret_value = ECMA_VALUE_EMPTY;

clean_up:
  /* Free values that were copied to the local array. */
  for (uint32_t index = 0; index < copied_num; index++)
  {
    ecma_free_value (context_p, values_buffer[index]);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (context_p, values_buffer);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_deref_object (a);
    ecma_collection_free (context_p, array_index_props_p);
    return ret_value;
  }

  JJS_ASSERT (ecma_is_value_empty (ret_value));

  /* Undefined properties should be in the back of the array. */
  ecma_value_t *buffer_p = array_index_props_p->buffer_p;

  for (uint32_t i = 0; i < array_index_props_p->item_count; i++)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (context_p, buffer_p[i]);

    uint32_t index = ecma_string_get_array_index (property_name_p);
    JJS_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);

    if (index >= copied_num && index < len)
    {
      ecma_value_t del_value = ecma_op_object_delete (context_p, a, property_name_p, true);

      if (ECMA_IS_VALUE_ERROR (del_value))
      {
        ecma_deref_object(a);
        ecma_collection_free (context_p, array_index_props_p);
        return del_value;
      }
    }
  }

  ecma_collection_free (context_p, array_index_props_p);

  return ecma_make_object_value (context_p, a);
} /* ecma_builtin_array_prototype_object_to_reversed */

/**
 * The Array.prototype object's 'toSpliced' routine
 *
 * See also:
 *          ECMA-262 v14, 23.1.3.35
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_to_spliced (ecma_context_t *context_p, /**< JJS context */
                                                const ecma_value_t args[], /**< arguments list */
                                                uint32_t args_number, /**< argument count */
                                                ecma_object_t *obj_p, /**< array object */
                                                ecma_length_t len) /**< array object's length */
{
  ecma_number_t relative_start;
  ecma_value_t result;

  // 23.1.3.35.3
  if (args_number > 0)
  {
    result = ecma_op_to_integer_or_infinity (context_p, args[0], &relative_start);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      return result;
    }

    ecma_free_value (context_p, result);
  }
  else
  {
    relative_start = ECMA_NUMBER_ZERO;
  }

  // 23.1.3.35.4-6
  ecma_length_t actual_start;

  if (ecma_number_is_infinity (relative_start))
  {
    if (relative_start < 0)
    {
      actual_start = 0;
    }
    else
    {
      actual_start = len;
    }
  }
  else if (relative_start < 0)
  {
    ecma_number_t n = JJS_MAX ((ecma_number_t)len + relative_start, ECMA_NUMBER_ZERO);

    actual_start = (ecma_length_t)n;
  }
  else
  {
    actual_start = JJS_MIN ((ecma_length_t)relative_start, len);
  }

  // 23.1.3.35.7
  ecma_length_t insert_count = args_number > 2 ? args_number - 2 : 0;

  // 23.1.3.35.8-10
  ecma_length_t actual_skip_count;

  if (args_number == 0)
  {
    actual_skip_count = 0;
  }
  else if (args_number < 2)
  {
    actual_skip_count = len - actual_start;
  }
  else
  {
    ecma_number_t sc;

    if (args_number > 1)
    {
      result = ecma_op_to_integer_or_infinity (context_p, args[1], &sc);

      if (ECMA_IS_VALUE_ERROR (result)) {
        return result;
      }

      ecma_free_value (context_p, result);
    }
    else
    {
      sc = ECMA_NUMBER_ZERO;
    }

    if (sc < 0)
    {
      actual_skip_count = 0;
    }
    else if ((ecma_length_t)sc > len - actual_start)
    {
      actual_skip_count = len - actual_start;
    }
    else
    {
      actual_skip_count = (ecma_length_t)sc;
    }
  }

  // 23.1.3.35.11
  ecma_length_t new_len = len + insert_count - actual_skip_count;

  // 23.1.3.35.12
  if (new_len > UINT32_MAX)
  {
    jjs_error_t e = new_len > (ecma_length_t)ECMA_NUMBER_MAX_SAFE_INTEGER ? JJS_ERROR_TYPE : JJS_ERROR_RANGE;
    return ecma_raise_standard_error (context_p, e, ECMA_ERR_ARRAY_CONSTRUCTOR_SIZE_EXCEEDED);
  }

  // 23.1.3.35.13
  ecma_object_t* a = ecma_op_new_array_object (context_p, (uint32_t)new_len);
  // 23.1.3.35.14
  ecma_length_t i = 0;
  // 23.1.3.35.15
  ecma_length_t r = actual_start + actual_skip_count;

  // 23.1.3.35.16
  while (i < actual_start)
  {
    // 23.1.3.35.16.a-b
    ecma_value_t from_value = ecma_op_object_get_by_index (context_p, obj_p, i);

    if (ECMA_IS_VALUE_ERROR (from_value))
    {
      ecma_deref_object (a);
      return from_value;
    }

    // 23.1.3.35.16.c
    result = ecma_op_object_put_by_index (context_p, a, i, from_value, true);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      ecma_deref_object (a);
      ecma_free_value (context_p, from_value);
      return result;
    }

    ecma_free_value (context_p, from_value);
    ecma_free_value (context_p, result);

    // 23.1.3.35.16.d
    i++;
  }

  // 23.1.3.35.17
  for (ecma_length_t index = 2; index < args_number; index++)
  {
    // 23.1.3.35.17.a-b
    result = ecma_op_object_put_by_index (context_p, a, i, args[index], true);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      ecma_deref_object (a);
      return result;
    }

    ecma_free_value (context_p, result);

    // 23.1.3.35.17.c
    i++;
  }

  // 23.1.3.35.18
  while (i < new_len)
  {
    // 23.1.3.35.18.a-c
    ecma_value_t from_value = ecma_op_object_get_by_index (context_p, obj_p, r);

    if (ECMA_IS_VALUE_ERROR (from_value))
    {
      ecma_deref_object (a);
      return from_value;
    }

    // 23.1.3.35.18.d
    result = ecma_op_object_put_by_index (context_p, a, i, from_value, true);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      ecma_deref_object (a);
      ecma_free_value (context_p, from_value);
      return result;
    }

    ecma_free_value (context_p, from_value);
    ecma_free_value (context_p, result);
    // 23.1.3.35.18.e
    i++;
    // 23.1.3.35.18.f
    r++;
  }

  // 23.1.3.35.19
  return ecma_make_object_value (context_p, a);
} /* ecma_builtin_array_prototype_object_to_spliced */

/**
 * Dispatcher of the built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_array_prototype_dispatch_routine (ecma_context_t *context_p, /**< JJS context */
                                               uint8_t builtin_routine_id, /**< built-in wide routine identifier */
                                               ecma_value_t this_arg, /**< 'this' argument value */
                                               const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                                       *   passed to routine */
                                               uint32_t arguments_number) /**< length of arguments' list */
{
  ecma_value_t obj_this = ecma_op_to_object (context_p, this_arg);

  if (ECMA_IS_VALUE_ERROR (obj_this))
  {
    return obj_this;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (context_p, obj_this);

  if (JJS_UNLIKELY (builtin_routine_id <= ECMA_ARRAY_PROTOTYPE_CONCAT))
  {
    ecma_value_t ret_value = ECMA_VALUE_EMPTY;

    if (builtin_routine_id == ECMA_ARRAY_PROTOTYPE_SORT)
    {
      ret_value = ecma_builtin_array_prototype_object_sort (context_p, this_arg, arguments_list_p[0], obj_p);
    }
    else if (builtin_routine_id == ECMA_ARRAY_PROTOTYPE_CONCAT)
    {
      ret_value = ecma_builtin_array_prototype_object_concat (context_p, arguments_list_p, arguments_number, obj_p);
    }

    ecma_deref_object (obj_p);
    return ret_value;
  }

  if (JJS_UNLIKELY (builtin_routine_id >= ECMA_ARRAY_PROTOTYPE_ENTRIES
                      && builtin_routine_id <= ECMA_ARRAY_PROTOTYPE_KEYS))
  {
    ecma_value_t ret_value;

    if (builtin_routine_id == ECMA_ARRAY_PROTOTYPE_ENTRIES)
    {
      ret_value = ecma_op_create_array_iterator (context_p, obj_p, ECMA_ITERATOR_ENTRIES);
    }
    else
    {
      JJS_ASSERT (builtin_routine_id == ECMA_ARRAY_PROTOTYPE_KEYS);
      ret_value = ecma_op_create_array_iterator (context_p, obj_p, ECMA_ITERATOR_KEYS);
    }

    ecma_deref_object (obj_p);
    return ret_value;
  }

  // the spec calls for the compare function to be checked before the length
  ecma_value_t compare_fn = ECMA_VALUE_UNDEFINED;

  if (ECMA_ARRAY_PROTOTYPE_TO_SORTED == builtin_routine_id)
  {
    if (arguments_number > 0)
    {
      compare_fn = arguments_list_p[0];

      if (!ecma_is_value_undefined (compare_fn) && !ecma_op_is_callable (context_p, compare_fn))
      {
        ecma_deref_object (obj_p);
        return ecma_raise_type_error (context_p, ECMA_ERR_COMPARE_FUNC_NOT_CALLABLE);
      }
    }
  }

  ecma_length_t length;
  ecma_value_t len_value = ecma_op_object_get_length (context_p, obj_p, &length);

  if (ECMA_IS_VALUE_ERROR (len_value))
  {
    ecma_deref_object (obj_p);
    return len_value;
  }

  ecma_value_t ret_value;

  switch (builtin_routine_id)
  {
    case ECMA_ARRAY_PROTOTYPE_TO_LOCALE_STRING:
    {
      ret_value = ecma_builtin_array_prototype_object_to_locale_string (context_p, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_JOIN:
    {
      ret_value = ecma_builtin_array_prototype_join (context_p, arguments_list_p[0], obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_POP:
    {
      ret_value = ecma_builtin_array_prototype_object_pop (context_p, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_PUSH:
    {
      ret_value = ecma_builtin_array_prototype_object_push (context_p, arguments_list_p, arguments_number, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_REVERSE:
    {
      ret_value = ecma_builtin_array_prototype_object_reverse (context_p, this_arg, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_SHIFT:
    {
      ret_value = ecma_builtin_array_prototype_object_shift (context_p, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_SLICE:
    {
      ret_value = ecma_builtin_array_prototype_object_slice (context_p, arguments_list_p[0], arguments_list_p[1], obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_SPLICE:
    {
      ret_value = ecma_builtin_array_prototype_object_splice (context_p, arguments_list_p, arguments_number, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_UNSHIFT:
    {
      ret_value = ecma_builtin_array_prototype_object_unshift (context_p, arguments_list_p, arguments_number, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_AT:
    {
      ret_value = ecma_builtin_array_prototype_object_at (context_p, arguments_list_p[0], obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_INDEX_OF:
    {
      ret_value = ecma_builtin_array_prototype_object_index_of (context_p, arguments_list_p, arguments_number, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_LAST_INDEX_OF:
    {
      ret_value = ecma_builtin_array_prototype_object_last_index_of (context_p, arguments_list_p, arguments_number, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_EVERY:
    case ECMA_ARRAY_PROTOTYPE_SOME:
    case ECMA_ARRAY_PROTOTYPE_FOR_EACH:
    {
      ret_value = ecma_builtin_array_apply (context_p,
                                            arguments_list_p[0],
                                            arguments_list_p[1],
                                            (array_routine_mode) builtin_routine_id - ECMA_ARRAY_PROTOTYPE_EVERY,
                                            obj_p,
                                            length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_MAP:
    {
      ret_value = ecma_builtin_array_prototype_object_map (context_p, arguments_list_p[0], arguments_list_p[1], obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_REDUCE:
    case ECMA_ARRAY_PROTOTYPE_REDUCE_RIGHT:
    {
      ret_value = ecma_builtin_array_reduce_from (context_p,
                                                  arguments_list_p,
                                                  arguments_number,
                                                  builtin_routine_id == ECMA_ARRAY_PROTOTYPE_REDUCE,
                                                  obj_p,
                                                  length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_COPY_WITHIN:
    {
      ret_value = ecma_builtin_array_prototype_object_copy_within (context_p, arguments_list_p, arguments_number, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_FIND:
    case ECMA_ARRAY_PROTOTYPE_FIND_INDEX:
    {
      ret_value = ecma_builtin_array_prototype_object_find (context_p,
                                                            arguments_list_p[0],
                                                            arguments_list_p[1],
                                                            builtin_routine_id == ECMA_ARRAY_PROTOTYPE_FIND,
                                                            obj_p,
                                                            length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_FILL:
    {
      ret_value = ecma_builtin_array_prototype_fill (context_p,
                                                     arguments_list_p[0],
                                                     arguments_list_p[1],
                                                     arguments_list_p[2],
                                                     obj_p,
                                                     length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_INCLUDES:
    {
      ret_value = ecma_builtin_array_prototype_includes (context_p, arguments_list_p, arguments_number, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_FLAT:
    {
      ret_value = ecma_builtin_array_prototype_object_flat (context_p, arguments_list_p, arguments_number, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_FLATMAP:
    {
      ret_value =
        ecma_builtin_array_prototype_object_flat_map (context_p, arguments_list_p[0], arguments_list_p[1], obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_FIND_LAST:
    case ECMA_ARRAY_PROTOTYPE_FIND_LAST_INDEX:
    {
      ret_value = ecma_builtin_array_prototype_object_find_last (context_p,
                                                                 arguments_list_p[0],
                                                                 arguments_list_p[1],
                                                                 builtin_routine_id == ECMA_ARRAY_PROTOTYPE_FIND_LAST,
                                                                 obj_p,
                                                                 length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_WITH:
    {
      ret_value = ecma_builtin_array_prototype_object_with (context_p, arguments_list_p, arguments_number, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_TO_REVERSED:
    {
      ret_value = ecma_builtin_array_prototype_object_to_reversed (context_p, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_TO_SORTED:
    {
      ret_value = ecma_builtin_array_prototype_object_to_sorted (context_p, compare_fn, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_TO_SPLICED:
    {
      ret_value = ecma_builtin_array_prototype_object_to_spliced (context_p, arguments_list_p, arguments_number, obj_p, length);
      break;
    }
    default:
    {
      JJS_ASSERT (builtin_routine_id == ECMA_ARRAY_PROTOTYPE_FILTER);

      ret_value = ecma_builtin_array_prototype_object_filter (context_p, arguments_list_p[0], arguments_list_p[1], obj_p, length);
      break;
    }
  }

  ecma_free_value (context_p, len_value);
  ecma_deref_object (obj_p);

  return ret_value;
} /* ecma_builtin_array_prototype_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* JJS_BUILTIN_ARRAY */
