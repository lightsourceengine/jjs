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

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtin-regexp.inc.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"

#include "jcontext.h"
#include "jrt-libc-includes.h"
#include "jrt.h"
#include "lit-char-helpers.h"
#include "lit-strings.h"

#if JJS_BUILTIN_REGEXP
#include "ecma-regexp-object.h"
#endif /* JJS_BUILTIN_REGEXP */

#if JJS_BUILTIN_STRING

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
#define BUILTIN_CUSTOM_DISPATCH

/**
 * List of built-in routine identifiers.
 */
enum
{
  ECMA_STRING_PROTOTYPE_ROUTINE_START = 0,
  /* Note: These 4 routines MUST be in this order */
  ECMA_STRING_PROTOTYPE_TO_STRING,
  ECMA_STRING_PROTOTYPE_VALUE_OF,
  ECMA_STRING_PROTOTYPE_CHAR_AT,
  ECMA_STRING_PROTOTYPE_CHAR_CODE_AT,

  ECMA_STRING_PROTOTYPE_CONCAT,
  ECMA_STRING_PROTOTYPE_SLICE,
  ECMA_STRING_PROTOTYPE_AT,

  ECMA_STRING_PROTOTYPE_LOCALE_COMPARE,

  ECMA_STRING_PROTOTYPE_MATCH,
  ECMA_STRING_PROTOTYPE_REPLACE,
  ECMA_STRING_PROTOTYPE_SEARCH,

  ECMA_STRING_PROTOTYPE_SPLIT,
  ECMA_STRING_PROTOTYPE_SUBSTRING,
  ECMA_STRING_PROTOTYPE_TO_LOWER_CASE,
  ECMA_STRING_PROTOTYPE_TO_LOCAL_LOWER_CASE,
  ECMA_STRING_PROTOTYPE_TO_UPPER_CASE,
  ECMA_STRING_PROTOTYPE_TO_LOCAL_UPPER_CASE,
  ECMA_STRING_PROTOTYPE_TRIM,

  ECMA_STRING_PROTOTYPE_SUBSTR,

  ECMA_STRING_PROTOTYPE_REPEAT,
  ECMA_STRING_PROTOTYPE_CODE_POINT_AT,
  ECMA_STRING_PROTOTYPE_PAD_START,
  ECMA_STRING_PROTOTYPE_PAD_END,
  /* Note: These 5 routines MUST be in this order */
  ECMA_STRING_PROTOTYPE_LAST_INDEX_OF,
  ECMA_STRING_PROTOTYPE_INDEX_OF,
  ECMA_STRING_PROTOTYPE_STARTS_WITH,
  ECMA_STRING_PROTOTYPE_INCLUDES,
  ECMA_STRING_PROTOTYPE_ENDS_WITH,

  ECMA_STRING_PROTOTYPE_ITERATOR,
  ECMA_STRING_PROTOTYPE_REPLACE_ALL,
  ECMA_STRING_PROTOTYPE_MATCH_ALL,

  ECMA_STRING_PROTOTYPE_IS_WELL_FORMED,
  ECMA_STRING_PROTOTYPE_TO_WELL_FORMED,
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-string-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID  string_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup stringprototype ECMA String.prototype object built-in
 * @{
 */

/**
 * The String.prototype object's 'toString' and 'valueOf' routines
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.2
 *          ECMA-262 v5, 15.5.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_to_string (ecma_context_t *context_p, /**< JJS context */
                                                ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_string (this_arg))
  {
    return ecma_copy_value (context_p, this_arg);
  }

  if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (context_p, this_arg);

    if (ecma_object_class_is (object_p, ECMA_OBJECT_CLASS_STRING))
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      JJS_ASSERT (ecma_is_value_string (ext_object_p->u.cls.u3.value));

      return ecma_copy_value (context_p, ext_object_p->u.cls.u3.value);
    }
  }

  return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_THIS_NOT_STRING_OBJECT);
} /* ecma_builtin_string_prototype_object_to_string */

/**
 * Helper function for the String.prototype object's 'charAt' and charCodeAt' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_char_at_helper (ecma_context_t *context_p, /**< JJS context */
                                              ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t arg, /**< routine's argument */
                                              bool charcode_mode) /**< routine mode */
{
  /* 3 */
  ecma_number_t index_num;
  ecma_value_t to_num_result = ecma_op_to_integer (context_p, arg, &index_num);

  if (JJS_UNLIKELY (!ecma_is_value_empty (to_num_result)))
  {
    return to_num_result;
  }

  /* 2 */
  ecma_string_t *original_string_p = ecma_op_to_string (context_p, this_arg);
  if (JJS_UNLIKELY (original_string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 4 */
  const lit_utf8_size_t len = ecma_string_get_length (context_p, original_string_p);

  /* 5 */
  // When index_num is NaN, then the first two comparisons are false
  if (index_num < 0 || index_num >= len || (ecma_number_is_nan (index_num) && len == 0))
  {
    ecma_deref_ecma_string (context_p, original_string_p);
    return (charcode_mode ? ecma_make_nan_value (context_p) : ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY));
  }

  /* 6 */
  /*
   * String length is currently uint32_t, but index_num may be bigger,
   * ToInteger performs floor, while ToUInt32 performs modulo 2^32,
   * hence after the check 0 <= index_num < len we assume to_uint32 can be used.
   * We assume to_uint32 (NaN) is 0.
   */
  JJS_ASSERT (ecma_number_is_nan (index_num) || ecma_number_to_uint32 (index_num) == ecma_number_trunc (index_num));

  ecma_char_t new_ecma_char = ecma_string_get_char_at_pos (context_p, original_string_p, ecma_number_to_uint32 (index_num));
  ecma_deref_ecma_string (context_p, original_string_p);

  return (charcode_mode ? ecma_make_uint32_value (context_p, new_ecma_char)
                        : ecma_make_string_value (context_p, ecma_new_ecma_string_from_code_unit (context_p, new_ecma_char)));
} /* ecma_builtin_string_prototype_char_at_helper */

/**
 * The String.prototype object's 'concat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_concat (ecma_context_t *context_p, /**< JJS context */
                                             ecma_string_t *this_string_p, /**< this argument */
                                             const ecma_value_t *argument_list_p, /**< arguments list */
                                             uint32_t arguments_number) /**< number of arguments */
{
  ecma_stringbuilder_t builder = ecma_stringbuilder_create_from (context_p, this_string_p);

  /* 5 */
  for (uint32_t arg_index = 0; arg_index < arguments_number; ++arg_index)
  {
    /* 5a, b */
    ecma_string_t *get_arg_string_p = ecma_op_to_string (context_p, argument_list_p[arg_index]);

    if (JJS_UNLIKELY (get_arg_string_p == NULL))
    {
      ecma_stringbuilder_destroy (&builder);
      return ECMA_VALUE_ERROR;
    }

    ecma_stringbuilder_append (&builder, get_arg_string_p);

    ecma_deref_ecma_string (context_p, get_arg_string_p);
  }

  /* 6 */
  return ecma_make_string_value (context_p, ecma_stringbuilder_finalize (&builder));
} /* ecma_builtin_string_prototype_object_concat */

/**
 * The String.prototype object's 'localeCompare' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_locale_compare (ecma_context_t *context_p, /**< JJS context */
                                                     ecma_string_t *this_string_p, /**< this argument */
                                                     ecma_value_t arg) /**< routine's argument */
{
  /* 3. */
  ecma_string_t *arg_string_p = ecma_op_to_string (context_p, arg);

  if (JJS_UNLIKELY (arg_string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_number_t result = ECMA_NUMBER_ZERO;

  if (ecma_compare_ecma_strings_relational (context_p, this_string_p, arg_string_p))
  {
    result = ECMA_NUMBER_MINUS_ONE;
  }
  else if (!ecma_compare_ecma_strings (this_string_p, arg_string_p))
  {
    result = ECMA_NUMBER_ONE;
  }
  else
  {
    result = ECMA_NUMBER_ZERO;
  }

  ecma_deref_ecma_string (context_p, arg_string_p);

  return ecma_make_number_value (context_p, result);
} /* ecma_builtin_string_prototype_object_locale_compare */

#if JJS_BUILTIN_REGEXP
/**
 * The String.prototype object's 'match' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_match (ecma_context_t *context_p, /**< JJS context */
                                            ecma_value_t this_argument, /**< this argument */
                                            ecma_value_t regexp_arg) /**< routine's argument */
{
  /* 3. */
  if (!(ecma_is_value_undefined (regexp_arg) || ecma_is_value_null (regexp_arg)))
  {
    /* 3.a */
    ecma_value_t matcher = ecma_op_get_method_by_symbol_id (context_p, regexp_arg, LIT_GLOBAL_SYMBOL_MATCH);

    /* 3.b */
    if (ECMA_IS_VALUE_ERROR (matcher))
    {
      return matcher;
    }

    /* 3.c */
    if (!ecma_is_value_undefined (matcher))
    {
      /* 3.c.i */
      ecma_object_t *matcher_method = ecma_get_object_from_value (context_p, matcher);
      ecma_value_t result = ecma_op_function_call (context_p, matcher_method, regexp_arg, &this_argument, 1);
      ecma_deref_object (matcher_method);
      return result;
    }
  }

  /* 4. */
  ecma_string_t *this_str_p = ecma_op_to_string (context_p, this_argument);

  /* 5. */
  if (JJS_UNLIKELY (this_str_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 6. */
  ecma_object_t *regexp_obj_p = ecma_op_regexp_alloc (context_p, NULL);

  if (JJS_UNLIKELY (regexp_obj_p == NULL))
  {
    ecma_deref_ecma_string (context_p, this_str_p);
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t new_regexp = ecma_op_create_regexp_from_pattern (context_p, regexp_obj_p, regexp_arg, ECMA_VALUE_UNDEFINED);

  /* 7. */
  if (ECMA_IS_VALUE_ERROR (new_regexp))
  {
    ecma_deref_object (regexp_obj_p);
    ecma_deref_ecma_string (context_p, this_str_p);
    return new_regexp;
  }
  ecma_value_t this_str_value = ecma_make_string_value (context_p, this_str_p);

  /* 8. */
  ecma_value_t ret_value = ecma_op_invoke_by_symbol_id (context_p, new_regexp, LIT_GLOBAL_SYMBOL_MATCH, &this_str_value, 1);

  ecma_deref_ecma_string (context_p, this_str_p);
  ecma_free_value (context_p, new_regexp);

  return ret_value;
} /* ecma_builtin_string_prototype_object_match */

/**
 * The String.prototype object's 'matchAll' routine
 *
 * See also:
 *          ECMA-262 v11, 21.1.3.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_match_all (ecma_context_t *context_p, /**< JJS context */
                                                ecma_value_t this_argument, /**< this argument */
                                                ecma_value_t regexp_arg) /**< routine's argument */
{
  /* 2. */
  if (!ecma_is_value_null (regexp_arg) && !ecma_is_value_undefined (regexp_arg))
  {
    /* 2.a */
    ecma_value_t is_regexp = ecma_op_is_regexp (context_p, regexp_arg);

    if (ECMA_IS_VALUE_ERROR (is_regexp))
    {
      return is_regexp;
    }

    /* 2.b */
    if (ecma_is_value_true (is_regexp))
    {
      /* 2.b.i */
      ecma_object_t *regexp_obj_p = ecma_get_object_from_value (context_p, regexp_arg);
      ecma_value_t get_flags = ecma_op_object_get_by_magic_id (context_p, regexp_obj_p, LIT_MAGIC_STRING_FLAGS);

      if (ECMA_IS_VALUE_ERROR (get_flags))
      {
        return get_flags;
      }

      /* 2.b.ii */
      if (!ecma_op_require_object_coercible (context_p, get_flags))
      {
        ecma_free_value (context_p, get_flags);
        return ECMA_VALUE_ERROR;
      }

      /* 2.b.iii */
      ecma_string_t *flags = ecma_op_to_string (context_p, get_flags);

      ecma_free_value (context_p, get_flags);

      if (JJS_UNLIKELY (flags == NULL))
      {
        return ECMA_VALUE_ERROR;
      }

      uint16_t parsed_flag;
      ecma_value_t flag_parse = ecma_regexp_parse_flags (context_p, flags, &parsed_flag);

      ecma_deref_ecma_string (context_p, flags);

      if (ECMA_IS_VALUE_ERROR (flag_parse))
      {
        return flag_parse;
      }

      if (!(parsed_flag & RE_FLAG_GLOBAL))
      {
        return ecma_raise_type_error (context_p, ECMA_ERR_REGEXP_ARGUMENT_SHOULD_HAVE_GLOBAL_FLAG);
      }
    }

    /* 2.c */
    ecma_value_t matcher = ecma_op_get_method_by_symbol_id (context_p, regexp_arg, LIT_GLOBAL_SYMBOL_MATCH_ALL);

    if (ECMA_IS_VALUE_ERROR (matcher))
    {
      return matcher;
    }

    /* 2.d */
    if (!ecma_is_value_undefined (matcher))
    {
      /* 2.d.i */
      ecma_object_t *matcher_method = ecma_get_object_from_value (context_p, matcher);
      ecma_value_t result = ecma_op_function_call (context_p, matcher_method, regexp_arg, &this_argument, 1);
      ecma_deref_object (matcher_method);
      return result;
    }
  }

  /* 3. */
  ecma_string_t *str_p = ecma_op_to_string (context_p, this_argument);

  if (JJS_UNLIKELY (str_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 4. */
  ecma_object_t *new_regexp_obj_p = ecma_op_regexp_alloc (context_p, NULL);

  if (JJS_UNLIKELY (new_regexp_obj_p == NULL))
  {
    ecma_deref_ecma_string (context_p, str_p);
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t new_regexp = ecma_op_create_regexp_from_pattern (context_p, new_regexp_obj_p, regexp_arg, ECMA_VALUE_UNDEFINED);

  if (ECMA_IS_VALUE_ERROR (new_regexp))
  {
    ecma_deref_ecma_string (context_p, str_p);
    ecma_deref_object (new_regexp_obj_p);
    return new_regexp;
  }

  /* 5. */
  ecma_value_t string_arg = ecma_make_string_value (context_p, str_p);
  ecma_value_t ret_value = ecma_op_invoke_by_symbol_id (context_p, new_regexp, LIT_GLOBAL_SYMBOL_MATCH_ALL, &string_arg, 1);

  ecma_deref_ecma_string (context_p, str_p);
  ecma_free_value (context_p, new_regexp);

  return ret_value;
} /* ecma_builtin_string_prototype_object_match_all */

/**
 * The String.prototype object's 'replace' and 'replaceAll' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.11 (replace ES5)
 *          ECMA-262 v6, 21.1.3.14 (replace ES6)
 *          ECMA-262 v12, 21.1.3.18 (replaceAll)
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_replace_helper (ecma_context_t *context_p, /**< JJS context */
                                                     ecma_value_t this_value, /**< this argument */
                                                     ecma_value_t search_value, /**< routine's first argument */
                                                     ecma_value_t replace_value, /**< routine's second argument */
                                                     bool replace_all)
{
  if (!(ecma_is_value_undefined (search_value) || ecma_is_value_null (search_value)))
  {
    if (replace_all)
    {
      ecma_value_t is_regexp = ecma_op_is_regexp (context_p, search_value);

      if (ECMA_IS_VALUE_ERROR (is_regexp))
      {
        return is_regexp;
      }

      if (ecma_is_value_true (is_regexp))
      {
        ecma_object_t *regexp_obj_p = ecma_get_object_from_value (context_p, search_value);
        ecma_value_t get_flags = ecma_op_object_get_by_magic_id (context_p, regexp_obj_p, LIT_MAGIC_STRING_FLAGS);

        if (ECMA_IS_VALUE_ERROR (get_flags))
        {
          return get_flags;
        }

        if (!ecma_op_require_object_coercible (context_p, get_flags))
        {
          ecma_free_value (context_p, get_flags);
          return ECMA_VALUE_ERROR;
        }

        ecma_string_t *flags = ecma_op_to_string (context_p, get_flags);

        ecma_free_value (context_p, get_flags);

        if (JJS_UNLIKELY (flags == NULL))
        {
          return ECMA_VALUE_ERROR;
        }

        bool have_global_flag = lit_find_char_in_string (context_p, flags, LIT_CHAR_LOWERCASE_G);

        ecma_deref_ecma_string (context_p, flags);

        if (!have_global_flag)
        {
          return ecma_raise_type_error (context_p, ECMA_ERR_REGEXP_ARGUMENT_SHOULD_HAVE_GLOBAL_FLAG);
        }
      }
    }

    ecma_object_t *obj_p = ecma_get_object_from_value (context_p, ecma_op_to_object (context_p, search_value));
    ecma_value_t replace_symbol = ecma_op_object_get_by_symbol_id (context_p, obj_p, LIT_GLOBAL_SYMBOL_REPLACE);
    ecma_deref_object (obj_p);

    if (ECMA_IS_VALUE_ERROR (replace_symbol))
    {
      return replace_symbol;
    }

    if (!ecma_is_value_undefined (replace_symbol) && !ecma_is_value_null (replace_symbol))
    {
      ecma_value_t arguments[] = { this_value, replace_value };
      ecma_value_t replace_result = ecma_op_function_validated_call (context_p, replace_symbol, search_value, arguments, 2);
      ecma_free_value (context_p, replace_symbol);

      return replace_result;
    }
  }

  ecma_string_t *input_str_p = ecma_op_to_string (context_p, this_value);

  if (input_str_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t result = ECMA_VALUE_ERROR;

  ecma_string_t *search_str_p = ecma_op_to_string (context_p, search_value);
  if (search_str_p == NULL)
  {
    ecma_deref_ecma_string (context_p, input_str_p);
    return result;
  }

  ecma_replace_context_t replace_ctx;
  replace_ctx.capture_count = 0;
  replace_ctx.u.captures_p = NULL;

  replace_ctx.replace_str_p = NULL;
  if (!ecma_op_is_callable (context_p, replace_value))
  {
    replace_ctx.replace_str_p = ecma_op_to_string (context_p, replace_value);
    if (replace_ctx.replace_str_p == NULL)
    {
      goto cleanup_search;
    }
  }

  uint8_t input_flags = ECMA_STRING_FLAG_IS_ASCII;
  lit_utf8_byte_t input_str_uint_buffer_p[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
  lit_utf8_byte_t search_str_uint_buffer_p[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
  lit_utf8_size_t search_size;
  lit_utf8_size_t search_length;
  uint8_t search_flags = ECMA_STRING_FLAG_IS_ASCII;
  const lit_utf8_byte_t *search_buf_p;

  replace_ctx.string_p = ecma_string_get_chars (context_p, input_str_p, &(replace_ctx.string_size), NULL, &input_str_uint_buffer_p[0], &input_flags);
  JJS_ASSERT ((input_flags & ECMA_STRING_FLAG_MUST_BE_FREED) == 0);

  search_buf_p = ecma_string_get_chars (context_p, search_str_p, &search_size, &search_length, &search_str_uint_buffer_p[0], &search_flags);
  JJS_ASSERT ((search_flags & ECMA_STRING_FLAG_MUST_BE_FREED) == 0);

  ecma_string_t *result_string_p = NULL;

  if (replace_ctx.string_size >= search_size)
  {
    replace_ctx.builder = ecma_stringbuilder_create (context_p);
    replace_ctx.matched_size = search_size;
    const lit_utf8_byte_t *const input_end_p = replace_ctx.string_p + replace_ctx.string_size;
    const lit_utf8_byte_t *const loop_end_p = input_end_p - search_size;
    const lit_utf8_byte_t *last_match_end_p = replace_ctx.string_p;
    const lit_utf8_byte_t *curr_p = replace_ctx.string_p;

    lit_utf8_size_t pos = 0;
    while (curr_p <= loop_end_p)
    {
      if (!memcmp (curr_p, search_buf_p, search_size))
      {
        const lit_utf8_size_t prefix_size = (lit_utf8_size_t) (curr_p - last_match_end_p);
        ecma_stringbuilder_append_raw (&replace_ctx.builder, last_match_end_p, prefix_size);

        last_match_end_p = curr_p + search_size;

        if (replace_ctx.replace_str_p == NULL)
        {
          ecma_object_t *function_p = ecma_get_object_from_value (context_p, replace_value);

          ecma_value_t args[] = { ecma_make_string_value (context_p, search_str_p),
                                  ecma_make_uint32_value (context_p, pos),
                                  ecma_make_string_value (context_p, input_str_p) };

          result = ecma_op_function_call (context_p, function_p, ECMA_VALUE_UNDEFINED, args, 3);

          if (ECMA_IS_VALUE_ERROR (result))
          {
            ecma_stringbuilder_destroy (&replace_ctx.builder);
            goto cleanup_replace;
          }

          ecma_string_t *const result_str_p = ecma_op_to_string (context_p, result);
          ecma_free_value (context_p, result);

          if (result_str_p == NULL)
          {
            ecma_stringbuilder_destroy (&replace_ctx.builder);
            result = ECMA_VALUE_ERROR;
            goto cleanup_replace;
          }

          ecma_stringbuilder_append (&replace_ctx.builder, result_str_p);
          ecma_deref_ecma_string (context_p, result_str_p);
        }
        else
        {
          replace_ctx.matched_p = curr_p;
          replace_ctx.match_byte_pos = (lit_utf8_size_t) (curr_p - replace_ctx.string_p);

          ecma_builtin_replace_substitute (context_p, &replace_ctx);
        }

        if (!replace_all || last_match_end_p == input_end_p)
        {
          break;
        }

        if (search_size != 0)
        {
          curr_p = last_match_end_p;
          pos += search_length;
          continue;
        }
      }

      pos++;
      lit_utf8_incr (&curr_p);
    }

    ecma_stringbuilder_append_raw (&replace_ctx.builder,
                                   last_match_end_p,
                                   (lit_utf8_size_t) (input_end_p - last_match_end_p));
    result_string_p = ecma_stringbuilder_finalize (&replace_ctx.builder);
  }

  if (result_string_p == NULL)
  {
    ecma_ref_ecma_string (input_str_p);
    result_string_p = input_str_p;
  }

  result = ecma_make_string_value (context_p, result_string_p);

cleanup_replace:
  if (replace_ctx.replace_str_p != NULL)
  {
    ecma_deref_ecma_string (context_p, replace_ctx.replace_str_p);
  }

cleanup_search:
  ecma_deref_ecma_string (context_p, search_str_p);
  ecma_deref_ecma_string (context_p, input_str_p);

  return result;
} /* ecma_builtin_string_prototype_object_replace_helper */

/**
 * The String.prototype object's 'search' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.12
 *          ECMA-262 v6, 21.1.3.15
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_search (ecma_context_t *context_p, /**< JJS context */
                                             ecma_value_t this_value, /**< this argument */
                                             ecma_value_t regexp_value) /**< routine's argument */
{
  if (!(ecma_is_value_undefined (regexp_value) || ecma_is_value_null (regexp_value)))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (context_p, ecma_op_to_object (context_p, regexp_value));
    ecma_value_t search_symbol = ecma_op_object_get_by_symbol_id (context_p, obj_p, LIT_GLOBAL_SYMBOL_SEARCH);
    ecma_deref_object (obj_p);

    if (ECMA_IS_VALUE_ERROR (search_symbol))
    {
      return search_symbol;
    }

    if (!ecma_is_value_undefined (search_symbol) && !ecma_is_value_null (search_symbol))
    {
      ecma_value_t search_result = ecma_op_function_validated_call (context_p, search_symbol, regexp_value, &this_value, 1);
      ecma_free_value (context_p, search_symbol);
      return search_result;
    }
  }

  ecma_value_t result = ECMA_VALUE_ERROR;

  ecma_string_t *string_p = ecma_op_to_string (context_p, this_value);
  if (string_p == NULL)
  {
    return result;
  }

  ecma_string_t *pattern_p = ecma_regexp_read_pattern_str_helper (context_p, regexp_value);
  if (pattern_p == NULL)
  {
    ecma_deref_ecma_string (context_p, string_p);
    return result;
  }

  ecma_object_t *new_regexp_obj_p = ecma_op_regexp_alloc (context_p, NULL);

  if (JJS_UNLIKELY (new_regexp_obj_p == NULL))
  {
    ecma_deref_ecma_string (context_p, string_p);
    ecma_deref_ecma_string (context_p, pattern_p);
    return result;
  }

  ecma_value_t new_regexp =
    ecma_op_create_regexp_from_pattern (context_p, new_regexp_obj_p, ecma_make_string_value (context_p, pattern_p), ECMA_VALUE_UNDEFINED);

  ecma_deref_ecma_string (context_p, pattern_p);

  if (ECMA_IS_VALUE_ERROR (new_regexp))
  {
    ecma_deref_ecma_string (context_p, string_p);
    ecma_deref_object (new_regexp_obj_p);
    return result;
  }

  ecma_object_t *regexp_obj_p = ecma_get_object_from_value (context_p, new_regexp);
  ecma_value_t this_str_value = ecma_make_string_value (context_p, string_p);
  result = ecma_op_invoke_by_symbol_id (context_p, new_regexp, LIT_GLOBAL_SYMBOL_SEARCH, &this_str_value, 1);
  ecma_deref_object (regexp_obj_p);
  ecma_deref_ecma_string (context_p, string_p);

  return result;
} /* ecma_builtin_string_prototype_object_search */

#endif /* JJS_BUILTIN_REGEXP */

/**
 * The String.prototype object's 'slice' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.13
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_slice (ecma_context_t *context_p, /**< JJS context */
                                            ecma_string_t *get_string_val, /**< this argument */
                                            ecma_value_t arg1, /**< routine's first argument */
                                            ecma_value_t arg2) /**< routine's second argument */
{
  const lit_utf8_size_t len = ecma_string_get_length (context_p, get_string_val);

  /* 4. 6. */
  lit_utf8_size_t start = 0, end = len;

  if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, arg1, len, &start)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 5. 7. */
  if (ecma_is_value_undefined (arg2))
  {
    end = len;
  }
  else
  {
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_uint32_index_normalize (context_p, arg2, len, &end)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  JJS_ASSERT (start <= len && end <= len);

  /* 8-9. */
  ecma_string_t *new_str_p = ecma_string_substr (context_p, get_string_val, start, end);

  return ecma_make_string_value (context_p, new_str_p);
} /* ecma_builtin_string_prototype_object_slice */

/**
 * The String.prototype object's 'at' routine
 *
 * See also:
 *          ECMA-262 Stage 3 Draft Relative Indexing Method proposal
 *          from: https://tc39.es/proposal-relative-indexing-method
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_at (ecma_context_t *context_p, /**< JJS context */
                                         ecma_string_t *string_val, /**< this argument */
                                         const ecma_value_t index) /**< index argument */
{
  ecma_length_t len = (ecma_length_t) ecma_string_get_length (context_p, string_val);
  ecma_length_t res_index;
  ecma_value_t return_value = ecma_builtin_helper_calculate_index (context_p, index, len, &res_index);

  if (return_value != ECMA_VALUE_EMPTY)
  {
    return return_value;
  }

  ecma_char_t character = ecma_string_get_char_at_pos (context_p, string_val, (lit_utf8_size_t) res_index);

  return ecma_make_string_value (context_p, ecma_new_ecma_string_from_code_unit (context_p, character));
} /* ecma_builtin_string_prototype_object_at */

/**
 * The String.prototype object's 'split' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.14
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_split (ecma_context_t *context_p, /**< JJS context */
                                            ecma_value_t this_value, /**< this argument */
                                            ecma_value_t separator_value, /**< separator */
                                            ecma_value_t limit_value) /**< limit */
{
  if (!(ecma_is_value_undefined (separator_value) || ecma_is_value_null (separator_value)))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (context_p, ecma_op_to_object (context_p, separator_value));
    ecma_value_t split_symbol = ecma_op_object_get_by_symbol_id (context_p, obj_p, LIT_GLOBAL_SYMBOL_SPLIT);
    ecma_deref_object (obj_p);

    if (ECMA_IS_VALUE_ERROR (split_symbol))
    {
      return split_symbol;
    }

    if (!ecma_is_value_undefined (split_symbol) && !ecma_is_value_null (split_symbol))
    {
      ecma_value_t arguments[] = { this_value, limit_value };
      ecma_value_t split_result = ecma_op_function_validated_call (context_p, split_symbol, separator_value, arguments, 2);
      ecma_free_value (context_p, split_symbol);

      return split_result;
    }
  }

  ecma_value_t result = ECMA_VALUE_ERROR;

  /* 4. */
  ecma_string_t *string_p = ecma_op_to_string (context_p, this_value);
  if (string_p == NULL)
  {
    return result;
  }

  /* 8. */
  uint32_t limit = UINT32_MAX - 1;

  if (!ecma_is_value_undefined (limit_value))
  {
    /* ECMA-262 v11, 21.1.3.20 6 */
    ecma_number_t num;
    if (ECMA_IS_VALUE_ERROR (ecma_op_to_number (context_p, limit_value, &num)))
    {
      goto cleanup_string;
    }
    limit = ecma_number_to_uint32 (num);
  }

  /* 12. */
  ecma_string_t *separator_p = ecma_op_to_string (context_p, separator_value);
  if (separator_p == NULL)
  {
    goto cleanup_string;
  }

  /* 6. */
  ecma_object_t *array_p = ecma_op_new_array_object (context_p, 0);
  result = ecma_make_object_value (context_p, array_p);

  /* 14. */
  if (limit == 0)
  {
    goto cleanup_separator;
  }

  /* 6. */
  lit_utf8_size_t array_length = 0;

  /* 15. */
  if (ecma_is_value_undefined (separator_value))
  {
    ecma_value_t put_result = ecma_builtin_helper_def_prop_by_index (context_p,
                                                                     array_p,
                                                                     array_length,
                                                                     ecma_make_string_value (context_p, string_p),
                                                                     ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
    JJS_ASSERT (put_result == ECMA_VALUE_TRUE);
    goto cleanup_separator;
  }

  /* 16. */
  if (ecma_string_is_empty (string_p))
  {
    if (!ecma_string_is_empty (separator_p))
    {
      ecma_value_t put_result = ecma_builtin_helper_def_prop_by_index (context_p,
                                                                       array_p,
                                                                       array_length,
                                                                       ecma_make_string_value (context_p, string_p),
                                                                       ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
      JJS_ASSERT (put_result == ECMA_VALUE_TRUE);
    }

    goto cleanup_separator;
  }

  lit_utf8_size_t string_size;
  uint8_t string_flags = ECMA_STRING_FLAG_IS_ASCII;
  lit_utf8_byte_t string_uint_buffer[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
  const lit_utf8_byte_t *string_buffer_p = ecma_string_get_chars (context_p, string_p, &string_size, NULL, &string_uint_buffer[0], &string_flags);
  lit_utf8_size_t separator_size;
  uint8_t separator_flags = ECMA_STRING_FLAG_IS_ASCII;
  lit_utf8_byte_t separator_uint_buffer[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
  const lit_utf8_byte_t *separator_buffer_p =
    ecma_string_get_chars (context_p, separator_p, &separator_size, NULL, &separator_uint_buffer[0], &separator_flags);

  const lit_utf8_byte_t *const string_end_p = string_buffer_p + string_size;
  const lit_utf8_byte_t *const compare_end_p = JJS_MIN (string_end_p - separator_size + 1, string_end_p);
  const lit_utf8_byte_t *current_p = string_buffer_p;
  const lit_utf8_byte_t *last_str_begin_p = string_buffer_p;

  JJS_ASSERT ((string_flags & ECMA_STRING_FLAG_MUST_BE_FREED) == 0);
  JJS_ASSERT ((separator_flags & ECMA_STRING_FLAG_MUST_BE_FREED) == 0);

  while (current_p < compare_end_p)
  {
    if (!memcmp (current_p, separator_buffer_p, separator_size) && (last_str_begin_p != current_p + separator_size))
    {
      ecma_string_t *substr_p =
        ecma_new_ecma_string_from_utf8 (context_p, last_str_begin_p, (lit_utf8_size_t) (current_p - last_str_begin_p));
      ecma_value_t put_result = ecma_builtin_helper_def_prop_by_index (context_p,
                                                                       array_p,
                                                                       array_length++,
                                                                       ecma_make_string_value (context_p, substr_p),
                                                                       ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
      JJS_ASSERT (put_result == ECMA_VALUE_TRUE);
      ecma_deref_ecma_string (context_p, substr_p);

      if (array_length >= limit)
      {
        goto cleanup_separator;
      }

      current_p += separator_size;
      last_str_begin_p = current_p;
      continue;
    }

    lit_utf8_incr (&current_p);
  }

  ecma_string_t *end_substr_p =
    ecma_new_ecma_string_from_utf8 (context_p, last_str_begin_p, (lit_utf8_size_t) (string_end_p - last_str_begin_p));
  ecma_value_t put_result = ecma_builtin_helper_def_prop_by_index (context_p,
                                                                   array_p,
                                                                   array_length,
                                                                   ecma_make_string_value (context_p, end_substr_p),
                                                                   ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
  JJS_ASSERT (put_result == ECMA_VALUE_TRUE);
  ecma_deref_ecma_string (context_p, end_substr_p);

cleanup_separator:
  ecma_deref_ecma_string (context_p, separator_p);
cleanup_string:
  ecma_deref_ecma_string (context_p, string_p);
  return result;
} /* ecma_builtin_string_prototype_object_split */

/**
 * The String.prototype object's 'substring' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.15
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_substring (ecma_context_t *context_p, /**< JJS context */
                                                ecma_string_t *original_string_p, /**< this argument */
                                                ecma_value_t arg1, /**< routine's first argument */
                                                ecma_value_t arg2) /**< routine's second argument */
{
  /* 3 */
  const lit_utf8_size_t len = ecma_string_get_length (context_p, original_string_p);
  lit_utf8_size_t start = 0, end = len;

  /* 4 */
  ecma_number_t start_num;

  if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, arg1, &start_num)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 6 */
  start = (uint32_t) JJS_MIN (JJS_MAX (start_num, 0), len);

  /* 5 */
  if (ecma_is_value_undefined (arg2))
  {
    end = len;
  }
  else
  {
    /* 5 part 2 */
    ecma_number_t end_num;

    if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, arg2, &end_num)))
    {
      return ECMA_VALUE_ERROR;
    }
    /* 7 */
    end = (uint32_t) JJS_MIN (JJS_MAX (end_num, 0), len);
  }

  JJS_ASSERT (start <= len && end <= len);

  /* 8 */
  uint32_t from = start < end ? start : end;

  /* 9 */
  uint32_t to = start > end ? start : end;

  /* 10 */
  ecma_string_t *new_str_p = ecma_string_substr (context_p, original_string_p, from, to);
  return ecma_make_string_value (context_p, new_str_p);
} /* ecma_builtin_string_prototype_object_substring */

/**
 * The common implementation of the String.prototype object's
 * 'toLowerCase', 'toLocaleLowerCase', 'toUpperCase', 'toLocalUpperCase' routines
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.16
 *          ECMA-262 v5, 15.5.4.17
 *          ECMA-262 v5, 15.5.4.18
 *          ECMA-262 v5, 15.5.4.19
 *
 * Helper function to convert a string to upper or lower case.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_conversion_helper (ecma_context_t *context_p, /**< JJS context */
                                                        ecma_string_t *input_string_p, /**< this argument */
                                                        bool lower_case) /**< convert to lower (true)
                                                                          *   or upper (false) case */
{
  ecma_stringbuilder_t builder = ecma_stringbuilder_create (context_p);

  ECMA_STRING_TO_UTF8_STRING (context_p, input_string_p, input_start_p, input_start_size);

  const lit_utf8_byte_t *input_curr_p = input_start_p;
  const lit_utf8_byte_t *input_str_end_p = input_start_p + input_start_size;

  while (input_curr_p < input_str_end_p)
  {
    lit_code_point_t cp = lit_cesu8_read_next (&input_curr_p);

    if (lit_is_code_point_utf16_high_surrogate (cp) && input_curr_p < input_str_end_p)
    {
      const ecma_char_t next_ch = lit_cesu8_peek_next (input_curr_p);
      if (lit_is_code_point_utf16_low_surrogate (next_ch))
      {
        cp = lit_convert_surrogate_pair_to_code_point ((ecma_char_t) cp, next_ch);
        input_curr_p += LIT_UTF8_MAX_BYTES_IN_CODE_UNIT;
      }
    }

    if (lower_case)
    {
      lit_char_to_lower_case (cp, &builder);
    }
    else
    {
      lit_char_to_upper_case (cp, &builder);
    }
  }

  ECMA_FINALIZE_UTF8_STRING (context_p, input_start_p, input_start_size);

  return ecma_make_string_value (context_p, ecma_stringbuilder_finalize (&builder));
} /* ecma_builtin_string_prototype_object_conversion_helper */

/**
 * The String.prototype object's 'trim' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.20
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_trim (ecma_context_t *context_p, /**< JJS context */
                                           ecma_string_t *original_string_p) /**< this argument */
{
  ecma_string_t *trimmed_string_p = ecma_string_trim (context_p, original_string_p);

  return ecma_make_string_value (context_p, trimmed_string_p);
} /* ecma_builtin_string_prototype_object_trim */

/**
 * The String.prototype object's 'repeat' routine
 *
 * See also:
 *          ECMA-262 v6, 21.1.3.13
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_repeat (ecma_context_t *context_p, /**< JJS context */
                                             ecma_string_t *original_string_p, /**< this argument */
                                             ecma_value_t repeat) /**< times to repeat */
{
  ecma_string_t *ret_string_p;

  /* 4 */
  ecma_number_t count_number;
  ecma_value_t count_value = ecma_op_to_integer (context_p, repeat, &count_number);

  /* 5 */
  if (ECMA_IS_VALUE_ERROR (count_value))
  {
    return count_value;
  }

  int32_t repeat_count = ecma_number_to_int32 (count_number);

  bool isNan = ecma_number_is_nan (count_number);

  /* 6, 7 */
  if (count_number < 0 || (!isNan && ecma_number_is_infinity (count_number)))
  {
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_COUNT_VALUE);
  }

  lit_utf8_size_t size = ecma_string_get_size (context_p, original_string_p);

  if (repeat_count == 0 || size == 0 || isNan)
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  if ((uint32_t) repeat_count >= (ECMA_STRING_SIZE_LIMIT / size))
  {
    return ecma_raise_range_error (context_p, ECMA_ERR_INVALID_STRING_);
  }

  lit_utf8_size_t total_size = size * (lit_utf8_size_t) repeat_count;

  JMEM_DEFINE_LOCAL_ARRAY (context_p, str_buffer, total_size, lit_utf8_byte_t);

  ecma_string_to_cesu8_bytes (context_p, original_string_p, str_buffer, size);
  lit_utf8_byte_t *buffer_ptr = str_buffer + size;

  for (int32_t n = 1; n < repeat_count; n++)
  {
    memcpy (buffer_ptr, str_buffer, size);
    buffer_ptr += size;
  }

  ret_string_p = ecma_new_ecma_string_from_utf8 (context_p, str_buffer, (lit_utf8_size_t) (buffer_ptr - str_buffer));
  JMEM_FINALIZE_LOCAL_ARRAY (context_p, str_buffer);

  return ecma_make_string_value (context_p, ret_string_p);
} /* ecma_builtin_string_prototype_object_repeat */

/**
 * The String.prototype object's 'codePointAt' routine
 *
 * See also:
 *          ECMA-262 v6, 21.1.3.3
 *
 * @return lit_code_point_t
 */
static ecma_value_t
ecma_builtin_string_prototype_object_code_point_at (ecma_context_t *context_p, /**< JJS context */
                                                    ecma_string_t *this_string_p, /**< this argument */
                                                    ecma_value_t pos) /**< given position */
{
  ecma_number_t pos_num;
  ecma_value_t error = ecma_op_to_integer (context_p, pos, &pos_num);

  if (ECMA_IS_VALUE_ERROR (error))
  {
    return error;
  }

  lit_utf8_size_t length = ecma_string_get_length (context_p, this_string_p);

  if (pos_num < 0 || pos_num >= length)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  uint32_t index = (uint32_t) pos_num;

  ecma_char_t first = ecma_string_get_char_at_pos (context_p, this_string_p, index);

  if (first < LIT_UTF16_HIGH_SURROGATE_MIN || first > LIT_UTF16_HIGH_SURROGATE_MAX || index + 1 == length)
  {
    return ecma_make_uint32_value (context_p, first);
  }

  ecma_char_t second = ecma_string_get_char_at_pos (context_p, this_string_p, index + 1);

  if (second < LIT_UTF16_LOW_SURROGATE_MARKER || second > LIT_UTF16_LOW_SURROGATE_MAX)
  {
    return ecma_make_uint32_value (context_p, first);
  }

  return ecma_make_uint32_value (context_p, lit_convert_surrogate_pair_to_code_point (first, second));
} /* ecma_builtin_string_prototype_object_code_point_at */

#if JJS_BUILTIN_ANNEXB

/**
 * The String.prototype object's 'substr' routine
 *
 * See also:
 *          ECMA-262 v5, B.2.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_substr (ecma_context_t *context_p, /**< JJS context */
                                             ecma_string_t *this_string_p, /**< this argument */
                                             ecma_value_t start, /**< routine's first argument */
                                             ecma_value_t length) /**< routine's second argument */
{
  /* 2. */
  ecma_number_t start_num;

  if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, start, &start_num)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 3. */
  ecma_number_t length_num = ecma_number_make_infinity (false);

  if (!ecma_is_value_undefined (length))
  {
    ecma_number_t len;

    if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (context_p, length, &len)))
    {
      return ECMA_VALUE_ERROR;
    }

    length_num = ecma_number_is_nan (len) ? 0 : len;
  }

  /* 4. */
  lit_utf8_size_t this_len = ecma_string_get_length (context_p, this_string_p);

  /* 5. */
  uint32_t from = (uint32_t) ((start_num < 0) ? JJS_MAX (this_len + start_num, 0) : start_num);

  if (from > this_len)
  {
    from = this_len;
  }

  /* 6. */
  ecma_number_t to_num = JJS_MIN (JJS_MAX (length_num, 0), this_len - from);

  /* 7. */
  uint32_t to = from + (uint32_t) to_num;

  /* 8. */
  ecma_string_t *new_str_p = ecma_string_substr (context_p, this_string_p, from, to);
  return ecma_make_string_value (context_p, new_str_p);
} /* ecma_builtin_string_prototype_object_substr */

#endif /* JJS_BUILTIN_ANNEXB */

/**
 * The String.prototype object's @@iterator routine
 *
 * See also:
 *          ECMA-262 v6, 21.1.3.27
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_string_prototype_object_iterator (ecma_context_t *context_p, /**< JJS context */
                                               ecma_value_t to_string) /**< this argument */
{
  return ecma_op_create_iterator_object (context_p,
                                         ecma_copy_value (context_p, to_string),
                                         ecma_builtin_get (context_p, ECMA_BUILTIN_ID_STRING_ITERATOR_PROTOTYPE),
                                         ECMA_OBJECT_CLASS_STRING_ITERATOR,
                                         ECMA_ITERATOR_VALUES);
} /* ecma_builtin_string_prototype_object_iterator */

/**
 * See also:
 *   ECMA (2025) v16 22.1.3.31 String.prototype.toWellFormed ()
 *
 * @param string_p
 * @return
 */
static ecma_value_t
ecma_builtin_string_prototype_object_to_well_formed (ecma_context_t *context_p, /**< JJS context */
                                                     ecma_string_t *string_p)
{
  if (ecma_string_is_empty (string_p))
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_stringbuilder_t out = ecma_stringbuilder_create (context_p);

  // note: path_bytes_p is a CESU8 encoded, despite the macro indicating otherwise
  ECMA_STRING_TO_UTF8_STRING (context_p, string_p, string_bytes_p, string_bytes_len);

  const lit_utf8_byte_t *string_cursor_p = string_bytes_p;
  const lit_utf8_byte_t *string_end_p = string_bytes_p + string_bytes_len;
  lit_utf8_size_t read_size;
  bool has_error = false;
  ecma_char_t ch;
  ecma_char_t next_ch;
  lit_code_point_t code_point;

  while (string_cursor_p < string_end_p)
  {
    read_size = lit_read_code_unit_from_cesu8_safe (string_cursor_p, string_end_p, &ch);

    if (read_size == 0)
    {
      has_error = true;
      break;
    }

    string_cursor_p += read_size;

    if (lit_is_code_point_utf16_low_surrogate (ch))
    {
      ecma_stringbuilder_append_codepoint (&out, 0xFFFD);
      continue;
    }

    code_point = ch;

    if (lit_is_code_point_utf16_high_surrogate (ch))
    {
      if (string_cursor_p == string_end_p)
      {
        ecma_stringbuilder_append_codepoint (&out, 0xFFFD);
        break;
      }

      read_size = lit_read_code_unit_from_cesu8_safe (string_cursor_p, string_end_p, &next_ch);

      if (read_size == 0)
      {
        has_error = true;
        break;
      }

      if (lit_is_code_point_utf16_low_surrogate (next_ch))
      {
        code_point = lit_convert_surrogate_pair_to_code_point (ch, next_ch);
        string_cursor_p += read_size;
      }
      else
      {
        code_point = 0xFFFD;
      }
    }

    ecma_stringbuilder_append_codepoint (&out, code_point);
  }

  ECMA_FINALIZE_UTF8_STRING (context_p, string_bytes_p, string_bytes_len);

  JJS_ASSERT (!has_error);
  if (has_error)
  {
    ecma_stringbuilder_destroy (&out);
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  return ecma_make_string_value (context_p, ecma_stringbuilder_finalize (&out));
} /* ecma_builtin_string_prototype_object_to_well_formed */

/**
 *
 * See also:
 *   ECMA (2025) v16 22.1.3.10 String.prototype.isWellFormed ()
 * @param string_p
 * @return
 */
static ecma_value_t
ecma_builtin_string_prototype_object_is_well_formed (ecma_context_t *context_p, /**< JJS context */
                                                     ecma_string_t *string_p)
{
  if (ecma_string_is_empty (string_p))
  {
    return ECMA_VALUE_TRUE;
  }

  // note: path_bytes_p is a CESU8 encoded, despite the macro indicating otherwise
  ECMA_STRING_TO_UTF8_STRING (context_p, string_p, string_bytes_p, string_bytes_len);

  const lit_utf8_byte_t *string_cursor_p = string_bytes_p;
  const lit_utf8_byte_t *string_end_p = string_bytes_p + string_bytes_len;
  lit_utf8_size_t read_size;
  bool has_error = false;
  ecma_char_t ch;
  ecma_char_t next_ch;

  while (string_cursor_p < string_end_p)
  {
    read_size = lit_read_code_unit_from_cesu8_safe (string_cursor_p, string_end_p, &ch);

    if (read_size == 0)
    {
      has_error = true;
      break;
    }

    string_cursor_p += read_size;

    if (lit_is_code_point_utf16_low_surrogate (ch))
    {
      has_error = true;
      break;
    }

    if (lit_is_code_point_utf16_high_surrogate (ch))
    {
      if (string_cursor_p == string_end_p)
      {
        has_error = true;
        break;
      }

      read_size = lit_read_code_unit_from_cesu8_safe (string_cursor_p, string_end_p, &next_ch);

      if (read_size == 0)
      {
        has_error = true;
        break;
      }

      if (lit_is_code_point_utf16_low_surrogate (next_ch))
      {
        string_cursor_p += read_size;
      }
      else
      {
        has_error = true;
        break;
      }
    }
  }

  ECMA_FINALIZE_UTF8_STRING (context_p, string_bytes_p, string_bytes_len);

  return has_error ? ECMA_VALUE_FALSE : ECMA_VALUE_TRUE;
} /* ecma_builtin_string_prototype_object_is_well_formed */

/**
 * Dispatcher of the built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_string_prototype_dispatch_routine (ecma_context_t *context_p, /**< JJS context */
                                                uint8_t builtin_routine_id, /**< built-in wide routine
                                                                             *   identifier */
                                                ecma_value_t this_arg, /**< 'this' argument value */
                                                const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                                        *   passed to routine */
                                                uint32_t arguments_number) /**< length of arguments' list */
{
  if (builtin_routine_id <= ECMA_STRING_PROTOTYPE_VALUE_OF)
  {
    return ecma_builtin_string_prototype_object_to_string (context_p, this_arg);
  }

  if (!ecma_op_require_object_coercible (context_p, this_arg))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t arg1 = arguments_list_p[0];
  ecma_value_t arg2 = arguments_list_p[1];

#if JJS_BUILTIN_REGEXP
  if (builtin_routine_id == ECMA_STRING_PROTOTYPE_MATCH)
  {
    return ecma_builtin_string_prototype_object_match (context_p, this_arg, arg1);
  }

  if (builtin_routine_id == ECMA_STRING_PROTOTYPE_MATCH_ALL)
  {
    return ecma_builtin_string_prototype_object_match_all (context_p, this_arg, arg1);
  }
#endif /* JJS_BUILTIN_REGEXP */

  if (builtin_routine_id <= ECMA_STRING_PROTOTYPE_CHAR_CODE_AT)
  {
    return ecma_builtin_string_prototype_char_at_helper (context_p,
                                                         this_arg,
                                                         arg1,
                                                         builtin_routine_id == ECMA_STRING_PROTOTYPE_CHAR_CODE_AT);
  }

#if JJS_BUILTIN_REGEXP
  if (builtin_routine_id == ECMA_STRING_PROTOTYPE_REPLACE)
  {
    return ecma_builtin_string_prototype_object_replace_helper (context_p, this_arg, arg1, arg2, false);
  }

  else if (builtin_routine_id == ECMA_STRING_PROTOTYPE_REPLACE_ALL)
  {
    return ecma_builtin_string_prototype_object_replace_helper (context_p, this_arg, arg1, arg2, true);
  }
#endif /* JJS_BUILTIN_REGEXP */

  ecma_string_t *string_p = ecma_op_to_string (context_p, this_arg);

  if (JJS_UNLIKELY (string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t to_string_val = ecma_make_string_value (context_p, string_p);
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  switch (builtin_routine_id)
  {
    case ECMA_STRING_PROTOTYPE_IS_WELL_FORMED:
      ret_value = ecma_builtin_string_prototype_object_is_well_formed (context_p, string_p);
      break;
    case ECMA_STRING_PROTOTYPE_TO_WELL_FORMED:
      ret_value = ecma_builtin_string_prototype_object_to_well_formed (context_p, string_p);
      break;
    case ECMA_STRING_PROTOTYPE_CONCAT:
    {
      ret_value = ecma_builtin_string_prototype_object_concat (context_p, string_p, arguments_list_p, arguments_number);
      break;
    }
    case ECMA_STRING_PROTOTYPE_SLICE:
    {
      ret_value = ecma_builtin_string_prototype_object_slice (context_p, string_p, arg1, arg2);
      break;
    }
    case ECMA_STRING_PROTOTYPE_AT:
    {
      ret_value = ecma_builtin_string_prototype_object_at (context_p, string_p, arg1);
      break;
    }
    case ECMA_STRING_PROTOTYPE_LAST_INDEX_OF:
    case ECMA_STRING_PROTOTYPE_INDEX_OF:
    case ECMA_STRING_PROTOTYPE_STARTS_WITH:
    case ECMA_STRING_PROTOTYPE_INCLUDES:
    case ECMA_STRING_PROTOTYPE_ENDS_WITH:
    {
      ecma_string_index_of_mode_t mode;
      mode = (ecma_string_index_of_mode_t) (builtin_routine_id - ECMA_STRING_PROTOTYPE_LAST_INDEX_OF);
      ret_value = ecma_builtin_helper_string_prototype_object_index_of (context_p, string_p, arg1, arg2, mode);
      break;
    }
    case ECMA_STRING_PROTOTYPE_LOCALE_COMPARE:
    {
      ret_value = ecma_builtin_string_prototype_object_locale_compare (context_p, string_p, arg1);
      break;
    }
#if JJS_BUILTIN_REGEXP
    case ECMA_STRING_PROTOTYPE_SEARCH:
    {
      ret_value = ecma_builtin_string_prototype_object_search (context_p, to_string_val, arg1);
      break;
    }
#endif /* JJS_BUILTIN_REGEXP */
    case ECMA_STRING_PROTOTYPE_SPLIT:
    {
      ret_value = ecma_builtin_string_prototype_object_split (context_p, to_string_val, arg1, arg2);
      break;
    }
    case ECMA_STRING_PROTOTYPE_SUBSTRING:
    {
      ret_value = ecma_builtin_string_prototype_object_substring (context_p, string_p, arg1, arg2);
      break;
    }
    case ECMA_STRING_PROTOTYPE_TO_LOWER_CASE:
    case ECMA_STRING_PROTOTYPE_TO_LOCAL_LOWER_CASE:
    case ECMA_STRING_PROTOTYPE_TO_UPPER_CASE:
    case ECMA_STRING_PROTOTYPE_TO_LOCAL_UPPER_CASE:
    {
      bool is_lower_case = builtin_routine_id <= ECMA_STRING_PROTOTYPE_TO_LOCAL_LOWER_CASE;
      ret_value = ecma_builtin_string_prototype_object_conversion_helper (context_p, string_p, is_lower_case);
      break;
    }
    case ECMA_STRING_PROTOTYPE_TRIM:
    {
      ret_value = ecma_builtin_string_prototype_object_trim (context_p, string_p);
      break;
    }
#if JJS_BUILTIN_ANNEXB
    case ECMA_STRING_PROTOTYPE_SUBSTR:
    {
      ret_value = ecma_builtin_string_prototype_object_substr (context_p, string_p, arg1, arg2);
      break;
    }
#endif /* JJS_BUILTIN_ANNEXB */
    case ECMA_STRING_PROTOTYPE_REPEAT:
    {
      ret_value = ecma_builtin_string_prototype_object_repeat (context_p, string_p, arg1);
      break;
    }
    case ECMA_STRING_PROTOTYPE_CODE_POINT_AT:
    {
      ret_value = ecma_builtin_string_prototype_object_code_point_at (context_p, string_p, arg1);
      break;
    }
    case ECMA_STRING_PROTOTYPE_ITERATOR:
    {
      ret_value = ecma_builtin_string_prototype_object_iterator (context_p, to_string_val);
      break;
    }
    case ECMA_STRING_PROTOTYPE_PAD_END:
    case ECMA_STRING_PROTOTYPE_PAD_START:
    {
      ret_value = ecma_string_pad (context_p, to_string_val, arg1, arg2, builtin_routine_id == ECMA_STRING_PROTOTYPE_PAD_START);
      break;
    }
    default:
    {
      JJS_UNREACHABLE ();
    }
  }

  ecma_deref_ecma_string (context_p, string_p);

  return ret_value;
} /* ecma_builtin_string_prototype_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* JJS_BUILTIN_STRING */
