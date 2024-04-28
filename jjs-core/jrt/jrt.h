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

#ifndef JRT_H
#define JRT_H

#include <stdio.h>
#include <string.h>

#include "jjs-core.h"

#include "config.h"
#include "jrt-types.h"

/*
 * Constants
 */
#define JJS_BITSINBYTE 8

/*
 * Make sure unused parameters, variables, or expressions trigger no compiler warning.
 */
#define JJS_UNUSED(x) ((void) (x))

#define JJS_UNUSED_1(_1)                             JJS_UNUSED (_1)
#define JJS_UNUSED_2(_1, _2)                         JJS_UNUSED (_1), JJS_UNUSED_1 (_2)
#define JJS_UNUSED_3(_1, _2, _3)                     JJS_UNUSED (_1), JJS_UNUSED_2 (_2, _3)
#define JJS_UNUSED_4(_1, _2, _3, _4)                 JJS_UNUSED (_1), JJS_UNUSED_3 (_2, _3, _4)
#define JJS_UNUSED_5(_1, _2, _3, _4, _5)             JJS_UNUSED (_1), JJS_UNUSED_4 (_2, _3, _4, _5)
#define JJS_UNUSED_6(_1, _2, _3, _4, _5, _6)         JJS_UNUSED (_1), JJS_UNUSED_5 (_2, _3, _4, _5, _6)
#define JJS_UNUSED_7(_1, _2, _3, _4, _5, _6, _7)     JJS_UNUSED (_1), JJS_UNUSED_6 (_2, _3, _4, _5, _6, _7)
#define JJS_UNUSED_8(_1, _2, _3, _4, _5, _6, _7, _8) JJS_UNUSED (_1), JJS_UNUSED_7 (_2, _3, _4, _5, _6, _7, _8)

#define JJS_VA_ARGS_NUM_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define JJS_VA_ARGS_NUM(...)                                         JJS_VA_ARGS_NUM_IMPL (__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define JJS_UNUSED_ALL_IMPL_(nargs) JJS_UNUSED_##nargs
#define JJS_UNUSED_ALL_IMPL(nargs)  JJS_UNUSED_ALL_IMPL_ (nargs)
#define JJS_UNUSED_ALL(...)         JJS_UNUSED_ALL_IMPL (JJS_VA_ARGS_NUM (__VA_ARGS__)) (__VA_ARGS__)

/*
 * Asserts
 *
 * Warning:
 *         Don't use JJS_STATIC_ASSERT in headers, because
 *         __LINE__ may be the same for asserts in a header
 *         and in an implementation file.
 */
#define JJS_STATIC_ASSERT_GLUE_(a, b, c) a##b##_##c
#define JJS_STATIC_ASSERT_GLUE(a, b, c)  JJS_STATIC_ASSERT_GLUE_ (a, b, c)
#define JJS_STATIC_ASSERT(x, msg)                                                  \
  enum                                                                               \
  {                                                                                  \
    JJS_STATIC_ASSERT_GLUE (static_assertion_failed_, __LINE__, msg) = 1 / (!!(x)) \
  }

#ifndef JJS_NDEBUG
void JJS_ATTR_NORETURN jjs_assert_fail (const char *assertion,
                                            const char *file,
                                            const char *function,
                                            const uint32_t line);
void JJS_ATTR_NORETURN jjs_unreachable (const char *file, const char *function, const uint32_t line);

#define JJS_ASSERT(x)                                     \
  do                                                        \
  {                                                         \
    if (JJS_UNLIKELY (!(x)))                              \
    {                                                       \
      jjs_assert_fail (#x, __FILE__, __func__, __LINE__); \
    }                                                       \
  } while (0)

#define JJS_UNREACHABLE()                           \
  do                                                  \
  {                                                   \
    jjs_unreachable (__FILE__, __func__, __LINE__); \
  } while (0)
#else /* JJS_NDEBUG */
#define JJS_ASSERT(x) \
  do                    \
  {                     \
    if (false)          \
    {                   \
      JJS_UNUSED (x); \
    }                   \
  } while (0)

#ifdef __GNUC__
#define JJS_UNREACHABLE() __builtin_unreachable ()
#endif /* __GNUC__ */

#ifdef _MSC_VER
#define JJS_UNREACHABLE() _assume (0)
#endif /* _MSC_VER */

#ifndef JJS_UNREACHABLE
#define JJS_UNREACHABLE()
#endif /* !JJS_UNREACHABLE */

#endif /* !JJS_NDEBUG */

/**
 * Exit on fatal error
 */
void JJS_ATTR_NORETURN jjs_fatal (jjs_fatal_code_t code);

jjs_log_level_t jjs_jrt_get_log_level (void);
void jjs_jrt_set_log_level (jjs_log_level_t level);

/*
 * Logging
 */
#if JJS_LOGGING
#define JJS_ERROR_MSG(...)   jjs_log (&JJS_CONTEXT_STRUCT, JJS_LOG_LEVEL_ERROR, __VA_ARGS__)
#define JJS_WARNING_MSG(...) jjs_log (&JJS_CONTEXT_STRUCT, JJS_LOG_LEVEL_WARNING, __VA_ARGS__)
#define JJS_DEBUG_MSG(...)   jjs_log (&JJS_CONTEXT_STRUCT, JJS_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define JJS_TRACE_MSG(...)   jjs_log (&JJS_CONTEXT_STRUCT, JJS_LOG_LEVEL_TRACE, __VA_ARGS__)
#else /* !JJS_LOGGING */
#define JJS_ERROR_MSG(...)          \
  do                                  \
  {                                   \
    if (false)                        \
    {                                 \
      JJS_UNUSED_ALL (__VA_ARGS__); \
    }                                 \
  } while (0)
#define JJS_WARNING_MSG(...)        \
  do                                  \
  {                                   \
    if (false)                        \
    {                                 \
      JJS_UNUSED_ALL (__VA_ARGS__); \
    }                                 \
  } while (0)
#define JJS_DEBUG_MSG(...)          \
  do                                  \
  {                                   \
    if (false)                        \
    {                                 \
      JJS_UNUSED_ALL (__VA_ARGS__); \
    }                                 \
  } while (0)
#define JJS_TRACE_MSG(...)          \
  do                                  \
  {                                   \
    if (false)                        \
    {                                 \
      JJS_UNUSED_ALL (__VA_ARGS__); \
    }                                 \
  } while (0)
#endif /* JJS_LOGGING */

/**
 * Size of struct member
 */
#define JJS_SIZE_OF_STRUCT_MEMBER(struct_name, member_name) sizeof (((struct_name *) NULL)->member_name)

/**
 * Aligns @a value to @a alignment. @a must be the power of 2.
 *
 * Returns minimum positive value, that divides @a alignment and is more than or equal to @a value
 */
#define JJS_ALIGNUP(value, alignment) (((value) + ((alignment) -1)) & ~((alignment) -1))

/**
 * Align value down
 */
#define JJS_ALIGNDOWN(value, alignment) ((value) & ~((alignment) -1))

/*
 * min, max
 */
#define JJS_MIN(v1, v2) (((v1) < (v2)) ? (v1) : (v2))
#define JJS_MAX(v1, v2) (((v1) < (v2)) ? (v2) : (v1))

/**
 * Calculate the index of the first non-zero bit of a 32 bit integer value
 */
#define JJS__LOG2_1(n) (((n) >= 2) ? 1 : 0)
#define JJS__LOG2_2(n) (((n) >= 1 << 2) ? (2 + JJS__LOG2_1 ((n) >> 2)) : JJS__LOG2_1 (n))
#define JJS__LOG2_4(n) (((n) >= 1 << 4) ? (4 + JJS__LOG2_2 ((n) >> 4)) : JJS__LOG2_2 (n))
#define JJS__LOG2_8(n) (((n) >= 1 << 8) ? (8 + JJS__LOG2_4 ((n) >> 8)) : JJS__LOG2_4 (n))
#define JJS_LOG2(n)    (((n) >= 1 << 16) ? (16 + JJS__LOG2_8 ((n) >> 16)) : JJS__LOG2_8 (n))

/**
 * JJS_BLOCK_TAIL_CALL_OPTIMIZATION
 *
 * Adapted from abseil ( https://github.com/abseil/ )
 *
 * Instructs the compiler to avoid optimizing tail-call recursion. This macro is
 * useful when you wish to preserve the existing function order within a stack
 * trace for logging, debugging, or profiling purposes.
 *
 * Example:
 *
 *   int f() {
 *     int result = g();
 *     JJS_BLOCK_TAIL_CALL_OPTIMIZATION();
 *     return result;
 *   }
 *
 * This macro is intentionally here as jjs-compiler.h is a public header and
 * it does not make sense to expose this macro to the public.
 */
#if defined(__clang__) || defined(__GNUC__)
/* Clang/GCC will not tail call given inline volatile assembly. */
#define JJS_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#else /* !defined(__clang__) && !defined(__GNUC__) */
/* On GCC 10.x this version also works. */
#define JJS_BLOCK_TAIL_CALL_OPTIMIZATION()                 \
  do                                                         \
  {                                                          \
    JJS_CONTEXT (status_flags) |= ECMA_STATUS_API_ENABLED; \
  } while (0)
#endif /* defined(__clang__) || defined (__GNUC__) */

#endif /* !JRT_H */
