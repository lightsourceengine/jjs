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

#include "jjs-test.h"

int
main (void)
{
  if (!jjs_feature_enabled (JJS_FEATURE_HEAP_STATS))
  {
    return 0;
  }
  const char test_source[] = TEST_STRING_LITERAL ("var a = 'hello';"
                                                          "var b = 'world';"
                                                          "var c = a + ' ' + b;");

  ctx_open (NULL);

  jjs_value_t parsed_code_val = jjs_parse_sz (ctx (), test_source, NULL);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), parsed_code_val));

  jjs_value_t res = jjs_run (ctx (), parsed_code_val, JJS_MOVE);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), res));

  jjs_heap_stats_t stats;
  memset (&stats, 0, sizeof (stats));
  bool get_stats_ret = jjs_heap_stats (ctx (), &stats);
  TEST_ASSERT (get_stats_ret);
  TEST_ASSERT (stats.version == 1);
  // TODO: not sure where this number comes from, but x86 linux builds with ubsan
  // are 8 bytes less than every other build configuration. Needs to be 
  // investigated.
  TEST_ASSERT (stats.size == 524280 || stats.size == 524272);

  TEST_ASSERT (!jjs_heap_stats (ctx (), NULL));

  jjs_value_free (ctx (), res);

  ctx_close ();

  return 0;
} /* main */
