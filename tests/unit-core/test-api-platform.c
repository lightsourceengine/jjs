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

static const char* TEST_FILE = "./unit-fixtures/modules/a.mjs";

static void
test_platform_after_init (void)
{
  TEST_ASSERT (jjs_platform_os_type () != JJS_PLATFORM_OS_UNKNOWN);
  TEST_ASSERT (jjs_platform_arch_type () != JJS_PLATFORM_ARCH_UNKNOWN);

  jjs_context_t *context_p;
  TEST_ASSERT (jjs_context_new (NULL, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (context_p != NULL);

  jjs_value_t os;
  jjs_value_t arch;

  TEST_ASSERT (jjs_platform_os_type () != JJS_PLATFORM_OS_UNKNOWN);
  TEST_ASSERT (jjs_value_is_string (context_p, os = jjs_platform_os (context_p)));
  TEST_ASSERT (jjs_platform_arch_type () != JJS_PLATFORM_ARCH_UNKNOWN);
  TEST_ASSERT (jjs_value_is_string (context_p, arch = jjs_platform_arch (context_p)));
  TEST_ASSERT (jjs_platform_has_cwd (context_p));
  TEST_ASSERT (jjs_platform_has_realpath (context_p));
  TEST_ASSERT (jjs_platform_has_read_file (context_p));

  jjs_value_free (context_p, os);
  jjs_value_free (context_p, arch);

  jjs_context_free (context_p);
}

static void
test_platform_cwd (void)
{
  jjs_context_t *context_p;
  TEST_ASSERT (jjs_context_new (NULL, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (context_p != NULL);

  jjs_value_t path;

  TEST_ASSERT (jjs_platform_has_cwd (context_p));
  TEST_ASSERT (jjs_value_is_string (context_p, path = jjs_platform_cwd (context_p)));

  jjs_value_free (context_p, path);

  jjs_context_free (context_p);
}

static void
test_platform_cwd_not_set (void)
{
//  jjs_context_options_t options = jjs_context_options ();
//
//  options.platform.path_cwd = NULL;
//
//  TEST_CONTEXT_NEW_OPTS (context_p, &options);
//
//  TEST_ASSERT (!jjs_platform_has_cwd ());
//  TEST_ASSERT (jjs_value_is_exception (push (jjs_platform_cwd ())));
//
//  free_values ();
//  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_realpath (void)
{
  ctx_open (NULL);

  jjs_value_t path;

  TEST_ASSERT (jjs_platform_has_realpath (ctx ()));

  path = jjs_platform_realpath (ctx (), ctx_cstr ("."), JJS_KEEP);
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_value (path)));

  path = jjs_platform_realpath (ctx (), ctx_cstr (TEST_FILE), JJS_KEEP);
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_value (path)));

  path = jjs_platform_realpath (ctx (), ctx_cstr ("does not exist"), JJS_KEEP);
  TEST_ASSERT (jjs_value_is_exception (ctx (), ctx_value (path)));

  path = jjs_platform_realpath (ctx (), ctx_null (), JJS_KEEP);
  TEST_ASSERT (jjs_value_is_exception (ctx (), ctx_value (path)));

  ctx_close ();
}

static void
test_platform_read_file (void)
{
  ctx_open (NULL);

  TEST_ASSERT (jjs_platform_has_read_file (ctx ()));

  jjs_value_t contents;
  jjs_platform_read_file_options_t options = { 0 };

  options.encoding = JJS_ENCODING_UTF8;
  contents = jjs_platform_read_file (ctx (), jjs_string_utf8_sz (ctx (), TEST_FILE), JJS_MOVE, &options);
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_value (contents)));

  options.encoding = JJS_ENCODING_CESU8;
  contents = jjs_platform_read_file (ctx (), jjs_string_utf8_sz (ctx (), TEST_FILE), JJS_MOVE, &options);
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_value (contents)));

  options.encoding = JJS_ENCODING_NONE;
  contents = jjs_platform_read_file (ctx (), jjs_string_utf8_sz (ctx (), TEST_FILE), JJS_MOVE, &options);
  TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), ctx_value (contents)));

  options.encoding = JJS_ENCODING_NONE;
  contents = jjs_platform_read_file (ctx (), jjs_string_utf8_sz (ctx (), "file not found"), JJS_MOVE, &options);
  TEST_ASSERT (jjs_value_is_exception (ctx (), ctx_value (contents)));

  options.encoding = JJS_ENCODING_NONE;
  contents = jjs_platform_read_file (ctx (), jjs_null (ctx ()), JJS_MOVE, &options);
  TEST_ASSERT (jjs_value_is_exception (ctx (), ctx_value (contents)));

  ctx_close ();
}

static void
test_platform_realpath_not_set (void)
{
//  jjs_context_options_t options = jjs_context_options ();
//
//  options.platform.path_realpath = NULL;
//
//  TEST_CONTEXT_NEW_OPTS (context_p, &options);
//
//  TEST_ASSERT (!jjs_platform_has_realpath ());
//  jjs_value_t path = push (jjs_platform_realpath (jjs_string_utf8_sz ("."), JJS_MOVE));
//  TEST_ASSERT (jjs_value_is_exception (path));
//
//  free_values ();
//  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_read_file_not_set (void)
{
//  jjs_context_options_t options = jjs_context_options ();
//
//  options.platform.fs_read_file = NULL;
//
//  TEST_CONTEXT_NEW_OPTS (context_p, &options);
//
//  TEST_ASSERT (!jjs_platform_has_read_file ());
//  jjs_value_t path = push (jjs_platform_read_file (jjs_string_utf8_sz (TEST_FILE), JJS_MOVE, NULL));
//  TEST_ASSERT (jjs_value_is_exception (path));
//
//  free_values ();
//  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_stream (void)
{
  jjs_context_t *context_p;
  TEST_ASSERT (jjs_context_new (NULL, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (context_p != NULL);

  TEST_ASSERT (jjs_platform_has_stdout (context_p) == true);

  jjs_platform_stdout_write (context_p, jjs_string_sz (context_p, "hello\n"), JJS_MOVE);
  jjs_platform_stdout_flush (context_p);

  TEST_ASSERT (jjs_platform_has_stderr (context_p) == true);

  jjs_platform_stderr_write (context_p, jjs_string_sz (context_p, "hello\n"), JJS_MOVE);
  jjs_platform_stderr_flush (context_p);

  jjs_context_free (context_p);
}

static void
test_platform_stream_requirements (void)
{
//  jjs_context_t *context_p;
//  jjs_context_options_t options;
//
//  /* invalid default encoding */
//  options = jjs_context_options ();
//  options.platform.io_stdout_encoding = JJS_ENCODING_UTF16;
//  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_CONTEXT_STDOUT_INVALID_ENCODING);
//
//  options = jjs_context_options ();
//  options.platform.io_stderr_encoding = JJS_ENCODING_UTF16;
//  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_CONTEXT_STDERR_INVALID_ENCODING);
//
//  /* streams without write function */
//  options = jjs_context_options ();
//  options.platform.io_write = NULL;
//  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
//  TEST_ASSERT (jjs_platform_has_stdout () == false);
//  TEST_ASSERT (jjs_platform_has_stderr () == false);
//  TEST_CONTEXT_FREE (context_p);
//
//  /* no streams */
//  options = jjs_context_options ();
//  options.platform.io_stdout = NULL;
//  options.platform.io_stderr = NULL;
//  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
//  TEST_ASSERT (jjs_platform_has_stdout () == false);
//  TEST_ASSERT (jjs_platform_has_stderr () == false);
//  TEST_CONTEXT_FREE (context_p);
//
//  /* default init */
//  options = jjs_context_options ();
//  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
//  TEST_ASSERT (jjs_platform_has_stdout () == true);
//  TEST_ASSERT (jjs_platform_has_stderr () == true);
//  TEST_CONTEXT_FREE (context_p);
//
//  /* no flush */
//  options = jjs_context_options ();
//  options.platform.io_flush = NULL;
//  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
//  TEST_ASSERT (jjs_platform_has_stdout () == true);
//  TEST_ASSERT (jjs_platform_has_stderr () == true);
//  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_debugger_requirements (void)
{
//#if JJS_DEBUGGER
//  jjs_context_t *context_p;
//  jjs_context_options_t options = jjs_context_options ();
//
//  options.platform.time_sleep = NULL;
//
//  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_CONTEXT_REQUIRES_API_TIME_SLEEP);
//#endif
}

static void
test_platform_date_requirements (void)
{
//#if JJS_BUILTIN_DATE
//  jjs_context_t *context_p;
//  jjs_context_options_t options = jjs_context_options ();
//
//  TEST_ASSERT (options.platform.time_local_tza != NULL);
//  options.platform.time_local_tza = NULL;
//
//  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_CONTEXT_REQUIRES_API_TIME_LOCAL_TZA);
//
//  options = jjs_context_options ();
//
//  TEST_ASSERT (options.platform.time_now_ms != NULL);
//  options.platform.time_now_ms = NULL;
//
//  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_CONTEXT_REQUIRES_API_TIME_NOW_MS);
//#endif
}

static void
test_jjs_namespace (void)
{
  jjs_context_t *context_p;
  TEST_ASSERT (jjs_context_new (NULL, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (context_p != NULL);

  jjs_value_t global = jjs_current_realm (context_p);
  jjs_value_t jjs = jjs_object_get_sz (context_p, global, "jjs");
  jjs_value_t result;

  TEST_ASSERT (jjs_value_is_object (context_p, jjs));
  TEST_ASSERT (jjs_value_is_string (context_p, result = jjs_object_get_sz (context_p, jjs, "version")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_string (context_p, result = jjs_object_get_sz (context_p, jjs, "os")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_string (context_p, result = jjs_object_get_sz (context_p, jjs, "arch")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_object (context_p, result = jjs_object_get_sz (context_p, jjs, "stdout")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_object (context_p, result = jjs_object_get_sz (context_p, jjs, "stderr")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_function (context_p, result = jjs_object_get_sz (context_p, jjs, "pmap")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_function (context_p, result = jjs_object_get_sz (context_p, jjs, "vmod")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_function (context_p, result = jjs_object_get_sz (context_p, jjs, "readFile")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_function (context_p, result = jjs_object_get_sz (context_p, jjs, "realpath")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_function (context_p, result = jjs_object_get_sz (context_p, jjs, "cwd")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_function (context_p, result = jjs_object_get_sz (context_p, jjs, "gc")));
  jjs_value_free (context_p, result);

  jjs_value_free (context_p, global);
  jjs_value_free (context_p, jjs);

  jjs_context_free (context_p);
}

static void
check_jjs_namespace_exclusion (jjs_namespace_exclusion_t exclusion, const char* api_name_sz)
{
  jjs_context_t *context_p;
  jjs_context_options_t options = {
    .jjs_namespace_exclusions = exclusion,
  };

  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (context_p != NULL);

  jjs_value_t global = jjs_current_realm (context_p);
  jjs_value_t jjs = jjs_object_get_sz (context_p, global, "jjs");
  jjs_value_t result;

  /* sanity check that jjs object exists and basics are populated */
  TEST_ASSERT (jjs_value_is_object (context_p, jjs));
  TEST_ASSERT (jjs_value_is_string (context_p, result = jjs_object_get_sz (context_p, jjs, "version")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_string (context_p, result = jjs_object_get_sz (context_p, jjs, "os")));
  jjs_value_free (context_p, result);
  TEST_ASSERT (jjs_value_is_string (context_p, result = jjs_object_get_sz (context_p, jjs, "arch")));
  jjs_value_free (context_p, result);

  TEST_ASSERT (jjs_value_is_false (context_p, result = jjs_object_has_sz (context_p, jjs, api_name_sz)));
  jjs_value_free (context_p, result);

  jjs_value_free (context_p, global);
  jjs_value_free (context_p, jjs);

  jjs_context_free (context_p);
}

static void
test_jjs_namespace_exclusions (void)
{
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_READ_FILE, "readFile");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_STDOUT, "stdout");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_STDERR, "stderr");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_PMAP, "pmap");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_VMOD, "vmod");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_REALPATH, "realpath");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_CWD, "cwd");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_GC, "gc");
}

int
main (void)
{
  test_platform_after_init ();

  test_platform_cwd ();
  test_platform_cwd_not_set ();

  test_platform_realpath ();
  test_platform_realpath_not_set ();

  test_platform_read_file ();
  test_platform_read_file_not_set ();

  test_platform_stream ();

//  test_platform_api_exists ();

  test_platform_date_requirements ();
  test_platform_debugger_requirements ();
  test_platform_stream_requirements ();

  test_jjs_namespace ();
  test_jjs_namespace_exclusions ();

  return 0;
} /* main */
