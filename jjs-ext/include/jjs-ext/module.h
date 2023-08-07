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

#ifndef JJSX_MODULE_H
#define JJSX_MODULE_H

#include "jjs.h"

JJS_C_API_BEGIN

/**
 * Declare the signature for the module initialization function.
 */
typedef jjs_value_t (*jjsx_native_module_on_resolve_t) (void);

/**
 * Declare the structure used to define a module. One should only make use of this structure via the
 * JJSX_NATIVE_MODULE macro declared below.
 */
typedef struct jjsx_native_module_t
{
  const jjs_char_t *name_p; /**< name of the module */
  const jjsx_native_module_on_resolve_t on_resolve_p; /**< function that returns a new instance of the module */
  struct jjsx_native_module_t *next_p; /**< pointer to next module in the list */
} jjsx_native_module_t;

/**
 * Declare the constructor and destructor attributes. These evaluate to nothing if this extension is built without
 * library constructor/destructor support.
 */
#ifdef ENABLE_INIT_FINI
#ifdef _MSC_VER
/**
 * Only Visual Studio 2008 and upper version support for __pragma keyword
 * refer to https://msdn.microsoft.com/en-us/library/d9x1s805(v=vs.90).aspx
 */
#if _MSC_VER >= 1500
#ifdef _WIN64
#define JJSX_MSVC_INCLUDE_SYM(s) comment (linker, "/include:" #s)
#else /* !_WIN64 */
#define JJSX_MSVC_INCLUDE_SYM(s) comment (linker, "/include:_" #s)
#endif /* _WIN64 */

#ifdef __cplusplus
#define JJSX_MSCV_EXTERN_C extern "C"
#else /* !__cplusplus */
#define JJSX_MSCV_EXTERN_C
#endif /* __cplusplus */

#pragma section(".CRT$XCU", read)
#pragma section(".CRT$XTU", read)

#define JJSX_MSVC_FUNCTION_ON_SECTION(sec_name, f)                               \
  static void f (void);                                                            \
  __pragma (JJSX_MSVC_INCLUDE_SYM (f##_section)) __declspec(allocate (sec_name)) \
    JJSX_MSCV_EXTERN_C void (*f##_section) (void) = f;                           \
  static void f (void)

#define JJSX_MODULE_CONSTRUCTOR(f) JJSX_MSVC_FUNCTION_ON_SECTION (".CRT$XCU", f)
#define JJSX_MODULE_DESTRUCTOR(f)  JJSX_MSVC_FUNCTION_ON_SECTION (".CRT$XTU", f)
#else /* !(_MSC_VER >= 1500) */
#error "Only Visual Studio 2008 and upper version are supported."
#endif /* _MSC_VER >= 1500 */
#elif defined(__GNUC__)
#define JJSX_MODULE_CONSTRUCTOR(f)                  \
  static void f (void) __attribute__ ((constructor)); \
  static void f (void)

#define JJSX_MODULE_DESTRUCTOR(f)                  \
  static void f (void) __attribute__ ((destructor)); \
  static void f (void)
#else /* __GNUC__ */
#error "`FEATURE_INIT_FINI` build flag isn't supported on this compiler"
#endif /* _MSC_VER */
#else /* !ENABLE_INIT_FINI */
#define JJSX_MODULE_CONSTRUCTOR(f) \
  void f (void);                     \
  void f (void)

#define JJSX_MODULE_DESTRUCTOR(f) \
  void f (void);                    \
  void f (void)
#endif /* ENABLE_INIT_FINI */

/**
 * Having two levels of macros allows strings to be used unquoted.
 */
#define JJSX_NATIVE_MODULE(module_name, on_resolve_cb) JJSX_NATIVE_MODULE_IMPLEM (module_name, on_resolve_cb)

#define JJSX_NATIVE_MODULE_IMPLEM(module_name, on_resolve_cb)                                          \
  static jjsx_native_module_t _##module_name##_definition = { .name_p = (jjs_char_t *) #module_name, \
                                                                .on_resolve_p = (on_resolve_cb),         \
                                                                .next_p = NULL };                        \
                                                                                                         \
  JJSX_MODULE_CONSTRUCTOR (module_name##_register)                                                     \
  {                                                                                                      \
    jjsx_native_module_register (&_##module_name##_definition);                                        \
  }                                                                                                      \
                                                                                                         \
  JJSX_MODULE_DESTRUCTOR (module_name##_unregister)                                                    \
  {                                                                                                      \
    jjsx_native_module_unregister (&_##module_name##_definition);                                      \
  }

/**
 * Register a native module. This makes it available for loading via jjsx_module_resolve, when
 * jjsx_module_native_resolver is passed in as a possible resolver.
 */
void jjsx_native_module_register (jjsx_native_module_t *module_p);

/**
 * Unregister a native module. This removes the module from the list of available native modules, meaning that
 * subsequent calls to jjsx_module_resolve with jjsx_module_native_resolver will not be able to find it.
 */
void jjsx_native_module_unregister (jjsx_native_module_t *module_p);

/**
 * Declare the function pointer type for canonical name resolution.
 */
typedef jjs_value_t (*jjsx_module_get_canonical_name_t) (const jjs_value_t name); /**< The name for which to
                                                                                         *   compute the canonical
                                                                                         *   name */

/**
 * Declare the function pointer type for module resolution.
 */
typedef bool (*jjsx_module_resolve_t) (const jjs_value_t canonical_name, /**< The module's canonical name */
                                         jjs_value_t *result); /**< The resulting module, if the function returns
                                                                  *   true */

/**
 * Declare the structure for module resolvers.
 */
typedef struct
{
  jjsx_module_get_canonical_name_t get_canonical_name_p; /**< function pointer to establish the canonical name of a
                                                            *   module */
  jjsx_module_resolve_t resolve_p; /**< function pointer to resolve a module */
} jjsx_module_resolver_t;

/**
 * Declare the JJS module resolver so that it may be added to an array of jjsx_module_resolver_t items and
 * thus passed to jjsx_module_resolve.
 */
extern jjsx_module_resolver_t jjsx_module_native_resolver;

/**
 * Load a copy of a module into the current context using the provided module resolvers, or return one that was already
 * loaded if it is found.
 */
jjs_value_t
jjsx_module_resolve (const jjs_value_t name, const jjsx_module_resolver_t **resolvers, size_t count);

/**
 * Delete a module from the cache or, if name has the JavaScript value of undefined, clear the entire cache.
 */
void jjsx_module_clear_cache (const jjs_value_t name, const jjsx_module_resolver_t **resolvers, size_t count);

JJS_C_API_END

#endif /* !JJSX_MODULE_H */
