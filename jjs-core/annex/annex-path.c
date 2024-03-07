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

#include "annex.h"

#include "ecma-builtin-helpers.h"

#define JJS_CONFIGURABLE_ENUMERABLE_WRITABLE_VALUE (JJS_PROP_IS_CONFIGURABLE_DEFINED \
                                                    | JJS_PROP_IS_ENUMERABLE_DEFINED \
                                                    | JJS_PROP_IS_WRITABLE_DEFINED \
                                                    | JJS_PROP_IS_CONFIGURABLE \
                                                    | JJS_PROP_IS_ENUMERABLE \
                                                    | JJS_PROP_IS_WRITABLE \
                                                    | JJS_PROP_IS_VALUE_DEFINED)

#ifdef _WIN32
#include <ctype.h>
#define is_separator(c) ((c) == '/' || (c) == '\\')
#define is_absolute(BUFFER) (is_separator (BUFFER[0]) || (isalpha (BUFFER[0]) && BUFFER[1] == ':' && is_separator (BUFFER[2])))
static ecma_value_t annex_replace_backslashes (ecma_string_t* path_p);
#else /* !_WIN32 */
#define is_separator(c) ((c) == '/')
#define is_absolute(BUFFER) is_separator (BUFFER[0])
#endif /* _WIN32 */

#define FILE_URL_PREFIX "file:"
#define FILE_URL_PREFIX_LEN 5

static lit_utf8_size_t annex_path_read_n (ecma_value_t str, lit_utf8_byte_t* buffer, lit_utf8_size_t buffer_size)
{
  if (!ecma_is_value_string (str))
  {
    return 0;
  }

  return ecma_string_copy_to_buffer (ecma_get_string_from_value (str), buffer, buffer_size, JJS_ENCODING_CESU8);
} /* annex_path_read_n */

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

  if ((path[0] == '.' && is_separator (path[1])) || (path[0] == '.' && path[1] == '.' && is_separator (path[2])))
  {
    return ANNEX_SPECIFIER_TYPE_RELATIVE;
  }

  if (is_absolute (path))
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
 * @param referrer referrer directory path
 * @param specifier file path
 * @param normalize if true, normalize the path
 * @return the resulting joined path. If the path is invalid, an empty value is returned.
 */
ecma_value_t
annex_path_join (ecma_value_t referrer, ecma_value_t specifier, bool normalize)
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
    ecma_value_t normalized = annex_path_normalize (result);

    ecma_free_value (result);

    return normalized;
  }
  else
  {
    return result;
  }
} /* annex_path_join */

/**
 * Normalize a path using a cstring and it's length.
 *
 * @param str path to normalize (must be null terminated)
 * @param str_len length of str (passed in for performance reasons)
 * @return a normalized path or an empty value if the path is invalid or normalization fails
 */
static ecma_value_t
annex_path_normalize_from_string (const char* str, jjs_size_t str_len)
{
  lit_utf8_byte_t* normalized_path_p = jjs_port_path_normalize ((const jjs_char_t *) str, str_len);

  if (normalized_path_p == NULL)
  {
    return ECMA_VALUE_EMPTY;
  }

  lit_utf8_size_t buffer_size = (lit_utf8_size_t)strlen ((const char*) normalized_path_p);
  ecma_string_t* result = ecma_new_ecma_string_from_utf8_converted_to_cesu8 (normalized_path_p, buffer_size);

  jjs_port_path_free (normalized_path_p);

  return ecma_make_string_value (result);
} /* annex_path_normalize_from_string */

/**
 * Normalize a path.
 *
 * @param path the string path
 * @return a normalized path or an empty value if the path is invalid or normalization fails
 */
ecma_value_t
annex_path_normalize (ecma_value_t path)
{
  ecma_cstr_t path_cstr = ecma_string_to_cstr (path);

  if (path_cstr.size == 0)
  {
    return ECMA_VALUE_EMPTY;
  }

  ecma_value_t result = annex_path_normalize_from_string (path_cstr.str_p, path_cstr.size);

  ecma_free_cstr(&path_cstr);

  return result;
} /* annex_path_normalize */

/**
 * Get the current working directory.
 */
ecma_value_t
annex_path_cwd (void)
{
  return annex_path_normalize_from_string (".", 1);
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
  ecma_cstr_t path_cstr = ecma_string_to_cstr (path);

  if (path_cstr.size == 0)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  jjs_size_t result_len;
  jjs_char_t* dirname_p = jjs_port_path_dirname (path_cstr.str_p, &result_len);
  ecma_free_cstr(&path_cstr);

  ecma_string_t* result_p = ecma_new_ecma_string_from_utf8_converted_to_cesu8 (
    (lit_utf8_byte_t*)dirname_p, result_len);
  jjs_port_path_free (dirname_p);

  return ecma_make_string_value (result_p);
} /* annex_path_dirname */

/**
 * Checks if a string ends with a given c-string.
 *
 * @param str string
 * @param search_sz search string
 * @return true if search string found, otherwise, false
 */
static bool
ends_with (ecma_value_t str, const char* search_sz)
{
  ecma_string_t* path_string_p = ecma_get_string_from_value (str);
  ecma_value_t search_string = ecma_string_ascii_sz (search_sz);

  ecma_value_t value = ecma_builtin_helper_string_prototype_object_index_of (
    path_string_p, search_string, ECMA_VALUE_UNDEFINED, ECMA_STRING_ENDS_WITH);
  bool result = ecma_is_value_true (value);

  ecma_free_value(value);
  ecma_free_value (search_string);

  return result;
} /* ends_with */

/**
 * Gets the format of a filename by looking at the file extension.
 *
 * @param path filename to check
 * @return format string. if the format is unknown, 'none' is returned.
 */
ecma_value_t
annex_path_format (ecma_value_t path)
{
  lit_magic_string_id_t id;

  if (ends_with (path, ".js"))
  {
    id = LIT_MAGIC_STRING_JS;
  }
  else if (ends_with (path, ".cjs"))
  {
    id = LIT_MAGIC_STRING_COMMONJS;
  }
  else if (ends_with (path, ".mjs"))
  {
    id = LIT_MAGIC_STRING_MODULE;
  }
  else if (ends_with (path, ".snapshot"))
  {
    id = LIT_MAGIC_STRING_SNAPSHOT;
  }
  else
  {
    id = LIT_MAGIC_STRING_NONE;
  }

  return ecma_make_magic_string_value (id);
} /* annex_path_format */

/**
 * Converts an absolute file path to a valid file url.
 *
 * @param path path string to an absolute filename
 * @return string file url or empty value on error
 */
ecma_value_t
annex_path_to_file_url (ecma_value_t path)
{
  lit_utf8_byte_t start[3] = {0};

  if (annex_path_read_n (path, start, 3) == 0)
  {
    return ECMA_VALUE_EMPTY;
  }

  ecma_value_t unencoded;
  ecma_value_t prefix;

#ifdef _WIN32
  if (start[0] == '\\' && start[1] == '\\')
  {
    // TODO: check for valid unc path \\host\..
    prefix = ecma_string_ascii_sz ("file:");
  }
  else if (isalpha(start[0]) && start[1] == ':' && is_separator (start[2]))
  {
    prefix = ecma_string_ascii_sz ("file:///");
  }
  else if (is_separator (start[0]))
  {
    prefix = ecma_string_ascii_sz ("file:///C:");
  }
  else
  {
    prefix = ECMA_VALUE_EMPTY;
  }

  if (ecma_is_value_string(prefix))
  {
    jjs_value_t t = annex_replace_backslashes (ecma_get_string_from_value (path));

    unencoded = ecma_is_value_string (t) ? jjs_binary_op (JJS_BIN_OP_ADD, prefix, t) : ECMA_VALUE_EMPTY;

    jjs_value_free (t);
  }
  else
  {
    unencoded = ECMA_VALUE_EMPTY;
  }
#else // !_WIN32
  prefix = is_separator (start[0]) ? ecma_string_ascii_sz ("file://") : ECMA_VALUE_EMPTY;
  unencoded = ecma_is_value_string(prefix) ? jjs_binary_op (JJS_BIN_OP_ADD, prefix, t) : ECMA_VALUE_EMPTY;
#endif // _WIN32

  jjs_value_free (prefix);

  if (!ecma_is_value_string(unencoded))
  {
    jjs_value_free (unencoded);
    return ECMA_VALUE_EMPTY;
  }

  // TODO: ? and # are not getting encoded
  jjs_value_t encoded = ecma_builtin_global_encode_uri (unencoded);

  jjs_value_free (unencoded);

  return encoded;
} /* annex_path_to_file_url */

#ifdef _WIN32
static ecma_value_t
annex_replace_backslashes (ecma_string_t* path_p)
{
  ecma_value_t result ;
  ECMA_STRING_TO_UTF8_STRING(path_p, path_bytes_p, path_bytes_size);
  lit_utf8_byte_t* copy_p = jmem_heap_alloc_block_null_on_error (path_bytes_size);

  if (copy_p)
  {
    for (lit_utf8_size_t i = 0; i < path_bytes_size; i++)
    {
      copy_p[i] = path_bytes_p[i] == '\\' ? '/' : path_bytes_p[i];
    }

    result = ecma_make_string_value (ecma_new_ecma_string_from_utf8 (copy_p, path_bytes_size));
    jmem_heap_free_block (copy_p, path_bytes_size);
  }
  else
  {
    result = ECMA_VALUE_EMPTY;
  }

  ECMA_FINALIZE_UTF8_STRING (path_bytes_p, path_bytes_size);

  return result;
} /* annex_replace_backslashes */
#endif // _WIN32