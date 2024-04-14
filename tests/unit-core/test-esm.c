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

#include "annex.h"
#include "config.h"
#define TEST_COMMON_IMPLEMENTATION
#include "test-common.h"

static const char* TEST_MODULE_A = "./unit-fixtures/modules/a.mjs";
static const char* TEST_MODULE_NESTED = "./unit-fixtures/modules/nested.mjs";
static const char* TEST_MODULE_CIRCULAR = "./unit-fixtures/modules/circular.mjs";

static const char* TEST_SOURCE_PARSE_ERROR = "import 434324 from dasdasd;";
static const char* TEST_SOURCE_LINK_ERROR = "import {f} from 'does-not-exist;";
static const char* TEST_SOURCE_EVALUATE_ERROR = "throw Error('you can't catch me!');";

static void
check_namespace_sz (jjs_value_t ns, const char* key, const char* expected)
{
  push (ns);
  TEST_ASSERT (!jjs_value_is_exception (ns));
  TEST_ASSERT (strict_equals_cstr (push (jjs_object_get_sz (ns, key)), expected));
} /* check_namespace_sz */

static void
check_namespace_int32 (jjs_value_t ns, const char* key, int32_t expected)
{
  push (ns);
  TEST_ASSERT (!jjs_value_is_exception (ns));
  TEST_ASSERT (strict_equals_int32 (push (jjs_object_get_sz (ns, key)), expected));
} /* check_namespace_int32 */

static void
check_evaluate_int32 (jjs_value_t value, int32_t expected)
{
  push (value);
  TEST_ASSERT (!jjs_value_is_exception (value));
  TEST_ASSERT (strict_equals_int32 (value, expected));
} /* check_evaluate_int32 */

static void
check_ok (jjs_value_t value)
{
  push (value);
  TEST_ASSERT (!jjs_value_is_exception (value));
  TEST_ASSERT (!jjs_value_is_exception (push (jjs_run_jobs ())));
} /* check_ok */

static void
check_exception (jjs_value_t value)
{
  push (value);
  TEST_ASSERT (jjs_value_is_exception (value));
} /* check_exception */

/** common exception tests for jjs_esm_*_source() functions. */
static void
source_exceptions_impl (jjs_value_t fn (jjs_esm_source_t*, jjs_value_ownership_t))
{
  jjs_esm_source_t sources [] = {
    {
      .source_sz = TEST_SOURCE_PARSE_ERROR,
    },
    {
      .source_sz = TEST_SOURCE_LINK_ERROR,
    },
    {
      .source_sz = TEST_SOURCE_EVALUATE_ERROR,
    },
  };

  for (size_t i = 0; i < JJS_ARRAY_SIZE (sources); i++)
  {
    check_exception (fn (&sources[i], JJS_MOVE));
  }
} /* source_exceptions_impl */

static void
test_esm_import_invalid_args (void)
{
  check_exception (jjs_esm_import (jjs_null (), JJS_MOVE));
  check_exception (jjs_esm_import (jjs_undefined (), JJS_MOVE));
  check_exception (jjs_esm_import (jjs_number (0), JJS_MOVE));
  check_exception (jjs_esm_import (jjs_boolean (true), JJS_MOVE));
  check_exception (jjs_esm_import (jjs_object (), JJS_MOVE));
  check_exception (jjs_esm_import (jjs_array (0), JJS_MOVE));
  check_exception (jjs_esm_import (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG), JJS_MOVE));

  check_exception (jjs_esm_import_sz (NULL));
  check_exception (jjs_esm_import_sz (""));
  check_exception (jjs_esm_import_sz ("unknown"));
  check_exception (jjs_esm_import_sz ("./unknown"));
  check_exception (jjs_esm_import_sz ("../unknown"));
  check_exception (jjs_esm_import_sz ("/unknown"));
} /* test_esm_import_invalid_args */

static void
test_invalid_jjs_esm_evaluate_invalid_args (void)
{
  check_exception (jjs_esm_evaluate (jjs_null (), JJS_MOVE));
  check_exception (jjs_esm_evaluate (jjs_undefined (), JJS_MOVE));
  check_exception (jjs_esm_evaluate (jjs_number (0), JJS_MOVE));
  check_exception (jjs_esm_evaluate (jjs_boolean (true), JJS_MOVE));
  check_exception (jjs_esm_evaluate (jjs_object (), JJS_MOVE));
  check_exception (jjs_esm_evaluate (jjs_array (0), JJS_MOVE));
  check_exception (jjs_esm_evaluate (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG), JJS_MOVE));

  check_exception (jjs_esm_evaluate_sz (NULL));
  check_exception (jjs_esm_evaluate_sz (""));
  check_exception (jjs_esm_evaluate_sz ("unknown"));
  check_exception (jjs_esm_evaluate_sz ("./unknown"));
  check_exception (jjs_esm_evaluate_sz ("../unknown"));
  check_exception (jjs_esm_evaluate_sz ("/unknown"));
} /* test_invalid_jjs_esm_evaluate_invalid_args */

static void
test_esm_import_relative_path (void)
{
  check_namespace_sz (jjs_esm_import_sz (TEST_MODULE_A), "default", "a");
  check_namespace_sz (jjs_esm_import_sz (TEST_MODULE_NESTED), "default", "a");
  check_ok (jjs_esm_import_sz (TEST_MODULE_CIRCULAR));
} /* test_esm_import_relative_path */

static void
test_esm_import_absolute_path (void)
{
  jjs_value_t a = annex_path_normalize (push_sz (TEST_MODULE_A));
  jjs_value_t nested = annex_path_normalize (push_sz (TEST_MODULE_NESTED));
  jjs_value_t circular = annex_path_normalize (push_sz (TEST_MODULE_CIRCULAR));

  check_namespace_sz (jjs_esm_import (a, JJS_MOVE), "default", "a");
  check_namespace_sz (jjs_esm_import (nested, JJS_MOVE), "default", "a");
  check_ok (jjs_esm_import (circular, JJS_MOVE));
} /* test_esm_import_absolute_path */

static void
test_esm_evaluate_relative_path (void)
{
  check_ok (jjs_esm_evaluate_sz (TEST_MODULE_A));
  check_ok (jjs_esm_import_sz (TEST_MODULE_NESTED));
  check_ok (jjs_esm_import_sz (TEST_MODULE_CIRCULAR));
} /* test_esm_evaluate_relative_path */

static void
test_esm_evaluate_absolute_path (void)
{
  check_ok (jjs_esm_import (annex_path_normalize (push_sz (TEST_MODULE_A)), JJS_MOVE));
  check_ok (jjs_esm_import (annex_path_normalize (push_sz (TEST_MODULE_NESTED)), JJS_MOVE));
  check_ok (jjs_esm_import (annex_path_normalize (push_sz (TEST_MODULE_CIRCULAR)), JJS_MOVE));
} /* test_esm_evaluate_absolute_path */

static void
test_esm_import_source (void)
{
  const char* source_p = "export default 5;";
  jjs_esm_source_t sources [] = {
    {
      .source_sz = source_p,
    },
    {
      .source_value = jjs_string_utf8_sz (source_p),
    },
    {
      .source_buffer_p = (const jjs_char_t*) source_p,
      .source_buffer_size = (jjs_size_t) strlen (source_p),
    },
  };

  size_t sources_size = JJS_ARRAY_SIZE (sources);
  size_t i;

  for (i = 0; i < sources_size; i++)
  {
    jjs_esm_source_t source = sources[i];

    if (source.source_value != 0)
    {
      source.source_value = jjs_value_copy (source.source_value);
    }

    check_namespace_int32 (jjs_esm_import_source (&sources[i], JJS_KEEP), "default", 5);

    jjs_esm_source_free_values (&source);
  }

  for (i = 0; i < sources_size; i++)
  {
    check_namespace_int32 (jjs_esm_import_source (&sources[i], JJS_MOVE), "default", 5);
  }
} /* test_esm_import_source */

static void
test_esm_evaluate_source (void)
{
  const char* source_p = "5";

  jjs_esm_source_t sources [] = {
    {
      .source_sz = source_p,
    },
    {
      .source_value = jjs_string_utf8_sz (source_p),
    },
    {
      .source_buffer_p = (const jjs_char_t*) source_p,
      .source_buffer_size = (jjs_size_t) strlen (source_p),
    },
  };

  size_t sources_size = JJS_ARRAY_SIZE (sources);
  size_t i;

  for (i = 0; i < sources_size; i++)
  {
    jjs_esm_source_t source = sources[i];

    if (source.source_value != 0)
    {
      source.source_value = jjs_value_copy (source.source_value);
    }

    check_evaluate_int32 (jjs_esm_evaluate_source (&sources[i], JJS_KEEP), 5);

    jjs_esm_source_free_values (&source);
  }

  for (i = 0; i < sources_size; i++)
  {
    check_evaluate_int32 (jjs_esm_evaluate_source (&sources[i], JJS_MOVE), 5);
  }
} /* test_esm_evaluate_source */

static void
test_esm_import_source_exceptions (void)
{
  source_exceptions_impl (&jjs_esm_import_source);
} /* test_esm_import_source_exceptions */

static void
test_esm_evaluate_source_exceptions (void)
{
  source_exceptions_impl (&jjs_esm_evaluate_source);
} /* test_esm_evaluate_source_exceptions */

static void
test_esm_source_validation (void)
{
  const char* source_p = "export default 10;";

  jjs_esm_source_t sources [] = {
    // Empty
    {
      .cache = false,
    },
    // source_value is not a string
    {
      .source_value = jjs_object (),
    },
    // source_value and source_sz set
    {
      .source_value = jjs_string_utf8_sz(source_p),
      .source_sz = source_p,
    },
    // source_buffer and source_sz set
    {
      .source_sz = source_p,
      .source_buffer_p = (const jjs_char_t*) source_p,
      .source_buffer_size = (jjs_size_t) strlen (source_p),
    },
    // source_value and source_buffer set
    {
      .source_value = jjs_string_utf8_sz(source_p),
      .source_buffer_p = (const jjs_char_t*) source_p,
      .source_buffer_size = (jjs_size_t) strlen (source_p),
    },
    // all sources set
    {
      .source_sz = source_p,
      .source_value = jjs_string_utf8_sz(source_p),
      .source_buffer_p = (const jjs_char_t*) source_p,
      .source_buffer_size = (jjs_size_t) strlen (source_p),
    },
    // invalid filename
    {
      .source_sz = source_p,
      .filename = jjs_number(1),
    },
    // invalid dirname
    {
      .source_sz = source_p,
      .dirname = jjs_number(1),
    },
  };

  size_t sources_size = JJS_ARRAY_SIZE (sources);
  size_t i;

  for (i = 0; i < sources_size; i++)
  {
    jjs_esm_import_source (&sources[i], JJS_MOVE);
  }

  for (i = 0; i < sources_size; i++)
  {
    jjs_esm_evaluate_source (&sources[i], JJS_MOVE);
  }
} /* test_esm_source_validation */

static void
test_esm_source (void)
{
  jjs_esm_source_t source = jjs_esm_source();
  jjs_esm_source_free_values (&source);

  jjs_esm_source_t source2;
  jjs_esm_source_free_values (jjs_esm_source_init (&source2));
} /* test_esm_source */

static void
test_esm_source_free_values (void)
{
  const char* source_p = "export default 10;";
  jjs_esm_source_t source;

  source = (jjs_esm_source_t) {
    .source_value = jjs_string_utf8_sz (source_p),
  };

  jjs_esm_source_free_values (&source);

  source = (jjs_esm_source_t) {
    .source_sz = source_p,
    .filename = jjs_string_utf8_sz ("filename"),
  };

  jjs_esm_source_free_values (&source);

  source = (jjs_esm_source_t) {
    .source_sz = source_p,
    .dirname = jjs_string_utf8_sz ("dirname"),
  };

  jjs_esm_source_free_values (&source);

  source = (jjs_esm_source_t) {
    .source_sz = source_p,
    .meta_extension = jjs_object (),
  };

  jjs_esm_source_free_values (&source);

  source = (jjs_esm_source_t) {
    .source_value = jjs_string_utf8_sz (source_p),
    .filename = jjs_string_utf8_sz ("filename"),
    .dirname = jjs_string_utf8_sz ("dirname"),
    .meta_extension = jjs_object (),
  };

  jjs_esm_source_free_values (&source);
} /* test_esm_source_free_values */

int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

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

  free_values ();
  jjs_cleanup ();
  return 0;
} /* main */
