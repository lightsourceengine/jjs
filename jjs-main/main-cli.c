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
#include <imcl.h>

#define JJS_CLI_IMPLEMENTATION
#include <jjs-cli.h>

#define JJS_TEST_RUNNER_IMPLEMENTATION
#include <jjs-test-runner.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

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
  jjs_cli_allocator_strategy_t buffer_allocator_strategy;
  char **argv;
  int argc;
} jjs_cli_state_t;

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

static void
unhandled_rejection_cb (jjs_context_t* context, jjs_value_t promise, jjs_value_t reason, void *user_ptr)
{
  (void) promise;
  (void) user_ptr;
  jjs_cli_fmt_info (context, "Unhandled promise rejection: {}\n", 1, reason);
}

uint8_t *
system_arraybuffer_allocate (jjs_context_t *context_p,
                             jjs_arraybuffer_type_t buffer_type,
                             uint32_t buffer_size,
                             void **arraybuffer_user_p,
                             void *user_p)
{
  (void) context_p, (void) buffer_type, (void) arraybuffer_user_p, (void) user_p;
  return malloc (buffer_size);
}

void
system_arraybuffer_free (jjs_context_t *context_p,
                         jjs_arraybuffer_type_t buffer_type,
                         uint8_t *buffer_p,
                         uint32_t buffer_size,
                         void *arraybuffer_user_p,
                         void *user_p)
{
  (void) context_p, (void) buffer_p, (void) buffer_size, (void) buffer_type, (void) arraybuffer_user_p, (void) user_p;
  free (buffer_p);
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

  if (state->has_log_level)
  {
    jjs_log_set_level (context, (jjs_log_level_t) state->log_level);
  }

  if (state->buffer_allocator_strategy == JJS_CLI_ALLOCATOR_STRATEGY_SYSTEM
      || state->buffer_allocator_strategy == JJS_CLI_ALLOCATOR_STRATEGY_AUTO)
  {
    jjs_arraybuffer_allocator (context, &system_arraybuffer_allocate, &system_arraybuffer_free, NULL);
  }

#if defined (JJS_PACK) && JJS_PACK
  jjs_pack_init (context, JJS_PACK_INIT_ALL);
#endif /* defined (JJS_PACK) && JJS_PACK */

  if (state->pmap_filename)
  {
    jjs_value_t result = jjs_pmap (context, jjs_string_sz (context, state->pmap_filename), JJS_MOVE, jjs_undefined (context), JJS_MOVE);

    if (jjs_value_is_exception (context, result))
    {
      jjs_cli_fmt_info (context, "Failed to load pmap: {}\n", 1, result);
      jjs_value_free (context, result);
      cleanup_engine (context);
      return false;
    }

    jjs_value_free (context, result);
  }

  jjs_value_t global = jjs_current_realm (context);
  jjs_value_t jjs = jjs_object_get_sz (context, global, "jjs");
  jjs_value_t argv = jjs_array (context, (jjs_size_t) state->argc);
  const jjs_size_t len = jjs_array_length (context, argv);

  for (jjs_size_t i = 0; i < len; i++)
  {
    jjs_value_free (context, jjs_object_set_index (context, argv, i, jjs_string_utf8_sz (context, state->argv[i]), JJS_MOVE));
  }

  jjs_value_free (context, jjs_object_set_sz (context, jjs, "argv", argv, JJS_MOVE));
  jjs_value_free (context, global);
  jjs_value_free (context, jjs);

  *out = context;
  return true;
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

    /* TODO: shouldn't have to do this. fix! */
    if (state->log_level < JJS_LOG_LEVEL_TRACE)
    {
      state->log_level = JJS_LOG_LEVEL_TRACE;
    }
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--show-opcodes"))
  {
    state->context_options.show_op_codes = true;
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--show-regexp-opcodes"))
  {
    state->context_options.show_regexp_op_codes = true;
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--vm-heap-size"))
  {
    /* TODO: validate */
    state->context_options.vm_heap_size_kb = jjs_optional_u32 (imcl_args_shift_uint (&state->imcl));
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--vm-stack-limit"))
  {
    /* TODO: validate */
    state->context_options.vm_stack_limit_kb = jjs_optional_u32 (imcl_args_shift_uint (&state->imcl));
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--gc-new-objects-fraction"))
  {
    /* TODO: validate */
    state->context_options.gc_new_objects_fraction = jjs_optional_u32 (imcl_args_shift_uint (&state->imcl));
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--gc-mark-limit"))
  {
    /* TODO: validate */
    state->context_options.gc_mark_limit = jjs_optional_u32 (imcl_args_shift_uint (&state->imcl));
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--gc-limit"))
  {
    /* TODO: validate */
    state->context_options.gc_limit_kb = jjs_optional_u32 (imcl_args_shift_uint (&state->imcl));
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--scratch-size"))
  {
    /* TODO: validate */
    state->context_options.scratch_size_kb = jjs_optional_u32 (imcl_args_shift_uint (&state->imcl));
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--scratch-allocator"))
  {
    switch (jjs_cli_allocator_strategy_from_string (imcl_args_shift (&state->imcl)))
    {
      case JJS_CLI_ALLOCATOR_STRATEGY_VM:
        state->context_options.scratch_fallback_allocator_type = JJS_SCRATCH_ALLOCATOR_VM;
        break;
      case JJS_CLI_ALLOCATOR_STRATEGY_SYSTEM:
        state->context_options.scratch_fallback_allocator_type = JJS_SCRATCH_ALLOCATOR_SYSTEM;
        break;
      default:
        break;
    }
  }
  else if (imcl_args_shift_if_option (&state->imcl, NULL, "--buffer-allocator"))
  {
    state->buffer_allocator_strategy = jjs_cli_allocator_strategy_from_string (imcl_args_shift (&state->imcl));
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
                                    JJS_SNAPSHOT_EXEC_HAS_SOURCE_NAME | JJS_SNAPSHOT_EXEC_COPY_DATA,
                                    &options);
        jjs_value_free (context, snapshot);
      }
      else
      {
        result = snapshot;
      }

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
    jjs_char_t *line_p = jjs_cli_stdin_readline (&length);

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

    jjs_cli_fmt_info (context, "{}\n", 1, result);

    jjs_value_free (context, result);
    result = jjs_run_jobs (context);

    if (jjs_value_free_unless (context, result, jjs_value_is_exception))
    {
      goto exception;
    }

    continue;

exception:
    jjs_cli_fmt_info (context, "{}\n", 1, result);
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

  jjs_test_runner_install(context);

  if (jjs_cli_run_module (context, entry_point, false) == EXIT_SUCCESS
      && jjs_test_runner_run_all_tests (context)
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

static int
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
    .buffer_allocator_strategy = JJS_CLI_ALLOCATOR_STRATEGY_AUTO,
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
      if (strcmp (imcl_args_current(it), "--") == 0)
      {
        imcl_args_shift(it);

        if (it->index < it->argc)
        {
          state.argv = &it->argv[it->index];
          state.argc = it->argc - it->index;
        }
      }

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
