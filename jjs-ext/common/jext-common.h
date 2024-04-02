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

#ifndef JEXT_COMMON_H
#define JEXT_COMMON_H

#include <stdio.h>
#include <string.h>

#include "jjs.h"

/*
 * Make sure unused parameters, variables, or expressions trigger no compiler warning.
 */
#define JJSX_UNUSED(x) ((void) (x))

/*
 * Asserts
 *
 * Warning:
 *         Don't use JJS_STATIC_ASSERT in headers, because
 *         __LINE__ may be the same for asserts in a header
 *         and in an implementation file.
 */
#define JJSX_STATIC_ASSERT_GLUE_(a, b, c) a##b##_##c
#define JJSX_STATIC_ASSERT_GLUE(a, b, c)  JJSX_STATIC_ASSERT_GLUE_ (a, b, c)
#define JJSX_STATIC_ASSERT(x, msg)                                                  \
  enum                                                                                \
  {                                                                                   \
    JJSX_STATIC_ASSERT_GLUE (static_assertion_failed_, __LINE__, msg) = 1 / (!!(x)) \
  }

#ifndef JJS_NDEBUG
void JJS_ATTR_NORETURN jjs_assert_fail (const char *assertion,
                                            const char *file,
                                            const char *function,
                                            const uint32_t line);
void JJS_ATTR_NORETURN jjs_unreachable (const char *file, const char *function, const uint32_t line);

#define JJSX_ASSERT(x)                                    \
  do                                                        \
  {                                                         \
    if (JJS_UNLIKELY (!(x)))                              \
    {                                                       \
      jjs_assert_fail (#x, __FILE__, __func__, __LINE__); \
    }                                                       \
  } while (0)

#define JJSX_UNREACHABLE()                          \
  do                                                  \
  {                                                   \
    jjs_unreachable (__FILE__, __func__, __LINE__); \
  } while (0)
#else /* JJS_NDEBUG */
#define JJSX_ASSERT(x) \
  do                     \
  {                      \
    if (false)           \
    {                    \
      JJSX_UNUSED (x); \
    }                    \
  } while (0)

#ifdef __GNUC__
#define JJSX_UNREACHABLE() __builtin_unreachable ()
#endif /* __GNUC__ */

#ifdef _MSC_VER
#define JJSX_UNREACHABLE() _assume (0)
#endif /* _MSC_VER */

#ifndef JJSX_UNREACHABLE
#define JJSX_UNREACHABLE()
#endif /* !JJSX_UNREACHABLE */

#endif /* !JJS_NDEBUG */

/*
 * Logging
 */
#define JJSX_ERROR_MSG(...)   jjs_log (JJS_LOG_LEVEL_ERROR, __VA_ARGS__)
#define JJSX_WARNING_MSG(...) jjs_log (JJS_LOG_LEVEL_WARNING, __VA_ARGS__)
#define JJSX_DEBUG_MSG(...)   jjs_log (JJS_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define JJSX_TRACE_MSG(...)   jjs_log (JJS_LOG_LEVEL_TRACE, __VA_ARGS__)

#endif /* !JEXT_COMMON_H */
