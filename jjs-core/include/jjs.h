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

#ifndef JJS_H
#define JJS_H

/**
 * Major version of JJS API.
 */
#define JJS_API_MAJOR_VERSION 4

/**
 * Minor version of JJS API.
 */
#define JJS_API_MINOR_VERSION 5

/**
 * Patch version of JJS API.
 */
#define JJS_API_PATCH_VERSION 0

#define JJS_STRINGIFY(X) #X
#define JJS_STRINGIFY2(X) JJS_STRINGIFY (X)

/**
 * Patch version string in dot separated semver format.
 */
#define JJS_API_VERSION_STRING JJS_STRINGIFY2 (JJS_API_MAJOR_VERSION) "." JJS_STRINGIFY2 (JJS_API_MINOR_VERSION) "." JJS_STRINGIFY2 (JJS_API_PATCH_VERSION)

#include "jjs-core.h"
#include "jjs-debugger.h"
#include "jjs-snapshot.h"

#endif /* !JJS_H */
