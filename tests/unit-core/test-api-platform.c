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

static const char* TEST_FILE = "./unit-fixtures/modules/a.mjs";

static void
test_platform_after_init (void)
{
  TEST_ASSERT (jjs_platform_os_type () != JJS_PLATFORM_OS_UNKNOWN);
  TEST_ASSERT (jjs_platform_arch_type () != JJS_PLATFORM_ARCH_UNKNOWN);

  TEST_CONTEXT_NEW (context_p);

  TEST_ASSERT (jjs_platform_os () != JJS_PLATFORM_OS_UNKNOWN);
  TEST_ASSERT (jjs_value_is_string (push (jjs_platform_os ())));
  TEST_ASSERT (jjs_platform_arch () != JJS_PLATFORM_ARCH_UNKNOWN);
  TEST_ASSERT (jjs_value_is_string (push (jjs_platform_arch ())));
  TEST_ASSERT (jjs_platform_has_cwd ());
  TEST_ASSERT (jjs_platform_has_realpath ());
  TEST_ASSERT (jjs_platform_has_read_file ());

  free_values ();
  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_cwd (void)
{
  TEST_CONTEXT_NEW (context_p);

  TEST_ASSERT (jjs_platform_has_cwd ());
  TEST_ASSERT (jjs_value_is_string (push (jjs_platform_cwd ())));

  free_values ();
  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_cwd_not_set (void)
{
  jjs_context_options_t options = jjs_context_options ();

  options.platform.path_cwd = NULL;

  TEST_CONTEXT_NEW_OPTS (context_p, &options);

  TEST_ASSERT (!jjs_platform_has_cwd ());
  TEST_ASSERT (jjs_value_is_exception (push (jjs_platform_cwd ())));

  free_values ();
  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_realpath (void)
{
  TEST_CONTEXT_NEW (context_p);
  jjs_value_t path;

  TEST_ASSERT (jjs_platform_has_realpath ());

  path = push (jjs_platform_realpath (jjs_string_utf8_sz ("."), JJS_MOVE));
  TEST_ASSERT (jjs_value_is_string (path));

  path = push (jjs_platform_realpath (jjs_string_utf8_sz (TEST_FILE), JJS_MOVE));
  TEST_ASSERT (jjs_value_is_string (path));

  path = push (jjs_platform_realpath (jjs_string_utf8_sz ("does not exit"), JJS_MOVE));
  TEST_ASSERT (jjs_value_is_exception (path));

  path = push (jjs_platform_realpath (jjs_null (), JJS_MOVE));
  TEST_ASSERT (jjs_value_is_exception (path));

  free_values ();
  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_read_file (void)
{
  TEST_CONTEXT_NEW (context_p);

  TEST_ASSERT (jjs_platform_has_read_file ());

  jjs_value_t contents;

  jjs_platform_read_file_options_t options = { 0 };

  options.encoding = JJS_ENCODING_UTF8;
  contents = push (jjs_platform_read_file (jjs_string_utf8_sz (TEST_FILE), JJS_MOVE, &options));
  TEST_ASSERT (jjs_value_is_string (contents));

  options.encoding = JJS_ENCODING_CESU8;
  contents = push (jjs_platform_read_file (jjs_string_utf8_sz (TEST_FILE), JJS_MOVE, &options));
  TEST_ASSERT (jjs_value_is_string (contents));

  options.encoding = JJS_ENCODING_NONE;
  contents = push (jjs_platform_read_file (jjs_string_utf8_sz (TEST_FILE), JJS_MOVE, &options));
  TEST_ASSERT (jjs_value_is_arraybuffer (contents));

  options.encoding = JJS_ENCODING_NONE;
  contents = push (jjs_platform_read_file (jjs_string_utf8_sz ("file not found"), JJS_MOVE, &options));
  TEST_ASSERT (jjs_value_is_exception (contents));

  options.encoding = JJS_ENCODING_NONE;
  contents = push (jjs_platform_read_file (jjs_null (), JJS_MOVE, &options));
  TEST_ASSERT (jjs_value_is_exception (contents));

  free_values ();
  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_realpath_not_set (void)
{
  jjs_context_options_t options = jjs_context_options ();

  options.platform.path_realpath = NULL;

  TEST_CONTEXT_NEW_OPTS (context_p, &options);

  TEST_ASSERT (!jjs_platform_has_realpath ());
  jjs_value_t path = push (jjs_platform_realpath (jjs_string_utf8_sz ("."), JJS_MOVE));
  TEST_ASSERT (jjs_value_is_exception (path));

  free_values ();
  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_read_file_not_set (void)
{
  jjs_context_options_t options = jjs_context_options ();

  options.platform.fs_read_file = NULL;

  TEST_CONTEXT_NEW_OPTS (context_p, &options);

  TEST_ASSERT (!jjs_platform_has_read_file ());
  jjs_value_t path = push (jjs_platform_read_file (jjs_string_utf8_sz (TEST_FILE), JJS_MOVE, NULL));
  TEST_ASSERT (jjs_value_is_exception (path));

  free_values ();
  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_stream (void)
{
  TEST_CONTEXT_NEW (context_p);

  TEST_ASSERT (jjs_platform_has_stdout () == true);

  jjs_platform_stdout_write (jjs_string_sz ("hello\n"), JJS_MOVE);
  jjs_platform_stdout_flush ();

  TEST_ASSERT (jjs_platform_has_stderr () == true);

  jjs_platform_stderr_write (jjs_string_sz ("hello\n"), JJS_MOVE);
  jjs_platform_stderr_flush ();

  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_stream_requirements (void)
{
  jjs_context_t *context_p;
  jjs_context_options_t options;

  /* invalid default encoding */
  options = jjs_context_options ();
  options.platform.io_stdout_encoding = JJS_ENCODING_UTF16;
  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_CONTEXT_STDOUT_INVALID_ENCODING);

  options = jjs_context_options ();
  options.platform.io_stderr_encoding = JJS_ENCODING_UTF16;
  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_CONTEXT_STDERR_INVALID_ENCODING);

  /* streams without write function */
  options = jjs_context_options ();
  options.platform.io_write = NULL;
  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (jjs_platform_has_stdout () == false);
  TEST_ASSERT (jjs_platform_has_stderr () == false);
  TEST_CONTEXT_FREE (context_p);

  /* no streams */
  options = jjs_context_options ();
  options.platform.io_stdout = NULL;
  options.platform.io_stderr = NULL;
  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (jjs_platform_has_stdout () == false);
  TEST_ASSERT (jjs_platform_has_stderr () == false);
  TEST_CONTEXT_FREE (context_p);

  /* default init */
  options = jjs_context_options ();
  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (jjs_platform_has_stdout () == true);
  TEST_ASSERT (jjs_platform_has_stderr () == true);
  TEST_CONTEXT_FREE (context_p);

  /* no flush */
  options = jjs_context_options ();
  options.platform.io_flush = NULL;
  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (jjs_platform_has_stdout () == true);
  TEST_ASSERT (jjs_platform_has_stderr () == true);
  TEST_CONTEXT_FREE (context_p);
}

static void
test_platform_debugger_requirements (void)
{
#if JJS_DEBUGGER
  jjs_context_t *context_p;
  jjs_context_options_t options = jjs_context_options ();

  options.platform.time_sleep = NULL;

  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_CONTEXT_REQUIRES_API_TIME_SLEEP);
#endif
}

static void
test_platform_date_requirements (void)
{
#if JJS_BUILTIN_DATE
  jjs_context_t *context_p;
  jjs_context_options_t options = jjs_context_options ();

  TEST_ASSERT (options.platform.time_local_tza != NULL);
  options.platform.time_local_tza = NULL;

  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_CONTEXT_REQUIRES_API_TIME_LOCAL_TZA);

  options = jjs_context_options ();

  TEST_ASSERT (options.platform.time_now_ms != NULL);
  options.platform.time_now_ms = NULL;

  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_CONTEXT_REQUIRES_API_TIME_NOW_MS);
#endif
}

static void
test_platform_api_exists (void)
{
  jjs_context_options_t options = jjs_context_options ();
  double unix_timestamp_ms;
  int32_t tza;

  TEST_ASSERT (options.platform.time_now_ms != NULL);
  TEST_ASSERT (JJS_STATUS_OK == options.platform.time_now_ms (&unix_timestamp_ms));
  TEST_ASSERT (unix_timestamp_ms > 0);

  TEST_ASSERT (options.platform.time_local_tza != NULL);
  TEST_ASSERT (JJS_STATUS_OK == options.platform.time_local_tza (unix_timestamp_ms, &tza));

  TEST_ASSERT (options.platform.time_sleep != NULL);
  TEST_ASSERT (JJS_STATUS_OK == options.platform.time_sleep (1));

  TEST_ASSERT (options.platform.fatal != NULL);

  TEST_ASSERT (options.platform.path_cwd != NULL);
  TEST_ASSERT (options.platform.path_realpath != NULL);

  TEST_ASSERT (options.platform.fs_read_file != NULL);

  TEST_ASSERT (options.platform.io_flush != NULL);
  TEST_ASSERT (options.platform.io_write != NULL);
  TEST_ASSERT (options.platform.io_stdout != NULL);
  TEST_ASSERT (options.platform.io_stdout_encoding != JJS_ENCODING_NONE);
  TEST_ASSERT (options.platform.io_stderr != NULL);
  TEST_ASSERT (options.platform.io_stderr_encoding != JJS_ENCODING_NONE);
}

static void
test_jjs_namespace (void)
{
  TEST_CONTEXT_NEW (context_p);
  jjs_value_t global = push (jjs_current_realm ());
  jjs_value_t jjs = push (jjs_object_get_sz (global, "jjs"));
  
  TEST_ASSERT (jjs_value_is_object (jjs));
  TEST_ASSERT (jjs_value_is_string (push (jjs_object_get_sz (jjs, "version"))));
  TEST_ASSERT (jjs_value_is_string (push (jjs_object_get_sz (jjs, "os"))));
  TEST_ASSERT (jjs_value_is_string (push (jjs_object_get_sz (jjs, "arch"))));
  TEST_ASSERT (jjs_value_is_object (push (jjs_object_get_sz (jjs, "stdout"))));
  TEST_ASSERT (jjs_value_is_object (push (jjs_object_get_sz (jjs, "stderr"))));
  TEST_ASSERT (jjs_value_is_function (push (jjs_object_get_sz (jjs, "pmap"))));
  TEST_ASSERT (jjs_value_is_function (push (jjs_object_get_sz (jjs, "vmod"))));
  TEST_ASSERT (jjs_value_is_function (push (jjs_object_get_sz (jjs, "readFile"))));
  TEST_ASSERT (jjs_value_is_function (push (jjs_object_get_sz (jjs, "realpath"))));
  TEST_ASSERT (jjs_value_is_function (push (jjs_object_get_sz (jjs, "cwd"))));
  TEST_ASSERT (jjs_value_is_function (push (jjs_object_get_sz (jjs, "gc"))));

  free_values ();
  TEST_CONTEXT_FREE (context_p);
}

static void
check_jjs_namespace_exclusion (jjs_namespace_exclusion_t exclusion, const char* api_name_sz)
{
  jjs_context_options_t options = jjs_context_options ();

  options.jjs_namespace_exclusions = exclusion;
  TEST_CONTEXT_NEW_OPTS (context_p, &options);
  jjs_value_t global = push (jjs_current_realm ());
  jjs_value_t jjs = push (jjs_object_get_sz (global, "jjs"));

  /* sanity check that jjs object exists and basics are populated */
  TEST_ASSERT (jjs_value_is_object (jjs));
  TEST_ASSERT (jjs_value_is_string (push (jjs_object_get_sz (jjs, "version"))));
  TEST_ASSERT (jjs_value_is_string (push (jjs_object_get_sz (jjs, "os"))));
  TEST_ASSERT (jjs_value_is_string (push (jjs_object_get_sz (jjs, "arch"))));

  TEST_ASSERT (jjs_value_is_false (push (jjs_object_has_sz (jjs, api_name_sz))));

  free_values ();
  TEST_CONTEXT_FREE (context_p);
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

  test_platform_api_exists ();

  test_platform_date_requirements ();
  test_platform_debugger_requirements ();
  test_platform_stream_requirements ();

  test_jjs_namespace ();
  test_jjs_namespace_exclusions ();

  return 0;
} /* main */
