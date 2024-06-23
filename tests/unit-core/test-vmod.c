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

#include "jjs-test.h"

const char* TEST_PACKAGE = "test";
const char* TEST_EXPORT = "test export";

static jjs_value_t
create_config (void)
{
  jjs_value_t config = jjs_object (ctx ());

  ctx_defer_free (jjs_object_set_sz (ctx (), config, "exports", ctx_cstr (TEST_EXPORT), JJS_KEEP));

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
  TEST_ASSERT (jjs_vmod_exists_sz (ctx (), package_name));

  jjs_value_t exports = ctx_defer_free (jjs_vmod_resolve_sz (ctx (), package_name));

  TEST_ASSERT (strict_equals_cstr (ctx (), exports, expected_export));
}

static void
test_jjs_vmod_with_callback (void)
{
  ctx_open (NULL);

  jjs_value_t result = jjs_vmod_sz (ctx (), TEST_PACKAGE, jjs_function_external (ctx (), vmod_callback), JJS_MOVE);

  ctx_defer_free (result);
  TEST_ASSERT (strict_equals (ctx (), result, jjs_undefined (ctx ())));
  assert_package (TEST_PACKAGE, TEST_EXPORT);

  ctx_close ();
}

static void
test_jjs_vmod_with_config (void)
{
  ctx_open (NULL);

  jjs_value_t result = ctx_defer_free (jjs_vmod_sz (ctx (), TEST_PACKAGE, create_config (), JJS_MOVE));

  TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
  TEST_ASSERT (jjs_vmod_exists_sz (ctx (), TEST_PACKAGE));
  assert_package (TEST_PACKAGE, TEST_EXPORT);

  ctx_close ();
}

static void
test_jjs_vmod_remove (void)
{
  ctx_open (NULL);

  jjs_value_t result = ctx_defer_free (jjs_vmod_sz (ctx (), TEST_PACKAGE, create_config (), JJS_MOVE));

  TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
  TEST_ASSERT (jjs_vmod_exists_sz (ctx (), TEST_PACKAGE));

  jjs_vmod_remove_sz (ctx (), TEST_PACKAGE);

  TEST_ASSERT (!jjs_vmod_exists_sz (ctx(), TEST_PACKAGE));

  ctx_close ();
}

TEST_MAIN({
  // note: api is more thoroughly tested in js
  test_jjs_vmod_with_callback ();
  test_jjs_vmod_with_config ();
  test_jjs_vmod_remove ();
})
