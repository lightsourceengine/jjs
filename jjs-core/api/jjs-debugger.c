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

#include "jjs.h"

#include "debugger.h"
#include "jcontext.h"

/**
 * Checks whether the debugger is connected.
 *
 * @return true - if the debugger is connected
 *         false - otherwise
 */
bool
jjs_debugger_is_connected (jjs_context_t* context_p) /**< JJS context */
{
#if JJS_DEBUGGER
  return context_p->debugger_flags & JJS_DEBUGGER_CONNECTED;
#else /* !JJS_DEBUGGER */
  JJS_UNUSED (context_p);
  return false;
#endif /* JJS_DEBUGGER */
} /* jjs_debugger_is_connected */

/**
 * Stop execution at the next available breakpoint.
 */
void
jjs_debugger_stop (jjs_context_t* context_p) /**< JJS context */
{
#if JJS_DEBUGGER
  if ((context_p->debugger_flags & JJS_DEBUGGER_CONNECTED)
      && !(context_p->debugger_flags & JJS_DEBUGGER_BREAKPOINT_MODE))
  {
    JJS_DEBUGGER_SET_FLAGS (JJS_DEBUGGER_VM_STOP);
    context_p->debugger_stop_context = NULL;
  }
#else /* !JJS_DEBUGGER */
  JJS_UNUSED (context_p);
#endif /* JJS_DEBUGGER */
} /* jjs_debugger_stop */

/**
 * Continue execution.
 */
void
jjs_debugger_continue (jjs_context_t* context_p) /**< JJS context */
{
#if JJS_DEBUGGER
  if ((context_p->debugger_flags & JJS_DEBUGGER_CONNECTED)
      && !(context_p->debugger_flags & JJS_DEBUGGER_BREAKPOINT_MODE))
  {
    JJS_DEBUGGER_CLEAR_FLAGS (JJS_DEBUGGER_VM_STOP);
    context_p->debugger_stop_context = NULL;
  }
#else /* !JJS_DEBUGGER */
  JJS_UNUSED (context_p);
#endif /* JJS_DEBUGGER */
} /* jjs_debugger_continue */

/**
 * Sets whether the engine should stop at breakpoints.
 */
void
jjs_debugger_stop_at_breakpoint (jjs_context_t* context_p, /**< JJS context */
                                 bool enable_stop_at_breakpoint) /**< enable/disable stop at breakpoint */
{
#if JJS_DEBUGGER
  if (context_p->debugger_flags & JJS_DEBUGGER_CONNECTED
      && !(context_p->debugger_flags & JJS_DEBUGGER_BREAKPOINT_MODE))
  {
    if (enable_stop_at_breakpoint)
    {
      JJS_DEBUGGER_SET_FLAGS (JJS_DEBUGGER_VM_IGNORE);
    }
    else
    {
      JJS_DEBUGGER_CLEAR_FLAGS (JJS_DEBUGGER_VM_IGNORE);
    }
  }
#else /* !JJS_DEBUGGER */
  JJS_UNUSED_ALL (context_p, enable_stop_at_breakpoint);
#endif /* JJS_DEBUGGER */
} /* jjs_debugger_stop_at_breakpoint */

/**
 * Sets whether the engine should wait and run a source.
 *
 * @return enum JJS_DEBUGGER_SOURCE_RECEIVE_FAILED - if the source is not received
 *              JJS_DEBUGGER_SOURCE_RECEIVED - if a source code received
 *              JJS_DEBUGGER_SOURCE_END - the end of the source codes
 *              JJS_DEBUGGER_CONTEXT_RESET_RECEIVED - the end of the context
 */
jjs_debugger_wait_for_source_status_t
jjs_debugger_wait_for_client_source (jjs_context_t* context_p, /**< JJS context */
                                     jjs_debugger_wait_for_source_callback_t callback_p, /**< callback function */
                                     void *user_p, /**< user pointer passed to the callback */
                                     jjs_value_t *return_value) /**< [out] parse and run return value */
{
  *return_value = jjs_undefined (context_p);

#if JJS_DEBUGGER
  if (context_p->debugger_flags & JJS_DEBUGGER_CONNECTED)
      && !(context_p->debugger_flags & JJS_DEBUGGER_BREAKPOINT_MODE))
  {
    JJS_DEBUGGER_SET_FLAGS (JJS_DEBUGGER_CLIENT_SOURCE_MODE);
    jjs_debugger_uint8_data_t *client_source_data_p = NULL;
    jjs_debugger_wait_for_source_status_t ret_type = JJS_DEBUGGER_SOURCE_RECEIVE_FAILED;

    /* Notify the client about that the engine is waiting for a source. */
    jjs_debugger_send_type (JJS_DEBUGGER_WAIT_FOR_SOURCE);

    while (true)
    {
      if (jjs_debugger_receive (&client_source_data_p))
      {
        if (!(context_p->debugger_flags & JJS_DEBUGGER_CONNECTED))
        {
          break;
        }

        /* Stop executing the current context. */
        if ((context_p->debugger_flags & JJS_DEBUGGER_CONTEXT_RESET_MODE))
        {
          ret_type = JJS_DEBUGGER_CONTEXT_RESET_RECEIVED;
          JJS_DEBUGGER_CLEAR_FLAGS (JJS_DEBUGGER_CONTEXT_RESET_MODE);
          break;
        }

        /* Stop waiting for a new source file. */
        if ((context_p->debugger_flags & JJS_DEBUGGER_CLIENT_NO_SOURCE))
        {
          ret_type = JJS_DEBUGGER_SOURCE_END;
          JJS_DEBUGGER_CLEAR_FLAGS (JJS_DEBUGGER_CLIENT_SOURCE_MODE);
          break;
        }

        /* The source arrived. */
        if (!(context_p->debugger_flags & JJS_DEBUGGER_CLIENT_SOURCE_MODE))
        {
          JJS_ASSERT (client_source_data_p != NULL);

          jjs_char_t *source_name_p = (jjs_char_t *) (client_source_data_p + 1);
          size_t source_name_size = strlen ((const char *) source_name_p);

          *return_value = callback_p (source_name_p,
                                      source_name_size,
                                      source_name_p + source_name_size + 1,
                                      client_source_data_p->uint8_size - source_name_size - 1,
                                      user_p);

          ret_type = JJS_DEBUGGER_SOURCE_RECEIVED;
          break;
        }
      }

      jjs_debugger_transport_sleep ();
    }

    JJS_ASSERT (!(context_p->debugger_flags & JJS_DEBUGGER_CLIENT_SOURCE_MODE)
                  || !(context_p->debugger_flags & JJS_DEBUGGER_CONNECTED));

    if (client_source_data_p != NULL)
    {
      /* The data may partly arrived. */
      jmem_heap_free_block (client_source_data_p,
                            client_source_data_p->uint8_size + sizeof (jjs_debugger_uint8_data_t));
    }

    return ret_type;
  }

  return JJS_DEBUGGER_SOURCE_RECEIVE_FAILED;
#else /* !JJS_DEBUGGER */
  JJS_UNUSED_ALL (context_p, callback_p, user_p);
  return JJS_DEBUGGER_SOURCE_RECEIVE_FAILED;
#endif /* JJS_DEBUGGER */
} /* jjs_debugger_wait_for_client_source */

/**
 * Send the output of the program to the debugger client.
 * Currently only sends print output.
 */
void
jjs_debugger_send_output (jjs_context_t* context_p, /**< JJS context */
                          const jjs_char_t *buffer, /**< buffer */
                          jjs_size_t str_size) /**< string size */
{
#if JJS_DEBUGGER
  if (context_p->debugger_flags & JJS_DEBUGGER_CONNECTED)
  {
    jjs_debugger_send_string (JJS_DEBUGGER_OUTPUT_RESULT,
                                JJS_DEBUGGER_OUTPUT_PRINT,
                                (const uint8_t *) buffer,
                                sizeof (uint8_t) * str_size);
  }
#else /* !JJS_DEBUGGER */
  JJS_UNUSED_ALL (context_p, buffer, str_size);
#endif /* JJS_DEBUGGER */
} /* jjs_debugger_send_output */
