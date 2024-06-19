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
    ;
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

/** common exception tests for jjs_esm_*_source() functions. */
static void
source_exceptions_impl (jjs_context_t *context_p, jjs_value_t fn (jjs_context_t *, const jjs_esm_source_t*, jjs_own_t))
{
  jjs_esm_source_t sources [] = {
    jjs_esm_source_of_sz (TEST_SOURCE_PARSE_ERROR),
    jjs_esm_source_of_sz (TEST_SOURCE_LINK_ERROR),
    jjs_esm_source_of_sz (TEST_SOURCE_EVALUATE_ERROR),
  };

  for (size_t i = 0; i < JJS_ARRAY_SIZE (sources); i++)
  {
    check_exception (fn (context_p, &sources[i], JJS_MOVE));
  }
} /* source_exceptions_impl */

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
  const char* source_p = "export default 5;";
  jjs_esm_source_t sources [] = {
    jjs_esm_source_of ((const jjs_char_t*) source_p, (jjs_size_t) strlen (source_p)),
    jjs_esm_source_of_sz (source_p),
    {
      .source_value = jjs_optional_value (jjs_string_utf8_sz (ctx (), source_p)),
    },
  };

  size_t sources_size = JJS_ARRAY_SIZE (sources);
  size_t i;

  for (i = 0; i < sources_size; i++)
  {
    check_namespace_int32 (jjs_esm_import_source (ctx (), &sources[i], JJS_KEEP), "default", 5);
  }

  for (i = 0; i < sources_size; i++)
  {
    check_namespace_int32 (jjs_esm_import_source (ctx (), &sources[i], JJS_MOVE), "default", 5);
  }
} /* test_esm_import_source */

static void
test_esm_evaluate_source (void)
{
  const char* source_p = "5";

  jjs_esm_source_t sources [] = {
    jjs_esm_source_of_sz (source_p),
    jjs_esm_source_of ((const jjs_char_t*) source_p, (jjs_size_t) strlen (source_p)),
    {
      .source_value = jjs_optional_value (jjs_string_utf8_sz (ctx (), source_p)),
    },
  };

  size_t sources_size = JJS_ARRAY_SIZE (sources);
  size_t i;

  for (i = 0; i < sources_size; i++)
  {
    check_evaluate_int32 (jjs_esm_evaluate_source (ctx (), &sources[i], JJS_KEEP), 5);
  }

  for (i = 0; i < sources_size; i++)
  {
    check_evaluate_int32 (jjs_esm_evaluate_source (ctx (), &sources[i], JJS_MOVE), 5);
  }
} /* test_esm_evaluate_source */

static void
test_esm_import_source_exceptions (void)
{
  source_exceptions_impl (ctx (), &jjs_esm_import_source);
} /* test_esm_import_source_exceptions */

static void
test_esm_evaluate_source_exceptions (void)
{
  source_exceptions_impl (ctx (), &jjs_esm_evaluate_source);
} /* test_esm_evaluate_source_exceptions */

static void
test_esm_source_validation (void)
{
  const char* source_p = "export default 10;";

  jjs_esm_source_t sources [] = {
    // Empty
    jjs_esm_source (),
    // source_value is not a string
    {
      .source_value = jjs_optional_value (jjs_object (ctx ())),
    },
    // source_value and source_buffer set
    {
      .source_value = jjs_optional_value (jjs_string_utf8_sz (ctx (), source_p)),
      .source_buffer_p = (const jjs_char_t*) source_p,
      .source_buffer_size = (jjs_size_t) strlen (source_p),
    },
    // invalid filename
    {
      .source_buffer_p = (const jjs_char_t*) source_p,
      .source_buffer_size = (jjs_size_t) strlen (source_p),
      .filename = jjs_optional_value (jjs_number (ctx (), 1)),
    },
    // invalid dirname
    {
      .source_buffer_p = (const jjs_char_t*) source_p,
      .source_buffer_size = (jjs_size_t) strlen (source_p),
      .dirname = jjs_optional_value (jjs_number (ctx (), 1)),
    },
  };

  const size_t sources_size = JJS_ARRAY_SIZE (sources);
  size_t i;

  for (i = 0; i < sources_size; i++)
  {
    jjs_esm_import_source (ctx (), &sources[i], JJS_KEEP);
  }

  for (i = 0; i < sources_size; i++)
  {
    jjs_esm_evaluate_source (ctx (), &sources[i], JJS_MOVE);
  }
} /* test_esm_source_validation */

static void
test_esm_source (void)
{
  jjs_esm_source_t source;

  source = jjs_esm_source ();
  jjs_esm_source_free_values (ctx (), &source);

  source = jjs_esm_source_of_sz (TEST_SOURCE_PARSE_ERROR);
  TEST_ASSERT (source.source_buffer_p != NULL);
  TEST_ASSERT (source.source_buffer_size > 0);
  jjs_esm_source_free_values (ctx (), &source);

  source =
    jjs_esm_source_of ((const jjs_char_t*) TEST_SOURCE_PARSE_ERROR, (jjs_size_t) strlen (TEST_SOURCE_PARSE_ERROR));
  TEST_ASSERT (source.source_buffer_p != NULL);
  TEST_ASSERT (source.source_buffer_size > 0);
  jjs_esm_source_free_values (ctx (), &source);
} /* test_esm_source */

static void
test_esm_source_free_values (void)
{
  const char* source_p = "export default 10;";
  jjs_esm_source_t source;

  source = (jjs_esm_source_t) {
    .source_value = jjs_optional_value (jjs_string_utf8_sz (ctx (), source_p)),
  };

  jjs_esm_source_free_values (ctx (), &source);

  source = jjs_esm_source_of_sz (source_p);
  source.filename = jjs_optional_value (jjs_string_utf8_sz (ctx (), "filename"));

  jjs_esm_source_free_values (ctx (), &source);

  source = jjs_esm_source_of_sz (source_p);
  source.dirname = jjs_optional_value (jjs_string_utf8_sz (ctx (), "dirname"));

  jjs_esm_source_free_values (ctx (), &source);

  source = jjs_esm_source_of_sz (source_p);
  source.meta_extension = jjs_optional_value (jjs_object (ctx ()));

  jjs_esm_source_free_values (ctx (), &source);

  source = (jjs_esm_source_t) {
    .source_value = jjs_optional_value (jjs_string_utf8_sz (ctx (), source_p)),
    .filename = jjs_optional_value (jjs_string_utf8_sz (ctx (), "filename")),
    .dirname = jjs_optional_value (jjs_string_utf8_sz (ctx (), "dirname")),
    .meta_extension = jjs_optional_value (jjs_object (ctx ())),
  };

  jjs_esm_source_free_values (ctx (), &source);
} /* test_esm_source_free_values */

int
main (void)
{
  ctx_open (NULL);

  test_esm_source ();
  test_esm_source_free_values ();
  test_esm_source_validation ();

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
  test_esm_import_source_exceptions ();

  // jjs_esm_evaluate_source()
  test_esm_evaluate_source ();
  test_esm_evaluate_source_exceptions ();

  ctx_close ();
  return 0;
} /* main */
