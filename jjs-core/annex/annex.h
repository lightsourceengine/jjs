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

#ifndef ANNEX_H
#define ANNEX_H

#include "ecma-objects.h"

typedef enum annex_specifier_type
{
  ANNEX_SPECIFIER_TYPE_NONE,
  ANNEX_SPECIFIER_TYPE_RELATIVE,
  ANNEX_SPECIFIER_TYPE_ABSOLUTE,
  ANNEX_SPECIFIER_TYPE_PACKAGE,
  ANNEX_SPECIFIER_TYPE_FILE_URL,
} annex_specifier_type_t;

typedef struct ecma_cstr_s
{
  char* str_p;
  lit_utf8_size_t size;
} ecma_cstr_t;

annex_specifier_type_t annex_path_specifier_type (jjs_context_t* context_p, ecma_value_t specifier);
ecma_value_t annex_path_join (jjs_context_t* context_p, ecma_value_t referrer, ecma_value_t specifier, bool normalize);
ecma_value_t annex_path_normalize (jjs_context_t* context_p, ecma_value_t path);
ecma_value_t annex_path_cwd (jjs_context_t* context_p);
ecma_value_t annex_path_format (jjs_context_t* context_p, ecma_value_t path);
ecma_value_t annex_path_dirname (jjs_context_t* context_p, ecma_value_t path);
ecma_value_t annex_path_to_file_url (jjs_context_t* context_p, ecma_value_t path);
ecma_value_t annex_path_basename (jjs_context_t* context_p, ecma_value_t path);

#define ecma_get_global_object(ctx) ((ecma_global_object_t*)ecma_builtin_get_global (ctx))
#define ecma_create_object_with_null_proto(ctx) ecma_make_object_value (ctx, ecma_create_object (ctx, NULL, 0, ECMA_OBJECT_TYPE_GENERAL))
#define ecma_arg0(ARGV, ARGC) (0 < (ARGC) ? (ARGV)[0] : ECMA_VALUE_UNDEFINED)
#define ecma_arg1(ARGV, ARGC) (1 < (ARGC) ? (ARGV)[1] : ECMA_VALUE_UNDEFINED)

void ecma_set_m (ecma_context_t* context_p, ecma_value_t object, lit_magic_string_id_t name_id, ecma_value_t value);
void ecma_set_v (ecma_value_t object, ecma_context_t* context_p, ecma_value_t key, ecma_value_t value);
void ecma_set_index_v (ecma_context_t* context_p, ecma_value_t object, ecma_length_t index, ecma_value_t value);
ecma_value_t ecma_find_own_m (ecma_context_t* context_p, ecma_value_t object, lit_magic_string_id_t key);
ecma_value_t ecma_find_own_v (ecma_context_t* context_p, ecma_value_t object, ecma_value_t key);
bool ecma_has_own_m (ecma_context_t* context_p, ecma_value_t object, lit_magic_string_id_t key);
bool ecma_has_own_v (ecma_context_t* context_p, ecma_value_t object, ecma_value_t key);
ecma_value_t ecma_string_ascii_sz (ecma_context_t* context_p, const char* string_p);

void annex_util_set_internal_m (jjs_context_t* context_p, ecma_value_t object, lit_magic_string_id_t key, ecma_value_t value);
ecma_value_t annex_util_get_internal_m (jjs_context_t* context_p, ecma_value_t object, lit_magic_string_id_t key);

void annex_util_define_function (jjs_context_t* context_p,
                                 ecma_object_t* global_p,
                                 lit_magic_string_id_t name_id,
                                 ecma_native_handler_t handler_p);
void annex_util_define_value (jjs_context_t* context_p,
                              ecma_object_t* object_p,
                              lit_magic_string_id_t name_id,
                              ecma_value_t value,
                              jjs_own_t value_o);
void annex_util_define_ro_value (jjs_context_t* context_p,
                                 ecma_object_t* object_p,
                                 lit_magic_string_id_t name_id,
                                 ecma_value_t value,
                                 jjs_own_t value_o);

bool annex_util_is_valid_package_name (jjs_context_t *context_p, ecma_value_t name);
jjs_value_t annex_util_create_string_utf8_sz (jjs_context_t* context_p, const char* str_p);

#endif /* !ANNEX_H */
