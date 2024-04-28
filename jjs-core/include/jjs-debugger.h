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

#ifndef JJS_DEBUGGER_H
#define JJS_DEBUGGER_H

#include "jjs-types.h"

JJS_C_API_BEGIN

/** \addtogroup jjs-debugger JJS engine interface - Debugger feature
 * @{
 */

/**
 * JJS debugger protocol version.
 */
#define JJS_DEBUGGER_VERSION (9)

/**
 * Types for the client source wait and run method.
 */
typedef enum
{
  JJS_DEBUGGER_SOURCE_RECEIVE_FAILED = 0, /**< source is not received */
  JJS_DEBUGGER_SOURCE_RECEIVED = 1, /**< a source has been received */
  JJS_DEBUGGER_SOURCE_END = 2, /**< the end of the sources signal received */
  JJS_DEBUGGER_CONTEXT_RESET_RECEIVED, /**< the context reset request has been received */
} jjs_debugger_wait_for_source_status_t;

/**
 * Callback for jjs_debugger_wait_and_run_client_source
 *
 * The callback receives the source name, source code and a user pointer.
 *
 * @return this value is passed back by jjs_debugger_wait_and_run_client_source
 */
typedef jjs_value_t (*jjs_debugger_wait_for_source_callback_t) (const jjs_char_t *source_name_p,
                                                                    size_t source_name_size,
                                                                    const jjs_char_t *source_p,
                                                                    size_t source_size,
                                                                    void *user_p);

/**
 * Engine debugger functions.
 */
bool jjs_debugger_is_connected (jjs_context_t* context_p);
void jjs_debugger_stop (jjs_context_t* context_p);
void jjs_debugger_continue (jjs_context_t* context_p);
void jjs_debugger_stop_at_breakpoint (jjs_context_t* context_p, bool enable_stop_at_breakpoint);
jjs_debugger_wait_for_source_status_t
jjs_debugger_wait_for_client_source (jjs_context_t* context_p,
                                     jjs_debugger_wait_for_source_callback_t callback_p,
                                     void *user_p,
                                     jjs_value_t *return_value);
void jjs_debugger_send_output (jjs_context_t* context_p, const jjs_char_t *buffer, jjs_size_t str_size);

/**
 * @}
 */

JJS_C_API_END

#endif /* !JJS_DEBUGGER_H */
