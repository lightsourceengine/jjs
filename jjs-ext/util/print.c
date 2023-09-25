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

#include "jjs-ext/print.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "jjs-core.h"
#include "jjs-debugger.h"
#include "jjs-port.h"

/**
 * Print buffer size
 */
#define JJSX_PRINT_BUFFER_SIZE 128

/**
 * Max line size that will be printed on a Syntax Error
 */
#define JJSX_SYNTAX_ERROR_MAX_LINE_LENGTH 512

/**
 * Struct for buffering print outpu
 */
typedef struct
{
  jjs_size_t index; /**< write index */
  jjs_char_t data[JJSX_PRINT_BUFFER_SIZE]; /**< buffer data */
} jjsx_print_buffer_t;

/**
 * Callback used by jjsx_print_value to batch written characters and print them in bulk.
 * NULL bytes are escaped and written as '\u0000'.
 *
 * @param value:  encoded byte value
 * @param user_p: user pointer
 */
static void
jjsx_buffered_print (uint32_t value, void *user_p)
{
  jjsx_print_buffer_t *buffer_p = (jjsx_print_buffer_t *) user_p;

  if (value == '\0')
  {
    jjsx_print_buffer (buffer_p->data, buffer_p->index);
    buffer_p->index = 0;

    jjsx_print_string ("\\u0000");
    return;
  }

  assert (value <= UINT8_MAX);
  buffer_p->data[buffer_p->index++] = (uint8_t) value;

  if (buffer_p->index >= JJSX_PRINT_BUFFER_SIZE)
  {
    jjsx_print_buffer (buffer_p->data, buffer_p->index);
    buffer_p->index = 0;
  }
} /* jjsx_buffered_print */

/**
 * Convert a value to string and print it to standard output.
 * NULL characters are escaped to "\u0000", other characters are output as-is.
 *
 * @param value: input value
 */
jjs_value_t
jjsx_print_value (const jjs_value_t value)
{
  jjs_value_t string;

  if (jjs_value_is_symbol (value))
  {
    string = jjs_symbol_descriptive_string (value);
  }
  else
  {
    string = jjs_value_to_string (value);

    if (jjs_value_is_exception (string))
    {
      return string;
    }
  }

  jjsx_print_buffer_t buffer;
  buffer.index = 0;

  jjs_string_iterate (string, JJS_ENCODING_UTF8, &jjsx_buffered_print, &buffer);
  jjs_value_free (string);

  jjsx_print_buffer (buffer.data, buffer.index);

  return jjs_undefined ();
} /* jjsx_print */

/**
 * Print a character to standard output, also sending it to the debugger, if connected.
 *
 * @param ch: input character
 */
void
jjsx_print_byte (jjs_char_t byte)
{
  jjs_port_print_byte (byte);
#if JJS_DEBUGGER
  jjs_debugger_send_output (&byte, 1);
#endif /* JJS_DEBUGGER */
} /* jjsx_print_char */

/**
 * Print a buffer to standard output, also sending it to the debugger, if connected.
 *
 * @param buffer_p: inptut string buffer
 * @param buffer_size: size of the string
 */
void
jjsx_print_buffer (const jjs_char_t *buffer_p, jjs_size_t buffer_size)
{
  jjs_port_print_buffer (buffer_p, buffer_size);
#if JJS_DEBUGGER
  jjs_debugger_send_output (buffer_p, buffer_size);
#endif /* JJS_DEBUGGER */
} /* jjsx_print_buffer */

/**
 * Print a zero-terminated string to standard output, also sending it to the debugger, if connected.
 *
 * @param buffer_p: inptut string buffer
 * @param buffer_size: size of the string
 */
void
jjsx_print_string (const char *str_p)
{
  const jjs_char_t *buffer_p = (jjs_char_t *) str_p;
  jjs_size_t buffer_size = (jjs_size_t) (strlen (str_p));

  jjs_port_print_buffer (buffer_p, buffer_size);
#if JJS_DEBUGGER
  jjs_debugger_send_output (buffer_p, buffer_size);
#endif /* JJS_DEBUGGER */
} /* jjsx_print_string */

/**
 * Print backtrace as log messages up to a specific depth.
 *
 * @param depth: backtrace depth
 */
void
jjsx_print_backtrace (unsigned depth)
{
  if (!jjs_feature_enabled (JJS_FEATURE_LINE_INFO))
  {
    return;
  }

  jjs_log (JJS_LOG_LEVEL_ERROR, "Script backtrace (top %u):\n", depth);

  jjs_value_t backtrace_array = jjs_backtrace (depth);
  unsigned array_length = jjs_array_length (backtrace_array);

  for (unsigned idx = 0; idx < array_length; idx++)
  {
    jjs_value_t property = jjs_object_get_index (backtrace_array, idx);

    jjs_char_t buffer[JJSX_PRINT_BUFFER_SIZE];

    jjs_size_t copied = jjs_string_to_buffer (property, JJS_ENCODING_UTF8, buffer, JJSX_PRINT_BUFFER_SIZE - 1);
    buffer[copied] = '\0';

    jjs_log (JJS_LOG_LEVEL_ERROR, " %u: %s\n", idx, buffer);
    jjs_value_free (property);
  }

  jjs_value_free (backtrace_array);
} /* jjsx_handler_assert_fatal */

/**
 * Print an unhandled exception value
 *
 * The function will take ownership of the value, and free it.
 *
 * @param exception: unhandled exception value
 */
void
jjsx_print_unhandled_exception (jjs_value_t exception) /**< exception value */
{
  assert (jjs_value_is_exception (exception));
  jjs_value_t value = jjs_exception_value (exception, true);

  JJS_VLA (jjs_char_t, buffer_p, JJSX_PRINT_BUFFER_SIZE);

  jjs_value_t string = jjs_value_to_string (value);

  jjs_size_t copied = jjs_string_to_buffer (string, JJS_ENCODING_UTF8, buffer_p, JJSX_PRINT_BUFFER_SIZE - 1);
  buffer_p[copied] = '\0';

  if (jjs_feature_enabled (JJS_FEATURE_ERROR_MESSAGES) && jjs_error_type (value) == JJS_ERROR_SYNTAX)
  {
    jjs_char_t *string_end_p = buffer_p + copied;
    jjs_size_t err_line = 0;
    jjs_size_t err_col = 0;
    char *path_str_p = NULL;
    char *path_str_end_p = NULL;

    /* 1. parse column and line information */
    for (jjs_char_t *current_p = buffer_p; current_p < string_end_p; current_p++)
    {
      if (*current_p == '[')
      {
        current_p++;

        if (*current_p == '<')
        {
          break;
        }

        path_str_p = (char *) current_p;
        while (current_p < string_end_p && *current_p != ':')
        {
          current_p++;
        }

        path_str_end_p = (char *) current_p;

        if (current_p == string_end_p)
        {
          break;
        }

        err_line = (unsigned int) strtol ((char *) current_p + 1, (char **) &current_p, 10);

        if (current_p == string_end_p)
        {
          break;
        }

        err_col = (unsigned int) strtol ((char *) current_p + 1, NULL, 10);
        break;
      }
    } /* for */

    if (err_line > 0 && err_col > 0 && err_col < JJSX_SYNTAX_ERROR_MAX_LINE_LENGTH)
    {
      /* Temporarily modify the error message, so we can use the path. */
      *path_str_end_p = '\0';

      jjs_size_t source_size;
      jjs_char_t *source_p = jjs_port_source_read (path_str_p, &source_size);

      /* Revert the error message. */
      *path_str_end_p = ':';

      if (source_p != NULL)
      {
        uint32_t curr_line = 1;
        jjs_size_t pos = 0;

        /* 2. seek and print */
        while (pos < source_size && curr_line < err_line)
        {
          if (source_p[pos] == '\n')
          {
            curr_line++;
          }

          pos++;
        }

        /* Print character if:
         * - The max line length is not reached.
         * - The current position is valid (it is not the end of the source).
         * - The current character is not a newline.
         **/
        for (uint32_t char_count = 0;
             (char_count < JJSX_SYNTAX_ERROR_MAX_LINE_LENGTH) && (pos < source_size) && (source_p[pos] != '\n');
             char_count++, pos++)
        {
          jjs_log (JJS_LOG_LEVEL_ERROR, "%c", source_p[pos]);
        }

        jjs_log (JJS_LOG_LEVEL_ERROR, "\n");
        jjs_port_source_free (source_p);

        while (--err_col)
        {
          jjs_log (JJS_LOG_LEVEL_ERROR, "~");
        }

        jjs_log (JJS_LOG_LEVEL_ERROR, "^\n\n");
      }
    }
  }

  jjs_log (JJS_LOG_LEVEL_ERROR, "Unhandled exception: %s\n", buffer_p);
  jjs_value_free (string);

  if (jjs_value_is_object (value))
  {
    jjs_value_t backtrace_val = jjs_object_get_sz (value, "stack");

    if (jjs_value_is_array (backtrace_val))
    {
      uint32_t length = jjs_array_length (backtrace_val);

      /* This length should be enough. */
      if (length > 32)
      {
        length = 32;
      }

      for (unsigned i = 0; i < length; i++)
      {
        jjs_value_t item_val = jjs_object_get_index (backtrace_val, i);

        if (jjs_value_is_string (item_val))
        {
          copied = jjs_string_to_buffer (item_val, JJS_ENCODING_UTF8, buffer_p, JJSX_PRINT_BUFFER_SIZE - 1);
          buffer_p[copied] = '\0';

          jjs_log (JJS_LOG_LEVEL_ERROR, " %u: %s\n", i, buffer_p);
        }

        jjs_value_free (item_val);
      }
    }

    jjs_value_free (backtrace_val);
  }

  jjs_value_free (value);
} /* jjsx_print_unhandled_exception */

/**
 * Print unhandled promise rejection.
 *
 * @param result: promise rejection result
 */
void
jjsx_print_unhandled_rejection (jjs_value_t result) /**< result value */
{
  jjs_value_t reason = jjs_value_to_string (result);

  if (!jjs_value_is_exception (reason))
  {
    JJS_VLA (jjs_char_t, buffer_p, JJSX_PRINT_BUFFER_SIZE);
    jjs_size_t copied = jjs_string_to_buffer (reason, JJS_ENCODING_UTF8, buffer_p, JJSX_PRINT_BUFFER_SIZE - 1);
    buffer_p[copied] = '\0';

    jjs_log (JJS_LOG_LEVEL_WARNING, "Uncaught Promise rejection: %s\n", buffer_p);
  }
  else
  {
    jjs_log (JJS_LOG_LEVEL_WARNING, "Uncaught Promise rejection: (reason cannot be converted to string)\n");
  }

  jjs_value_free (reason);
} /* jjsx_print_unhandled_rejection */
