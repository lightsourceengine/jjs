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
}

#include <windows.h>

void
jjsp_time_sleep (uint32_t sleep_time_ms) /**< milliseconds to sleep */
{
  Sleep (sleep_time_ms);
}

#define UNIX_EPOCH_IN_TICKS 116444736000000000ull /* difference between 1970 and 1601 */
#define TICKS_PER_MS        10000ull /* 1 tick is 100 nanoseconds */

static void
unix_time_to_filetime (double t, LPFILETIME ft_p)
{
  // https://support.microsoft.com/en-us/help/167296/how-to-convert-a-unix-time-t-to-a-win32-filetime-or-systemtime

  LONGLONG ll = (LONGLONG) t * (LONGLONG) TICKS_PER_MS + (LONGLONG) UNIX_EPOCH_IN_TICKS;

  /* FILETIME values before the epoch are invalid. */
  if (ll < 0)
  {
    ll = 0;
  }

  ft_p->dwLowDateTime = (DWORD) ll;
  ft_p->dwHighDateTime = (DWORD) (ll >> 32);
}

static double
filetime_to_unix_time (LPFILETIME ft_p)
{
  ULARGE_INTEGER date;
  date.HighPart = ft_p->dwHighDateTime;
  date.LowPart = ft_p->dwLowDateTime;
  return (((double) date.QuadPart - (double) UNIX_EPOCH_IN_TICKS) / (double) TICKS_PER_MS);
}

int32_t
jjsp_time_local_tza (double unix_ms)
{
  FILETIME utc;
  FILETIME local;
  SYSTEMTIME utc_sys;
  SYSTEMTIME local_sys;

  unix_time_to_filetime (unix_ms, &utc);

  if (FileTimeToSystemTime (&utc, &utc_sys) && SystemTimeToTzSpecificLocalTime (NULL, &utc_sys, &local_sys)
      && SystemTimeToFileTime (&local_sys, &local))
  {
    double unix_local = filetime_to_unix_time (&local);
    return (int32_t) (unix_local - unix_ms);
  }

  return 0;
}

double
jjsp_time_now_ms (void)
{
  // Based on https://doxygen.postgresql.org/gettimeofday_8c_source.html
  const uint64_t epoch = (uint64_t) 116444736000000000ULL;
  FILETIME file_time;
  ULARGE_INTEGER ularge;

  GetSystemTimeAsFileTime (&file_time);
  ularge.LowPart = file_time.dwLowDateTime;
  ularge.HighPart = file_time.dwHighDateTime;

  int64_t tv_sec = (int64_t) ((ularge.QuadPart - epoch) / 10000000L);
  int32_t tv_usec = (int32_t) (((ularge.QuadPart - epoch) % 10000000L) / 10);

  return ((double) tv_sec) * 1000.0 + ((double) tv_usec) / 1000.0;
}

uint64_t
jjsp_time_hrtime (void)
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
      fprintf (stderr, "jjs_port_hrtime: %s: %i\n", "QueryPerformanceFrequency", (int32_t) GetLastError());
      jjs_port_fatal (JJS_FATAL_FAILED_ASSERTION);
    }
  }

  LARGE_INTEGER counter;

  JJS_ASSERT (scaled_frequency != 0);

  if (!QueryPerformanceCounter(&counter))
  {
    fprintf (stderr, "jjs_port_hrtime: %s: %i\n", "QueryPerformanceCounter", (int32_t) GetLastError());
    jjs_port_fatal (JJS_FATAL_FAILED_ASSERTION);
  }

  JJS_ASSERT (counter.QuadPart != 0);

  return (uint64_t) ((double) counter.QuadPart / scaled_frequency);
}

#endif /* JJS_OS_IS_WINDOWS */
