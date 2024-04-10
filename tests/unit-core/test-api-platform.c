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

  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  TEST_ASSERT (jjs_platform_os () != JJS_PLATFORM_OS_UNKNOWN);
  TEST_ASSERT (jjs_value_is_string (push (jjs_platform_os ())));
  TEST_ASSERT (jjs_platform_arch () != JJS_PLATFORM_ARCH_UNKNOWN);
  TEST_ASSERT (jjs_value_is_string (push (jjs_platform_arch ())));
  TEST_ASSERT (jjs_platform_has_cwd ());
  TEST_ASSERT (jjs_platform_has_realpath ());
  TEST_ASSERT (jjs_platform_has_read_file ());

  free_values ();
  jjs_cleanup ();
}

static void
test_platform_cwd (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  TEST_ASSERT (jjs_platform_has_cwd ());
  TEST_ASSERT (jjs_value_is_string (push (jjs_platform_cwd ())));

  free_values ();
  jjs_cleanup ();
}

static void
test_platform_cwd_not_set (void)
{
  jjs_context_options_t options = jjs_context_options ();

  options.platform.path_cwd = NULL;

  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_OK);

  TEST_ASSERT (!jjs_platform_has_cwd ());
  TEST_ASSERT (jjs_value_is_exception (push (jjs_platform_cwd ())));

  free_values ();
  jjs_cleanup ();
}

static void
test_platform_realpath (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);
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
  jjs_cleanup ();
}

static void
test_platform_read_file (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

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
  jjs_cleanup ();
}

static void
test_platform_realpath_not_set (void)
{
  jjs_context_options_t options = jjs_context_options ();

  options.platform.path_realpath = NULL;

  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_OK);

  TEST_ASSERT (!jjs_platform_has_realpath ());
  jjs_value_t path = push (jjs_platform_realpath (jjs_string_utf8_sz ("."), JJS_MOVE));
  TEST_ASSERT (jjs_value_is_exception (path));

  free_values ();
  jjs_cleanup ();
}

static void
test_platform_read_file_not_set (void)
{
  jjs_context_options_t options = jjs_context_options ();

  options.platform.fs_read_file = NULL;

  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_OK);

  TEST_ASSERT (!jjs_platform_has_read_file ());
  jjs_value_t path = push (jjs_platform_read_file (jjs_string_utf8_sz (TEST_FILE), JJS_MOVE, NULL));
  TEST_ASSERT (jjs_value_is_exception (path));

  free_values ();
  jjs_cleanup ();
}

static void
test_platform_stream (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  TEST_ASSERT (jjs_platform_has_stdout () == true);

  jjs_platform_stdout_write (jjs_string_sz ("hello\n"), JJS_MOVE);
  jjs_platform_stdout_flush ();

  TEST_ASSERT (jjs_platform_has_stderr () == true);

  jjs_platform_stderr_write (jjs_string_sz ("hello\n"), JJS_MOVE);
  jjs_platform_stderr_flush ();

  jjs_cleanup ();
}

static void
test_platform_stream_requirements (void)
{
  jjs_context_options_t options;

  /* invalid default encoding */
  options = jjs_context_options ();
  options.platform.io_stdout_encoding = JJS_ENCODING_UTF16;
  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_STDOUT_INVALID_ENCODING);

  options = jjs_context_options ();
  options.platform.io_stderr_encoding = JJS_ENCODING_UTF16;
  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_STDERR_INVALID_ENCODING);

  /* streams without write function */
  options = jjs_context_options ();
  options.platform.io_write = NULL;
  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_OK);
  TEST_ASSERT (jjs_platform_has_stdout () == false);
  TEST_ASSERT (jjs_platform_has_stderr () == false);
  jjs_cleanup();

  /* no streams */
  options = jjs_context_options ();
  options.platform.io_stdout = NULL;
  options.platform.io_stderr = NULL;
  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_OK);
  TEST_ASSERT (jjs_platform_has_stdout () == false);
  TEST_ASSERT (jjs_platform_has_stderr () == false);
  jjs_cleanup();

  /* default init */
  options = jjs_context_options ();
  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_OK);
  TEST_ASSERT (jjs_platform_has_stdout () == true);
  TEST_ASSERT (jjs_platform_has_stderr () == true);
  jjs_cleanup();

  /* no flush */
  options = jjs_context_options ();
  options.platform.io_flush = NULL;
  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_OK);
  TEST_ASSERT (jjs_platform_has_stdout () == true);
  TEST_ASSERT (jjs_platform_has_stderr () == true);
  jjs_cleanup();
}

static void
test_platform_debugger_requirements (void)
{
#if JJS_DEBUGGER
  jjs_context_options_t options = jjs_context_options ();

  options.platform.time_sleep = NULL;

  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_REQUIRES_TIME_SLEEP);
#endif
}

static void
test_platform_date_requirements (void)
{
#if JJS_BUILTIN_DATE
  jjs_context_options_t options = jjs_context_options ();

  TEST_ASSERT (options.platform.time_local_tza != NULL);
  options.platform.time_local_tza = NULL;

  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_REQUIRES_TIME_LOCAL_TZA);

  options = jjs_context_options ();

  TEST_ASSERT (options.platform.time_now_ms != NULL);
  options.platform.time_now_ms = NULL;

  TEST_ASSERT (jjs_init (&options) == JJS_CONTEXT_STATUS_REQUIRES_TIME_NOW_MS);
#endif
}

static void
test_platform_api_exists (void)
{
  jjs_context_options_t options = jjs_context_options ();
  double unix_timestamp_ms;
  int32_t tza;

  TEST_ASSERT (options.platform.time_now_ms != NULL);
  TEST_ASSERT (JJS_PLATFORM_STATUS_OK == options.platform.time_now_ms (&unix_timestamp_ms));
  TEST_ASSERT (unix_timestamp_ms > 0);

  TEST_ASSERT (options.platform.time_local_tza != NULL);
  TEST_ASSERT (JJS_PLATFORM_STATUS_OK == options.platform.time_local_tza (unix_timestamp_ms, &tza));

  TEST_ASSERT (options.platform.time_sleep != NULL);
  TEST_ASSERT (JJS_PLATFORM_STATUS_OK == options.platform.time_sleep (1));

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

  return 0;
} /* main */
