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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jjs-port.h"

/**
 * Default implementation of jjs_port_print_byte. Uses 'putchar' to
 * print a single character to standard output.
 */
void JJS_ATTR_WEAK
jjs_port_print_byte (jjs_char_t byte) /**< the character to print */
{
  putchar (byte);
} /* jjs_port_print_byte */

/**
 * Default implementation of jjs_port_print_buffer. Uses 'jjs_port_print_byte' to
 * print characters of the input buffer.
 */
void JJS_ATTR_WEAK
jjs_port_print_buffer (const jjs_char_t *buffer_p, /**< string buffer */
                         jjs_size_t buffer_size) /**< string size*/
{
  for (jjs_size_t i = 0; i < buffer_size; i++)
  {
    jjs_port_print_byte (buffer_p[i]);
  }
} /* jjs_port_print_byte */

/**
 * Read a line from standard input as a zero-terminated string.
 *
 * @param out_size_p: length of the string
 *
 * @return pointer to the buffer storing the string,
 *         or NULL if end of input
 */
jjs_char_t *JJS_ATTR_WEAK
jjs_port_line_read (jjs_size_t *out_size_p)
{
  char *line_p = NULL;
  size_t allocated = 0;
  size_t bytes = 0;

  while (true)
  {
    allocated += 64;
    line_p = realloc (line_p, allocated);

    while (bytes < allocated - 1)
    {
      char ch = (char) fgetc (stdin);

      if (feof (stdin))
      {
        free (line_p);
        return NULL;
      }

      line_p[bytes++] = ch;

      if (ch == '\n')
      {
        *out_size_p = (jjs_size_t) bytes;
        line_p[bytes++] = '\0';
        return (jjs_char_t *) line_p;
      }
    }
  }
} /* jjs_port_line_read */

/**
 * Free a line buffer allocated by jjs_port_line_read
 *
 * @param buffer_p: buffer that has been allocated by jjs_port_line_read
 */
void JJS_ATTR_WEAK
jjs_port_line_free (jjs_char_t *buffer_p)
{
  free (buffer_p);
} /* jjs_port_line_free */
