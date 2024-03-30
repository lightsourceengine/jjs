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

#include "jjs-pack-config.h"

#if JJS_PACK_FS
#include "fs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#if defined(_WIN32)
#define platform_stat _stat
#else
#define platform_stat stat
#endif

// TODO: revisit. stat size type is different on different platforms
//#if UINTPTR_MAX == 0xffffffff
// 32-bit
#define JJS_PACK_FS_MAX_FILE_SIZE (INT32_MAX)
//#else
// 64-bit
//#define JJS_PACK_FS_MAX_FILE_SIZE (UINT32_MAX)
//#endif

#if !defined(EFTYPE)
#define EFTYPE (4028)
#endif

#if !defined(ESOCKTNOSUPPORT)
#define ESOCKTNOSUPPORT (4025)
#endif

#if !defined(EHOSTDOWN)
#define EHOSTDOWN (4031)
#endif

#if !defined(ESHUTDOWN)
#define ESHUTDOWN (4042)
#endif

#define FS_ERRNO_MAP(XX)                                                      \
  XX(E2BIG, "argument list too long")                                         \
  XX(EACCES, "permission denied")                                             \
  XX(EADDRINUSE, "address already in use")                                    \
  XX(EADDRNOTAVAIL, "address not available")                                  \
  XX(EAFNOSUPPORT, "address family not supported")                            \
  XX(EAGAIN, "resource temporarily unavailable")                              \
  XX(EALREADY, "connection already in progress")                              \
  XX(EBADF, "bad file descriptor")                                            \
  XX(EBUSY, "resource busy or locked")                                        \
  XX(ECANCELED, "operation canceled")                                         \
  XX(ECONNABORTED, "software caused connection abort")                        \
  XX(ECONNREFUSED, "connection refused")                                      \
  XX(ECONNRESET, "connection reset by peer")                                  \
  XX(EDESTADDRREQ, "destination address required")                            \
  XX(EEXIST, "file already exists")                                           \
  XX(EFAULT, "bad address in system call argument")                           \
  XX(EFBIG, "file too large")                                                 \
  XX(EHOSTUNREACH, "host is unreachable")                                     \
  XX(EINTR, "interrupted system call")                                        \
  XX(EINVAL, "invalid argument")                                              \
  XX(EIO, "i/o error")                                                        \
  XX(EISCONN, "socket is already connected")                                  \
  XX(EISDIR, "illegal operation on a directory")                              \
  XX(ELOOP, "too many symbolic links encountered")                            \
  XX(EMFILE, "too many open files")                                           \
  XX(EMSGSIZE, "message too long")                                            \
  XX(ENAMETOOLONG, "name too long")                                           \
  XX(ENETDOWN, "network is down")                                             \
  XX(ENETUNREACH, "network is unreachable")                                   \
  XX(ENFILE, "file table overflow")                                           \
  XX(ENOBUFS, "no buffer space available")                                    \
  XX(ENODEV, "no such device")                                                \
  XX(ENOENT, "no such file or directory")                                     \
  XX(ENOMEM, "not enough memory")                                             \
  XX(ENOPROTOOPT, "protocol not available")                                   \
  XX(ENOSPC, "no space left on device")                                       \
  XX(ENOSYS, "function not implemented")                                      \
  XX(ENOTCONN, "socket is not connected")                                     \
  XX(ENOTDIR, "not a directory")                                              \
  XX(ENOTEMPTY, "directory not empty")                                        \
  XX(ENOTSOCK, "socket operation on non-socket")                              \
  XX(ENOTSUP, "operation not supported on socket")                            \
  XX(EOVERFLOW, "value too large for defined data type")                      \
  XX(EPERM, "operation not permitted")                                        \
  XX(EPIPE, "broken pipe")                                                    \
  XX(EPROTO, "protocol error")                                                \
  XX(EPROTONOSUPPORT, "protocol not supported")                               \
  XX(EPROTOTYPE, "protocol wrong type for socket")                            \
  XX(ERANGE, "result too large")                                              \
  XX(EROFS, "read-only file system")                                          \
  XX(ESHUTDOWN, "cannot send after transport endpoint shutdown")              \
  XX(ESPIPE, "invalid seek")                                                  \
  XX(ESRCH, "no such process")                                                \
  XX(ETIMEDOUT, "connection timed out")                                       \
  XX(ETXTBSY, "text file is busy")                                            \
  XX(EXDEV, "cross-device link not permitted")                                \
  XX(EOF, "end of file")                                                      \
  XX(ENXIO, "no such device or address")                                      \
  XX(EMLINK, "too many links")                                                \
  XX(EHOSTDOWN, "host is down")                                               \
  XX(ENOTTY, "inappropriate ioctl for device")                                \
  XX(EFTYPE, "inappropriate file type or format")                             \
  XX(EILSEQ, "illegal byte sequence")                                         \
  XX(ESOCKTNOSUPPORT, "socket type not supported")                            \
  XX(ENODATA, "no data available")                                            \

int fs_get_size (const char* path, uint32_t* size_p)
{
  struct platform_stat st;
  int result = platform_stat(path, &st);

  if (result == 0)
  {
    if (st.st_size <= (off_t) JJS_PACK_FS_MAX_FILE_SIZE)
    {
      *size_p = st.st_size < 0 ? 0 : (uint32_t) st.st_size;
    }
    else
    {
      result = EOVERFLOW;
    }
  }

  return result;
} /* fs_get_size */

int fs_read(const char* path, uint8_t** buffer_p, uint32_t* buffer_size)
{
  errno = 0;
  FILE* file_p = fopen(path, "rb");

  if (!file_p)
  {
    return errno;
  }

  uint32_t target_size = 1 << 12; // 4K
  uint8_t* target_p = NULL;
  uint8_t* temp_p;
  uint32_t file_size = 0;
  int result;

  do
  {
    if (target_size == UINT32_MAX)
    {
      result = ENOMEM;
      break;
    }

    target_size *= 2;
    temp_p = realloc(target_p, target_size);

    if (temp_p == NULL) {
      result = ENOMEM;
      break;
    }

    target_p = temp_p;
    file_size += (uint32_t) fread(target_p + file_size, 1, target_size - file_size, file_p);

    if (file_size == 0)
    {
      result = errno == 0 ? EIO : errno;
      break;
    }

    if (file_size < target_size)
    {
      *buffer_p = target_p;
      *buffer_size = file_size;
      target_p = NULL;
      result = 0;
      break;
    }
  } while (true);

  fclose(file_p);
  free(target_p);

  return result;
} /* fs_read */

void fs_read_free(uint8_t* buffer_p)
{
  free(buffer_p);
} /* fs_read_free */

int fs_copy (const char* path_p, const char* source_p, uint32_t* written_p)
{
  errno = 0;

  int err = 0;
  FILE* source_file_p = fopen (source_p, "rb");
  FILE* destination_file_p = NULL;

  if (!source_file_p)
  {
    err = ferror (source_file_p);
    goto done;
  }

  destination_file_p = fopen (path_p, "wb");

  if (!destination_file_p)
  {
    err = ferror (destination_file_p);
    goto done;
  }

  uint8_t buffer[8192];

  *written_p = 0;

  while (true)
  {
    uint32_t read_size = (uint32_t) fread (buffer, 1, sizeof (buffer), source_file_p);

    if ((err = ferror (source_file_p)))
    {
      break;
    }

    if (read_size > 0)
    {
      *written_p += (uint32_t) fwrite (buffer, 1, read_size, destination_file_p);

      if ((err = ferror (destination_file_p)))
      {
        break;
      }
    }

    if (read_size < sizeof (buffer))
    {
      break;
    }
  }

done:
  if (source_file_p != NULL)
  {
    fclose (source_file_p);
  }

  if (destination_file_p != NULL)
  {
    fclose (destination_file_p);
  }

  return err;
} /* fs_copy */

int fs_write (const char* path_p, uint8_t* buffer_p, uint32_t buffer_size, uint32_t* written_p)
{
  errno = 0;
  FILE* file_p = fopen (path_p, "wb");

  if (!file_p)
  {
    return errno;
  }

  if (buffer_size > 0)
  {
    *written_p = (uint32_t) fwrite (buffer_p, 1, buffer_size, file_p);
  }
  else
  {
    *written_p = 0;
  }

  fclose (file_p);

  if (*written_p != buffer_size)
  {
    return errno == 0 ? EIO : errno;
  }

  return 0;
} /* fs_write */

int fs_remove (const char* path)
{
  return remove (path);
} /* fs_remove */

const char* fs_errno_to_string(int errno_value)
{
#define XX(code, message) case code: return #code;
  switch (errno_value)
  {
    FS_ERRNO_MAP(XX)
    default: return "UNKNOWN";
  }
#undef XX
} /* fs_errno_to_string */

const char* fs_errno_message(int errno_value)
{
#define XX(code, message) case code: return message;
  switch (errno_value)
  {
    FS_ERRNO_MAP(XX)
    default: return "unknown error";
  }
#undef XX
} /* fs_errno_message */

#endif /* JJS_PACK_FS */
