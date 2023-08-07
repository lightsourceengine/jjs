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

#ifndef JJS_DEBUGGER_TRANSPORT_H
#define JJS_DEBUGGER_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "jjs-compiler.h"

JJS_C_API_BEGIN

/** \addtogroup jjs-debugger-transport JJS engine debugger interface - transport control
 * @{
 */

/**
 * Maximum number of bytes transmitted or received.
 */
#define JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE 128

/**
 * Receive message context.
 */
typedef struct
{
  uint8_t *buffer_p; /**< buffer for storing the received data */
  size_t received_length; /**< number of currently received bytes */
  uint8_t *message_p; /**< start of the received message */
  size_t message_length; /**< length of the received message */
  size_t message_total_length; /**< total length for datagram protocols,
                                *   0 for stream protocols */
} jjs_debugger_transport_receive_context_t;

/**
 * Forward definition of jjs_debugger_transport_header_t.
 */
struct jjs_debugger_transport_interface_t;

/**
 * Close connection callback.
 */
typedef void (*jjs_debugger_transport_close_t) (struct jjs_debugger_transport_interface_t *header_p);

/**
 * Send data callback.
 */
typedef bool (*jjs_debugger_transport_send_t) (struct jjs_debugger_transport_interface_t *header_p,
                                                 uint8_t *message_p,
                                                 size_t message_length);

/**
 * Receive data callback.
 */
typedef bool (*jjs_debugger_transport_receive_t) (struct jjs_debugger_transport_interface_t *header_p,
                                                    jjs_debugger_transport_receive_context_t *context_p);

/**
 * Transport layer header.
 */
typedef struct jjs_debugger_transport_interface_t
{
  /* The following fields must be filled before calling jjs_debugger_transport_add(). */
  jjs_debugger_transport_close_t close; /**< close connection callback */
  jjs_debugger_transport_send_t send; /**< send data callback */
  jjs_debugger_transport_receive_t receive; /**< receive data callback */

  /* The following fields are filled by jjs_debugger_transport_add(). */
  struct jjs_debugger_transport_interface_t *next_p; /**< next transport layer */
} jjs_debugger_transport_header_t;

void jjs_debugger_transport_add (jjs_debugger_transport_header_t *header_p,
                                   size_t send_message_header_size,
                                   size_t max_send_message_size,
                                   size_t receive_message_header_size,
                                   size_t max_receive_message_size);
void jjs_debugger_transport_start (void);

bool jjs_debugger_transport_is_connected (void);
void jjs_debugger_transport_close (void);

bool jjs_debugger_transport_send (const uint8_t *message_p, size_t message_length);
bool jjs_debugger_transport_receive (jjs_debugger_transport_receive_context_t *context_p);
void jjs_debugger_transport_receive_completed (jjs_debugger_transport_receive_context_t *context_p);

void jjs_debugger_transport_sleep (void);

/**
 * @}
 */

JJS_C_API_END

#endif /* !JJS_DEBUGGER_TRANSPORT_H */
