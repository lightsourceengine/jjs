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

#ifdef JJS_OS_IS_WINDOWS

#include <windows.h>

jjs_platform_status_t
jjsp_cwd (jjs_platform_buffer_t* buffer_p)
{
  WCHAR* p;
  DWORD n;
  DWORD t = GetCurrentDirectoryW (0, NULL);

  for (;;) {
    if (t == 0)
    {
      return JJS_PLATFORM_STATUS_ERR;
    }

    /* |t| is the size of the buffer _including_ nul. */
    p = malloc (t * sizeof (*p));
    if (p == NULL)
    {
      return JJS_PLATFORM_STATUS_ERR;
    }

    /* |n| is the size of the buffer _excluding_ nul but _only on success_.
     * If |t| was too small because another thread changed the working
     * directory, |n| is the size the buffer should be _including_ nul.
     * It therefore follows we must resize when n >= t and fail when n == 0.
     */
    n = GetCurrentDirectoryW (t, p);
    if (n > 0)
    {
      if (n < t)
      {
        break;
      }
    }

    free (p);
    t = n;
  }

  /* The returned directory should not have a trailing slash, unless it points
   * at a drive root, like c:\. Remove it if needed.
   */
  t = n - 1;
  if (p[t] == L'\\' && !(n == 3 && p[1] == L':'))
  {
    p[t] = L'\0';
    n = t;
  }

  buffer_p->data_p = p;
  buffer_p->length = n * sizeof (WCHAR);
  buffer_p->encoding = JJS_PLATFORM_BUFFER_ENCODING_UTF16;
  buffer_p->free = jjsp_buffer_free;

  return JJS_PLATFORM_STATUS_OK;
} /* jjsp_cwd */

#endif /* JJS_OS_IS_WINDOWS */
