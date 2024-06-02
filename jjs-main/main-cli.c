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

#include <jjs.h>

#if defined (JJS_PACK) && JJS_PACK
#include <jjs-pack.h>
#endif /* defined (JJS_PACK) && JJS_PACK */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#ifndef chdir
#define chdir _chdir
#endif
#else
#include <unistd.h>
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define READ_COMMON_OPTION_ERROR (-1)
#define READ_COMMON_OPTION_NO_ARGS (0)
#define JJS_TEST_PACKAGE_NAME "jjs:test"
#define JJS_TEST_PACKAGE_FUNCTION "test"
#define JJS_TEST_META_PROP_DESCRIPTION "description"
#define JJS_TEST_META_PROP_TEST "test"
#define JJS_TEST_META_PROP_OPTIONS "options"
#define JJS_TEST_META_PROP_OPTIONS_SKIP "skip"

#define main_cli_log(CTX, STREAM, FORMAT, ...)                                                        \
  do                                                                                                  \
  {                                                                                                   \
    jjs_value_t args__[] = { __VA_ARGS__ };                                                           \
    jjs_fmt_v ((CTX), (STREAM), FORMAT, args__, (jjs_size_t) (sizeof (args__) / sizeof (args__[0]))); \
  } while (false)

typedef enum
{
  CLI_JS_LOADER_UNKNOWN,
  CLI_JS_LOADER_MODULE,
  CLI_JS_LOADER_COMMONJS,
  CLI_JS_LOADER_STRICT,
  CLI_JS_LOADER_SLOPPY,
  CLI_JS_LOADER_SNAPSHOT,
} cli_js_loader_t;

typedef struct
{
  const char *filename;
  cli_js_loader_t loader;
} cli_executable_t;

typedef struct
{
  cli_executable_t *executables;
  size_t size;
  size_t capacity;
} cli_execution_plan_t;

typedef struct
{
  uint32_t context_flags;
  const char *pmap_filename;
  const char *cwd_filename;
  int32_t log_level;
  bool has_log_level;
} cli_common_options_t;

static void
stdout_wstream_write (jjs_context_t* context_p, const jjs_wstream_t *stream, const uint8_t *data, jjs_size_t size)
{
  (void) context_p;
  (void) stream;
  fwrite (data, 1, size, stdout);
}

static jjs_wstream_t stdout_wstream = {
  .write = stdout_wstream_write,
  .encoding = JJS_ENCODING_UTF8,
};

static void
stderr_wstream_write (jjs_context_t* context_p, const jjs_wstream_t *stream, const uint8_t *data, jjs_size_t size)
{
  (void) context_p;
  (void) stream;
  fwrite (data, 1, size, stderr);
}

static jjs_wstream_t stderr_wstream = {
  .write = stderr_wstream_write,
  .encoding = JJS_ENCODING_UTF8,
};

static void
main_cli_assert (bool condition, const char* message)
{
  if (!condition)
  {
    fprintf (stderr, "assertion failed: %s", message ? message : "");
    fputs ("\n", stderr);
    fflush (stderr);

    abort ();
  }
}

static void
print_help_common_flags (void)
{
  printf ("      --cwd DIR                  Set the process' cwd\n");
  printf ("      --pmap FILE                Set the pmap json file for loading esm and commonjs packages\n");
  printf ("      --log-level LEVEL          Set the JJS log level. Value: [0,3] Default: 0\n");
  printf ("      --mem-stats                Dump vm heap mem stats at exit\n");
  printf ("      --show-opcodes             Dump parser byte code\n");
  printf ("      --show-regexp-opcodes      Dump regular expression byte code\n");
  printf ("  -h, --help                     Print this help message\n");
}

static void
print_help (void)
{
  printf ("JJS commandline tool\n\n");
  printf ("Usage: jjs <command> [...flags] [...args]\n\n");
  printf ("Commands:\n");
  printf ("  run                            Execute a file with JJS\n");
  printf ("  test                           Run a single unit test js file\n");
  printf ("  repl                           Start a simple REPL session\n");
  printf ("\n");
  printf ("Flags:\n");
  print_help_common_flags ();
  printf ("  -v, --version                  Print the JJS version\n");
}

static void
print_command_help (const char *name)
{
  const char *meta;

  if (strcmp (name, "run") == 0 || strcmp (name, "test") == 0)
  {
    meta = "FILE";
  }
  else
  {
    meta = "";
  }

  printf ("Usage: jjs %s [...args] %s\n\n", name, meta);
  printf ("Args:\n");

  if (strcmp (name, "run") == 0)
  {
    printf ("      --loader TYPE            Set the loader for the main FILE. Values: [module, commonjs, strict, sloppy, snapshot] Default: module");
    printf ("      --require FILE           Preload a commonjs file\n");
    printf ("      --import FILE            Preload an ES module file\n");
    printf ("      --preload FILE           Preload a js file in strict mode\n");
    printf ("      --preload-strict FILE    Preload a js file in strict mode\n");
    printf ("      --preload-sloppy FILE    Preload a js file in sloppy mode\n");
    printf ("      --preload-snapshot FILE  Preload a snapshot file\n");
  }
  else if (strcmp (name, "repl") == 0)
  {

  }
  else if (strcmp (name, "test") == 0)
  {
    printf ("      --loader TYPE            Set the loader for the test FILE. Values: [module, commonjs, strict, sloppy, snapshot] Default: module");
  }
  else
  {
    main_cli_assert (false, "unsupported command name");
  }

  print_help_common_flags ();
}

static void
cli_execution_plan_init (cli_execution_plan_t *plan)
{
  plan->size = 0;
  plan->capacity = 8;
  plan->executables = malloc (plan->capacity * sizeof (plan->executables[0]));
  main_cli_assert (plan->executables, "cli_execution_plan_init: bad alloc");
}

static void
cli_execution_plan_drop (cli_execution_plan_t *plan)
{
  free (plan->executables);
}

static void
cli_execution_plan_push (cli_execution_plan_t *plan, const char *filename, cli_js_loader_t loader)
{
  if (plan->size == plan->capacity)
  {
    plan->capacity += 8;
    plan->executables = realloc (plan->executables, plan->capacity * sizeof (plan->executables[0]));
    main_cli_assert (plan->executables, "cli_execution_plan_push: bad realloc");
  }

  plan->executables[plan->size++] = (cli_executable_t){
    .filename = filename,
    .loader = loader,
  };
}

static bool
cli_execution_plan_run (cli_execution_plan_t *plan, jjs_context_t* context_p)
{
  for (size_t i = 0; i < plan->size; i++)
  {
    jjs_value_t resolved_filename =
      jjs_platform_realpath (context_p, jjs_string_utf8_sz (context_p, plan->executables[i].filename), JJS_MOVE);

    if (jjs_value_is_exception (context_p, resolved_filename))
    {
      main_cli_log (context_p, &stdout_wstream, "realpath: {}\n", resolved_filename);
      jjs_value_free (context_p, resolved_filename);
      return false;
    }

    jjs_value_t result;

    switch (plan->executables[i].loader)
    {
      case CLI_JS_LOADER_MODULE:
      {
        result = jjs_esm_evaluate (context_p, resolved_filename, JJS_MOVE);
        break;
      }
      case CLI_JS_LOADER_COMMONJS:
      {
        result = jjs_commonjs_require (context_p, resolved_filename, JJS_MOVE);
        break;
      }
      case CLI_JS_LOADER_SNAPSHOT:
      {
        jjs_value_t snapshot = jjs_platform_read_file (context_p, resolved_filename, JJS_KEEP, NULL);

        if (!jjs_value_is_exception (context_p, snapshot))
        {
          const uint32_t *buffer = (const uint32_t *) jjs_arraybuffer_data (context_p, snapshot);
          jjs_size_t buffer_size = jjs_arraybuffer_size (context_p, snapshot);
          jjs_exec_snapshot_option_values_t options = {
            .source_name = resolved_filename,
            .user_value = resolved_filename,
          };

          result = jjs_exec_snapshot (context_p,
                                      buffer,
                                      buffer_size,
                                      0,
                                      JJS_SNAPSHOT_EXEC_HAS_USER_VALUE | JJS_SNAPSHOT_EXEC_HAS_SOURCE_NAME
                                        | JJS_SNAPSHOT_EXEC_COPY_DATA,
                                      &options);
          jjs_value_free (context_p, snapshot);
        }
        else
        {
          result = snapshot;
        }

        jjs_value_free (context_p, resolved_filename);

        break;
      }
      case CLI_JS_LOADER_SLOPPY:
      case CLI_JS_LOADER_STRICT:
      {
        jjs_platform_read_file_options_t read_options = {
          .encoding = JJS_ENCODING_UTF8,
        };

        jjs_value_t source = jjs_platform_read_file (context_p, resolved_filename, JJS_KEEP, &read_options);

        if (jjs_value_is_exception (context_p, source))
        {
          result = source;
        }
        else
        {
          jjs_parse_options_t options = { .source_name = resolved_filename,
                                          .user_value = resolved_filename,
                                          .options = JJS_PARSE_HAS_SOURCE_NAME | JJS_PARSE_HAS_USER_VALUE };

          if (plan->executables[i].loader == CLI_JS_LOADER_STRICT)
          {
            options.options |= JJS_PARSE_STRICT_MODE;
          }

          result = jjs_parse_value (context_p, source, &options);
          jjs_value_free (context_p, source);

          if (!jjs_value_is_exception (context_p, result))
          {
            jjs_value_t run_result = jjs_run (context_p, result);

            jjs_value_free (context_p, result);
            result = run_result;
          }

          jjs_value_free (context_p, resolved_filename);
        }

        break;
      }
      default:
      {
        jjs_value_free (context_p, resolved_filename);
        result = jjs_throw_sz (context_p, JJS_ERROR_COMMON, "Unsupported loader type");
        break;
      }
    }

    if (jjs_value_is_exception (context_p, result))
    {
      main_cli_log (context_p, &stdout_wstream, "Compile/Run: {}\n", result);
      jjs_value_free (context_p, result);
      return false;
    }

    jjs_value_free (context_p, result);

    result = jjs_run_jobs (context_p);

    if (jjs_value_is_exception (context_p, result))
    {
      main_cli_log (context_p, &stdout_wstream, "Unhandled Error: {}\n", result);
      jjs_value_free (context_p, result);
      return false;
    }

    jjs_value_free (context_p, result);
  }

  return true;
}

static cli_js_loader_t
cli_js_loader_from_string (const char *value)
{
  if (strcmp (value, "module") == 0)
  {
    return CLI_JS_LOADER_MODULE;
  }
  else if (strcmp (value, "commonjs") == 0)
  {
    return CLI_JS_LOADER_COMMONJS;
  }
  else if (strcmp (value, "strict") == 0)
  {
    return CLI_JS_LOADER_STRICT;
  }
  else if (strcmp (value, "sloppy") == 0)
  {
    return CLI_JS_LOADER_SLOPPY;
  }
  else if (strcmp (value, "snapshot") == 0)
  {
    return CLI_JS_LOADER_SNAPSHOT;
  }
  else
  {
    return CLI_JS_LOADER_UNKNOWN;
  }
}

static bool
set_cwd (const char *cwd)
{
  return cwd ? chdir (cwd) == 0 : true;
}

static jjs_char_t *
stdin_readline (jjs_size_t *out_size_p)
{
  static const jjs_size_t READ_SIZE = 1024;

  jjs_size_t capacity = READ_SIZE;
  char *line_p = malloc (capacity);
  size_t bytes_read = 0;

  if (!line_p)
  {
    return NULL;
  }

  while (true)
  {
    if (bytes_read == capacity)
    {
      capacity += READ_SIZE;
      line_p = realloc (line_p, capacity);
    }

    int character = fgetc (stdin);

    if ((character == '\n') || (character == EOF))
    {
      line_p[bytes_read] = 0;
      *out_size_p = (jjs_size_t) bytes_read;
      return (jjs_char_t *) line_p;
    }

    line_p[bytes_read++] = (char) character;
  }
}

static jjs_value_t
js_print (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;

  jjs_value_t value = jjs_fmt_join_v (context_p, jjs_string_sz (context_p, " "), JJS_MOVE, args_p, args_cnt);

  if (jjs_value_is_exception (context_p, value))
  {
    return value;
  }

  jjs_platform_stdout_write (context_p, value, JJS_MOVE);
  jjs_platform_stdout_write (context_p, jjs_string_sz (context_p, "\n"), JJS_MOVE);
  jjs_platform_stdout_flush (context_p);

  return jjs_undefined (context_p);
}

static jjs_value_t
js_create_realm (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  (void) args_p;
  (void) args_cnt;
  return jjs_realm (call_info_p->context_p);
}

static jjs_value_t
js_queue_async_assert (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;
  jjs_value_t callback = args_cnt > 0 ? args_p[0] : jjs_undefined (context_p);

  if (!jjs_value_is_function (context_p, callback))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "queueAsyncAssert expected a function");
  }

  jjs_value_t key = jjs_string_sz (context_p, "queue");
  jjs_value_t queue = jjs_object_get_internal (context_p, call_info_p->function, key);

  if (jjs_value_is_undefined (context_p, queue) || jjs_value_is_exception (context_p, queue))
  {
    jjs_value_free (context_p, queue);
    queue = jjs_array (context_p, 0);
    main_cli_assert (jjs_object_set_internal (context_p, call_info_p->function, key, queue),
                     "error setting internal async assert queue");
  }

  main_cli_assert (jjs_value_is_array (context_p, queue), "async assert queue must be an array");

  jjs_object_set_index (context_p, queue, jjs_array_length (context_p, queue), callback);

  jjs_value_free (context_p, queue);
  jjs_value_free (context_p, key);

  return jjs_undefined (context_p);
}

static jjs_value_t
js_assert (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;

  if (args_cnt > 0 && jjs_value_is_true (context_p, args_p[0]))
  {
    return jjs_undefined (context_p);
  }

  jjs_value_t message;

  if (args_cnt > 1 && jjs_value_is_string (context_p, args_p[1]))
  {
    message = jjs_fmt_to_string_v (context_p, "assertion failed: {}\n", &args_p[1], 1);
  }
  else
  {
    message = jjs_fmt_to_string_v (context_p, "assertion failed\n", NULL, 0);
  }

  return jjs_throw (context_p, JJS_ERROR_COMMON, message);
}

static jjs_value_t
get_internal_tests (jjs_context_t *context_p, jjs_value_t obj)
{
  jjs_value_t tests_prop = jjs_string_sz (context_p, "tests");
  jjs_value_t tests;

  if (jjs_object_has_internal (context_p, obj, tests_prop))
  {
    tests = jjs_object_get_internal (context_p, obj, tests_prop);
  }
  else
  {
    tests = jjs_array (context_p, 0);

    if (!jjs_object_set_internal (context_p, obj, tests_prop, tests))
    {
      jjs_value_free (context_p, tests);
      tests = jjs_undefined (context_p);
    }
  }

  jjs_value_free (context_p, tests_prop);

  if (!jjs_value_is_array (context_p, tests))
  {
    jjs_value_free (context_p, tests);
    tests = jjs_throw_sz (context_p, JJS_ERROR_COMMON, "Failed to store internal tests array in test function");
  }

  return tests;
}

static jjs_value_t
js_test (const jjs_call_info_t *call_info_p, const jjs_value_t args_p[], const jjs_length_t args_cnt)
{
  jjs_context_t *context_p = call_info_p->context_p;
  jjs_value_t arg0 = args_cnt > 0 ? args_p[0] : jjs_undefined (context_p);
  jjs_value_t arg1 = args_cnt > 1 ? args_p[1] : jjs_undefined (context_p);
  jjs_value_t arg2 = args_cnt > 2 ? args_p[2] : jjs_undefined (context_p);
  jjs_value_t options;
  jjs_value_t test_function;

  if (jjs_value_is_function (context_p, arg1))
  {
    test_function = arg1;
    options = jjs_undefined (context_p);
  }
  else
  {
    test_function = arg2;
    options = arg1;
  }

  if (!jjs_value_is_string (context_p, arg0))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "test(): expected a string description");
  }

  if (!jjs_value_is_function (context_p, test_function))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "test(): expected a function");
  }

  if (!jjs_value_is_undefined (context_p, options) && !jjs_value_is_object (context_p, options))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "test(): expected undefined or object for options");
  }

  jjs_value_t tests = get_internal_tests (context_p, call_info_p->function);

  if (jjs_value_is_exception (context_p, tests))
  {
    return tests;
  }

  jjs_value_t test_meta = jjs_object (context_p);

  jjs_value_free (context_p, jjs_object_set_sz (context_p, test_meta, JJS_TEST_META_PROP_DESCRIPTION, arg0));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, test_meta, JJS_TEST_META_PROP_TEST, test_function));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, test_meta, JJS_TEST_META_PROP_OPTIONS, options));

  jjs_value_free (context_p, jjs_object_set_index (context_p, tests, jjs_array_length (context_p, tests), test_meta));

  jjs_value_free (context_p, tests);
  jjs_value_free (context_p, test_meta);

  return jjs_undefined (context_p);
}

static void
unhandled_rejection_cb (jjs_context_t* context_p, jjs_value_t promise, jjs_value_t reason, void *user_p)
{
  (void) promise;
  (void) user_p;
  main_cli_log (context_p, &stdout_wstream, "Unhandled promise rejection: {}\n", reason);
}

static void
cleanup_engine (jjs_context_t *context_p)
{
#if defined (JJS_PACK) && JJS_PACK
  jjs_pack_cleanup (context_p);
#endif /* defined (JJS_PACK) && JJS_PACK */
  jjs_context_free (context_p);
}

static jjs_context_t*
init_engine (const cli_common_options_t *options)
{
  jjs_context_t *context_p = NULL;
  jjs_context_options_t context_options = {
    .context_flags = options->context_flags,
    .unhandled_rejection_cb = unhandled_rejection_cb,
  };

  if (jjs_context_new (&context_options, &context_p) != JJS_STATUS_OK)
  {
    return NULL;
  }

#if defined (JJS_PACK) && JJS_PACK
  jjs_pack_init (context_p, JJS_PACK_INIT_ALL);
#endif /* defined (JJS_PACK) && JJS_PACK */

  if (options->has_log_level)
  {
    jjs_log_set_level (context_p, (jjs_log_level_t) options->log_level);
  }

  if (options->pmap_filename)
  {
    jjs_value_t result = jjs_pmap (context_p, jjs_string_sz (context_p, options->pmap_filename), JJS_MOVE, jjs_undefined (context_p), JJS_MOVE);

    if (jjs_value_is_exception (context_p, result))
    {
      main_cli_log (context_p, &stdout_wstream, "Failed to load pmap: {}\n", result);
      jjs_value_free (context_p, result);
      cleanup_engine (context_p);
      return NULL;
    }

    jjs_value_free (context_p, result);
  }

  return context_p;
}

static void
init_test_realm (jjs_context_t* context_p)
{
  jjs_value_t queue_async_assert_fn = jjs_function_external (context_p, js_queue_async_assert);
  jjs_value_t assert_fn = jjs_function_external (context_p, js_assert);
  jjs_value_t print_fn = jjs_function_external (context_p, js_print);
  jjs_value_t create_realm_fn = jjs_function_external (context_p, js_create_realm);
  jjs_value_t queue_async_assert_key = jjs_string_sz (context_p, "queueAsyncAssert");
  jjs_value_t realm = jjs_current_realm (context_p);

  jjs_value_free (context_p, jjs_object_set_sz (context_p, realm, "queueAsyncAssert", queue_async_assert_fn));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, realm, "print", print_fn));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, realm, "assert", assert_fn));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, realm, "createRealm", create_realm_fn));
  /* store the async function in internal so the queue can be retrieved later */
  main_cli_assert (jjs_object_set_internal (context_p, realm, queue_async_assert_key, queue_async_assert_fn),
                   "cannot store queueAsyncAssert in internal global");

  jjs_value_free (context_p, realm);
  jjs_value_free (context_p, print_fn);
  jjs_value_free (context_p, assert_fn);
  jjs_value_free (context_p, create_realm_fn);
  jjs_value_free (context_p, queue_async_assert_fn);
  jjs_value_free (context_p, queue_async_assert_key);

  jjs_value_t test_function = jjs_function_external (context_p, js_test);
  jjs_value_t exports = jjs_object (context_p);

  jjs_value_free (context_p, jjs_object_set_sz (context_p, exports, JJS_TEST_PACKAGE_FUNCTION, test_function));
  jjs_value_free (context_p, test_function);

  jjs_value_t pkg = jjs_object (context_p);
  jjs_value_t format = jjs_string_sz (context_p, "object");

  jjs_value_free (context_p, jjs_object_set_sz (context_p, pkg, "exports", exports));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, pkg, "format", format));

  jjs_value_free (context_p, format);
  jjs_value_free (context_p, exports);

  jjs_value_t vmod_result = jjs_vmod_sz (context_p, JJS_TEST_PACKAGE_NAME, pkg, JJS_MOVE);

  if (jjs_value_is_exception (context_p, vmod_result))
  {
    main_cli_log(context_p, &stderr_wstream, "unhandled exception while loading jjs:test: {}\n", vmod_result);
  }

  main_cli_assert (!jjs_value_is_exception (context_p, vmod_result), "");
}

static bool
should_run_test (jjs_context_t *context_p, jjs_value_t test_obj)
{
  if (!jjs_object_has_sz (context_p, test_obj, JJS_TEST_META_PROP_OPTIONS))
  {
    return true;
  }

  jjs_value_t options = jjs_object_get_sz (context_p, test_obj, JJS_TEST_META_PROP_OPTIONS);
  bool result;

  if (jjs_object_has_sz (context_p, options, JJS_TEST_META_PROP_OPTIONS_SKIP))
  {
    jjs_value_t skip = jjs_object_get_sz (context_p, options, JJS_TEST_META_PROP_OPTIONS_SKIP);

    result = !jjs_value_to_boolean (context_p, skip);
    jjs_value_free (context_p, skip);
  }
  else
  {
    result = true;
  }

  jjs_value_free (context_p, options);

  return result;
}

static bool
run_all_tests (jjs_context_t* context_p)
{
  /* note: this runner assumes run-tests.py will do the reporting. */
  jjs_value_t pkg = jjs_vmod_resolve_sz (context_p, JJS_TEST_PACKAGE_NAME);
  jjs_value_t test_function = jjs_object_get_sz (context_p, pkg, JJS_TEST_PACKAGE_FUNCTION);
  jjs_value_t tests = get_internal_tests (context_p, test_function);
  jjs_size_t len = jjs_array_length (context_p, tests);
  jjs_value_t self = jjs_current_realm (context_p);
  bool result = true;

  for (jjs_size_t i = 0; i < len; i++)
  {
    jjs_value_t test_meta = jjs_object_get_index (context_p, tests, i);

    if (should_run_test (context_p, test_meta))
    {
      jjs_value_t test = jjs_object_get_sz (context_p, test_meta, JJS_TEST_META_PROP_TEST);
      jjs_value_t test_result = jjs_call (context_p, test, self, NULL, 0);
      jjs_value_t description = jjs_object_get_sz (context_p, test_meta, JJS_TEST_META_PROP_DESCRIPTION);

      if (jjs_value_is_exception (context_p, test_result))
      {
        main_cli_log (context_p, &stderr_wstream, "unhandled exception in test: {}\n{}\n", description, test_result);
        result = false;
      }
      else if (jjs_value_is_promise (context_p, test_result))
      {
        jjs_value_t jobs_result = jjs_run_jobs (context_p);

        if (jjs_value_is_exception (context_p, jobs_result))
        {
          main_cli_log (context_p, &stderr_wstream, "unhandled exception running async jobs after test: {}\n{}\n", description, jobs_result);
          result = false;
        }
        else if (jjs_promise_state (context_p, test_result) != JJS_PROMISE_STATE_FULFILLED)
        {
          main_cli_log (context_p, &stderr_wstream, "unfulfilled promise after test: {}\n{}\n", description, test_result);
          result = false;
        }

        jjs_value_free (context_p, jobs_result);
      }

      jjs_value_free (context_p, description);
      jjs_value_free (context_p, test);
      jjs_value_free (context_p, test_result);
    }

    jjs_value_free (context_p, test_meta);
  }

  jjs_value_free (context_p, self);
  jjs_value_free (context_p, pkg);
  jjs_value_free (context_p, test_function);
  jjs_value_free (context_p, tests);

  return result;
}

static bool
process_async_asserts (jjs_context_t* context_p)
{
  jjs_value_t realm = jjs_current_realm (context_p);
  jjs_value_t internal_key = jjs_string_sz (context_p, "queueAsyncAssert");
  jjs_value_t queue_async_assert = jjs_object_get_internal (context_p, realm, internal_key);
  jjs_value_t key = jjs_string_sz (context_p, "queue");
  jjs_value_t queue = jjs_object_get_internal (context_p, queue_async_assert, key);
  jjs_value_t self = jjs_current_realm (context_p);
  jjs_size_t len = jjs_array_length (context_p, queue);
  bool has_error = false;

  for (jjs_size_t i = 0; i < len; i++)
  {
    jjs_value_t fn = jjs_object_get_index (context_p, queue, i);
    jjs_value_t async_assert_result;

    if (jjs_value_is_function (context_p, fn))
    {
      async_assert_result = jjs_call (context_p, fn, self, NULL, 0);
    }
    else
    {
      async_assert_result = jjs_throw_sz (context_p, JJS_ERROR_COMMON, "Unknown object in async assert queue!");
    }

    jjs_value_free (context_p, fn);

    if (jjs_value_is_exception (context_p, async_assert_result))
    {
      main_cli_log (context_p, &stdout_wstream, "{}\n", async_assert_result);
      has_error = true;
      break;
    }

    jjs_value_free (context_p, async_assert_result);
  }

  jjs_value_free (context_p, key);
  jjs_value_free (context_p, queue);
  jjs_value_free (context_p, realm);
  jjs_value_free (context_p, internal_key);
  jjs_value_free (context_p, queue_async_assert);
  jjs_value_free (context_p, self);

  return !has_error;
}

static bool
read_int32 (const char *str, int32_t *out)
{
  char *endptr;
  long value = strtol (str, &endptr, 10);

  if (*endptr != '\0' || value > INT32_MAX)
  {
    return false;
  }

  *out = (int32_t) value;

  return true;
}

static int
read_common_option (cli_common_options_t *options, int index, int argc, char **argv)
{
  int offset = 0;
  const char *current = index + offset < argc ? argv[index + offset] : "";
  const char *next = (index + offset + 1) < argc ? argv[index + offset + 1] : NULL;

  if (strcmp ("--mem-stats", current) == 0)
  {
    options->context_flags |= JJS_CONTEXT_FLAG_MEM_STATS;
  }
  else if (strcmp ("--show-opcodes", current) == 0)
  {
    options->context_flags |= JJS_CONTEXT_FLAG_SHOW_OPCODES;
  }
  else if (strcmp ("--show-regexp-opcodes", current) == 0)
  {
    options->context_flags |= JJS_CONTEXT_FLAG_SHOW_REGEXP_OPCODES;
  }
  else if (strcmp ("--pmap", current) == 0)
  {
    if (next)
    {
      options->pmap_filename = next;
      offset++;
    }
    else
    {
      printf ("--pmap arg missing FILE\n");
      return READ_COMMON_OPTION_ERROR;
    }
  }
  else if (strcmp ("--cwd", current) == 0)
  {
    if (next)
    {
      options->cwd_filename = next;
      offset++;
    }
    else
    {
      printf ("--cwd arg missing FILE\n");
      return READ_COMMON_OPTION_ERROR;
    }
  }
  else if (strcmp ("--log-level", current) == 0)
  {
    if (next)
    {
      int32_t value;

      if (read_int32 (next, &value) && value >= 0 && value <= 3)
      {
        options->log_level = value;
        options->has_log_level = true;
        offset++;
      }
      else
      {
        printf ("--log-level expects a number [0,3] got %s\n", next);
        return READ_COMMON_OPTION_ERROR;
      }
    }
    else
    {
      printf ("--log-level arg missing FILE\n");
      return READ_COMMON_OPTION_ERROR;
    }
  }
  else
  {
    return READ_COMMON_OPTION_NO_ARGS;
  }

  offset++;

  return offset;
}

static int
repl (int argc, char **argv)
{
  int i = 0;
  cli_common_options_t options = { 0 };

  while (i < argc)
  {
    if (strcmp (argv[i], "-h") == 0 || strcmp (argv[i], "--help") == 0)
    {
      print_command_help ("repl");
      return EXIT_SUCCESS;
    }
    else
    {
      int offset = read_common_option (&options, i, argc, argv);

      if (offset == READ_COMMON_OPTION_ERROR)
      {
        return EXIT_FAILURE;
      }
      else if (offset == READ_COMMON_OPTION_NO_ARGS)
      {
        break;
      }
      else if (offset > 1)
      {
        i += (offset - 1);
      }
    }

    i++;
  }

  if (!set_cwd (options.cwd_filename))
  {
    return EXIT_FAILURE;
  }

  jjs_context_t *context_p = init_engine (&options);

  if (context_p == NULL)
  {
    return EXIT_FAILURE;
  }

  jjs_value_t result;
  jjs_value_t prompt = jjs_string_sz (context_p, "jjs> ");
  jjs_value_t new_line = jjs_string_sz (context_p, "\n");
  jjs_value_t source_name = jjs_string_sz (context_p, "<repl>");

  while (true)
  {
    jjs_platform_stdout_write (context_p, prompt, JJS_KEEP);

    jjs_size_t length;
    jjs_char_t *line_p = stdin_readline (&length);

    if (line_p == NULL)
    {
      jjs_platform_stdout_write (context_p, new_line, JJS_KEEP);
      goto done;
    }

    if (length == 0)
    {
      continue;
    }

    if (memcmp ((const char *) line_p, ".exit", strlen (".exit")) == 0)
    {
      free (line_p);
      break;
    }

    if (!jjs_validate_string (context_p, line_p, length, JJS_ENCODING_UTF8))
    {
      free (line_p);
      result = jjs_throw_sz (context_p, JJS_ERROR_SYNTAX, "Input is not a valid UTF-8 string");
      goto exception;
    }

    jjs_parse_options_t opts = {
      .options = JJS_PARSE_HAS_SOURCE_NAME,
      .source_name = source_name,
    };

    result = jjs_parse (context_p, line_p, length, &opts);

    free (line_p);

    if (jjs_value_is_exception (context_p, result))
    {
      goto exception;
    }

    jjs_value_t script = result;
    result = jjs_run (context_p, script);
    jjs_value_free (context_p, script);

    if (jjs_value_is_exception (context_p, result))
    {
      goto exception;
    }

    main_cli_log (context_p, &stdout_wstream, "{}\n", result);

    jjs_value_free (context_p, result);
    result = jjs_run_jobs (context_p);

    if (jjs_value_is_exception (context_p, result))
    {
      goto exception;
    }

    jjs_value_free (context_p, result);
    continue;

exception:
    main_cli_log (context_p, &stdout_wstream, "{}\n", result);
    jjs_value_free (context_p, result);
  }

done:
  jjs_value_free (context_p, prompt);
  jjs_value_free (context_p, new_line);
  jjs_value_free (context_p, source_name);
  cleanup_engine (context_p);
  return 0;
}

static int
run (int argc, char **argv)
{
  cli_js_loader_t main_loader = CLI_JS_LOADER_MODULE;
  int i = 0;
  int exit_code = EXIT_SUCCESS;
  cli_execution_plan_t plan;
  cli_common_options_t options = { 0 };

  cli_execution_plan_init (&plan);

  while (i < argc)
  {
    const char *next = i + 1 < argc ? argv[i + 1] : NULL;

    if (strcmp (argv[i], "--loader") == 0)
    {
      if (i + 1 < argc)
      {
        main_loader = cli_js_loader_from_string (argv[++i]);
      }
      else
      {
        exit_code = EXIT_FAILURE;
        goto done;
      }
    }
    else if (strcmp (argv[i], "--require") == 0)
    {
      if (next)
      {
        cli_execution_plan_push (&plan, next, CLI_JS_LOADER_COMMONJS);
        i++;
      }
      else
      {
        printf ("--require arg missing FILE\n");
        exit_code = EXIT_FAILURE;
        goto done;
      }
    }
    else if (strcmp (argv[i], "--import") == 0)
    {
      if (next)
      {
        cli_execution_plan_push (&plan, next, CLI_JS_LOADER_MODULE);
        i++;
      }
      else
      {
        printf ("--import arg missing FILE\n");
        exit_code = EXIT_FAILURE;
        goto done;
      }
    }
    else if (strcmp (argv[i], "--preload") == 0 || strcmp (argv[i], "--preload-strict") == 0)
    {
      if (next)
      {
        cli_execution_plan_push (&plan, next, CLI_JS_LOADER_STRICT);
        i++;
      }
      else
      {
        printf ("--preload arg missing FILE\n");
        exit_code = EXIT_FAILURE;
        goto done;
      }
    }
    else if (strcmp (argv[i], "--preload-sloppy") == 0)
    {
      if (next)
      {
        cli_execution_plan_push (&plan, next, CLI_JS_LOADER_SLOPPY);
        i++;
      }
      else
      {
        printf ("--preload-sloppy arg missing FILE\n");
        exit_code = EXIT_FAILURE;
        goto done;
      }
    }
    else if (strcmp (argv[i], "--preload-snapshot") == 0)
    {
      if (next)
      {
        cli_execution_plan_push (&plan, next, CLI_JS_LOADER_SNAPSHOT);
        i++;
      }
      else
      {
        printf ("--preload-snapshot arg missing FILE\n");
        exit_code = EXIT_FAILURE;
        goto done;
      }
    }
    else if (strcmp (argv[i], "-h") == 0 || strcmp (argv[i], "--help") == 0)
    {
      print_command_help ("run");
      goto done;
    }
    else
    {
      int offset = read_common_option (&options, i, argc, argv);

      if (offset == READ_COMMON_OPTION_ERROR)
      {
        exit_code = EXIT_FAILURE;
        goto done;
      }
      else if (offset == READ_COMMON_OPTION_NO_ARGS)
      {
        if (i == argc - 1)
        {
          cli_execution_plan_push (&plan, argv[i], main_loader);
          break;
        }
        else
        {
          printf ("FILE must be the last arg of run.\n");
          exit_code = EXIT_FAILURE;
          goto done;
        }
      }
      else if (offset > 1)
      {
        i += (offset - 1);
      }
    }

    i++;
  }

  if (plan.size == 0)
  {
    exit_code = EXIT_FAILURE;
    goto done;
  }

  if (!set_cwd (options.cwd_filename))
  {
    exit_code = EXIT_FAILURE;
    goto done;
  }

  jjs_context_t *context_p = init_engine (&options);

  if (context_p)
  {
    exit_code = cli_execution_plan_run (&plan, context_p) ? 0 : 1;
    cleanup_engine (context_p);
  }
  else
  {
    exit_code = EXIT_FAILURE;
  }

done:
  cli_execution_plan_drop (&plan);

  return exit_code;
}

static int
test (int argc, char **argv)
{
  cli_js_loader_t loader = CLI_JS_LOADER_MODULE;
  int i = 0;
  int exit_code = EXIT_SUCCESS;
  bool vm_cleanup = false;
  cli_execution_plan_t plan;
  cli_common_options_t options = { 0 };

  cli_execution_plan_init (&plan);

  while (i < argc)
  {
    if (strcmp (argv[i], "--loader") == 0)
    {
      if (i + 1 < argc)
      {
        loader = cli_js_loader_from_string (argv[++i]);
      }
      else
      {
        exit_code = EXIT_FAILURE;
        goto done;
      }
    }
    else if (strcmp (argv[i], "-h") == 0 || strcmp (argv[i], "--help") == 0)
    {
      print_command_help ("test");
      goto done;
    }
    else
    {
      int offset = read_common_option (&options, i, argc, argv);

      if (offset == READ_COMMON_OPTION_ERROR)
      {
        exit_code = EXIT_FAILURE;
        goto done;
      }
      else if (offset == READ_COMMON_OPTION_NO_ARGS)
      {
        if (i == argc - 1)
        {
          cli_execution_plan_push (&plan, argv[i], loader);
          break;
        }
        else
        {
          printf ("FILE must be the last arg of test.\n");
          exit_code = EXIT_FAILURE;
          goto done;
        }
      }
      else if (offset > 1)
      {
        i += (offset - 1);
      }
    }

    i++;
  }

  if (plan.size == 0)
  {
    exit_code = EXIT_FAILURE;
    goto done;
  }

  if (!set_cwd (options.cwd_filename))
  {
    exit_code = EXIT_FAILURE;
    goto done;
  }

  jjs_context_t *context_p = init_engine (&options);

  if (!context_p)
  {
    exit_code = EXIT_FAILURE;
    goto done;
  }

  vm_cleanup = true;
  init_test_realm (context_p);

  if (!cli_execution_plan_run (&plan, context_p) || !run_all_tests (context_p) || !process_async_asserts (context_p))
  {
    exit_code = EXIT_FAILURE;
  }

done:
  cli_execution_plan_drop (&plan);

  if (vm_cleanup)
  {
    cleanup_engine (context_p);
  }

  return exit_code;
}

int
main (int argc, char **argv)
{
  static const int ARG0_FIRST_SIZE = 2;

  if (argc < ARG0_FIRST_SIZE)
  {
    print_help ();
    return EXIT_SUCCESS;
  }

  const char *first = argv[1];

  srand ((unsigned) time (NULL));

  if (strcmp (first, "run") == 0)
  {
    int exit_code = run (argc - ARG0_FIRST_SIZE, argv + ARG0_FIRST_SIZE);

    if (exit_code != EXIT_SUCCESS)
    {
      print_command_help (first);
    }

    return exit_code;
  }
  else if (strcmp (first, "repl") == 0)
  {
    int exit_code = repl (argc - ARG0_FIRST_SIZE, argv + ARG0_FIRST_SIZE);

    if (exit_code != EXIT_SUCCESS)
    {
      print_command_help (first);
    }

    return exit_code;
  }
  else if (strcmp (first, "test") == 0)
  {
    int exit_code = test (argc - ARG0_FIRST_SIZE, argv + ARG0_FIRST_SIZE);

    if (exit_code != EXIT_SUCCESS)
    {
      print_command_help (first);
    }

    return exit_code;
  }
  else if (strcmp (first, "-v") == 0 || strcmp (first, "--version") == 0)
  {
    printf ("%i.%i.%i\n", JJS_API_MAJOR_VERSION, JJS_API_MINOR_VERSION, JJS_API_PATCH_VERSION);
    return EXIT_SUCCESS;
  }
  else if (strcmp (first, "-h") == 0 || strcmp (first, "--help") == 0)
  {
    print_help ();
    return EXIT_SUCCESS;
  }
  else
  {
    printf ("Invalid command: %s\n\n", first);
    print_help ();
    return EXIT_FAILURE;
  }
}
