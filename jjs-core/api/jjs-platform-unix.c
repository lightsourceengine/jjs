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

#include "jjs-platform.h"
#include "jjs-compiler.h"

#ifdef JJS_OS_IS_UNIX

#include <string.h>

#include <unistd.h>
#ifdef JJS_OS_IS_MACOS
#include <sys/syslimits.h>
#else
#include <linux/limits.h>
#endif

jjs_platform_status_t
jjsp_cwd (jjs_platform_buffer_t* buffer_p)
{
  char buffer[PATH_MAX];
  char* path_p = getcwd (buffer, sizeof (buffer) / sizeof (buffer[0]));

  if (path_p == NULL)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  uint32_t path_len = (uint32_t) strlen (path_p);

  if (path_len == 0)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  // no trailing slash
  if (path_len > 1 && buffer[path_len - 1] == '/') {
    path_len -= 1;
  }

  char* result_p = jjsp_strndup (path_p, path_len);

  if (result_p == NULL)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  buffer_p->data_p = result_p;
  buffer_p->length = path_len;
  buffer_p->free = jjsp_buffer_free;
  buffer_p->encoding = JJS_PLATFORM_BUFFER_ENCODING_UTF8;

  return JJS_PLATFORM_STATUS_OK;
} /* jjsp_cwd */

#endif /* JJS_OS_IS_UNIX */
