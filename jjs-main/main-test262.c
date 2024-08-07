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

#define JJS_CLI_IMPLEMENTATION
#include "./lib/jjs-cli.h"

static jjs_value_t create_262 (jjs_context_t* context_p, jjs_value_t realm);

static jjs_value_t
js_print (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;

  jjs_value_t value = args_cnt > 0 ? args_p[0] : jjs_undefined (context_p);
  jjs_value_t value_as_string =
    jjs_value_is_symbol (context_p, value) ? jjs_symbol_description (context_p, value) : jjs_value_to_string (context_p, value);

  if (!jjs_value_is_string (context_p, value_as_string))
  {
    jjs_value_free (context_p, value_as_string);
    value_as_string = jjs_string_sz (context_p, "Error converting exception to string.");
  }

  jjs_platform_io_write (context_p, JJS_STDOUT, value_as_string, JJS_MOVE);
  jjs_platform_io_write (context_p, JJS_STDOUT, jjs_string_sz (context_p, "\n"), JJS_MOVE);
  jjs_platform_io_flush (context_p, JJS_STDOUT);

  return jjs_undefined (context_p);
}

static void
object_set_value (jjs_context_t* context_p, jjs_value_t object, const char *key, jjs_value_t value, jjs_own_t value_o)
{
  jjs_value_free (context_p, jjs_object_set_sz (context_p, object, key, value, value_o));
}

static void
object_set_function (jjs_context_t* context_p, jjs_value_t object, const char *key, jjs_external_handler_t fn)
{
  object_set_value (context_p, object, key, jjs_function_external (context_p, fn), JJS_MOVE);
}

static jjs_value_t
js_262_detach_array_buffer (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;

  if (args_cnt < 1 || !jjs_value_is_arraybuffer (context_p, args_p[0]))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "Expected an ArrayBuffer object");
  }

  /* TODO: support the optional 'key' argument */

  return jjs_arraybuffer_detach (context_p, args_p[0]);
}

static jjs_value_t
js_262_eval_script (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;

  if (args_cnt < 1 || !jjs_value_is_string (context_p, args_p[0]))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "Expected a string");
  }

  jjs_value_t ret_value = jjs_parse_value (context_p, args_p[0], JJS_KEEP, NULL);

  if (!jjs_value_is_exception (context_p, ret_value))
  {
    ret_value = jjs_run (context_p, ret_value, JJS_MOVE);
  }

  return ret_value;
}

static jjs_value_t
js_262_create_realm (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  (void) args_p;
  (void) args_cnt;

  jjs_context_t *context_p = call_info_p->context_p;
  jjs_value_t realm_object = jjs_realm (context_p);
  jjs_value_t previous_realm = jjs_set_realm (context_p, realm_object);
  assert (!jjs_value_is_exception (context_p, previous_realm));

  jjs_value_t test262_object = create_262 (context_p, realm_object);
  jjs_set_realm (context_p, previous_realm);
  jjs_value_free (context_p, realm_object);

  return test262_object;
}

static jjs_value_t
js_262_gc (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;

  jjs_gc_mode_t mode =
    ((args_cnt > 0 && jjs_value_to_boolean (context_p, args_p[0])) ? JJS_GC_PRESSURE_HIGH : JJS_GC_PRESSURE_LOW);

  jjs_heap_gc (context_p, mode);
  return jjs_undefined (context_p);
}

static jjs_value_t
create_262 (jjs_context_t* context_p, jjs_value_t realm)
{
  jjs_value_t value = jjs_object (context_p);

  object_set_function (context_p, value, "detachArrayBuffer", js_262_detach_array_buffer);
  object_set_function (context_p, value, "evalScript", js_262_eval_script);
  object_set_function (context_p, value, "createRealm", js_262_create_realm);
  object_set_function (context_p, value, "gc", js_262_gc);

  jjs_value_t result = jjs_object_set_sz (context_p, value, "global", realm, JJS_KEEP);
  assert (!jjs_value_is_exception (context_p, result));
  jjs_value_free (context_p, result);

  return value;
}

static bool
resolve_result_value (jjs_context_t* context_p, jjs_value_t result, jjs_own_t result_o)
{
  bool status = !jjs_value_is_exception (context_p, result);

  if (!status)
  {
    jjs_value_t value = jjs_exception_value (context_p, result, JJS_KEEP);
    jjs_value_t value_as_string =
      jjs_value_is_symbol (context_p, value) ? jjs_symbol_description (context_p, value) : jjs_value_to_string (context_p, value);

    jjs_value_free (context_p, value);

    if (jjs_value_is_string (context_p, value_as_string))
    {
      jjs_platform_io_write (context_p, JJS_STDOUT, value_as_string, JJS_MOVE);
    }
    else
    {
      jjs_platform_io_write (context_p, JJS_STDERR, jjs_string_sz (context_p, "Failed to toString() exception."), JJS_MOVE);
      jjs_value_free (context_p, value_as_string);
    }
  }

  if (result_o == JJS_MOVE)
  {
    jjs_value_free (context_p, result);
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

  uint8_t *source_buffer = NULL;
  jjs_size_t source_buffer_size;

  if (!jjs_cli_stdin_drain (&source_buffer, &source_buffer_size))
  {
    puts ("Failed to read source from stdin\n");
    return 1;
  }

  jjs_context_t *context_p = NULL;
  jjs_status_t context_status = jjs_context_new (NULL, &context_p);

  if (context_status != JJS_STATUS_OK)
  {
    free (source_buffer);
    printf ("Failed to create JJS context: %i\n", context_status);
    return 1;
  }

  jjs_value_t global = jjs_current_realm (context_p);

  object_set_function (context_p, global, "print", js_print);
  object_set_value (context_p, global, "$262", create_262 (context_p, global), JJS_MOVE);

  jjs_value_free (context_p, global);

  bool status;

  if (as_module)
  {
    /* test262 test can import their own files. set their filename and include in esm cache so imports work. */
    jjs_esm_source_options_t options = {
      .cache = true,
      .filename = jjs_optional_value (jjs_string_sz (context_p, test_filename)),
      .filename_o = JJS_MOVE,
    };
    jjs_value_t result = jjs_esm_evaluate_source (context_p, source_buffer, source_buffer_size, &options);
    
    status = resolve_result_value (context_p, result, JJS_MOVE);
  }
  else
  {
    /* parse in sloppy mode. harness will add use strict, as necessary. add user value for import() to work. */
    jjs_value_t user_value = jjs_platform_realpath_sz (context_p, test_filename);
    jjs_parse_options_t options = {
      .user_value = jjs_optional_value (user_value),
      .user_value_o = JJS_MOVE,
    };
    jjs_value_t parsed = jjs_parse (context_p, source_buffer, source_buffer_size, &options);

    if (resolve_result_value (context_p, parsed, JJS_KEEP))
    {
      status = resolve_result_value (context_p, jjs_run (context_p, parsed, JJS_KEEP), JJS_MOVE);
    }
    else
    {
      status = false;
    }

    jjs_value_free (context_p, parsed);
  }

  if (status)
  {
    status = resolve_result_value (context_p, jjs_run_jobs (context_p), JJS_MOVE);
  }

  jjs_context_free (context_p);
  free (source_buffer);

  return status ? 0 : 1;
}
