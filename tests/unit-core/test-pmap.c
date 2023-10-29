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

#include "jjs.h"

#include "config.h"

#define TEST_COMMON_IMPLEMENTATION
#include "test-common.h"

static jjs_value_t
from_json_sz (const char* json)
{
  jjs_value_t input = jjs_string_sz (json);
  jjs_value_t root = jjs_string_sz (".");
  jjs_value_t result = jjs_pmap_from_json (input, root);

  jjs_value_free (root);
  jjs_value_free (input);

  return result;
}

#define TRY_VALID_PMAP(JSON)                       \
  do                                               \
  {                                                \
    jjs_value_t result = from_json_sz(JSON);       \
    print_if_exception (result);                   \
    TEST_ASSERT (jjs_value_is_true (result));      \
    jjs_value_free (result);                       \
  } while (0)

static void
test_valid_pmap (void)
{
  TRY_VALID_PMAP("{\"packages\": {}}");

  TRY_VALID_PMAP("{\"packages\": { \"a\": \"./index.js\" } }");
  TRY_VALID_PMAP("{\"packages\": { \"a\": { \"main\": \"./index.js\" } } }");
  TRY_VALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": \"./index.cjs\" } } }");
  TRY_VALID_PMAP("{\"packages\": { \"a\": { \"module\": \"./index.mjs\" } } }");
  TRY_VALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": { \"main\": \"./index.cjs\" } } } }");
  TRY_VALID_PMAP("{\"packages\": { \"a\": { \"module\": { \"main\": \"./index.mjs\" } } } }");

  TRY_VALID_PMAP("{\"packages\": { \"a/b\": { \"path\": \"./b_path\" } } }");
  TRY_VALID_PMAP("{\"packages\": { \"a/b\": { \"path\": \"./b_path\" } } }");
  TRY_VALID_PMAP("{\"packages\": { \"a/b\": { \"commonjs\": \"./b_commonjs_path\" } } }");
  TRY_VALID_PMAP("{\"packages\": { \"a/b\": { \"module\": \"./b_module_path\" } } }");
  TRY_VALID_PMAP("{\"packages\": { \"a/b\": { \"commonjs\": { \"main\": \"./b_commonjs_path\" } } } }");
  TRY_VALID_PMAP("{\"packages\": { \"a/b\": { \"module\": { \"main\": \"./b_module_path\" } } } }");
}

#define TRY_INVALID_PMAP(JSON)                     \
  do                                               \
  {                                                \
    jjs_value_t result = from_json_sz(JSON);       \
    TEST_ASSERT (jjs_value_is_exception (result)); \
    jjs_value_free (result);                       \
  } while (0)

static void
test_invalid_pmap (void)
{
  TRY_INVALID_PMAP("");
  TRY_INVALID_PMAP("null");
  TRY_INVALID_PMAP("{}");
  TRY_INVALID_PMAP("[]");

  TRY_INVALID_PMAP("{\"packages\": null}");
  TRY_INVALID_PMAP("{\"packages\": [] }");
  TRY_INVALID_PMAP("{\"packages\": \"\" }");
  TRY_INVALID_PMAP("{\"packages\": 3 }");

  TRY_INVALID_PMAP("{\"packages\": { \"a\": \"\" } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": null } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": 3 } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": [] } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": {} } }");

  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"main\": \"\" } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"main\": mull } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"main\": 3 } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"main\": [] } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"main\": {} } }");

  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"path\": \"\" } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"path\": mull } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"path\": 3 } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"path\": [] } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"path\": {} } }");

  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": \"\" } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": null } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": 3 } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": [] } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": {} } } }");

  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": { \"main\": \"\" } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": { \"main\": mull } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": { \"main\": 3 } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": { \"main\": [] } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": { \"main\": {} } } }");

   TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": { \"path\": \"\" } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": { \"path\": mull } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": { \"path\": 3 } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": { \"path\": [] } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"module\": { \"path\": {} } } }");
  
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": \"\" } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": null } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": 3 } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": [] } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": {} } } }");
  
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": { \"main\": \"\" } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": { \"main\": mull } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": { \"main\": 3 } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": { \"main\": [] } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": { \"main\": {} } } }");
  
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": { \"path\": \"\" } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": { \"path\": mull } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": { \"path\": 3 } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": { \"path\": [] } } }");
  TRY_INVALID_PMAP("{\"packages\": { \"a\": { \"commonjs\": { \"path\": {} } } }");
}

#define TRY_INVALID_JSON_ARG(VALUE)                        \
  do                                                       \
  {                                                        \
    jjs_value_t root = jjs_string_sz(".");                 \
    jjs_value_t value = VALUE;                             \
    jjs_value_t result = jjs_pmap_from_json (value, root); \
    TEST_ASSERT (jjs_value_is_exception (result));         \
    jjs_value_free (result);                               \
    jjs_value_free (root);                                 \
    jjs_value_free (value);                                \
  } while (0)

static void
test_invalid_json_arg (void)
{
  TRY_INVALID_JSON_ARG (jjs_null ());
  TRY_INVALID_JSON_ARG (jjs_undefined ());
  TRY_INVALID_JSON_ARG (jjs_number (0));
  TRY_INVALID_JSON_ARG (jjs_boolean (true));
  TRY_INVALID_JSON_ARG (jjs_object ());
  TRY_INVALID_JSON_ARG (jjs_array (0));
  TRY_INVALID_JSON_ARG (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG));
}

#define TRY_INVALID_ROOT_ARG(VALUE)                           \
  do                                                          \
  {                                                           \
    jjs_value_t input = jjs_string_sz ("{\"packages\": {}}"); \
    jjs_value_t value = VALUE;                                \
    jjs_value_t result = jjs_pmap_from_json (input, value);   \
    TEST_ASSERT (jjs_value_is_exception (result));            \
    jjs_value_free (result);                                  \
    jjs_value_free (input);                                   \
    jjs_value_free (value);                                   \
  } while (0)

static void
test_invalid_root_arg (void)
{
  TRY_INVALID_ROOT_ARG (jjs_null ());
  TRY_INVALID_ROOT_ARG (jjs_undefined ());
  TRY_INVALID_ROOT_ARG (jjs_number (0));
  TRY_INVALID_ROOT_ARG (jjs_boolean (true));
  TRY_INVALID_ROOT_ARG (jjs_object ());
  TRY_INVALID_ROOT_ARG (jjs_array (0));
  TRY_INVALID_ROOT_ARG (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG));
}

int
main (void)
{
 TEST_INIT ();

 jjs_init (JJS_INIT_EMPTY);

 test_valid_pmap ();
 test_invalid_pmap ();
 test_invalid_json_arg ();
 test_invalid_root_arg ();

 jjs_cleanup ();
 return 0;
} /* main */
