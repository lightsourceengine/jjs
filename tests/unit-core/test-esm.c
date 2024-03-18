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
check_namespace (jjs_value_t ns, const char* key, jjs_value_t expected)
{
  push (ns);
  push (expected);
  TEST_ASSERT (!jjs_value_is_exception (ns));
  TEST_ASSERT (strict_equals (push (jjs_object_get_sz (ns, key)), expected));
}

static void
check_namespace_sz (jjs_value_t ns, const char* key, const char* expected)
{
  push (ns);
  TEST_ASSERT (!jjs_value_is_exception (ns));
  TEST_ASSERT (strict_equals_cstr (push (jjs_object_get_sz (ns, key)), expected));
}

static void
check_namespace_int32 (jjs_value_t ns, const char* key, int32_t expected)
{
  push (ns);
  TEST_ASSERT (!jjs_value_is_exception (ns));
  TEST_ASSERT (strict_equals_int32 (push (jjs_object_get_sz (ns, key)), expected));
}

static void
check_evaluate_int32 (jjs_value_t value, int32_t expected)
{
  push (value);
  TEST_ASSERT (!jjs_value_is_exception (value));
  TEST_ASSERT (strict_equals_int32 (value, expected));
}

static void
check_ok (jjs_value_t value)
{
  push (value);
  TEST_ASSERT (!jjs_value_is_exception (value));
  TEST_ASSERT (!jjs_value_is_exception (push (jjs_run_jobs ())));
}

static void
check_exception (jjs_value_t value)
{
  push (value);
  TEST_ASSERT (jjs_value_is_exception (value));
}

static void
test_invalid_jjs_esm_import_arg (void)
{
  check_exception (jjs_esm_import (push (jjs_null ())));
  check_exception (jjs_esm_import (push (jjs_undefined ())));
  check_exception (jjs_esm_import (push (jjs_number (0))));
  check_exception (jjs_esm_import (push (jjs_boolean (true))));
  check_exception (jjs_esm_import (push (jjs_object ())));
  check_exception (jjs_esm_import (push (jjs_array (0))));
  check_exception (jjs_esm_import (push (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG))));
}

static void
test_invalid_jjs_esm_import_sz_arg (void)
{
  check_exception (jjs_esm_import_sz (NULL));
  check_exception (jjs_esm_import_sz (""));
  check_exception (jjs_esm_import_sz ("unknown"));
  check_exception (jjs_esm_import_sz ("./unknown"));
  check_exception (jjs_esm_import_sz ("../unknown"));
  check_exception (jjs_esm_import_sz ("/unknown"));
}

static void
test_invalid_jjs_esm_evaluate_arg (void)
{
  check_exception (jjs_esm_evaluate (push (jjs_null ())));
  check_exception (jjs_esm_evaluate (push (jjs_undefined ())));
  check_exception (jjs_esm_evaluate (push (jjs_number (0))));
  check_exception (jjs_esm_evaluate (push (jjs_boolean (true))));
  check_exception (jjs_esm_evaluate (push (jjs_object ())));
  check_exception (jjs_esm_evaluate (push (jjs_array (0))));
  check_exception (jjs_esm_evaluate (push (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG))));
}

static void
test_invalid_jjs_esm_evaluate_sz_arg (void)
{
  check_exception (jjs_esm_evaluate_sz (NULL));
  check_exception (jjs_esm_evaluate_sz (""));
  check_exception (jjs_esm_evaluate_sz ("unknown"));
  check_exception (jjs_esm_evaluate_sz ("./unknown"));
  check_exception (jjs_esm_evaluate_sz ("../unknown"));
  check_exception (jjs_esm_evaluate_sz ("/unknown"));
}

static void
test_esm_import_relative_path (void)
{
  check_namespace_sz (jjs_esm_import (push_sz (TEST_MODULE_A)), "default", "a");
  check_namespace_sz (jjs_esm_import_sz (TEST_MODULE_A), "default", "a");
}

static void
test_esm_import_relative_path_from_script (void)
{
  check_namespace_sz (jjs_esm_import (push_sz (TEST_MODULE_NESTED)), "default", "a");
  check_namespace_sz (jjs_esm_import_sz (TEST_MODULE_NESTED), "default", "a");

  check_ok (jjs_esm_import (push_sz (TEST_MODULE_CIRCULAR)));
  check_ok (jjs_esm_import_sz (TEST_MODULE_CIRCULAR));
}

static void
test_esm_import_absolute_path (void)
{
  jjs_char_t* full_path = jjs_port_path_normalize (JJS_STR (TEST_MODULE_A), JJS_STRLEN (TEST_MODULE_A));

  check_namespace_sz (jjs_esm_import_sz ((const char*) full_path), "default", "a");

  jjs_port_path_free (full_path);
}

static void
test_esm_import_source (void)
{
  const char* source = "export default 5;";

  check_namespace_int32 (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), NULL), "default", 5);
  check_namespace_int32 (jjs_esm_import_source_value (push_sz (source), NULL), "default", 5);
}

static void
test_esm_import_source_options (void)
{
  const char* source;
  jjs_esm_options_t options;
  jjs_value_t expected;

  // meta_extension
  source = "export default import.meta.extension";
  options = jjs_esm_options_init ();

  options.meta_extension = jjs_number_from_int32 (7);

  check_namespace_int32 (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), &options), "default", 7);
  check_namespace_int32 (jjs_esm_import_source_value (push_sz (source), &options), "default", 7);

  jjs_esm_options_free (&options);

  // filename: default
  source = "export default import.meta.filename";
  options = jjs_esm_options_init ();
  expected = annex_path_join (push (annex_path_cwd ()), push_sz ("<anonymous>.mjs"), false);

  check_namespace (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), &options), "default", expected);
  check_namespace (jjs_esm_import_source_value (push_sz (source), &options), "default", jjs_value_copy (expected));

  jjs_esm_options_free (&options);

  // filename: override
  source = "export default import.meta.filename";
  options = jjs_esm_options_init ();
  options.filename = jjs_string_sz ("override.js");
  expected = annex_path_join (push (annex_path_cwd ()), push_sz ("override.js"), false);

  check_namespace (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), &options), "default", expected);
  check_namespace (jjs_esm_import_source_value (push_sz (source), &options), "default", jjs_value_copy (expected));

  jjs_esm_options_free (&options);

  // dirname: default
  source = "export default import.meta.dirname";
  options = jjs_esm_options_init ();
  expected = annex_path_cwd ();

  check_namespace (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), &options), "default", expected);
  check_namespace (jjs_esm_import_source_value (push_sz (source), &options), "default", jjs_value_copy (expected));

  jjs_esm_options_free (&options);

  // dirname: override
  // dirname must exist on fs, as it goes through realpath. / will work cross platform.
  source = "export default import.meta.dirname";
  options = jjs_esm_options_init ();
  options.dirname = annex_path_normalize (push_sz ("/"));
  expected = annex_path_normalize (push_sz ("/"));

  check_namespace (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), &options), "default", expected);
  check_namespace (jjs_esm_import_source_value (push_sz (source), &options), "default", jjs_value_copy (expected));

  jjs_esm_options_free (&options);
}

static void
test_esm_import_source_exceptions (void)
{
  const char* source = "export default 1";
  jjs_esm_options_t options;

  check_exception (
    jjs_esm_import_source (JJS_STR (TEST_SOURCE_PARSE_ERROR), JJS_STRLEN (TEST_SOURCE_PARSE_ERROR), NULL));
  check_exception (jjs_esm_import_source_value (push_sz (TEST_SOURCE_PARSE_ERROR), NULL));

  check_exception (jjs_esm_import_source (JJS_STR (TEST_SOURCE_LINK_ERROR), JJS_STRLEN (TEST_SOURCE_LINK_ERROR), NULL));
  check_exception (jjs_esm_import_source_value (push_sz (TEST_SOURCE_LINK_ERROR), NULL));

  check_exception (
    jjs_esm_import_source (JJS_STR (TEST_SOURCE_EVALUATE_ERROR), JJS_STRLEN (TEST_SOURCE_EVALUATE_ERROR), NULL));
  check_exception (jjs_esm_import_source_value (push_sz (TEST_SOURCE_EVALUATE_ERROR), NULL));

  options = jjs_esm_options_init ();
  options.filename = jjs_string_sz ("");
  check_exception (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), &options));
  check_exception (jjs_esm_import_source_value (push_sz (source), &options));
  jjs_esm_options_free (&options);

  options = jjs_esm_options_init ();
  options.filename = jjs_number_from_int32 (0);
  check_exception (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), &options));
  check_exception (jjs_esm_import_source_value (push_sz (source), &options));
  jjs_esm_options_free (&options);

  options = jjs_esm_options_init ();
  options.dirname = jjs_string_sz ("/path-does-not-exist");
  check_exception (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), &options));
  check_exception (jjs_esm_import_source_value (push_sz (source), &options));
  jjs_esm_options_free (&options);

  options = jjs_esm_options_init ();
  options.dirname = jjs_number_from_int32 (0);
  check_exception (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), &options));
  check_exception (jjs_esm_import_source_value (push_sz (source), &options));
  jjs_esm_options_free (&options);

  options = jjs_esm_options_init ();
  options.cache = true;
  options.filename = jjs_string_sz ("cached.js");
  check_ok (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), &options));
  check_exception (jjs_esm_import_source (JJS_STR (source), JJS_STRLEN (source), &options));
  check_exception (jjs_esm_import_source_value (push_sz (source), &options));
  jjs_esm_options_free (&options);
}

static void
test_esm_evaluate_source (void)
{
  const char* source = "5";

  check_evaluate_int32 (jjs_esm_evaluate_source (JJS_STR (source), JJS_STRLEN (source), NULL), 5);
  check_evaluate_int32 (jjs_esm_evaluate_source_value (push_sz (source), NULL), 5);
}

int
main (void)
{
  TEST_INIT ();

  jjs_init (JJS_INIT_EMPTY);

  test_esm_import_relative_path ();
  test_esm_import_relative_path_from_script ();
  test_esm_import_absolute_path ();
  test_invalid_jjs_esm_import_arg ();
  test_invalid_jjs_esm_import_sz_arg ();

  test_invalid_jjs_esm_evaluate_arg ();
  test_invalid_jjs_esm_evaluate_sz_arg ();

  test_esm_import_source ();
  test_esm_import_source_options ();
  test_esm_import_source_exceptions ();

  test_esm_evaluate_source ();

  free_values ();
  jjs_cleanup ();
  return 0;
} /* main */
