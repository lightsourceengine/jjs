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

static void
check_fmt_to_string (const char* format_p, jjs_value_t value, const char* expected)
{
  ctx_assert_strict_equals (ctx_defer_free (jjs_fmt_to_string_v (ctx (), format_p, &value, 1)), ctx_cstr (expected));
}

static void
check_fmt_to_buffer (const char* format_p, jjs_value_t value, const char* expected)
{
  char buffer[256];
  jjs_size_t written =
    jjs_fmt_to_buffer_v (ctx (), (jjs_char_t*) buffer, JJS_ARRAY_SIZE (buffer), JJS_ENCODING_UTF8, format_p, &value, 1);

  buffer[written] = '\0';

  TEST_ASSERT (strlen (buffer) == strlen (expected));
  TEST_ASSERT (strcmp (buffer, expected) == 0);
}

static void
test_fmt_to_function (void (*check) (const char*, jjs_value_t, const char*))
{
  jjs_value_t array = ctx_array (2);

  ctx_defer_free (jjs_object_set_index (ctx (), array, 0, ctx_number (1), JJS_KEEP));
  ctx_defer_free (jjs_object_set_index (ctx (), array, 1, ctx_number (2), JJS_KEEP));

  check ("{}", ctx_null (), "null");
  check ("{}", ctx_undefined (), "undefined");
  check ("{}", ctx_object (), "[object Object]");
  check ("{}", ctx_array (0), "[]");
  check ("{}", array, "[1,2]");
  check ("{}", ctx_symbol ("desc"), "Symbol(desc)");
  check ("{}", ctx_cstr ("hello"), "hello");
  check ("{}", ctx_number (100), "100");

  check ("", ctx_number (100), "");
  check ("no format", ctx_number (100), "no format");
  check ("{}:{}", ctx_number (100), "100:undefined");
}

static void
test_fmt_to_string (void)
{
  test_fmt_to_function (&check_fmt_to_string);

  jjs_value_t values[] = {
    ctx_number (1),
    ctx_number (2),
    ctx_number (3),
  };

  jjs_value_t formatted = jjs_fmt_to_string_v (ctx (), "{}{}{}", values, JJS_ARRAY_SIZE (values));

  ctx_assert_strict_equals (ctx_defer_free (formatted), ctx_cstr ("123"));
}

static void
test_fmt_to_buffer (void)
{
  test_fmt_to_function (&check_fmt_to_buffer);

  jjs_value_t values[] = {
    ctx_number (1),
    ctx_number (2),
    ctx_number (3),
  };

  char buffer[256];
  jjs_size_t written = jjs_fmt_to_buffer_v (ctx (),
                                            (jjs_char_t*) buffer,
                                            JJS_ARRAY_SIZE (buffer),
                                            JJS_ENCODING_UTF8,
                                            "{}{}{}",
                                            values,
                                            JJS_ARRAY_SIZE (values));

  buffer[written] = '\0';

  TEST_ASSERT (strcmp (buffer, "123") == 0);
}

static void
test_fmt_join (void)
{
  jjs_value_t values[] = {
    ctx_number (1),
    ctx_number (2),
    ctx_number (3),
  };

  jjs_value_t joined = jjs_fmt_join_v (ctx (), jjs_string_sz (ctx (), ", "), JJS_MOVE, values, JJS_ARRAY_SIZE (values));

  ctx_assert_strict_equals (ctx_defer_free (joined), ctx_cstr ("1, 2, 3"));
}

static void
test_fmt_logging (void)
{
  jjs_value_t logging_values[] = {
    ctx_cstr ("test"),
    ctx_number (1),
    ctx_array (1),
  };

  jjs_log_fmt_v (ctx (), JJS_LOG_LEVEL_TRACE, "{}{}\n", logging_values, JJS_ARRAY_SIZE (logging_values));
  jjs_log_fmt_v (ctx (), JJS_LOG_LEVEL_TRACE, "{}{}{}\n", logging_values, JJS_ARRAY_SIZE (logging_values));
  jjs_log_fmt_v (ctx (), JJS_LOG_LEVEL_TRACE, "{}{}{}{}\n", logging_values, JJS_ARRAY_SIZE (logging_values));

  jjs_log_fmt (ctx (), JJS_LOG_LEVEL_TRACE, "{}{}\n", logging_values[0], logging_values[1]);
  jjs_log_fmt (ctx (), JJS_LOG_LEVEL_TRACE, "{}{}{}\n", logging_values[0], logging_values[1], logging_values[2]);
  jjs_log_fmt (ctx (), JJS_LOG_LEVEL_TRACE, "{}{}{}{}\n", logging_values[0], logging_values[1], logging_values[2]);
}

static void
test_fmt_throw (void)
{
  jjs_value_t expected_message = ctx_cstr ("test");

  /* format and move values */
  {
    jjs_value_t hot_values[] = {
      jjs_string_utf8_sz (ctx (), "t"),
      jjs_string_utf8_sz (ctx (), "e"),
      jjs_string_utf8_sz (ctx (), "s"),
      jjs_string_utf8_sz (ctx (), "t"),
    };

    jjs_value_t ex =
      jjs_fmt_throw (ctx (), JJS_ERROR_RANGE, "{}{}{}{}", hot_values, JJS_ARRAY_SIZE (hot_values), JJS_MOVE);
    TEST_ASSERT (jjs_value_is_exception (ctx (), ex));
    jjs_value_t ex_value = ctx_defer_free (jjs_exception_value (ctx (), ex, JJS_MOVE));
    TEST_ASSERT (jjs_error_type (ctx (), ex_value) == JJS_ERROR_RANGE);
    jjs_value_t message = ctx_defer_free (jjs_object_get_sz (ctx (), ex_value, "message"));
    ctx_assert_strict_equals (message, expected_message);
  }

  /* format and retain values */
  {
    jjs_value_t values[] = {
      ctx_cstr ("t"),
      ctx_cstr ("e"),
      ctx_cstr ("s"),
      ctx_cstr ("t"),
    };

    jjs_value_t ex =
      jjs_fmt_throw (ctx (), JJS_ERROR_RANGE, "{}{}{}{}", values, JJS_ARRAY_SIZE (values), JJS_KEEP);
    TEST_ASSERT (jjs_value_is_exception (ctx (), ex));
    jjs_value_t ex_value = ctx_defer_free (jjs_exception_value (ctx (), ex, JJS_MOVE));
    TEST_ASSERT (jjs_error_type (ctx (), ex_value) == JJS_ERROR_RANGE);
    jjs_value_t message = ctx_defer_free (jjs_object_get_sz (ctx (), ex_value, "message"));
    ctx_assert_strict_equals (message, expected_message);
  }
}

TEST_MAIN ({
  test_fmt_to_string ();
  test_fmt_to_buffer ();
  test_fmt_join ();
  test_fmt_throw ();

  test_fmt_logging ();
})
