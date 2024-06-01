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
#include <time.h>
#include <assert.h>
#include <stdio.h>

#include "jjs.h"

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  srand ((unsigned) time (NULL));

  jjs_context_t *context_p = NULL;
  jjs_status_t status = jjs_context_new (NULL, &context_p);

  if (status != JJS_STATUS_OK)
  {
    printf ("Failed to create JJS context: %i\n", status);
    return 1;
  }

  if (jjs_validate_string (context_p, (jjs_char_t *) data, (jjs_size_t) size, JJS_ENCODING_UTF8))
  {
    jjs_parse_options_t parse_options;
    parse_options.options = JJS_PARSE_NO_OPTS;
    jjs_value_t parse_value = jjs_parse (context_p, (jjs_char_t *) data, size, &parse_options);

    if (!jjs_value_is_exception (context_p, parse_value))
    {
      jjs_value_t run_value = jjs_run (context_p, parse_value);
      jjs_value_free (context_p, run_value);

      run_value = jjs_run_jobs (context_p);
      jjs_value_free (context_p, run_value);
    }

    jjs_value_free (context_p, parse_value);
  }

  jjs_context_free (context_p);

  return 0;
} /* LLVMFuzzerTestOneInput */
