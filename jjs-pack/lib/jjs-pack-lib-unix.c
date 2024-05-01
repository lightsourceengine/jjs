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

#if defined(JJS_OS_IS_UNIX)

#ifdef JJS_OS_IS_MACOS
#include <mach/mach_time.h>

uint64_t
jjs_pack_platform_hrtime (void)
{
  // adapted from uv_hrtime(): https://github.com/libuv/libuv/src/unix/darwin.c
  static uint64_t timebase = 0;
  static uint64_t (*hrtime_fn) (void) = NULL;

  if (hrtime_fn == NULL)
  {
    mach_timebase_info_data_t hrtime_timebase = {0};

    if (KERN_SUCCESS != mach_timebase_info (&hrtime_timebase))
    {
      jjs_platform_fatal (NULL, JJS_FATAL_FAILED_ASSERTION);
      return 0;
    }

    timebase = hrtime_timebase.numer / hrtime_timebase.denom;
    hrtime_fn = &mach_continuous_time;

    if (hrtime_fn == NULL)
    {
      hrtime_fn = &mach_absolute_time;
    }
  }

  return hrtime_fn () * timebase;
}

#else
#include <time.h>

uint64_t
jjs_pack_platform_hrtime (void)
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

#include <time.h>
#include <sys/time.h>

double jjs_pack_platform_date_now (void)
{
  struct timeval tv;

  if (gettimeofday (&tv, NULL) != 0)
  {
    return 0;
  }
  else
  {
    return ((double) tv.tv_sec) * 1000.0 + ((double) tv.tv_usec) / 1000.0;
  }
}

#endif /* JJS_OS_IS_UNIX */
