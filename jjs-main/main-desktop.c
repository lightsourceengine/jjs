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

#include "arguments/options.h"
#include "jjs-ext/debugger.h"
#include "jjs-ext/handlers.h"
#include "jjs-ext/print.h"
#include "jjs-ext/properties.h"
#include "jjs-ext/repl.h"
#include "jjs-ext/sources.h"
#include "jjs-ext/test262.h"

/**
 * Initialize random seed
 */
static void
main_init_random_seed (void)
{
  union
  {
    double d;
    unsigned u;
  } now = { .d = jjs_port_current_time () };

  srand (now.u);
} /* main_init_random_seed */

/**
 * Initialize debugger
 */
static bool
main_init_debugger (main_args_t *arguments_p) /**< main arguments */
{
  bool result = false;

  if (!strcmp (arguments_p->debug_protocol, "tcp"))
  {
    result = jjsx_debugger_tcp_create (arguments_p->debug_port);
  }
  else
  {
    assert (!strcmp (arguments_p->debug_protocol, "serial"));
    result = jjsx_debugger_serial_create (arguments_p->debug_serial_config);
  }

  if (!strcmp (arguments_p->debug_channel, "rawpacket"))
  {
    result = result && jjsx_debugger_rp_create ();
  }
  else
  {
    assert (!strcmp (arguments_p->debug_channel, "websocket"));
    result = result && jjsx_debugger_ws_create ();
  }

  jjsx_debugger_after_connect (result);
  return result;
} /* main_init_debugger */

/**
 * Inits the engine and the debugger
 */
static void
main_init_engine (main_args_t *arguments_p) /**< main arguments */
{
  jjs_init (arguments_p->init_flags);

  jjs_promise_on_event (JJS_PROMISE_EVENT_FILTER_ERROR, jjsx_handler_promise_reject, NULL);

  if (arguments_p->option_flags & OPT_FLAG_DEBUG_SERVER)
  {
    if (!main_init_debugger (arguments_p))
    {
      jjs_log (JJS_LOG_LEVEL_WARNING, "Failed to initialize debugger!\n");
    }
  }

  if (arguments_p->option_flags & OPT_FLAG_TEST262_OBJECT)
  {
    jjsx_test262_register ();
  }

  jjsx_register_global ("assert", jjsx_handler_assert);
  jjsx_register_global ("gc", jjsx_handler_gc);
  jjsx_register_global ("print", jjsx_handler_print);
  jjsx_register_global ("sourceName", jjsx_handler_source_name);
  jjsx_register_global ("createRealm", jjsx_handler_create_realm);
  jjsx_register_global ("include", jjsx_handler_include);
} /* main_init_engine */

int
main (int argc, char **argv)
{
  main_init_random_seed ();
  JJS_VLA (main_source_t, sources_p, argc);

  main_args_t arguments;
  arguments.sources_p = sources_p;

  if (!main_parse_args (argc, argv, &arguments))
  {
    return arguments.parse_result;
  }

restart:
  main_init_engine (&arguments);
  int return_code = JJS_STANDALONE_EXIT_CODE_FAIL;
  jjs_value_t result;

  for (uint32_t source_index = 0; source_index < arguments.source_count; source_index++)
  {
    main_source_t *source_file_p = sources_p + source_index;
    const char *file_path_p = argv[source_file_p->path_index];

    switch (source_file_p->type)
    {
      case SOURCE_MODULE:
      {
        result = jjsx_source_exec_module (file_path_p);
        break;
      }
      case SOURCE_SNAPSHOT:
      {
        result = jjsx_source_exec_snapshot (file_path_p, source_file_p->snapshot_index);
        break;
      }
      default:
      {
        assert (source_file_p->type == SOURCE_SCRIPT);

        if ((arguments.option_flags & OPT_FLAG_PARSE_ONLY) != 0)
        {
          result = jjsx_source_parse_script (file_path_p);
        }
        else
        {
          result = jjsx_source_exec_script (file_path_p);
        }

        break;
      }
    }

    if (jjs_value_is_exception (result))
    {
      if (jjsx_debugger_is_reset (result))
      {
        jjs_cleanup ();

        goto restart;
      }

      jjsx_print_unhandled_exception (result);
      goto exit;
    }

    jjs_value_free (result);
  }

  if (arguments.option_flags & OPT_FLAG_WAIT_SOURCE)
  {
    while (true)
    {
      jjs_debugger_wait_for_source_status_t receive_status;
      receive_status = jjs_debugger_wait_for_client_source (jjsx_handler_source_received, NULL, &result);

      if (receive_status == JJS_DEBUGGER_SOURCE_RECEIVE_FAILED)
      {
        jjs_log (JJS_LOG_LEVEL_ERROR, "Connection aborted before source arrived.");
        goto exit;
      }

      if (receive_status == JJS_DEBUGGER_SOURCE_END)
      {
        jjs_log (JJS_LOG_LEVEL_DEBUG, "No more client source.\n");
        break;
      }

      assert (receive_status == JJS_DEBUGGER_CONTEXT_RESET_RECEIVED
              || receive_status == JJS_DEBUGGER_SOURCE_RECEIVED);

      if (receive_status == JJS_DEBUGGER_CONTEXT_RESET_RECEIVED || jjsx_debugger_is_reset (result))
      {
        jjs_cleanup ();
        goto restart;
      }

      assert (receive_status == JJS_DEBUGGER_SOURCE_RECEIVED);
      jjs_value_free (result);
    }
  }
  else if (arguments.option_flags & OPT_FLAG_USE_STDIN)
  {
    result = jjsx_source_exec_stdin ();

    if (jjs_value_is_exception (result))
    {
      jjsx_print_unhandled_exception (result);
      goto exit;
    }

    jjs_value_free (result);
  }
  else if (arguments.source_count == 0)
  {
    const char *prompt_p = (arguments.option_flags & OPT_FLAG_NO_PROMPT) ? "" : "jjs> ";
    jjsx_repl (prompt_p);
  }

  result = jjs_run_jobs ();

  if (jjs_value_is_exception (result))
  {
    jjsx_print_unhandled_exception (result);
    goto exit;
  }

  jjs_value_free (result);

  if (arguments.exit_cb_name_p != NULL)
  {
    jjs_value_t global = jjs_current_realm ();
    jjs_value_t callback_fn = jjs_object_get_sz (global, arguments.exit_cb_name_p);
    jjs_value_free (global);

    if (jjs_value_is_function (callback_fn))
    {
      result = jjs_call (callback_fn, jjs_undefined (), NULL, 0);

      if (jjs_value_is_exception (result))
      {
        jjsx_print_unhandled_exception (result);
        goto exit;
      }

      jjs_value_free (result);
    }

    jjs_value_free (callback_fn);
  }

  return_code = JJS_STANDALONE_EXIT_CODE_OK;

exit:
  jjs_cleanup ();

  return return_code;
} /* main */
