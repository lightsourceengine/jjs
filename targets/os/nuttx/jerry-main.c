/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jjs-port.h"
#include "jjs.h"

#include "jjs-ext/debugger.h"
#include "jjs-ext/handlers.h"
#include "jjs-ext/print.h"
#include "jjs-ext/properties.h"
#include "jjs-ext/repl.h"
#include "jjs-ext/sources.h"
#include "setjmp.h"

/**
 * Maximum command line arguments number.
 */
#define JJS_MAX_COMMAND_LINE_ARGS (16)

/**
 * Standalone JJS exit codes.
 */
#define JJS_STANDALONE_EXIT_CODE_OK   (0)
#define JJS_STANDALONE_EXIT_CODE_FAIL (1)

/**
 * Context size of the SYNTAX_ERROR
 */
#define SYNTAX_ERROR_CONTEXT_SIZE 2

/**
 * Print usage and available options
 */
static void
print_help (char *name)
{
  printf ("Usage: %s [OPTION]... [FILE]...\n"
          "\n"
          "Options:\n"
          "  --log-level [0-3]\n"
          "  --mem-stats\n"
          "  --mem-stats-separate\n"
          "  --show-opcodes\n"
          "  --start-debug-server\n"
          "  --debug-server-port [port]\n"
          "\n",
          name);
} /* print_help */

/**
 * Convert string into unsigned integer
 *
 * @return converted number
 */
static uint32_t
str_to_uint (const char *num_str_p, /**< string to convert */
             char **out_p) /**< [out] end of the number */
{
  assert (jjs_feature_enabled (JJS_FEATURE_ERROR_MESSAGES));

  uint32_t result = 0;

  while (*num_str_p >= '0' && *num_str_p <= '9')
  {
    result *= 10;
    result += (uint32_t) (*num_str_p - '0');
    num_str_p++;
  }

  if (out_p != NULL)
  {
    *out_p = num_str_p;
  }

  return result;
} /* str_to_uint */

/**
 * Register a JavaScript function in the global object.
 */
static void
register_js_function (const char *name_p, /**< name of the function */
                      jjs_external_handler_t handler_p) /**< function callback */
{
  jjs_value_t result_val = jjsx_register_global (name_p, handler_p);

  if (jjs_value_is_exception (result_val))
  {
    jjs_log (JJS_LOG_LEVEL_WARNING, "Warning: failed to register '%s' method.", name_p);
  }

  jjs_value_free (result_val);
} /* register_js_function */

/**
 * Main program.
 *
 * @return 0 if success, error code otherwise
 */
#ifdef CONFIG_BUILD_KERNEL
int
main (int argc, FAR char *argv[])
#else /* !defined(CONFIG_BUILD_KERNEL) */
int
jjs_main (int argc, char *argv[])
#endif /* defined(CONFIG_BUILD_KERNEL) */
{
  if (argc > JJS_MAX_COMMAND_LINE_ARGS)
  {
    jjs_log (JJS_LOG_LEVEL_ERROR,
               "Too many command line arguments. Current maximum is %d\n",
               JJS_MAX_COMMAND_LINE_ARGS);

    return JJS_STANDALONE_EXIT_CODE_FAIL;
  }

  const char *file_names[JJS_MAX_COMMAND_LINE_ARGS];
  int i;
  int files_counter = 0;
  bool start_debug_server = false;
  uint16_t debug_port = 5001;

  jjs_init_flag_t flags = JJS_INIT_EMPTY;

  for (i = 1; i < argc; i++)
  {
    if (!strcmp ("-h", argv[i]) || !strcmp ("--help", argv[i]))
    {
      print_help (argv[0]);
      return JJS_STANDALONE_EXIT_CODE_OK;
    }
    else if (!strcmp ("--mem-stats", argv[i]))
    {
      flags |= JJS_INIT_MEM_STATS;
      jjs_log_set_level (JJS_LOG_LEVEL_DEBUG);
    }
    else if (!strcmp ("--show-opcodes", argv[i]))
    {
      flags |= JJS_INIT_SHOW_OPCODES | JJS_INIT_SHOW_REGEXP_OPCODES;
      jjs_log_set_level (JJS_LOG_LEVEL_DEBUG);
    }
    else if (!strcmp ("--log-level", argv[i]))
    {
      if (++i < argc && strlen (argv[i]) == 1 && argv[i][0] >= '0' && argv[i][0] <= '3')
      {
        jjs_log_set_level (argv[i][0] - '0');
      }
      else
      {
        jjs_log (JJS_LOG_LEVEL_ERROR, "Error: wrong format or invalid argument\n");
        return JJS_STANDALONE_EXIT_CODE_FAIL;
      }
    }
    else if (!strcmp ("--start-debug-server", argv[i]))
    {
      start_debug_server = true;
    }
    else if (!strcmp ("--debug-server-port", argv[i]))
    {
      if (++i < argc)
      {
        debug_port = str_to_uint (argv[i], NULL);
      }
      else
      {
        jjs_log (JJS_LOG_LEVEL_ERROR, "Error: wrong format or invalid argument\n");
        return JJS_STANDALONE_EXIT_CODE_FAIL;
      }
    }
    else
    {
      file_names[files_counter++] = argv[i];
    }
  }

  jjs_init (flags);

  if (start_debug_server)
  {
    jjsx_debugger_after_connect (jjsx_debugger_tcp_create (debug_port) && jjsx_debugger_ws_create ());
  }

  register_js_function ("assert", jjsx_handler_assert);
  register_js_function ("gc", jjsx_handler_gc);
  register_js_function ("print", jjsx_handler_print);

  jjs_value_t ret_value = jjs_undefined ();
  int ret_code = JJS_STANDALONE_EXIT_CODE_OK;

  if (files_counter == 0)
  {
    jjsx_repl ("jjs> ");
  }
  else
  {
    for (i = 0; i < files_counter; i++)
    {
      ret_value = jjsx_source_exec_script (file_names[i]);
      if (jjs_value_is_exception (ret_value))
      {
        ret_code = JJS_STANDALONE_EXIT_CODE_FAIL;
        jjsx_print_unhandled_exception (ret_value);
        break;
      }

      jjs_value_free (ret_value);
    }
  }

  ret_value = jjs_run_jobs ();

  if (jjs_value_is_exception (ret_value))
  {
    ret_code = JJS_STANDALONE_EXIT_CODE_FAIL;
  }

  jjs_value_free (ret_value);
  jjs_cleanup ();

  return ret_code;
} /* main */
