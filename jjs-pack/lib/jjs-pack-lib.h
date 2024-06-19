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

jjs_value_t jjs_pack_console_init (jjs_context_t *context_p);
jjs_value_t jjs_pack_domexception_init (jjs_context_t *context_p);
jjs_value_t jjs_pack_fs_init (jjs_context_t *context_p);
jjs_value_t jjs_pack_path_init (jjs_context_t *context_p);
jjs_value_t jjs_pack_performance_init (jjs_context_t *context_p);
jjs_value_t jjs_pack_text_init (jjs_context_t *context_p);
jjs_value_t jjs_pack_url_init (jjs_context_t *context_p);

jjs_value_t jjs_pack_lib_main (jjs_context_t *context_p, uint8_t* source, jjs_size_t source_size, jjs_value_t bindings, jjs_own_t bindings_o);

jjs_value_t jjs_pack_lib_main_vmod (jjs_context_t *context_p, const char* package_name, jjs_external_handler_t vmod_callback);

jjs_value_t jjs_pack_lib_read_exports (jjs_context_t *context_p,
                                       uint8_t* source,
                                       jjs_size_t source_size,
                                       jjs_value_t bindings,
                                       jjs_own_t bindings_o,
                                       jjs_pack_lib_exports_format_t exports_format);

#define jjs_bindings(CTX) jjs_object (CTX)
void jjs_bindings_function (jjs_context_t *context_p, jjs_value_t bindings, const char* name, jjs_external_handler_t function_p);
void jjs_bindings_value (jjs_context_t *context_p, jjs_value_t bindings, const char* name, jjs_value_t value, jjs_own_t value_o);

uint64_t jjs_pack_platform_hrtime (void);
double jjs_pack_platform_date_now (void);

#define JJS_UNUSED(x) (void) (x)

#define JJS_HANDLER(NAME) \
  jjs_value_t NAME (const jjs_call_info_t* call_info_p, const jjs_value_t args_p[], jjs_length_t args_cnt)

#define JJS_HANDLER_HEADER() \
  JJS_UNUSED (call_info_p);  \
  JJS_UNUSED (args_p);       \
  JJS_UNUSED (args_cnt)

#define JJS_READ_STRING(CTX, IDENT, VALUE, SSIZE)                                                   \
  uint8_t IDENT##_stack_buffer[SSIZE];                                                              \
  uint8_t* IDENT##_p;                                                                               \
  bool IDENT##_free_buffer;                                                                         \
  jjs_size_t IDENT##_size = jjs_string_size ((CTX), VALUE, JJS_ENCODING_UTF8);                      \
                                                                                                    \
  if (IDENT##_size < (SSIZE))                                                                       \
  {                                                                                                 \
    jjs_size_t IDENT##_written =                                                                    \
      jjs_string_to_buffer ((CTX), VALUE, JJS_ENCODING_UTF8, &IDENT##_stack_buffer[0], (SSIZE) -1u);\
    IDENT##_stack_buffer[IDENT##_written] = '\0';                                                   \
    IDENT##_p = &IDENT##_stack_buffer[0];                                                           \
    IDENT##_free_buffer = false;                                                                    \
  }                                                                                                 \
  else                                                                                              \
  {                                                                                                 \
    IDENT##_p = malloc ((SSIZE) + 1u);                                                              \
    jjs_size_t IDENT##_written = jjs_string_to_buffer (                                             \
      (CTX), VALUE, JJS_ENCODING_UTF8, IDENT##_p, SSIZE);                                           \
    IDENT##_p[IDENT##_written] = '\0';                                                              \
    IDENT##_free_buffer = true;                                                                     \
  }

#define JJS_READ_STRING_FREE(IDENT) \
  if (IDENT##_free_buffer)          \
  {                                 \
    free (IDENT##_p);               \
  }

#define JJS_ARG(CTX, IDENT, INDEX, EXPECT)                                        \
  jjs_value_t IDENT = args_cnt > (INDEX) ? args_p[INDEX] : jjs_undefined (CTX);   \
  do                                                                              \
  {                                                                               \
    if (!EXPECT ((CTX), IDENT))                                                   \
    {                                                                             \
      return jjs_throw_sz ((CTX), JJS_ERROR_TYPE, "Invalid argument.");           \
    }                                                                             \
  } while (0)

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#define JJS_OS_IS_WINDOWS
#elif defined(_AIX) || defined(__TOS_AIX__)
#define JJS_OS_IS_AIX
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#define JJS_OS_IS_LINUX
#elif defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#define JJS_OS_IS_MACOS
#endif

#if defined(JJS_OS_IS_LINUX) || defined(JJS_OS_IS_MACOS) || defined(JJS_OS_IS_AIX)
#define JJS_OS_IS_UNIX
#endif

JJS_HANDLER (jjs_pack_hrtime_handler);
JJS_HANDLER (jjs_pack_hrtime_bigint_handler);
JJS_HANDLER (jjs_pack_date_now_handler);

#endif /* !JJS_PACK_LIB_H */
