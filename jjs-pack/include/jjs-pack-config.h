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

#ifndef JJS_PACK_CONFIG_H
#define JJS_PACK_CONFIG_H

#ifndef JJS_PACK
#define JJS_PACK 0
#endif /* !defined (JJS_PACK) */

#ifndef JJS_PACK_CONSOLE
#define JJS_PACK_CONSOLE JJS_PACK
#endif /* !defined (JJS_PACK_CONSOLE) */

#ifndef JJS_PACK_DOMEXCEPTION
#define JJS_PACK_DOMEXCEPTION JJS_PACK
#endif /* !defined (JJS_PACK_DOMEXCEPTION) */

#ifndef JJS_PACK_FS
#define JJS_PACK_FS JJS_PACK
#endif /* !defined (JJS_PACK_FS) */

#ifndef JJS_PACK_PATH
#define JJS_PACK_PATH JJS_PACK
#endif /* !defined (JJS_PACK_PATH) */

#ifndef JJS_PACK_PATH_URL
#define JJS_PACK_PATH_URL JJS_PACK
#endif /* !defined (JJS_PACK_PATH_URL) */

#ifndef JJS_PACK_PERFORMANCE
#define JJS_PACK_PERFORMANCE JJS_PACK
#endif /* !defined (JJS_PACK_PERFORMANCE) */

#ifndef JJS_PACK_TEXT
#define JJS_PACK_TEXT JJS_PACK
#endif /* !defined (JJS_PACK_TEXT) */

#ifndef JJS_PACK_URL
#define JJS_PACK_URL JJS_PACK
#endif /* !defined (JJS_PACK_URL) */

#if (JJS_PACK_CONSOLE != 0) && (JJS_PACK_CONSOLE != 1)
#error "Invalid value for 'JJS_PACK_CONSOLE' macro."
#endif /* (JJS_PACK_CONSOLE != 0) && (JJS_PACK_CONSOLE != 1) */

#if (JJS_PACK_DOMEXCEPTION != 0) && (JJS_PACK_DOMEXCEPTION != 1)
#error "Invalid value for 'JJS_PACK_DOMEXCEPTION' macro."
#endif /* (JJS_PACK_DOMEXCEPTION != 0) && (JJS_PACK_DOMEXCEPTION != 1) */

#if (JJS_PACK_FS != 0) && (JJS_PACK_FS != 1)
#error "Invalid value for 'JJS_PACK_FS' macro."
#endif /* (JJS_PACK_FS != 0) && (JJS_PACK_FS != 1) */

#if (JJS_PACK_PATH != 0) && (JJS_PACK_PATH != 1)
#error "Invalid value for 'JJS_PACK_PATH' macro."
#endif /* (JJS_PACK_PATH != 0) && (JJS_PACK_PATH != 1) */

#if (JJS_PACK_PATH_URL != 0) && (JJS_PACK_PATH_URL != 1)
#error "Invalid value for 'JJS_PACK_PATH_URL' macro."
#endif /* (JJS_PACK_PATH_URL != 0) && (JJS_PACK_PATH_URL != 1) */

#if (JJS_PACK_PERFORMANCE != 0) && (JJS_PACK_PERFORMANCE != 1)
#error "Invalid value for 'JJS_PACK_PERFORMANCE' macro."
#endif /* (JJS_PACK_PERFORMANCE != 0) && (JJS_PACK_PERFORMANCE != 1) */

#if (JJS_PACK_TEXT != 0) && (JJS_PACK_TEXT != 1)
#error "Invalid value for 'JJS_PACK_TEXT' macro."
#endif /* (JJS_PACK_TEXT != 0) && (JJS_PACK_TEXT != 1) */

#if (JJS_PACK_URL != 0) && (JJS_PACK_URL != 1)
#error "Invalid value for 'JJS_PACK_URL' macro."
#endif /* (JJS_PACK_URL != 0) && (JJS_PACK_URL != 1) */

#endif /* !JJS_PACK_CONFIG_H */
