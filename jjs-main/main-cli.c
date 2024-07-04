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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMCL_IMPLEMENTATION
#include <imcl.h>

#define JJS_CLI_IMPLEMENTATION
#include <jjs-cli.h>

#define JJS_TEST_RUNNER_IMPLEMENTATION
#include <jjs-test-runner.h>

typedef struct
{
  jjs_cli_config_t config;
  jjs_cli_module_t entry_point;
  jjs_cli_module_list_t includes;
} jjs_app_t;

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
jjs_app_command_help (void)
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
  else if (strcmp (name, "parse") == 0)
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

static bool
jjs_app_has_ext (const char *filename, const char *ext)
{
  filename = strrchr (filename, '.');
  return (filename != NULL) ? strcmp (ext, filename) == 0 : false;
}

static bool
jjs_app_starts_with (const char *prefix, const char *value)
{
  size_t prefix_len = strlen (prefix);
  size_t value_len = strlen (value);

  return value_len < prefix_len ? false : memcmp (prefix, value, prefix_len) == 0;
}

static jjs_cli_loader_t
jjs_app_resolve_loader (const char *filename, jjs_cli_loader_t loader)
{
  jjs_cli_assert (loader != JJS_CLI_LOADER_UNKNOWN, "loader should have been selected by now");

  if (loader != JJS_CLI_LOADER_UNDEFINED)
  {
    return loader;
  }

  if (jjs_app_has_ext (filename, ".js") || jjs_app_has_ext (filename, ".mjs"))
  {
    return JJS_CLI_LOADER_ESM;
  }

  if (jjs_app_has_ext (filename, ".cjs"))
  {
    return JJS_CLI_LOADER_CJS;
  }

  if (jjs_app_has_ext (filename, ".snapshot") || jjs_app_has_ext (filename, ".bin"))
  {
    return JJS_CLI_LOADER_SNAPSHOT;
  }

  return JJS_CLI_LOADER_ESM;
}

static bool
jjs_app_shift_common_option (imcl_args_t *args, jjs_cli_config_t *config)
{
  if (imcl_args_shift_if_option (args, NULL, "--cwd"))
  {
    config->cwd = imcl_args_shift (args);
  }
  else if (imcl_args_shift_if_option (args, NULL, "--pmap"))
  {
    config->pmap_filename = imcl_args_shift (args);
  }
  else if (imcl_args_shift_if_option (args, NULL, "--mem-stats"))
  {
    config->context_options.enable_mem_stats = true;

    /* TODO: shouldn't have to do this. fix! */
    if (config->log_level < JJS_LOG_LEVEL_TRACE)
    {
      config->log_level = JJS_LOG_LEVEL_TRACE;
    }
  }
  else if (imcl_args_shift_if_option (args, NULL, "--show-opcodes"))
  {
    config->context_options.show_op_codes = true;
  }
  else if (imcl_args_shift_if_option (args, NULL, "--show-regexp-opcodes"))
  {
    config->context_options.show_regexp_op_codes = true;
  }
  else if (imcl_args_shift_if_option (args, NULL, "--vm-heap-size"))
  {
    /* TODO: validate */
    config->context_options.vm_heap_size_kb = jjs_optional_u32 (imcl_args_shift_uint (args));
  }
  else if (imcl_args_shift_if_option (args, NULL, "--vm-stack-limit"))
  {
    /* TODO: validate */
    config->context_options.vm_stack_limit_kb = jjs_optional_u32 (imcl_args_shift_uint (args));
  }
  else if (imcl_args_shift_if_option (args, NULL, "--gc-new-objects-fraction"))
  {
    /* TODO: validate */
    config->context_options.gc_new_objects_fraction = jjs_optional_u32 (imcl_args_shift_uint (args));
  }
  else if (imcl_args_shift_if_option (args, NULL, "--gc-mark-limit"))
  {
    /* TODO: validate */
    config->context_options.gc_mark_limit = jjs_optional_u32 (imcl_args_shift_uint (args));
  }
  else if (imcl_args_shift_if_option (args, NULL, "--gc-limit"))
  {
    /* TODO: validate */
    config->context_options.gc_limit_kb = jjs_optional_u32 (imcl_args_shift_uint (args));
  }
  else if (imcl_args_shift_if_option (args, NULL, "--scratch-size"))
  {
    /* TODO: validate */
    config->context_options.scratch_size_kb = jjs_optional_u32 (imcl_args_shift_uint (args));
  }
  else if (imcl_args_shift_if_option (args, NULL, "--scratch-allocator"))
  {
    switch (jjs_cli_allocator_strategy_from_string (imcl_args_shift (args)))
    {
      case JJS_CLI_ALLOCATOR_STRATEGY_VM:
        config->context_options.scratch_fallback_allocator_type = JJS_SCRATCH_ALLOCATOR_VM;
        break;
      case JJS_CLI_ALLOCATOR_STRATEGY_SYSTEM:
        config->context_options.scratch_fallback_allocator_type = JJS_SCRATCH_ALLOCATOR_SYSTEM;
        break;
      default:
        break;
    }
  }
  else if (imcl_args_shift_if_option (args, NULL, "--buffer-allocator"))
  {
    config->buffer_allocator_strategy = jjs_cli_allocator_strategy_from_string (imcl_args_shift (args));
  }
  else if (imcl_args_shift_if_option (args, NULL, "--log-level"))
  {
    config->log_level = imcl_args_shift_ranged_int (args, 0, 3);
    config->has_log_level = true;
  }
  else
  {
    return false;
  }

  return true;
}

static int
jjs_cli_parse_only (jjs_context_t *context, jjs_cli_module_t *module)
{
  jjs_value_t filename = jjs_platform_realpath_sz (context, module->filename);

  if (jjs_value_is_exception (context, filename))
  {
    jjs_value_free (context, filename);
    jjs_cli_fmt_error (context, "Cannot find module: {}\n", 1, filename);
    return JJS_CLI_EXIT_FAILURE;
  }

  jjs_cli_loader_t loader = jjs_app_resolve_loader (module->filename, module->loader);
  jjs_platform_read_file_options_t read_file_options = {
    .encoding = JJS_ENCODING_UTF8,
  };
  jjs_value_t source = jjs_platform_read_file (context, filename, JJS_KEEP, &read_file_options);
  jjs_value_t result;

  if (!jjs_value_is_exception (context, source))
  {
    jjs_parse_options_t opts = {
      .is_strict_mode = (loader == JJS_CLI_LOADER_STRICT),
      .parse_module = (loader == JJS_CLI_LOADER_ESM),
      .source_name = jjs_optional_value (filename),
      .source_name_o = JJS_KEEP,
    };

    result = jjs_parse_value (context, source, JJS_MOVE, &opts);
  }
  else
  {
    result = source;
  }

  jjs_value_free (context, filename);

  if (!jjs_value_free_unless (context, result, jjs_value_is_exception))
  {
    // TODO: print exception
    jjs_value_free (context, result);
    return JJS_CLI_EXIT_FAILURE;
  }

  return JJS_CLI_EXIT_SUCCESS;
}

static int
jjs_cli_run_module (jjs_context_t *context, jjs_cli_module_t *module)
{
  jjs_value_t filename = module->is_main ? jjs_platform_realpath_sz (context, module->filename) : jjs_string_utf8_sz (context, module->filename);

  if (jjs_value_is_exception (context, filename))
  {
    jjs_value_free (context, filename);
    jjs_cli_fmt_error (context, "Cannot find module: {}\n", 1, filename);
    return JJS_CLI_EXIT_FAILURE;
  }

  jjs_value_t result;
  jjs_cli_loader_t loader = jjs_app_resolve_loader (module->filename, module->loader);

  switch (loader)
  {
    case JJS_CLI_LOADER_CJS:
    {
      result = jjs_commonjs_require (context, filename, JJS_MOVE);
      break;
    }
    case JJS_CLI_LOADER_ESM:
    {
      result = jjs_esm_import (context, filename, JJS_MOVE);
      break;
    }
    case JJS_CLI_LOADER_SNAPSHOT:
    {
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
    case JJS_CLI_LOADER_STRICT:
    case JJS_CLI_LOADER_SLOPPY:
    {
      jjs_platform_read_file_options_t read_file_options = {
        .encoding = JJS_ENCODING_UTF8,
      };
      jjs_value_t source = jjs_platform_read_file (context, filename, JJS_KEEP, &read_file_options);

      if (!jjs_value_is_exception (context, source))
      {
        jjs_parse_options_t opts = {
          .is_strict_mode = (loader == JJS_CLI_LOADER_STRICT),
          .source_name = jjs_optional_value (filename),
          .source_name_o = JJS_KEEP,
        };

        jjs_value_t parse_result = jjs_parse_value (context, source, JJS_MOVE, &opts);

        if (!jjs_value_is_exception (context, parse_result))
        {
          result = jjs_run (context, parse_result, JJS_MOVE);
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
      jjs_value_free (context, filename);
      return JJS_CLI_EXIT_FAILURE;
  }

  if (!jjs_value_free_unless (context, result, jjs_value_is_exception))
  {
    // TODO: print exception
    jjs_value_free (context, result);
    return JJS_CLI_EXIT_FAILURE;
  }

  result = jjs_run_jobs (context);

  if (!jjs_value_free_unless (context, result, jjs_value_is_exception))
  {
    // TODO: print exception
    jjs_value_free (context, result);
    return JJS_CLI_EXIT_FAILURE;
  }

  return JJS_CLI_EXIT_SUCCESS;
}

static int
jjs_cli_run_entry_point (jjs_context_t *context,
                         jjs_cli_module_list_t *includes,
                         jjs_cli_module_t *entry_point)
{
  if (includes && includes->size)
  {
    const jjs_size_t size = (jjs_size_t) includes->size;

    for (size_t i = 0; i < size; i++)
    {
      jjs_cli_module_t *include = &includes->items[i];

      if (jjs_cli_run_module (context, include) != JJS_CLI_EXIT_SUCCESS)
      {
        return JJS_CLI_EXIT_FAILURE;
      }
    }
  }

  return jjs_cli_run_module (context, entry_point);
}

static int
jjs_app_command_parse (jjs_app_t *app)
{
  jjs_context_t *context;
  int exit_code = JJS_CLI_EXIT_FAILURE;

  if (jjs_cli_engine_init (&app->config, &context))
  {
    exit_code = jjs_cli_parse_only (context, &app->entry_point);
    jjs_cli_engine_drop (context);
  }

  return exit_code;
}

static int
jjs_app_command_repl (jjs_app_t *config)
{
  jjs_context_t *context;

  if (!jjs_cli_engine_init (&config->config, &context))
  {
    return JJS_CLI_EXIT_FAILURE;
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
  jjs_cli_engine_drop (context);
  return JJS_CLI_EXIT_SUCCESS;
}

static int
jjs_app_command_run (jjs_app_t *app)
{
  int exit_code;
  jjs_context_t *context;

  if (jjs_cli_engine_init (&app->config, &context))
  {
    exit_code = jjs_cli_run_entry_point (context, &app->includes, &app->entry_point);
    jjs_cli_engine_drop (context);
  }
  else
  {
    exit_code = JJS_CLI_EXIT_FAILURE;
  }

  return exit_code;
}

static int
jjs_app_command_test (jjs_app_t *app)
{
  int exit_code;
  jjs_context_t *context;

  if (!jjs_cli_engine_init (&app->config, &context))
  {
    return JJS_CLI_EXIT_FAILURE;
  }

  jjs_test_runner_install(context);

  if (jjs_cli_run_module (context, &app->entry_point) == JJS_CLI_EXIT_SUCCESS
      && jjs_test_runner_run_all_tests (context)
      && process_async_asserts (context))
  {
    exit_code = JJS_CLI_EXIT_SUCCESS;
  }
  else
  {
    exit_code = JJS_CLI_EXIT_FAILURE;
  }

  jjs_cli_engine_drop (context);

  return exit_code;
}

static int
main_cli_log_imcl_error (imcl_args_t *args)
{
  jjs_cli_assert (args->has_error, "imcl should be in the error state");

  printf ("imcl error!\n");

  return JJS_CLI_EXIT_FAILURE;
}

static bool
jjs_app_shift_if_preload (imcl_args_t *args, jjs_cli_loader_t *out_loader, jjs_size_t *out_index)
{
  if (!imcl_args_has_more (args))
  {
    return false;
  }

  const size_t PREFIX_LEN = sizeof ("--preload") - 1;
  const char* option = imcl_args_current (args);

  if (!jjs_app_starts_with ("--preload", option))
  {
    return false;
  }

  args->option_long = option;
  args->option_short = NULL;

  if (option[PREFIX_LEN] == '\0')
  {
    *out_loader = JJS_CLI_LOADER_UNDEFINED;
    *out_index = 0;
  }
  else if (strcmp(&option[PREFIX_LEN], ":sloppy") == 0)
  {
    *out_loader = JJS_CLI_LOADER_SLOPPY;
    *out_index = 0;
  }
  else if (strcmp(&option[PREFIX_LEN], ":strict") == 0)
  {
    *out_loader = JJS_CLI_LOADER_STRICT;
    *out_index = 0;
  }
  else if (strcmp(&option[PREFIX_LEN], ":snapshot") == 0)
  {
    *out_loader = JJS_CLI_LOADER_SNAPSHOT;
    *out_index = 0;
  }
  else
  {
    printf ("invalid tagged option %s\n", option);
    return false;
  }

  imcl_args_shift (args);
  return true;
}

static bool
jjs_app_shift_loader_option (imcl_args_t *args, jjs_cli_loader_t *out_loader)
{
  if (!imcl_args_shift_if_option (args, NULL, "--loader"))
  {
    return false;
  }

  jjs_cli_loader_t loader = jjs_cli_loader_from_string (imcl_args_shift (args));

  if (args->has_error)
  {
    return false;
  }

  if (loader == JJS_CLI_LOADER_UNKNOWN)
  {
    args->has_error = true;
    // TODO: fix error
    args->error = "invalid loader value";
    return false;
  }

  *out_loader = loader;
  return true;
}

int
main (int argc, char **argv)
{
  jjs_app_t app = {0};
  imcl_args_t args = imcl_args (argc, argv);
  int exit_code = JJS_CLI_EXIT_SUCCESS;
  jjs_cli_loader_t preload_loader;
  jjs_size_t preload_index;

  /* consume arg0 */
  imcl_args_shift (&args);

  if (imcl_args_shift_if_command (&args, "run"))
  {
    while (imcl_args_has_more (&args))
    {
      if (jjs_app_shift_loader_option (&args, &app.entry_point.loader))
      {
        continue;
      }
      else if (imcl_args_shift_if_option (&args, NULL, "--index"))
      {
        app.entry_point.snapshot_index = imcl_args_shift_uint (&args);
      }
      else if (imcl_args_shift_if_option (&args, NULL, "--require"))
      {
        jjs_cli_module_list_append (&app.includes,
                                    imcl_args_shift (&args),
                                    JJS_CLI_LOADER_CJS,
                                    0);
      }
      else if (imcl_args_shift_if_option (&args, NULL, "--import"))
      {
        jjs_cli_module_list_append (&app.includes,
                                    imcl_args_shift (&args),
                                    JJS_CLI_LOADER_ESM,
                                    0);
      }
      else if (jjs_app_shift_if_preload (&args, &preload_loader, &preload_index))
      {
        jjs_cli_module_list_append (&app.includes,
                                    imcl_args_shift (&args),
                                    preload_loader,
                                    preload_index);
      }
      else if (imcl_args_shift_if_help_option (&args))
      {
        print_command_help ("run");
        goto done;
      }
      else if (!jjs_app_shift_common_option (&args, &app.config))
      {
        app.entry_point.filename = imcl_args_shift_file (&args);
        app.entry_point.is_main = true;
        break;
      }
    }

    if (!args.has_error && strcmp (imcl_args_current(&args), "--") == 0)
    {
      imcl_args_shift(&args);

      if (args.index < args.argc)
      {
        app.config.argv = &args.argv[args.index];
        app.config.argc = args.argc - args.index;
      }
    }

    exit_code = args.has_error ? JJS_CLI_EXIT_FAILURE : jjs_app_command_run (&app);
  }
  else if (imcl_args_shift_if_command (&args, "parse"))
  {
    while (imcl_args_has_more (&args))
    {
      if (jjs_app_shift_loader_option (&args, &app.entry_point.loader))
      {
        continue;
      }
      else if (imcl_args_shift_if_option (&args, NULL, "--index"))
      {
        app.entry_point.snapshot_index = imcl_args_shift_uint (&args);
      }
      else if (imcl_args_shift_if_help_option (&args))
      {
        print_command_help ("parse");
        goto done;
      }
      else if (!jjs_app_shift_common_option (&args, &app.config))
      {
        app.entry_point.filename = imcl_args_shift_file (&args);
        app.entry_point.is_main = true;
        break;
      }
    }

    exit_code = args.has_error ? JJS_CLI_EXIT_FAILURE : jjs_app_command_parse (&app);
  }
  else if (imcl_args_shift_if_command (&args, "repl"))
  {
    while (imcl_args_has_more (&args))
    {
      if (imcl_args_shift_if_help_option (&args))
      {
        print_command_help ("repl");
        goto done;
      }
      else if (!jjs_app_shift_common_option (&args, &app.config))
      {
        if (imcl_args_has_more (&args))
        {
          args.has_error = true;
          // TODO: better error handling.
          args.error = "unexpected option for repl command";
        }
        break;
      }
    }

    exit_code = args.has_error ? main_cli_log_imcl_error (&args) : jjs_app_command_repl (&app);
  }
  else if (imcl_args_shift_if_command (&args, "test"))
  {
    while (imcl_args_has_more (&args))
    {
      if (jjs_app_shift_loader_option (&args, &app.entry_point.loader))
      {
        continue;
      }
      else if (imcl_args_shift_if_option (&args, NULL, "--index"))
      {
        app.entry_point.snapshot_index = imcl_args_shift_uint (&args);
      }
      else if (imcl_args_shift_if_help_option (&args))
      {
        print_command_help ("test");
        goto done;
      }
      else if (!jjs_app_shift_common_option (&args, &app.config))
      {
        app.entry_point.filename = imcl_args_shift_file (&args);
        app.entry_point.is_main = true;
        break;
      }
    }

    exit_code = args.has_error ? JJS_CLI_EXIT_FAILURE : jjs_app_command_test (&app);
  }
  else if (imcl_args_shift_if_command (&args, "version") || imcl_args_shift_if_version_option (&args))
  {
    printf ("%i.%i.%i\n", JJS_API_MAJOR_VERSION, JJS_API_MINOR_VERSION, JJS_API_PATCH_VERSION);
  }
  else if (imcl_args_shift_if_command (&args, "help") || imcl_args_shift_if_help_option (&args))
  {
    jjs_app_command_help ();
  }
  else
  {
    printf ("Invalid command: %s\n\n", imcl_args_shift (&args));
    jjs_app_command_help ();
    exit_code = JJS_CLI_EXIT_FAILURE;
  }

done:
  if (args.has_error)
  {
    exit_code = main_cli_log_imcl_error (&args);
  }

  imcl_args_drop (&args);
  jjs_cli_module_list_drop (&app.includes);

  return exit_code;
}
