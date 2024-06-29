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

#define IMCL_IMPLEMENTATION
#include "imcl.h"

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

#define jjs_cli_log(CTX, STREAM, FORMAT, ...)                                                         \
  do                                                                                                  \
  {                                                                                                   \
    jjs_value_t args__[] = { __VA_ARGS__ };                                                           \
    jjs_fmt_v ((CTX), (STREAM), FORMAT, args__, (jjs_size_t) (sizeof (args__) / sizeof (args__[0]))); \
  } while (false)

typedef enum
{
  JJS_CLI_LOADER_UNKNOWN,
  JJS_CLI_LOADER_AUTO,
  JJS_CLI_LOADER_ESM,
  JJS_CLI_LOADER_CJS,
  JJS_CLI_LOADER_JS,
  JJS_CLI_LOADER_SNAPSHOT,
} jjs_cli_loader_t;

typedef enum
{
  JJS_CLI_PARSER_UNKNOWN,
  JJS_CLI_PARSER_SLOPPY,
  JJS_CLI_PARSER_STRICT,
  JJS_CLI_PARSER_MODULE,
  JJS_CLI_PARSER_SNAPSHOT,
} jjs_cli_parser_t;

typedef struct
{
  const char *filename;
  jjs_cli_loader_t loader;
  jjs_cli_parser_t parser;
  jjs_size_t snapshot_index;
} jjs_cli_module_t;

typedef struct
{
  jjs_cli_module_t *items;
  size_t size;
  size_t capacity;
} jjs_cli_include_list_t;

typedef struct
{
  imcl_args_t imcl;
  const char *cwd;
  jjs_context_options_t context_options;
  const char *pmap_filename;
  const char *cwd_filename;
  int32_t log_level;
  bool has_log_level;
} jjs_cli_state_t;

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
jjs_cli_assert (bool condition, const char* message)
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
    jjs_cli_assert (false, "unsupported command name");
  }

  print_help_common_flags ();
}

static void
jjs_cli_include_list_init (jjs_cli_include_list_t *includes)
{
  includes->size = 0;
  includes->capacity = 8;
  includes->items = malloc (includes->capacity * sizeof (includes->items[0]));
  jjs_cli_assert (includes->items, "jjs_cli_include_list_init: bad alloc");
}

static void
jjs_cli_include_list_drop (jjs_cli_include_list_t *includes)
{
  free (includes->items);
}

static void
jjs_cli_include_list_append (jjs_cli_include_list_t *includes,
                             const char *filename,
                             jjs_cli_loader_t loader,
                             jjs_cli_parser_t parser,
                             jjs_size_t snapshot_index)
{
  if (includes->size == includes->capacity)
  {
    includes->capacity += 8;
    includes->items = realloc (includes->items, includes->capacity * sizeof (includes->items[0]));
    jjs_cli_assert (includes->items, "jjs_cli_include_list_append: bad realloc");
  }

  includes->items[includes->size++] = (jjs_cli_module_t){
    .filename = filename,
    .loader = loader,
    .parser = parser,
    .snapshot_index = snapshot_index,
  };
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
    jjs_cli_assert (jjs_object_set_internal (context_p, call_info_p->function, key, queue, JJS_KEEP),
                    "error setting internal async assert queue");
  }

  jjs_cli_assert (jjs_value_is_array (context_p, queue), "async assert queue must be an array");

  jjs_object_set_index (context_p, queue, jjs_array_length (context_p, queue), callback, JJS_KEEP);

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

  if (args_cnt > 1 && jjs_value_is_string (context_p, args_p[1]))
  {
    return jjs_fmt_throw (context_p, JJS_ERROR_COMMON, "assertion failed: {}\n", &args_p[1], 1, JJS_KEEP);
  }

  return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "assertion failed");
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

    if (!jjs_object_set_internal (context_p, obj, tests_prop, tests, JJS_KEEP))
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

  jjs_value_free (context_p, jjs_object_set_sz (context_p, test_meta, JJS_TEST_META_PROP_DESCRIPTION, arg0, JJS_KEEP));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, test_meta, JJS_TEST_META_PROP_TEST, test_function, JJS_KEEP));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, test_meta, JJS_TEST_META_PROP_OPTIONS, options, JJS_KEEP));

  jjs_value_free (context_p, jjs_object_set_index (context_p, tests, jjs_array_length (context_p, tests), test_meta, JJS_MOVE));

  jjs_value_free (context_p, tests);

  return jjs_undefined (context_p);
}

static void
unhandled_rejection_cb (jjs_context_t* context_p, jjs_value_t promise, jjs_value_t reason, void *user_p)
{
  (void) promise;
  (void) user_p;
  jjs_cli_log (context_p, &stdout_wstream, "Unhandled promise rejection: {}\n", reason);
}

static void
cleanup_engine (jjs_context_t *context_p)
{
#if defined (JJS_PACK) && JJS_PACK
  jjs_pack_cleanup (context_p);
#endif /* defined (JJS_PACK) && JJS_PACK */
  jjs_context_free (context_p);
}

static bool
init_engine (const jjs_cli_state_t *state, jjs_context_t **out)
{
  srand ((unsigned) time (NULL));
  set_cwd (state->cwd);

  jjs_context_t *context;

  if (jjs_context_new (&state->context_options, &context) != JJS_STATUS_OK)
  {
    return false;
  }

  jjs_promise_on_unhandled_rejection (context, &unhandled_rejection_cb, NULL);

#if defined (JJS_PACK) && JJS_PACK
  jjs_pack_init (context, JJS_PACK_INIT_ALL);
#endif /* defined (JJS_PACK) && JJS_PACK */

  if (state->has_log_level)
  {
    jjs_log_set_level (context, (jjs_log_level_t) state->log_level);
  }

  if (state->pmap_filename)
  {
    jjs_value_t result = jjs_pmap (context, jjs_string_sz (context, state->pmap_filename), JJS_MOVE, jjs_undefined (context), JJS_MOVE);

    if (jjs_value_is_exception (context, result))
    {
      jjs_cli_log (context, &stdout_wstream, "Failed to load pmap: {}\n", result);
      jjs_value_free (context, result);
      cleanup_engine (context);
      return false;
    }

    jjs_value_free (context, result);
  }

  *out = context;
  return true;
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

  /* store the async function in internal so the queue can be retrieved later */
  jjs_cli_assert (jjs_object_set_internal (context_p, realm, queue_async_assert_key, queue_async_assert_fn, JJS_KEEP),
                  "cannot store queueAsyncAssert in internal global");

  jjs_value_free (context_p, jjs_object_set (context_p, realm, queue_async_assert_key, queue_async_assert_fn, JJS_MOVE));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, realm, "print", print_fn, JJS_MOVE));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, realm, "assert", assert_fn, JJS_MOVE));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, realm, "createRealm", create_realm_fn, JJS_MOVE));

  jjs_value_free (context_p, queue_async_assert_key);
  jjs_value_free (context_p, realm);

  jjs_value_t test_function = jjs_function_external (context_p, js_test);
  jjs_value_t exports = jjs_object (context_p);

  jjs_value_free (context_p, jjs_object_set_sz (context_p, exports, JJS_TEST_PACKAGE_FUNCTION, test_function, JJS_MOVE));

  jjs_value_t pkg = jjs_object (context_p);
  jjs_value_t format = jjs_string_sz (context_p, "object");

  jjs_value_free (context_p, jjs_object_set_sz (context_p, pkg, "exports", exports, JJS_MOVE));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, pkg, "format", format, JJS_MOVE));

  jjs_value_t vmod_result = jjs_vmod_sz (context_p, JJS_TEST_PACKAGE_NAME, pkg, JJS_MOVE);

  if (jjs_value_is_exception (context_p, vmod_result))
  {
    jjs_cli_log (context_p, &stderr_wstream, "unhandled exception while loading jjs:test: {}\n", vmod_result);
  }

  jjs_cli_assert (!jjs_value_is_exception (context_p, vmod_result), "");
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
      jjs_value_t test_result = jjs_call_this_noargs (context_p, test, self, JJS_KEEP);
      jjs_value_t description = jjs_object_get_sz (context_p, test_meta, JJS_TEST_META_PROP_DESCRIPTION);

      if (jjs_value_is_exception (context_p, test_result))
      {
        jjs_cli_log (context_p, &stderr_wstream, "unhandled exception in test: {}\n{}\n", description, test_result);
        result = false;
      }
      else if (jjs_value_is_promise (context_p, test_result))
      {
        jjs_value_t jobs_result = jjs_run_jobs (context_p);

        if (jjs_value_is_exception (context_p, jobs_result))
        {
          jjs_cli_log (context_p, &stderr_wstream, "unhandled exception running async jobs after test: {}\n{}\n", description, jobs_result);
          result = false;
        }
        else if (jjs_promise_state (context_p, test_result) != JJS_PROMISE_STATE_FULFILLED)
        {
          jjs_cli_log (context_p, &stderr_wstream, "unfulfilled promise after test: {}\n{}\n", description, test_result);
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
      async_assert_result = jjs_call_this_noargs (context_p, fn, self, JJS_KEEP);
    }
    else
    {
      async_assert_result = jjs_throw_sz (context_p, JJS_ERROR_COMMON, "Unknown object in async assert queue!");
    }

    jjs_value_free (context_p, fn);

    if (jjs_value_is_exception (context_p, async_assert_result))
    {
      jjs_cli_log (context_p, &stdout_wstream, "{}\n", async_assert_result);
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
shift_common_option (jjs_cli_state_t *state)
{
  if (imcl_args_shift_if_option (&state->imcl, NULL, "--cwd"))
  {
    state->cwd = imcl_args_shift (&state->imcl);
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--pmap"))
  {
    state->pmap_filename = imcl_args_shift (&state->imcl);
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--mem-stats"))
  {
    state->context_options.enable_mem_stats = true;
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--show-opcodes"))
  {
    state->context_options.show_op_codes = true;
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--show-regexp-opcodes"))
  {
    state->context_options.show_regexp_op_codes = true;
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--log-level"))
  {
    state->log_level = imcl_args_shift_ranged_int (&state->imcl, 0, 3);
    state->has_log_level = true;
  }
  else
  {
    return false;
  }

  return true;
}

static jjs_cli_loader_t
jjs_cli_loader_from_string (const char *value)
{
  if (strcmp ("esm", value) == 0)
  {
    return JJS_CLI_LOADER_ESM;
  }
  else if (strcmp ("auto", value) == 0)
  {
    return JJS_CLI_LOADER_AUTO;
  }
  else if (strcmp ("cjs", value) == 0)
  {
    return JJS_CLI_LOADER_CJS;
  }
  else if (strcmp ("js", value) == 0)
  {
    return JJS_CLI_LOADER_JS;
  }
  else if (strcmp ("snapshot", value) == 0)
  {
    return JJS_CLI_LOADER_SNAPSHOT;
  }

  return JJS_CLI_LOADER_UNKNOWN;
}

static jjs_cli_parser_t
jjs_cli_parser_from_string (const char *value)
{
  if (strcmp ("sloppy", value) == 0)
  {
    return JJS_CLI_PARSER_SLOPPY;
  }
  else if (strcmp ("strict", value) == 0)
  {
    return JJS_CLI_PARSER_STRICT;
  }
  else if (strcmp ("module", value) == 0)
  {
    return JJS_CLI_PARSER_MODULE;
  }
  else if (strcmp ("snapshot", value) == 0)
  {
    return JJS_CLI_PARSER_SNAPSHOT;
  }

  return JJS_CLI_PARSER_UNKNOWN;
}

static int
jjs_cli_run_module (jjs_context_t *context, jjs_cli_module_t *module, bool parse_only)
{
  jjs_value_t result;

  switch (module->loader)
  {
    case JJS_CLI_LOADER_CJS:
    {
      result = jjs_commonjs_require_sz (context, module->filename);
      break;
    }
    case JJS_CLI_LOADER_ESM:
    {
      result = jjs_esm_import_sz (context, module->filename);
      break;
    }
    case JJS_CLI_LOADER_SNAPSHOT:
    {
      jjs_value_t filename = jjs_string_utf8_sz (context, module->filename);
      jjs_value_t snapshot = jjs_platform_read_file (context, filename, JJS_KEEP, NULL);

      if (!jjs_value_is_exception (context, snapshot))
      {
        uint8_t *snapshot_buffer = jjs_arraybuffer_data (context, snapshot);
        jjs_size_t snapshot_buffer_size = jjs_arraybuffer_size (context, snapshot);
        jjs_exec_snapshot_option_values_t options = {
          .source_name = filename,
        };

        result = jjs_exec_snapshot (context,
                                    (const uint32_t *) snapshot_buffer,
                                    snapshot_buffer_size,
                                    module->snapshot_index,
                                    JJS_SNAPSHOT_EXEC_HAS_SOURCE_NAME,
                                    &options);
      }
      else
      {
        result = jjs_value_copy(context, snapshot);
      }

      jjs_value_free (context, snapshot);
      jjs_value_free (context, filename);

      break;
    }
    case JJS_CLI_LOADER_JS:
    {
      jjs_value_t filename = jjs_string_utf8_sz (context, module->filename);
      jjs_platform_read_file_options_t read_file_options = {
        .encoding = JJS_ENCODING_UTF8,
      };
      jjs_value_t source = jjs_platform_read_file (context, filename, JJS_KEEP, &read_file_options);

      if (!jjs_value_is_exception (context, source))
      {
        jjs_parse_options_t opts = {
          .is_strict_mode = (module->parser == JJS_CLI_PARSER_STRICT),
          .parse_module = (module->parser == JJS_CLI_PARSER_MODULE),
          .source_name = jjs_optional_value (filename),
          .source_name_o = JJS_KEEP,
        };

        jjs_value_t parse_result = jjs_parse_value (context, source, JJS_MOVE, &opts);

        if (!jjs_value_is_exception (context, parse_result))
        {
          if (parse_only)
          {
            result = parse_result;
          }
          else
          {
            result = jjs_run (context, parse_result, JJS_MOVE);
          }
        }
        else
        {
          result = parse_result;
        }
      }
      else
      {
        result = source;
      }

      jjs_value_free (context, filename);
      break;
    }
    default:
      // TODO: assert?
      return 1;
  }

  if (!jjs_value_free_unless (context, result, jjs_value_is_exception))
  {
    // TODO: print exception
    jjs_value_free (context, result);
    return 1;
  }

  result = jjs_run_jobs (context);

  if (!jjs_value_free_unless (context, result, jjs_value_is_exception))
  {
    // TODO: print exception
    jjs_value_free (context, result);
    return 1;
  }

  return 0;
}

static int
jjs_cli_run_entry_point (jjs_context_t *context,
                         jjs_cli_include_list_t *includes,
                         jjs_cli_module_t *entry_point,
                         bool parse_only)
{
  if (includes && includes->size)
  {
    const jjs_size_t size = (jjs_size_t) includes->size;

    for (size_t i = 0; i < size; i++)
    {
      jjs_cli_module_t *include = &includes->items[i];

      if (jjs_cli_run_module (context, include, false) != EXIT_SUCCESS)
      {
        return EXIT_FAILURE;
      }
    }
  }

  return jjs_cli_run_module (context, entry_point, parse_only);
}

static int
jjs_main_command_parse (jjs_cli_state_t *state, jjs_cli_module_t *entry_point)
{
  jjs_context_t *context;
  int exit_code = EXIT_FAILURE;

  if (init_engine (state, &context))
  {
    exit_code = jjs_cli_run_module (context, entry_point, true);
    cleanup_engine (context);
  }

  return exit_code;
}

static int
repl (jjs_cli_state_t *state)
{
  if (!set_cwd (state->cwd))
  {
    return EXIT_FAILURE;
  }

  jjs_context_t *context;

  if (!init_engine (state, &context))
  {
    return EXIT_FAILURE;
  }

  jjs_value_t result;
  jjs_value_t prompt = jjs_string_sz (context, "jjs> ");
  jjs_value_t new_line = jjs_string_sz (context, "\n");
  jjs_value_t source_name = jjs_string_sz (context, "<repl>");

  while (true)
  {
    jjs_platform_stdout_write (context, prompt, JJS_KEEP);

    jjs_size_t length;
    jjs_char_t *line_p = stdin_readline (&length);

    if (line_p == NULL)
    {
      jjs_platform_stdout_write (context, new_line, JJS_KEEP);
      goto done;
    }

    if (length == 0)
    {
      continue;
    }

    if (memcmp ((const char *) line_p, ".exit", sizeof (".exit")) == 0)
    {
      free (line_p);
      break;
    }

    if (!jjs_validate_string (context, line_p, length, JJS_ENCODING_UTF8))
    {
      free (line_p);
      result = jjs_throw_sz (context, JJS_ERROR_SYNTAX, "Input is not a valid UTF-8 string");
      goto exception;
    }

    jjs_parse_options_t opts = {
      .source_name = jjs_optional_value (source_name),
    };

    result = jjs_parse (context, line_p, length, &opts);

    free (line_p);

    if (jjs_value_is_exception (context, result))
    {
      goto exception;
    }

    result = jjs_run (context, result, JJS_MOVE);

    if (jjs_value_is_exception (context, result))
    {
      goto exception;
    }

    jjs_cli_log (context, &stdout_wstream, "{}\n", result);

    jjs_value_free (context, result);
    result = jjs_run_jobs (context);

    if (jjs_value_free_unless (context, result, jjs_value_is_exception))
    {
      goto exception;
    }

    continue;

exception:
    jjs_cli_log (context, &stdout_wstream, "{}\n", result);
    jjs_value_free (context, result);
  }

done:
  jjs_value_free (context, prompt);
  jjs_value_free (context, new_line);
  jjs_value_free (context, source_name);
  cleanup_engine (context);
  return 0;
}

static int
run (jjs_cli_state_t *state, jjs_cli_include_list_t *includes, jjs_cli_module_t *entry_point)
{
  int exit_code = 0;
  jjs_context_t *context;

  if (init_engine (state, &context))
  {
    exit_code = jjs_cli_run_entry_point (context, includes, entry_point, false);
    cleanup_engine (context);
  }
  else
  {
    exit_code = EXIT_FAILURE;
  }

  return exit_code;
}

static int
test (jjs_cli_state_t *state, jjs_cli_module_t *entry_point)
{
  int exit_code;
  jjs_context_t *context;

  if (!init_engine (state, &context))
  {
    return EXIT_FAILURE;
  }

  init_test_realm (context);

  if (jjs_cli_run_module (context, entry_point, false) == EXIT_SUCCESS
      && run_all_tests (context)
      && process_async_asserts (context))
  {
    exit_code = EXIT_SUCCESS;
  }
  else
  {
    exit_code = EXIT_FAILURE;
  }

  cleanup_engine (context);

  return exit_code;
}

int
main_cli_log_imcl_error (imcl_args_t *state)
{
  jjs_cli_assert (state->has_error, "imcl should be in the error state");

  printf ("imcl error!\n");

  return EXIT_FAILURE;
}

int
main (int argc, char **argv)
{
  jjs_cli_state_t state = {
    .imcl = imcl_args (argc, argv),
  };
  imcl_args_t *it = &state.imcl;
  int exit_code = EXIT_SUCCESS;

  /* consume arg0 */
  imcl_args_shift (it);

  if (imcl_args_shift_if_command (it, "run"))
  {
    jjs_cli_include_list_t includes;
    jjs_cli_module_t entry_point = {
      .loader = JJS_CLI_LOADER_AUTO,
      .parser = JJS_CLI_PARSER_SLOPPY,
    };

    jjs_cli_include_list_init (&includes);

    while (imcl_args_has_more (it))
    {
      if (imcl_args_shift_if_option (it, NULL, "--loader"))
      {
        entry_point.loader = jjs_cli_loader_from_string (imcl_args_shift (it));
      }
      else if (imcl_args_shift_if_option (it, NULL, "--parser"))
      {
        entry_point.parser = jjs_cli_parser_from_string (imcl_args_shift (it));
      }
      else if (imcl_args_shift_if_option (it, NULL, "--index"))
      {
        // TODO: shift_uint
        entry_point.snapshot_index = imcl_args_shift_uint (it);
      }
      else if (imcl_args_shift_if_option (it, NULL, "--require"))
      {
        jjs_cli_include_list_append (&includes,
                                     imcl_args_shift (it),
                                     JJS_CLI_LOADER_CJS,
                                     JJS_CLI_PARSER_UNKNOWN,
                                     0);
      }
      else if (imcl_args_shift_if_option (it, NULL, "--import"))
      {
        jjs_cli_include_list_append (&includes,
                                     imcl_args_shift (it),
                                     JJS_CLI_LOADER_ESM,
                                     JJS_CLI_PARSER_UNKNOWN,
                                     0);
      }
      else if (imcl_args_shift_if_option (it, NULL, "--preload")
               || imcl_args_shift_if_option (it, NULL, "--preload:sloppy"))
      {
        jjs_cli_include_list_append (&includes,
                                     imcl_args_shift (it),
                                     JJS_CLI_LOADER_JS,
                                     JJS_CLI_PARSER_SLOPPY,
                                     0);
      }
      else if (imcl_args_shift_if_option (it, NULL, "--preload:strict"))
      {
        jjs_cli_include_list_append (&includes,
                                     imcl_args_shift (it),
                                     JJS_CLI_LOADER_JS,
                                     JJS_CLI_PARSER_STRICT,
                                     0);
      }
      else if (imcl_args_shift_if_option (it, NULL, "--preload:snapshot"))
      {
        jjs_cli_include_list_append (&includes,
                                     imcl_args_shift (it),
                                     JJS_CLI_LOADER_JS,
                                     JJS_CLI_PARSER_SNAPSHOT,
                                     // TODO: snapshot index from command
                                     0);
      }
      else if (imcl_args_shift_if_help_option (it))
      {
        print_command_help ("run");
        jjs_cli_include_list_drop (&includes);
        goto done;
      }
      else if (!shift_common_option (&state))
      {
        break;
      }
    }

    if (!it->has_error)
    {
      entry_point.filename = imcl_args_shift (it);
    }

    if (it->has_error)
    {
      exit_code = main_cli_log_imcl_error (it);
    }
    else
    {
      exit_code = run (&state, &includes, &entry_point);
    }

    jjs_cli_include_list_drop (&includes);
  }
  else if (imcl_args_shift_if_command (it, "parse"))
  {
    jjs_cli_module_t entry_point = {
      .loader = JJS_CLI_LOADER_AUTO,
      .parser = JJS_CLI_PARSER_SLOPPY,
    };

    while (imcl_args_has_more (it))
    {
      if (imcl_args_shift_if_option (it, NULL, "--parser"))
      {
        entry_point.parser = jjs_cli_parser_from_string (imcl_args_shift(it));
      }
      else if (imcl_args_shift_if_option (it, NULL, "--index"))
      {
        entry_point.snapshot_index = imcl_args_shift_uint (it);
      }
      else if (imcl_args_shift_if_help_option (it))
      {
        print_command_help ("parse");
        goto done;
      }
      else if (!shift_common_option (&state))
      {
        break;
      }
    }

    entry_point.filename = imcl_args_shift (it);

    if (it->has_error)
    {
      exit_code = main_cli_log_imcl_error (it);
    }
    else
    {
      // TODO: auto
      exit_code = jjs_main_command_parse (&state, &entry_point);
    }
  }
  else if (imcl_args_shift_if_command (it, "repl"))
  {
    while (imcl_args_has_more (it))
    {
      if (imcl_args_shift_if_help_option (it))
      {
        print_command_help ("repl");
        goto done;
      }
      else if (!shift_common_option (&state))
      {
        break;
      }
    }

    exit_code = it->has_error ? main_cli_log_imcl_error (it) : repl (&state);
  }
  else if (imcl_args_shift_if_command (it, "test"))
  {
    jjs_cli_module_t entry_point = {
      .loader = JJS_CLI_LOADER_AUTO,
      .parser = JJS_CLI_PARSER_SLOPPY,
    };

    while (imcl_args_has_more (it))
    {
      if (imcl_args_shift_if_option (it, NULL, "--loader"))
      {
        entry_point.loader = jjs_cli_loader_from_string (imcl_args_shift (it));
      }
      if (imcl_args_shift_if_option (it, NULL, "--parser"))
      {
        entry_point.parser = jjs_cli_parser_from_string (imcl_args_shift (it));
      }
      else if (imcl_args_shift_if_help_option (it))
      {
        print_command_help ("test");
        goto done;
      }
      else if (!shift_common_option (&state))
      {
        break;
      }
    }

    if (!it->has_error)
    {
      entry_point.filename = imcl_args_shift (it);
    }

    if (it->has_error)
    {
      exit_code = main_cli_log_imcl_error (it);
    }
    else
    {
      exit_code = test (&state, &entry_point);
    }
  }
  else if (imcl_args_shift_if_version_option (it))
  {
    printf ("%i.%i.%i\n", JJS_API_MAJOR_VERSION, JJS_API_MINOR_VERSION, JJS_API_PATCH_VERSION);
  }
  else if (imcl_args_shift_if_help_option (it))
  {
    print_help ();
  }
  else
  {
    printf ("Invalid command: %s\n\n", imcl_args_shift (it));
    print_help ();
    exit_code = EXIT_FAILURE;
  }

done:
  imcl_args_drop (it);

  return exit_code;
}
