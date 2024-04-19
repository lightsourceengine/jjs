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

/**
 * jjs-test262
 *
 * JJS runtime build specifically for test262 test run by test262-harness.py. $262
 * and print are added to the global namespace. The source is always read from
 * stdin. The harness cat the includes and test source. The source is read in
 * sloppy mode. If a -m is passed, the test is parsed as an ES module. The test
 * filename (basename) is passed in for stack traces and imports to work.
 *
 * When called, the cwd should be the folder of the test file. The test filename
 * is passed in as well. Some tests import themselves or fixtures within their
 * own directory. The cwd and filename need to be set this way so import will
 * find files.
 */

#include <jjs.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static jjs_value_t create_262 (jjs_value_t realm);

static jjs_value_t
js_print (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  (void) call_info_p;

  jjs_value_t value = args_cnt > 0 ? args_p[0] : jjs_undefined ();
  jjs_value_t value_as_string =
    jjs_value_is_symbol (value) ? jjs_symbol_description (value) : jjs_value_to_string (value);

  if (!jjs_value_is_string (value_as_string))
  {
    jjs_value_free (value_as_string);
    value_as_string = jjs_string_sz ("Error converting exception to string.");
  }

  jjs_platform_stdout_write (value_as_string, JJS_MOVE);
  jjs_platform_stdout_write (jjs_string_sz ("\n"), JJS_MOVE);
  jjs_platform_stdout_flush ();

  return jjs_undefined ();
}

static void
object_set_value (jjs_value_t object, const char *key, jjs_value_t value, jjs_value_ownership_t value_o)
{
  jjs_value_t result = jjs_object_set_sz (object, key, value);

  if (value_o == JJS_MOVE)
  {
    jjs_value_free (value);
  }

  jjs_value_free (result);
}

static void
object_set_function (jjs_value_t object, const char *key, jjs_external_handler_t fn)
{
  object_set_value (object, key, jjs_function_external (fn), JJS_MOVE);
}

static jjs_value_t
js_262_detach_array_buffer (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  (void) call_info_p;

  if (args_cnt < 1 || !jjs_value_is_arraybuffer (args_p[0]))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Expected an ArrayBuffer object");
  }

  /* TODO: support the optional 'key' argument */

  return jjs_arraybuffer_detach (args_p[0]);
}

static jjs_value_t
js_262_eval_script (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  (void) call_info_p;

  if (args_cnt < 1 || !jjs_value_is_string (args_p[0]))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, "Expected a string");
  }

  jjs_value_t ret_value = jjs_parse_value (args_p[0], NULL);

  if (!jjs_value_is_exception (ret_value))
  {
    jjs_value_t func_val = ret_value;
    ret_value = jjs_run (func_val);
    jjs_value_free (func_val);
  }

  return ret_value;
}

static jjs_value_t
js_262_create_realm (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  (void) call_info_p;
  (void) args_p;
  (void) args_cnt;

  jjs_value_t realm_object = jjs_realm ();
  jjs_value_t previous_realm = jjs_set_realm (realm_object);
  assert (!jjs_value_is_exception (previous_realm));

  jjs_value_t test262_object = create_262 (realm_object);
  jjs_set_realm (previous_realm);
  jjs_value_free (realm_object);

  return test262_object;
}

static jjs_value_t
js_262_gc (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  (void) call_info_p;

  jjs_gc_mode_t mode =
    ((args_cnt > 0 && jjs_value_to_boolean (args_p[0])) ? JJS_GC_PRESSURE_HIGH : JJS_GC_PRESSURE_LOW);

  jjs_heap_gc (mode);
  return jjs_undefined ();
}

static jjs_value_t
create_262 (jjs_value_t realm)
{
  jjs_value_t value = jjs_object ();

  object_set_function (value, "detachArrayBuffer", js_262_detach_array_buffer);
  object_set_function (value, "evalScript", js_262_eval_script);
  object_set_function (value, "createRealm", js_262_create_realm);
  object_set_function (value, "gc", js_262_gc);

  jjs_value_t result = jjs_object_set_sz (value, "global", realm);
  assert (!jjs_value_is_exception (result));
  jjs_value_free (result);

  return value;
}

static bool
resolve_result_value (jjs_value_t result, jjs_value_ownership_t result_o)
{
  bool status = !jjs_value_is_exception (result);

  if (!status)
  {
    jjs_value_t value = jjs_exception_value (result, false);
    jjs_value_t value_as_string =
      jjs_value_is_symbol (value) ? jjs_symbol_description (value) : jjs_value_to_string (value);

    jjs_value_free (value);

    if (jjs_value_is_string (value_as_string))
    {
      jjs_platform_stderr_write (value_as_string, JJS_MOVE);
    }
    else
    {
      jjs_platform_stderr_write (jjs_string_sz ("Failed to toString() exception."), JJS_MOVE);
      jjs_value_free (value_as_string);
    }
  }

  if (result_o == JJS_MOVE)
  {
    jjs_value_free (result);
  }

  return status;
}

static void
print_usage (void)
{
  printf ("test262 JJS engine for use bby test262-harness.py\n\n");
  printf ("usage: echo $SOURCE | jjs-test262 - testFileName.js\n");
  printf ("       echo $SOURCE | jjs-test262 - -m testFileName.mjs\n");
}

int
main (int argc, char **argv)
{
  const char *test_filename = NULL;
  bool as_module = false;
  bool from_stdin = false;

  srand ((unsigned) time (NULL));

  /* parse arguments */
  for (int i = 0; i < argc; i++)
  {
    if (strcmp (argv[i], "-") == 0)
    {
      from_stdin = true;
    }
    else if (strcmp (argv[i], "-m") == 0)
    {
      as_module = true;
    }
    else if (strcmp (argv[i], "--help") == 0)
    {
      print_usage ();
      return 1;
    }
    else
    {
      test_filename = argv[i];
    }
  }

  if (!from_stdin)
  {
    printf ("Error: missing '-' to indicate source is being read from stdin");
    return 1;
  }

  if (!test_filename)
  {
    printf ("Error: missing filename. used for stack traces and module loading");
    return 1;
  }

  /* being lazy. the test262 tests are small so 100K is more than enough. saves reallocing and buffer math. */
  uint8_t source_buffer [100 * 1024];
  static const size_t SOURCE_BUFFER_SIZE = sizeof (source_buffer) / sizeof (*source_buffer);
  size_t bytes_read = fread (source_buffer, 1, SOURCE_BUFFER_SIZE, stdin);

  if (bytes_read == 0)
  {
    printf ("Error: reading from stdin");
    return 1;
  }

  if (!feof (stdin))
  {
    printf ("Error: missing stdin eof");
    return 1;
  }

  jjs_init_default ();

  jjs_value_t global = jjs_current_realm ();

  object_set_function (global, "print", js_print);
  object_set_value (global, "$262", create_262 (global), JJS_MOVE);

  jjs_value_free (global);

  bool status;

  if (as_module)
  {
    /* need to set up filename, dirname (uses cwd) and ensure cache is on. some tests import themselves. */
    jjs_esm_source_t options = jjs_esm_source ();

    options.source_buffer_p = source_buffer;
    options.source_buffer_size = (jjs_size_t) bytes_read;
    options.filename = jjs_string_sz (test_filename);
    options.cache = true;

    status = resolve_result_value (jjs_esm_evaluate_source (&options, JJS_MOVE), JJS_MOVE);
  }
  else
  {
    /* parse in sloppy mode. harness will add use strict, as necessary. add user value for import() to work. */
    jjs_value_t user_value = jjs_platform_realpath (jjs_string_sz (test_filename), JJS_MOVE);
    jjs_parse_options_t options = {
      .options = JJS_PARSE_HAS_USER_VALUE,
      .user_value = user_value,
    };
    jjs_value_t parsed = jjs_parse (source_buffer, (jjs_size_t) bytes_read, &options);

    if (resolve_result_value (parsed, JJS_KEEP))
    {
      status = resolve_result_value (jjs_run (parsed), JJS_MOVE);
    }
    else
    {
      status = false;
    }

    jjs_value_free (user_value);
    jjs_value_free (parsed);
  }

  if (status)
  {
    status = resolve_result_value (jjs_run_jobs (), JJS_MOVE);
  }

  jjs_cleanup ();

  return status ? 0 : 1;
}
