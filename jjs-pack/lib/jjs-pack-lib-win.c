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

#include "jjs-pack-lib.h"

#if defined(JJS_OS_IS_WINDOWS)

#include <windows.h>

uint64_t
jjs_pack_platform_hrtime (jjs_context_t *context_p)
{
  // adapted from uv_hrtime(): https://github.com/libuv/libuv/src/win/util.c

  static double scaled_frequency = 0;

  if (scaled_frequency == 0)
  {
    LARGE_INTEGER frequency;

    if (QueryPerformanceFrequency (&frequency))
    {
      scaled_frequency = ((double) frequency.QuadPart) / 1e9;
    }
    else
    {
      jjs_log (JJS_LOG_LEVEL_ERROR, "hrtime: %s: %i\n", "QueryPerformanceFrequency", (int32_t) GetLastError());
      jjs_platform_fatal (context_p, JJS_FATAL_FAILED_ASSERTION);
      return 0;
    }
  }

  LARGE_INTEGER counter;

  if (!QueryPerformanceCounter (&counter))
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "hrtime: %s: %i\n", "QueryPerformanceCounter", (int32_t) GetLastError());
    jjs_platform_fatal (context_p, JJS_FATAL_FAILED_ASSERTION);
    return 0;
  }

  /* Because we have no guarantee about the order of magnitude of the
   * performance counter interval, integer math could cause this computation
   * to overflow. Therefore we resort to floating point math.
   */
  return (uint64_t) ((double) counter.QuadPart / scaled_frequency);
}

double
jjs_pack_platform_date_now (void)
{
  // Based on https://doxygen.postgresql.org/gettimeofday_8c_source.html
  static const uint64_t epoch = (uint64_t) 116444736000000000ULL;
  FILETIME file_time;
  ULARGE_INTEGER ularge;

  GetSystemTimeAsFileTime (&file_time);
  ularge.LowPart = file_time.dwLowDateTime;
  ularge.HighPart = file_time.dwHighDateTime;

  int64_t tv_sec = (int64_t) ((ularge.QuadPart - epoch) / 10000000L);
  int32_t tv_usec = (int32_t) (((ularge.QuadPart - epoch) % 10000000L) / 10L);

  return ((double) tv_sec) * 1000.0 + ((double) tv_usec) / 1000.0;
}

#endif /* JJS_OS_IS_WIN */
