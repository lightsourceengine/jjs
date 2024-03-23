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
source_exceptions_impl (jjs_value_t fn (jjs_esm_source_t*))
{
  jjs_esm_source_t source;
  const char* bad_source_code_p[] = {
    TEST_SOURCE_PARSE_ERROR,
    TEST_SOURCE_LINK_ERROR,
    TEST_SOURCE_EVALUATE_ERROR,
  };

  for (size_t i = 0; i < JJS_ARRAY_SIZE (bad_source_code_p); i++)
  {
    jjs_esm_source_init_sz (&source, bad_source_code_p[i]);
    check_exception (fn (&source));
    jjs_esm_source_deinit (&source);
  }

  const char* perfect_source_code_p = "export default 1";
  jjs_esm_source_t bad_config_p[5];
  size_t p = 0;

  // empty filename
  jjs_esm_source_init_sz (&bad_config_p[p], perfect_source_code_p);
  jjs_esm_source_set_filename (&bad_config_p[p], jjs_string_sz (""), true);
  p++;

  // invalid filename
  jjs_esm_source_init_sz (&bad_config_p[p], perfect_source_code_p);
  jjs_esm_source_set_filename (&bad_config_p[p], jjs_number_from_int32 (1), true);
  p++;

  // dirname does not exist on fs
  jjs_esm_source_init_sz (&bad_config_p[p], perfect_source_code_p);
  jjs_esm_source_set_dirname (&bad_config_p[p], jjs_string_sz ("/does-not-exist"), true);
  p++;

  // invalid dirname
  jjs_esm_source_init_sz (&bad_config_p[p], perfect_source_code_p);
  jjs_esm_source_set_dirname (&bad_config_p[p], jjs_number_from_int32 (1), true);
  p++;

  // invalid source value
  jjs_esm_source_init_value (&bad_config_p[p], jjs_number_from_int32 (1), true);
  p++;

  TEST_ASSERT (p == JJS_ARRAY_SIZE (bad_config_p));

  for (size_t i = 0; i < JJS_ARRAY_SIZE (bad_config_p); i++)
  {
    check_exception (fn (&bad_config_p[i]));
    jjs_esm_source_deinit (&bad_config_p[i]);
  }
} /* source_exceptions_impl */

static void
test_esm_import_invalid_args (void)
{
  check_exception (jjs_esm_import (push (jjs_null ())));
  check_exception (jjs_esm_import (push (jjs_undefined ())));
  check_exception (jjs_esm_import (push (jjs_number (0))));
  check_exception (jjs_esm_import (push (jjs_boolean (true))));
  check_exception (jjs_esm_import (push (jjs_object ())));
  check_exception (jjs_esm_import (push (jjs_array (0))));
  check_exception (jjs_esm_import (push (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG))));

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
  check_exception (jjs_esm_evaluate (push (jjs_null ())));
  check_exception (jjs_esm_evaluate (push (jjs_undefined ())));
  check_exception (jjs_esm_evaluate (push (jjs_number (0))));
  check_exception (jjs_esm_evaluate (push (jjs_boolean (true))));
  check_exception (jjs_esm_evaluate (push (jjs_object ())));
  check_exception (jjs_esm_evaluate (push (jjs_array (0))));
  check_exception (jjs_esm_evaluate (push (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG))));

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
  jjs_value_t a = push (annex_path_normalize (push_sz (TEST_MODULE_A)));
  jjs_value_t nested = push (annex_path_normalize (push_sz (TEST_MODULE_NESTED)));
  jjs_value_t circular = push (annex_path_normalize (push_sz (TEST_MODULE_CIRCULAR)));

  check_namespace_sz (jjs_esm_import (a), "default", "a");
  check_namespace_sz (jjs_esm_import (nested), "default", "a");
  check_ok (jjs_esm_import (circular));
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
  check_ok (jjs_esm_import (push (annex_path_normalize (push_sz (TEST_MODULE_A)))));
  check_ok (jjs_esm_import (push (annex_path_normalize (push_sz (TEST_MODULE_NESTED)))));
  check_ok (jjs_esm_import (push (annex_path_normalize (push_sz (TEST_MODULE_CIRCULAR)))));
} /* test_esm_evaluate_absolute_path */

static void
test_esm_import_source (void)
{
  const char* source_p = "export default 5;";
  jjs_esm_source_t source[4];

  jjs_esm_source_init_sz (&source[0], source_p);
  jjs_esm_source_init_value (&source[1], jjs_string_sz (source_p), true);
  jjs_esm_source_init_value (&source[2], push_sz (source_p), false);
  jjs_esm_source_init (&source[3], JJS_STR (source_p), JJS_STRLEN (source_p));

  for (size_t i = 0; i < JJS_ARRAY_SIZE (source); i++)
  {
    check_namespace_int32 (jjs_esm_import_source (&source[i]), "default", 5);
    jjs_esm_source_deinit (&source[i]);
  }
} /* test_esm_import_source */

static void
test_esm_evaluate_source (void)
{
  const char* source_p = "5";
  jjs_esm_source_t source[4];

  jjs_esm_source_init_sz (&source[0], source_p);
  jjs_esm_source_init_value (&source[1], jjs_string_sz (source_p), true);
  jjs_esm_source_init_value (&source[2], push_sz (source_p), false);
  jjs_esm_source_init (&source[3], JJS_STR (source_p), JJS_STRLEN (source_p));

  for (size_t i = 0; i < JJS_ARRAY_SIZE (source); i++)
  {
    check_evaluate_int32 (jjs_esm_evaluate_source (&source[i]), 5);
    jjs_esm_source_deinit (&source[i]);
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
test_esm_source (void)
{
  const char* source_p = "export default 10;";
  jjs_esm_source_t source;

  // should set source and size
  jjs_esm_source_init_sz (&source, source_p);
  TEST_ASSERT (source.source_buffer_p);
  TEST_ASSERT (source.source_buffer_size);
  jjs_esm_source_deinit (&source);

  // should set source and size
  jjs_esm_source_init (&source, (const jjs_char_t*) source_p, (jjs_size_t) strlen (source_p));
  TEST_ASSERT (source.source_buffer_p);
  TEST_ASSERT (source.source_buffer_size);
  jjs_esm_source_deinit (&source);

  // should set source value and own value
  jjs_esm_source_init_value (&source, jjs_string_sz (source_p), true);
  TEST_ASSERT (jjs_value_is_string (source.source_value));
  jjs_esm_source_deinit (&source);

  // should set source value and take a copy of value
  jjs_esm_source_init_value (&source, push_sz (source_p), false);
  TEST_ASSERT (jjs_value_is_string (source.source_value));
  jjs_esm_source_deinit (&source);

  jjs_esm_source_init_sz (&source, source_p);

  // should set column and line
  jjs_esm_source_set_start (&source, 1, 2);
  TEST_ASSERT (source.start_column == 1);
  TEST_ASSERT (source.start_line == 2);

  // should set cache
  jjs_esm_source_set_cache (&source, true);
  TEST_ASSERT (source.cache);

  // should set meta extension and own value
  jjs_esm_source_set_meta_extension (&source, jjs_string_sz ("ext"), true);
  TEST_ASSERT (jjs_value_is_string (source.meta_extension));

  // should set meta extension and take a copy of value
  jjs_esm_source_set_meta_extension (&source, push_sz ("ext"), false);
  TEST_ASSERT (jjs_value_is_string (source.meta_extension));

  // should set filename and own value
  jjs_esm_source_set_filename (&source, jjs_string_sz ("test.js"), true);
  TEST_ASSERT (jjs_value_is_string (source.filename));

  // should set filename and take a copy of value
  jjs_esm_source_set_filename (&source, push_sz ("test.js"), false);
  TEST_ASSERT (jjs_value_is_string (source.filename));

  // should set dirname and own value
  jjs_esm_source_set_dirname (&source, jjs_string_sz ("."), true);
  TEST_ASSERT (jjs_value_is_string (source.dirname));

  // should set dirname and take a copy of value
  jjs_esm_source_set_dirname (&source, push_sz ("."), false);
  TEST_ASSERT (jjs_value_is_string (source.dirname));

  // should set filename + dirname and own the values
  jjs_esm_source_set_path (&source, jjs_string_sz ("."), true, jjs_string_sz ("test.js"), true);
  TEST_ASSERT (jjs_value_is_string (source.filename));
  TEST_ASSERT (jjs_value_is_string (source.dirname));

  // should set filename + dirname and take a copy of values
  jjs_esm_source_set_path (&source, push_sz ("."), false, push_sz ("test.js"), false);
  TEST_ASSERT (jjs_value_is_string (source.filename));
  TEST_ASSERT (jjs_value_is_string (source.dirname));

  // debug builds will assert if the set methods incorrectly handled reference management
  jjs_esm_source_deinit (&source);
} /* test_esm_source */

int
main (void)
{
  TEST_INIT ();

  jjs_init (JJS_INIT_EMPTY);

  // jjs_esm_source*()
  test_esm_source ();

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
