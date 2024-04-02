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
#include <stdio.h>

#if defined(__APPLE__)
#include <sys/syslimits.h>
#endif /* defined(__APPLE__) */

#if defined(__unix__)
#include <linux/limits.h>
#endif /* defined(__unix__) */

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

/**
 * Free a path buffer returned by jjs_port_path_normalize.
 */
void
jjs_port_path_free (jjs_char_t *path_p)
{
  free (path_p);
} /* jjs_port_path_free */

#endif /* defined(__unix__) || defined(__APPLE__) */
