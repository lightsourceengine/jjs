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

#include "jjs-test.h"

#define TRY_PMAP_PARSE_VALID(JSON)                                                                     \
  do                                                                                                   \
  {                                                                                                    \
    jjs_value_t result = jjs_pmap (ctx (), jjs_json_parse_sz (ctx (), JSON), JJS_MOVE, jjs_string_sz (ctx (), "."), JJS_MOVE); \
    TEST_ASSERT (jjs_value_is_undefined (ctx (), result));                                                     \
    jjs_value_free (ctx (), result);                                                                           \
  } while (0)

static void
test_pmap (void)
{
  TRY_PMAP_PARSE_VALID ("{\"packages\": {}}");

  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a\": \"./index.js\" } }");
  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a\": { \"main\": \"./index.js\" } } }");
  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a\": { \"commonjs\": \"./index.cjs\" } } }");
  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a\": { \"module\": \"./index.mjs\" } } }");
  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": \"./index.cjs\" } } } }");
  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a\": { \"module\": { \"main\": \"./index.mjs\" } } } }");

  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a/b\": { \"path\": \"./b_path\" } } }");
  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a/b\": { \"path\": \"./b_path\" } } }");
  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a/b\": { \"commonjs\": \"./b_commonjs_path\" } } }");
  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a/b\": { \"module\": \"./b_module_path\" } } }");
  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a/b\": { \"commonjs\": { \"main\": \"./b_commonjs_path\" } } } }");
  TRY_PMAP_PARSE_VALID ("{\"packages\": { \"a/b\": { \"module\": { \"main\": \"./b_module_path\" } } } }");
}

#define TRY_PMAP_PARSE_INVALID(JSON)                                                                   \
  do                                                                                                   \
  {                                                                                                    \
    jjs_value_t result = jjs_pmap (ctx (), jjs_json_parse_sz (ctx (), JSON), JJS_MOVE, jjs_string_sz (ctx (), "."), JJS_MOVE); \
    TEST_ASSERT (jjs_value_is_exception (ctx (), result));                                                     \
    jjs_value_free (ctx (), result);                                                                           \
  } while (0)

static void
test_pmap_invalid (void)
{
  TRY_PMAP_PARSE_INVALID ("");
  TRY_PMAP_PARSE_INVALID ("null");
  TRY_PMAP_PARSE_INVALID ("{}");
  TRY_PMAP_PARSE_INVALID ("[]");

  TRY_PMAP_PARSE_INVALID ("{\"packages\": null}");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": [] }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": \"\" }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": 3 }");

  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": \"\" } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": null } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": 3 } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": [] } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": {} } }");

  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"main\": \"\" } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"main\": mull } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"main\": 3 } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"main\": [] } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"main\": {} } }");

  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"path\": \"\" } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"path\": mull } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"path\": 3 } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"path\": [] } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"path\": {} } }");

  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": \"\" } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": null } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": 3 } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": [] } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": {} } } }");

  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": { \"main\": \"\" } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": { \"main\": mull } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": { \"main\": 3 } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": { \"main\": [] } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": { \"main\": {} } } }");

  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": { \"path\": \"\" } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": { \"path\": mull } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": { \"path\": 3 } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": { \"path\": [] } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"module\": { \"path\": {} } } }");

  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": \"\" } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": null } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": 3 } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": [] } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": {} } } }");

  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": \"\" } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": mull } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": 3 } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": [] } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": { \"main\": {} } } }");

  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": { \"path\": \"\" } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": { \"path\": mull } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": { \"path\": 3 } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": { \"path\": [] } } }");
  TRY_PMAP_PARSE_INVALID ("{\"packages\": { \"a\": { \"commonjs\": { \"path\": {} } } }");
}

#define TRY_INVALID_JSON_ARG(VALUE)                                              \
  do                                                                             \
  {                                                                              \
    jjs_value_t result = jjs_pmap (ctx (), VALUE, JJS_MOVE, jjs_undefined (ctx ()), JJS_MOVE); \
    TEST_ASSERT (jjs_value_is_exception (ctx (), result));                               \
    jjs_value_free (ctx (), result);                                                     \
  } while (0)

static void
test_pmap_type_error (void)
{
  TRY_INVALID_JSON_ARG (jjs_null (ctx ()));
  TRY_INVALID_JSON_ARG (jjs_undefined (ctx ()));
  TRY_INVALID_JSON_ARG (jjs_number (ctx (), 0));
  TRY_INVALID_JSON_ARG (jjs_boolean (ctx (), true));
  TRY_INVALID_JSON_ARG (jjs_object (ctx ()));
  TRY_INVALID_JSON_ARG (jjs_array (ctx (), 0));
  TRY_INVALID_JSON_ARG (jjs_symbol_get_well_known (ctx (), JJS_SYMBOL_TO_STRING_TAG));
}

#define TRY_INVALID_ROOT_ARG(VALUE)                                                                      \
  do                                                                                                     \
  {                                                                                                      \
    jjs_value_t result = jjs_pmap (ctx (), jjs_json_parse_sz (ctx (), "{\"packages\": {}}"), JJS_MOVE, VALUE, JJS_MOVE); \
    TEST_ASSERT (jjs_value_is_exception (ctx (), result));                                                       \
    jjs_value_free (ctx (), result);                                                                             \
  } while (0)

static void
test_pmap_root_type_error (void)
{
  TRY_INVALID_ROOT_ARG (jjs_null (ctx ()));
  TRY_INVALID_ROOT_ARG (jjs_number (ctx (), 0));
  TRY_INVALID_ROOT_ARG (jjs_boolean (ctx (), true));
  TRY_INVALID_ROOT_ARG (jjs_object (ctx ()));
  TRY_INVALID_ROOT_ARG (jjs_array (ctx (), 0));
  TRY_INVALID_ROOT_ARG (jjs_symbol_get_well_known (ctx (), JJS_SYMBOL_TO_STRING_TAG));
}

#define TRY_JJS_PMAP_FROM_FILE(VALUE)                                            \
  do                                                                             \
  {                                                                              \
    jjs_value_t value = VALUE;                                                   \
    jjs_value_t result = jjs_pmap (ctx (), VALUE, JJS_MOVE, jjs_undefined (ctx ()), JJS_MOVE); \
    TEST_ASSERT (jjs_value_is_exception (ctx (), result));                               \
    jjs_value_free (ctx (), result);                                                     \
    jjs_value_free (ctx (), value);                                                      \
  } while (0)

static void
test_pmap_from_file_error (void)
{
  TRY_JJS_PMAP_FROM_FILE (jjs_string_sz (ctx (), ""));
  TRY_JJS_PMAP_FROM_FILE (jjs_string_sz (ctx (), "unknown"));
  TRY_JJS_PMAP_FROM_FILE (jjs_string_sz (ctx (), "./unknown"));
  TRY_JJS_PMAP_FROM_FILE (jjs_string_sz (ctx (), "../unknown"));
  TRY_JJS_PMAP_FROM_FILE (jjs_string_sz (ctx (), "/unknown"));

  TRY_JJS_PMAP_FROM_FILE (jjs_null (ctx ()));
  TRY_JJS_PMAP_FROM_FILE (jjs_undefined (ctx ()));
  TRY_JJS_PMAP_FROM_FILE (jjs_number (ctx (), 0));
  TRY_JJS_PMAP_FROM_FILE (jjs_boolean (ctx (), true));
  TRY_JJS_PMAP_FROM_FILE (jjs_object (ctx ()));
  TRY_JJS_PMAP_FROM_FILE (jjs_array (ctx (), 0));
  TRY_JJS_PMAP_FROM_FILE (jjs_symbol_get_well_known (ctx (), JJS_SYMBOL_TO_STRING_TAG));
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
      jjs_value_t result = jjs_pmap_resolve (ctx (), value, JJS_KEEP, types[i]); \
      TEST_ASSERT (jjs_value_is_exception (ctx (), result));                     \
      jjs_value_free (ctx (), result);                                           \
    }                                                                    \
    jjs_value_free (ctx (), value);                                              \
  } while (0)

static void
test_pmap_resolve_error (void)
{
  TRY_JJS_PMAP_RESOLVE (jjs_null (ctx ()));
  TRY_JJS_PMAP_RESOLVE (jjs_undefined (ctx ()));
  TRY_JJS_PMAP_RESOLVE (jjs_number (ctx (), 0));
  TRY_JJS_PMAP_RESOLVE (jjs_boolean (ctx (), true));
  TRY_JJS_PMAP_RESOLVE (jjs_object (ctx ()));
  TRY_JJS_PMAP_RESOLVE (jjs_array (ctx (), 0));
  TRY_JJS_PMAP_RESOLVE (jjs_symbol_get_well_known (ctx (), JJS_SYMBOL_TO_STRING_TAG));

  TRY_JJS_PMAP_RESOLVE (jjs_string_sz (ctx (), ""));
  TRY_JJS_PMAP_RESOLVE (jjs_string_sz (ctx (), "unknown"));
  TRY_JJS_PMAP_RESOLVE (jjs_string_sz (ctx (), "./unknown"));
  TRY_JJS_PMAP_RESOLVE (jjs_string_sz (ctx (), "../unknown"));
  TRY_JJS_PMAP_RESOLVE (jjs_string_sz (ctx (), "/unknown"));
}

static jjs_value_t
join_with_cwd (const char *base)
{
  jjs_value_t components[] = {
    jjs_platform_cwd (ctx ()),
    jjs_string_sz (ctx (), base),
  };

  jjs_value_t raw = jjs_fmt_join_v (ctx (), jjs_string_sz (ctx (), "/"), JJS_MOVE, components, JJS_ARRAY_SIZE (components));
  jjs_value_t result = jjs_platform_realpath (ctx (), raw, JJS_MOVE);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));

  for (size_t i = 0; i < JJS_ARRAY_SIZE (components); i++)
  {
    jjs_value_free (ctx (), components[i]);
  }

  return result;
}

static void
expect_filename (jjs_value_t actual, jjs_value_ownership_t actual_o, const char *expected_base)
{
  jjs_value_t expected = join_with_cwd (expected_base);

  TEST_ASSERT (strict_equals (ctx (), actual, expected));
  jjs_value_free (ctx (), expected);

  if (actual_o == JJS_MOVE)
  {
    jjs_value_free (ctx (), actual);
  }
}

static void
try_pmap_parse (const char *json)
{
  jjs_value_t result = jjs_json_parse_sz (ctx (), json);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));

  jjs_pmap (ctx (), result, JJS_MOVE, join_with_cwd ("./unit-fixtures/pmap"), JJS_MOVE);
}

static void
test_pmap_resolve_package_string (void)
{
  try_pmap_parse ("{ \"packages\": { \"pkg\": \"./pkg.cjs\" } }");

  jjs_value_t resolved;
  const char *expected = "./unit-fixtures/pmap/pkg.cjs";

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "pkg"), JJS_MOVE, JJS_MODULE_TYPE_NONE);
  expect_filename (resolved, JJS_MOVE, expected);

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "pkg"), JJS_MOVE, JJS_MODULE_TYPE_COMMONJS);
  expect_filename (resolved, JJS_MOVE, expected);

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "pkg"), JJS_MOVE, JJS_MODULE_TYPE_MODULE);
  expect_filename (resolved, JJS_MOVE, expected);
}

static void
test_pmap_resolve_package_main (void)
{
  try_pmap_parse ("{ \"packages\": { \"pkg\": { \"main\": \"./pkg.cjs\" } } }");

  jjs_value_t resolved;
  const char *expected = "./unit-fixtures/pmap/pkg.cjs";

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "pkg"), JJS_MOVE, JJS_MODULE_TYPE_NONE);
  expect_filename (resolved, JJS_MOVE, expected);

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "pkg"), JJS_MOVE, JJS_MODULE_TYPE_COMMONJS);
  expect_filename (resolved, JJS_MOVE, expected);

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "pkg"), JJS_MOVE, JJS_MODULE_TYPE_MODULE);
  expect_filename (resolved, JJS_MOVE, expected);
}

static void
test_pmap_resolve_package_main_by_module_type (void)
{
  const char *json = "{"
                     "\"packages\": {"
                     "\"pkg\": {"
                     "\"commonjs\": {"
                     "\"main\": \"./pkg.cjs\""
                     "},"
                     "\"module\": {"
                     "\"main\": \"./pkg.mjs\""
                     "}"
                     "}"
                     "}"
                     "}";

  try_pmap_parse (json);

  jjs_value_t resolved;

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "pkg"), JJS_MOVE, JJS_MODULE_TYPE_COMMONJS);
  expect_filename (resolved, JJS_MOVE, "./unit-fixtures/pmap/pkg.cjs");

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "pkg"), JJS_MOVE, JJS_MODULE_TYPE_MODULE);
  expect_filename (resolved, JJS_MOVE, "./unit-fixtures/pmap/pkg.mjs");
}

static void
test_pmap_resolve_package_string_by_module_type (void)
{
  const char *json = "{"
                     "\"packages\": {"
                     "\"pkg\": {"
                     "\"commonjs\": \"./pkg.cjs\","
                     "\"module\": \"./pkg.mjs\""
                     "}"
                     "}"
                     "}";

  try_pmap_parse (json);

  jjs_value_t resolved;

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "pkg"), JJS_MOVE, JJS_MODULE_TYPE_COMMONJS);
  expect_filename (resolved, JJS_MOVE, "./unit-fixtures/pmap/pkg.cjs");

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "pkg"), JJS_MOVE, JJS_MODULE_TYPE_MODULE);
  expect_filename (resolved, JJS_MOVE, "./unit-fixtures/pmap/pkg.mjs");
}

static void
test_pmap_resolve_scoped_package_string (void)
{
  const char *json =
    "{ \"packages\": { \"@test/pkg1\": { \"main\": \"./@test/pkg1/b.cjs\", \"path\": \"./@test/pkg1\" } } }";

  try_pmap_parse (json);

  jjs_value_t resolved;
  const char *expected = "./unit-fixtures/pmap/@test/pkg1/b.cjs";

  /* @test/pkg1/b.cjs -> root + package.@test/pkg1.path + b.cjs */

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "@test/pkg1/b.cjs"), JJS_MOVE, JJS_MODULE_TYPE_NONE);
  expect_filename (resolved, JJS_MOVE, expected);

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "@test/pkg1/b.cjs"), JJS_MOVE, JJS_MODULE_TYPE_COMMONJS);
  expect_filename (resolved, JJS_MOVE, expected);

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "@test/pkg1/b.cjs"), JJS_MOVE, JJS_MODULE_TYPE_MODULE);
  expect_filename (resolved, JJS_MOVE, expected);

  /* @test/pkg1 -> root + package.@test/pkg1.main */

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "@test/pkg1"), JJS_MOVE, JJS_MODULE_TYPE_NONE);
  expect_filename (resolved, JJS_MOVE, expected);

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "@test/pkg1"), JJS_MOVE, JJS_MODULE_TYPE_COMMONJS);
  expect_filename (resolved, JJS_MOVE, expected);

  resolved = jjs_pmap_resolve (ctx (), jjs_string_sz (ctx (), "@test/pkg1"), JJS_MOVE, JJS_MODULE_TYPE_MODULE);
  expect_filename (resolved, JJS_MOVE, expected);
}

TEST_MAIN({
  test_pmap ();
  test_pmap_invalid ();
  test_pmap_type_error ();
  test_pmap_root_type_error ();
  test_pmap_from_file_error ();

  test_pmap_resolve_package_string ();
  test_pmap_resolve_package_main ();
  test_pmap_resolve_package_main_by_module_type ();
  test_pmap_resolve_package_string_by_module_type ();
  test_pmap_resolve_scoped_package_string ();
  test_pmap_resolve_error ();
})
