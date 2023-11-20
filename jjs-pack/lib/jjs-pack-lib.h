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

typedef jjs_value_t (*jjs_pack_bindings_cb_t) (void);

jjs_value_t jjs_pack_lib_load_from_snapshot (uint8_t* source,
                                             jjs_size_t source_size,
                                             jjs_pack_bindings_cb_t bindings,
                                             bool vmod_wrap);

jjs_value_t jjs_pack_lib_global_set_from_snapshot (const char* id_p,
                                                   uint8_t* source_p,
                                                   jjs_size_t source_size,
                                                   jjs_pack_bindings_cb_t bindings);

jjs_value_t jjs_pack_lib_vmod_sz (const char* name_p, jjs_vmod_create_cb_t create_cb);

jjs_value_t jjs_pack_lib_global_has_sz (const char* id_p);
void jjs_pack_lib_global_set_sz (const char* id_p, jjs_value_t value);
void jjs_pack_lib_add_is_windows (jjs_value_t object);

void jjs_pack_lib_set_function_sz (jjs_value_t bindings, const char* name, jjs_external_handler_t handler);

#define JJS_UNUSED(x) (void)(x)
#define JJS_HANDLER(NAME) \
  jjs_value_t NAME (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_cnt)

#define JJS_PACK_DEFINE_EXTERN_SOURCE(NS) \
  extern uint8_t NS##_snapshot[]; \
  extern const uint32_t NS##_snapshot_len;

#define JJS_PACK_LIB_GLOBAL_SET(ID, NS, BINDINGS) \
  jjs_pack_lib_global_set_from_snapshot (ID, NS ## _snapshot, NS ## _snapshot_len, BINDINGS)

#define JJS_PACK_LIB_VMOD_SETUP(NS, BINDINGS) \
  static jjs_value_t NS ## _vmod_setup (jjs_value_t name, void* user_p) \
  { \
    JJS_UNUSED (name); \
    JJS_UNUSED (user_p); \
    return jjs_pack_lib_load_from_snapshot (NS ## _snapshot, NS ## _snapshot_len, BINDINGS, true); \
  }

#define JJS_READ_STRING(IDENT, VALUE, SSIZE) \
  uint8_t IDENT##_stack_buffer[SSIZE]; \
  uint8_t* IDENT##_p; \
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
