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
#include "jjs-compiler.h"

static const char* const jjs_os_identifier_p =
#ifndef JJS_PLATFORM_OS
#if defined (JJS_OS_IS_WINDOWS)
"win32"
#elif defined (JJS_OS_IS_AIX)
"aix"
#elif defined (JJS_OS_IS_LINUX)
"linux"
#elif defined (JJS_OS_IS_MACOS)
"darwin"
#else
"unknown"
#endif
#endif /* JJS_PLATFORM_OS */
;

static const char* const jjs_arch_identifier_p =
#ifndef JJS_PLATFORM_ARCH
#if defined(JJS_ARCH_IS_X32)
"ia32"
#elif defined(JJS_ARCH_IS_ARM)
"arm"
#elif defined(JJS_ARCH_IS_X64)
"arm64"
#elif defined(JJS_ARCH_IS_X64)
"x64"
#else
"unknown"
#endif
#endif /* JJS_PLATFORM_ARCH */
;

static bool jjsp_set_string (const char* value_p, char* dest_p, uint32_t dest_len);

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
  return jjsp_set_string (value_p, &platform_p->arch_sz[0], (uint32_t) sizeof (platform_p->arch_sz));
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
  return jjsp_set_string (value_p, &platform_p->os_sz[0], (uint32_t) sizeof (platform_p->os_sz));
} /* jjs_platform_set_os_sz */

jjs_platform_t
jjsp_defaults (void)
{
  jjs_platform_t platform;

  jjs_platform_set_os_sz (&platform, jjs_os_identifier_p);
  jjs_platform_set_arch_sz (&platform, jjs_arch_identifier_p);

  platform.cwd = jjsp_cwd;

  return platform;
} /* jjsp_defaults */

static bool
jjsp_set_string (const char* value_p, char* dest_p, uint32_t dest_len)
{
  // TODO: context check
  size_t len = (uint32_t) (value_p ? strlen (value_p) : 0);

  if (len > 0 && len < dest_len - 1)
  {
    memcpy (dest_p, value_p, len + 1);
    return true;
  }

  return false;
} /* jjsp_set_string */

void
jjsp_buffer_free (jjs_platform_buffer_t* buffer_p)
{
  if (buffer_p)
  {
    free (buffer_p->data_p);
    memset (buffer_p, 0, sizeof (jjs_platform_buffer_t));
  }
} /* jjsp_buffer_free */

char*
jjsp_strndup (char* str_p, uint32_t length)
{
  if (str_p == NULL || length == 0)
  {
    return NULL;
  }

  char* result_p = malloc (length);

  if (result_p != NULL)
  {
    memcpy (result_p, str_p, length);
  }

  return result_p;
} /* jjsp_strndup */

/**
 * Create an ecma string value from a platform buffer.
 *
 * @param buffer_p buffer object
 * @param move if true, this function owns buffer_p and will call buffer_p->free
 * @return on success, ecma string value; on error, ECMA_VALUE_EMPTY. return value must be freed with ecma_free_value.
 */
ecma_value_t
jjsp_buffer_to_string_value (jjs_platform_buffer_t* buffer_p, bool move)
{
  ecma_value_t result;

  if (buffer_p->encoding == JJS_PLATFORM_BUFFER_ENCODING_UTF8)
  {
    result =
      ecma_make_string_value (ecma_new_ecma_string_from_utf8 ((const lit_utf8_byte_t*) buffer_p->data_p, buffer_p->length));
  }
  else if (buffer_p->encoding == JJS_PLATFORM_BUFFER_ENCODING_UTF16)
  {
    JJS_ASSERT(buffer_p->length % 2 == 0);
    // buffer.length is in bytes convert to utf16 array size
    result = ecma_make_string_value (ecma_new_ecma_string_from_utf16 ((const uint16_t*) buffer_p->data_p,
                                                                      buffer_p->length / (uint32_t) sizeof (ecma_char_t)));
  }
  else
  {
    result = ECMA_VALUE_EMPTY;
  }

  if (move)
  {
    buffer_p->free (buffer_p);
  }

  return result;
} /* jjsp_buffer_to_string */
