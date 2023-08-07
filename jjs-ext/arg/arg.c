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

#include "jjs-ext/arg.h"

#include "jjs.h"

#include "arg-internal.h"
#include "jext-common.h"

JJSX_STATIC_ASSERT (sizeof (jjsx_arg_int_option_t) <= sizeof (((jjsx_arg_t *) 0)->extra_info),
                      jjsx_arg_number_options_t_must_fit_into_extra_info);

/**
 * Validate the JS arguments and assign them to the native arguments.
 *
 * @return JJS undefined: all validators passed,
 *         JJS error: a validator failed.
 */
jjs_value_t
jjsx_arg_transform_args (const jjs_value_t *js_arg_p, /**< points to the array with JS arguments */
                           const jjs_length_t js_arg_cnt, /**< the count of the `js_arg_p` array */
                           const jjsx_arg_t *c_arg_p, /**< points to the array of validation/transformation steps */
                           jjs_length_t c_arg_cnt) /**< the count of the `c_arg_p` array */
{
  jjs_value_t ret = jjs_undefined ();

  jjsx_arg_js_iterator_t iterator = { .js_arg_p = js_arg_p, .js_arg_cnt = js_arg_cnt, .js_arg_idx = 0 };

  for (; c_arg_cnt != 0 && !jjs_value_is_exception (ret); c_arg_cnt--, c_arg_p++)
  {
    ret = c_arg_p->func (&iterator, c_arg_p);
  }

  return ret;
} /* jjsx_arg_transform_args */

/**
 * Validate the this value and the JS arguments,
 * and assign them to the native arguments.
 * This function is useful to perform input validation inside external
 * function handlers (see jjs_external_handler_t).
 * @note this_val is processed as the first value, before the array of arguments.
 *
 * @return JJS undefined: all validators passed,
 *         JJS error: a validator failed.
 */
jjs_value_t
jjsx_arg_transform_this_and_args (const jjs_value_t this_val, /**< the this_val for the external function */
                                    const jjs_value_t *js_arg_p, /**< points to the array with JS arguments */
                                    const jjs_length_t js_arg_cnt, /**< the count of the `js_arg_p` array */
                                    const jjsx_arg_t *c_arg_p, /**< points to the array of transformation steps */
                                    jjs_length_t c_arg_cnt) /**< the count of the `c_arg_p` array */
{
  if (c_arg_cnt == 0)
  {
    return jjs_undefined ();
  }

  jjsx_arg_js_iterator_t iterator = { .js_arg_p = &this_val, .js_arg_cnt = 1, .js_arg_idx = 0 };

  jjs_value_t ret = c_arg_p->func (&iterator, c_arg_p);

  if (jjs_value_is_exception (ret))
  {
    jjs_value_free (ret);

    return jjs_throw_sz (JJS_ERROR_TYPE, "'this' validation failed.");
  }

  return jjsx_arg_transform_args (js_arg_p, js_arg_cnt, c_arg_p + 1, c_arg_cnt - 1);
} /* jjsx_arg_transform_this_and_args */

/**
 * Validate the `obj_val`'s properties,
 * and assign them to the native arguments.
 *
 * @return JJS undefined: all validators passed,
 *         JJS error: a validator failed.
 */
jjs_value_t
jjsx_arg_transform_object_properties (const jjs_value_t obj_val, /**< the JS object */
                                        const jjs_char_t **name_p, /**< property name list of the JS object */
                                        const jjs_length_t name_cnt, /**< count of the name list */
                                        const jjsx_arg_t *c_arg_p, /**< points to the array of transformation steps */
                                        jjs_length_t c_arg_cnt) /**< the count of the `c_arg_p` array */
{
  if (!jjs_value_is_object (obj_val))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Not an object.");
  }

  JJS_VLA (jjs_value_t, prop, name_cnt);

  for (jjs_length_t i = 0; i < name_cnt; i++, name_p++)
  {
    const jjs_value_t name_str = jjs_string_sz ((char *) (*name_p));
    prop[i] = jjs_object_get (obj_val, name_str);
    jjs_value_free (name_str);

    if (jjs_value_is_exception (prop[i]))
    {
      for (jjs_length_t j = 0; j < i; j++)
      {
        jjs_value_free (prop[j]);
      }

      return prop[i];
    }
  }

  const jjs_value_t ret = jjsx_arg_transform_args (prop, name_cnt, c_arg_p, c_arg_cnt);

  for (jjs_length_t i = 0; i < name_cnt; i++)
  {
    jjs_value_free (prop[i]);
  }

  return ret;
} /* jjsx_arg_transform_object_properties */

/**
 * Validate the items in the JS array and assign them to the native arguments.
 *
 * @return JJS undefined: all validators passed,
 *         JJS error: a validator failed.
 */
jjs_value_t
jjsx_arg_transform_array (const jjs_value_t array_val, /**< points to the JS array */
                            const jjsx_arg_t *c_arg_p, /**< points to the array of validation/transformation steps */
                            jjs_length_t c_arg_cnt) /**< the count of the `c_arg_p` array */
{
  if (!jjs_value_is_array (array_val))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Not an array.");
  }

  JJS_VLA (jjs_value_t, arr, c_arg_cnt);

  for (jjs_length_t i = 0; i < c_arg_cnt; i++)
  {
    arr[i] = jjs_object_get_index (array_val, i);

    if (jjs_value_is_exception (arr[i]))
    {
      for (jjs_length_t j = 0; j < i; j++)
      {
        jjs_value_free (arr[j]);
      }

      return arr[i];
    }
  }

  const jjs_value_t ret = jjsx_arg_transform_args (arr, c_arg_cnt, c_arg_p, c_arg_cnt);

  for (jjs_length_t i = 0; i < c_arg_cnt; i++)
  {
    jjs_value_free (arr[i]);
  }

  return ret;
} /* jjsx_arg_transform_array */
