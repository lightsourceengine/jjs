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

#if defined(_WIN32)

#include <windows.h>
#include <stdio.h>
#include <assert.h>

/**
 * Windows implementation of jjs_port_hrtime.
 */
uint64_t jjs_port_hrtime (void)
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

  assert (scaled_frequency != 0);

  if (!QueryPerformanceCounter(&counter))
  {
    fprintf (stderr, "jjs_port_hrtime: %s: %i\n", "QueryPerformanceCounter", (int32_t) GetLastError());
    jjs_port_fatal (JJS_FATAL_FAILED_ASSERTION);
  }

  assert (counter.QuadPart != 0);

  return (uint64_t) ((double) counter.QuadPart / scaled_frequency);
} /* jjs_port_hrtime */

/**
 * Default implementation of jjs_port_sleep, uses 'Sleep'.
 */
void
jjs_port_sleep (uint32_t sleep_time) /**< milliseconds to sleep */
{
  Sleep (sleep_time);
} /* jjs_port_sleep */

#endif /* defined(_WIN32) */
