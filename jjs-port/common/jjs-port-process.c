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

#include "jjs-port.h"

/**
 * Default implementation of jjs_port_fatal. Calls 'abort' if exit code is
 * non-zero, 'exit' otherwise.
 */
void JJS_ATTR_WEAK
jjs_port_fatal (jjs_fatal_code_t code) /**< cause of error */
{
  if (code != 0 && code != JJS_FATAL_OUT_OF_MEMORY)
  {
    abort ();
  }

  exit ((int) code);
} /* jjs_port_fatal */
