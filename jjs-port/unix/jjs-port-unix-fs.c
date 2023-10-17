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

#if defined(__unix__) || defined(__APPLE__)

#include <stdlib.h>
#include <string.h>
#include <libgen.h>

/**
 * Normalize a file path using realpath.
 *
 * @param path_p: input path
 * @param path_size: input path size
 *
 * @return a newly allocated buffer with the normalized path if the operation is successful,
 *         NULL otherwise
 */
jjs_char_t *
jjs_port_path_normalize (const jjs_char_t *path_p, /**< input path */
                           jjs_size_t path_size) /**< size of the path */
{
  (void) path_size;

  return (jjs_char_t *) realpath ((char *) path_p, NULL);
} /* jjs_port_path_normalize */

jjs_char_t *jjs_port_path_dirname (char* path_p, jjs_size_t* dirname_size_p)
{
  char* p;

  if (path_p == NULL || *path_p == '\0') {
    p = ".";
  } else {
    p = dirname (path_p);
  }

  size_t len = strlen (p);

  if (dirname_size_p)
  {
    *dirname_size_p = (jjs_size_t) len;
  }

  return (jjs_char_t *)strcpy (malloc(len + 1), p);
} /* jjs_port_path_dirname */

/**
 * Free a path buffer returned by jjs_port_path_normalize.
 */
void
jjs_port_path_free (jjs_char_t *path_p)
{
  free (path_p);
} /* jjs_port_path_free */

/**
 * Computes the end of the directory part of a path.
 *
 * @return end of the directory part of a path.
 */
jjs_size_t JJS_ATTR_WEAK
jjs_port_path_base (const jjs_char_t *path_p) /**< path */
{
  const jjs_char_t *basename_p = (jjs_char_t *) strrchr ((char *) path_p, '/') + 1;

  return (jjs_size_t) (basename_p - path_p);
} /* jjs_port_get_directory_end */

#endif /* defined(__unix__) || defined(__APPLE__) */
