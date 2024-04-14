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

#define TRY_VALID_PMAP(JSON)                                                                           \
  do                                                                                                   \
  {                                                                                                    \
    jjs_value_t result = jjs_pmap (jjs_json_parse_sz (JSON), JJS_MOVE, jjs_string_sz ("."), JJS_MOVE); \
    TEST_ASSERT (jjs_value_is_undefined (result));                                                     \
    jjs_value_free (result);                                                                           \
  } while (0)

static void
test_pmap (void)
{
  TRY_VALID_PMAP ("{\"packages\": {}}");

  TRY_VALID_PMAP ("{\"packages\": { \"a\": \"./index.js\" } }");
  TRY_VALID_PMAP ("{\"packages\": { \"a\": { \"main\": \"./index.js\" } } }");
  TRY_VALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": \"./index.cjs\" } } }");
  TRY_VALID_PMAP ("{\"packages\": { \"a\": { \"module\": \"./index.mjs\" } } }");
  TRY_VALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": \"./index.cjs\" } } } }");
  TRY_VALID_PMAP ("{\"packages\": { \"a\": { \"module\": { \"main\": \"./index.mjs\" } } } }");

  TRY_VALID_PMAP ("{\"packages\": { \"a/b\": { \"path\": \"./b_path\" } } }");
  TRY_VALID_PMAP ("{\"packages\": { \"a/b\": { \"path\": \"./b_path\" } } }");
  TRY_VALID_PMAP ("{\"packages\": { \"a/b\": { \"commonjs\": \"./b_commonjs_path\" } } }");
  TRY_VALID_PMAP ("{\"packages\": { \"a/b\": { \"module\": \"./b_module_path\" } } }");
  TRY_VALID_PMAP ("{\"packages\": { \"a/b\": { \"commonjs\": { \"main\": \"./b_commonjs_path\" } } } }");
  TRY_VALID_PMAP ("{\"packages\": { \"a/b\": { \"module\": { \"main\": \"./b_module_path\" } } } }");
}

#define TRY_INVALID_PMAP(JSON)                                                                         \
  do                                                                                                   \
  {                                                                                                    \
    jjs_value_t result = jjs_pmap (jjs_json_parse_sz (JSON), JJS_MOVE, jjs_string_sz ("."), JJS_MOVE); \
    TEST_ASSERT (jjs_value_is_exception (result));                                                     \
    jjs_value_free (result);                                                                           \
  } while (0)

static void
test_pmap_invalid (void)
{
  TRY_INVALID_PMAP ("");
  TRY_INVALID_PMAP ("null");
  TRY_INVALID_PMAP ("{}");
  TRY_INVALID_PMAP ("[]");

  TRY_INVALID_PMAP ("{\"packages\": null}");
  TRY_INVALID_PMAP ("{\"packages\": [] }");
  TRY_INVALID_PMAP ("{\"packages\": \"\" }");
  TRY_INVALID_PMAP ("{\"packages\": 3 }");

  TRY_INVALID_PMAP ("{\"packages\": { \"a\": \"\" } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": null } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": 3 } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": [] } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": {} } }");

  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"main\": \"\" } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"main\": mull } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"main\": 3 } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"main\": [] } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"main\": {} } }");

  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"path\": \"\" } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"path\": mull } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"path\": 3 } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"path\": [] } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"path\": {} } }");

  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": \"\" } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": null } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": 3 } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": [] } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": {} } } }");

  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": { \"main\": \"\" } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": { \"main\": mull } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": { \"main\": 3 } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": { \"main\": [] } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": { \"main\": {} } } }");

  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": { \"path\": \"\" } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": { \"path\": mull } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": { \"path\": 3 } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": { \"path\": [] } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"module\": { \"path\": {} } } }");

  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": \"\" } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": null } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": 3 } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": [] } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": {} } } }");

  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": \"\" } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": mull } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": 3 } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": [] } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": {} } } }");

  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": { \"path\": \"\" } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": { \"path\": mull } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": { \"path\": 3 } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": { \"path\": [] } } }");
  TRY_INVALID_PMAP ("{\"packages\": { \"a\": { \"commonjs\": { \"path\": {} } } }");
}

#define TRY_INVALID_JSON_ARG(VALUE)                                              \
  do                                                                             \
  {                                                                              \
    jjs_value_t result = jjs_pmap (VALUE, JJS_MOVE, jjs_undefined (), JJS_MOVE); \
    TEST_ASSERT (jjs_value_is_exception (result));                               \
    jjs_value_free (result);                                                     \
  } while (0)

static void
test_pmap_type_error (void)
{
  TRY_INVALID_JSON_ARG (jjs_null ());
  TRY_INVALID_JSON_ARG (jjs_undefined ());
  TRY_INVALID_JSON_ARG (jjs_number (0));
  TRY_INVALID_JSON_ARG (jjs_boolean (true));
  TRY_INVALID_JSON_ARG (jjs_object ());
  TRY_INVALID_JSON_ARG (jjs_array (0));
  TRY_INVALID_JSON_ARG (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG));
}

#define TRY_INVALID_ROOT_ARG(VALUE)                                                                      \
  do                                                                                                     \
  {                                                                                                      \
    jjs_value_t result = jjs_pmap (jjs_json_parse_sz ("{\"packages\": {}}"), JJS_MOVE, VALUE, JJS_MOVE); \
    TEST_ASSERT (jjs_value_is_exception (result));                                                       \
    jjs_value_free (result);                                                                             \
  } while (0)

static void
test_pmap_root_type_error (void)
{
  TRY_INVALID_ROOT_ARG (jjs_null ());
  TRY_INVALID_ROOT_ARG (jjs_number (0));
  TRY_INVALID_ROOT_ARG (jjs_boolean (true));
  TRY_INVALID_ROOT_ARG (jjs_object ());
  TRY_INVALID_ROOT_ARG (jjs_array (0));
  TRY_INVALID_ROOT_ARG (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG));
}

#define TRY_JJS_PMAP_FROM_FILE(VALUE)                          \
  do                                                           \
  {                                                            \
    jjs_value_t value = VALUE;                                 \
    jjs_value_t result = jjs_pmap_from_file (VALUE, JJS_MOVE); \
    TEST_ASSERT (jjs_value_is_exception (result));             \
    jjs_value_free (result);                                   \
    jjs_value_free (value);                                    \
  } while (0)

static void
test_pmap_from_file_error (void)
{
  TRY_JJS_PMAP_FROM_FILE (jjs_string_sz (""));
  TRY_JJS_PMAP_FROM_FILE (jjs_string_sz ("unknown"));
  TRY_JJS_PMAP_FROM_FILE (jjs_string_sz ("./unknown"));
  TRY_JJS_PMAP_FROM_FILE (jjs_string_sz ("../unknown"));
  TRY_JJS_PMAP_FROM_FILE (jjs_string_sz ("/unknown"));

  TRY_JJS_PMAP_FROM_FILE (jjs_null ());
  TRY_JJS_PMAP_FROM_FILE (jjs_undefined ());
  TRY_JJS_PMAP_FROM_FILE (jjs_number (0));
  TRY_JJS_PMAP_FROM_FILE (jjs_boolean (true));
  TRY_JJS_PMAP_FROM_FILE (jjs_object ());
  TRY_JJS_PMAP_FROM_FILE (jjs_array (0));
  TRY_JJS_PMAP_FROM_FILE (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG));
}

#define TRY_JJS_PMAP_RESOLVE(VALUE)                                      \
  do                                                                     \
  {                                                                      \
    jjs_value_t value = VALUE;                                           \
    jjs_module_type_t types[] = {                                        \
      JJS_MODULE_TYPE_NONE,                                              \
      JJS_MODULE_TYPE_MODULE,                                            \
      JJS_MODULE_TYPE_COMMONJS,                                          \
    };                                                                   \
    for (size_t i = 0; i < sizeof (types) / sizeof (types[0]); i++)      \
    {                                                                    \
      jjs_value_t result = jjs_pmap_resolve (value, JJS_KEEP, types[i]); \
      TEST_ASSERT (jjs_value_is_exception (result));                     \
      jjs_value_free (result);                                           \
    }                                                                    \
    jjs_value_free (value);                                              \
  } while (0)

static void
test_pmap_resolve_error (void)
{
  TRY_JJS_PMAP_RESOLVE (jjs_null ());
  TRY_JJS_PMAP_RESOLVE (jjs_undefined ());
  TRY_JJS_PMAP_RESOLVE (jjs_number (0));
  TRY_JJS_PMAP_RESOLVE (jjs_boolean (true));
  TRY_JJS_PMAP_RESOLVE (jjs_object ());
  TRY_JJS_PMAP_RESOLVE (jjs_array (0));
  TRY_JJS_PMAP_RESOLVE (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG));

  TRY_JJS_PMAP_RESOLVE (jjs_string_sz (""));
  TRY_JJS_PMAP_RESOLVE (jjs_string_sz ("unknown"));
  TRY_JJS_PMAP_RESOLVE (jjs_string_sz ("./unknown"));
  TRY_JJS_PMAP_RESOLVE (jjs_string_sz ("../unknown"));
  TRY_JJS_PMAP_RESOLVE (jjs_string_sz ("/unknown"));
}

int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

  test_pmap ();
  test_pmap_invalid ();
  test_pmap_type_error ();
  test_pmap_root_type_error ();
  // positive jjs_pmap_from_file tests in js
  test_pmap_from_file_error ();
  // positive jjs_pmap_resolve tests in js
  test_pmap_resolve_error ();

  jjs_cleanup ();
  return 0;
} /* main */
