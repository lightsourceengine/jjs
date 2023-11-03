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

#endif /* !JJS_PACK_LIB_H */
