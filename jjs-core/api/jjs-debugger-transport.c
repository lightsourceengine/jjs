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
#include "jjs-platform.h"

#if JJS_DEBUGGER

/**
 * Minimum number of bytes transmitted or received.
 */
#define JJS_DEBUGGER_TRANSPORT_MIN_BUFFER_SIZE 64

/**
 * Sleep time in milliseconds between each jjs_debugger_receive call
 */
#define JJS_DEBUGGER_TRANSPORT_TIMEOUT 100

/**
 * Add a new transport layer.
 */
void
jjs_debugger_transport_add (jjs_context_t* context_p, /**< JJS context */
                            jjs_debugger_transport_header_t *header_p, /**< transport implementation */
                            size_t send_message_header_size, /**< header bytes reserved for outgoing messages */
                            size_t max_send_message_size, /**< maximum number of bytes transmitted in a message */
                            size_t receive_message_header_size, /**< header bytes reserved for incoming messages */
                            size_t max_receive_message_size) /**< maximum number of bytes received in a message */
{
  JJS_ASSERT (max_send_message_size > JJS_DEBUGGER_TRANSPORT_MIN_BUFFER_SIZE
                && max_receive_message_size > JJS_DEBUGGER_TRANSPORT_MIN_BUFFER_SIZE);

  header_p->next_p = context_p->debugger_transport_header_p;
  context_p->debugger_transport_header_p = header_p;

  uint8_t *payload_p;
  size_t max_send_size;
  size_t max_receive_size;

  if (context_p->debugger_flags & JJS_DEBUGGER_CONNECTED)
  {
    payload_p = context_p->debugger_send_buffer_payload_p;
    max_send_size = context_p->debugger_max_send_size;
    max_receive_size = context_p->debugger_max_receive_size;
  }
  else
  {
    JJS_DEBUGGER_SET_FLAGS (JJS_DEBUGGER_CONNECTED);
    payload_p = context_p->debugger_send_buffer;
    max_send_size = JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE;
    max_receive_size = JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE;
  }

  JJS_ASSERT (max_send_size > JJS_DEBUGGER_TRANSPORT_MIN_BUFFER_SIZE + send_message_header_size);
  JJS_ASSERT (max_receive_size > JJS_DEBUGGER_TRANSPORT_MIN_BUFFER_SIZE + receive_message_header_size);

  context_p->debugger_send_buffer_payload_p = payload_p + send_message_header_size;

  max_send_size = max_send_size - send_message_header_size;
  max_receive_size = max_receive_size - receive_message_header_size;

  if (max_send_size > max_send_message_size)
  {
    max_send_size = max_send_message_size;
  }

  if (max_receive_size > max_receive_message_size)
  {
    max_receive_size = max_receive_message_size;
  }

  context_p->debugger_max_send_size = (uint8_t) max_send_size;
  context_p->debugger_max_receive_size = (uint8_t) max_receive_size;
} /* jjs_debugger_transport_add */

/**
 * Starts the communication to the debugger client.
 * Must be called after the connection is successfully established.
 */
void
jjs_debugger_transport_start (jjs_context_t* context_p) /**< JJS context */
{
  JJS_ASSERT (context_p->debugger_flags & JJS_DEBUGGER_CONNECTED);

  if (jjs_debugger_send_configuration (context_p, context_p->debugger_max_receive_size))
  {
    JJS_DEBUGGER_SET_FLAGS (JJS_DEBUGGER_VM_STOP);
    context_p->debugger_stop_context = NULL;
  }
} /* jjs_debugger_transport_start */

/**
 * Returns true if a debugger client is connected.
 *
 * @return true - a debugger client is connected,
 *         false - otherwise
 */
bool
jjs_debugger_transport_is_connected (jjs_context_t* context_p) /**< JJS context */
{
  return (context_p->debugger_flags & JJS_DEBUGGER_CONNECTED) != 0;
} /* jjs_debugger_transport_is_connected */

/**
 * Notifies the debugger server that the connection is closed.
 */
void
jjs_debugger_transport_close (jjs_context_t* context_p) /**< JJS context */
{
  if (!(context_p->debugger_flags & JJS_DEBUGGER_CONNECTED))
  {
    return;
  }

  context_p->debugger_flags = JJS_DEBUGGER_VM_IGNORE;

  jjs_debugger_transport_header_t *current_p = context_p->debugger_transport_header_p;

  JJS_ASSERT (current_p != NULL);

  do
  {
    jjs_debugger_transport_header_t *next_p = current_p->next_p;

    current_p->close (context_p, current_p);

    current_p = next_p;
  } while (current_p != NULL);

  jjs_log (context_p, JJS_LOG_LEVEL_DEBUG, "Debugger client connection closed.\n");

  jjs_debugger_free_unreferenced_byte_code (context_p);
} /* jjs_debugger_transport_close */

/**
 * Send data over the current connection
 *
 * @return true - data sent successfully,
 *         false - connection closed
 */
bool
jjs_debugger_transport_send (jjs_context_t* context_p, /**< JJS context */
                             const uint8_t *message_p, /**< message to be sent */
                             size_t message_length) /**< message length in bytes */
{
  JJS_ASSERT (jjs_debugger_transport_is_connected (context_p));
  JJS_ASSERT (message_length > 0);

  jjs_debugger_transport_header_t *header_p = context_p->debugger_transport_header_p;
  uint8_t *payload_p = context_p->debugger_send_buffer_payload_p;
  size_t max_send_size = context_p->debugger_max_send_size;

  do
  {
    size_t fragment_length = (message_length <= max_send_size ? message_length : max_send_size);

    memcpy (payload_p, message_p, fragment_length);

    if (!header_p->send (context_p, header_p, payload_p, fragment_length))
    {
      return false;
    }

    message_p += fragment_length;
    message_length -= fragment_length;
  } while (message_length > 0);

  return true;
} /* jjs_debugger_transport_send */

/**
 * Receive data from the current connection
 *
 * Note:
 *   A message is received if message_start_p is not NULL
 *
 * @return true - function successfully completed,
 *         false - connection closed
 */
bool
jjs_debugger_transport_receive (jjs_context_t* context_p, /**< JJS context */
                                jjs_debugger_transport_receive_context_t *receive_context_p) /**< [out] receive context */
{
  JJS_ASSERT (jjs_debugger_transport_is_connected (context_p));

  receive_context_p->buffer_p = context_p->debugger_receive_buffer;
  receive_context_p->received_length = context_p->debugger_received_length;
  receive_context_p->message_p = NULL;
  receive_context_p->message_length = 0;
  receive_context_p->message_total_length = 0;

  jjs_debugger_transport_header_t *header_p = context_p->debugger_transport_header_p;

  return header_p->receive (context_p, header_p, receive_context_p);
} /* jjs_debugger_transport_receive */

/**
 * Clear the message buffer after the message is processed
 */
void
jjs_debugger_transport_receive_completed (jjs_context_t* context_p, /**< JJS context */
                                          jjs_debugger_transport_receive_context_t *receive_context_p) /**< receive context */
{
  JJS_ASSERT (receive_context_p->message_p != NULL);
  JJS_ASSERT (receive_context_p->buffer_p == context_p->debugger_receive_buffer);

  size_t message_total_length = receive_context_p->message_total_length;
  size_t received_length = receive_context_p->received_length;

  JJS_ASSERT (message_total_length <= received_length);

  if (message_total_length == 0 || message_total_length == received_length)
  {
    /* All received data is processed. */
    context_p->debugger_received_length = 0;
    return;
  }

  uint8_t *buffer_p = receive_context_p->buffer_p;
  received_length -= message_total_length;

  memmove (buffer_p, buffer_p + message_total_length, received_length);

  context_p->debugger_received_length = (uint16_t) received_length;
} /* jjs_debugger_transport_receive_completed */

/**
 * Suspend execution for a predefined time (JJS_DEBUGGER_TRANSPORT_TIMEOUT ms).
 */
void
jjs_debugger_transport_sleep (jjs_context_t *context_p)
{
  JJS_ASSERT(context_p->platform_p->time_sleep != NULL);
  context_p->platform_p->time_sleep (JJS_DEBUGGER_TRANSPORT_TIMEOUT);
} /* jjs_debugger_transport_sleep */

#endif /* JJS_DEBUGGER */
