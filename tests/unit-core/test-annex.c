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

#include "jjs.h"

#include "config.h"
#include "annex.h"
#define TEST_COMMON_IMPLEMENTATION
#include "test-common.h"

static void
test_annex_system_info_props (void)
{
  jjs_value_t realm = jjs_current_realm ();
  jjs_value_t platform = jjs_object_get_sz (realm, "@platform");
  jjs_value_t arch = jjs_object_get_sz (realm, "@arch");

  // ensure a platform was chosen
  TEST_ASSERT (jjs_string_length (platform) > 0);
  TEST_ASSERT (!strict_equals_cstr(platform, "unknown"));

  // ensure a arch was chosen
  TEST_ASSERT (jjs_string_length (arch) > 0);
  TEST_ASSERT (!strict_equals_cstr(arch, "unknown"));

  jjs_value_free (arch);
  jjs_value_free (platform);
  jjs_value_free (realm);
}

int
main (void)
{
  TEST_INIT ();

  jjs_init (JJS_INIT_EMPTY);

  test_annex_system_info_props ();
  
  jjs_cleanup ();
  return 0;
} /* main */
