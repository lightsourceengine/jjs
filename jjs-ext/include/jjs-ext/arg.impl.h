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

#ifndef JJSX_ARG_IMPL_H
#define JJSX_ARG_IMPL_H

/* transform functions for each type. */

#define JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL(type)                                                               \
  jjs_value_t jjsx_arg_transform_##type (jjsx_arg_js_iterator_t *js_arg_iter_p, const jjsx_arg_t *c_arg_p); \
  jjs_value_t jjsx_arg_transform_##type##_optional (jjsx_arg_js_iterator_t *js_arg_iter_p,                    \
                                                        const jjsx_arg_t *c_arg_p);

#define JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT(type) \
  JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL (type)                 \
  JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL (type##_strict)

JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT (uint8)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT (int8)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT (uint16)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT (int16)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT (uint32)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT (int32)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT (number)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT (string)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT (utf8_string)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT (boolean)

JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL (function)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL (native_pointer)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL (object_props)
JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL (array_items)

jjs_value_t jjsx_arg_transform_ignore (jjsx_arg_js_iterator_t *js_arg_iter_p, const jjsx_arg_t *c_arg_p);

#undef JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL
#undef JJSX_ARG_TRANSFORM_FUNC_WITH_OPTIONAL_AND_STRICT

/**
 * The structure indicates the options used to transform integer argument.
 * It will be passed into jjsx_arg_t's  extra_info field.
 */
typedef struct
{
  uint8_t round; /**< rounding policy */
  uint8_t clamp; /**< clamping policy */
} jjsx_arg_int_option_t;

/**
 * The macro used to generate jjsx_arg_xxx for int type.
 */
#define JJSX_ARG_INT(type)                                                                  \
  static inline jjsx_arg_t jjsx_arg_##type (type##_t *dest,                               \
                                                jjsx_arg_round_t round_flag,                \
                                                jjsx_arg_clamp_t clamp_flag,                \
                                                jjsx_arg_coerce_t coerce_flag,              \
                                                jjsx_arg_optional_t opt_flag)               \
  {                                                                                           \
    jjsx_arg_transform_func_t func;                                                         \
    if (coerce_flag == JJSX_ARG_NO_COERCE)                                                  \
    {                                                                                         \
      if (opt_flag == JJSX_ARG_OPTIONAL)                                                    \
      {                                                                                       \
        func = jjsx_arg_transform_##type##_strict_optional;                                 \
      }                                                                                       \
      else                                                                                    \
      {                                                                                       \
        func = jjsx_arg_transform_##type##_strict;                                          \
      }                                                                                       \
    }                                                                                         \
    else                                                                                      \
    {                                                                                         \
      if (opt_flag == JJSX_ARG_OPTIONAL)                                                    \
      {                                                                                       \
        func = jjsx_arg_transform_##type##_optional;                                        \
      }                                                                                       \
      else                                                                                    \
      {                                                                                       \
        func = jjsx_arg_transform_##type;                                                   \
      }                                                                                       \
    }                                                                                         \
    union                                                                                     \
    {                                                                                         \
      jjsx_arg_int_option_t int_option;                                                     \
      uintptr_t extra_info;                                                                   \
    } u = { .int_option = { .round = (uint8_t) round_flag, .clamp = (uint8_t) clamp_flag } }; \
    return (jjsx_arg_t){ .func = func, .dest = (void *) dest, .extra_info = u.extra_info }; \
  }

JJSX_ARG_INT (uint8)
JJSX_ARG_INT (int8)
JJSX_ARG_INT (uint16)
JJSX_ARG_INT (int16)
JJSX_ARG_INT (uint32)
JJSX_ARG_INT (int32)

#undef JJSX_ARG_INT

/**
 * Create a validation/transformation step (`jjsx_arg_t`) that expects to
 * consume one `number` JS argument and stores it into a C `double`.
 *
 * @return a jjsx_arg_t instance.
 */
static inline jjsx_arg_t
jjsx_arg_number (double *dest, /**< pointer to the double where the result should be stored */
                   jjsx_arg_coerce_t coerce_flag, /**< whether type coercion is allowed */
                   jjsx_arg_optional_t opt_flag) /**< whether the argument is optional */
{
  jjsx_arg_transform_func_t func;

  if (coerce_flag == JJSX_ARG_NO_COERCE)
  {
    if (opt_flag == JJSX_ARG_OPTIONAL)
    {
      func = jjsx_arg_transform_number_strict_optional;
    }
    else
    {
      func = jjsx_arg_transform_number_strict;
    }
  }
  else
  {
    if (opt_flag == JJSX_ARG_OPTIONAL)
    {
      func = jjsx_arg_transform_number_optional;
    }
    else
    {
      func = jjsx_arg_transform_number;
    }
  }

  return (jjsx_arg_t){ .func = func, .dest = (void *) dest };
} /* jjsx_arg_number */

/**
 * Create a validation/transformation step (`jjsx_arg_t`) that expects to
 * consume one `boolean` JS argument and stores it into a C `bool`.
 *
 * @return a jjsx_arg_t instance.
 */
static inline jjsx_arg_t
jjsx_arg_boolean (bool *dest, /**< points to the native bool */
                    jjsx_arg_coerce_t coerce_flag, /**< whether type coercion is allowed */
                    jjsx_arg_optional_t opt_flag) /**< whether the argument is optional */
{
  jjsx_arg_transform_func_t func;

  if (coerce_flag == JJSX_ARG_NO_COERCE)
  {
    if (opt_flag == JJSX_ARG_OPTIONAL)
    {
      func = jjsx_arg_transform_boolean_strict_optional;
    }
    else
    {
      func = jjsx_arg_transform_boolean_strict;
    }
  }
  else
  {
    if (opt_flag == JJSX_ARG_OPTIONAL)
    {
      func = jjsx_arg_transform_boolean_optional;
    }
    else
    {
      func = jjsx_arg_transform_boolean;
    }
  }

  return (jjsx_arg_t){ .func = func, .dest = (void *) dest };
} /* jjsx_arg_boolean */

/**
 * Create a validation/transformation step (`jjsx_arg_t`) that expects to
 * consume one `string` JS argument and stores it into a C `char` array.
 *
 * @return a jjsx_arg_t instance.
 */
static inline jjsx_arg_t
jjsx_arg_string (char *dest, /**< pointer to the native char array where the result should be stored */
                   uint32_t size, /**< the size of native char array */
                   jjsx_arg_coerce_t coerce_flag, /**< whether type coercion is allowed */
                   jjsx_arg_optional_t opt_flag) /**< whether the argument is optional */
{
  jjsx_arg_transform_func_t func;

  if (coerce_flag == JJSX_ARG_NO_COERCE)
  {
    if (opt_flag == JJSX_ARG_OPTIONAL)
    {
      func = jjsx_arg_transform_string_strict_optional;
    }
    else
    {
      func = jjsx_arg_transform_string_strict;
    }
  }
  else
  {
    if (opt_flag == JJSX_ARG_OPTIONAL)
    {
      func = jjsx_arg_transform_string_optional;
    }
    else
    {
      func = jjsx_arg_transform_string;
    }
  }

  return (jjsx_arg_t){ .func = func, .dest = (void *) dest, .extra_info = (uintptr_t) size };
} /* jjsx_arg_string */

/**
 * Create a validation/transformation step (`jjsx_arg_t`) that expects to
 * consume one `string` JS argument and stores it into a C utf8 `char` array.
 *
 * @return a jjsx_arg_t instance.
 */
static inline jjsx_arg_t
jjsx_arg_utf8_string (char *dest, /**< [out] pointer to the native char array where the result should be stored */
                        uint32_t size, /**< the size of native char array */
                        jjsx_arg_coerce_t coerce_flag, /**< whether type coercion is allowed */
                        jjsx_arg_optional_t opt_flag) /**< whether the argument is optional */
{
  jjsx_arg_transform_func_t func;

  if (coerce_flag == JJSX_ARG_NO_COERCE)
  {
    if (opt_flag == JJSX_ARG_OPTIONAL)
    {
      func = jjsx_arg_transform_utf8_string_strict_optional;
    }
    else
    {
      func = jjsx_arg_transform_utf8_string_strict;
    }
  }
  else
  {
    if (opt_flag == JJSX_ARG_OPTIONAL)
    {
      func = jjsx_arg_transform_utf8_string_optional;
    }
    else
    {
      func = jjsx_arg_transform_utf8_string;
    }
  }

  return (jjsx_arg_t){ .func = func, .dest = (void *) dest, .extra_info = (uintptr_t) size };
} /* jjsx_arg_utf8_string */

/**
 * Create a validation/transformation step (`jjsx_arg_t`) that expects to
 * consume one `function` JS argument and stores it into a C `jjs_value_t`.
 *
 * @return a jjsx_arg_t instance.
 */
static inline jjsx_arg_t
jjsx_arg_function (jjs_value_t *dest, /**< pointer to the jjs_value_t where the result should be stored */
                     jjsx_arg_optional_t opt_flag) /**< whether the argument is optional */
{
  jjsx_arg_transform_func_t func;

  if (opt_flag == JJSX_ARG_OPTIONAL)
  {
    func = jjsx_arg_transform_function_optional;
  }
  else
  {
    func = jjsx_arg_transform_function;
  }

  return (jjsx_arg_t){ .func = func, .dest = (void *) dest };
} /* jjsx_arg_function */

/**
 * Create a validation/transformation step (`jjsx_arg_t`) that expects to
 * consume one `object` JS argument that is 'backed' with a native pointer with
 * a given type info. In case the native pointer info matches, the transform
 * will succeed and the object's native pointer will be assigned to *dest.
 *
 * @return a jjsx_arg_t instance.
 */
static inline jjsx_arg_t
jjsx_arg_native_pointer (void **dest, /**< pointer to where the resulting native pointer should be stored */
                           const jjs_object_native_info_t *info_p, /**< expected the type info */
                           jjsx_arg_optional_t opt_flag) /**< whether the argument is optional */
{
  jjsx_arg_transform_func_t func;

  if (opt_flag == JJSX_ARG_OPTIONAL)
  {
    func = jjsx_arg_transform_native_pointer_optional;
  }
  else
  {
    func = jjsx_arg_transform_native_pointer;
  }

  return (jjsx_arg_t){ .func = func, .dest = (void *) dest, .extra_info = (uintptr_t) info_p };
} /* jjsx_arg_native_pointer */

/**
 * Create a jjsx_arg_t instance for ignored argument.
 *
 * @return a jjsx_arg_t instance.
 */
static inline jjsx_arg_t
jjsx_arg_ignore (void)
{
  return (jjsx_arg_t){ .func = jjsx_arg_transform_ignore };
} /* jjsx_arg_ignore */

/**
 * Create a jjsx_arg_t instance with custom transform.
 *
 * @return a jjsx_arg_t instance.
 */
static inline jjsx_arg_t
jjsx_arg_custom (void *dest, /**< pointer to the native argument where the result should be stored */
                   uintptr_t extra_info, /**< the extra parameter, specific to the transform function */
                   jjsx_arg_transform_func_t func) /**< the custom transform function */
{
  return (jjsx_arg_t){ .func = func, .dest = dest, .extra_info = extra_info };
} /* jjsx_arg_custom */

/**
 * Create a jjsx_arg_t instance for object properties.
 *
 * @return a jjsx_arg_t instance.
 */
static inline jjsx_arg_t
jjsx_arg_object_properties (const jjsx_arg_object_props_t *object_props, /**< pointer to object property mapping */
                              jjsx_arg_optional_t opt_flag) /**< whether the argument is optional */
{
  jjsx_arg_transform_func_t func;

  if (opt_flag == JJSX_ARG_OPTIONAL)
  {
    func = jjsx_arg_transform_object_props_optional;
  }
  else
  {
    func = jjsx_arg_transform_object_props;
  }

  return (jjsx_arg_t){ .func = func, .dest = NULL, .extra_info = (uintptr_t) object_props };
} /* jjsx_arg_object_properties */

/**
 * Create a jjsx_arg_t instance for array.
 *
 * @return a jjsx_arg_t instance.
 */
static inline jjsx_arg_t
jjsx_arg_array (const jjsx_arg_array_items_t *array_items_p, /**< pointer to array items mapping */
                  jjsx_arg_optional_t opt_flag) /**< whether the argument is optional */
{
  jjsx_arg_transform_func_t func;

  if (opt_flag == JJSX_ARG_OPTIONAL)
  {
    func = jjsx_arg_transform_array_items_optional;
  }
  else
  {
    func = jjsx_arg_transform_array_items;
  }

  return (jjsx_arg_t){ .func = func, .dest = NULL, .extra_info = (uintptr_t) array_items_p };
} /* jjsx_arg_array */

#endif /* !JJSX_ARG_IMPL_H */
