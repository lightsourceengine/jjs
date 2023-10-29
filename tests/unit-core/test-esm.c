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

#define TRY_JJS_ESM_IMPORT(VALUE)                  \
  do                                               \
  {                                                \
    jjs_value_t value = VALUE;                     \
    jjs_value_t result = jjs_esm_import (value);   \
    TEST_ASSERT (jjs_value_is_exception (result)); \
    jjs_value_free (result);                       \
    jjs_value_free (value);                        \
  } while (0)

static void
test_invalid_jjs_esm_import_arg (void)
{
  TRY_JJS_ESM_IMPORT (jjs_null ());
  TRY_JJS_ESM_IMPORT (jjs_undefined ());
  TRY_JJS_ESM_IMPORT (jjs_number (0));
  TRY_JJS_ESM_IMPORT (jjs_boolean (true));
  TRY_JJS_ESM_IMPORT (jjs_object ());
  TRY_JJS_ESM_IMPORT (jjs_array (0));
  TRY_JJS_ESM_IMPORT (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG));
}

#define TRY_JJS_ESM_IMPORT_SZ(VALUE)                \
  do                                                \
  {                                                 \
    jjs_value_t result = jjs_esm_import_sz (VALUE); \
    TEST_ASSERT (jjs_value_is_exception (result));  \
    jjs_value_free (result);                        \
  } while (0)

static void
test_invalid_jjs_esm_import_sz_arg (void)
{
  TRY_JJS_ESM_IMPORT_SZ (NULL);
  TRY_JJS_ESM_IMPORT_SZ ("");
  TRY_JJS_ESM_IMPORT_SZ ("unknown");
  TRY_JJS_ESM_IMPORT_SZ ("./unknown");
  TRY_JJS_ESM_IMPORT_SZ ("../unknown");
  TRY_JJS_ESM_IMPORT_SZ ("/unknown");
}

#define TRY_JJS_ESM_EVALUATE(VALUE)                \
  do                                               \
  {                                                \
    jjs_value_t value = VALUE;                     \
    jjs_value_t result = jjs_esm_evaluate (value); \
    TEST_ASSERT (jjs_value_is_exception (result)); \
    jjs_value_free (result);                       \
    jjs_value_free (value);                        \
  } while (0)

static void
test_invalid_jjs_esm_evaluate_arg (void)
{
  TRY_JJS_ESM_EVALUATE (jjs_null ());
  TRY_JJS_ESM_EVALUATE (jjs_undefined ());
  TRY_JJS_ESM_EVALUATE (jjs_number (0));
  TRY_JJS_ESM_EVALUATE (jjs_boolean (true));
  TRY_JJS_ESM_EVALUATE (jjs_object ());
  TRY_JJS_ESM_EVALUATE (jjs_array (0));
  TRY_JJS_ESM_EVALUATE (jjs_symbol (JJS_SYMBOL_TO_STRING_TAG));
}

#define TRY_JJS_ESM_EVALUATE_SZ(VALUE)                \
  do                                                  \
  {                                                   \
    jjs_value_t result = jjs_esm_evaluate_sz (VALUE); \
    TEST_ASSERT (jjs_value_is_exception (result));    \
    jjs_value_free (result);                          \
  } while (0)

static void
test_invalid_jjs_esm_evaluate_sz_arg (void)
{
  TRY_JJS_ESM_EVALUATE_SZ (NULL);
  TRY_JJS_ESM_EVALUATE_SZ ("");
  TRY_JJS_ESM_EVALUATE_SZ ("unknown");
  TRY_JJS_ESM_EVALUATE_SZ ("./unknown");
  TRY_JJS_ESM_EVALUATE_SZ ("../unknown");
  TRY_JJS_ESM_EVALUATE_SZ ("/unknown");
}

int
main (void)
{
  TEST_INIT ();

  jjs_init (JJS_INIT_EMPTY);

  test_invalid_jjs_esm_import_arg ();
  test_invalid_jjs_esm_import_sz_arg ();
  test_invalid_jjs_esm_evaluate_arg ();
  test_invalid_jjs_esm_evaluate_sz_arg ();

  jjs_cleanup ();
  return 0;
} /* main */
