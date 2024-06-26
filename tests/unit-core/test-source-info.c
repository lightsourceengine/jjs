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

static void
compare_string (jjs_value_t left_string, /**< left string */
                const char *right_string_p) /**< right string */
{
  size_t size = strlen (right_string_p);
  uint8_t buffer[64];

  TEST_ASSERT (size == jjs_string_size (ctx (), left_string, JJS_ENCODING_CESU8));
  TEST_ASSERT (size < sizeof (buffer));
  TEST_ASSERT (jjs_string_to_buffer (ctx (), left_string, JJS_ENCODING_CESU8, buffer, (jjs_size_t) size) == size);
  TEST_ASSERT (memcmp (buffer, right_string_p, size) == 0);
} /* compare_string */

int
main (void)
{
  ctx_open (NULL);

  if (!jjs_feature_enabled (JJS_FEATURE_FUNCTION_TO_STRING))
  {
    jjs_log (ctx (), JJS_LOG_LEVEL_ERROR, "Source code is not stored!\n");
    ctx_close ();
    return 0;
  }

  jjs_value_t value = jjs_null (ctx ());
  jjs_source_info_t *source_info_p = jjs_source_info (ctx (), value);
  TEST_ASSERT (source_info_p == NULL);
  jjs_source_info_free (ctx (), source_info_p);
  jjs_value_free (ctx (), value);

  value = jjs_object (ctx ());
  source_info_p = jjs_source_info (ctx (), value);
  TEST_ASSERT (source_info_p == NULL);
  jjs_source_info_free (ctx (), source_info_p);
  jjs_value_free (ctx (), value);

  jjs_parse_options_t parse_options;
  const char *source_p = TEST_STRING_LITERAL ("var a = 6");

  value = jjs_parse_sz (ctx (), source_p, NULL);
  source_info_p = jjs_source_info (ctx (), value);
  TEST_ASSERT (source_info_p != NULL);
  TEST_ASSERT (source_info_p->enabled_fields == JJS_SOURCE_INFO_HAS_SOURCE_CODE);
  compare_string (source_info_p->source_code, source_p);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), source_info_p->function_arguments));
  TEST_ASSERT (source_info_p->source_range_start == 0);
  TEST_ASSERT (source_info_p->source_range_length == 0);
  jjs_source_info_free (ctx (), source_info_p);
  jjs_value_free (ctx (), value);

  if (jjs_feature_enabled (JJS_FEATURE_MODULE))
  {
    parse_options = (jjs_parse_options_t) {
      .parse_module = true,
    };

    value = jjs_parse_sz (ctx (), source_p, &parse_options);

    jjs_value_t result = jjs_module_link (ctx (), value, NULL, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
    jjs_value_free (ctx (), result);

    source_info_p = jjs_source_info (ctx (), value);
    TEST_ASSERT (source_info_p != NULL);
    TEST_ASSERT (source_info_p->enabled_fields == JJS_SOURCE_INFO_HAS_SOURCE_CODE);
    compare_string (source_info_p->source_code, source_p);
    TEST_ASSERT (jjs_value_is_undefined (ctx (), source_info_p->function_arguments));
    TEST_ASSERT (source_info_p->source_range_start == 0);
    TEST_ASSERT (source_info_p->source_range_length == 0);
    jjs_source_info_free (ctx (), source_info_p);

    result = jjs_module_evaluate (ctx (), value);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
    jjs_value_free (ctx (), result);

    /* Byte code is released after a successful evaluation. */
    source_info_p = jjs_source_info (ctx (), value);
    TEST_ASSERT (source_info_p == NULL);
    jjs_source_info_free (ctx (), source_info_p);
    jjs_value_free (ctx (), value);
  }

  source_p = TEST_STRING_LITERAL ("( function f() {} )");

  value = jjs_eval_sz (ctx (), source_p, 0);
  source_info_p = jjs_source_info (ctx (), value);
  TEST_ASSERT (source_info_p != NULL);
  TEST_ASSERT (source_info_p->enabled_fields
               == (JJS_SOURCE_INFO_HAS_SOURCE_CODE | JJS_SOURCE_INFO_HAS_SOURCE_RANGE));
  compare_string (source_info_p->source_code, source_p);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), source_info_p->function_arguments));
  TEST_ASSERT (source_info_p->source_range_start == 2);
  TEST_ASSERT (source_info_p->source_range_length == 15);
  jjs_source_info_free (ctx (), source_info_p);
  jjs_value_free (ctx (), value);

  source_p = TEST_STRING_LITERAL ("new Function('a', 'b', 'return 0;')");

  value = jjs_eval_sz (ctx (), source_p, 0);
  source_info_p = jjs_source_info (ctx (), value);
  TEST_ASSERT (source_info_p != NULL);
  TEST_ASSERT (source_info_p->enabled_fields
               == (JJS_SOURCE_INFO_HAS_SOURCE_CODE | JJS_SOURCE_INFO_HAS_FUNCTION_ARGUMENTS));
  compare_string (source_info_p->source_code, "return 0;");
  compare_string (source_info_p->function_arguments, "a,b");
  TEST_ASSERT (source_info_p->source_range_start == 0);
  TEST_ASSERT (source_info_p->source_range_length == 0);
  jjs_source_info_free (ctx (), source_info_p);
  jjs_value_free (ctx (), value);

  source_p = TEST_STRING_LITERAL ("(new Function('a = ( function() { } )', 'return a;'))()");

  value = jjs_eval_sz (ctx (), source_p, 0);
  source_info_p = jjs_source_info (ctx (), value);
  TEST_ASSERT (source_info_p != NULL);
  TEST_ASSERT (source_info_p->enabled_fields
               == (JJS_SOURCE_INFO_HAS_SOURCE_CODE | JJS_SOURCE_INFO_HAS_SOURCE_RANGE));
  compare_string (source_info_p->source_code, "a = ( function() { } )");
  TEST_ASSERT (jjs_value_is_undefined (ctx (), source_info_p->function_arguments));
  TEST_ASSERT (source_info_p->source_range_start == 6);
  TEST_ASSERT (source_info_p->source_range_length == 14);
  jjs_source_info_free (ctx (), source_info_p);
  jjs_value_free (ctx (), value);

  source_p = TEST_STRING_LITERAL ("(function f(a) { return 7 }).bind({})");

  value = jjs_eval_sz (ctx (), source_p, 0);
  source_info_p = jjs_source_info (ctx (), value);
  TEST_ASSERT (source_info_p != NULL);
  TEST_ASSERT (source_info_p->enabled_fields
               == (JJS_SOURCE_INFO_HAS_SOURCE_CODE | JJS_SOURCE_INFO_HAS_SOURCE_RANGE));
  compare_string (source_info_p->source_code, source_p);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), source_info_p->function_arguments));
  TEST_ASSERT (source_info_p->source_range_start == 1);
  TEST_ASSERT (source_info_p->source_range_length == 26);
  jjs_source_info_free (ctx (), source_info_p);
  jjs_value_free (ctx (), value);

  ctx_close ();
  return 0;
} /* main */
