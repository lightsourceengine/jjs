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

typedef struct
{
  jjs_type_t type_info;
  jjs_value_t value;
} test_entry_t;

#define ENTRY(TYPE, VALUE) \
  {                        \
    TYPE, VALUE            \
  }

static jjs_value_t
test_ext_function (const jjs_call_info_t *call_info_p, /**< call information */
                   const jjs_value_t args_p[], /**< array of arguments */
                   const jjs_length_t args_cnt) /**< number of arguments */
{
  (void) call_info_p;
  (void) args_p;
  (void) args_cnt;
  return jjs_boolean (ctx (), true);
} /* test_ext_function */

int
main (void)
{
  ctx_open (NULL);

  const char test_eval_function[] = "function demo(a) { return a + 1; }; demo";

  test_entry_t entries[] = {
    ENTRY (JJS_TYPE_NUMBER, jjs_number (ctx (), -33.0)),
    ENTRY (JJS_TYPE_NUMBER, jjs_number (ctx (), 3)),
    ENTRY (JJS_TYPE_NUMBER, jjs_nan (ctx ())),
    ENTRY (JJS_TYPE_NUMBER, jjs_infinity (ctx (), false)),
    ENTRY (JJS_TYPE_NUMBER, jjs_infinity (ctx (), true)),

    ENTRY (JJS_TYPE_BOOLEAN, jjs_boolean (ctx (), true)),
    ENTRY (JJS_TYPE_BOOLEAN, jjs_boolean (ctx (), false)),

    ENTRY (JJS_TYPE_UNDEFINED, jjs_undefined (ctx ())),

    ENTRY (JJS_TYPE_OBJECT, jjs_object (ctx ())),
    ENTRY (JJS_TYPE_OBJECT, jjs_array (ctx (), 10)),
    ENTRY (JJS_TYPE_EXCEPTION, jjs_throw_sz (ctx (), JJS_ERROR_TYPE, "error")),

    ENTRY (JJS_TYPE_NULL, jjs_null (ctx ())),

    ENTRY (JJS_TYPE_FUNCTION,
           jjs_eval (ctx (), (jjs_char_t *) test_eval_function, sizeof (test_eval_function) - 1, JJS_PARSE_NO_OPTS)),
    ENTRY (JJS_TYPE_FUNCTION, jjs_function_external (ctx (), test_ext_function)),

    ENTRY (JJS_TYPE_STRING, jjs_string_sz (ctx (), test_eval_function)),
    ENTRY (JJS_TYPE_STRING, jjs_string_sz (ctx (), "")),
  };

  for (size_t idx = 0; idx < sizeof (entries) / sizeof (entries[0]); idx++)
  {
    jjs_type_t type_info = jjs_value_type (ctx (), entries[idx].value);

    TEST_ASSERT (type_info != JJS_TYPE_NONE);
    TEST_ASSERT (type_info == entries[idx].type_info);

    jjs_value_free (ctx (), entries[idx].value);
  }

  jjs_value_t symbol_value = jjs_symbol_with_description_sz (ctx (), "foo");
  jjs_type_t type_info = jjs_value_type (ctx (), symbol_value);

  TEST_ASSERT (type_info != JJS_TYPE_NONE);
  TEST_ASSERT (type_info == JJS_TYPE_SYMBOL);

  jjs_value_free (ctx (), symbol_value);

  if (jjs_feature_enabled (JJS_FEATURE_BIGINT))
  {
    /* Check simple bigint value type */
    uint64_t digits_buffer[2] = { 1, 0 };
    jjs_value_t value_bigint = jjs_bigint (ctx (), digits_buffer, 2, false);
    jjs_type_t value_type_info = jjs_value_type (ctx (), value_bigint);

    TEST_ASSERT (value_type_info != JJS_TYPE_NONE);
    TEST_ASSERT (value_type_info == JJS_TYPE_BIGINT);

    jjs_value_free (ctx (), value_bigint);

    /* Check bigint wrapped in object type */
    jjs_char_t object_bigint_src[] = "Object(5n)";
    jjs_value_t object_bigint = jjs_eval (ctx (), object_bigint_src, sizeof (object_bigint_src) - 1, JJS_PARSE_NO_OPTS);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), object_bigint));

    jjs_type_t object_type_info = jjs_value_type (ctx (), object_bigint);

    TEST_ASSERT (object_type_info != JJS_TYPE_NONE);
    TEST_ASSERT (object_type_info == JJS_TYPE_OBJECT);

    jjs_value_free (ctx (), object_bigint);
  }

  if (jjs_feature_enabled (JJS_FEATURE_REALM))
  {
    jjs_value_t new_realm = jjs_realm (ctx ());
    jjs_value_t old_realm = jjs_set_realm (ctx (), new_realm);

    jjs_type_t new_realm_type = jjs_value_type (ctx (), new_realm);
    TEST_ASSERT (new_realm_type == JJS_TYPE_OBJECT);

    jjs_value_t new_realm_this = jjs_realm_this (ctx (), new_realm);
    jjs_type_t new_realm_this_type = jjs_value_type (ctx (), new_realm_this);
    TEST_ASSERT (new_realm_this_type == JJS_TYPE_OBJECT);
    jjs_value_free (ctx (), new_realm_this);

    jjs_type_t old_realm_type = jjs_value_type (ctx (), old_realm);
    TEST_ASSERT (old_realm_type == JJS_TYPE_OBJECT);

    jjs_value_free (ctx (), new_realm);

    jjs_value_t old_realm_this = jjs_realm_this (ctx (), old_realm);
    jjs_type_t old_realm_this_type = jjs_value_type (ctx (), old_realm_this);
    TEST_ASSERT (old_realm_this_type == JJS_TYPE_OBJECT);
    jjs_value_free (ctx (), old_realm_this);

    /* Restore the old realm as per docs */
    jjs_set_realm (ctx (), old_realm);
  }

  {
    jjs_value_t ex = jjs_throw_sz (ctx (), JJS_ERROR_COMMON, "error");

    TEST_ASSERT (jjs_value_free_unless (ctx (), ex, jjs_value_is_exception) == false);
    jjs_value_free (ctx (), ex);

    jjs_value_t obj = jjs_object (ctx ());

    TEST_ASSERT (jjs_value_free_unless (ctx (), obj, jjs_value_is_exception) == true);
  }

  ctx_close ();

  return 0;
} /* main */
