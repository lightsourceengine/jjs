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
#include "jcontext.h"

#ifdef JJS_OS_IS_UNIX

#if JJS_PLATFORM_API_PATH_CWD
#include <unistd.h>
#ifdef JJS_OS_IS_MACOS
#include <sys/syslimits.h>
#else
#include <linux/limits.h>
#endif

jjs_platform_status_t
jjsp_cwd (jjs_platform_buffer_t* buffer_p)
{
  char* path_p = getcwd (NULL, 0);

  if (path_p == NULL)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  uint32_t path_len = (uint32_t) strlen (path_p);

  // no trailing slash
  if (path_len > 1 && jjsp_path_is_separator ((lit_utf8_byte_t) path_p[path_len - 1])) {
    path_len -= 1;
  }

  buffer_p->data_p = path_p;
  buffer_p->length = path_len;
  buffer_p->free = jjsp_buffer_free;
  buffer_p->encoding = JJS_ENCODING_UTF8;

  return JJS_PLATFORM_STATUS_OK;
}
#endif /* JJS_PLATFORM_API_PATH_CWD */

#if JJS_PLATFORM_API_TIME_SLEEP

#include <time.h>
#include <errno.h>

void
jjsp_time_sleep (uint32_t sleep_time_ms) /**< milliseconds to sleep */
{
  struct timespec timeout;
  int rc;
  int32_t ms = (sleep_time_ms == UINT32_MAX) ? INT32_MAX : (int32_t) sleep_time_ms;

  timeout.tv_sec = ms / 1000;
  timeout.tv_nsec = (ms % 1000) * 1000 * 1000;

  do
  {
    rc = nanosleep (&timeout, &timeout);
  }
  while (rc == -1 && errno == EINTR);
}

#endif /* JJS_PLATFORM_API_TIME_SLEEP */

#if JJS_PLATFORM_API_TIME_LOCAL_TZA

#include <time.h>

int32_t
jjsp_time_local_tza (double unix_ms)
{
  time_t time = (time_t) unix_ms / 1000;

#if defined(HAVE_TM_GMTOFF)
  struct tm tm;
  localtime_r (&time, &tm);

  return ((int32_t) tm.tm_gmtoff) * 1000;
#else /* !defined(HAVE_TM_GMTOFF) */
  struct tm gmt_tm;
  struct tm local_tm;

  gmtime_r (&time, &gmt_tm);
  localtime_r (&time, &local_tm);

  time_t gmt = mktime (&gmt_tm);

  /* mktime removes the daylight saving time from the result time value, however we want to keep it */
  local_tm.tm_isdst = 0;
  time_t local = mktime (&local_tm);

  return (int32_t) difftime (local, gmt) * 1000;
#endif /* HAVE_TM_GMTOFF */
}

#endif /* JJS_PLATFORM_API_TIME_LOCAL_TZA */

#if JJS_PLATFORM_API_TIME_NOW_MS

#include <time.h>
#include <sys/time.h>

double
jjsp_time_now_ms (void)
{
  struct timeval tv;

  if (gettimeofday (&tv, NULL) != 0)
  {
    return 0;
  }

  return ((double) tv.tv_sec) * 1000.0 + ((double) tv.tv_usec) / 1000.0;
}

#endif /* JJS_PLATFORM_API_TIME_NOW_MS */

#if JJS_PLATFORM_API_TIME_HRTIME
#ifdef JJS_OS_IS_MACOS
#include <mach/mach_time.h>

uint64_t jjsp_time_hrtime (void)
{
  // adapted from uv_hrtime(): https://github.com/libuv/libuv/src/unix/darwin.c
  static mach_timebase_info_data_t hrtime_timebase = {0};
  static uint64_t (*hrtime_fn) (void) = NULL;

  if (hrtime_fn == NULL)
  {
    if (KERN_SUCCESS != mach_timebase_info (&hrtime_timebase))
    {
      JJS_CONTEXT (platform_api).fatal (JJS_FATAL_FAILED_ASSERTION);
    }

    hrtime_fn = &mach_continuous_time;

    if (hrtime_fn == NULL)
    {
      hrtime_fn = &mach_absolute_time;
    }
  }

  return hrtime_fn () * hrtime_timebase.numer / hrtime_timebase.denom;
}

#else
#include <time.h>

uint64_t jjsp_time_hrtime (void)
{
  // adapted from uv_hrtime(): https://github.com/libuv/libuv/src/unix/linux.c

  static clockid_t clock_id = -1;
  struct timespec t;

  if (clock_id == -1)
  {
    clock_id = CLOCK_MONOTONIC;

    // prefer coarse clock iff it has millisecond accuracy or better. in
    // certain situations, CLOCK_MONOTONIC can be very slow.
    if (0 == clock_getres (CLOCK_MONOTONIC_COARSE, &t))
    {
      if (t.tv_nsec <= 1 * 1000 * 1000)
      {
        clock_id = CLOCK_MONOTONIC_COARSE;
      }
    }
  }

  if (clock_gettime (clock_id, &t))
  {
    return 0;
  }

  return (uint64_t) t.tv_sec * (uint64_t) 1e9 + (uint64_t) t.tv_nsec;
}
#endif /* JJS_OS_IS_MACOS */
#endif /* JJS_PLATFORM_API_TIME_HRTIME */

#if JJS_PLATFORM_API_PATH_REALPATH

#include <stdlib.h>

jjs_platform_status_t
jjsp_path_realpath (const uint8_t* cesu8_p, uint32_t size, jjs_platform_buffer_t* buffer_p)
{
  char* path_p = jjsp_cesu8_to_utf8_sz (cesu8_p, size);

  if (path_p == NULL)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  char* data_p;

#if defined (_POSIX_VERSION) && _POSIX_VERSION >= 200809L
  data_p = realpath (path_p, NULL);
#else
  data_p = malloc (PATH_MAX);

  if (data_p && realpath (path_p, data_p) == NULL) {
    free (data_p);
    data_p = NULL;
  }
#endif

  free (path_p);

  if (data_p == NULL)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  buffer_p->data_p = data_p;
  buffer_p->encoding = JJS_ENCODING_UTF8;
  buffer_p->length = (uint32_t) strlen (data_p);
  buffer_p->free = jjsp_buffer_free;

  return JJS_PLATFORM_STATUS_OK;
}

#endif /* JJS_PLATFORM_API_PATH_REALPATH */

#if JJS_PLATFORM_API_FS_READ_FILE

#include <stdlib.h>
#include <stdio.h>

jjs_platform_status_t
jjsp_fs_read_file (const uint8_t* cesu8_p, uint32_t size, jjs_platform_buffer_t* buffer_p)
{
  jjs_platform_status_t status;
  char* path_p = jjsp_cesu8_to_utf8_sz (cesu8_p, size);

  if (path_p == NULL)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  FILE* file_p = fopen (path_p, "rb");
  free (path_p);

  if (!file_p)
  {
    return JJS_PLATFORM_STATUS_ERR;
  }

  if (ftell (file_p) != 0)
  {
    status = JJS_PLATFORM_STATUS_ERR;
    goto done;
  }

  int err = fseek (file_p, 0, SEEK_END);

  if (err != 0)
  {
    status = JJS_PLATFORM_STATUS_ERR;
    goto done;
  }

  long file_size = ftell (file_p);

  if (file_size <= 0 || file_size > INT32_MAX)
  {
    status = JJS_PLATFORM_STATUS_ERR;
    goto done;
  }

  err = fseek (file_p, 0, SEEK_SET);

  if (err != 0)
  {
    status = JJS_PLATFORM_STATUS_ERR;
    goto done;
  }

  void* data_p = malloc ((size_t) file_size);

  if (data_p == NULL)
  {
    status = JJS_PLATFORM_STATUS_ERR;
    goto done;
  }

  size_t num_read = fread (data_p, (size_t) file_size, 1, file_p);

  if (num_read != 1)
  {
    free (data_p);
    status = JJS_PLATFORM_STATUS_ERR;
    goto done;
  }

  buffer_p->data_p = data_p;
  buffer_p->length = (uint32_t) file_size;
  buffer_p->encoding = JJS_ENCODING_NONE;
  buffer_p->free = jjsp_buffer_free;

  status = JJS_PLATFORM_STATUS_OK;

done:
  fclose (file_p);
  return status;
}

#endif /* JJS_PLATFORM_API_FS_READ_FILE */

bool
jjsp_find_root_end_index (const lit_utf8_byte_t* str_p, lit_utf8_size_t size, lit_utf8_size_t* index)
{
  if (size == 0 || !jjsp_path_is_separator(*str_p))
  {
    return false;
  }

  lit_utf8_size_t i = 1;

  while (i < size && jjsp_path_is_separator (str_p[i]))
  {
    i++;
  }

  *index = i;

  return true;
}

bool
jjsp_path_is_separator (lit_utf8_byte_t ch)
{
  return ch == '/';
}

#endif /* JJS_OS_IS_UNIX */
