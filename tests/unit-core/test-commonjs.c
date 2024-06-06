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

#define TRY_JJS_COMMONJS_REQUIRE(VALUE)                                  \
  do                                                                     \
  {                                                                      \
    jjs_value_t value = VALUE;                                           \
    jjs_value_t result = jjs_commonjs_require (ctx (), value, JJS_MOVE); \
    TEST_ASSERT (jjs_value_is_exception (ctx (), result));               \
    jjs_value_free (ctx (), result);                                     \
  } while (0)

static void
test_invalid_jjs_commonjs_require_arg (void)
{
  TRY_JJS_COMMONJS_REQUIRE (jjs_null (ctx ()));
  TRY_JJS_COMMONJS_REQUIRE (jjs_undefined (ctx ()));
  TRY_JJS_COMMONJS_REQUIRE (jjs_number (ctx (), 0));
  TRY_JJS_COMMONJS_REQUIRE (jjs_boolean (ctx (), true));
  TRY_JJS_COMMONJS_REQUIRE (jjs_object (ctx ()));
  TRY_JJS_COMMONJS_REQUIRE (jjs_array (ctx (), 0));
  TRY_JJS_COMMONJS_REQUIRE (jjs_symbol_get_well_known (ctx (), JJS_SYMBOL_TO_STRING_TAG));
}

#define TRY_JJS_COMMONJS_REQUIRE_SZ(VALUE)                        \
  do                                                              \
  {                                                               \
    jjs_value_t result = jjs_commonjs_require_sz (ctx (), VALUE); \
    TEST_ASSERT (jjs_value_is_exception (ctx (), result));        \
    jjs_value_free (ctx (), result);                              \
  } while (0)

static void
test_invalid_jjs_commonjs_require_sz_arg (void)
{
  TRY_JJS_COMMONJS_REQUIRE_SZ (NULL);
  TRY_JJS_COMMONJS_REQUIRE_SZ ("");
  TRY_JJS_COMMONJS_REQUIRE_SZ ("unknown");
  TRY_JJS_COMMONJS_REQUIRE_SZ ("./unknown");
  TRY_JJS_COMMONJS_REQUIRE_SZ ("../unknown");
  TRY_JJS_COMMONJS_REQUIRE_SZ ("/unknown");
}

int
main (void)
{
  ctx_open (NULL);

  // note: it is slightly difficult to test filesystem operations
  //       from these unit tests. mostly negative tests are done here.
  //       positive tests are done in js tests.

  test_invalid_jjs_commonjs_require_arg ();
  test_invalid_jjs_commonjs_require_sz_arg ();

  ctx_close ();
  return 0;
} /* main */
