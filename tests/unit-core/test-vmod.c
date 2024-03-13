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
#define TEST_COMMON_IMPLEMENTATION
#include "test-common.h"

const char* TEST_PACKAGE = "test";
const char* TEST_EXPORT = "test export";

static jjs_value_t
create_config (void)
{
  jjs_value_t config = jjs_object ();
  jjs_value_t exports = jjs_string_sz (TEST_EXPORT);

  jjs_value_free (jjs_object_set_sz (config, "exports", exports));
  jjs_value_free (exports);

  return config;
}

static jjs_value_t
vmod_callback (const jjs_call_info_t* call_info_p, const jjs_value_t args_p[], jjs_length_t args_count)
{
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_p);
  JJS_UNUSED (args_count);

  return create_config ();
}

static void
assert_package (const char* package_name, const char* expected_export)
{
  TEST_ASSERT (jjs_vmod_exists_sz (package_name));

  jjs_value_t exports = jjs_vmod_resolve_sz (package_name);

  TEST_ASSERT (strict_equals_cstr (exports, expected_export));
  jjs_value_free (exports);
}

void
test_jjs_vmod_with_callback (void)
{
  jjs_init (JJS_INIT_EMPTY);

  jjs_value_t callback = jjs_function_external (vmod_callback);
  jjs_value_t result = jjs_vmod_sz (TEST_PACKAGE, callback);

  TEST_ASSERT (strict_equals (result, jjs_undefined ()));
  assert_package (TEST_PACKAGE, TEST_EXPORT);

  jjs_value_free (result);
  jjs_value_free (callback);

  jjs_cleanup ();
}

void
test_jjs_vmod_with_config (void)
{
  jjs_init (JJS_INIT_EMPTY);

  jjs_value_t config = create_config ();
  jjs_value_t result = jjs_vmod_sz (TEST_PACKAGE, config);

  TEST_ASSERT (jjs_vmod_exists_sz (TEST_PACKAGE));
  assert_package (TEST_PACKAGE, TEST_EXPORT);

  jjs_value_free (result);
  jjs_value_free (config);

  jjs_cleanup ();
}

void
test_jjs_vmod_remove (void)
{
  jjs_init (JJS_INIT_EMPTY);

  jjs_value_t config = create_config ();
  jjs_value_t result = jjs_vmod_sz (TEST_PACKAGE, config);

  TEST_ASSERT (jjs_vmod_exists_sz (TEST_PACKAGE));

  jjs_vmod_remove_sz (TEST_PACKAGE);

  TEST_ASSERT (!jjs_vmod_exists_sz (TEST_PACKAGE));

  jjs_value_free (result);
  jjs_value_free (config);

  jjs_cleanup ();
}

int
main (void)
{
  TEST_INIT ();

  // note: api is more thoroughly tested in js
  test_jjs_vmod_with_callback ();
  test_jjs_vmod_with_config ();
  test_jjs_vmod_remove ();

  return 0;
} /* main */
