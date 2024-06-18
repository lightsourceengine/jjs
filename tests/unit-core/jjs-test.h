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

#ifndef JJS_TEST_H
#define JJS_TEST_H

#include <jjs.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define JJS_UNUSED(x) ((void) (x))

#define TEST_MAIN(CODE)             \
  int main (void)                   \
  {                                 \
    /* NOLINTNEXTLINE */            \
    srand ((uint32_t) time (NULL)); \
    ctx_open (NULL);                \
    CODE ctx_close ();              \
  }

#define TEST_ASSERT(x)                                                                                              \
  do                                                                                                                \
  {                                                                                                                 \
    if (JJS_UNLIKELY (!(x)))                                                                                        \
    {                                                                                                               \
      fprintf (stderr, "TEST: Assertion '%s' failed at %s(%s):%u.\n", #x, __FILE__, __func__, (uint32_t) __LINE__); \
      exit (1);                                                                                                     \
    }                                                                                                               \
  } while (0)

#define TEST_ASSERT_STR(EXPECTED, RESULT)                       \
  do                                                            \
  {                                                             \
    const char *__expected = (const char *) (EXPECTED);         \
    const char *__result = (const char *) (RESULT);             \
    if (strcmp (__expected, __result) != 0)                     \
    {                                                           \
      fprintf (stderr,                                          \
               "TEST: String comparison failed at %s(%s):%u.\n" \
               " Expected: '%s'\n Got: '%s'\n",                 \
               __FILE__,                                        \
               __func__,                                        \
               (uint32_t) __LINE__,                             \
               __expected,                                      \
               __result);                                       \
      exit (1);                                                 \
    }                                                           \
  } while (0)

#define JJS_EXPECT_EXCEPTION(EXPR)     TEST_ASSERT (jjs_value_is_exception (ctx (), EXPR))
#define JJS_EXPECT_NOT_EXCEPTION(EXPR) TEST_ASSERT (!jjs_value_is_exception (ctx (), EXPR))
#define JJS_EXPECT_TRUE(EXPR)          TEST_ASSERT (jjs_value_is_true (ctx (), EXPR))
#define JJS_EXPECT_UNDEFINED(EXPR)     TEST_ASSERT (jjs_value_is_undefined (ctx (), EXPR))
#define JJS_EXPECT_PROMISE(EXPR)       TEST_ASSERT (jjs_value_is_promise (ctx (), EXPR))

#define JJS_EXPECT_EXCEPTION_MOVE(EXPR) JJS_EXPECT_EXCEPTION (ctx_defer_free (EXPR))
#define JJS_EXPECT_TRUE_MOVE(EXPR)      JJS_EXPECT_TRUE (ctx_defer_free (EXPR))
#define JJS_EXPECT_UNDEFINED_MOVE(EXPR) JJS_EXPECT_UNDEFINED (ctx_defer_free (EXPR))
#define JJS_EXPECT_PROMISE_MOVE(EXPR)   JJS_EXPECT_PROMISE (ctx_defer_free (EXPR))

#define JJS_ARRAY_SIZE(ARRAY) (sizeof (ARRAY) / sizeof (ARRAY[0]))

/**
 * Assert that a double is approximately equal (within .001) to an expected double.
 */
#define TEST_ASSERT_DOUBLE_EQUALS(ACTUAL, EXPECTED) TEST_ASSERT (fabs ((ACTUAL) - (EXPECTED)) < .001)

/**
 * Dummy macro to enable the breaking of long string literals into multiple
 * substrings on separate lines. (Style checker doesn't allow it without putting
 * the whole literal into parentheses but the compiler warns about parenthesized
 * string constants.)
 */
#define TEST_STRING_LITERAL(x) x

/**
 * Compare a JJS string value to a c string.
 */
bool strict_equals_cstr (jjs_context_t *context_p, jjs_value_t a, const char *b);

/**
 * Compare a JJS value to a c integer.
 */
bool strict_equals_int32 (jjs_context_t *context_p, jjs_value_t a, int32_t b);

/**
 * Compare two JJS values with ===.
 */
bool strict_equals (jjs_context_t *context_p, jjs_value_t a, jjs_value_t b);

/**
 * Create a new context.
 *
 * The test environment has a context stack. This method pushes a new context
 * onto the stack and sets the new context as current. All ctx operation will
 * work on this context.
 */
jjs_context_t *ctx_open (const jjs_context_options_t *options);

/**
 * Destroy the current context.
 *
 * If the stack is not empty after destroy, the previous context will be set
 * as current.
 */
void ctx_close (void);

/**
 * Get the current context.
 */
jjs_context_t *ctx (void);

/**
 * Bootstrap a minimal context that does not enable the public jjs api. This is
 * for low level tests only.
 */
jjs_context_t *ctx_bootstrap (const jjs_context_options_t *options);

/**
 * Cleanup a minimal context. This is for low level tests only.
 */
void ctx_bootstrap_cleanup (jjs_context_t *context_p);

/*
 * Create values and store their reference in the current test context. Values
 * will be cleaned up when the text context is destroyed.
 */

jjs_value_t ctx_defer_free (jjs_value_t value);
jjs_value_t ctx_global (void);
jjs_value_t ctx_cstr (const char *s);
jjs_value_t ctx_number (double n);
jjs_value_t ctx_null (void);
jjs_value_t ctx_undefined (void);
jjs_value_t ctx_object (void);
jjs_value_t ctx_array (jjs_length_t len);
jjs_value_t ctx_boolean (bool value);
jjs_value_t ctx_symbol (const char *description);

/* assert helpers that work on the current context */

void ctx_assert_strict_equals (jjs_value_t expected, jjs_value_t actual);

#endif /* JJS_TEST_H */
