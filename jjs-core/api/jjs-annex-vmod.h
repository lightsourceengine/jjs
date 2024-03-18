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

#ifndef JJS_ANNEX_VMOD_H
#define JJS_ANNEX_VMOD_H

#include "ecma-globals.h"

typedef enum jjs_annex_vmod_format_t
{
  JJS_ANNEX_VMOD_FORMAT_OBJECT,
  JJS_ANNEX_VMOD_FORMAT_MODULE,
  JJS_ANNEX_VMOD_FORMAT_COMMONJS,
  JJS_ANNEX_VMOD_FORMAT_UNKNOWN,
} jjs_annex_vmod_format_t;

void jjs_annex_vmod_init_realm (ecma_value_t realm);
bool jjs_annex_vmod_exists (ecma_value_t name);
jjs_value_t jjs_annex_vmod_resolve (jjs_value_t name);

#endif /* !JJS_ANNEX_VMOD_H */
