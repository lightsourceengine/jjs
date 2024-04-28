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

/**
 * Implementation of exit with specified status code.
 */

#include "jrt-libc-includes.h"
#include "jrt.h"
#include "jcontext.h"

/*
 * Exit with specified status code.
 *
 * If !JJS_NDEBUG and code != 0, print status code with description
 * and call assertion fail handler.
 */
void JJS_ATTR_NORETURN
jjs_fatal (jjs_fatal_code_t code) /**< status code */
{
#ifndef JJS_NDEBUG
  switch (code)
  {
    case JJS_FATAL_OUT_OF_MEMORY:
    {
      JJS_ERROR_MSG ("Error: JJS_FATAL_OUT_OF_MEMORY\n");
      break;
    }
    case JJS_FATAL_REF_COUNT_LIMIT:
    {
      JJS_ERROR_MSG ("Error: JJS_FATAL_REF_COUNT_LIMIT\n");
      break;
    }
    case JJS_FATAL_UNTERMINATED_GC_LOOPS:
    {
      JJS_ERROR_MSG ("Error: JJS_FATAL_UNTERMINATED_GC_LOOPS\n");
      break;
    }
    case JJS_FATAL_DISABLED_BYTE_CODE:
    {
      JJS_ERROR_MSG ("Error: JJS_FATAL_DISABLED_BYTE_CODE\n");
      break;
    }
    case JJS_FATAL_FAILED_ASSERTION:
    {
      JJS_ERROR_MSG ("Error: JJS_FATAL_FAILED_ASSERTION\n");
      break;
    }
  }
#endif /* !JJS_NDEBUG */

  jjs_platform_fatal (&JJS_CONTEXT_STRUCT, code);

  /* to make compiler happy for some RTOS: 'control reaches end of non-void function' */
  while (true)
  {
  }
} /* jjs_fatal */

#ifndef JJS_NDEBUG
/**
 * Handle failed assertion
 */
void JJS_ATTR_NORETURN
jjs_assert_fail (const char *assertion, /**< assertion condition string */
                   const char *file, /**< file name */
                   const char *function, /**< function name */
                   const uint32_t line) /**< line */
{
  JJS_ERROR_MSG ("ICE: Assertion '%s' failed at %s(%s):%u.\n", assertion, file, function, line);

  jjs_fatal (JJS_FATAL_FAILED_ASSERTION);
} /* jjs_assert_fail */

/**
 * Handle execution of control path that should be unreachable
 */
void JJS_ATTR_NORETURN
jjs_unreachable (const char *file, /**< file name */
                   const char *function, /**< function name */
                   const uint32_t line) /**< line */
{
  JJS_ERROR_MSG ("ICE: Unreachable control path at %s(%s):%u was executed.\n", file, function, line);

  jjs_fatal (JJS_FATAL_FAILED_ASSERTION);
} /* jjs_unreachable */
#endif /* !JJS_NDEBUG */
