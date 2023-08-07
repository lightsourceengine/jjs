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

#include "jjs.h"

#include "test-common.h"

static jjs_value_t
create_special_proxy_handler (const jjs_call_info_t *call_info_p, /**< call information */
                              const jjs_value_t args_p[], /**< argument list */
                              const jjs_length_t args_count) /**< argument count */
{
  JJS_UNUSED (call_info_p);

  if (args_count < 2)
  {
    return jjs_undefined ();
  }

  return jjs_proxy_custom (args_p[0], args_p[1], JJS_PROXY_SKIP_RESULT_VALIDATION);
} /* create_special_proxy_handler */

static void
run_eval (const char *source_p)
{
  jjs_value_t result = jjs_eval ((const jjs_char_t *) source_p, strlen (source_p), 0);

  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);
} /* run_eval */

/**
 * Unit test's main function.
 */
int
main (void)
{
  TEST_INIT ();

  if (!jjs_feature_enabled (JJS_FEATURE_PROXY))
  {
    printf ("Skipping test, Proxy not enabled\n");
    return 0;
  }

  jjs_init (JJS_INIT_EMPTY);

  jjs_value_t global = jjs_current_realm ();

  jjs_value_t function = jjs_function_external (create_special_proxy_handler);
  jjs_value_t name = jjs_string_sz ("create_special_proxy");
  jjs_value_t result = jjs_object_set (global, name, function);
  TEST_ASSERT (!jjs_value_is_exception (result));

  jjs_value_free (result);
  jjs_value_free (name);
  jjs_value_free (function);

  jjs_value_free (global);

  run_eval ("function assert (v) {\n"
            "  if (v !== true)\n"
            "     throw 'Assertion failed!'\n"
            "}");

  /* These tests fail unless JJS_PROXY_SKIP_RESULT_VALIDATION is set. */

  run_eval ("var o = {}\n"
            "Object.preventExtensions(o)\n"
            "var proxy = create_special_proxy(o, {\n"
            "  getPrototypeOf(target) { return Array.prototype }\n"
            "})\n"
            "assert(Object.getPrototypeOf(proxy) === Array.prototype)");

  run_eval ("var o = {}\n"
            "Object.preventExtensions(o)\n"
            "var proxy = create_special_proxy(o, {\n"
            "  setPrototypeOf(target, proto) { return true }\n"
            "})\n"
            "Object.setPrototypeOf(proxy, Array.prototype)");

  run_eval ("var o = {}\n"
            "var proxy = create_special_proxy(o, {\n"
            "  isExtensible(target) { return false }\n"
            "})\n"
            "assert(Object.isExtensible(proxy) === false)");

  run_eval ("var o = {}\n"
            "var proxy = create_special_proxy(o, {\n"
            "  preventExtensions(target) { return true }\n"
            "})\n"
            "Object.preventExtensions(proxy)");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4, enumerable:true })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  getOwnPropertyDescriptor(target, key) {\n"
            "    return { value:5, configurable:true, writable:true }\n"
            "  }\n"
            "})\n"
            "var desc = Object.getOwnPropertyDescriptor(proxy, 'prop')\n"
            "assert(desc.value === 5)\n"
            "assert(desc.configurable === true)\n"
            "assert(desc.enumerable === false)\n"
            "assert(desc.writable === true)\n");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { get() {} })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  defineProperty(target, key, descriptor) { return true }\n"
            "})\n"
            "Object.defineProperty(proxy, 'prop', { value:5 })");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4 })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  has(target, key) { return false }\n"
            "})\n"
            "assert(!Reflect.has(proxy, 'prop'))");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4 })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  get(target, key) { return 5 }\n"
            "})\n"
            "assert(proxy.prop === 5)");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4 })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  set(target, key, value) { return true }\n"
            "})\n"
            "proxy.prop = 8");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4 })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  deleteProperty(target, key) { return true }\n"
            "})\n"
            "assert(delete proxy.prop)");

  run_eval ("var o = {}\n"
            "Object.defineProperty(o, 'prop', { value:4 })\n"
            "var proxy = create_special_proxy(o, {\n"
            "  ownKeys(target) { return [] }\n"
            "})\n"
            "Object.keys(proxy)");

  jjs_cleanup ();
  return 0;
} /* main */
