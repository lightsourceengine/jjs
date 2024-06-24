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

#include "annex.h"

static const char* TEST_MODULE_A = "./unit-fixtures/modules/a.mjs";
static const char* TEST_MODULE_NESTED = "./unit-fixtures/modules/nested.mjs";
static const char* TEST_MODULE_CIRCULAR = "./unit-fixtures/modules/circular.mjs";

static const char* TEST_SOURCE_PARSE_ERROR = "import 434324 from dasdasd;";
static const char* TEST_SOURCE_LINK_ERROR = "import {f} from 'does-not-exist;";
static const char* TEST_SOURCE_EVALUATE_ERROR = "throw Error('you can't catch me!');";

static void
check_namespace_sz (jjs_value_t ns, const char* key, const char* expected)
{
  ctx_defer_free (ns);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), ns));
  TEST_ASSERT (strict_equals_cstr (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), ns, key)), expected));
} /* check_namespace_sz */

static void
check_namespace_int32 (jjs_value_t ns, const char* key, int32_t expected)
{
  ctx_defer_free (ns);
  if (jjs_value_is_exception (ctx (), ns))
  {
    jjs_log_fmt (ctx (), JJS_LOG_LEVEL_ERROR, "{}\n", ns);
  }
  TEST_ASSERT (!jjs_value_is_exception (ctx (), ns));
  TEST_ASSERT (strict_equals_int32 (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), ns, key)), expected));
} /* check_namespace_int32 */

static void
check_evaluate_int32 (jjs_value_t value, int32_t expected)
{
  ctx_defer_free (value);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), value));
  TEST_ASSERT (strict_equals_int32 (ctx (), value, expected));
} /* check_evaluate_int32 */

static void
check_ok (jjs_value_t value)
{
  ctx_defer_free (value);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), value));
  TEST_ASSERT (!jjs_value_is_exception (ctx (), ctx_defer_free (jjs_run_jobs (ctx ()))));
} /* check_ok */

static void
check_exception (jjs_value_t value)
{
  ctx_defer_free (value);
  TEST_ASSERT (jjs_value_is_exception (ctx (), value));
} /* check_exception */

static jjs_value_t
copy_to_buffer (jjs_value_t target, const jjs_char_t *data_p, jjs_size_t length)
{
  if (jjs_value_is_typedarray (ctx (), target))
  {
    jjs_size_t offset;
    jjs_size_t buffer_length;
    jjs_value_t buffer = jjs_typedarray_buffer (ctx (), target, &offset, &buffer_length);

    TEST_ASSERT (buffer_length == length);
    memcpy (jjs_arraybuffer_data (ctx (), buffer) + offset, data_p, length);
    jjs_value_free (ctx (), buffer);
  }
  else
  {
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), target) == length);
    memcpy (jjs_arraybuffer_data (ctx (), target), data_p, length);
  }

  return target;
} /* copy_to_buffer */

static void
test_esm_import_invalid_args (void)
{
  check_exception (jjs_esm_import (ctx (), jjs_null (ctx ()), JJS_MOVE));
  check_exception (jjs_esm_import (ctx (), jjs_undefined (ctx ()), JJS_MOVE));
  check_exception (jjs_esm_import (ctx (), jjs_number (ctx (), 0), JJS_MOVE));
  check_exception (jjs_esm_import (ctx (), jjs_boolean (ctx (), true), JJS_MOVE));
  check_exception (jjs_esm_import (ctx (), jjs_object (ctx ()), JJS_MOVE));
  check_exception (jjs_esm_import (ctx (), jjs_array (ctx (), 0), JJS_MOVE));
  check_exception (jjs_esm_import (ctx (), jjs_symbol_get_well_known (ctx (), JJS_SYMBOL_TO_STRING_TAG), JJS_MOVE));

  check_exception (jjs_esm_import_sz (ctx (), NULL));
  check_exception (jjs_esm_import_sz (ctx (), ""));
  check_exception (jjs_esm_import_sz (ctx (), "unknown"));
  check_exception (jjs_esm_import_sz (ctx (), "./unknown"));
  check_exception (jjs_esm_import_sz (ctx (), "../unknown"));
  check_exception (jjs_esm_import_sz (ctx (), "/unknown"));
} /* test_esm_import_invalid_args */

static void
test_invalid_jjs_esm_evaluate_invalid_args (void)
{
  check_exception (jjs_esm_evaluate (ctx (), jjs_null (ctx ()), JJS_MOVE));
  check_exception (jjs_esm_evaluate (ctx (), jjs_undefined (ctx ()), JJS_MOVE));
  check_exception (jjs_esm_evaluate (ctx (), jjs_number (ctx (), 0), JJS_MOVE));
  check_exception (jjs_esm_evaluate (ctx (), jjs_boolean (ctx (), true), JJS_MOVE));
  check_exception (jjs_esm_evaluate (ctx (), jjs_object (ctx ()), JJS_MOVE));
  check_exception (jjs_esm_evaluate (ctx (), jjs_array (ctx (), 0), JJS_MOVE));
  check_exception (jjs_esm_evaluate (ctx (), jjs_symbol_get_well_known (ctx (), JJS_SYMBOL_TO_STRING_TAG), JJS_MOVE));

  check_exception (jjs_esm_evaluate_sz (ctx (), NULL));
  check_exception (jjs_esm_evaluate_sz (ctx (), ""));
  check_exception (jjs_esm_evaluate_sz (ctx (), "unknown"));
  check_exception (jjs_esm_evaluate_sz (ctx (), "./unknown"));
  check_exception (jjs_esm_evaluate_sz (ctx (), "../unknown"));
  check_exception (jjs_esm_evaluate_sz (ctx (), "/unknown"));
} /* test_invalid_jjs_esm_evaluate_invalid_args */

static void
test_esm_import_relative_path (void)
{
  check_namespace_sz (jjs_esm_import_sz (ctx (), TEST_MODULE_A), "default", "a");
  check_namespace_sz (jjs_esm_import_sz (ctx (), TEST_MODULE_NESTED), "default", "a");
  check_ok (jjs_esm_import_sz (ctx (), TEST_MODULE_CIRCULAR));
} /* test_esm_import_relative_path */

static void
test_esm_import_absolute_path (void)
{
  jjs_value_t a = annex_path_normalize (ctx (), ctx_cstr (TEST_MODULE_A));
  jjs_value_t nested = annex_path_normalize (ctx (), ctx_cstr (TEST_MODULE_NESTED));
  jjs_value_t circular = annex_path_normalize (ctx (), ctx_cstr (TEST_MODULE_CIRCULAR));

  check_namespace_sz (jjs_esm_import (ctx (), a, JJS_MOVE), "default", "a");
  check_namespace_sz (jjs_esm_import (ctx (), nested, JJS_MOVE), "default", "a");
  check_ok (jjs_esm_import (ctx (), circular, JJS_MOVE));
} /* test_esm_import_absolute_path */

static void
test_esm_evaluate_relative_path (void)
{
  check_ok (jjs_esm_evaluate_sz (ctx (), TEST_MODULE_A));
  check_ok (jjs_esm_import_sz (ctx (), TEST_MODULE_NESTED));
  check_ok (jjs_esm_import_sz (ctx (), TEST_MODULE_CIRCULAR));
} /* test_esm_evaluate_relative_path */

static void
test_esm_evaluate_absolute_path (void)
{
  check_ok (jjs_esm_import (ctx (), annex_path_normalize (ctx (), ctx_cstr (TEST_MODULE_A)), JJS_MOVE));
  check_ok (jjs_esm_import (ctx (), annex_path_normalize (ctx (), ctx_cstr (TEST_MODULE_NESTED)), JJS_MOVE));
  check_ok (jjs_esm_import (ctx (), annex_path_normalize (ctx (), ctx_cstr (TEST_MODULE_CIRCULAR)), JJS_MOVE));
} /* test_esm_evaluate_absolute_path */

static void
test_esm_import_source (void)
{
  const char *source_p = "export default 5;";
  const jjs_char_t *source_bytes_p = (const jjs_char_t *) source_p;
  jjs_size_t source_bytes_len = (jjs_size_t) strlen (source_p);
  jjs_esm_source_options_t options;

  /* import source from buffer */
  check_namespace_int32 (jjs_esm_import_source (ctx (), source_bytes_p, source_bytes_len, NULL), "default", 5);
  options = jjs_esm_source_options ();
  check_namespace_int32 (jjs_esm_import_source (ctx (), source_bytes_p, source_bytes_len, &options), "default", 5);

  /* import source from C string */
  check_namespace_int32 (jjs_esm_import_source_sz (ctx (), source_p, NULL), "default", 5);
  options = jjs_esm_source_options ();
  check_namespace_int32 (jjs_esm_import_source_sz (ctx (), source_p, &options), "default", 5);

  /* import source from JS string value (JJS_MOVE) */
  check_namespace_int32 (jjs_esm_import_source_value (ctx (), jjs_string_utf8_sz (ctx (), source_p), JJS_MOVE, NULL), "default", 5);
  options = jjs_esm_source_options ();
  check_namespace_int32 (jjs_esm_import_source_value (ctx (), jjs_string_utf8_sz (ctx (), source_p), JJS_MOVE, &options), "default", 5);

  /* import source from JS string value (JJS_KEEP) */
  check_namespace_int32 (jjs_esm_import_source_value (ctx (), ctx_cstr (source_p), JJS_KEEP, NULL), "default", 5);
  options = jjs_esm_source_options ();
  check_namespace_int32 (jjs_esm_import_source_value (ctx (), ctx_cstr(source_p), JJS_KEEP, &options), "default", 5);

  /* import source from ArrayBuffer */
  jjs_value_t arraybuffer = copy_to_buffer (jjs_arraybuffer (ctx (), source_bytes_len), source_bytes_p, source_bytes_len);
  check_namespace_int32 (jjs_esm_import_source_value (ctx (), arraybuffer, JJS_MOVE, NULL), "default", 5);

  /* import source from SharedArrayBuffer */
  jjs_value_t shared_arraybuffer = copy_to_buffer (jjs_shared_arraybuffer (ctx (), source_bytes_len), source_bytes_p, source_bytes_len);
  check_namespace_int32 (jjs_esm_import_source_value (ctx (), shared_arraybuffer, JJS_MOVE, NULL), "default", 5);

  /* import source from TypedArray */
  jjs_value_t typedarray = copy_to_buffer (jjs_typedarray(ctx (), JJS_TYPEDARRAY_UINT8, source_bytes_len), source_bytes_p, source_bytes_len);
  check_namespace_int32 (jjs_esm_import_source_value (ctx (), typedarray, JJS_MOVE, NULL), "default", 5);
} /* test_esm_import_source */

static void
test_esm_evaluate_source (void)
{
  const char* source_p = "5";
  const jjs_char_t *source_bytes_p = (const jjs_char_t *) source_p;
  jjs_size_t source_bytes_len = (jjs_size_t) strlen (source_p);
  jjs_esm_source_options_t options;

  /* import source from buffer */
  check_evaluate_int32 (jjs_esm_evaluate_source (ctx (), source_bytes_p, source_bytes_len, NULL), 5);
  options = jjs_esm_source_options ();
  check_evaluate_int32 (jjs_esm_evaluate_source (ctx (), source_bytes_p, source_bytes_len, &options), 5);

  /* import source from C string */
  check_evaluate_int32 (jjs_esm_evaluate_source_sz (ctx (), source_p, NULL), 5);
  options = jjs_esm_source_options ();
  check_evaluate_int32 (jjs_esm_evaluate_source_sz (ctx (), source_p, &options), 5);

  /* import source from JS string value (JJS_MOVE) */
  check_evaluate_int32 (jjs_esm_evaluate_source_value (ctx (), jjs_string_utf8_sz (ctx (), source_p), JJS_MOVE, NULL), 5);
  options = jjs_esm_source_options ();
  check_evaluate_int32 (jjs_esm_evaluate_source_value (ctx (), jjs_string_utf8_sz (ctx (), source_p), JJS_MOVE, &options), 5);

  /* import source from JS string value (JJS_KEEP) */
  check_evaluate_int32 (jjs_esm_evaluate_source_value (ctx (), ctx_cstr (source_p), JJS_KEEP, NULL), 5);
  options = jjs_esm_source_options ();
  check_evaluate_int32 (jjs_esm_evaluate_source_value (ctx (), ctx_cstr(source_p), JJS_KEEP, &options), 5);

  /* import source from ArrayBuffer */
  jjs_value_t arraybuffer = copy_to_buffer (jjs_arraybuffer (ctx (), source_bytes_len), source_bytes_p, source_bytes_len);
  check_evaluate_int32 (jjs_esm_evaluate_source_value (ctx (), arraybuffer, JJS_MOVE, NULL), 5);

  /* import source from SharedArrayBuffer */
  jjs_value_t shared_arraybuffer = copy_to_buffer (jjs_shared_arraybuffer (ctx (), source_bytes_len), source_bytes_p, source_bytes_len);
  check_evaluate_int32 (jjs_esm_evaluate_source_value (ctx (), shared_arraybuffer, JJS_MOVE, NULL), 5);

  /* import source from TypedArray */
  jjs_value_t typedarray = copy_to_buffer (jjs_typedarray(ctx (), JJS_TYPEDARRAY_UINT8, source_bytes_len), source_bytes_p, source_bytes_len);
  check_evaluate_int32 (jjs_esm_evaluate_source_value (ctx (), typedarray, JJS_MOVE, NULL), 5);
} /* test_esm_evaluate_source */

static void
test_esm_source_parse_exceptions (void)
{
  const char *bad_sources [] = {
    TEST_SOURCE_PARSE_ERROR,
    TEST_SOURCE_LINK_ERROR,
    TEST_SOURCE_EVALUATE_ERROR,
  };

  for (size_t i = 0; i < JJS_ARRAY_SIZE (bad_sources); i++)
  {
    check_exception (jjs_esm_import_source_sz (ctx (), bad_sources[i], NULL));
    check_exception (jjs_esm_import_source_value (ctx (), jjs_string_utf8_sz (ctx (), bad_sources[i]), JJS_MOVE, NULL));
    check_exception (jjs_esm_import_source_value (ctx (), ctx_cstr (bad_sources[i]), JJS_KEEP, NULL));

    check_exception (jjs_esm_evaluate_source_sz (ctx (), bad_sources[i], NULL));
    check_exception (jjs_esm_evaluate_source_value (ctx (), jjs_string_utf8_sz (ctx (), bad_sources[i]), JJS_MOVE, NULL));
    check_exception (jjs_esm_evaluate_source_value (ctx (), ctx_cstr (bad_sources[i]), JJS_KEEP, NULL));
  }
} /* test_esm_source_parse_exceptions */

static void
test_esm_source_cache (void)
{
  const char *source_p = "typeof globalThis;";
  jjs_esm_source_options_t options;

  /* open new context per test to clear the esm cache */

  /* expect first source import to be cached */
  ctx_open (NULL);

  options = (jjs_esm_source_options_t) {
    .cache = true,
  };

  check_ok (jjs_esm_import_source_sz (ctx (), source_p, &options));
  check_exception (jjs_esm_import_source_sz (ctx (), source_p, &options));

  ctx_close ();

  /* expect first source import NOT to be cached */
  ctx_open (NULL);

  options = (jjs_esm_source_options_t) {
    .cache = false,
  };

  check_ok (jjs_esm_import_source_sz (ctx (), source_p, &options));
  check_ok (jjs_esm_import_source_sz (ctx (), source_p, &options));

  ctx_close ();
} /* test_esm_source_cache */

static void
test_esm_source_filename (void)
{
  const char *source_p = "import.meta.filename.substr(import.meta.dirname.length + 1 /* leading sep */)";

  /* expect import.meta.filename to be <source> */
  jjs_value_t filename = ctx_defer_free (jjs_esm_evaluate_source_sz (ctx (), source_p, NULL));
  jjs_value_t expected_filename = ctx_cstr ("<source>");

  ctx_assert_strict_equals (expected_filename, filename);

  /* expect import.meta.filename to be overridden */
  jjs_value_t test_filename = ctx_cstr ("test.js");
  jjs_esm_source_options_t options = {
    .filename = jjs_optional_value (test_filename),
  };

  filename = ctx_defer_free (jjs_esm_evaluate_source_sz (ctx (), source_p, &options));
  ctx_assert_strict_equals (test_filename, filename);
} /* test_esm_source_filename */

static void
test_esm_source_meta_extension (void)
{
  const char *source_p = "import.meta.extension";
  jjs_value_t extension_value = ctx_cstr ("test");
  jjs_esm_source_options_t options = {
    .meta_extension = jjs_optional_value (extension_value),
  };
  jjs_value_t result = ctx_defer_free (jjs_esm_evaluate_source_sz (ctx (), source_p, &options));

  ctx_assert_strict_equals (extension_value, result);
} /* test_esm_source_filename */

static void
test_esm_source_validation (void)
{
  check_exception (jjs_esm_import_source (ctx (), NULL, 0, NULL));
  check_exception (jjs_esm_import_source (ctx (), NULL, 1, NULL));
  check_exception (jjs_esm_import_source (ctx (), (const jjs_char_t *)"", 0, NULL));

  check_exception (jjs_esm_evaluate_source (ctx (), NULL, 0, NULL));
  check_exception (jjs_esm_evaluate_source (ctx (), NULL, 1, NULL));
  check_exception (jjs_esm_evaluate_source (ctx (), (const jjs_char_t *)"", 0, NULL));
} /* test_esm_source_validation */

static void
test_esm_source_options_disown (void)
{
  jjs_esm_source_options_t options = jjs_esm_source_options ();

  /* expect NULL options are tolerated */
  jjs_esm_source_options_disown (ctx (), NULL);

  /* expect empty options are tolerated */
  jjs_esm_source_options_disown (ctx (), &options);

  /* expect values to be freed */
  options = (jjs_esm_source_options_t) {
    .dirname = jjs_optional_value (jjs_string_utf8_sz (ctx (), "test")),
    .dirname_o = JJS_MOVE,
    .filename = jjs_optional_value (jjs_string_utf8_sz (ctx (), "test")),
    .filename_o = JJS_MOVE,
    .meta_extension = jjs_optional_value (jjs_string_utf8_sz (ctx (), "test")),
    .meta_extension_o = JJS_MOVE,
  };

  /* expect values NOT to be freed */
  jjs_esm_source_options_disown (ctx (), &options);

  options = (jjs_esm_source_options_t) {
    .dirname = jjs_optional_value (ctx_cstr ("test")),
    .filename = jjs_optional_value (ctx_cstr ("test")),
    .meta_extension = jjs_optional_value (ctx_cstr ("test")),
  };

  jjs_esm_source_options_disown (ctx (), &options);

} /* test_esm_source_options_disown */

int
main (void)
{
  ctx_open (NULL);

  test_esm_source_options_disown ();
  test_esm_source_validation ();
  test_esm_source_parse_exceptions ();
  test_esm_source_cache ();
  test_esm_source_filename ();
  test_esm_source_meta_extension ();

  // jjs_esm_import*()
  test_esm_import_relative_path ();
  test_esm_import_absolute_path ();
  test_esm_import_invalid_args ();

  // jjs_esm_evaluate*()
  test_esm_evaluate_relative_path ();
  test_esm_evaluate_absolute_path ();
  test_invalid_jjs_esm_evaluate_invalid_args ();

  // jjs_esm_import_source()
  test_esm_import_source ();

  // jjs_esm_evaluate_source()
  test_esm_evaluate_source ();

  ctx_close ();
  return 0;
} /* main */
