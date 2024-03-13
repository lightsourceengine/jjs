/* Copyright Light Source Software, LLC and other contributors.
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

#ifndef JJS_PACK_LIB_H
#define JJS_PACK_LIB_H

#include "jjs.h"

typedef enum jjs_pack_lib_exports_format_t
{
  JJS_PACK_LIB_EXPORTS_FORMAT_VMOD,
  JJS_PACK_LIB_EXPORTS_FORMAT_OBJECT,
} jjs_pack_lib_exports_format_t;

jjs_value_t jjs_pack_console_init (void);
jjs_value_t jjs_pack_domexception_init (void);
jjs_value_t jjs_pack_fs_init (void);
jjs_value_t jjs_pack_path_init (void);
jjs_value_t jjs_pack_performance_init (void);
jjs_value_t jjs_pack_text_init (void);
jjs_value_t jjs_pack_url_init (void);

jjs_value_t jjs_pack_lib_main (uint8_t* source, jjs_size_t source_size, jjs_value_t bindings, bool bindings_move);

jjs_value_t jjs_pack_lib_main_vmod (const char* package_name, jjs_external_handler_t vmod_callback);

jjs_value_t jjs_pack_lib_read_exports (uint8_t* source,
                                       jjs_size_t source_size,
                                       jjs_value_t bindings,
                                       bool bindings_move,
                                       jjs_pack_lib_exports_format_t exports_format);

#define jjs_bindings() jjs_object ()
void jjs_bindings_function (jjs_value_t bindings, const char* name, jjs_external_handler_t function_p);
void jjs_bindings_number (jjs_value_t bindings, const char* name, double number);

#define JJS_UNUSED(x) (void) (x)

#define JJS_HANDLER(NAME) \
  jjs_value_t NAME (const jjs_call_info_t* call_info_p, const jjs_value_t args_p[], jjs_length_t args_cnt)
#define JJS_HANDLER_HEADER() \
  JJS_UNUSED (call_info_p);  \
  JJS_UNUSED (args_p);       \
  JJS_UNUSED (args_cnt)

#define JJS_READ_STRING(IDENT, VALUE, SSIZE)                                                        \
  uint8_t IDENT##_stack_buffer[SSIZE];                                                              \
  uint8_t* IDENT##_p;                                                                               \
  bool IDENT##_free_buffer; \
  jjs_size_t IDENT##_size = jjs_string_size (VALUE, JJS_ENCODING_UTF8); \
  \
  if (IDENT##_size < (SSIZE)) \
  { \
    jjs_size_t IDENT##_written = jjs_string_to_buffer (VALUE, JJS_ENCODING_UTF8, &IDENT##_stack_buffer[0], (SSIZE) - 1u); \
    IDENT##_stack_buffer[IDENT##_written] = '\0'; \
    IDENT##_p = &IDENT##_stack_buffer[0]; \
    IDENT##_free_buffer = false; \
  } \
  else \
  { \
    IDENT##_p = malloc ((SSIZE) + 1u); \
    jjs_size_t IDENT##_written = jjs_string_to_buffer (VALUE, JJS_ENCODING_UTF8, IDENT##_p, SSIZE); \
    IDENT##_p[IDENT##_written] = '\0'; \
    IDENT##_free_buffer = true; \
  }

#define JJS_READ_STRING_FREE(IDENT) \
  if (IDENT##_free_buffer) \
  { \
    free (IDENT##_p); \
  }

#define JJS_ARG(IDENT, INDEX, EXPECT) \
  jjs_value_t IDENT = args_cnt > (INDEX) ? args_p[INDEX] : jjs_undefined (); \
  do { \
    if (!EXPECT (IDENT)) { \
      return jjs_throw_sz(JJS_ERROR_TYPE, "Invalid argument.");\
    } \
  } while (0)

#endif /* !JJS_PACK_LIB_H */
