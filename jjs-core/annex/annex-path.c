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

#include <ctype.h>

#include "ecma-builtin-helpers.h"
#include "jjs-platform.h"
#include "jjs-util.h"

#include "annex.h"

#define JJS_CONFIGURABLE_ENUMERABLE_WRITABLE_VALUE                                                  \
  (JJS_PROP_IS_CONFIGURABLE_DEFINED | JJS_PROP_IS_ENUMERABLE_DEFINED | JJS_PROP_IS_WRITABLE_DEFINED \
   | JJS_PROP_IS_CONFIGURABLE | JJS_PROP_IS_ENUMERABLE | JJS_PROP_IS_WRITABLE | JJS_PROP_IS_VALUE_DEFINED)

#define FILE_URL_PREFIX     "file:"
#define FILE_URL_PREFIX_LEN 5

#ifdef _WIN32
#include <ctype.h>
#define FILE_URL_ENCODE_PREFIX_WIN              "file:///"
#define FILE_URL_ENCODE_PREFIX_WIN_LEN          8
#define FILE_URL_ENCODE_PREFIX_UNC              FILE_URL_PREFIX
#define FILE_URL_ENCODE_PREFIX_UNC_LEN          FILE_URL_PREFIX_LEN
#define FILE_URL_ENCODE_PREFIX_WIN_NO_DRIVE     "file:///C:"
#define FILE_URL_ENCODE_PREFIX_WIN_NO_DRIVE_LEN 10
#else /* !_WIN32 */
#define FILE_URL_ENCODE_PREFIX_NIX     "file://"
#define FILE_URL_ENCODE_PREFIX_NIX_LEN 7
#endif /* _WIN32 */

static ecma_value_t annex_encode_path (jjs_context_t* context_p,
                                       const lit_utf8_byte_t* path_p,
                                       lit_utf8_size_t path_len,
                                       const lit_utf8_byte_t* prefix,
                                       lit_utf8_size_t prefix_len);
static lit_utf8_size_t annex_path_read_n (ecma_value_t str, lit_utf8_byte_t* buffer, lit_utf8_size_t buffer_size);

static lit_utf8_byte_t s_annex_to_hex_char[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

/**
 * Get the type (fs path or package) of a CommonJS request or ESM specifier.
 *
 * @param specifier string specifier or request
 * @return specifier type or ANNEX_SPECIFIER_TYPE_NONE if invalid
 */
annex_specifier_type_t
annex_path_specifier_type (ecma_value_t specifier)
{
  lit_utf8_byte_t path[FILE_URL_PREFIX_LEN] = { 0 };

  if (annex_path_read_n (specifier, path, FILE_URL_PREFIX_LEN) == 0)
  {
    return ANNEX_SPECIFIER_TYPE_NONE;
  }

  /* check for relative prefix . or C: , relative paths like file.txt will fallthrough to package type */
  if (jjsp_path_is_relative (path, FILE_URL_PREFIX_LEN))
  {
    return ANNEX_SPECIFIER_TYPE_RELATIVE;
  }

  /* check for absolute prefix / or C:/ */
  if (jjsp_path_is_absolute (path, FILE_URL_PREFIX_LEN))
  {
    return ANNEX_SPECIFIER_TYPE_ABSOLUTE;
  }

  if (memcmp (path, FILE_URL_PREFIX, FILE_URL_PREFIX_LEN) == 0)
  {
    return ANNEX_SPECIFIER_TYPE_FILE_URL;
  }

  return ANNEX_SPECIFIER_TYPE_PACKAGE;
} /* annex_path_specifier_type */

/**
 * Join a referrer path and specifier path. Optionally, normalize the resulting
 * full path.
 *
 * @param context_p JJS context
 * @param referrer referrer directory path
 * @param specifier file path
 * @param normalize if true, normalize the path
 * @return the resulting joined path. If the path is invalid, an empty value is returned.
 */
ecma_value_t
annex_path_join (jjs_context_t* context_p, ecma_value_t referrer, ecma_value_t specifier, bool normalize)
{
  if (!ecma_is_value_string (referrer) || !ecma_is_value_string (specifier))
  {
    return ECMA_VALUE_EMPTY;
  }

  ecma_string_t* path_components [] = {
    ecma_get_string_from_value (referrer),
    ecma_get_magic_string (LIT_MAGIC_STRING_SLASH_CHAR),
    ecma_get_string_from_value (specifier),
  };

  lit_utf8_size_t path_component_sizes [] = {
    ecma_string_get_size (path_components[0]),
    1,
    ecma_string_get_size (path_components[2]),
  };

  ecma_stringbuilder_t builder = ecma_stringbuilder_create_from_array (path_components, path_component_sizes, sizeof (path_components) / sizeof (path_components[0]));
  ecma_value_t result = ecma_make_string_value (ecma_stringbuilder_finalize (&builder));

  if (normalize)
  {
    ecma_value_t normalized = annex_path_normalize (context_p, result);

    ecma_free_value (result);

    return normalized;
  }
  else
  {
    return result;
  }
} /* annex_path_join */

/**
 * Normalize a path.
 *
 * @param context_p JJS context
 * @param path the string path
 * @return a normalized path or an empty value if the path is invalid or normalization fails
 */
ecma_value_t
annex_path_normalize (jjs_context_t* context_p, ecma_value_t path)
{
  if (!ecma_is_value_string (path) || jjs_string_length (context_p, path) == 0)
  {
    return ECMA_VALUE_EMPTY;
  }

  jjs_value_t result = jjs_platform_realpath (context_p, path, JJS_KEEP);

  if (!jjs_value_is_string (context_p, result))
  {
    jjs_value_free (context_p, result);
    result = ECMA_VALUE_EMPTY;
  }

  return result;
} /* annex_path_normalize */

/**
 * Get the current working directory.
 */
ecma_value_t
annex_path_cwd (jjs_context_t* context_p)
{
  jjs_value_t cwd = jjs_platform_cwd (context_p);

  if (!jjs_value_is_string (context_p, cwd))
  {
    jjs_value_free (context_p, cwd);
    cwd = ECMA_VALUE_EMPTY;
  }

  return cwd;
} /* annex_path_cwd */

/**
 * Get the directory name of a path.
 *
 * @param path filename or directory path
 * @return the directory name or undefined if the path is invalid
 */
ecma_value_t
annex_path_dirname (ecma_value_t path)
{
  if (!ecma_is_value_string (path))
  {
    return ECMA_VALUE_EMPTY;
  }

  ecma_string_t* path_p = ecma_get_string_from_value (path);

  if (ecma_string_get_length (path_p) == 0)
  {
    return ECMA_VALUE_EMPTY;
  }

  ecma_value_t result;

  ECMA_STRING_TO_UTF8_STRING (path_p, path_bytes_p, path_bytes_len);

  lit_utf8_size_t start_index;

  if (!jjsp_find_root_end_index (path_bytes_p, path_bytes_len, &start_index))
  {
    result = ECMA_VALUE_EMPTY;
    goto done;
  }

  lit_utf8_size_t last_index = path_bytes_len - 1;

  if (last_index == path_bytes_len)
  {
    result = ecma_copy_value (path);
    goto done;
  }

  // remove trailing slashes
  while (last_index > start_index && jjsp_path_is_separator (path_bytes_p[last_index]))
  {
    last_index--;
  }

  // move past basename to next slash
  while (last_index > start_index && !jjsp_path_is_separator (path_bytes_p[last_index]))
  {
    last_index--;
  }

  // remove any trailing slashes
  if (jjsp_path_is_separator (path_bytes_p[last_index]))
  {
    while (last_index > start_index && jjsp_path_is_separator (path_bytes_p[last_index]))
    {
      last_index--;
    }

    last_index++;
  }

  result = ecma_make_string_value (ecma_new_ecma_string_from_utf8 (&path_bytes_p[0], last_index));

done:
  ECMA_FINALIZE_UTF8_STRING (path_bytes_p, path_bytes_len);

  return result;
} /* annex_path_dirname */

/**
 * Gets the format of a filename by looking at the file extension.
 *
 * @param path filename to check
 * @return format string. if the format is unknown, 'none' is returned.
 */
ecma_value_t
annex_path_format (ecma_value_t path)
{
  static const char* JS = ".js";
  static const jjs_size_t JS_LEN = 3;
  static const char* CJS = ".cjs";
  static const jjs_size_t CJS_LEN = 4;
  static const char* MJS = ".mjs";
  static const jjs_size_t MJS_LEN = 4;
  static const char* SNAPSHOT = ".snapshot";
  static const jjs_size_t SNAPSHOT_LEN = 9;

  if (!ecma_is_value_string (path))
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING_NONE);
  }

  lit_magic_string_id_t id;
  ecma_string_t* path_p = ecma_get_string_from_value (path);

  ECMA_STRING_TO_UTF8_STRING (path_p, path_bytes_p, path_bytes_len);

  if (path_bytes_len > JS_LEN && memcmp (JS, (path_bytes_p + (path_bytes_len - JS_LEN)), JS_LEN) == 0)
  {
    id = LIT_MAGIC_STRING_JS;
  }
  else if (path_bytes_len > CJS_LEN
           && memcmp (CJS, (path_bytes_p + (path_bytes_len - CJS_LEN)), CJS_LEN) == 0)
  {
    id = LIT_MAGIC_STRING_COMMONJS;
  }
  else if (path_bytes_len > MJS_LEN
           && memcmp (MJS, (path_bytes_p + (path_bytes_len - MJS_LEN)), MJS_LEN) == 0)
  {
    id = LIT_MAGIC_STRING_MODULE;
  }
  else if (path_bytes_len > SNAPSHOT_LEN
           && memcmp (SNAPSHOT, (path_bytes_p + (path_bytes_len - SNAPSHOT_LEN)), SNAPSHOT_LEN) == 0)
  {
    id = LIT_MAGIC_STRING_SNAPSHOT;
  }
  else
  {
    id = LIT_MAGIC_STRING_NONE;
  }

  ECMA_FINALIZE_UTF8_STRING (path_bytes_p, path_bytes_len);

  return ecma_make_magic_string_value (id);
} /* annex_path_format */

/**
 * Converts an absolute file path to a valid file url.
 *
 * @param path path string to an absolute filename
 * @return string file url or empty value on error
 */
ecma_value_t
annex_path_to_file_url (jjs_context_t* context_p, ecma_value_t path)
{
  if (!ecma_is_value_string (path))
  {
    return ECMA_VALUE_EMPTY;
  }

  ecma_string_t* path_p = ecma_get_string_from_value (path);

  if (ecma_string_is_empty (path_p))
  {
    return ECMA_VALUE_EMPTY;
  }

  // note: path_bytes_p is a CESU8 encoded, despite the macro indicating otherwise
  ECMA_STRING_TO_UTF8_STRING (path_p, path_bytes_p, path_bytes_len);

  const char* prefix;
  lit_utf8_size_t prefix_len;

#ifdef _WIN32
  if (path_bytes_len > 2 && path_bytes_p[0] == '\\' && path_bytes_p[1] == '\\')
  {
    // we could further perform unc path validation, but not sure if its necessary
    prefix = FILE_URL_ENCODE_PREFIX_UNC;
    prefix_len = FILE_URL_ENCODE_PREFIX_UNC_LEN;
  }
  else if (path_bytes_len > 2 && isalpha (path_bytes_p[0]) && path_bytes_p[1] == ':' && jjsp_path_is_separator (path_bytes_p[2]))
  {
    prefix = FILE_URL_ENCODE_PREFIX_WIN;
    prefix_len = FILE_URL_ENCODE_PREFIX_WIN_LEN;
  }
  else if (path_bytes_len > 0 && jjsp_path_is_separator (path_bytes_p[0]))
  {
    // if the path is just a slash, C: is picked. not sure if this is correct
    prefix = FILE_URL_ENCODE_PREFIX_WIN_NO_DRIVE;
    prefix_len = FILE_URL_ENCODE_PREFIX_WIN_NO_DRIVE_LEN;
  }
#else // !_WIN32
  if (path_bytes_len > 0 && jjsp_path_is_separator (path_bytes_p[0]))
  {
    prefix = FILE_URL_ENCODE_PREFIX_NIX;
    prefix_len = FILE_URL_ENCODE_PREFIX_NIX_LEN;
  }
#endif // _WIN32
  else
  {
    // relative paths and anything else are not handled by this function
    prefix = NULL;
    prefix_len = 0;
  }

  ecma_value_t result;

  if (prefix_len > 0)
  {
    result = annex_encode_path (context_p, path_bytes_p, path_bytes_len, (const lit_utf8_byte_t*) prefix, prefix_len);
  }
  else
  {
    result = ECMA_VALUE_EMPTY;
  }

  ECMA_FINALIZE_UTF8_STRING (path_bytes_p, path_bytes_len);

  return result;
} /* annex_path_to_file_url */

/**
 * Gets the basename of the given path.
 *
 * If not a string, an invalid filename, "", "." or "..", ECMA_EMPTY_VALUE is returned.
 *
 * @param path the string path to work on
 * @return the basename of the given path or ECMA_EMPTY_VALUE for all other cases.
 */
ecma_value_t
annex_path_basename (ecma_value_t path)
{
  // note: this function may not work correctly with unc paths on windows

  if (!ecma_is_value_string(path))
  {
    return ECMA_VALUE_EMPTY;
  }

  ecma_value_t result;
  ecma_string_t* path_p = ecma_get_string_from_value (path);

  ECMA_STRING_TO_UTF8_STRING (path_p, path_bytes_p, path_bytes_len);

  // "" or "." or ".." -> empty
  if ((path_bytes_len == 0) || (path_bytes_len == 1 && path_bytes_p[0] == '.') || (path_bytes_len == 2 && path_bytes_p[0] == '.' && path_bytes_p[1] == '.'))
  {
    result = ECMA_VALUE_EMPTY;
    goto done;
  }

  lit_utf8_size_t i;
  lit_utf8_size_t last_slash_index = path_bytes_len;

  for (i = 0; i < path_bytes_len; i++)
  {
    if (jjsp_path_is_separator (path_bytes_p[i]))
    {
      last_slash_index = i;
    }
  }

  if (last_slash_index == path_bytes_len)
  {
    result = ecma_copy_value (path);
    goto done;
  }

  if (last_slash_index + 1 >= path_bytes_len)
  {
    result = ECMA_VALUE_EMPTY;
    goto done;
  }

  result = ecma_make_string_value (ecma_new_ecma_string_from_utf8 (&path_bytes_p[last_slash_index + 1], path_bytes_len - last_slash_index - 1));

done:
  ECMA_FINALIZE_UTF8_STRING (path_bytes_p, path_bytes_len);
  return result;
} /* annex_path_basename */

static lit_utf8_size_t
annex_encode_char (lit_utf8_byte_t c, lit_utf8_byte_t* buffer)
{
  // TODO: bounds checking

  if (isalnum (c))
  {
    *buffer = c;
    return 1;
  }
  else
  {
    switch (c)
    {
#ifdef _WIN32
      case '\\':
        *buffer = '/';
        return 1;
#endif // _WIN32
      case '-':
      case '.':
      case '_':
      case '~':
      case ':':
      case '&':
      case '=':
      case ';':
      case '/':
        *buffer = c;
        return 1;
      default:
        // TODO: no bounds checking!
        buffer[0] = '%';
        buffer[1] = s_annex_to_hex_char[(c >> 4) & 0x0f];
        buffer[2] = s_annex_to_hex_char[c & 0x0f];
        return 3;
    }
  }
} /* annex_encode_char */

static ecma_value_t
annex_encode_path (jjs_context_t* context_p,
                   const lit_utf8_byte_t* path_p,
                   lit_utf8_size_t path_len,
                   const lit_utf8_byte_t* prefix,
                   lit_utf8_size_t prefix_len)
{
  /* maximum size if every character in path gets encoded to %XX format */
  const lit_utf8_size_t encoded_capacity = prefix_len + (path_len * 3);
  jjs_allocator_t* allocator = jjs_util_context_acquire_scratch_allocator (context_p);
  lit_utf8_byte_t* encoded_p = allocator->alloc (allocator, encoded_capacity);

  if (encoded_p == NULL)
  {
    jjs_util_context_release_scratch_allocator (context_p);
    return ECMA_VALUE_EMPTY;
  }

  memcpy (encoded_p, prefix, prefix_len);

  const lit_utf8_byte_t* path_cursor_p = path_p;
  const lit_utf8_byte_t* path_end_p = path_p + path_len;
  ecma_char_t ch;
  ecma_char_t next_ch;
  lit_code_point_t code_point;
  lit_utf8_size_t read_size;
  lit_utf8_byte_t octets[LIT_UTF8_MAX_BYTES_IN_CODE_POINT];
  lit_utf8_size_t encoded_len = prefix_len;
  bool has_error = false;

  while (path_cursor_p < path_end_p)
  {
    read_size = lit_read_code_unit_from_cesu8_safe (path_cursor_p, path_end_p, &ch);

    if (read_size == 0)
    {
      has_error = true;
      break;
    }

    path_cursor_p += read_size;

    if (lit_is_code_point_utf16_low_surrogate (ch))
    {
      has_error = true;
      break;
    }

    code_point = ch;

    if (lit_is_code_point_utf16_high_surrogate (ch))
    {
      if (path_cursor_p >= path_end_p)
      {
        has_error = true;
        break;
      }

      read_size = lit_read_code_unit_from_cesu8_safe (path_cursor_p, path_end_p, &next_ch);

      if (read_size == 0)
      {
        has_error = true;
        break;
      }

      if (lit_is_code_point_utf16_low_surrogate (next_ch))
      {
        code_point = lit_convert_surrogate_pair_to_code_point (ch, next_ch);
        path_cursor_p += read_size;
      }
      else
      {
        has_error = true;
        break;
      }
    }

    read_size = lit_code_point_to_utf8 (code_point, octets);

    JJS_ASSERT (encoded_len + read_size < encoded_capacity);

    if (read_size == 1)
    {
      encoded_len += annex_encode_char (octets[0], &encoded_p[encoded_len]);
    }
    else
    {
      for (lit_utf8_size_t i = 0; i < read_size; i++)
      {
        encoded_p[encoded_len++] = '%';
        encoded_p[encoded_len++] = s_annex_to_hex_char[(octets[i] >> 4) & 0x0f];
        encoded_p[encoded_len++] = s_annex_to_hex_char[octets[i] & 0x0f];
      }
    }
  }

  ecma_value_t result;

  if (!has_error)
  {
    result = ecma_make_string_value (ecma_new_ecma_string_from_ascii (encoded_p, encoded_len));
  }
  else
  {
    result = ECMA_VALUE_EMPTY;
  }

  allocator->free (allocator, encoded_p, encoded_capacity);
  jjs_util_context_release_scratch_allocator (context_p);

  return result;
} /* annex_encode_path */

static lit_utf8_size_t
annex_path_read_n (ecma_value_t str, lit_utf8_byte_t* buffer, lit_utf8_size_t buffer_size)
{
  if (!ecma_is_value_string (str))
  {
    return 0;
  }

  return ecma_string_copy_to_buffer (ecma_get_string_from_value (str), buffer, buffer_size, JJS_ENCODING_CESU8);
} /* annex_path_read_n */
