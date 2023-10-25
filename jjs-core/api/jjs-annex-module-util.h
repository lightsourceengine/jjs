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

#ifndef JJS_ANNEX_MODULE_UTIL_H
#define JJS_ANNEX_MODULE_UTIL_H

#include "ecma-globals.h"

typedef struct jjs_annex_module_resolve_t
{
  jjs_value_t path;
  jjs_value_t format;
  jjs_value_t result;
} jjs_annex_module_resolve_t;

typedef struct jjs_annex_module_load_t
{
  jjs_value_t source;
  jjs_value_t format;
  jjs_value_t result;
} jjs_annex_module_load_t;

jjs_annex_module_resolve_t jjs_annex_module_resolve (ecma_value_t request,
                                                     ecma_value_t referrer_path,
                                                     jjs_module_type_t module_type);
void jjs_annex_module_resolve_free (jjs_annex_module_resolve_t *resolve_result_p);

jjs_annex_module_load_t jjs_annex_module_load (ecma_value_t path,
                                               ecma_value_t format,
                                               jjs_module_type_t module_type);
void jjs_annex_module_load_free (jjs_annex_module_load_t *load_result_p);

#endif /* !JJS_ANNEX_MODULE_UTIL_H */
