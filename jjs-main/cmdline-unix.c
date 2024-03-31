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

#include "cmdline.h"

#ifdef CMDLINE_IS_UNIX

#include <stdlib.h>
#include <sys/time.h>

void cmdline_srand_init (void)
{
  struct timeval time;

  if (gettimeofday (&time, NULL) == 0)
  {
    srand (((time.tv_sec * 1000) + (time.tv_usec / 1000)) & 0xFFFFFFFF);
  }
}

#endif /* CMDLINE_IS_UNIX */
