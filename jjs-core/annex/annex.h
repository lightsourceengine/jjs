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

annex_specifier_type_t annex_path_specifier_type (ecma_value_t specifier);
ecma_value_t annex_path_join (ecma_value_t referrer, ecma_value_t specifier, bool normalize);
ecma_value_t annex_path_normalize (ecma_value_t path);
ecma_value_t annex_path_cwd (void);
ecma_value_t annex_path_format (ecma_value_t path);
ecma_value_t annex_path_dirname (ecma_value_t path);
ecma_value_t annex_path_from_file_url (ecma_value_t file_url);

#define ecma_get_global_object() ((ecma_global_object_t*)ecma_builtin_get_global ())
#define ecma_create_object_with_null_proto() ecma_make_object_value (ecma_create_object (NULL, 0, ECMA_OBJECT_TYPE_GENERAL))

void ecma_set_m (ecma_value_t object, lit_magic_string_id_t name_id, ecma_value_t value);
void ecma_set_v (ecma_value_t object, ecma_value_t key, ecma_value_t value);
ecma_value_t ecma_find_own_m (ecma_value_t object, lit_magic_string_id_t key);
ecma_value_t ecma_find_own_v (ecma_value_t object, ecma_value_t key);
bool ecma_has_own_m (ecma_value_t object, lit_magic_string_id_t key);
bool ecma_has_own_v (ecma_value_t object, ecma_value_t key);
ecma_value_t ecma_string_ascii_sz (const char* string_p);
ecma_cstr_t ecma_string_to_cstr (ecma_value_t value);
void ecma_free_cstr (ecma_cstr_t* cstr_p);

void annex_util_define_function (ecma_object_t* global_p,
                                 lit_magic_string_id_t name_id,
                                 ecma_native_handler_t handler_p);
void annex_util_define_value (ecma_object_t* global_p,
                              lit_magic_string_id_t name_id,
                              ecma_value_t value);
jjs_value_t annex_util_create_string_utf8_sz (const char* str_p);

#endif /* !ANNEX_H */
