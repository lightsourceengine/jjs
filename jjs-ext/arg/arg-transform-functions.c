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

#include "jjs.h"

#include "jjs-ext/arg.h"

/**
 * The common function to deal with optional arguments.
 * The core transform function is provided by argument `func`.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_optional (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                               const jjsx_arg_t *c_arg_p, /**< native arg */
                               jjsx_arg_transform_func_t func) /**< the core transform function */
{
  jjs_value_t js_arg = jjsx_arg_js_iterator_peek (js_arg_iter_p);

  if (jjs_value_is_undefined (js_arg))
  {
    return jjsx_arg_js_iterator_pop (js_arg_iter_p);
  }

  return func (js_arg_iter_p, c_arg_p);
} /* jjsx_arg_transform_optional */

/**
 * The common part in transforming a JS argument to a number (double or certain int) type.
 * Type coercion is not allowed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
static jjs_value_t
jjsx_arg_transform_number_strict_common (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                           double *number_p) /**< [out] the number in JS arg */
{
  jjs_value_t js_arg = jjsx_arg_js_iterator_pop (js_arg_iter_p);

  if (!jjs_value_is_number (js_arg))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "It is not a number.");
  }

  *number_p = jjs_value_as_number (js_arg);

  return jjs_undefined ();
} /* jjsx_arg_transform_number_strict_common */

/**
 * The common part in transforming a JS argument to a number (double or certain int) type.
 * Type coercion is allowed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
static jjs_value_t
jjsx_arg_transform_number_common (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                    double *number_p) /**< [out] the number in JS arg */
{
  jjs_value_t js_arg = jjsx_arg_js_iterator_pop (js_arg_iter_p);

  jjs_value_t to_number = jjs_value_to_number (js_arg);

  if (jjs_value_is_exception (to_number))
  {
    jjs_value_free (to_number);

    return jjs_throw_sz (JJS_ERROR_TYPE, "It can not be converted to a number.");
  }

  *number_p = jjs_value_as_number (to_number);
  jjs_value_free (to_number);

  return jjs_undefined ();
} /* jjsx_arg_transform_number_common */

/**
 * Transform a JS argument to a double. Type coercion is not allowed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_number_strict (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                    const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  return jjsx_arg_transform_number_strict_common (js_arg_iter_p, c_arg_p->dest);
} /* jjsx_arg_transform_number_strict */

/**
 * Transform a JS argument to a double. Type coercion is allowed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_number (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                             const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  return jjsx_arg_transform_number_common (js_arg_iter_p, c_arg_p->dest);
} /* jjsx_arg_transform_number */

/**
 * Helper function to process a double number before converting it
 * to an integer.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
static jjs_value_t
jjsx_arg_helper_process_double (double *d, /**< [in, out] the number to be processed */
                                  double min, /**< the min value for clamping */
                                  double max, /**< the max value for clamping */
                                  jjsx_arg_int_option_t option) /**< the converting policies */
{
  if (*d != *d) /* isnan (*d) triggers conversion warning on clang<9 */
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "The number is NaN.");
  }

  if (option.clamp == JJSX_ARG_NO_CLAMP)
  {
    if (*d > max || *d < min)
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, "The number is out of range.");
    }
  }
  else
  {
    *d = *d < min ? min : *d;
    *d = *d > max ? max : *d;
  }

  if (option.round == JJSX_ARG_ROUND)
  {
    *d = (*d >= 0.0) ? floor (*d + 0.5) : ceil (*d - 0.5);
  }
  else if (option.round == JJSX_ARG_FLOOR)
  {
    *d = floor (*d);
  }
  else
  {
    *d = ceil (*d);
  }

  return jjs_undefined ();
} /* jjsx_arg_helper_process_double */

/**
 * Use the macro to define thr transform functions for int type.
 */
#define JJSX_ARG_TRANSFORM_FUNC_FOR_INT_TEMPLATE(type, suffix, min, max)                    \
  jjs_value_t jjsx_arg_transform_##type##suffix (jjsx_arg_js_iterator_t *js_arg_iter_p, \
                                                     const jjsx_arg_t *c_arg_p)             \
  {                                                                                           \
    double tmp = 0.0;                                                                         \
    jjs_value_t rv = jjsx_arg_transform_number##suffix##_common (js_arg_iter_p, &tmp);    \
    if (jjs_value_is_exception (rv))                                                        \
    {                                                                                         \
      return rv;                                                                              \
    }                                                                                         \
    jjs_value_free (rv);                                                                    \
    union                                                                                     \
    {                                                                                         \
      jjsx_arg_int_option_t int_option;                                                     \
      uintptr_t extra_info;                                                                   \
    } u = { .extra_info = c_arg_p->extra_info };                                              \
    rv = jjsx_arg_helper_process_double (&tmp, min, max, u.int_option);                     \
    if (jjs_value_is_exception (rv))                                                        \
    {                                                                                         \
      return rv;                                                                              \
    }                                                                                         \
    *(type##_t *) c_arg_p->dest = (type##_t) tmp;                                             \
    return rv;                                                                                \
  }

#define JJSX_ARG_TRANSFORM_FUNC_FOR_INT(type, min, max)              \
  JJSX_ARG_TRANSFORM_FUNC_FOR_INT_TEMPLATE (type, _strict, min, max) \
  JJSX_ARG_TRANSFORM_FUNC_FOR_INT_TEMPLATE (type, , min, max)

JJSX_ARG_TRANSFORM_FUNC_FOR_INT (uint8, 0, UINT8_MAX)
JJSX_ARG_TRANSFORM_FUNC_FOR_INT (int8, INT8_MIN, INT8_MAX)
JJSX_ARG_TRANSFORM_FUNC_FOR_INT (uint16, 0, UINT16_MAX)
JJSX_ARG_TRANSFORM_FUNC_FOR_INT (int16, INT16_MIN, INT16_MAX)
JJSX_ARG_TRANSFORM_FUNC_FOR_INT (uint32, 0, UINT32_MAX)
JJSX_ARG_TRANSFORM_FUNC_FOR_INT (int32, INT32_MIN, INT32_MAX)

#undef JJSX_ARG_TRANSFORM_FUNC_FOR_INT_TEMPLATE
#undef JJSX_ARG_TRANSFORM_FUNC_FOR_INT
/**
 * Transform a JS argument to a boolean. Type coercion is not allowed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_boolean_strict (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                     const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  jjs_value_t js_arg = jjsx_arg_js_iterator_pop (js_arg_iter_p);

  if (!jjs_value_is_boolean (js_arg))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "It is not a boolean.");
  }

  bool *dest = c_arg_p->dest;
  *dest = jjs_value_is_true (js_arg);

  return jjs_undefined ();
} /* jjsx_arg_transform_boolean_strict */

/**
 * Transform a JS argument to a boolean. Type coercion is allowed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_boolean (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                              const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  jjs_value_t js_arg = jjsx_arg_js_iterator_pop (js_arg_iter_p);

  bool to_boolean = jjs_value_to_boolean (js_arg);

  bool *dest = c_arg_p->dest;
  *dest = to_boolean;

  return jjs_undefined ();
} /* jjsx_arg_transform_boolean */

/**
 * The common routine for string transformer.
 * It works for both CESU-8 and UTF-8 string.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
static jjs_value_t
jjsx_arg_string_to_buffer_common_routine (jjs_value_t js_arg, /**< JS arg */
                                            const jjsx_arg_t *c_arg_p, /**< native arg */
                                            jjs_encoding_t encoding) /**< string encoding */
{
  jjs_char_t *target_p = (jjs_char_t *) c_arg_p->dest;
  jjs_size_t target_buf_size = (jjs_size_t) c_arg_p->extra_info;

  jjs_size_t size = jjs_string_size (js_arg, encoding);

  if (size > target_buf_size - 1)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Buffer size is not large enough.");
  }

  jjs_string_to_buffer (js_arg, encoding, target_p, target_buf_size);
  target_p[size] = '\0';

  return jjs_undefined ();
} /* jjsx_arg_string_to_buffer_common_routine */

/**
 * Transform a JS argument to a UTF-8/CESU-8 char array. Type coercion is not allowed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
static jjs_value_t
jjsx_arg_transform_string_strict_common (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                           const jjsx_arg_t *c_arg_p, /**< the native arg */
                                           jjs_encoding_t encoding) /**< string encoding */
{
  jjs_value_t js_arg = jjsx_arg_js_iterator_pop (js_arg_iter_p);

  if (!jjs_value_is_string (js_arg))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "It is not a string.");
  }

  return jjsx_arg_string_to_buffer_common_routine (js_arg, c_arg_p, encoding);
} /* jjsx_arg_transform_string_strict_common */

/**
 * Transform a JS argument to a UTF-8/CESU-8 char array. Type coercion is allowed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
static jjs_value_t
jjsx_arg_transform_string_common (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                    const jjsx_arg_t *c_arg_p, /**< the native arg */
                                    jjs_encoding_t encoding) /**< string encoding */
{
  jjs_value_t js_arg = jjsx_arg_js_iterator_pop (js_arg_iter_p);

  jjs_value_t to_string = jjs_value_to_string (js_arg);

  if (jjs_value_is_exception (to_string))
  {
    jjs_value_free (to_string);

    return jjs_throw_sz (JJS_ERROR_TYPE, "It can not be converted to a string.");
  }

  jjs_value_t ret = jjsx_arg_string_to_buffer_common_routine (to_string, c_arg_p, encoding);
  jjs_value_free (to_string);

  return ret;
} /* jjsx_arg_transform_string_common */

/**
 * Transform a JS argument to a cesu8 char array. Type coercion is not allowed.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_string_strict (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                    const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  return jjsx_arg_transform_string_strict_common (js_arg_iter_p, c_arg_p, JJS_ENCODING_CESU8);
} /* jjsx_arg_transform_string_strict */

/**
 * Transform a JS argument to a utf8 char array. Type coercion is not allowed.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_utf8_string_strict (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                         const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  return jjsx_arg_transform_string_strict_common (js_arg_iter_p, c_arg_p, JJS_ENCODING_UTF8);
} /* jjsx_arg_transform_utf8_string_strict */

/**
 * Transform a JS argument to a cesu8 char array. Type coercion is allowed.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_string (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                             const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  return jjsx_arg_transform_string_common (js_arg_iter_p, c_arg_p, JJS_ENCODING_CESU8);
} /* jjsx_arg_transform_string */

/**
 * Transform a JS argument to a utf8 char array. Type coercion is allowed.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_utf8_string (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                  const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  return jjsx_arg_transform_string_common (js_arg_iter_p, c_arg_p, JJS_ENCODING_UTF8);
} /* jjsx_arg_transform_utf8_string */

/**
 * Check whether the JS argument is JJS function, if so, assign to the native argument.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_function (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                               const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  jjs_value_t js_arg = jjsx_arg_js_iterator_pop (js_arg_iter_p);

  if (!jjs_value_is_function (js_arg))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "It is not a function.");
  }

  jjs_value_t *func_p = c_arg_p->dest;
  *func_p = jjs_value_copy (js_arg);

  return jjs_undefined ();
} /* jjsx_arg_transform_function */

/**
 * Check whether the native pointer has the expected type info.
 * If so, assign it to the native argument.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_native_pointer (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                     const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  jjs_value_t js_arg = jjsx_arg_js_iterator_pop (js_arg_iter_p);

  if (!jjs_value_is_object (js_arg))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "It is not an object.");
  }

  const jjs_object_native_info_t *expected_info_p;
  expected_info_p = (const jjs_object_native_info_t *) c_arg_p->extra_info;
  void **ptr_p = (void **) c_arg_p->dest;
  *ptr_p = jjs_object_get_native_ptr (js_arg, expected_info_p);

  if (*ptr_p == NULL)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "The object has no native pointer or type does not match.");
  }

  return jjs_undefined ();
} /* jjsx_arg_transform_native_pointer */

/**
 * Check whether the JS object's properties have expected types, and transform them into native args.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_object_props (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                   const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  jjs_value_t js_arg = jjsx_arg_js_iterator_pop (js_arg_iter_p);

  const jjsx_arg_object_props_t *object_props = (const jjsx_arg_object_props_t *) c_arg_p->extra_info;

  return jjsx_arg_transform_object_properties (js_arg,
                                                 object_props->name_p,
                                                 object_props->name_cnt,
                                                 object_props->c_arg_p,
                                                 object_props->c_arg_cnt);
} /* jjsx_arg_transform_object_props */

/**
 * Check whether the JS array's items have expected types, and transform them into native args.
 *
 * @return JJS undefined: the transformer passes,
 *         JJS error: the transformer fails.
 */
jjs_value_t
jjsx_arg_transform_array_items (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                  const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  jjs_value_t js_arg = jjsx_arg_js_iterator_pop (js_arg_iter_p);

  const jjsx_arg_array_items_t *array_items_p = (const jjsx_arg_array_items_t *) c_arg_p->extra_info;

  return jjsx_arg_transform_array (js_arg, array_items_p->c_arg_p, array_items_p->c_arg_cnt);
} /* jjsx_arg_transform_array_items */

/**
 * Define transformer for optional argument.
 */
#define JJSX_ARG_TRANSFORM_OPTIONAL(type)                                                      \
  jjs_value_t jjsx_arg_transform_##type##_optional (jjsx_arg_js_iterator_t *js_arg_iter_p, \
                                                        const jjsx_arg_t *c_arg_p)             \
  {                                                                                              \
    return jjsx_arg_transform_optional (js_arg_iter_p, c_arg_p, jjsx_arg_transform_##type);  \
  }

JJSX_ARG_TRANSFORM_OPTIONAL (number)
JJSX_ARG_TRANSFORM_OPTIONAL (number_strict)
JJSX_ARG_TRANSFORM_OPTIONAL (boolean)
JJSX_ARG_TRANSFORM_OPTIONAL (boolean_strict)
JJSX_ARG_TRANSFORM_OPTIONAL (string)
JJSX_ARG_TRANSFORM_OPTIONAL (string_strict)
JJSX_ARG_TRANSFORM_OPTIONAL (utf8_string)
JJSX_ARG_TRANSFORM_OPTIONAL (utf8_string_strict)
JJSX_ARG_TRANSFORM_OPTIONAL (function)
JJSX_ARG_TRANSFORM_OPTIONAL (native_pointer)
JJSX_ARG_TRANSFORM_OPTIONAL (object_props)
JJSX_ARG_TRANSFORM_OPTIONAL (array_items)

JJSX_ARG_TRANSFORM_OPTIONAL (uint8)
JJSX_ARG_TRANSFORM_OPTIONAL (uint16)
JJSX_ARG_TRANSFORM_OPTIONAL (uint32)
JJSX_ARG_TRANSFORM_OPTIONAL (int8)
JJSX_ARG_TRANSFORM_OPTIONAL (int16)
JJSX_ARG_TRANSFORM_OPTIONAL (int32)
JJSX_ARG_TRANSFORM_OPTIONAL (int8_strict)
JJSX_ARG_TRANSFORM_OPTIONAL (int16_strict)
JJSX_ARG_TRANSFORM_OPTIONAL (int32_strict)
JJSX_ARG_TRANSFORM_OPTIONAL (uint8_strict)
JJSX_ARG_TRANSFORM_OPTIONAL (uint16_strict)
JJSX_ARG_TRANSFORM_OPTIONAL (uint32_strict)

#undef JJSX_ARG_TRANSFORM_OPTIONAL

/**
 * Ignore the JS argument.
 *
 * @return JJS undefined
 */
jjs_value_t
jjsx_arg_transform_ignore (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                             const jjsx_arg_t *c_arg_p) /**< the native arg */
{
  (void) js_arg_iter_p; /* unused */
  (void) c_arg_p; /* unused */

  return jjs_undefined ();
} /* jjsx_arg_transform_ignore */
