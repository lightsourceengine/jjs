/* Copyright Light Source Software, LLC and other contributors.
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

#include "cmdline.h"

#include <stdlib.h>
#include <stdio.h>

/**
 * Reads a line from stdin.
 *
 * The input data is read in as bytes. The caller will decode the bytes as
 * necessary. The end of line character is '\n'.
 *
 * @param buffer_size size of the read buffer. if 0, a default size is used.
 * @param out_size_p the size, in bytes of the returned buffer
 * @return null-terminated buffer of bytes with a size returned in out_size_p. the
 * caller must cleanup the buffer with free. on error, NULL is returned.
 */
jjs_char_t *
cmdline_stdin_readline (size_t buffer_size, jjs_size_t *out_size_p)
{
  // TODO: this code "works" for running all the tests, but it looks sussy. investigate.
  char *line_p = NULL;
  size_t allocated = 0;
  size_t bytes = 0;

  if (buffer_size == 0)
  {
    buffer_size = 256;
  }

  while (true)
  {
    allocated += buffer_size;
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
        line_p[bytes] = '\0';
        return (jjs_char_t *) line_p;
      }
    }
  }
}
