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

#ifndef JJSX_ARG_H
#define JJSX_ARG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "jjs.h"

JJS_C_API_BEGIN

/**
 * The forward declaration of jjsx_arg_t.
 */
typedef struct jjsx_arg_t jjsx_arg_t;

/**
 * The forward declaration of jjsx_arg_js_iterator_t
 */
typedef struct jjsx_arg_js_iterator_t jjsx_arg_js_iterator_t;

/**
 * Signature of the transform function.
 */
typedef jjs_value_t (*jjsx_arg_transform_func_t) (jjsx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                                      const jjsx_arg_t *c_arg_p); /**< native arg */

/**
 * The structure used in jjsx_arg_object_properties
 */
typedef struct
{
  const jjs_char_t **name_p; /**< property name list of the JS object */
  jjs_length_t name_cnt; /**< count of the name list */
  const jjsx_arg_t *c_arg_p; /**< points to the array of transformation steps */
  jjs_length_t c_arg_cnt; /**< the count of the `c_arg_p` array */
} jjsx_arg_object_props_t;

/**
 * The structure used in jjsx_arg_array
 */
typedef struct
{
  const jjsx_arg_t *c_arg_p; /**< points to the array of transformation steps */
  jjs_length_t c_arg_cnt; /**< the count of the `c_arg_p` array */
} jjsx_arg_array_items_t;

/**
 * The structure defining a single validation & transformation step.
 */
struct jjsx_arg_t
{
  jjsx_arg_transform_func_t func; /**< the transform function */
  void *dest; /**< pointer to destination where func should store the result */
  uintptr_t extra_info; /**< extra information, specific to func */
};

jjs_value_t jjsx_arg_transform_this_and_args (const jjs_value_t this_val,
                                                  const jjs_value_t *js_arg_p,
                                                  const jjs_length_t js_arg_cnt,
                                                  const jjsx_arg_t *c_arg_p,
                                                  jjs_length_t c_arg_cnt);

jjs_value_t jjsx_arg_transform_args (const jjs_value_t *js_arg_p,
                                         const jjs_length_t js_arg_cnt,
                                         const jjsx_arg_t *c_arg_p,
                                         jjs_length_t c_arg_cnt);

jjs_value_t jjsx_arg_transform_object_properties (const jjs_value_t obj_val,
                                                      const jjs_char_t **name_p,
                                                      const jjs_length_t name_cnt,
                                                      const jjsx_arg_t *c_arg_p,
                                                      jjs_length_t c_arg_cnt);
jjs_value_t
jjsx_arg_transform_array (const jjs_value_t array_val, const jjsx_arg_t *c_arg_p, jjs_length_t c_arg_cnt);

/**
 * Indicates whether an argument is allowed to be coerced into the expected JS type.
 */
typedef enum
{
  JJSX_ARG_COERCE, /**< the transform inside will invoke toNumber, toBoolean or toString */
  JJSX_ARG_NO_COERCE /**< the type coercion is not allowed. */
} jjsx_arg_coerce_t;

/**
 * Indicates whether an argument is optional or required.
 */
typedef enum
{
  /**
   * The argument is optional. If the argument is `undefined` the transform is
   * successful and `c_arg_p->dest` remains untouched.
   */
  JJSX_ARG_OPTIONAL,
  /**
   * The argument is required. If the argument is `undefined` the transform
   * will fail and `c_arg_p->dest` remains untouched.
   */
  JJSX_ARG_REQUIRED
} jjsx_arg_optional_t;

/**
 * Indicates the rounding policy which will be chosen to transform an integer.
 */
typedef enum
{
  JJSX_ARG_ROUND, /**< round */
  JJSX_ARG_FLOOR, /**< floor */
  JJSX_ARG_CEIL /**< ceil */
} jjsx_arg_round_t;

/**
 * Indicates the clamping policy which will be chosen to transform an integer.
 * If the policy is NO_CLAMP, and the number is out of range,
 * then the transformer will throw a range error.
 */
typedef enum
{
  JJSX_ARG_CLAMP, /**< clamp the number when it is out of range */
  JJSX_ARG_NO_CLAMP /**< throw a range error */
} jjsx_arg_clamp_t;

/* Inline functions for initializing jjsx_arg_t */

#define JJSX_ARG_INTEGER(type)                                                 \
  static inline jjsx_arg_t jjsx_arg_##type (type##_t *dest,                  \
                                                jjsx_arg_round_t round_flag,   \
                                                jjsx_arg_clamp_t clamp_flag,   \
                                                jjsx_arg_coerce_t coerce_flag, \
                                                jjsx_arg_optional_t opt_flag);

JJSX_ARG_INTEGER (uint8)
JJSX_ARG_INTEGER (int8)
JJSX_ARG_INTEGER (uint16)
JJSX_ARG_INTEGER (int16)
JJSX_ARG_INTEGER (uint32)
JJSX_ARG_INTEGER (int32)

#undef JJSX_ARG_INTEGER

static inline jjsx_arg_t
jjsx_arg_number (double *dest, jjsx_arg_coerce_t coerce_flag, jjsx_arg_optional_t opt_flag);
static inline jjsx_arg_t
jjsx_arg_boolean (bool *dest, jjsx_arg_coerce_t coerce_flag, jjsx_arg_optional_t opt_flag);
static inline jjsx_arg_t
jjsx_arg_string (char *dest, uint32_t size, jjsx_arg_coerce_t coerce_flag, jjsx_arg_optional_t opt_flag);
static inline jjsx_arg_t
jjsx_arg_utf8_string (char *dest, uint32_t size, jjsx_arg_coerce_t coerce_flag, jjsx_arg_optional_t opt_flag);
static inline jjsx_arg_t jjsx_arg_function (jjs_value_t *dest, jjsx_arg_optional_t opt_flag);
static inline jjsx_arg_t
jjsx_arg_native_pointer (void **dest, const jjs_object_native_info_t *info_p, jjsx_arg_optional_t opt_flag);
static inline jjsx_arg_t jjsx_arg_ignore (void);
static inline jjsx_arg_t jjsx_arg_custom (void *dest, uintptr_t extra_info, jjsx_arg_transform_func_t func);
static inline jjsx_arg_t jjsx_arg_object_properties (const jjsx_arg_object_props_t *object_props_p,
                                                         jjsx_arg_optional_t opt_flag);
static inline jjsx_arg_t jjsx_arg_array (const jjsx_arg_array_items_t *array_items_p,
                                             jjsx_arg_optional_t opt_flag);

jjs_value_t jjsx_arg_transform_optional (jjsx_arg_js_iterator_t *js_arg_iter_p,
                                             const jjsx_arg_t *c_arg_p,
                                             jjsx_arg_transform_func_t func);

/* Helper functions for transform functions. */
jjs_value_t jjsx_arg_js_iterator_pop (jjsx_arg_js_iterator_t *js_arg_iter_p);
jjs_value_t jjsx_arg_js_iterator_restore (jjsx_arg_js_iterator_t *js_arg_iter_p);
jjs_value_t jjsx_arg_js_iterator_peek (jjsx_arg_js_iterator_t *js_arg_iter_p);
jjs_length_t jjsx_arg_js_iterator_index (jjsx_arg_js_iterator_t *js_arg_iter_p);

#include "arg.impl.h"

JJS_C_API_END

#endif /* !JJSX_ARG_H */
