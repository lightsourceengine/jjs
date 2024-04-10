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

#include "jjs-util.h"

#include "ecma-helpers.h"
#include "lit-char-helpers.h"

/**
 * Create an API compatible return value.
 *
 * @return return value for JJS API functions
 */
jjs_value_t
jjs_return (const jjs_value_t value) /**< return value */
{
  if (ECMA_IS_VALUE_ERROR (value))
  {
    return ecma_create_exception_from_context ();
  }

  return value;
} /* jjs_return */

bool jjs_util_parse_encoding (jjs_value_t value, jjs_encoding_t* out_p)
{
  if (jjs_value_is_string (value))
  {
    char buffer[8];

    static const jjs_size_t size = (jjs_size_t) (sizeof (buffer) / sizeof (buffer[0]));
    jjs_value_t w = jjs_string_to_buffer (value, JJS_ENCODING_CESU8, (lit_utf8_byte_t*) &buffer[0], size - 1);

    JJS_ASSERT(w < size);
    buffer[w] = '\0';

    lit_utf8_byte_t* cursor_p = (lit_utf8_byte_t*) buffer;
    lit_utf8_byte_t c;

    while (*cursor_p != '\0')
    {
      c = *cursor_p;

      if (c <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
      {
        *cursor_p += (lit_utf8_byte_t) lit_char_to_lower_case (c, NULL);
      }

      cursor_p++;
    }

    if (strcmp (buffer, "utf8") == 0 || strcmp (buffer, "utf-8") == 0)
    {
      *out_p = JJS_ENCODING_UTF8;
    }
    else if (strcmp (buffer, "cesu8") == 0)
    {
      *out_p = JJS_ENCODING_CESU8;
    }
    else if (strcmp (buffer, "none") == 0)
    {
      *out_p = JJS_ENCODING_NONE;
    }
    else
    {
      return false;
    }

    return true;
  }
  else if (jjs_value_is_undefined (value))
  {
    *out_p = JJS_ENCODING_NONE;
    return true;
  }

  return false;
} /* jjs_util_parse_encoding */
