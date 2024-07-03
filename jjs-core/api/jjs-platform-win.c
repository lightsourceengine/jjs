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
#include "jcontext.h"

#ifdef JJS_OS_IS_WINDOWS

#if JJS_PLATFORM_API_PATH_CWD

#include <windows.h>

jjs_status_t
jjs_platform_path_cwd_impl (const jjs_allocator_t* allocator, jjs_platform_buffer_view_t* buffer_view_p)
{
  WCHAR* p;
  DWORD n;
  DWORD t = GetCurrentDirectoryW (0, NULL);
  jjs_size_t allocated_size;

  for (;;) {
    if (t == 0)
    {
      return JJS_STATUS_PLATFORM_CWD_ERR;
    }

    /* |t| is the size of the buffer _including_ nul. */
    allocated_size = (jjs_size_t) (t * sizeof (*p));
    p = allocator->vtab_p->alloc (allocator->impl_p, allocated_size);

    if (p == NULL)
    {
      return JJS_STATUS_PLATFORM_CWD_ERR;
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

    allocator->vtab_p->free (allocator->impl_p, p, allocated_size);
    t = n;
  }

  jjs_size_t size = allocated_size;

  /* The returned directory should not have a trailing slash, unless it points
   * at a drive root, like c:\. Remove it if needed.
   */
  t = n - 1;
  if (p[t] == L'\\' && !(n == 3 && p[1] == L':'))
  {
    p[t] = L'\0';
    n = t;
    size -= (1 * sizeof (ecma_char_t));
  }

  size -= (1 * sizeof (ecma_char_t));

  jjs_platform_buffer_t source = jjs_platform_buffer (p, allocated_size, allocator);

  jjs_platform_buffer_view_from_buffer (buffer_view_p, &source, JJS_ENCODING_UTF16);
  buffer_view_p->data_size = size;

  return JJS_STATUS_OK;
}

#endif /* JJS_PLATFORM_API_PATH_CWD */

#if JJS_PLATFORM_API_TIME_SLEEP
#include <windows.h>

jjs_status_t
jjs_platform_time_sleep_impl (uint32_t sleep_time_ms) /**< milliseconds to sleep */
{
  Sleep (sleep_time_ms);

  return JJS_STATUS_OK;
}

#endif /* JJS_PLATFORM_API_TIME_SLEEP */

#if JJS_PLATFORM_API_TIME_LOCAL_TZA

#include <time.h>

jjs_status_t
jjs_platform_time_local_tza_impl (double unix_ms, int32_t* out_p)
{
  time_t time = (time_t) unix_ms / 1000;

  struct tm gmt_tm;
  struct tm local_tm;

  gmtime_s (&gmt_tm, &time);
  localtime_s (&local_tm, &time);

  time_t gmt = mktime (&gmt_tm);

  /* mktime removes the daylight saving time from the result time value, however we want to keep it */
  local_tm.tm_isdst = 0;
  time_t local = mktime (&local_tm);

  *out_p =  (int32_t) difftime (local, gmt) * 1000;

  return JJS_STATUS_OK;
}

#endif /* JJS_PLATFORM_API_TIME_LOCAL_TZA */

#if JJS_PLATFORM_API_TIME_NOW_MS

#include <windows.h>

jjs_status_t
jjs_platform_time_now_ms_impl (double* out_p)
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

  *out_p = ((double) tv_sec) * 1000.0 + ((double) tv_usec) / 1000.0;

  return JJS_STATUS_OK;
}

#endif /* JJS_PLATFORM_API_TIME_NOW_MS */

#if JJS_PLATFORM_API_PATH_REALPATH

#include <windows.h>

static const WCHAR LONG_PATH_PREFIX[] = L"\\\\?\\";
static const WCHAR LONG_PATH_PREFIX_LEN = 4;

static const WCHAR UNC_PATH_PREFIX[] = L"\\\\?\\UNC\\";
static const WCHAR UNC_PATH_PREFIX_LEN = 8;

jjs_status_t
jjs_platform_path_realpath_impl (const jjs_allocator_t* allocator,
                         jjs_platform_path_t* path_p,
                         jjs_platform_buffer_view_t* buffer_view_p)
{
  jjs_status_t status;
  jjs_platform_buffer_view_t path_view;

  status = path_p->convert (path_p, JJS_ENCODING_UTF16, JJS_PATH_FLAG_NULL_TERMINATE, &path_view);

  if (status != JJS_STATUS_OK)
  {
    return status;
  }

  ecma_char_t* wpath_p = (ecma_char_t*) path_view.data_p;

  HANDLE file = CreateFileW (wpath_p, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);

  path_view.free (&path_view);

  if (file == INVALID_HANDLE_VALUE)
  {
    return JJS_STATUS_PLATFORM_FILE_OPEN_ERR;
  }

  DWORD length = GetFinalPathNameByHandleW (file, NULL, 0, VOLUME_NAME_DOS);

  if (length <= 0)
  {
    CloseHandle(file);
    return JJS_STATUS_PLATFORM_REALPATH_ERR;
  }

  // length includes null terminator
  jjs_size_t allocated_size = ((size_t) length) * sizeof (ecma_char_t);
  ecma_char_t* data_p = allocator->vtab_p->alloc (allocator->impl_p, allocated_size);

  if (GetFinalPathNameByHandleW (file, data_p, length, VOLUME_NAME_DOS) <= 0)
  {
    allocator->vtab_p->free (allocator->impl_p, data_p, allocated_size);
    CloseHandle(file);
    return JJS_STATUS_PLATFORM_REALPATH_ERR;
  }

  CloseHandle(file);

  jjs_platform_buffer_t source = jjs_platform_buffer (data_p, allocated_size, allocator);

  jjs_platform_buffer_view_from_buffer (buffer_view_p, &source, JJS_ENCODING_UTF16);

  ecma_char_t* p = (ecma_char_t*) buffer_view_p->data_p;

  /* convert UNC path to long path */
  if (wcsncmp (buffer_view_p->data_p,
               UNC_PATH_PREFIX,
               UNC_PATH_PREFIX_LEN) == 0) {
    p += 6;
    *p = L'\\';
    buffer_view_p->data_p = p;
    buffer_view_p->data_size -= (6 * sizeof (ecma_char_t));
  } else if (wcsncmp (buffer_view_p->data_p,
                    LONG_PATH_PREFIX,
                    LONG_PATH_PREFIX_LEN) == 0) {
    p += 4;
    buffer_view_p->data_p = p;
    buffer_view_p->data_size -= (4 * sizeof (ecma_char_t));
  } else {
    /* Is this an error case? */
  }

  /* length includes null terminator */
  buffer_view_p->data_size -= (1 * sizeof (ecma_char_t));

  return JJS_STATUS_OK;
}

#endif /* JJS_PLATFORM_API_PATH_REALPATH */

#if JJS_PLATFORM_API_FS_READ_FILE

#include <windows.h>

jjs_status_t
jjs_platform_fs_read_file_impl (const jjs_allocator_t* allocator, jjs_platform_path_t* path_p, jjs_platform_buffer_t* out_p)
{
  jjs_status_t status;
  jjs_platform_buffer_view_t path_view;

  status = path_p->convert (path_p, JJS_ENCODING_UTF16, JJS_PATH_FLAG_NULL_TERMINATE, &path_view);

  if (status != JJS_STATUS_OK)
  {
    return status;
  }

  ecma_char_t* wpath_p = (ecma_char_t*) path_view.data_p;

  HANDLE file = CreateFileW (wpath_p, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  path_view.free (&path_view);

  if (file == INVALID_HANDLE_VALUE)
  {
    return JJS_STATUS_PLATFORM_FILE_OPEN_ERR;
  }

  LARGE_INTEGER file_size_result;

  if (GetFileSizeEx (file, &file_size_result) != TRUE || file_size_result.QuadPart > INT_MAX)
  {
    CloseHandle (file);
    return JJS_STATUS_PLATFORM_FILE_SEEK_ERR;
  }

  uint32_t file_size = (uint32_t) file_size_result.QuadPart;

  status = jjs_platform_buffer_new (out_p, allocator, (jjs_size_t) file_size);

  if (status != JJS_STATUS_OK)
  {
    CloseHandle (file);
    return status;
  }

  if (ReadFile (file, out_p->data_p, (DWORD) file_size, NULL, NULL) != TRUE)
  {
    out_p->free (out_p);
    CloseHandle (file);
    return JJS_STATUS_PLATFORM_FILE_READ_ERR;
  }

  return JJS_STATUS_OK;
}

#endif /* JJS_PLATFORM_API_FS_READ_FILE */

bool
jjsp_path_is_relative (const lit_utf8_byte_t* path_p, lit_utf8_size_t size)
{
  if (size > 0 && path_p[0] == '.')
  {
    return true;
  }

  if (size > 1 && isalpha (path_p[0]) && path_p[1] == ':')
  {
    return size == 2 || (size > 2 && !jjsp_path_is_separator (path_p[2]));
  }

  return false;
}

bool
jjsp_path_is_absolute (const lit_utf8_byte_t* path_p, lit_utf8_size_t size)
{
  return (size > 0 && jjsp_path_is_separator (path_p[0]))
         || (size > 2 && isalpha (path_p[0]) && path_p[1] == ':' && jjsp_path_is_separator (path_p[2]));
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

#endif /* JJS_OS_IS_WINDOWS */
