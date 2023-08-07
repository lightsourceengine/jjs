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

#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "jjs-debugger-transport.h"

#include "ecma-globals.h"

#if JJS_DEBUGGER

/* JJS debugger protocol is a simplified version of RFC-6455 (WebSockets). */

/**
 * Frequency of calling jjs_debugger_receive() by the VM.
 */
#define JJS_DEBUGGER_MESSAGE_FREQUENCY 5

/**
 * This constant represents that the string to be sent has no subtype.
 */
#define JJS_DEBUGGER_NO_SUBTYPE 0

/**
 * Limited resources available for the engine, so it is important to
 * check the maximum buffer size. It needs to be between 64 and 256 bytes.
 */
#if JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE < 64 || JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE > 256
#error Please define the MAX_BUFFER_SIZE between 64 and 256 bytes.
#endif /* JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE < 64 || JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE > 256 */

/**
 * Calculate the maximum number of items for a given type
 * which can be transmitted in one message.
 */
#define JJS_DEBUGGER_SEND_MAX(type) \
  ((size_t) ((JJS_CONTEXT (debugger_max_send_size) - sizeof (jjs_debugger_send_type_t)) / sizeof (type)))

/**
 * Calculate the size of a message when a count number of items transmitted.
 */
#define JJS_DEBUGGER_SEND_SIZE(count, type) ((size_t) ((count * sizeof (type)) + sizeof (jjs_debugger_send_type_t)))

/**
 * Debugger operation modes:
 *
 * The debugger has two operation modes: run mode and breakpoint mode.
 *
 * In run mode the debugger server accepts only a limited number of message
 * types from the debugger client (e.g. stop execution, set breakpoint).
 *
 * In breakpoint mode the JavaScript execution is stopped at a breakpoint and
 * more message types are accepted (e.g. get backtrace, evaluate expression).
 *
 * Switching between modes:
 *
 * When the JavaScript execution stops at a breakpoint the server sends a
 * JJS_DEBUGGER_BREAKPOINT_HIT message to the client. The client can only
 * issue breakpoint mode commands after this message is received.
 *
 * Certain breakpoint mode commands (e.g. continue) resumes the JavaScript
 * execution and the client must not send any breakpoint mode messages
 * until the JJS_DEBUGGER_BREAKPOINT_HIT is received again.
 *
 * The debugger server starts in run mode but stops at the first available
 * breakpoint.
 */

/**
 * Debugger option flags.
 */
typedef enum
{
  JJS_DEBUGGER_CONNECTED = 1u << 0, /**< debugger is connected */
  JJS_DEBUGGER_BREAKPOINT_MODE = 1u << 1, /**< debugger waiting at a breakpoint */
  JJS_DEBUGGER_VM_STOP = 1u << 2, /**< stop at the next breakpoint even if disabled */
  JJS_DEBUGGER_VM_IGNORE = 1u << 3, /**< ignore all breakpoints */
  JJS_DEBUGGER_VM_IGNORE_EXCEPTION = 1u << 4, /**< debugger doesn't stop at any exception */
  JJS_DEBUGGER_VM_EXCEPTION_THROWN = 1u << 5, /**< no need to stop for this exception */
  JJS_DEBUGGER_PARSER_WAIT = 1u << 6, /**< debugger should wait after parsing is completed */
  JJS_DEBUGGER_PARSER_WAIT_MODE = 1u << 7, /**< debugger is waiting after parsing is completed */
  JJS_DEBUGGER_CLIENT_SOURCE_MODE = 1u << 8, /**< debugger waiting for client code */
  JJS_DEBUGGER_CLIENT_NO_SOURCE = 1u << 9, /**< debugger leaving the client source loop */
  JJS_DEBUGGER_CONTEXT_RESET_MODE = 1u << 10, /**< debugger and engine reinitialization mode */
} jjs_debugger_flags_t;

/**
 * Set debugger flags.
 */
#define JJS_DEBUGGER_SET_FLAGS(flags) \
  JJS_CONTEXT (debugger_flags) = (JJS_CONTEXT (debugger_flags) | (uint32_t) (flags))

/**
 * Clear debugger flags.
 */
#define JJS_DEBUGGER_CLEAR_FLAGS(flags) \
  JJS_CONTEXT (debugger_flags) = (JJS_CONTEXT (debugger_flags) & (uint32_t) ~(flags))

/**
 * Set and clear debugger flags.
 */
#define JJS_DEBUGGER_UPDATE_FLAGS(flags_to_set, flags_to_clear) \
  JJS_CONTEXT (debugger_flags) =                                \
    ((JJS_CONTEXT (debugger_flags) | (uint32_t) (flags_to_set)) & (uint32_t) ~(flags_to_clear))

/**
 * Types for the package.
 */
typedef enum
{
  /* Messages sent by the server to client. */
  /* This is a handshake message, sent once during initialization. */
  JJS_DEBUGGER_CONFIGURATION = 1, /**< debugger configuration */
  /* These messages are sent by the parser. */
  JJS_DEBUGGER_PARSE_ERROR = 2, /**< parse error */
  JJS_DEBUGGER_BYTE_CODE_CP = 3, /**< byte code compressed pointer */
  JJS_DEBUGGER_PARSE_FUNCTION = 4, /**< parsing a new function */
  JJS_DEBUGGER_BREAKPOINT_LIST = 5, /**< list of line offsets */
  JJS_DEBUGGER_BREAKPOINT_OFFSET_LIST = 6, /**< list of byte code offsets */
  JJS_DEBUGGER_SOURCE_CODE = 7, /**< source code fragment */
  JJS_DEBUGGER_SOURCE_CODE_END = 8, /**< source code last fragment */
  JJS_DEBUGGER_SOURCE_CODE_NAME = 9, /**< source code name fragment */
  JJS_DEBUGGER_SOURCE_CODE_NAME_END = 10, /**< source code name last fragment */
  JJS_DEBUGGER_FUNCTION_NAME = 11, /**< function name fragment */
  JJS_DEBUGGER_FUNCTION_NAME_END = 12, /**< function name last fragment */
  JJS_DEBUGGER_WAITING_AFTER_PARSE = 13, /**< engine waiting for a parser resume */
  /* These messages are generic messages. */
  JJS_DEBUGGER_RELEASE_BYTE_CODE_CP = 14, /**< invalidate byte code compressed pointer */
  JJS_DEBUGGER_MEMSTATS_RECEIVE = 15, /**< memstats sent to the client */
  JJS_DEBUGGER_BREAKPOINT_HIT = 16, /**< notify breakpoint hit */
  JJS_DEBUGGER_EXCEPTION_HIT = 17, /**< notify exception hit */
  JJS_DEBUGGER_EXCEPTION_STR = 18, /**< exception string fragment */
  JJS_DEBUGGER_EXCEPTION_STR_END = 19, /**< exception string last fragment */
  JJS_DEBUGGER_BACKTRACE_TOTAL = 20, /**< number of total frames */
  JJS_DEBUGGER_BACKTRACE = 21, /**< backtrace data */
  JJS_DEBUGGER_BACKTRACE_END = 22, /**< last backtrace data */
  JJS_DEBUGGER_EVAL_RESULT = 23, /**< eval result */
  JJS_DEBUGGER_EVAL_RESULT_END = 24, /**< last part of eval result */
  JJS_DEBUGGER_WAIT_FOR_SOURCE = 25, /**< engine waiting for source code */
  JJS_DEBUGGER_OUTPUT_RESULT = 26, /**< output sent by the program to the debugger */
  JJS_DEBUGGER_OUTPUT_RESULT_END = 27, /**< last output result data */
  JJS_DEBUGGER_SCOPE_CHAIN = 28, /**< scope chain */
  JJS_DEBUGGER_SCOPE_CHAIN_END = 29, /**< last output of scope chain */
  JJS_DEBUGGER_SCOPE_VARIABLES = 30, /**< scope variables */
  JJS_DEBUGGER_SCOPE_VARIABLES_END = 31, /**< last output of scope variables */
  JJS_DEBUGGER_CLOSE_CONNECTION = 32, /**< close connection with the client */
  JJS_DEBUGGER_MESSAGES_OUT_MAX_COUNT, /**< number of different type of output messages by the debugger */

  /* Messages sent by the client to server. */

  /* The following messages are accepted in both run and breakpoint modes. */
  JJS_DEBUGGER_FREE_BYTE_CODE_CP = 1, /**< free byte code compressed pointer */
  JJS_DEBUGGER_UPDATE_BREAKPOINT = 2, /**< update breakpoint status */
  JJS_DEBUGGER_EXCEPTION_CONFIG = 3, /**< exception handler config */
  JJS_DEBUGGER_PARSER_CONFIG = 4, /**< parser config */
  JJS_DEBUGGER_MEMSTATS = 5, /**< list memory statistics */
  JJS_DEBUGGER_STOP = 6, /**< stop execution */
  /* The following message is only available in waiting after parse mode. */
  JJS_DEBUGGER_PARSER_RESUME = 7, /**< stop waiting after parse */
  /* The following four messages are only available in client switch mode. */
  JJS_DEBUGGER_CLIENT_SOURCE = 8, /**< first message of client source */
  JJS_DEBUGGER_CLIENT_SOURCE_PART = 9, /**< next message of client source */
  JJS_DEBUGGER_NO_MORE_SOURCES = 10, /**< no more sources notification */
  JJS_DEBUGGER_CONTEXT_RESET = 11, /**< context reset request */
  /* The following messages are only available in breakpoint
   * mode and they switch the engine to run mode. */
  JJS_DEBUGGER_CONTINUE = 12, /**< continue execution */
  JJS_DEBUGGER_STEP = 13, /**< next breakpoint, step into functions */
  JJS_DEBUGGER_NEXT = 14, /**< next breakpoint in the same context */
  JJS_DEBUGGER_FINISH = 15, /**< Continue running just after the function in the current stack frame returns */
  /* The following messages are only available in breakpoint
   * mode and this mode is kept after the message is processed. */
  JJS_DEBUGGER_GET_BACKTRACE = 16, /**< get backtrace */
  JJS_DEBUGGER_EVAL = 17, /**< first message of evaluating a string */
  JJS_DEBUGGER_EVAL_PART = 18, /**< next message of evaluating a string */
  JJS_DEBUGGER_GET_SCOPE_CHAIN = 19, /**< get type names of the scope chain */
  JJS_DEBUGGER_GET_SCOPE_VARIABLES = 20, /**< get variables of a scope */
  JJS_DEBUGGER_MESSAGES_IN_MAX_COUNT, /**< number of different type of input messages */
} jjs_debugger_header_type_t;

/**
 * Debugger option flags.
 */
typedef enum
{
  JJS_DEBUGGER_LITTLE_ENDIAN = 1u << 0, /**< little endian */
} jjs_debugger_configuration_flags_t;

/**
 * Subtypes of eval.
 */
typedef enum
{
  JJS_DEBUGGER_EVAL_EVAL = 0, /**< evaluate expression */
  JJS_DEBUGGER_EVAL_THROW = 1, /**< evaluate expression and throw the result */
  JJS_DEBUGGER_EVAL_ABORT = 2, /**< evaluate expression and abrot with the result */
} jjs_debugger_eval_type_t;

/**
 * Subtypes of eval_result.
 */
typedef enum
{
  JJS_DEBUGGER_EVAL_OK = 1, /**< eval result, no error */
  JJS_DEBUGGER_EVAL_ERROR = 2, /**< eval result when an error has occurred */
} jjs_debugger_eval_result_type_t;

/**
 * Subtypes of output_result.
 *
 * Note:
 *      This enum has to be kept in sync with jjs_log_level_t with an offset
 *      of +2.
 */
typedef enum
{
  JJS_DEBUGGER_OUTPUT_PRINT = 1, /**< printed output */
  JJS_DEBUGGER_OUTPUT_LOG = 2, /**< logged output */
} jjs_debugger_output_subtype_t;

/**
 * Types of scopes.
 */
typedef enum
{
  JJS_DEBUGGER_SCOPE_WITH = 1, /**< with */
  JJS_DEBUGGER_SCOPE_LOCAL = 2, /**< local */
  JJS_DEBUGGER_SCOPE_CLOSURE = 3, /**< closure */
  JJS_DEBUGGER_SCOPE_GLOBAL = 4, /**< global */
  JJS_DEBUGGER_SCOPE_NON_CLOSURE = 5 /**< non closure */
} jjs_debugger_scope_chain_type_t;

/**
 * Type of scope variables.
 */
typedef enum
{
  JJS_DEBUGGER_VALUE_NONE = 1,
  JJS_DEBUGGER_VALUE_UNDEFINED = 2,
  JJS_DEBUGGER_VALUE_NULL = 3,
  JJS_DEBUGGER_VALUE_BOOLEAN = 4,
  JJS_DEBUGGER_VALUE_NUMBER = 5,
  JJS_DEBUGGER_VALUE_STRING = 6,
  JJS_DEBUGGER_VALUE_FUNCTION = 7,
  JJS_DEBUGGER_VALUE_ARRAY = 8,
  JJS_DEBUGGER_VALUE_OBJECT = 9
} jjs_debugger_scope_variable_type_t;

/**
 * Byte data for evaluating expressions and receiving client source.
 */
typedef struct
{
  uint32_t uint8_size; /**< total size of the client source */
  uint32_t uint8_offset; /**< current offset in the client source */
} jjs_debugger_uint8_data_t;

/**
 * Delayed free of byte code data.
 */
typedef struct
{
  uint16_t size; /**< size of the byte code header divided by JMEM_ALIGNMENT */
  jmem_cpointer_t prev_cp; /**< previous byte code data to be freed */
} jjs_debugger_byte_code_free_t;

/**
 * Outgoing message: JJS configuration.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t configuration; /**< configuration option bits */
  uint8_t version[sizeof (uint32_t)]; /**< debugger version */
  uint8_t max_message_size; /**< maximum incoming message size */
  uint8_t cpointer_size; /**< size of compressed pointers */
} jjs_debugger_send_configuration_t;

/**
 * Outgoing message: message without arguments.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
} jjs_debugger_send_type_t;

/**
 * Incoming message: message without arguments.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
} jjs_debugger_receive_type_t;

/**
 * Outgoing message: string (Source file name or function name).
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t string[]; /**< string data */
} jjs_debugger_send_string_t;

/**
 * Outgoing message: uint32 value.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t line[sizeof (uint32_t)]; /**< value data */
  uint8_t column[sizeof (uint32_t)]; /**< value data */
} jjs_debugger_send_parse_function_t;

/**
 * Outgoing message: byte code compressed pointer.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t byte_code_cp[sizeof (jmem_cpointer_t)]; /**< byte code compressed pointer */
} jjs_debugger_send_byte_code_cp_t;

/**
 * Incoming message: byte code compressed pointer.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t byte_code_cp[sizeof (jmem_cpointer_t)]; /**< byte code compressed pointer */
} jjs_debugger_receive_byte_code_cp_t;

/**
 * Incoming message: update (enable/disable) breakpoint status.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t is_set_breakpoint; /**< set or clear breakpoint */
  uint8_t byte_code_cp[sizeof (jmem_cpointer_t)]; /**< byte code compressed pointer */
  uint8_t offset[sizeof (uint32_t)]; /**< breakpoint offset */
} jjs_debugger_receive_update_breakpoint_t;

/**
 * Outgoing message: send memory statistics
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t allocated_bytes[sizeof (uint32_t)]; /**< allocated bytes */
  uint8_t byte_code_bytes[sizeof (uint32_t)]; /**< byte code bytes */
  uint8_t string_bytes[sizeof (uint32_t)]; /**< string bytes */
  uint8_t object_bytes[sizeof (uint32_t)]; /**< object bytes */
  uint8_t property_bytes[sizeof (uint32_t)]; /**< property bytes */
} jjs_debugger_send_memstats_t;

/**
 * Outgoing message: notify breakpoint hit.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t byte_code_cp[sizeof (jmem_cpointer_t)]; /**< byte code compressed pointer */
  uint8_t offset[sizeof (uint32_t)]; /**< breakpoint offset */
} jjs_debugger_send_breakpoint_hit_t;

/**
 * Stack frame descriptor for sending backtrace information.
 */
typedef struct
{
  uint8_t byte_code_cp[sizeof (jmem_cpointer_t)]; /**< byte code compressed pointer */
  uint8_t offset[sizeof (uint32_t)]; /**< last breakpoint offset */
} jjs_debugger_frame_t;

/**
 * Outgoing message: backtrace information.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  jjs_debugger_frame_t frames[]; /**< frames */
} jjs_debugger_send_backtrace_t;

/**
 * Outgoing message: scope chain.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t scope_types[]; /**< scope types */
} jjs_debugger_send_scope_chain_t;

/**
 * Outgoing message: number of total frames in backtrace.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t frame_count[sizeof (uint32_t)]; /**< total number of frames */
} jjs_debugger_send_backtrace_total_t;

/**
 * Incoming message: set behaviour when exception occures.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t enable; /**< non-zero: enable stop at exception */
} jjs_debugger_receive_exception_config_t;

/**
 * Incoming message: set parser configuration.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t enable_wait; /**< non-zero: wait after parsing is completed */
} jjs_debugger_receive_parser_config_t;

/**
 * Incoming message: get backtrace.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t min_depth[sizeof (uint32_t)]; /**< minimum depth*/
  uint8_t max_depth[sizeof (uint32_t)]; /**< maximum depth (0 - unlimited) */
  uint8_t get_total_frame_count; /**< non-zero: if total frame count is also requested */
} jjs_debugger_receive_get_backtrace_t;

/**
 * Incoming message: first message of evaluating expression.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t eval_size[sizeof (uint32_t)]; /**< total size of the message */
} jjs_debugger_receive_eval_first_t;

/**
 * Incoming message: get scope variables
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t chain_index[sizeof (uint32_t)]; /**< index element of the scope */
} jjs_debugger_receive_get_scope_variables_t;

/**
 * Incoming message: first message of client source.
 */
typedef struct
{
  uint8_t type; /**< type of the message */
  uint8_t code_size[sizeof (uint32_t)]; /**< total size of the message */
} jjs_debugger_receive_client_source_first_t;

void jjs_debugger_free_unreferenced_byte_code (void);

bool jjs_debugger_receive (jjs_debugger_uint8_data_t **message_data_p);

void jjs_debugger_breakpoint_hit (uint8_t message_type);

void jjs_debugger_send_type (jjs_debugger_header_type_t type);
bool jjs_debugger_send_configuration (uint8_t max_message_size);
void jjs_debugger_send_data (jjs_debugger_header_type_t type, const void *data, size_t size);
bool jjs_debugger_send_string (uint8_t message_type, uint8_t sub_type, const uint8_t *string_p, size_t string_length);
bool jjs_debugger_send_function_cp (jjs_debugger_header_type_t type, ecma_compiled_code_t *compiled_code_p);
bool jjs_debugger_send_parse_function (uint32_t line, uint32_t column);
void jjs_debugger_send_memstats (void);
bool jjs_debugger_send_exception_string (ecma_value_t exception_value);

#endif /* JJS_DEBUGGER */

#endif /* !DEBUGGER_H */
