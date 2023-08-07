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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zephyr.h>

#include "jjs-port.h"

#include "getline-zephyr.h"

/**
 * Aborts the program.
 */
void
jjs_port_fatal (jjs_fatal_code_t code)
{
  exit ((int) code);
} /* jjs_port_fatal */

/**
 * Dummy function to get the time zone adjustment.
 *
 * @return 0
 */
int32_t
jjs_port_local_tza (double unix_ms)
{
  (void) unix_ms;

  /* We live in UTC. */
  return 0;
} /* jjs_port_local_tza */

/**
 * Dummy function to get the current time.
 *
 * @return 0
 */
double
jjs_port_current_time (void)
{
  int64_t ms = k_uptime_get ();
  return (double) ms;
} /* jjs_port_current_time */

/**
 * Provide implementation of jjs_port_sleep.
 */
void
jjs_port_sleep (uint32_t sleep_time) /**< milliseconds to sleep */
{
  k_usleep ((useconds_t) sleep_time * 1000);
} /* jjs_port_sleep */

/**
 * Read line from stdin
 */
jjs_char_t *
jjs_port_line_read (jjs_size_t *out_size_p)
{
  char *line_p = zephyr_getline ();

  *out_size_p = (jjs_size_t) strlen (line_p);
  return (jjs_char_t *) line_p;
} /* jjs_port_line_read */

/**
 * Free line, no-op.
 */
void
jjs_port_line_free (jjs_char_t *line_p)
{
  (void) line_p;
} /* jjs_port_line_free */
