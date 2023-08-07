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

#ifndef JJSX_PROPERTIES_H
#define JJSX_PROPERTIES_H

#include "jjs-types.h"

JJS_C_API_BEGIN

/*
 * Handler registration helper
 */

bool jjsx_register_global (const char *name_p, jjs_external_handler_t handler_p);

/**
 * Struct used by the `jjsx_set_functions` method to
 * register multiple methods for a given object.
 */
typedef struct
{
  const char *name; /**< name of the property to add */
  jjs_value_t value; /**< value of the property */
} jjsx_property_entry;

#define JJSX_PROPERTY_NUMBER(NAME, NUMBER) \
  (jjsx_property_entry)                    \
  {                                          \
    NAME, jjs_number (NUMBER)              \
  }
#define JJSX_PROPERTY_STRING(NAME, STR, SIZE)                                \
  (jjsx_property_entry)                                                      \
  {                                                                            \
    NAME, jjs_string ((const jjs_char_t *) STR, SIZE, JJS_ENCODING_UTF8) \
  }
#define JJSX_PROPERTY_STRING_SZ(NAME, STR) \
  (jjsx_property_entry)                    \
  {                                          \
    NAME, jjs_string_sz (STR)              \
  }
#define JJSX_PROPERTY_BOOLEAN(NAME, VALUE) \
  (jjsx_property_entry)                    \
  {                                          \
    NAME, jjs_boolean (VALUE)              \
  }
#define JJSX_PROPERTY_FUNCTION(NAME, FUNC) \
  (jjsx_property_entry)                    \
  {                                          \
    NAME, jjs_function_external (FUNC)     \
  }
#define JJSX_PROPERTY_UNDEFINED(NAME) \
  (jjsx_property_entry)               \
  {                                     \
    NAME, jjs_undefined ()            \
  }
#define JJSX_PROPERTY_LIST_END() \
  (jjsx_property_entry)          \
  {                                \
    NULL, 0                        \
  }

/**
 * Stores the result of property register operation.
 */
typedef struct
{
  jjs_value_t result; /**< result of property registration (undefined or error object) */
  uint32_t registered; /**< number of successfully registered methods */
} jjsx_register_result;

jjsx_register_result jjsx_set_properties (const jjs_value_t target_object, const jjsx_property_entry entries[]);

void jjsx_release_property_entry (const jjsx_property_entry entries[],
                                    const jjsx_register_result register_result);

JJS_C_API_END

#endif /* !JJSX_PROPERTIES_H */
