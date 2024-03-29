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

#ifndef JJSX_AUTORELEASE_H
#define JJSX_AUTORELEASE_H

JJS_C_API_BEGIN

#include "autorelease.impl.h"

/*
 * Macro for `const jjs_value_t` for which jjs_value_free () is
 * automatically called when the variable goes out of scope.
 *
 * Example usage:
 * static void foo (bool enable)
 * {
 *   JJSX_AR_VALUE_T bar = jjs_string (...);
 *
 *   if (enable) {
 *     JJSX_AR_VALUE_T baz = jjs_current_realm ();
 *
 *     // ...
 *
 *     // jjs_value_free (baz) and jjs_value_free (bar) is called automatically before
 *     // returning, because `baz` and `bar` go out of scope.
 *     return;
 *   }
 *
 *   // jjs_value_free (bar) is called automatically when the function returns,
 *   // because `bar` goes out of scope.
 * }
 */
#define JJSX_AR_VALUE_T __JJSX_AR_VALUE_T_IMPL

JJS_C_API_END

#endif /* !JJSX_AUTORELEASE_H */
