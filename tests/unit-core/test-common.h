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

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jjs-port.h"

#define JJS_UNUSED(x) ((void) (x))

#define TEST_ASSERT(x)                                           \
  do                                                             \
  {                                                              \
    if (JJS_UNLIKELY (!(x)))                                     \
    {                                                            \
      jjs_log (JJS_LOG_LEVEL_ERROR,                              \
                 "TEST: Assertion '%s' failed at %s(%s):%u.\n",  \
                 #x,                                             \
                 __FILE__,                                       \
                 __func__,                                       \
                 (uint32_t) __LINE__);                           \
      jjs_port_fatal (JJS_FATAL_FAILED_ASSERTION);               \
    }                                                            \
  } while (0)

#define TEST_ASSERT_STR(EXPECTED, RESULT)                          \
  do                                                               \
  {                                                                \
    const char* __expected = (const char*) (EXPECTED);             \
    const char* __result = (const char*) (RESULT);                 \
    if (strcmp (__expected, __result) != 0)                        \
    {                                                              \
      jjs_log (JJS_LOG_LEVEL_ERROR,                                \
                 "TEST: String comparison failed at %s(%s):%u.\n"  \
                 " Expected: '%s'\n Got: '%s'\n",                  \
                 __FILE__,                                         \
                 __func__,                                         \
                 (uint32_t) __LINE__,                              \
                 __expected,                                       \
                 __result);                                        \
      jjs_port_fatal (JJS_FATAL_FAILED_ASSERTION);                 \
    }                                                              \
  } while (0)

/**
 * Test initialization statement that should be included
 * at the beginning of main function in every unit test.
 */
#define TEST_INIT()                              \
  do                                             \
  {                                              \
    union                                        \
    {                                            \
      double d;                                  \
      unsigned u;                                \
    } now = { .d = jjs_port_current_time () };   \
    srand (now.u);                               \
  } while (0)

/**
 * Dummy macro to enable the breaking of long string literals into multiple
 * substrings on separate lines. (Style checker doesn't allow it without putting
 * the whole literal into parentheses but the compiler warns about parenthesized
 * string constants.)
 */
#define TEST_STRING_LITERAL(x) x

/** Cast null terminated C string (char) to JJS string (uint8_t). */
#define JJS_STR(CSTRING) (const jjs_char_t*)(CSTRING)
/** Get a null terminated C string's length and cast to jjs_size_t. */
#define JJS_STRLEN(CSTRING) (const jjs_size_t)(strlen(CSTRING))

/**
 * If value is an exception, prints a toString() of the exception's value (usually an Error object).
 */
void print_if_exception (jjs_value_t value);

/**
 * Compare a JJS string value to a c string.
 */
bool strict_equals_cstr (jjs_value_t a, const char* b);

/**
 * Compare a JJS value to a c integer.
 */
bool strict_equals_int32 (jjs_value_t a, int32_t b);

/**
 * Compare two JJS values with ===.
 */
bool strict_equals (jjs_value_t a, jjs_value_t b);

#endif /* !TEST_COMMON_H */

#ifdef TEST_COMMON_IMPLEMENTATION

void print_if_exception (jjs_value_t value)
{
  if (!jjs_value_is_exception (value))
  {
    return;
  }

  jjs_value_t err = jjs_exception_value (value, false);
  jjs_value_t message = jjs_value_to_string (err);
  char message_p[512];
  jjs_size_t written = jjs_string_to_buffer (message,
                                             JJS_ENCODING_UTF8,
                                             (jjs_char_t*) message_p,
                                             sizeof (message_p));

  message_p[written] = '\0';
  printf ("%s\n", message_p);

  jjs_value_free (message);
  jjs_value_free (err);
}

bool
strict_equals (jjs_value_t a, jjs_value_t b)
{
  jjs_value_t op_result = jjs_binary_op (JJS_BIN_OP_STRICT_EQUAL, a, b);
  bool result = jjs_value_is_true (op_result);

  jjs_value_free (op_result);

  return result;
}

bool
strict_equals_cstr (jjs_value_t a, const char* b)
{
  jjs_value_t b_value = jjs_string_sz (b);
  bool result = strict_equals (a, b_value);

  jjs_value_free (b_value);

  return result;
}

bool
strict_equals_int32 (jjs_value_t a, int32_t b)
{
  jjs_value_t b_value = jjs_number_from_int32 (b);
  bool result = strict_equals (a, b_value);

  jjs_value_free (b_value);

  return result;
}

#undef TEST_COMMON_IMPLEMENTATION

#endif /* TEST_COMMON_IMPLEMENTATION */
