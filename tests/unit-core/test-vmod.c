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
#include "test-common.h"

static int32_t i = 1;

static bool
string_eq (jjs_value_t lhs, const char* rhs_p)
{
  jjs_value_t rhs = jjs_string_sz (rhs_p);
  jjs_value_t result = jjs_binary_op (JJS_BIN_OP_STRICT_EQUAL, lhs, rhs);

  jjs_value_free (rhs);
  jjs_value_free (result);

  return jjs_value_is_true (result);
}

static jjs_value_t
vmod_create_test (jjs_value_t name, void* user_p)
{
  TEST_ASSERT (string_eq (name, "test"));
  TEST_ASSERT (user_p == NULL);

  jjs_value_t obj = jjs_object ();
  jjs_value_t export = jjs_string_sz ("test_export");

  jjs_value_free (jjs_object_set_sz (obj, "exports", export));
  jjs_value_free (export);

  return obj;
}

static jjs_value_t
vmod_create_test_context (jjs_value_t name, void* user_p)
{
  TEST_ASSERT (string_eq (name, "test"));
  TEST_ASSERT (user_p == &i);

  jjs_value_t obj = jjs_object ();
  jjs_value_t export = jjs_string_sz ("test_export");

  jjs_value_free (jjs_object_set_sz (obj, "exports", export));
  jjs_value_free (export);

  return obj;
}

static jjs_value_t
vmod_create_throws (jjs_value_t name, void* user_p)
{
  JJS_UNUSED (name);
  JJS_UNUSED (user_p);
  return jjs_throw_sz (JJS_ERROR_TYPE, "just an exception");
}

static jjs_value_t
vmod_create_no_exports (jjs_value_t name, void* user_p)
{
  JJS_UNUSED (name);
  JJS_UNUSED (user_p);
  return jjs_object ();
}

static jjs_value_t
test_create_handler (const jjs_call_info_t* call_info_p, const jjs_value_t args_p[], jjs_length_t args_count)
{
  JJS_UNUSED (call_info_p);

  TEST_ASSERT (args_count == 1);
  TEST_ASSERT (jjs_value_is_string (args_p[0]));
  TEST_ASSERT (string_eq (args_p[0], "test"));

  jjs_value_t obj = jjs_object ();
  jjs_value_t export = jjs_string_sz ("test_export");

  jjs_value_free (jjs_object_set_sz (obj, "exports", export));
  jjs_value_free (export);

  return obj;
}

static void
test_jjs_vmod (void)
{
  jjs_value_t result;

  jjs_init (JJS_INIT_EMPTY);

  jjs_value_t name = jjs_string_sz ("test");
  jjs_value_t fn = jjs_function_external (test_create_handler);

  result = jjs_vmod (jjs_undefined (), jjs_undefined ());
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_vmod (name, jjs_undefined ());
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_vmod (name, fn);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  jjs_value_t exports = jjs_commonjs_require_sz ("test");

  TEST_ASSERT (string_eq (exports, "test_export"));
  jjs_value_free (exports);

  jjs_value_free (fn);
  jjs_value_free (name);

  jjs_cleanup ();
}

static void
test_jjs_vmod_native (void)
{
  jjs_value_t result;
  jjs_value_t name;

  jjs_init (JJS_INIT_EMPTY);

  name = jjs_string_sz ("");
  result = jjs_vmod_native (name, &vmod_create_test_context, &i);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);
  jjs_value_free (name);

  result = jjs_vmod_native (jjs_undefined (), &vmod_create_test_context, &i);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_vmod_native (jjs_null (), &vmod_create_test_context, &i);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  name = jjs_number (0);
  result = jjs_vmod_native (name, &vmod_create_test_context, &i);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);
  jjs_value_free (name);

  name = jjs_string_sz ("test");
  result = jjs_vmod_native (name, NULL, &i);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);
  jjs_value_free (name);

  name = jjs_string_sz ("test");
  result = jjs_vmod_native (name, &vmod_create_test_context, &i);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);
  jjs_value_free (name);

  jjs_value_t exports = jjs_commonjs_require_sz ("test");

  TEST_ASSERT (string_eq (exports, "test_export"));
  jjs_value_free (exports);

  jjs_cleanup ();
}

static void
test_jjs_vmod_native_sz (void)
{
  jjs_value_t result;

  jjs_init (JJS_INIT_EMPTY);

  result = jjs_vmod_native_sz (NULL, &vmod_create_test, NULL);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_vmod_native_sz ("", &vmod_create_test, NULL);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_vmod_native_sz ("test", NULL, NULL);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_vmod_native_sz ("test", &vmod_create_test, NULL);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  jjs_value_t exports = jjs_commonjs_require_sz ("test");

  TEST_ASSERT (string_eq (exports, "test_export"));
  jjs_value_free (exports);

  jjs_cleanup ();
}

static void
test_bad_create_function (void)
{
  jjs_value_t result;

  jjs_init (JJS_INIT_EMPTY);

  result = jjs_vmod_native_sz ("test", &vmod_create_throws, NULL);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_commonjs_require_sz ("test");
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  jjs_cleanup ();
  jjs_init (JJS_INIT_EMPTY);

  result = jjs_vmod_native_sz ("test", &vmod_create_no_exports, NULL);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_commonjs_require_sz ("test");
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  jjs_cleanup ();
}

static void
test_jjs_vmod_exists (void)
{
  jjs_init (JJS_INIT_EMPTY);

  jjs_value_t name = jjs_string_sz ("test");

  TEST_ASSERT (jjs_vmod_exists (name) == false);

  jjs_value_t result = jjs_vmod_native (name, &vmod_create_test, NULL);
  TEST_ASSERT (!jjs_value_is_exception (result));

  TEST_ASSERT (jjs_vmod_exists (name));

  jjs_value_free (name);

  jjs_cleanup ();
}

static void
test_jjs_vmod_exists_sz (void)
{
  jjs_init (JJS_INIT_EMPTY);

  TEST_ASSERT (jjs_vmod_exists_sz ("test") == false);

  jjs_value_t result = jjs_vmod_native_sz ("test", &vmod_create_test, NULL);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  TEST_ASSERT (jjs_vmod_exists_sz ("test"));

  jjs_cleanup ();
}

int
main (void)
{
  TEST_INIT ();

  test_jjs_vmod ();
  test_jjs_vmod_native ();
  test_jjs_vmod_native_sz ();
  test_bad_create_function ();
  test_jjs_vmod_exists ();
  test_jjs_vmod_exists_sz ();

  return 0;
} /* main */
