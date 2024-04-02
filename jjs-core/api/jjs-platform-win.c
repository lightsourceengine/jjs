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

#include "jjs-compiler.h"
#include "jjs-platform.h"

#ifdef JJS_OS_IS_WINDOWS

#include "jcontext.h"
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
      jjs_log (JJS_LOG_LEVEL_ERROR, "hrtime: %s: %i\n", "QueryPerformanceFrequency", (int32_t) GetLastError());
      JJS_CONTEXT (platform_api).fatal (JJS_FATAL_FAILED_ASSERTION);
    }
  }

  LARGE_INTEGER counter;

  JJS_ASSERT (scaled_frequency != 0);

  if (!QueryPerformanceCounter(&counter))
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "hrtime: %s: %i\n", "QueryPerformanceCounter", (int32_t) GetLastError());
    JJS_CONTEXT (platform_api).fatal (JJS_FATAL_FAILED_ASSERTION);
  }

  JJS_ASSERT (counter.QuadPart != 0);

  return (uint64_t) ((double) counter.QuadPart / scaled_frequency);
}

jjs_platform_status_t
jjsp_path_normalize (const uint8_t* utf8_p, uint32_t size, jjs_platform_buffer_t* buffer_p)
{
  JJS_UNUSED_ALL(utf8_p, size, buffer_p);
  return JJS_PLATFORM_STATUS_ERR;
}

static lit_utf8_size_t
jjsp_remove_long_path_prefixes (ecma_char_t *path_p, lit_utf8_size_t len)
{
  static const WCHAR LONG_PATH_PREFIX[] = L"\\\\?\\";
  static const WCHAR LONG_PATH_PREFIX_LEN = 4;

  static const WCHAR UNC_PATH_PREFIX[] = L"\\\\?\\UNC\\";
  static const WCHAR UNC_PATH_PREFIX_LEN = 8;

  ecma_char_t *copy_point_p;
  lit_utf8_size_t copy_len;
  lit_utf8_size_t i;

  if (wcsncmp (path_p, UNC_PATH_PREFIX, UNC_PATH_PREFIX_LEN) == 0)
  {
    copy_point_p = path_p + 6;
    *copy_point_p = L'\\';
    copy_len = len - 6;
  }
  else if (wcsncmp (path_p, LONG_PATH_PREFIX, LONG_PATH_PREFIX_LEN) == 0)
  {
    copy_point_p = path_p + 4;
    copy_len = len - 4;
  }
  else
  {
    return len;
  }

  for (i = 0; i < copy_len; i++)
  {
    path_p[i] = copy_point_p[i];
  }

  path_p[i] = L'\0';

  return copy_len;
}

jjs_platform_status_t
jjsp_path_realpath (const uint8_t* cesu8_p, uint32_t size, jjs_platform_buffer_t* buffer_p)
{
  ecma_char_t* path_p = jjsp_cesu8_to_utf16_sz (cesu8_p, size);

  if (path_p == NULL)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  HANDLE file = CreateFileW (path_p, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);

  free (path_p);

  if (file == INVALID_HANDLE_VALUE)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  DWORD length = GetFinalPathNameByHandleW (file, NULL, 0, VOLUME_NAME_DOS);

  if (length <= 0)
  {
    CloseHandle(file);
    return JJS_PLATFORM_STATUS_ERR;
  }

  // length includes null terminator
  ecma_char_t* data_p = malloc (((size_t) length) * sizeof (ecma_char_t));

  if (GetFinalPathNameByHandleW (file, data_p, length, VOLUME_NAME_DOS) <= 0)
  {
    free (data_p);
    CloseHandle(file);
    return JJS_PLATFORM_STATUS_ERR;
  }

  CloseHandle(file);

  // length includes null terminator
  lit_utf8_size_t data_len = jjsp_remove_long_path_prefixes (data_p, (lit_utf8_size_t) (length - 1));

  buffer_p->data_p = data_p;
  buffer_p->length = data_len * ((uint32_t) sizeof (ecma_char_t));
  buffer_p->encoding = JJS_PLATFORM_BUFFER_ENCODING_UTF16;
  buffer_p->free = jjsp_buffer_free;

  return JJS_PLATFORM_STATUS_OK;
}

bool
jjsp_find_root_end_index (const lit_utf8_byte_t* str_p, lit_utf8_size_t size, lit_utf8_size_t* index)
{
  if (size == 0)
  {
    return false;
  }

  lit_utf8_size_t start_index;

  if (size >= 2 && isalpha (str_p[0]) && str_p[1] == ':')
  {
    start_index = 2;
  }
  else if (size >= 4 && memcmp (str_p, "\\\\?\\", 4) == 0)
  {
    if (size >= 6 && isalpha (str_p[4]) && str_p[5] == ':')
    {
      start_index = 6;
    }
    else if (size >= 10 && memcmp (&str_p[4], "Volume", 6) == 0)
    {
      const lit_utf8_byte_t* guid_p = str_p + 10;

      if (size < 47)
      {
        return false;
      }

      for (lit_utf8_size_t i = 0; i <= 37; i++)
      {
        switch (i)
        {
          case 0:
          {
            if (guid_p[i] != '{')
            {
              return false;
            }
            break;
          }
          case 9:
          case 14:
          case 19:
          case 24:
          {
            if (guid_p[i] != '-')
            {
              return false;
            }
            break;
          }
          case 37:
          {
            if (guid_p[i] != '}')
            {
              return FALSE;
            }
            break;
          }
          default:
          {
            if (!isxdigit (guid_p[i]))
            {
              return false;
            }
            break;
          }
        }
      }
      start_index = 10;
    }
    else if (size >= 8 && memcmp (&str_p[4], "UNC\\", 4) == 0)
    {
      start_index = 8;
    }
    else
    {
      return false;
    }
  }
  else if (size >= 2 && str_p[0] == '\\' && str_p[1] == '\\')
  {
    start_index = 2;
  }
  else if (size >= 1 && jjsp_path_is_separator (str_p[0]))
  {
    start_index = 1;
  }
  else
  {
    return false;
  }

  while (start_index < size && jjsp_path_is_separator (str_p[start_index]))
  {
    start_index++;
  }

  *index = start_index;

  return true;
}

bool
jjsp_path_is_separator (lit_utf8_byte_t ch)
{
  return ch == '\\' || ch == '/';
}

jjs_platform_status_t
jjsp_fs_read_file (const uint8_t* cesu8_p, uint32_t size, jjs_platform_buffer_t* buffer_p)
{
  ecma_char_t* path_p = jjsp_cesu8_to_utf16_sz (cesu8_p, size);

  if (path_p == NULL)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  HANDLE file = CreateFileW (path_p, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  free (path_p);

  if (file == INVALID_HANDLE_VALUE)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  LARGE_INTEGER file_size_result;

  if (GetFileSizeEx (file, &file_size_result) != TRUE || file_size_result.QuadPart > INT_MAX)
  {
    CloseHandle (file);
    return JJS_PLATFORM_STATUS_ERR;
  }

  uint32_t file_size = (uint32_t) file_size_result.QuadPart;
  void* data_p = malloc (file_size);

  if (data_p == NULL)
  {
    CloseHandle (file);
    return JJS_PLATFORM_STATUS_ERR;
  }

  if (ReadFile (file, data_p, (DWORD) file_size, NULL, NULL) != TRUE)
  {
    free (data_p);
    CloseHandle (file);
    return JJS_PLATFORM_STATUS_ERR;
  }

  buffer_p->data_p = data_p;
  buffer_p->length = file_size;
  buffer_p->encoding = JJS_PLATFORM_BUFFER_ENCODING_NONE;
  buffer_p->free = jjsp_buffer_free;

  return JJS_PLATFORM_STATUS_OK;
}

#endif /* JJS_OS_IS_WINDOWS */
