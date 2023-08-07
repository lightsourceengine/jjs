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

#include "jjs-ext/properties.h"

#include "jjs-core.h"

/**
 * Register a JavaScript function in the global object.
 *
 * @return true - if the operation was successful,
 *         false - otherwise.
 */
bool
jjsx_register_global (const char *name_p, /**< name of the function */
                        jjs_external_handler_t handler_p) /**< function callback */
{
  jjs_value_t global_obj_val = jjs_current_realm ();
  jjs_value_t function_name_val = jjs_string_sz (name_p);
  jjs_value_t function_val = jjs_function_external (handler_p);

  jjs_value_t result_val = jjs_object_set (global_obj_val, function_name_val, function_val);
  bool result = jjs_value_is_true (result_val);

  jjs_value_free (result_val);
  jjs_value_free (function_val);
  jjs_value_free (function_name_val);
  jjs_value_free (global_obj_val);

  return result;
} /* jjsx_register_global */

/**
 * Set multiple properties on a target object.
 *
 * The properties are an array of (name, property value) pairs and
 * this list must end with a (NULL, 0) entry.
 *
 * Notes:
 *  - Each property value in the input array is released after a successful property registration.
 *  - The property name must be a zero terminated UTF-8 string.
 *  - There should be no '\0' (NULL) character in the name excluding the string terminator.
 *  - The method `jjsx_release_property_entry` must be called if there is any failed registration
 *    to release the values in the entries array.
 *
 * @return `jjsx_register_result` struct - if everything is ok with the (undefined, property entry count) values.
 *         In case of error the (error object, registered property count) pair.
 */
jjsx_register_result
jjsx_set_properties (const jjs_value_t target_object, /**< target object */
                       const jjsx_property_entry entries[]) /**< array of method entries */
{
#define JJSX_SET_PROPERTIES_RESULT(VALUE, IDX) ((jjsx_register_result){ VALUE, IDX })
  uint32_t idx = 0;

  if (entries == NULL)
  {
    return JJSX_SET_PROPERTIES_RESULT (jjs_undefined (), 0);
  }

  for (; (entries[idx].name != NULL); idx++)
  {
    const jjsx_property_entry *entry = &entries[idx];

    jjs_value_t prop_name = jjs_string_sz (entry->name);
    jjs_value_t result = jjs_object_set (target_object, prop_name, entry->value);

    jjs_value_free (prop_name);

    // By API definition:
    // The jjs_object_set returns TRUE if there is no problem
    // and error object if there is any problem.
    // Thus there is no need to check if the boolean value is false or not.
    if (!jjs_value_is_boolean (result))
    {
      return JJSX_SET_PROPERTIES_RESULT (result, idx);
    }

    jjs_value_free (entry->value);
    jjs_value_free (result);
  }

  return JJSX_SET_PROPERTIES_RESULT (jjs_undefined (), idx);
#undef JJSX_SET_PROPERTIES_RESULT
} /* jjsx_set_properties */

/**
 * Release all jjs_value_t in a jjsx_property_entry array based on
 * a previous jjsx_set_properties call.
 *
 * In case of a successful registration it is safe to call this method.
 */
void
jjsx_release_property_entry (const jjsx_property_entry entries[], /**< list of property entries */
                               const jjsx_register_result register_result) /**< previous result of registration */
{
  if (entries == NULL)
  {
    return;
  }

  for (uint32_t idx = register_result.registered; entries[idx].name != NULL; idx++)
  {
    jjs_value_free (entries[idx].value);
  }
} /* jjsx_release_property_entry */
