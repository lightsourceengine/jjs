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

#include "jjs-ext/debugger.h"
#include "jext-common.h"

#if defined(JJS_DEBUGGER) && (JJS_DEBUGGER == 1)

/* A simplified transmission layer. */

/**
 * Size of the raw packet header.
 */
#define JJSX_DEBUGGER_RAWPACKET_HEADER_SIZE 1
/**
 * Maximum message size with 1 byte size field.
 */
#define JJSX_DEBUGGER_RAWPACKET_ONE_BYTE_LEN_MAX 255

/**
 * Header for incoming packets.
 */
typedef struct
{
  uint8_t size; /**< size of the message */
} jjsx_rawpacket_receive_header_t;

/**
 * Close a tcp connection.
 */
static void
jjsx_debugger_rp_close (jjs_debugger_transport_header_t *header_p) /**< header for the transport interface */
{
  JJSX_ASSERT (!jjs_debugger_transport_is_connected ());

  jjs_heap_free ((void *) header_p, sizeof (jjs_debugger_transport_header_t));
} /* jjsx_debugger_rp_close */

/**
 * Send data over a simple raw packet connection.
 *
 * @return true - if the data has been sent successfully
 *         false - otherwise
 */
static bool
jjsx_debugger_rp_send (jjs_debugger_transport_header_t *header_p, /**< header for the transport interface */
                         uint8_t *message_p, /**< message to be sent */
                         size_t message_length) /**< message length in bytes */
{
  JJSX_ASSERT (message_length <= JJSX_DEBUGGER_RAWPACKET_ONE_BYTE_LEN_MAX);

  message_p[-1] = (uint8_t) message_length;

  return header_p->next_p->send (header_p->next_p, message_p - 1, message_length + 1);
} /* jjsx_debugger_rp_send */

/**
 * Receive data from a rawpacket connection.
 *
 * @return true - if data has been received successfully
 *         false - otherwise
 */
static bool
jjsx_debugger_rp_receive (jjs_debugger_transport_header_t *header_p, /**< header for the transport interface */
                            jjs_debugger_transport_receive_context_t *receive_context_p) /**< receive context */
{
  if (!header_p->next_p->receive (header_p->next_p, receive_context_p))
  {
    return false;
  }

  if (receive_context_p->message_p == NULL)
  {
    return true;
  }

  size_t message_total_length = receive_context_p->message_total_length;

  if (message_total_length == 0)
  {
    /* Byte stream. */
    if (receive_context_p->message_length < sizeof (jjsx_rawpacket_receive_header_t))
    {
      receive_context_p->message_p = NULL;
      return true;
    }
  }
  else
  {
    /* Datagram packet. */
    JJSX_ASSERT (receive_context_p->message_length >= sizeof (jjsx_rawpacket_receive_header_t));
  }

  uint8_t *message_p = receive_context_p->message_p;
  size_t message_length = (size_t) (message_p[0]);

  if (message_total_length == 0)
  {
    size_t new_total_length = message_length + sizeof (jjsx_rawpacket_receive_header_t);

    /* Byte stream. */
    if (receive_context_p->message_length < new_total_length)
    {
      receive_context_p->message_p = NULL;
      return true;
    }

    receive_context_p->message_total_length = new_total_length;
  }
  else
  {
    /* Datagram packet. */
    JJSX_ASSERT (receive_context_p->message_length == (message_length + sizeof (jjsx_rawpacket_receive_header_t)));
  }

  receive_context_p->message_p = message_p + sizeof (jjsx_rawpacket_receive_header_t);
  receive_context_p->message_length = message_length;

  return true;
} /* jjsx_debugger_rp_receive */

/**
 * Initialize a simple raw packet transmission layer.
 *
 * @return true - if the connection succeeded
 *         false - otherwise
 */
bool
jjsx_debugger_rp_create (void)
{
  const jjs_size_t interface_size = sizeof (jjs_debugger_transport_header_t);
  jjs_debugger_transport_header_t *header_p;
  header_p = (jjs_debugger_transport_header_t *) jjs_heap_alloc (interface_size);

  if (!header_p)
  {
    return false;
  }

  header_p->close = jjsx_debugger_rp_close;
  header_p->send = jjsx_debugger_rp_send;
  header_p->receive = jjsx_debugger_rp_receive;

  jjs_debugger_transport_add (header_p,
                                JJSX_DEBUGGER_RAWPACKET_HEADER_SIZE,
                                JJSX_DEBUGGER_RAWPACKET_ONE_BYTE_LEN_MAX,
                                JJSX_DEBUGGER_RAWPACKET_HEADER_SIZE,
                                JJSX_DEBUGGER_RAWPACKET_ONE_BYTE_LEN_MAX);

  return true;
} /* jjsx_debugger_rp_create */

#else /* !(defined (JJS_DEBUGGER) && (JJS_DEBUGGER == 1)) */

/**
 * Dummy function when debugger is disabled.
 *
 * @return false
 */
bool
jjsx_debugger_rp_create (void)
{
  return false;
} /* jjsx_debugger_rp_create */

#endif /* defined (JJS_DEBUGGER) && (JJS_DEBUGGER == 1) */
