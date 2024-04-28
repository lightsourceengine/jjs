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

static void
check_fmt_to_string (const char* format_p, jjs_value_t value, const char* expected)
{
  jjs_value_t formatted = push (jjs_fmt_to_string_v (format_p, &value, 1));

  TEST_ASSERT (strict_equals_cstr (formatted, expected));
  jjs_value_free (value);
}

static void
check_fmt_to_buffer (const char* format_p, jjs_value_t value, const char* expected)
{
  char buffer[256];
  jjs_size_t written =
    jjs_fmt_to_buffer_v ((jjs_char_t*) buffer, JJS_ARRAY_SIZE (buffer), JJS_ENCODING_UTF8, format_p, &value, 1);

  buffer[written] = '\0';

  TEST_ASSERT (strlen (buffer) == strlen (expected));
  TEST_ASSERT (strcmp (buffer, expected) == 0);
  jjs_value_free (value);
}

static void
test_fmt_to_function (void (*check) (const char*, jjs_value_t, const char*))
{
  jjs_value_t array = jjs_array (2);

  push (jjs_object_set_index (array, 0, push (jjs_number (1))));
  push (jjs_object_set_index (array, 1, push (jjs_number (2))));

  check ("{}", jjs_null (), "null");
  check ("{}", jjs_undefined (), "undefined");
  check ("{}", jjs_object (), "[object Object]");
  check ("{}", jjs_array (0), "[]");
  check ("{}", array, "[1,2]");
  check ("{}", jjs_symbol_with_description (push_sz ("desc")), "Symbol(desc)");
  check ("{}", jjs_string_utf8_sz ("hello"), "hello");
  check ("{}", jjs_number (100), "100");

  check ("", push (jjs_number (100)), "");
  check ("no format", push (jjs_number (100)), "no format");
  check ("{}:{}", push (jjs_number (100)), "100:undefined");
}

static void
test_fmt_to_string (void)
{
  test_fmt_to_function (&check_fmt_to_string);

  jjs_value_t values[] = {
    push (jjs_number (1)),
    push (jjs_number (2)),
    push (jjs_number (3)),
  };

  jjs_value_t formatted = push (jjs_fmt_to_string_v ("{}{}{}", values, JJS_ARRAY_SIZE (values)));

  TEST_ASSERT (strict_equals_cstr (formatted, "123"));
}

static void
test_fmt_to_buffer (void)
{
  test_fmt_to_function (&check_fmt_to_buffer);

  jjs_value_t values[] = {
    push (jjs_number (1)),
    push (jjs_number (2)),
    push (jjs_number (3)),
  };

  char buffer[256];
  jjs_size_t written = jjs_fmt_to_buffer_v ((jjs_char_t*) buffer,
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
    push (jjs_number (1)),
    push (jjs_number (2)),
    push (jjs_number (3)),
  };

  jjs_value_t joined = push (jjs_fmt_join_v (jjs_string_sz (", "), JJS_MOVE, values, JJS_ARRAY_SIZE (values)));

  TEST_ASSERT (strict_equals_cstr (joined, "1, 2, 3"));
}

static void
test_fmt_logging (void)
{
  jjs_value_t logging_values[] = {
    push_sz ("test"),
    push (jjs_number (1)),
    push (jjs_array (1)),
  };

  jjs_log_fmt_v (JJS_LOG_LEVEL_TRACE, "{}{}\n", logging_values, JJS_ARRAY_SIZE (logging_values));
  jjs_log_fmt_v (JJS_LOG_LEVEL_TRACE, "{}{}{}\n", logging_values, JJS_ARRAY_SIZE (logging_values));
  jjs_log_fmt_v (JJS_LOG_LEVEL_TRACE, "{}{}{}{}\n", logging_values, JJS_ARRAY_SIZE (logging_values));

  jjs_log_fmt (JJS_LOG_LEVEL_TRACE, "{}{}\n", logging_values[0], logging_values[1]);
  jjs_log_fmt (JJS_LOG_LEVEL_TRACE, "{}{}{}\n", logging_values[0], logging_values[1], logging_values[2]);
  jjs_log_fmt (JJS_LOG_LEVEL_TRACE, "{}{}{}{}\n", logging_values[0], logging_values[1], logging_values[2]);
}

int
main (void)
{
  TEST_CONTEXT_NEW (context_p);

  test_fmt_to_string ();
  test_fmt_to_buffer ();
  test_fmt_join ();

  test_fmt_logging ();

  free_values ();
  TEST_CONTEXT_FREE (context_p);

  return 0;
} /* main */
