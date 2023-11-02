/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "jjs-port.h"

#if defined(__unix__)
#include <time.h>

/**
 * Linux implementation of jjs_port_hrtime.
 */
uint64_t jjs_port_hrtime (void)
{
  // adapted from uv_hrtime(): https://github.com/libuv/libuv/src/unix/linux.c

  static clock_t clock_id = -1;
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
} /* jjs_port_hrtime */

#endif /* defined(__unix__) */

#if defined(__APPLE__)
#include <mach/mach_time.h>

/**
 * MacOS implementation of jjs_port_hrtime.
 */
uint64_t jjs_port_hrtime (void)
{
  // adapted from uv_hrtime(): https://github.com/libuv/libuv/src/unix/darwin.c
  static mach_timebase_info_data_t hrtime_timebase = {0};
  static uint64_t (*hrtime_fn) (void) = NULL;

  if (hrtime_fn == NULL)
  {
    if (KERN_SUCCESS != mach_timebase_info (&hrtime_timebase))
    {
      jjs_port_fatal (JJS_FATAL_FAILED_ASSERTION);
    }

    hrtime_fn = &mach_continuous_time;

    if (hrtime_fn == NULL)
    {
      hrtime_fn = &mach_absolute_time;
    }
  }

  return hrtime_fn () * hrtime_timebase.numer / hrtime_timebase.denom;
} /* jjs_port_hrtime */

#endif /* defined(__APPLE__) */

#if defined(__unix__) || defined(__APPLE__)

#include <unistd.h>

/**
 * Default implementation of jjs_port_sleep, uses 'usleep'.
 */
void
jjs_port_sleep (uint32_t sleep_time) /**< milliseconds to sleep */
{
  usleep ((useconds_t) sleep_time * 1000);
} /* jjs_port_sleep */

#endif /* defined(__unix__) || defined(__APPLE__) */
