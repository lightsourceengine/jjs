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

#include <string.h>

#include "jjs.h"

#include "jjs-ext/module.h"
#include "test-common.h"

/* Load a module. */
const char eval_string1[] = "require ('my_custom_module');";

/* Load a module using a different resolver. */
const char eval_string2[] = "require ('differently-handled-module');";

/* Load a broken module using the built-in resolver. */
const char eval_string3[] = "(function() {"
                            "  var theError;"
                            "  try {"
                            "    require ('my_broken_module');"
                            "  } catch (anError) {"
                            "    theError = anError;"
                            "  }"
                            "  return (((theError.message === 'Module on_resolve () must not be NULL') &&"
                            "    (theError.moduleName === 'my_broken_module') &&"
                            "    (theError instanceof TypeError)) ? 1 : 0);"
                            "}) ();";

/* Load a non-existent module. */
const char eval_string4[] = "(function() {"
                            "  var theError;"
                            "  try {"
                            "    require ('some_missing_module_xyzzy');"
                            "  } catch (anError) {"
                            "    theError = anError;"
                            "  }"
                            "  return (((theError.message === 'Module not found') &&"
                            "    (theError.moduleName === 'some_missing_module_xyzzy')) ? 1 : 0);"
                            "}) ();";

/* Make sure the result of a module load is cached. */
const char eval_string5[] = "(function() {"
                            "  var x = require('cache-check');"
                            "  var y = require('cache-check');"
                            "  return x === y ? 1 : 0;"
                            "}) ();";

/* Make sure the result of a module load is removed from the cache. */
const char eval_string6[] = "(function() {"
                            "  var x = require('cache-check');"
                            "  clear_require_cache('cache-check');"
                            "  var y = require('cache-check');"
                            "  return x !== y ? 1 : 0;"
                            "}) ();";

/* Make sure the entire cache is cleared. */
const char eval_string7[] = "(function() {"
                            "  var x = require('cache-check');"
                            "  clear_require_cache(undefined);"
                            "  var y = require('cache-check');"
                            "  return x !== y ? 1 : 0;"
                            "}) ();";

/*
 * Define a resolver for a module named "differently-handled-module" to check that custom resolvers work.
 */
static bool
resolve_differently_handled_module (const jjs_value_t name, jjs_value_t *result)
{
  jjs_size_t name_size = jjs_string_size (name, JJS_ENCODING_UTF8);
  JJS_VLA (jjs_char_t, name_string, name_size);
  jjs_string_to_buffer (name, JJS_ENCODING_UTF8, name_string, name_size);

  if (!strncmp ((char *) name_string, "differently-handled-module", name_size))
  {
    (*result) = jjs_number (29);
    return true;
  }
  return false;
} /* resolve_differently_handled_module */

static jjsx_module_resolver_t differently_handled_module_resolver = { NULL, resolve_differently_handled_module };

/*
 * Define module "cache-check" via its own resolver as an empty object. Since objects are accessible only via references
 * we can strictly compare the object returned on subsequent attempts at loading "cache-check" with the object returned
 * on the first attempt and establish that the two are in fact the same object - which in turn shows that caching works.
 */
static bool
cache_check (const jjs_value_t name, jjs_value_t *result)
{
  jjs_size_t name_size = jjs_string_size (name, JJS_ENCODING_UTF8);
  JJS_VLA (jjs_char_t, name_string, name_size);
  jjs_string_to_buffer (name, JJS_ENCODING_UTF8, name_string, name_size);

  if (!strncmp ((char *) name_string, "cache-check", name_size))
  {
    (*result) = jjs_object ();
    return true;
  }
  return false;
} /* cache_check */

static jjsx_module_resolver_t cache_check_resolver = { NULL, cache_check };

static const jjsx_module_resolver_t *resolvers[3] = { &jjsx_module_native_resolver,
                                                        &differently_handled_module_resolver,
                                                        &cache_check_resolver };

static jjs_value_t
handle_clear_require_cache (const jjs_call_info_t *call_info_p,
                            const jjs_value_t args_p[],
                            const jjs_length_t args_count)
{
  (void) call_info_p;
  (void) args_count;

  TEST_ASSERT (args_count == 1);
  jjsx_module_clear_cache (args_p[0], resolvers, 3);

  return 0;
} /* handle_clear_require_cache */

static jjs_value_t
handle_require (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_count)
{
  (void) call_info_p;
  (void) args_count;

  jjs_value_t return_value = 0;

  TEST_ASSERT (args_count == 1);
  return_value = jjsx_module_resolve (args_p[0], resolvers, 3);

  return return_value;
} /* handle_require */

static void
assert_number (jjs_value_t js_value, double expected_result)
{
  TEST_ASSERT (!jjs_value_is_exception (js_value));
  TEST_ASSERT (jjs_value_as_number (js_value) == expected_result);
} /* assert_number */

static void
eval_one (const char *the_string, double expected_result)
{
  jjs_value_t js_eval_result =
    jjs_eval ((const jjs_char_t *) the_string, strlen (the_string), JJS_PARSE_STRICT_MODE);
  assert_number (js_eval_result, expected_result);
  jjs_value_free (js_eval_result);
} /* eval_one */

#ifndef ENABLE_INIT_FINI
extern void my_broken_module_register (void);
extern void my_custom_module_register (void);
#endif /* !ENABLE_INIT_FINI */

int
main (int argc, char **argv)
{
  (void) argc;
  (void) argv;
  jjs_value_t js_global = 0, js_function = 0, js_property_name = 0;
  jjs_value_t res;

#ifndef ENABLE_INIT_FINI
  my_broken_module_register ();
  my_custom_module_register ();
#endif /* !ENABLE_INIT_FINI */

  jjs_init (JJS_INIT_EMPTY);

  js_global = jjs_current_realm ();

  js_function = jjs_function_external (handle_require);
  js_property_name = jjs_string_sz ("require");
  res = jjs_object_set (js_global, js_property_name, js_function);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_boolean (res) && jjs_value_is_true (res));
  jjs_value_free (res);
  jjs_value_free (js_property_name);
  jjs_value_free (js_function);

  js_function = jjs_function_external (handle_clear_require_cache);
  js_property_name = jjs_string_sz ("clear_require_cache");
  res = jjs_object_set (js_global, js_property_name, js_function);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_boolean (res) && jjs_value_is_true (res));
  jjs_value_free (res);
  jjs_value_free (js_property_name);
  jjs_value_free (js_function);

  jjs_value_free (js_global);

  eval_one (eval_string1, 42);
  eval_one (eval_string2, 29);
  eval_one (eval_string3, 1);
  eval_one (eval_string4, 1);
  eval_one (eval_string5, 1);
  eval_one (eval_string6, 1);
  eval_one (eval_string7, 1);

  jjs_cleanup ();
} /* main */
