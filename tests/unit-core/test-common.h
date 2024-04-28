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
#include <time.h>

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
      jjs_platform_fatal (JJS_FATAL_FAILED_ASSERTION);           \
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
      jjs_platform_fatal (JJS_FATAL_FAILED_ASSERTION);             \
    }                                                              \
  } while (0)

#define JJS_EXPECT_EXCEPTION(EXPR)         \
  TEST_ASSERT (jjs_value_is_exception(EXPR))
#define JJS_EXPECT_NOT_EXCEPTION(EXPR)         \
  TEST_ASSERT (!jjs_value_is_exception(EXPR))
#define JJS_EXPECT_TRUE(EXPR)         \
  TEST_ASSERT (jjs_value_is_true(EXPR))
#define JJS_EXPECT_UNDEFINED(EXPR)         \
  TEST_ASSERT (jjs_value_is_undefined(EXPR))
#define JJS_EXPECT_PROMISE(EXPR)         \
  TEST_ASSERT (jjs_value_is_promise(EXPR))

#define JJS_EXPECT_EXCEPTION_MOVE(EXPR)         \
  JJS_EXPECT_EXCEPTION (push(EXPR))
#define JJS_EXPECT_TRUE_MOVE(EXPR)         \
  JJS_EXPECT_TRUE (push(EXPR))
#define JJS_EXPECT_UNDEFINED_MOVE(EXPR)         \
  JJS_EXPECT_UNDEFINED (push(EXPR))
#define JJS_EXPECT_PROMISE_MOVE(EXPR)         \
  JJS_EXPECT_PROMISE (push(EXPR))

#define JJS_ARRAY_SIZE(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

/**
 * Assert that a double is approximately equal (within .001) to an expected double.
 */
#define TEST_ASSERT_DOUBLE_EQUALS(ACTUAL, EXPECTED) TEST_ASSERT (fabs ((ACTUAL) - (EXPECTED)) < .001)

/**
 * Test initialization statement that should be included
 * at the beginning of main function in every unit test.
 */
#define TEST_INIT()                 \
  do                                \
  {                                 \
    srand ((uint32_t) time (NULL)); \
  } while (0)

#define TEST_CONTEXT_NEW(CONTEXT_IDENT) \
  jjs_context_t* CONTEXT_IDENT;         \
  TEST_ASSERT (jjs_context_new (NULL, &(CONTEXT_IDENT)) == JJS_STATUS_OK)

#define TEST_CONTEXT_NEW_OPTS(CONTEXT_IDENT, OPTS_P) \
  jjs_context_t* CONTEXT_IDENT;                      \
  TEST_ASSERT (jjs_context_new ((OPTS_P), &(CONTEXT_IDENT)) == JJS_STATUS_OK)

#define TEST_CONTEXT_NEW_FLAGS(CONTEXT_IDENT, FLAGS)                             \
  jjs_context_t* CONTEXT_IDENT;                                                  \
  do                                                                             \
  {                                                                              \
    jjs_context_options_t options = {                                            \
      .context_flags = (FLAGS),                                                  \
    };                                                                           \
    TEST_ASSERT (jjs_context_new (&options, &(CONTEXT_IDENT)) == JJS_STATUS_OK); \
  } while (false)

#define TEST_CONTEXT_FREE(CONTEXT_IDENT) jjs_context_free (CONTEXT_IDENT);

/**
 * Initialize just the global context, but not the ecma, annex and pack layers.
 *
 * Most tests should use jjs_init. This helper is for tests that target lower
 * layers that need the context to be setup.
 */
#define TEST_CONTEXT_INIT() TEST_ASSERT (jjs_context_init (NULL) == JJS_STATUS_OK)

/**
 * Cleanup a test environment that called TEST_CONTEXT_INIT() to start.
 *
 * This is for specialized tests. Most test should use jjs_init/jjs_cleanup to
 * manage the environment.
 */
#define TEST_CONTEXT_CLEANUP() jjs_context_cleanup()

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

/**
 * Queue a value for deletion when free_values() is called.
 */
jjs_value_t push (jjs_value_t value);

/**
 * Creates a JS value for the string and queue the JS value for deletion when free_values() is called.
 */
jjs_value_t push_sz (const char* value);

/**
 * Free all values that have been pushed.
 */
void free_values (void);

#endif /* !TEST_COMMON_H */

#ifdef TEST_COMMON_IMPLEMENTATION

// 256 is arbitrary. if push assertion is hit, just makes this number larger.
static jjs_value_t s_values [1024] = { 0 };
static size_t s_values_index = 0;

jjs_value_t
push (jjs_value_t value)
{
  TEST_ASSERT(s_values_index < (sizeof (s_values) / sizeof (s_values[0])));
  s_values[s_values_index++] = value;

  return value;
}

jjs_value_t
push_sz (const char* value)
{
  return push (jjs_string_sz (value));
}

void
free_values (void)
{
  for (size_t i = 0; i < s_values_index; i++)
  {
    jjs_value_free (s_values[i]);
  }

  s_values_index = 0;
}

void
print_if_exception (jjs_value_t value)
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
