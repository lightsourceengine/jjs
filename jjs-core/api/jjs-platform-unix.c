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

#ifdef JJS_OS_IS_UNIX

#include <stdlib.h>

static void
stdlib_free (jjs_platform_buffer_t* self_p)
{
  if (self_p->data_p)
  {
    free (self_p->data_p);
    self_p->data_p = NULL;
    self_p->data_size = 0;
  }
}

static void
jjsp_buffer_view_from_stdlib_alloc (void* buffer,
                                    jjs_size_t buffer_size,
                                    jjs_encoding_t encoding,
                                    jjs_platform_buffer_view_t* buffer_view_p)
{
  jjs_platform_buffer_t source = jjs_platform_buffer (buffer, buffer_size, NULL);

  source.free = stdlib_free;

  jjs_platform_buffer_view_from_buffer (buffer_view_p, &source, encoding);
}

#if JJS_PLATFORM_API_PATH_CWD == 1
#include <unistd.h>

jjs_status_t
jjs_platform_path_cwd_impl (const jjs_allocator_t* allocator, jjs_platform_buffer_view_t* buffer_view_p)
{
  JJS_UNUSED (allocator);
  char* path_p = getcwd (NULL, 0);

  if (path_p == NULL)
  {
    return JJS_STATUS_PLATFORM_CWD_ERR;
  }

  lit_utf8_byte_t* utf8_p = (lit_utf8_byte_t*) path_p;
  jjs_size_t len = (uint32_t) strlen (path_p);

  /* no trailing slash */
  if (len > 1 && jjsp_path_is_separator (utf8_p[len - 1]))
  {
    len -= 1;
  }

  jjsp_buffer_view_from_stdlib_alloc (path_p, len, JJS_ENCODING_UTF8, buffer_view_p);

  return JJS_STATUS_OK;
}
#endif /* JJS_PLATFORM_API_PATH_CWD */

#if JJS_PLATFORM_API_TIME_SLEEP == 1

#include <time.h>
#include <errno.h>

jjs_status_t
jjs_platform_time_sleep_impl (uint32_t sleep_time_ms) /**< milliseconds to sleep */
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

  return JJS_STATUS_OK;
}

#endif /* JJS_PLATFORM_API_TIME_SLEEP */

#if JJS_PLATFORM_API_TIME_LOCAL_TZA == 1

#include <time.h>

jjs_status_t
jjs_platform_time_local_tza_impl (double unix_ms, int32_t* out_p)
{
  time_t time = (time_t) unix_ms / 1000;

#if defined(HAVE_TM_GMTOFF)
  struct tm tm;
  localtime_r (&time, &tm);

  *out_p = ((int32_t) tm.tm_gmtoff) * 1000;
#else /* !defined(HAVE_TM_GMTOFF) */
  struct tm gmt_tm;
  struct tm local_tm;

  gmtime_r (&time, &gmt_tm);
  localtime_r (&time, &local_tm);

  time_t gmt = mktime (&gmt_tm);

  /* mktime removes the daylight saving time from the result time value, however we want to keep it */
  local_tm.tm_isdst = 0;
  time_t local = mktime (&local_tm);

  *out_p =  (int32_t) difftime (local, gmt) * 1000;
#endif /* HAVE_TM_GMTOFF */

  return JJS_STATUS_OK;
}

#endif /* JJS_PLATFORM_API_TIME_LOCAL_TZA */

#if JJS_PLATFORM_API_TIME_NOW_MS == 1

#include <time.h>
#include <sys/time.h>

jjs_status_t
jjs_platform_time_now_ms_impl (double* out_p)
{
  struct timeval tv;

  if (gettimeofday (&tv, NULL) != 0)
  {
    *out_p = 0;
    return JJS_STATUS_PLATFORM_TIME_API_ERR;
  }
  else
  {
    *out_p = ((double) tv.tv_sec) * 1000.0 + ((double) tv.tv_usec) / 1000.0;
    return JJS_STATUS_OK;
  }
}

#endif /* JJS_PLATFORM_API_TIME_NOW_MS */

#if JJS_PLATFORM_API_PATH_REALPATH == 1

#include <unistd.h>
#include <stdlib.h>

jjs_status_t
jjs_platform_path_realpath_impl (const jjs_allocator_t* allocator,
                         jjs_platform_path_t* path_p,
                         jjs_platform_buffer_view_t* buffer_view_p)
{
  JJS_UNUSED (allocator);
  jjs_status_t status;
  jjs_platform_buffer_view_t path_view_p;

  status = path_p->convert (path_p, JJS_ENCODING_UTF8, JJS_PATH_FLAG_NULL_TERMINATE, &path_view_p);

  if (status != JJS_STATUS_OK)
  {
    return status;
  }

  char* data_p;

#if defined(HAVE_CANONICALIZE_FILE_NAME)
  data_p = canonicalize_file_name ((const char*) path_view_p.data_p);
#elif (defined(JJS_OS_IS_MACOS) && defined(__clang__))
  data_p = realpath ((const char*) path_view_p.data_p, NULL);
#elif (defined(_POSIX_VERSION) && _POSIX_VERSION >= 200809L)
  data_p = realpath ((const char*) path_view_p.data_p, NULL);
#else
  /*
   * Problem:
   *
   * realpath was fixed to accept a NULL resolved path and internall malloc the space
   * needed for the resolved path. The previous version wanted the caller to allocate
   * enough space for the maximum size path.
   *
   * On some systems, passing a PATH_MAX mallocated buffer to realpath works fine, but
   * on other systems PATH_MAX is a suggestion and realpath will fail by error or a
   * buffer overflow. Or, PATH_MAX from pathconf is -1 (meaning unbound PATH_MAX) or
   * a really huge number. jjs-core chooses not to implement a fallback because the
   * behavior is not completely defined.
   *
   * Configuration:
   *
   * If you are using an amalgam build, it could simply be that build flags have not
   * been set. JJS build defines _DEFAULT_SOURCE and _BSD_SOURCE. On recent gcc compilers,
   * this gives a POSIX version of 2008 or later.
   *
   * On linux gcc, you can enable gnu extensions. If canonical_file_name is available,
   * your build can define HAVE_CANONICALIZE_FILE_NAME and JJS will use that.
   *
   * If configuration does not work, the compiler or system libc may not support POSIX
   * 2008 (or a known working realpath alternative).
   *
   * Solution:
   *
   * If configuration is not possible, you can exclude THIS function in the build with the
   * compiler define:
   *
   *   -DJJS_PLATFORM_API_PATH_REALPATH=0
   *
   * JJS needs realpath for pmap resolution, import'ing / require'ing relative and absolute
   * paths, the jjs_platform_realpath api or commandline tools. If none of this is needed,
   * you can just leave out realpath from the engine.
   *
   * If you think your platform realpath is ok or realpath with PATH_MAX is ok, you can
   * implement you own version of this function and pass it to jjs_init as the realpath.
   * jjs_platform_cesu8_convert () is available to convert the passed in path from CESU8 to
   * a null terminated UTF8 (or UTF16). Your implementation will look similar with your own
   * way of calling realpath or your own version of canonical paths.
   *
   * Examples of realpath with PATH_MAX/pathconf fallback:
   *
   * - https://github.com/libuv/libuv/blob/v1.x/src/unix/fs.c
   * - https://github.com/gcc-mirror/gcc/blob/master/libiberty/lrealpath.c
   */

#error "compiler does not have a known, working realpath"
#endif

  path_view_p.free (&path_view_p);

  if (data_p == NULL)
  {
    return JJS_STATUS_PLATFORM_REALPATH_ERR;
  }

  jjsp_buffer_view_from_stdlib_alloc (data_p, (uint32_t) strlen (data_p), JJS_ENCODING_UTF8, buffer_view_p);

  return JJS_STATUS_OK;
}

#endif /* JJS_PLATFORM_API_PATH_REALPATH */

#if JJS_PLATFORM_API_FS_READ_FILE == 1

#include <stdlib.h>
#include <stdio.h>

jjs_status_t
jjs_platform_fs_read_file_impl (const jjs_allocator_t* allocator, jjs_platform_path_t* path_p, jjs_platform_buffer_t* out_p)
{
  jjs_status_t status;
  jjs_platform_buffer_view_t path_view_p;

  status = path_p->convert (path_p, JJS_ENCODING_UTF8, JJS_PATH_FLAG_NULL_TERMINATE, &path_view_p);

  if (status != JJS_STATUS_OK)
  {
    return status;
  }

  FILE* file_p = fopen ((const char*) path_view_p.data_p, "rb");
  path_view_p.free (&path_view_p);

  if (!file_p)
  {
    return JJS_STATUS_PLATFORM_FILE_OPEN_ERR;
  }

  if (ftell (file_p) != 0)
  {
    status = JJS_STATUS_PLATFORM_FILE_SEEK_ERR;
    goto done;
  }

  int err = fseek (file_p, 0, SEEK_END);

  if (err != 0)
  {
    status = JJS_STATUS_PLATFORM_FILE_SEEK_ERR;
    goto done;
  }

  long file_size = ftell (file_p);

  if (file_size <= 0 || file_size > INT32_MAX)
  {
    status = JJS_STATUS_PLATFORM_FILE_SIZE_TOO_BIG;
    goto done;
  }

  err = fseek (file_p, 0, SEEK_SET);

  if (err != 0)
  {
    status = JJS_STATUS_PLATFORM_FILE_SEEK_ERR;
    goto done;
  }

  status = jjs_platform_buffer_new (out_p, allocator, (jjs_size_t) file_size);

  if (status != JJS_STATUS_OK)
  {
    goto done;
  }

  size_t num_read = fread (out_p->data_p, (size_t) file_size, 1, file_p);

  if (num_read != 1)
  {
    out_p->free (out_p);
    status = JJS_STATUS_PLATFORM_FILE_READ_ERR;
    goto done;
  }

  status = JJS_STATUS_OK;

done:
  fclose (file_p);
  return status;
}

#endif /* JJS_PLATFORM_API_FS_READ_FILE */

bool
jjsp_path_is_relative (const lit_utf8_byte_t* path_p, lit_utf8_size_t size)
{
  return size > 0 && path_p[0] == '.';
}

bool
jjsp_path_is_absolute (const lit_utf8_byte_t* path_p, lit_utf8_size_t size)
{
  return size > 0 && path_p[0] == '/';
}

bool
jjsp_find_root_end_index (const lit_utf8_byte_t* str_p, lit_utf8_size_t size, lit_utf8_size_t* index)
{
  if (size == 0 || !jjsp_path_is_separator (*str_p))
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
