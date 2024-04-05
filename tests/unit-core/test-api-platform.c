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

static void
test_platform_after_init (void)
{
  jjs_init_default ();

  TEST_ASSERT (jjs_value_is_string (push (jjs_platform_os ())));
  TEST_ASSERT (jjs_value_is_string (push (jjs_platform_arch ())));
  TEST_ASSERT (jjs_platform_has_cwd ());
  TEST_ASSERT (jjs_platform_has_realpath ());
  TEST_ASSERT (jjs_platform_has_read_file ());

  free_values ();
  jjs_cleanup ();
}

static void
test_platform_os_override (void)
{
  static const char* TEST_OS = "override_os";
  jjs_context_options_t options = jjs_context_options ();

  jjs_platform_set_os_sz (&options.platform, TEST_OS);

  jjs_init (&options);

  TEST_ASSERT (strict_equals_cstr (push (jjs_platform_os ()), TEST_OS));

  free_values ();
  jjs_cleanup ();
}

static void
test_platform_arch_override (void)
{
  static const char* TEST_ARCH = "override_arch";
  jjs_context_options_t options = jjs_context_options ();

  jjs_platform_set_arch_sz (&options.platform, TEST_ARCH);

  jjs_init (&options);

  TEST_ASSERT (strict_equals_cstr (push (jjs_platform_arch ()), TEST_ARCH));

  free_values ();
  jjs_cleanup ();
}

static void
test_platform_set_os_sz (void)
{
  const char* OVERRIDE = "test";
  jjs_platform_t platform = {0};

  TEST_ASSERT (jjs_platform_set_os_sz (&platform, OVERRIDE));
  TEST_ASSERT (strcmp (OVERRIDE, &platform.os_sz[0]) == 0);
  TEST_ASSERT (jjs_platform_set_os_sz (&platform, NULL) == false);
  TEST_ASSERT (jjs_platform_set_os_sz (&platform, "") == false);
  TEST_ASSERT (jjs_platform_set_os_sz (&platform, "012345678901234567890") == false);
}

static void
test_platform_set_arch_sz (void)
{
  const char* OVERRIDE = "test";
  jjs_platform_t platform = {0};

  TEST_ASSERT (jjs_platform_set_arch_sz (&platform, OVERRIDE));
  TEST_ASSERT (strcmp (OVERRIDE, &platform.arch_sz[0]) == 0);
  TEST_ASSERT (jjs_platform_set_arch_sz (&platform, NULL) == false);
  TEST_ASSERT (jjs_platform_set_arch_sz (&platform, "") == false);
  TEST_ASSERT (jjs_platform_set_arch_sz (&platform, "012345678901234567890") == false);
}

static void
test_platform_cwd (void)
{
  jjs_init_default ();

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

  jjs_init (&options);

  TEST_ASSERT (!jjs_platform_has_cwd ());
  TEST_ASSERT (jjs_value_is_exception (push (jjs_platform_cwd ())));

  free_values ();
  jjs_cleanup ();
}

static void
test_platform_realpath (void)
{
  jjs_init_default ();

  TEST_ASSERT (jjs_platform_has_realpath ());
  jjs_value_t path = push (jjs_platform_realpath (jjs_string_utf8_sz ("."), JJS_MOVE));
  TEST_ASSERT (jjs_value_is_string (path));

  free_values ();
  jjs_cleanup ();
}

static void
test_platform_realpath_not_set (void)
{
  jjs_context_options_t options = jjs_context_options ();

  options.platform.path_realpath = NULL;

  jjs_init (&options);

  TEST_ASSERT (!jjs_platform_has_realpath ());
  jjs_value_t path = push (jjs_platform_realpath (jjs_string_utf8_sz ("."), JJS_MOVE));
  TEST_ASSERT (jjs_value_is_exception (path));

  free_values ();
  jjs_cleanup ();
}

static void
test_platform_debugger_requirements (void)
{
#if JJS_DEBUGGER
  jjs_context_options_t options = jjs_context_options();

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

  TEST_ASSERT (options.platform.io_log != NULL);
}

int
main (void)
{
  test_platform_after_init ();

  test_platform_set_arch_sz ();
  test_platform_set_os_sz ();

  test_platform_os_override ();
  test_platform_arch_override ();

  test_platform_cwd ();
  test_platform_cwd_not_set ();

  test_platform_realpath ();
  test_platform_realpath_not_set ();

  test_platform_api_exists ();

  test_platform_date_requirements ();
  test_platform_debugger_requirements ();

  return 0;
} /* main */
