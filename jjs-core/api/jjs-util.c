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

/**
 * Maps a JS string option argument to an enum.
 *
 * The pattern in JS is fn('option') or fn({ key: 'option') or fn(). The string is
 * extracted and looked up in option_mappings_p. If found, return true. If not found,
 * return false. If the option is undefined, true is returned with default_mapped_value.
 */
bool
jjs_util_map_option (jjs_value_t option,
                     jjs_value_ownership_t option_o,
                     jjs_value_t key,
                     jjs_value_t key_o,
                     const jjs_util_option_pair_t* option_mappings_p,
                     jjs_size_t len,
                     uint32_t default_mapped_value,
                     uint32_t* out_p)
{
  if (jjs_value_is_undefined (option))
  {
    JJS_DISOWN (option, option_o);
    JJS_DISOWN (key, key_o);
    *out_p = default_mapped_value;
    return true;
  }

  /* option_value = option.key or option */
  jjs_value_t option_value;

  if (jjs_value_is_string (option))
  {
    option_value = jjs_value_copy (option);
  }
  else if (jjs_value_is_string (key) && jjs_value_is_object (option))
  {
    option_value = jjs_object_get (option, key);

    if (jjs_value_is_undefined (option_value))
    {
      JJS_DISOWN (option, option_o);
      JJS_DISOWN (key, key_o);
      jjs_value_free (option_value);
      *out_p = default_mapped_value;
      return true;
    }
  }
  else
  {
    option_value = ECMA_VALUE_EMPTY;
  }

  JJS_DISOWN (option, option_o);
  JJS_DISOWN (key, key_o);

  if (!jjs_value_is_string (option_value))
  {
    jjs_value_free (option_value);
    return false;
  }

  /* tolower option_value */
  char buffer[32];

  static const jjs_size_t size = (jjs_size_t) (sizeof (buffer) / sizeof (buffer[0]));
  jjs_value_t w = jjs_string_to_buffer (option_value, JJS_ENCODING_CESU8, (lit_utf8_byte_t*) &buffer[0], size - 1);

  jjs_value_free (option_value);

  JJS_ASSERT (w < size);
  buffer[w] = '\0';

  lit_utf8_byte_t* cursor_p = (lit_utf8_byte_t*) buffer;
  lit_utf8_byte_t c;

  while (*cursor_p != '\0')
  {
    c = *cursor_p;

    if (c <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
    {
      *cursor_p = (lit_utf8_byte_t) lit_char_to_lower_case (c, NULL);
    }

    cursor_p++;
  }

  for (jjs_size_t i = 0; i < len; i++)
  {
    if (strcmp (buffer, option_mappings_p[i].name_sz) == 0)
    {
      *out_p = option_mappings_p[i].value;
      return true;
    }
  }

  return false;
} /* jjs_util_map_option */
