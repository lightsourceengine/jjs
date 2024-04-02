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

#include "jjs-port.h"

#if defined(_WIN32)

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#define F_OK 0

/**
 * Normalize a file path.
 *
 * @return a newly allocated buffer with the normalized path if the operation is successful,
 *         NULL otherwise
 */
jjs_char_t *
jjs_port_path_normalize (const jjs_char_t *path_p, /**< input path */
                           jjs_size_t path_size) /**< size of the path */
{
  (void) path_size;

  jjs_char_t * p = (jjs_char_t *) _fullpath (NULL, (const char*) path_p, _MAX_PATH);

  if (p && _access ((const char*)p, F_OK) == 0)
  {
    return p;
  }

  jjs_port_path_free (p);

  return NULL;
} /* jjs_port_path_normalize */

/**
 * Free a path buffer returned by jjs_port_path_normalize.
 */
void
jjs_port_path_free (jjs_char_t *path_p)
{
  free (path_p);
} /* jjs_port_path_free */

#endif /* defined(_WIN32) */
