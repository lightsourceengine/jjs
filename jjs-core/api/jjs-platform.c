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

#include "jjs-annex.h"
#include "jjs-core.h"

#include "jcontext.h"
#include "jjs-platform.h"

// Allow user to specify JJS_PLATFORM_OS at compile time. Otherwise, pick a supported one based on compiler flags.
#ifndef JJS_PLATFORM_OS
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#define JJS_PLATFORM_OS "win32"
#elif defined(_AIX) || defined(__TOS_AIX__)
#define JJS_PLATFORM_OS "aix"
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#define JJS_PLATFORM_OS "linux"
#elif defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#define JJS_PLATFORM_OS "darwin"
#else
#define JJS_PLATFORM_OS "unknown"
#endif
#endif /* JJS_PLATFORM_OS */

// Allow user to specify JJS_PLATFORM_ARCH at compile time. Otherwise, pick a supported one based on compiler flags.
#ifndef JJS_PLATFORM_ARCH
#if defined(i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)    \
  || defined(__i386) || defined(_M_IX86) || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) \
  || defined(__INTEL__)
#define JJS_PLATFORM_ARCH "ia32"
#elif defined(__ARM_ARCH) || defined(__TARGET_ARCH_ARM) || defined(__TARGET_ARCH_THUMB) || defined(_M_ARM) \
  || defined(__arm__) || defined(__thumb__)
#define JJS_PLATFORM_ARCH "arm"
#elif defined(__arm64) || defined(_M_ARM64) || defined(__aarch64__) || defined(__AARCH64EL__)
#define JJS_PLATFORM_ARCH "arm64"
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
#define JJS_PLATFORM_ARCH "x64"
#else
#define JJS_PLATFORM_ARCH "unknown"
#endif
#endif /* JJS_PLATFORM_ARCH */

static bool platform_set_string (const char* value_p, char* dest_p, uint32_t dest_len);

// TODO: should this be exposed publicly?
const jjs_platform_t*
jjs_platform (void)
{
  jjs_assert_api_enabled ();
  return &JJS_CONTEXT (platform_api);
} /* jjs_platform */

/**
 * Helper function to set platform arch string. Should only be used at context initialization time.
 *
 * @param platform_p platform object
 * @param value_p string value. strlen(value_p) should be > 0 && < 16
 * @return true if string successfully set, false on error
 */
bool
jjs_platform_set_arch_sz (jjs_platform_t* platform_p, const char* value_p)
{
  return platform_set_string (value_p, &platform_p->arch_sz[0], (uint32_t) sizeof (platform_p->arch_sz));
} /* jjs_platform_set_arch_sz */

/**
 * Helper function to set platform os string. Should only be used at context initialization time.
 *
 * @param platform_p platform object
 * @param value_p string value. strlen(value_p) should be > 0 && < 16
 * @return true if string successfully set, false on error
 */
bool
jjs_platform_set_os_sz (jjs_platform_t* platform_p, const char* value_p)
{
  return platform_set_string (value_p, &platform_p->os_sz[0], (uint32_t) sizeof (platform_p->os_sz));
} /* jjs_platform_set_os_sz */

jjs_platform_t
jjs_platform_defaults (void)
{
  jjs_platform_t platform;

  jjs_platform_set_os_sz (&platform, JJS_PLATFORM_OS);
  jjs_platform_set_arch_sz (&platform, JJS_PLATFORM_ARCH);

  return platform;
} /* jjs_platform_defaults */

static bool
platform_set_string (const char* value_p, char* dest_p, uint32_t dest_len)
{
  // TODO: context check
  size_t len = (uint32_t) (value_p ? strlen (value_p) : 0);

  if (len > 0 && len < dest_len - 1)
  {
    memcpy (dest_p, value_p, len + 1);
    return true;
  }

  return false;
} /* platform_set_string */
