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

  TEST_ASSERT (jjs_value_is_string (context_p, path = jjs_platform_cwd (context_p)));

  jjs_value_free (context_p, path);

  jjs_context_free (context_p);
}

static void
test_platform_realpath (void)
{
  ctx_open (NULL);

  jjs_value_t path;

  path = jjs_platform_realpath (ctx (), ctx_cstr ("."), JJS_KEEP);
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_defer_free (path)));

  path = jjs_platform_realpath (ctx (), ctx_cstr (TEST_FILE), JJS_KEEP);
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_defer_free (path)));

  path = jjs_platform_realpath (ctx (), ctx_cstr ("does not exist"), JJS_KEEP);
  TEST_ASSERT (jjs_value_is_exception (ctx (), ctx_defer_free (path)));

  path = jjs_platform_realpath (ctx (), ctx_null (), JJS_KEEP);
  TEST_ASSERT (jjs_value_is_exception (ctx (), ctx_defer_free (path)));

  ctx_close ();
}

static void
test_platform_read_file (void)
{
  ctx_open (NULL);

  jjs_value_t contents;
  jjs_platform_read_file_options_t options = { 0 };

  options.encoding = JJS_ENCODING_UTF8;
  contents = jjs_platform_read_file (ctx (), jjs_string_utf8_sz (ctx (), TEST_FILE), JJS_MOVE, &options);
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_defer_free (contents)));

  options.encoding = JJS_ENCODING_CESU8;
  contents = jjs_platform_read_file (ctx (), jjs_string_utf8_sz (ctx (), TEST_FILE), JJS_MOVE, &options);
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_defer_free (contents)));

  options.encoding = JJS_ENCODING_NONE;
  contents = jjs_platform_read_file (ctx (), jjs_string_utf8_sz (ctx (), TEST_FILE), JJS_MOVE, &options);
  TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), ctx_defer_free (contents)));

  options.encoding = JJS_ENCODING_NONE;
  contents = jjs_platform_read_file (ctx (), jjs_string_utf8_sz (ctx (), "file not found"), JJS_MOVE, &options);
  TEST_ASSERT (jjs_value_is_exception (ctx (), ctx_defer_free (contents)));

  options.encoding = JJS_ENCODING_NONE;
  contents = jjs_platform_read_file (ctx (), jjs_null (ctx ()), JJS_MOVE, &options);
  TEST_ASSERT (jjs_value_is_exception (ctx (), ctx_defer_free (contents)));

  ctx_close ();
}

static void
test_platform_stream (void)
{
  jjs_context_t *context_p;
  TEST_ASSERT (jjs_context_new (NULL, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (context_p != NULL);

  jjs_platform_io_write (context_p, JJS_STDOUT, jjs_string_sz (context_p, "hello\n"), JJS_MOVE);
  jjs_platform_io_flush (context_p, JJS_STDOUT);

  jjs_platform_io_write (context_p, JJS_STDERR, jjs_string_sz (context_p, "hello\n"), JJS_MOVE);
  jjs_platform_io_flush (context_p, JJS_STDERR);

  jjs_context_free (context_p);
}

int
main (void)
{
  test_platform_after_init ();

  test_platform_cwd ();

  test_platform_realpath ();

  test_platform_read_file ();

  test_platform_stream ();

  return 0;
} /* main */
