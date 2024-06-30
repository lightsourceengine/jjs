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
#include "ecma-bigint.h"
#include "ecma-function-object.h"
#include "ecma-helpers.h"
#include "ecma-literal-storage.h"

#include "jcontext.h"
#include "js-parser-internal.h"
#include "lit-char-helpers.h"

#if JJS_PARSER

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_lexer Lexer
 * @{
 */

JJS_STATIC_ASSERT (LEXER_NUMBER_BINARY > LEXER_NUMBER_OCTAL,
                     lexer_number_binary_must_be_greater_than_lexer_number_octal);

/**
 * Check whether the UTF-8 intermediate is an octet or not
 */
#define IS_UTF8_INTERMEDIATE_OCTET(byte) (((byte) &LIT_UTF8_EXTRA_BYTE_MASK) == LIT_UTF8_2_BYTE_CODE_POINT_MIN)

/**
 * Align column to the next tab position.
 *
 * @return aligned position
 */
static parser_line_counter_t
align_column_to_tab (parser_line_counter_t column) /**< current column */
{
  /* Tab aligns to zero column start position. */
  return (parser_line_counter_t) (((column + (8u - 1u)) & ~ECMA_STRING_CONTAINER_MASK) + 1u);
} /* align_column_to_tab */

/**
 * Parse hexadecimal character sequence
 *
 * @return character value or UINT32_MAX on error
 */
static lit_code_point_t
lexer_hex_to_code_point (const uint8_t *source_p, /**< current source position */
                         parser_line_counter_t length) /**< source length */
{
  lit_code_point_t result = 0;

  do
  {
    uint32_t byte = *source_p++;

    result <<= 4;

    if (byte >= LIT_CHAR_0 && byte <= LIT_CHAR_9)
    {
      result += byte - LIT_CHAR_0;
    }
    else
    {
      byte = LEXER_TO_ASCII_LOWERCASE (byte);
      if (byte >= LIT_CHAR_LOWERCASE_A && byte <= LIT_CHAR_LOWERCASE_F)
      {
        result += byte - (LIT_CHAR_LOWERCASE_A - 10);
      }
      else
      {
        return UINT32_MAX;
      }
    }
  } while (--length > 0);

  return result;
} /* lexer_hex_to_code_point */

/**
 * Parse hexadecimal character sequence enclosed in braces
 *
 * @return character value or UINT32_MAX on error
 */
static lit_code_point_t
lexer_hex_in_braces_to_code_point (const uint8_t *source_p, /**< current source position */
                                   const uint8_t *source_end_p, /**< source end */
                                   uint32_t *length_p) /**< [out] length of the sequence */
{
  lit_code_point_t result = 0;
  /* Four is the size of \u{} sequence. */
  uint32_t length = 4;

  JJS_ASSERT (source_p[-1] == LIT_CHAR_LEFT_BRACE);
  JJS_ASSERT (source_p < source_end_p);

  do
  {
    uint32_t byte = *source_p++;

    result <<= 4;

    if (byte >= LIT_CHAR_0 && byte <= LIT_CHAR_9)
    {
      result += byte - LIT_CHAR_0;
    }
    else
    {
      byte = LEXER_TO_ASCII_LOWERCASE (byte);
      if (byte >= LIT_CHAR_LOWERCASE_A && byte <= LIT_CHAR_LOWERCASE_F)
      {
        result += byte - (LIT_CHAR_LOWERCASE_A - 10);
      }
      else
      {
        return UINT32_MAX;
      }
    }

    if (result >= (LIT_UNICODE_CODE_POINT_MAX + 1) || source_p >= source_end_p)
    {
      return UINT32_MAX;
    }
    length++;
  } while (*source_p != LIT_CHAR_RIGHT_BRACE);

  *length_p = length;
  return result;
} /* lexer_hex_in_braces_to_code_point */

/**
 * Parse hexadecimal character sequence
 *
 * @return character value
 */
static lit_code_point_t
lexer_unchecked_hex_to_character (const uint8_t **source_p) /**< [in, out] current source position */
{
  lit_code_point_t result = 0;
  const uint8_t *char_p = *source_p;
  uint32_t length = (char_p[-1] == LIT_CHAR_LOWERCASE_U) ? 4 : 2;

  if (char_p[0] == LIT_CHAR_LEFT_BRACE)
  {
    length = 0;
    char_p++;
  }

  while (true)
  {
    uint32_t byte = *char_p++;

    result <<= 4;

    if (byte >= LIT_CHAR_0 && byte <= LIT_CHAR_9)
    {
      result += byte - LIT_CHAR_0;
    }
    else
    {
      JJS_ASSERT ((byte >= LIT_CHAR_LOWERCASE_A && byte <= LIT_CHAR_LOWERCASE_F)
                    || (byte >= LIT_CHAR_UPPERCASE_A && byte <= LIT_CHAR_UPPERCASE_F));

      result += LEXER_TO_ASCII_LOWERCASE (byte) - (LIT_CHAR_LOWERCASE_A - 10);
    }

    JJS_ASSERT (result <= LIT_UNICODE_CODE_POINT_MAX);

    if (length == 0)
    {
      if (*char_p != LIT_CHAR_RIGHT_BRACE)
      {
        continue;
      }

      *source_p = char_p + 1;
      return result;
    }

    if (--length == 0)
    {
      *source_p = char_p;
      return result;
    }
  }
} /* lexer_unchecked_hex_to_character */

/**
 * Skip space mode
 */
typedef enum
{
  LEXER_SKIP_SPACES, /**< skip spaces mode */
  LEXER_SKIP_SINGLE_LINE_COMMENT, /**< parse single line comment */
  LEXER_SKIP_MULTI_LINE_COMMENT, /**< parse multi line comment */
} skip_mode_t;

/**
 * Internal function to initialize the skip mode. Should not be called outside
 * of lexer_skip_* functions.
 */
static void
lexer_skip_spaces_mode (parser_context_t *context_p, /**< context */
                        skip_mode_t mode) /**< initial skip mode */
{
  const uint8_t *source_end_p = context_p->source_end_p;

  if (context_p->token.flags & LEXER_NO_SKIP_SPACES)
  {
    context_p->token.flags &= (uint8_t) ~LEXER_NO_SKIP_SPACES;
    return;
  }

  context_p->token.flags = 0;

  while (true)
  {
    if (context_p->source_p >= source_end_p)
    {
      if (mode == LEXER_SKIP_MULTI_LINE_COMMENT)
      {
        parser_raise_error (context_p, PARSER_ERR_UNTERMINATED_MULTILINE_COMMENT);
      }
      return;
    }

    switch (context_p->source_p[0])
    {
      case LIT_CHAR_CR:
      {
        if (context_p->source_p + 1 < source_end_p && context_p->source_p[1] == LIT_CHAR_LF)
        {
          context_p->source_p++;
        }
        /* FALLTHRU */
      }

      case LIT_CHAR_LF:
      {
        context_p->line++;
        context_p->column = 0;
        context_p->token.flags = LEXER_WAS_NEWLINE;

        if (mode == LEXER_SKIP_SINGLE_LINE_COMMENT)
        {
          mode = LEXER_SKIP_SPACES;
        }
        /* FALLTHRU */
      }

      case LIT_CHAR_VTAB:
      case LIT_CHAR_FF:
      case LIT_CHAR_SP:
      {
        context_p->source_p++;
        context_p->column++;
        continue;
      }

      case LIT_CHAR_TAB:
      {
        context_p->column = align_column_to_tab (context_p->column);
        context_p->source_p++;
        continue;
      }

      case LIT_CHAR_SLASH:
      {
        if (mode == LEXER_SKIP_SPACES && context_p->source_p + 1 < source_end_p)
        {
          if (context_p->source_p[1] == LIT_CHAR_SLASH)
          {
            mode = LEXER_SKIP_SINGLE_LINE_COMMENT;
          }
          else if (context_p->source_p[1] == LIT_CHAR_ASTERISK)
          {
            mode = LEXER_SKIP_MULTI_LINE_COMMENT;
            context_p->token.line = context_p->line;
            context_p->token.column = context_p->column;
          }

          if (mode != LEXER_SKIP_SPACES)
          {
            context_p->source_p += 2;
            PARSER_PLUS_EQUAL_LC (context_p->column, 2);
            continue;
          }
        }
        break;
      }

      case LIT_CHAR_ASTERISK:
      {
        if (mode == LEXER_SKIP_MULTI_LINE_COMMENT && context_p->source_p + 1 < source_end_p
            && context_p->source_p[1] == LIT_CHAR_SLASH)
        {
          mode = LEXER_SKIP_SPACES;
          context_p->source_p += 2;
          PARSER_PLUS_EQUAL_LC (context_p->column, 2);
          continue;
        }
        break;
      }

      case 0xc2:
      {
        if (context_p->source_p + 1 < source_end_p && context_p->source_p[1] == 0xa0)
        {
          /* Codepoint \u00A0 */
          context_p->source_p += 2;
          context_p->column++;
          continue;
        }
        break;
      }

      case LEXER_NEWLINE_LS_PS_BYTE_1:
      {
        JJS_ASSERT (context_p->source_p + 2 < source_end_p);
        if (LEXER_NEWLINE_LS_PS_BYTE_23 (context_p->source_p))
        {
          /* Codepoint \u2028 and \u2029 */
          context_p->source_p += 3;
          context_p->line++;
          context_p->column = 1;
          context_p->token.flags = LEXER_WAS_NEWLINE;

          if (mode == LEXER_SKIP_SINGLE_LINE_COMMENT)
          {
            mode = LEXER_SKIP_SPACES;
          }
          continue;
        }
        break;
      }

      case 0xef:
      {
        if (context_p->source_p + 2 < source_end_p && context_p->source_p[1] == 0xbb && context_p->source_p[2] == 0xbf)
        {
          /* Codepoint \uFEFF */
          context_p->source_p += 3;
          context_p->column++;
          continue;
        }
        break;
      }

      default:
      {
        break;
      }
    }

    if (mode == LEXER_SKIP_SPACES)
    {
      return;
    }

    context_p->source_p++;

    if (context_p->source_p < source_end_p && !IS_UTF8_INTERMEDIATE_OCTET (context_p->source_p[0]))
    {
      context_p->column++;
    }
  }
} /* lexer_skip_spaces_mode */

/**
 * Skip spaces.
 */
static void
lexer_skip_spaces (parser_context_t *context_p) /**< context */
{
  lexer_skip_spaces_mode (context_p, LEXER_SKIP_SPACES);
} /* lexer_skip_spaces */

/**
 * Checks the next token start character.
 *
 * @return LIT_INVALID_CP - if there is no more characters to read
 *         next byte - otherwise
 */
lit_code_point_t
lexer_peek_next_character (parser_context_t *parser_context_p) /**< context */
{
  if (!(parser_context_p->token.flags & LEXER_NO_SKIP_SPACES))
  {
    lexer_skip_spaces (parser_context_p);
    parser_context_p->token.flags = (uint8_t) (parser_context_p->token.flags | LEXER_NO_SKIP_SPACES);
  }

  if (parser_context_p->source_p < parser_context_p->source_end_p)
  {
    return parser_context_p->source_p[0];
  }

  return LIT_INVALID_CP;
} /* lexer_peek_next_character */

/**
 * Skip all the continuous empty statements.
 */
void
lexer_skip_empty_statements (parser_context_t *parser_context_p) /**< context */
{
  lexer_skip_spaces (parser_context_p);

  while (parser_context_p->source_p < parser_context_p->source_end_p && * parser_context_p->source_p == LIT_CHAR_SEMICOLON)
  {
    lexer_consume_next_character (parser_context_p);
    lexer_skip_spaces (parser_context_p);
  }

  parser_context_p->token.flags = (uint8_t) (parser_context_p->token.flags | LEXER_NO_SKIP_SPACES);
} /* lexer_skip_empty_statements */

/**
 * Checks whether the keyword has escape sequences.
 */
#define LEXER_CHECK_INVALID_KEYWORD(ident_start_p, buffer_p) \
  (JJS_UNLIKELY ((ident_start_p) == (buffer_p))            \
   && !(context_p->global_status_flags & ECMA_PARSE_INTERNAL_PRE_SCANNING))

/**
 * Keyword data.
 */
typedef struct
{
  const uint8_t *keyword_p; /**< keyword string */
  lexer_token_type_t type; /**< keyword token type */
} keyword_string_t;

/**
 * @{
 * Keyword defines
 */
#define LEXER_KEYWORD(name, type)    \
  {                                  \
    (const uint8_t *) (name), (type) \
  }
#define LEXER_KEYWORD_LIST_LENGTH(name) (const uint8_t) (sizeof ((name)) / sizeof ((name)[0]))
/** @} */

/**
 * Length of the shortest keyword.
 */
#define LEXER_KEYWORD_MIN_LENGTH 2

/**
 * Length of the longest keyword.
 */
#define LEXER_KEYWORD_MAX_LENGTH 10

/**
 * Keywords with 2 characters.
 */
static const keyword_string_t keywords_with_length_2[] = {
  LEXER_KEYWORD ("do", LEXER_KEYW_DO),
  LEXER_KEYWORD ("if", LEXER_KEYW_IF),
  LEXER_KEYWORD ("in", LEXER_KEYW_IN),
};

/**
 * Keywords with 3 characters.
 */
static const keyword_string_t keywords_with_length_3[] = {
  LEXER_KEYWORD ("for", LEXER_KEYW_FOR), LEXER_KEYWORD ("let", LEXER_KEYW_LET), LEXER_KEYWORD ("new", LEXER_KEYW_NEW),
  LEXER_KEYWORD ("try", LEXER_KEYW_TRY), LEXER_KEYWORD ("var", LEXER_KEYW_VAR),
};

/**
 * Keywords with 4 characters.
 */
static const keyword_string_t keywords_with_length_4[] = {
  LEXER_KEYWORD ("case", LEXER_KEYW_CASE), LEXER_KEYWORD ("else", LEXER_KEYW_ELSE),
  LEXER_KEYWORD ("enum", LEXER_KEYW_ENUM), LEXER_KEYWORD ("eval", LEXER_KEYW_EVAL),
  LEXER_KEYWORD ("meta", LEXER_KEYW_META), LEXER_KEYWORD ("null", LEXER_LIT_NULL),  
  LEXER_KEYWORD ("this", LEXER_KEYW_THIS), LEXER_KEYWORD ("true", LEXER_LIT_TRUE),  
  LEXER_KEYWORD ("void", LEXER_KEYW_VOID), LEXER_KEYWORD ("with", LEXER_KEYW_WITH),
};

/**
 * Keywords with 5 characters.
 */
static const keyword_string_t keywords_with_length_5[] = {
  LEXER_KEYWORD ("async", LEXER_KEYW_ASYNC), LEXER_KEYWORD ("await", LEXER_KEYW_AWAIT),
  LEXER_KEYWORD ("break", LEXER_KEYW_BREAK), LEXER_KEYWORD ("catch", LEXER_KEYW_CATCH),
  LEXER_KEYWORD ("class", LEXER_KEYW_CLASS), LEXER_KEYWORD ("const", LEXER_KEYW_CONST),
  LEXER_KEYWORD ("false", LEXER_LIT_FALSE),  LEXER_KEYWORD ("super", LEXER_KEYW_SUPER),
  LEXER_KEYWORD ("throw", LEXER_KEYW_THROW), LEXER_KEYWORD ("while", LEXER_KEYW_WHILE),
  LEXER_KEYWORD ("yield", LEXER_KEYW_YIELD),
};

/**
 * Keywords with 6 characters.
 */
static const keyword_string_t keywords_with_length_6[] = {
  LEXER_KEYWORD ("delete", LEXER_KEYW_DELETE), LEXER_KEYWORD ("export", LEXER_KEYW_EXPORT),
  LEXER_KEYWORD ("import", LEXER_KEYW_IMPORT), LEXER_KEYWORD ("public", LEXER_KEYW_PUBLIC),
  LEXER_KEYWORD ("return", LEXER_KEYW_RETURN), LEXER_KEYWORD ("static", LEXER_KEYW_STATIC),
  LEXER_KEYWORD ("switch", LEXER_KEYW_SWITCH), LEXER_KEYWORD ("typeof", LEXER_KEYW_TYPEOF),
};

/**
 * Keywords with 7 characters.
 */
static const keyword_string_t keywords_with_length_7[] = {
  LEXER_KEYWORD ("default", LEXER_KEYW_DEFAULT), LEXER_KEYWORD ("extends", LEXER_KEYW_EXTENDS),
  LEXER_KEYWORD ("finally", LEXER_KEYW_FINALLY), LEXER_KEYWORD ("package", LEXER_KEYW_PACKAGE),
  LEXER_KEYWORD ("private", LEXER_KEYW_PRIVATE),
};

/**
 * Keywords with 8 characters.
 */
static const keyword_string_t keywords_with_length_8[] = {
  LEXER_KEYWORD ("continue", LEXER_KEYW_CONTINUE),
  LEXER_KEYWORD ("debugger", LEXER_KEYW_DEBUGGER),
  LEXER_KEYWORD ("function", LEXER_KEYW_FUNCTION),
};

/**
 * Keywords with 9 characters.
 */
static const keyword_string_t keywords_with_length_9[] = {
  LEXER_KEYWORD ("arguments", LEXER_KEYW_ARGUMENTS),
  LEXER_KEYWORD ("interface", LEXER_KEYW_INTERFACE),
  LEXER_KEYWORD ("protected", LEXER_KEYW_PROTECTED),
};

/**
 * Keywords with 10 characters.
 */
static const keyword_string_t keywords_with_length_10[] = {
  LEXER_KEYWORD ("implements", LEXER_KEYW_IMPLEMENTS),
  LEXER_KEYWORD ("instanceof", LEXER_KEYW_INSTANCEOF),
};

#undef LEXER_KEYWORD
#undef LEXER_KEYWORD_LIST_LENGTH

JJS_STATIC_ASSERT (LEXER_FIRST_NON_RESERVED_KEYWORD < LEXER_FIRST_FUTURE_STRICT_RESERVED_WORD,
                     lexer_first_non_reserved_keyword_must_be_before_lexer_first_future_strict_reserved_word);

#define LEXER_KEYW_MATCH_START(CH) switch (CH) {

#define LEXER_KEYW_MATCH_END(RESULT) default: { (RESULT) = NULL; break; } } break;

#define LEXER_KEYW_MATCH_CASE(RESULT, STR, CH, KEYW_INDEX, KEYW_LEN) \
  case CH: { \
    (RESULT) = memcmp (STR, keywords_with_length_##KEYW_LEN[KEYW_INDEX].keyword_p, KEYW_LEN) == 0 ? &keywords_with_length_##KEYW_LEN[KEYW_INDEX] : NULL; \
    break; \
  }

#define LEXER_KEYW_MATCH_CASE_COLLISION(RESULT, STR, CH, A0, A0_KEYW_INDEX, B0, B0_KEYW_INDEX, KEYW_LEN) \
  case CH: {                                                                                             \
    if (*(STR) == (A0))                                                                                                     \
      (RESULT) = memcmp (STR, keywords_with_length_##KEYW_LEN[A0_KEYW_INDEX].keyword_p, KEYW_LEN) == 0 ? &keywords_with_length_##KEYW_LEN[A0_KEYW_INDEX] : NULL; \
    else if (*(STR) == (B0))                                                                                                     \
       (RESULT) = memcmp (STR, keywords_with_length_##KEYW_LEN[B0_KEYW_INDEX].keyword_p, KEYW_LEN) == 0 ? &keywords_with_length_##KEYW_LEN[B0_KEYW_INDEX] : NULL;\
    else                                                                                                      \
       (RESULT) = NULL;                                                                                                     \
    break; \
  }

/**
 * Parse identifier.
 *
 * @return true, if an identifier is parsed, false otherwise
 */
static bool
lexer_parse_identifier (parser_context_t *context_p, /**< context */
                        lexer_parse_options_t options) /**< check keywords */
{
  /* Only very few identifiers contains \u escape sequences. */
  const uint8_t *source_p = context_p->source_p;
  /* Note: newline or tab cannot be part of an identifier. */
  parser_line_counter_t column = context_p->column;
  const uint8_t *source_end_p = context_p->source_end_p;
  size_t length = 0;
  lexer_lit_location_flags_t status_flags = LEXER_LIT_LOCATION_IS_ASCII;

  do
  {
    if (*source_p == LIT_CHAR_BACKSLASH)
    {
      /* After a backslash an identifier must start. */
      lit_code_point_t code_point = UINT32_MAX;
      uint32_t escape_length = 6;

      if (options & (LEXER_PARSE_CHECK_START_AND_RETURN | LEXER_PARSE_CHECK_PART_AND_RETURN))
      {
        return true;
      }

      status_flags = LEXER_LIT_LOCATION_HAS_ESCAPE;

      if (source_p + 5 <= source_end_p && source_p[1] == LIT_CHAR_LOWERCASE_U)
      {
        if (source_p[2] == LIT_CHAR_LEFT_BRACE)
        {
          code_point = lexer_hex_in_braces_to_code_point (source_p + 3, source_end_p, &escape_length);
        }
        else if (source_p + 6 <= source_end_p)
        {
          code_point = lexer_hex_to_code_point (source_p + 2, 4);
        }
      }

      if (code_point == UINT32_MAX)
      {
        context_p->source_p = source_p;
        context_p->token.column = column;
        parser_raise_error (context_p, PARSER_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE);
      }

      if (length == 0)
      {
        if (!lit_code_point_is_identifier_start (code_point))
        {
          parser_raise_error (context_p, PARSER_ERR_INVALID_IDENTIFIER_START);
        }
      }
      else
      {
        if (!lit_code_point_is_identifier_part (code_point))
        {
          parser_raise_error (context_p, PARSER_ERR_INVALID_IDENTIFIER_PART);
        }
      }

      length += lit_code_point_get_cesu8_length (code_point);
      source_p += escape_length;
      PARSER_PLUS_EQUAL_LC (column, escape_length);
      continue;
    }

    lit_code_point_t code_point = *source_p;
    lit_utf8_size_t utf8_length = 1, decoded_length = 1, char_count = 1;

    if (JJS_UNLIKELY (code_point >= LIT_UTF8_2_BYTE_MARKER))
    {
      status_flags &= (uint32_t) ~LEXER_LIT_LOCATION_IS_ASCII;

      utf8_length = lit_read_code_point_from_utf8 (source_p, (lit_utf8_size_t) (source_end_p - source_p), &code_point);
      decoded_length = utf8_length;

      /* Only ES2015+ supports code points outside of the basic plane which can be part of an identifier. */
      if ((code_point >= LIT_UTF16_HIGH_SURROGATE_MIN && code_point <= LIT_UTF16_HIGH_SURROGATE_MAX)
          && source_p + 3 < source_end_p)
      {
        lit_code_point_t low_surrogate;
        lit_read_code_point_from_utf8 (source_p + 3, (lit_utf8_size_t) (source_end_p - (source_p + 3)), &low_surrogate);

        if (low_surrogate >= LIT_UTF16_LOW_SURROGATE_MIN && low_surrogate <= LIT_UTF16_LOW_SURROGATE_MAX)
        {
          code_point = lit_convert_surrogate_pair_to_code_point ((ecma_char_t) code_point, (ecma_char_t) low_surrogate);
          utf8_length = 2 * 3;
          decoded_length = 2 * 3;
          char_count = 2;
        }
      }
      else if (source_p[0] >= LIT_UTF8_4_BYTE_MARKER)
      {
        decoded_length = 2 * 3;
        status_flags = LEXER_LIT_LOCATION_HAS_ESCAPE;
#if JJS_FUNCTION_TO_STRING
        context_p->global_status_flags |= ECMA_PARSE_INTERNAL_HAS_4_BYTE_MARKER;
#endif /* JJS_FUNCTION_TO_STRING */
      }
    }

    if (length == 0)
    {
      if (JJS_UNLIKELY (options & (LEXER_PARSE_CHECK_START_AND_RETURN | LEXER_PARSE_CHECK_PART_AND_RETURN)))
      {
        if (options & LEXER_PARSE_CHECK_START_AND_RETURN)
        {
          return lit_code_point_is_identifier_start (code_point);
        }
        else
        {
          return lit_code_point_is_identifier_part (code_point);
        }
      }

      if (!lit_code_point_is_identifier_start (code_point))
      {
        return false;
      }
    }
    else if (!lit_code_point_is_identifier_part (code_point))
    {
      break;
    }

    source_p += utf8_length;
    length += decoded_length;
    PARSER_PLUS_EQUAL_LC (column, char_count);
  } while (source_p < source_end_p);

  JJS_ASSERT (length > 0);

  context_p->token.type = LEXER_LITERAL;
  context_p->token.lit_location.type = LEXER_IDENT_LITERAL;
  context_p->token.lit_location.status_flags = (uint8_t) status_flags;

  context_p->token.column = context_p->column;
  context_p->token.lit_location.char_p = context_p->source_p;
  context_p->token.lit_location.length = (prop_length_t) length;

  if (JJS_UNLIKELY (length > PARSER_MAXIMUM_IDENT_LENGTH))
  {
    parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_TOO_LONG);
  }

  /* Check keywords. */
  if ((options & LEXER_PARSE_CHECK_KEYWORDS)
      && (length >= LEXER_KEYWORD_MIN_LENGTH && length <= LEXER_KEYWORD_MAX_LENGTH))
  {
    const uint8_t *ident_start_p = context_p->source_p;
    uint8_t buffer_p[LEXER_KEYWORD_MAX_LENGTH];

    if (JJS_UNLIKELY (context_p->token.lit_location.status_flags & LEXER_LIT_LOCATION_HAS_ESCAPE))
    {
      lexer_convert_ident_to_cesu8 (buffer_p, ident_start_p, (prop_length_t) length);
      ident_start_p = buffer_p;
    }

    const keyword_string_t *match_p;

    /*
     * Imperfect Hashtable for Keywords
     *
     * Keywords are stored in separate arrays by length. Rather than trying
     * a conventional hash, switch on the keyword size to get the keyword
     * table. Then, manually select an index that contains a unique char
     * for all same sized keywords. If a single index is not unique another
     * one is also considered. If those checks pass, the ident is compared
     * to the matching keyword.
     */
    switch (length)
    {
      case 2:
      {
        LEXER_KEYW_MATCH_START (ident_start_p[1])
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'o', 0, 2) /* do */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'f', 1, 2) /* if */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'n', 2, 2) /* in */
        LEXER_KEYW_MATCH_END (match_p)
      }
      case 3:
        LEXER_KEYW_MATCH_START (ident_start_p[0])
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'f', 0, 3) /* for */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'l', 1, 3) /* let */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'n', 2, 3) /* new */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 't', 3, 3) /* try */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'v', 4, 3) /* var */
        LEXER_KEYW_MATCH_END (match_p)
      case 4:
        LEXER_KEYW_MATCH_START (ident_start_p[1])
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'a', 0, 4) /* case */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'l', 1, 4) /* else */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'n', 2, 4) /* enum */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'v', 3, 4) /* eval */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'e', 4, 4) /* meta */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'u', 5, 4) /* null */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'h', 6, 4) /* this */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'r', 7, 4) /* true */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'o', 8, 4) /* void */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'i', 9, 4) /* with */
        LEXER_KEYW_MATCH_END (match_p)
      case 5:
        LEXER_KEYW_MATCH_START (ident_start_p[2])
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'y', 0, 5)  /* async */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 't', 3, 5)  /* catch */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'n', 5, 5)  /* const */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'l', 6, 5)  /* false */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'p', 7, 5)  /* super */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'r', 8, 5)  /* throw */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'i', 9, 5)  /* while */
        LEXER_KEYW_MATCH_CASE_COLLISION (match_p, ident_start_p, 'a', 'a', 1, 'c', 4, 5) /* await, class */
        LEXER_KEYW_MATCH_CASE_COLLISION (match_p, ident_start_p, 'e', 'b', 2, 'y', 10, 5) /* break, yield */
        LEXER_KEYW_MATCH_END (match_p)
      case 6:
        LEXER_KEYW_MATCH_START (ident_start_p[1])
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'x', 1, 6) /* export */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'm', 2, 6) /* import */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'u', 3, 6) /* public */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 't', 5, 6) /* static */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'w', 6, 6) /* switch */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'y', 7, 6) /* typeof */
        LEXER_KEYW_MATCH_CASE_COLLISION (match_p, ident_start_p, 'e', 'd', 0, 'r', 4, 6) /* delete, return */
        LEXER_KEYW_MATCH_END (match_p)
      case 7:
        LEXER_KEYW_MATCH_START (ident_start_p[1])
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'e', 0, 7) /* default */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'x', 1, 7) /* extends */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'i', 2, 7) /* finally */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'a', 3, 7) /* package */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'r', 4, 7) /* private */
        LEXER_KEYW_MATCH_END (match_p)
      case 8:
        LEXER_KEYW_MATCH_START (ident_start_p[1])
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'o', 0, 8) /* continue */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'e', 1, 8) /* debugger */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'u', 2, 8) /* function */
        LEXER_KEYW_MATCH_END (match_p)
      case 9:
        LEXER_KEYW_MATCH_START (ident_start_p[2])
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'g', 0, 9) /* arguments */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 't', 1, 9) /* interface */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'o', 2, 9) /* protected */
        LEXER_KEYW_MATCH_END (match_p)
      case 10:
        LEXER_KEYW_MATCH_START (ident_start_p[1])
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'm', 0, 10) /* implements */
        LEXER_KEYW_MATCH_CASE (match_p, ident_start_p, 'n', 1, 10) /* instanceof */
        LEXER_KEYW_MATCH_END (match_p)
      case 0:
      case 1:
      default:
      {
        match_p = NULL;
        break;
      }
    }

    if (match_p)
    {
      context_p->token.keyword_type = (uint8_t) match_p->type;

      /* TODO: should this be a simpler switch? */
      if (JJS_LIKELY (match_p->type < LEXER_FIRST_NON_RESERVED_KEYWORD))
      {
        if (JJS_UNLIKELY (match_p->type == LEXER_KEYW_AWAIT))
        {
          if (!(context_p->status_flags & (PARSER_IS_ASYNC_FUNCTION | PARSER_IS_CLASS_STATIC_BLOCK))
              && !(context_p->global_status_flags & ECMA_PARSE_MODULE))
          {
            /* fallthrough */
          }
          else if (context_p->status_flags & PARSER_DISALLOW_AWAIT_YIELD)
          {
            if (LEXER_CHECK_INVALID_KEYWORD (ident_start_p, buffer_p))
            {
              parser_raise_error (context_p, PARSER_ERR_INVALID_KEYWORD);
            }
            parser_raise_error (context_p, PARSER_ERR_AWAIT_NOT_ALLOWED);

            context_p->token.type = (uint8_t) LEXER_KEYW_AWAIT;
          }
          else
          {
            context_p->token.type = (uint8_t) LEXER_KEYW_AWAIT;
          }
        }
        else if (LEXER_CHECK_INVALID_KEYWORD (ident_start_p, buffer_p))
        {
          /* Escape sequences are not allowed in a keyword. */
          parser_raise_error (context_p, PARSER_ERR_INVALID_KEYWORD);
          context_p->token.type = (uint8_t) match_p->type;
        }
        else
        {
          context_p->token.type = (uint8_t) match_p->type;
        }
      }
      else if (match_p->type == LEXER_KEYW_LET && (context_p->status_flags & PARSER_IS_STRICT))
      {
        if (LEXER_CHECK_INVALID_KEYWORD (ident_start_p, buffer_p))
        {
          parser_raise_error (context_p, PARSER_ERR_INVALID_KEYWORD);
        }

        context_p->token.type = (uint8_t) LEXER_KEYW_LET;
      }
      else if (match_p->type == LEXER_KEYW_YIELD && (context_p->status_flags & PARSER_IS_GENERATOR_FUNCTION))
      {
        if (context_p->status_flags & PARSER_DISALLOW_AWAIT_YIELD)
        {
          if (LEXER_CHECK_INVALID_KEYWORD (ident_start_p, buffer_p))
          {
            parser_raise_error (context_p, PARSER_ERR_INVALID_KEYWORD);
          }
          parser_raise_error (context_p, PARSER_ERR_YIELD_NOT_ALLOWED);
        }

        context_p->token.type = (uint8_t) LEXER_KEYW_YIELD;
      }
      else if (match_p->type == LEXER_KEYW_ARGUMENTS && (context_p->status_flags & PARSER_INSIDE_CLASS_FIELD))
      {
        parser_raise_error (context_p, PARSER_ERR_ARGUMENTS_IN_CLASS_FIELD);
      }
      else if (match_p->type >= LEXER_FIRST_FUTURE_STRICT_RESERVED_WORD && (context_p->status_flags & PARSER_IS_STRICT)
          && !(options & LEXER_PARSE_NO_STRICT_IDENT_ERROR))
      {
        parser_raise_error (context_p, PARSER_ERR_STRICT_IDENT_NOT_ALLOWED);
      }
    }
  }

  context_p->source_p = source_p;
  context_p->column = column;
  return true;
} /* lexer_parse_identifier */

#undef LEXER_CHECK_INVALID_KEYWORD

/**
 * Parse string.
 */
void
lexer_parse_string (parser_context_t *parser_context_p, /**< context */
                    lexer_string_options_t opts) /**< options */
{
  int32_t raw_length_adjust = 0;

  uint8_t str_end_character = parser_context_p->source_p[0];
  const uint8_t *source_p = parser_context_p->source_p + 1;
  const uint8_t *string_start_p = source_p;
  const uint8_t *source_end_p = parser_context_p->source_end_p;
  parser_line_counter_t line = parser_context_p->line;
  parser_line_counter_t column = (parser_line_counter_t) (parser_context_p->column + 1);
  parser_line_counter_t original_line = line;
  parser_line_counter_t original_column = column;
  size_t length = 0;
  lexer_lit_location_flags_t status_flags = LEXER_LIT_LOCATION_IS_ASCII;

  if (str_end_character == LIT_CHAR_RIGHT_BRACE)
  {
    str_end_character = LIT_CHAR_GRAVE_ACCENT;
  }

  while (true)
  {
    if (source_p >= source_end_p)
    {
      parser_context_p->token.line = original_line;
      parser_context_p->token.column = (parser_line_counter_t) (original_column - 1);
      parser_raise_error (parser_context_p, PARSER_ERR_UNTERMINATED_STRING);
    }

    if (*source_p == str_end_character)
    {
      break;
    }

    if (*source_p == LIT_CHAR_BACKSLASH)
    {
      source_p++;
      column++;
      if (source_p >= source_end_p)
      {
        /* Will throw an unterminated string error. */
        continue;
      }

      status_flags = LEXER_LIT_LOCATION_HAS_ESCAPE;

      /* Newline is ignored. */
      if (*source_p == LIT_CHAR_CR)
      {
        source_p++;
        if (source_p < source_end_p && *source_p == LIT_CHAR_LF)
        {
          raw_length_adjust--;
          source_p++;
        }

        line++;
        column = 1;
        continue;
      }
      else if (*source_p == LIT_CHAR_LF)
      {
        source_p++;
        line++;
        column = 1;
        continue;
      }
      else if (*source_p == LEXER_NEWLINE_LS_PS_BYTE_1 && LEXER_NEWLINE_LS_PS_BYTE_23 (source_p))
      {
        source_p += 3;
        line++;
        column = 1;
        continue;
      }

      if (opts & LEXER_STRING_RAW)
      {
        if ((*source_p == LIT_CHAR_GRAVE_ACCENT) || (*source_p == LIT_CHAR_BACKSLASH))
        {
          source_p++;
          column++;
          length++;
        }
        continue;
      }

      if (*source_p == LIT_CHAR_0 && source_p + 1 < source_end_p
          && (*(source_p + 1) < LIT_CHAR_0 || *(source_p + 1) > LIT_CHAR_9))
      {
        source_p++;
        column++;
        length++;
        continue;
      }

      /* Except \x, \u, and octal numbers, everything is
       * converted to a character which has the same byte length. */
      if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_3)
      {
        if (str_end_character == LIT_CHAR_GRAVE_ACCENT)
        {
          parser_raise_error (parser_context_p, PARSER_ERR_TEMPLATE_STR_OCTAL_ESCAPE);
        }

        if (parser_context_p->status_flags & PARSER_IS_STRICT)
        {
          parser_raise_error (parser_context_p, PARSER_ERR_OCTAL_ESCAPE_NOT_ALLOWED);
        }

        source_p++;
        column++;

        if (source_p < source_end_p && *source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
        {
          source_p++;
          column++;

          if (source_p < source_end_p && *source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
          {
            /* Numbers >= 0x200 (0x80) requires
             * two bytes for encoding in UTF-8. */
            if (source_p[-2] >= LIT_CHAR_2)
            {
              length++;
            }

            source_p++;
            column++;
          }
        }

        length++;
        continue;
      }

      if (*source_p >= LIT_CHAR_4 && *source_p <= LIT_CHAR_7)
      {
        if (parser_context_p->status_flags & PARSER_IS_STRICT)
        {
          parser_raise_error (parser_context_p, PARSER_ERR_OCTAL_ESCAPE_NOT_ALLOWED);
        }

        source_p++;
        column++;

        if (source_p < source_end_p && *source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
        {
          source_p++;
          column++;
        }

        /* The maximum number is 0x4d so the UTF-8
         * representation is always one byte. */
        length++;
        continue;
      }

      if (*source_p == LIT_CHAR_LOWERCASE_X || *source_p == LIT_CHAR_LOWERCASE_U)
      {
        uint32_t escape_length = (*source_p == LIT_CHAR_LOWERCASE_X) ? 3 : 5;
        lit_code_point_t code_point = UINT32_MAX;

        if (source_p + 4 <= source_end_p && source_p[0] == LIT_CHAR_LOWERCASE_U && source_p[1] == LIT_CHAR_LEFT_BRACE)
        {
          code_point = lexer_hex_in_braces_to_code_point (source_p + 2, source_end_p, &escape_length);
          escape_length--;
        }
        else
        {
          if (source_p + escape_length <= source_end_p)
          {
            code_point = lexer_hex_to_code_point (source_p + 1, escape_length - 1);
          }
        }

        if (code_point == UINT32_MAX)
        {
          parser_context_p->token.line = line;
          parser_context_p->token.column = (parser_line_counter_t) (column - 1);
          parser_raise_error (parser_context_p, PARSER_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE);
        }

        length += lit_code_point_get_cesu8_length (code_point);

        source_p += escape_length;
        PARSER_PLUS_EQUAL_LC (column, escape_length);
        continue;
      }
    }
    else if (str_end_character == LIT_CHAR_GRAVE_ACCENT && source_p[0] == LIT_CHAR_DOLLAR_SIGN
             && source_p + 1 < source_end_p && source_p[1] == LIT_CHAR_LEFT_BRACE)
    {
      raw_length_adjust--;
      source_p++;
      break;
    }

    if (*source_p >= LIT_UTF8_4_BYTE_MARKER)
    {
      /* Processing 4 byte unicode sequence (even if it is
       * after a backslash). Always converted to two 3 byte
       * long sequence. */
      length += 2 * 3;
      status_flags = LEXER_LIT_LOCATION_HAS_ESCAPE;
      source_p += 4;
      raw_length_adjust += 2;
      column++;
#if JJS_FUNCTION_TO_STRING
      parser_context_p->global_status_flags |= ECMA_PARSE_INTERNAL_HAS_4_BYTE_MARKER;
#endif /* JJS_FUNCTION_TO_STRING */
      continue;
    }
    else if (*source_p == LIT_CHAR_TAB)
    {
      column = align_column_to_tab (column);
      /* Subtract -1 because column is increased below. */
      column--;
    }
    else if (*source_p == LEXER_NEWLINE_LS_PS_BYTE_1 && LEXER_NEWLINE_LS_PS_BYTE_23 (source_p))
    {
      source_p += 3;
      length += 3;
      line++;
      column = 1;
      continue;
    }
    else if (str_end_character == LIT_CHAR_GRAVE_ACCENT)
    {
      /* Newline (without backslash) is part of the string.
         Note: ECMAScript v6, 11.8.6.1 <CR> or <CR><LF> are both normalized to <LF> */
      if (*source_p == LIT_CHAR_CR)
      {
        status_flags = LEXER_LIT_LOCATION_HAS_ESCAPE;
        source_p++;
        length++;
        if (source_p < source_end_p && *source_p == LIT_CHAR_LF)
        {
          source_p++;
          raw_length_adjust--;
        }
        line++;
        column = 1;
        continue;
      }
      else if (*source_p == LIT_CHAR_LF)
      {
        source_p++;
        length++;
        line++;
        column = 1;
        continue;
      }
    }
    else if (*source_p == LIT_CHAR_CR || *source_p == LIT_CHAR_LF)
    {
      parser_context_p->token.line = line;
      parser_context_p->token.column = column;
      parser_raise_error (parser_context_p, PARSER_ERR_NEWLINE_NOT_ALLOWED);
    }

    source_p++;
    column++;
    length++;

    while (source_p < source_end_p && IS_UTF8_INTERMEDIATE_OCTET (*source_p))
    {
      source_p++;
      length++;
    }
  }

  if (opts & LEXER_STRING_RAW)
  {
    length = (size_t) ((source_p - string_start_p) + raw_length_adjust);
  }

  if (length > PARSER_MAXIMUM_STRING_LENGTH)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_STRING_TOO_LONG);
  }

  parser_context_p->token.type = ((str_end_character != LIT_CHAR_GRAVE_ACCENT) ? LEXER_LITERAL : LEXER_TEMPLATE_LITERAL);

  /* Fill literal data. */
  parser_context_p->token.lit_location.char_p = string_start_p;
  parser_context_p->token.lit_location.length = (prop_length_t) length;
  parser_context_p->token.lit_location.type = LEXER_STRING_LITERAL;
  parser_context_p->token.lit_location.status_flags = (uint8_t) status_flags;

  parser_context_p->source_p = source_p + 1;
  parser_context_p->line = line;
  parser_context_p->column = (parser_line_counter_t) (column + 1);
} /* lexer_parse_string */

/**
 * Check number
 */
static void
lexer_check_numbers (parser_context_t *context_p, /**< context */
                     const uint8_t **source_p, /**< source_pointer */
                     const uint8_t *source_end_p, /**< end of the source */
                     const ecma_char_t digit_max, /**< maximum of the number range */
                     const bool is_legacy) /**< is legacy octal number  */
{
  while (true)
  {
    while (*source_p < source_end_p && *source_p[0] >= LIT_CHAR_0 && *source_p[0] <= digit_max)
    {
      *source_p += 1;
    }
    if (*source_p != source_end_p && *source_p[0] == LIT_CHAR_UNDERSCORE)
    {
      *source_p += 1;
      if (is_legacy || *source_p == source_end_p || *source_p[0] == LIT_CHAR_UNDERSCORE || *source_p[0] > digit_max
          || *source_p[0] < LIT_CHAR_0)
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_UNDERSCORE_IN_NUMBER);
      }
      continue;
    }

    break;
  }
} /* lexer_check_numbers */

/**
 * Parse number.
 */
static void
lexer_parse_number (parser_context_t *context_p) /**< context */
{
  const uint8_t *source_p = context_p->source_p;
  const uint8_t *source_end_p = context_p->source_end_p;
  bool can_be_float = false;
#if JJS_BUILTIN_BIGINT
  bool can_be_bigint = true;
#endif /* JJS_BUILTIN_BIGINT */
  size_t length;

  context_p->token.type = LEXER_LITERAL;
  context_p->token.extra_value = LEXER_NUMBER_DECIMAL;
  context_p->token.lit_location.char_p = source_p;
  context_p->token.lit_location.type = LEXER_NUMBER_LITERAL;
  context_p->token.lit_location.status_flags = LEXER_LIT_LOCATION_IS_ASCII;

  if (source_p[0] == LIT_CHAR_0 && source_p + 1 < source_end_p)
  {
    if (source_p[1] == LIT_CHAR_UNDERSCORE)
    {
      parser_raise_error (context_p, PARSER_ERR_INVALID_UNDERSCORE_IN_NUMBER);
    }

    if (LEXER_TO_ASCII_LOWERCASE (source_p[1]) == LIT_CHAR_LOWERCASE_X)
    {
      context_p->token.extra_value = LEXER_NUMBER_HEXADECIMAL;
      source_p += 2;

      if (source_p >= source_end_p || !lit_char_is_hex_digit (source_p[0]))
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_HEX_DIGIT);
      }

      do
      {
        source_p++;
        if (source_p < source_end_p && source_p[0] == LIT_CHAR_UNDERSCORE)
        {
          source_p++;
          if (source_p == source_end_p || !lit_char_is_hex_digit (source_p[0]))
          {
            parser_raise_error (context_p, PARSER_ERR_INVALID_UNDERSCORE_IN_NUMBER);
          }
        }
      } while (source_p < source_end_p && lit_char_is_hex_digit (source_p[0]));
    }
    else if (LEXER_TO_ASCII_LOWERCASE (source_p[1]) == LIT_CHAR_LOWERCASE_O)
    {
      context_p->token.extra_value = LEXER_NUMBER_OCTAL;
      source_p += 2;

      if (source_p >= source_end_p || !lit_char_is_octal_digit (source_p[0]))
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_OCTAL_DIGIT);
      }

      lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_7, false);
    }
    else if (source_p[1] >= LIT_CHAR_0 && source_p[1] <= LIT_CHAR_9)
    {
      context_p->token.extra_value = LEXER_NUMBER_OCTAL;
#if JJS_BUILTIN_BIGINT
      can_be_bigint = false;
#endif /* JJS_BUILTIN_BIGINT */

      if (context_p->status_flags & PARSER_IS_STRICT)
      {
        parser_raise_error (context_p, PARSER_ERR_OCTAL_NUMBER_NOT_ALLOWED);
      }

      lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_7, true);

      if (source_p < source_end_p && source_p[0] >= LIT_CHAR_8 && source_p[0] <= LIT_CHAR_9)
      {
        lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_9, true);
        context_p->token.extra_value = LEXER_NUMBER_DECIMAL;
      }
    }

    else if (LEXER_TO_ASCII_LOWERCASE (source_p[1]) == LIT_CHAR_LOWERCASE_B)
    {
      context_p->token.extra_value = LEXER_NUMBER_BINARY;
      source_p += 2;

      if (source_p >= source_end_p || !lit_char_is_binary_digit (source_p[0]))
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_BIN_DIGIT);
      }

      do
      {
        source_p++;
        if (source_p < source_end_p && source_p[0] == LIT_CHAR_UNDERSCORE)
        {
          source_p++;
          if (source_p == source_end_p || source_p[0] > LIT_CHAR_9 || source_p[0] < LIT_CHAR_0)
          {
            parser_raise_error (context_p, PARSER_ERR_INVALID_UNDERSCORE_IN_NUMBER);
          }
        }
      } while (source_p < source_end_p && lit_char_is_binary_digit (source_p[0]));
    }
    else
    {
      can_be_float = true;
      source_p++;
    }
  }
  else
  {
    lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_9, false);
    can_be_float = true;
  }

  if (can_be_float)
  {
    if (source_p < source_end_p && source_p[0] == LIT_CHAR_DOT)
    {
      source_p++;
#if JJS_BUILTIN_BIGINT
      can_be_bigint = false;
#endif /* JJS_BUILTIN_BIGINT */

      if (source_p < source_end_p && source_p[0] == LIT_CHAR_UNDERSCORE)
      {
        parser_raise_error (context_p, PARSER_ERR_INVALID_UNDERSCORE_IN_NUMBER);
      }

      lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_9, false);
    }

    if (source_p < source_end_p && LEXER_TO_ASCII_LOWERCASE (source_p[0]) == LIT_CHAR_LOWERCASE_E)
    {
      source_p++;
#if JJS_BUILTIN_BIGINT
      can_be_bigint = false;
#endif /* JJS_BUILTIN_BIGINT */

      if (source_p < source_end_p && (source_p[0] == LIT_CHAR_PLUS || source_p[0] == LIT_CHAR_MINUS))
      {
        source_p++;
      }

      if (source_p >= source_end_p || source_p[0] < LIT_CHAR_0 || source_p[0] > LIT_CHAR_9)
      {
        parser_raise_error (context_p, PARSER_ERR_MISSING_EXPONENT);
      }

      lexer_check_numbers (context_p, &source_p, source_end_p, LIT_CHAR_9, false);
    }
  }

#if JJS_BUILTIN_BIGINT
  if (source_p < source_end_p && source_p[0] == LIT_CHAR_LOWERCASE_N)
  {
    if (!can_be_bigint)
    {
      parser_raise_error (context_p, PARSER_ERR_INVALID_BIGINT);
    }
    context_p->token.extra_value = LEXER_NUMBER_BIGINT;
    source_p++;
  }
#endif /* JJS_BUILTIN_BIGINT */

  length = (size_t) (source_p - context_p->source_p);
  if (length > PARSER_MAXIMUM_STRING_LENGTH)
  {
    parser_raise_error (context_p, PARSER_ERR_NUMBER_TOO_LONG);
  }

  context_p->token.lit_location.length = (prop_length_t) length;
  PARSER_PLUS_EQUAL_LC (context_p->column, length);
  context_p->source_p = source_p;

  if (source_p < source_end_p && lexer_parse_identifier (context_p, LEXER_PARSE_CHECK_START_AND_RETURN))
  {
    parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_AFTER_NUMBER);
  }
} /* lexer_parse_number */

/**
 * One character long token (e.g. comma).
 *
 * @param char1 character
 * @param type1 type
 */
#define LEXER_TYPE_A_TOKEN(char1, type1) \
  case (uint8_t) (char1):                \
  {                                         \
    parser_context_p->token.type = (type1);     \
    length = 1;                          \
    break;                               \
  }

/**
 * Token pair, where the first token is prefix of the second (e.g. % and %=).
 *
 * @param char1 first character
 * @param type1 type of the first character
 * @param char2 second character
 * @param type2 type of the second character
 */
#define LEXER_TYPE_B_TOKEN(char1, type1, char2, type2)              \
  case (uint8_t) (char1):                                           \
  {                                                                 \
    if (length >= 2 && parser_context_p->source_p[1] == (uint8_t) (char2)) \
    {                                                                      \
      parser_context_p->token.type = (type2);                              \
      length = 2;                                                   \
      break;                                                        \
    }                                                                      \
                                                                           \
    parser_context_p->token.type = (type1);                                \
    length = 1;                                                     \
    break;                                                          \
  }

/**
 * Three tokens, where the first is the prefix of the other two (e.g. &, &&, &=).
 *
 * @param char1 first character
 * @param type1 type of the first character
 * @param char2 second character
 * @param type2 type of the second character
 * @param char3 third character
 * @param type3 type of the third character
 */
#define LEXER_TYPE_C_TOKEN(char1, type1, char2, type2, char3, type3) \
  case (uint8_t) (char1):                                            \
  {                                                                  \
    if (length >= 2)                                                 \
    {                                                                \
      if (parser_context_p->source_p[1] == (uint8_t) (char2))               \
      {                                                              \
        parser_context_p->token.type = (type2);                             \
        length = 2;                                                  \
        break;                                                       \
      }                                                              \
                                                                     \
      if (parser_context_p->source_p[1] == (uint8_t) (char3))               \
      {                                                              \
        parser_context_p->token.type = (type3);                             \
        length = 2;                                                  \
        break;                                                       \
      }                                                              \
    }                                                                \
                                                                     \
    parser_context_p->token.type = (type1);                                 \
    length = 1;                                                      \
    break;                                                           \
  }

/**
 * Four tokens, where the first is the prefix of the other three
 * and the second is prefix of the fourth (e.g. &, &&, &=, &&= ).
 *
 * @param char1 first character
 * @param type1 type of the first character
 * @param char2 second character
 * @param type2 type of the second character
 * @param char3 third character
 * @param type3 type of the third character
 * @param char4 fourth character
 * @param type4 type of the fourth character
 */
#define LEXER_TYPE_D_TOKEN(char1, type1, char2, type2, char3, type3, char4, type4) \
  case (uint8_t) (char1):                                                          \
  {                                                                                \
    if (length >= 2)                                                               \
    {                                                                              \
      if (parser_context_p->source_p[1] == (uint8_t) (char2))                             \
      {                                                                            \
        parser_context_p->token.type = (type2);                                           \
        length = 2;                                                                \
        break;                                                                     \
      }                                                                            \
                                                                                   \
      if (parser_context_p->source_p[1] == (uint8_t) (char3))                             \
      {                                                                            \
        if (length >= 3 && parser_context_p->source_p[2] == (uint8_t) (char4))            \
        {                                                                          \
          parser_context_p->token.type = (type4);                                         \
          length = 3;                                                              \
          break;                                                                   \
        }                                                                          \
        parser_context_p->token.type = (type3);                                           \
        length = 2;                                                                \
        break;                                                                     \
      }                                                                            \
    }                                                                              \
                                                                                   \
    parser_context_p->token.type = (type1);                                               \
    length = 1;                                                                    \
    break;                                                                         \
  }

/**
 * Skip the #! at the beginning of the source code. The function is expected
 * to be called at the beginning of the parsing phase.
 */
void
lexer_skip_hashbang (parser_context_t *parser_context_p) /**< context */
{
  const uint8_t *source_end_p = parser_context_p->source_end_p;

  if (parser_context_p->source_p + 1 < source_end_p && parser_context_p->source_p[0] == LIT_CHAR_HASHMARK && parser_context_p->source_p[1] == LIT_CHAR_EXCLAMATION)
  {
    parser_context_p->source_p += 2;
    PARSER_PLUS_EQUAL_LC (parser_context_p->column, 2);
    lexer_skip_spaces_mode (parser_context_p, LEXER_SKIP_SINGLE_LINE_COMMENT);
  }
} /* lexer_skip_hashbang */

/**
 * Get next token.
 */
void
lexer_next_token (parser_context_t *parser_context_p) /**< context */
{
  size_t length;

#if JJS_FUNCTION_TO_STRING
  /* Needed by arrow functions with expression body */
  parser_context_p->function_end_p = parser_context_p->source_p;
#endif /* JJS_FUNCTION_TO_STRING */

  lexer_skip_spaces (parser_context_p);

  parser_context_p->token.keyword_type = LEXER_EOS;
  parser_context_p->token.line = parser_context_p->line;
  parser_context_p->token.column = parser_context_p->column;

  length = (size_t) (parser_context_p->source_end_p - parser_context_p->source_p);
  if (length == 0)
  {
    parser_context_p->token.type = LEXER_EOS;
    return;
  }

  if (lexer_parse_identifier (parser_context_p, LEXER_PARSE_CHECK_KEYWORDS))
  {
    return;
  }

  if (parser_context_p->source_p[0] >= LIT_CHAR_0 && parser_context_p->source_p[0] <= LIT_CHAR_9)
  {
    lexer_parse_number (parser_context_p);
    return;
  }

  switch (parser_context_p->source_p[0])
  {
    LEXER_TYPE_A_TOKEN (LIT_CHAR_LEFT_BRACE, LEXER_LEFT_BRACE);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_LEFT_PAREN, LEXER_LEFT_PAREN);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_LEFT_SQUARE, LEXER_LEFT_SQUARE);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_RIGHT_BRACE, LEXER_RIGHT_BRACE);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_RIGHT_PAREN, LEXER_RIGHT_PAREN);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_RIGHT_SQUARE, LEXER_RIGHT_SQUARE);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_SEMICOLON, LEXER_SEMICOLON);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_COMMA, LEXER_COMMA);
    LEXER_TYPE_A_TOKEN (LIT_CHAR_HASHMARK, LEXER_HASHMARK);

    case (uint8_t) LIT_CHAR_DOT:
    {
      if (length >= 2 && (parser_context_p->source_p[1] >= LIT_CHAR_0 && parser_context_p->source_p[1] <= LIT_CHAR_9))
      {
        lexer_parse_number (parser_context_p);
        return;
      }

      if (length >= 3 && parser_context_p->source_p[1] == LIT_CHAR_DOT && parser_context_p->source_p[2] == LIT_CHAR_DOT)
      {
        parser_context_p->token.type = LEXER_THREE_DOTS;
        length = 3;
        break;
      }

      parser_context_p->token.type = LEXER_DOT;
      length = 1;
      break;
    }

    case (uint8_t) LIT_CHAR_LESS_THAN:
    {
      if (length >= 2)
      {
        if (parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_EQUALS)
        {
          parser_context_p->token.type = LEXER_LESS_EQUAL;
          length = 2;
          break;
        }

        if (parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_LESS_THAN)
        {
          if (length >= 3 && parser_context_p->source_p[2] == (uint8_t) LIT_CHAR_EQUALS)
          {
            parser_context_p->token.type = LEXER_ASSIGN_LEFT_SHIFT;
            length = 3;
            break;
          }

          parser_context_p->token.type = LEXER_LEFT_SHIFT;
          length = 2;
          break;
        }
      }

      parser_context_p->token.type = LEXER_LESS;
      length = 1;
      break;
    }

    case (uint8_t) LIT_CHAR_GREATER_THAN:
    {
      if (length >= 2)
      {
        if (parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_EQUALS)
        {
          parser_context_p->token.type = LEXER_GREATER_EQUAL;
          length = 2;
          break;
        }

        if (parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_GREATER_THAN)
        {
          if (length >= 3)
          {
            if (parser_context_p->source_p[2] == (uint8_t) LIT_CHAR_EQUALS)
            {
              parser_context_p->token.type = LEXER_ASSIGN_RIGHT_SHIFT;
              length = 3;
              break;
            }

            if (parser_context_p->source_p[2] == (uint8_t) LIT_CHAR_GREATER_THAN)
            {
              if (length >= 4 && parser_context_p->source_p[3] == (uint8_t) LIT_CHAR_EQUALS)
              {
                parser_context_p->token.type = LEXER_ASSIGN_UNS_RIGHT_SHIFT;
                length = 4;
                break;
              }

              parser_context_p->token.type = LEXER_UNS_RIGHT_SHIFT;
              length = 3;
              break;
            }
          }

          parser_context_p->token.type = LEXER_RIGHT_SHIFT;
          length = 2;
          break;
        }
      }

      parser_context_p->token.type = LEXER_GREATER;
      length = 1;
      break;
    }

    case (uint8_t) LIT_CHAR_EQUALS:
    {
      if (length >= 2)
      {
        if (parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_EQUALS)
        {
          if (length >= 3 && parser_context_p->source_p[2] == (uint8_t) LIT_CHAR_EQUALS)
          {
            parser_context_p->token.type = LEXER_STRICT_EQUAL;
            length = 3;
            break;
          }

          parser_context_p->token.type = LEXER_EQUAL;
          length = 2;
          break;
        }

        if (parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_GREATER_THAN)
        {
          parser_context_p->token.type = LEXER_ARROW;
          length = 2;
          break;
        }
      }

      parser_context_p->token.type = LEXER_ASSIGN;
      length = 1;
      break;
    }

    case (uint8_t) LIT_CHAR_EXCLAMATION:
    {
      if (length >= 2 && parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_EQUALS)
      {
        if (length >= 3 && parser_context_p->source_p[2] == (uint8_t) LIT_CHAR_EQUALS)
        {
          parser_context_p->token.type = LEXER_STRICT_NOT_EQUAL;
          length = 3;
          break;
        }

        parser_context_p->token.type = LEXER_NOT_EQUAL;
        length = 2;
        break;
      }

      parser_context_p->token.type = LEXER_LOGICAL_NOT;
      length = 1;
      break;
    }

      LEXER_TYPE_C_TOKEN (LIT_CHAR_PLUS, LEXER_ADD, LIT_CHAR_EQUALS, LEXER_ASSIGN_ADD, LIT_CHAR_PLUS, LEXER_INCREASE)
      LEXER_TYPE_C_TOKEN (LIT_CHAR_MINUS,
                          LEXER_SUBTRACT,
                          LIT_CHAR_EQUALS,
                          LEXER_ASSIGN_SUBTRACT,
                          LIT_CHAR_MINUS,
                          LEXER_DECREASE)

    case (uint8_t) LIT_CHAR_ASTERISK:
    {
      if (length >= 2)
      {
        if (parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_EQUALS)
        {
          parser_context_p->token.type = LEXER_ASSIGN_MULTIPLY;
          length = 2;
          break;
        }

        if (parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_ASTERISK)
        {
          if (length >= 3 && parser_context_p->source_p[2] == (uint8_t) LIT_CHAR_EQUALS)
          {
            parser_context_p->token.type = LEXER_ASSIGN_EXPONENTIATION;
            length = 3;
            break;
          }

          parser_context_p->token.type = LEXER_EXPONENTIATION;
          length = 2;
          break;
        }
      }

      parser_context_p->token.type = LEXER_MULTIPLY;
      length = 1;
      break;
    }

      LEXER_TYPE_B_TOKEN (LIT_CHAR_SLASH, LEXER_DIVIDE, LIT_CHAR_EQUALS, LEXER_ASSIGN_DIVIDE)
      LEXER_TYPE_B_TOKEN (LIT_CHAR_PERCENT, LEXER_MODULO, LIT_CHAR_EQUALS, LEXER_ASSIGN_MODULO)

      LEXER_TYPE_D_TOKEN (LIT_CHAR_AMPERSAND,
                          LEXER_BIT_AND,
                          LIT_CHAR_EQUALS,
                          LEXER_ASSIGN_BIT_AND,
                          LIT_CHAR_AMPERSAND,
                          LEXER_LOGICAL_AND,
                          LIT_CHAR_EQUALS,
                          LEXER_ASSIGN_LOGICAL_AND)
      LEXER_TYPE_D_TOKEN (LIT_CHAR_VLINE,
                          LEXER_BIT_OR,
                          LIT_CHAR_EQUALS,
                          LEXER_ASSIGN_BIT_OR,
                          LIT_CHAR_VLINE,
                          LEXER_LOGICAL_OR,
                          LIT_CHAR_EQUALS,
                          LEXER_ASSIGN_LOGICAL_OR)

      LEXER_TYPE_B_TOKEN (LIT_CHAR_CIRCUMFLEX, LEXER_BIT_XOR, LIT_CHAR_EQUALS, LEXER_ASSIGN_BIT_XOR)

      LEXER_TYPE_A_TOKEN (LIT_CHAR_TILDE, LEXER_BIT_NOT);
    case (uint8_t) (LIT_CHAR_QUESTION):
    {
      if (length >= 2)
      {
        if (parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_QUESTION)
        {
          if (length >= 3 && parser_context_p->source_p[2] == (uint8_t) LIT_CHAR_EQUALS)
          {
            parser_context_p->token.type = LEXER_ASSIGN_NULLISH_COALESCING;
            length = 3;
            break;
          }
          parser_context_p->token.type = LEXER_NULLISH_COALESCING;
          length = 2;
          break;
        }
        if (parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_DOT)
        {
          if (length < 3 || !lit_char_is_decimal_digit (parser_context_p->source_p[2]))
          {
            parser_context_p->token.type = LEXER_QUESTION_MARK_DOT;
            length = 2;
            break;
          }
        }
      }

      parser_context_p->token.type = LEXER_QUESTION_MARK;
      length = 1;
      break;
    }

      LEXER_TYPE_A_TOKEN (LIT_CHAR_COLON, LEXER_COLON);

    case LIT_CHAR_SINGLE_QUOTE:
    case LIT_CHAR_DOUBLE_QUOTE:
    case LIT_CHAR_GRAVE_ACCENT:
    {
      lexer_parse_string (parser_context_p, LEXER_STRING_NO_OPTS);
      return;
    }

    default:
    {
      parser_raise_error (parser_context_p, PARSER_ERR_INVALID_CHARACTER);
    }
  }

  parser_context_p->source_p += length;
  PARSER_PLUS_EQUAL_LC (parser_context_p->column, length);
} /* lexer_next_token */

#undef LEXER_TYPE_A_TOKEN
#undef LEXER_TYPE_B_TOKEN
#undef LEXER_TYPE_C_TOKEN
#undef LEXER_TYPE_D_TOKEN

/**
 * Checks whether the next token starts with the specified character.
 *
 * @return true - if the next is the specified character
 *         false - otherwise
 */
bool
lexer_check_next_character (parser_context_t *parser_context_p, /**< context */
                            lit_utf8_byte_t character) /**< specified character */
{
  if (!(parser_context_p->token.flags & LEXER_NO_SKIP_SPACES))
  {
    lexer_skip_spaces (parser_context_p);
    parser_context_p->token.flags = (uint8_t) (parser_context_p->token.flags | LEXER_NO_SKIP_SPACES);
  }

  return (parser_context_p->source_p < parser_context_p->source_end_p && parser_context_p->source_p[0] == (uint8_t) character);
} /* lexer_check_next_character */

/**
 * Checks whether the next token starts with either specified characters.
 *
 * @return true - if the next is the specified character
 *         false - otherwise
 */
bool
lexer_check_next_characters (parser_context_t *parser_context_p, /**< context */
                             lit_utf8_byte_t character1, /**< first alternative character */
                             lit_utf8_byte_t character2) /**< second alternative character */
{
  if (!(parser_context_p->token.flags & LEXER_NO_SKIP_SPACES))
  {
    lexer_skip_spaces (parser_context_p);
    parser_context_p->token.flags = (uint8_t) (parser_context_p->token.flags | LEXER_NO_SKIP_SPACES);
  }

  return (parser_context_p->source_p < parser_context_p->source_end_p
          && (parser_context_p->source_p[0] == (uint8_t) character1 || parser_context_p->source_p[0] == (uint8_t) character2));
} /* lexer_check_next_characters */

/**
 * Consumes the next character. The character cannot be a white space.
 *
 * @return consumed character
 */
extern inline uint8_t JJS_ATTR_ALWAYS_INLINE
lexer_consume_next_character (parser_context_t *parser_context_p) /**< context */
{
  JJS_ASSERT (parser_context_p->source_p < parser_context_p->source_end_p);

  parser_context_p->token.flags &= (uint8_t) ~LEXER_NO_SKIP_SPACES;

  PARSER_PLUS_EQUAL_LC (parser_context_p->column, 1);
  return *parser_context_p->source_p++;
} /* lexer_consume_next_character */

/**
 * Checks whether the next character can be the start of a post primary expression
 *
 * Note:
 *     the result is not precise, but this inprecise result
 *     has no side effects for negating number literals
 *
 * @return true if the next character can be the start of a post primary expression
 */
bool
lexer_check_post_primary_exp (parser_context_t *parser_context_p) /**< context */
{
  if (!(parser_context_p->token.flags & LEXER_NO_SKIP_SPACES))
  {
    lexer_skip_spaces (parser_context_p);
    parser_context_p->token.flags = (uint8_t) (parser_context_p->token.flags | LEXER_NO_SKIP_SPACES);
  }

  if (parser_context_p->source_p >= parser_context_p->source_end_p)
  {
    return false;
  }

  switch (parser_context_p->source_p[0])
  {
    case LIT_CHAR_DOT:
    case LIT_CHAR_LEFT_PAREN:
    case LIT_CHAR_LEFT_SQUARE:
    case LIT_CHAR_GRAVE_ACCENT:
    {
      return true;
    }
    case LIT_CHAR_PLUS:
    case LIT_CHAR_MINUS:
    {
      return (!(parser_context_p->token.flags & LEXER_WAS_NEWLINE) && parser_context_p->source_p + 1 < parser_context_p->source_end_p
              && parser_context_p->source_p[1] == parser_context_p->source_p[0]);
    }
    case LIT_CHAR_ASTERISK:
    {
      return (parser_context_p->source_p + 1 < parser_context_p->source_end_p
              && parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_ASTERISK);
    }
  }

  return false;
} /* lexer_check_post_primary_exp */

/**
 * Checks whether the next token is a type used for detecting arrow functions.
 *
 * @return true if the next token is an arrow token
 */
bool
lexer_check_arrow (parser_context_t *parser_context_p) /**< context */
{
  if (!(parser_context_p->token.flags & LEXER_NO_SKIP_SPACES))
  {
    lexer_skip_spaces (parser_context_p);
    parser_context_p->token.flags = (uint8_t) (parser_context_p->token.flags | LEXER_NO_SKIP_SPACES);
  }

  return (!(parser_context_p->token.flags & LEXER_WAS_NEWLINE) && parser_context_p->source_p + 2 <= parser_context_p->source_end_p
          && parser_context_p->source_p[0] == (uint8_t) LIT_CHAR_EQUALS
          && parser_context_p->source_p[1] == (uint8_t) LIT_CHAR_GREATER_THAN);
} /* lexer_check_arrow */

/**
 * Checks whether the next token is a comma or equal sign.
 *
 * @return true if the next token is a comma or equal sign
 */
bool
lexer_check_arrow_param (parser_context_t *parser_context_p) /**< context */
{
  JJS_ASSERT (parser_context_p->token.flags & LEXER_NO_SKIP_SPACES);

  if (parser_context_p->source_p >= parser_context_p->source_end_p)
  {
    return false;
  }

  if (parser_context_p->source_p[0] == LIT_CHAR_COMMA)
  {
    return true;
  }

  if (parser_context_p->source_p[0] != LIT_CHAR_EQUALS)
  {
    return false;
  }

  return (parser_context_p->source_p + 1 >= parser_context_p->source_end_p || parser_context_p->source_p[1] != LIT_CHAR_EQUALS);
} /* lexer_check_arrow_param */

/**
 * Checks whether the yield expression has no argument.
 *
 * @return true if it has no argument
 */
bool
lexer_check_yield_no_arg (parser_context_t *parser_context_p) /**< context */
{
  if (parser_context_p->token.flags & LEXER_WAS_NEWLINE)
  {
    return true;
  }

  switch (parser_context_p->token.type)
  {
    case LEXER_RIGHT_BRACE:
    case LEXER_RIGHT_PAREN:
    case LEXER_RIGHT_SQUARE:
    case LEXER_COMMA:
    case LEXER_COLON:
    case LEXER_SEMICOLON:
    case LEXER_EOS:
    {
      return true;
    }
    default:
    {
      return false;
    }
  }
} /* lexer_check_yield_no_arg */

/**
 * Checks whether the next token is a multiply and consumes it.
 *
 * @return true if the next token is a multiply
 */
bool
lexer_consume_generator (parser_context_t *parser_context_p) /**< context */
{
  if (!(parser_context_p->token.flags & LEXER_NO_SKIP_SPACES))
  {
    lexer_skip_spaces (parser_context_p);
    parser_context_p->token.flags = (uint8_t) (parser_context_p->token.flags | LEXER_NO_SKIP_SPACES);
  }

  if (parser_context_p->source_p >= parser_context_p->source_end_p || parser_context_p->source_p[0] != LIT_CHAR_ASTERISK
      || (parser_context_p->source_p + 1 < parser_context_p->source_end_p
          && (parser_context_p->source_p[1] == LIT_CHAR_EQUALS || parser_context_p->source_p[1] == LIT_CHAR_ASTERISK)))
  {
    return false;
  }

  lexer_consume_next_character (parser_context_p);
  parser_context_p->token.type = LEXER_MULTIPLY;
  return true;
} /* lexer_consume_generator */

/**
 * Checks whether the next token is an equal sign and consumes it.
 *
 * @return true if the next token is an equal sign
 */
bool
lexer_consume_assign (parser_context_t *parser_context_p) /**< context */
{
  if (!(parser_context_p->token.flags & LEXER_NO_SKIP_SPACES))
  {
    lexer_skip_spaces (parser_context_p);
    parser_context_p->token.flags = (uint8_t) (parser_context_p->token.flags | LEXER_NO_SKIP_SPACES);
  }

  if (parser_context_p->source_p >= parser_context_p->source_end_p || parser_context_p->source_p[0] != LIT_CHAR_EQUALS
      || (parser_context_p->source_p + 1 < parser_context_p->source_end_p
          && (parser_context_p->source_p[1] == LIT_CHAR_EQUALS || parser_context_p->source_p[1] == LIT_CHAR_GREATER_THAN)))
  {
    return false;
  }

  lexer_consume_next_character (parser_context_p);
  parser_context_p->token.type = LEXER_ASSIGN;
  return true;
} /* lexer_consume_assign */

/**
 * Update await / yield keywords after an arrow function with expression.
 */
void
lexer_update_await_yield (parser_context_t *parser_context_p, /**< context */
                          uint32_t status_flags) /**< parser status flags after restore */
{
  if (!(status_flags & PARSER_IS_STRICT))
  {
    if (status_flags & PARSER_IS_GENERATOR_FUNCTION)
    {
      if (parser_context_p->token.type == LEXER_LITERAL && parser_context_p->token.keyword_type == LEXER_KEYW_YIELD)
      {
        parser_context_p->token.type = LEXER_KEYW_YIELD;
      }
    }
    else
    {
      if (parser_context_p->token.type == LEXER_KEYW_YIELD)
      {
        JJS_ASSERT (parser_context_p->token.keyword_type == LEXER_KEYW_YIELD);
        parser_context_p->token.type = LEXER_LITERAL;
      }
    }
  }

  if (!(parser_context_p->global_status_flags & ECMA_PARSE_MODULE))
  {
    if (status_flags & PARSER_IS_ASYNC_FUNCTION)
    {
      if (parser_context_p->token.type == LEXER_LITERAL && parser_context_p->token.keyword_type == LEXER_KEYW_AWAIT)
      {
        parser_context_p->token.type = LEXER_KEYW_AWAIT;
      }
    }
    else
    {
      if (parser_context_p->token.type == LEXER_KEYW_AWAIT)
      {
        JJS_ASSERT (parser_context_p->token.keyword_type == LEXER_KEYW_AWAIT);
        parser_context_p->token.type = LEXER_LITERAL;
      }
    }
  }
} /* lexer_update_await_yield */

/**
 * Read next token without skipping whitespaces and checking keywords
 *
 * @return true if the next literal is private identifier, false otherwise
 */
bool
lexer_scan_private_identifier (parser_context_t *parser_context_p) /**< context */
{
  parser_context_p->token.keyword_type = LEXER_EOS;
  parser_context_p->token.line = parser_context_p->line;
  parser_context_p->token.column = parser_context_p->column;

  return (parser_context_p->source_p < parser_context_p->source_end_p && lexer_parse_identifier (parser_context_p, LEXER_PARSE_NO_OPTS));
} /* lexer_scan_private_identifier */

/**
 * Convert an ident with escapes to a utf8 string.
 */
void
lexer_convert_ident_to_cesu8 (uint8_t *destination_p, /**< destination string */
                              const uint8_t *source_p, /**< source string */
                              prop_length_t length) /**< length of destination string */
{
  const uint8_t *destination_end_p = destination_p + length;

  JJS_ASSERT (length <= PARSER_MAXIMUM_IDENT_LENGTH);

  do
  {
    if (*source_p == LIT_CHAR_BACKSLASH)
    {
      source_p += 2;
      destination_p += lit_code_point_to_cesu8_bytes (destination_p, lexer_unchecked_hex_to_character (&source_p));
      continue;
    }

    if (*source_p >= LIT_UTF8_4_BYTE_MARKER)
    {
      lit_four_byte_utf8_char_to_cesu8 (destination_p, source_p);

      destination_p += 6;
      source_p += 4;
      continue;
    }

    *destination_p++ = *source_p++;
  } while (destination_p < destination_end_p);
} /* lexer_convert_ident_to_cesu8 */

/**
 * Convert literal to character sequence
 */
const uint8_t *
lexer_convert_literal_to_chars (parser_context_t *parser_context_p, /**< context */
                                const lexer_lit_location_t *literal_p, /**< literal location */
                                uint8_t *local_byte_array_p, /**< local byte array to store chars */
                                lexer_string_options_t opts) /**< options */
{
  JJS_ASSERT (parser_context_p->u.allocated_buffer_p == NULL);

  if (!(literal_p->status_flags & LEXER_LIT_LOCATION_HAS_ESCAPE))
  {
    return literal_p->char_p;
  }

  uint8_t *destination_start_p;
  if (literal_p->length > LEXER_MAX_LITERAL_LOCAL_BUFFER_SIZE)
  {
    parser_context_p->u.allocated_buffer_p = (uint8_t *) parser_malloc_scratch (parser_context_p, literal_p->length);
    parser_context_p->allocated_buffer_size = literal_p->length;
    destination_start_p = parser_context_p->u.allocated_buffer_p;
  }
  else
  {
    destination_start_p = local_byte_array_p;
  }

  if (literal_p->type == LEXER_IDENT_LITERAL)
  {
    lexer_convert_ident_to_cesu8 (destination_start_p, literal_p->char_p, literal_p->length);
    return destination_start_p;
  }

  const uint8_t *source_p = literal_p->char_p;
  uint8_t *destination_p = destination_start_p;

  uint8_t str_end_character = source_p[-1];

  if (str_end_character == LIT_CHAR_RIGHT_BRACE)
  {
    str_end_character = LIT_CHAR_GRAVE_ACCENT;
  }

  bool is_raw = (opts & LEXER_STRING_RAW) != 0;

  while (true)
  {
    if (*source_p == str_end_character)
    {
      break;
    }

    if (*source_p == LIT_CHAR_BACKSLASH && !is_raw)
    {
      uint8_t conv_character;

      source_p++;
      JJS_ASSERT (source_p < parser_context_p->source_end_p);

      /* Newline is ignored. */
      if (*source_p == LIT_CHAR_CR)
      {
        source_p++;
        JJS_ASSERT (source_p < parser_context_p->source_end_p);

        if (*source_p == LIT_CHAR_LF)
        {
          source_p++;
        }
        continue;
      }
      else if (*source_p == LIT_CHAR_LF)
      {
        source_p++;
        continue;
      }
      else if (*source_p == LEXER_NEWLINE_LS_PS_BYTE_1 && LEXER_NEWLINE_LS_PS_BYTE_23 (source_p))
      {
        source_p += 3;
        continue;
      }

      if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_3)
      {
        lit_code_point_t octal_number = (uint32_t) (*source_p - LIT_CHAR_0);

        source_p++;
        JJS_ASSERT (source_p < parser_context_p->source_end_p);

        if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
        {
          octal_number = octal_number * 8 + (uint32_t) (*source_p - LIT_CHAR_0);
          source_p++;
          JJS_ASSERT (source_p < parser_context_p->source_end_p);

          if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
          {
            octal_number = octal_number * 8 + (uint32_t) (*source_p - LIT_CHAR_0);
            source_p++;
            JJS_ASSERT (source_p < parser_context_p->source_end_p);
          }
        }

        destination_p += lit_code_point_to_cesu8_bytes (destination_p, octal_number);
        continue;
      }

      if (*source_p >= LIT_CHAR_4 && *source_p <= LIT_CHAR_7)
      {
        uint32_t octal_number = (uint32_t) (*source_p - LIT_CHAR_0);

        source_p++;
        JJS_ASSERT (source_p < parser_context_p->source_end_p);

        if (*source_p >= LIT_CHAR_0 && *source_p <= LIT_CHAR_7)
        {
          octal_number = octal_number * 8 + (uint32_t) (*source_p - LIT_CHAR_0);
          source_p++;
          JJS_ASSERT (source_p < parser_context_p->source_end_p);
        }

        *destination_p++ = (uint8_t) octal_number;
        continue;
      }

      if (*source_p == LIT_CHAR_LOWERCASE_X || *source_p == LIT_CHAR_LOWERCASE_U)
      {
        source_p++;
        destination_p += lit_code_point_to_cesu8_bytes (destination_p, lexer_unchecked_hex_to_character (&source_p));
        continue;
      }

      conv_character = *source_p;
      switch (*source_p)
      {
        case LIT_CHAR_LOWERCASE_B:
        {
          conv_character = 0x08;
          break;
        }
        case LIT_CHAR_LOWERCASE_T:
        {
          conv_character = 0x09;
          break;
        }
        case LIT_CHAR_LOWERCASE_N:
        {
          conv_character = 0x0a;
          break;
        }
        case LIT_CHAR_LOWERCASE_V:
        {
          conv_character = 0x0b;
          break;
        }
        case LIT_CHAR_LOWERCASE_F:
        {
          conv_character = 0x0c;
          break;
        }
        case LIT_CHAR_LOWERCASE_R:
        {
          conv_character = 0x0d;
          break;
        }
      }

      if (conv_character != *source_p)
      {
        *destination_p++ = conv_character;
        source_p++;
        continue;
      }
    }
    else if (str_end_character == LIT_CHAR_GRAVE_ACCENT)
    {
      if (source_p[0] == LIT_CHAR_DOLLAR_SIGN && source_p[1] == LIT_CHAR_LEFT_BRACE)
      {
        source_p++;
        JJS_ASSERT (source_p < parser_context_p->source_end_p);
        break;
      }
      if (*source_p == LIT_CHAR_CR)
      {
        *destination_p++ = LIT_CHAR_LF;
        source_p++;
        if (*source_p != str_end_character && *source_p == LIT_CHAR_LF)
        {
          source_p++;
        }
        continue;
      }
      if ((*source_p == LIT_CHAR_BACKSLASH) && is_raw)
      {
        JJS_ASSERT (source_p + 1 < parser_context_p->source_end_p);
        if ((*(source_p + 1) == LIT_CHAR_GRAVE_ACCENT) || (*(source_p + 1) == LIT_CHAR_BACKSLASH))
        {
          *destination_p++ = *source_p++;
          *destination_p++ = *source_p++;
          continue;
        }
      }
    }

    if (*source_p >= LIT_UTF8_4_BYTE_MARKER)
    {
      /* Processing 4 byte unicode sequence (even if it is
       * after a backslash). Always converted to two 3 byte
       * long sequence. */
      lit_four_byte_utf8_char_to_cesu8 (destination_p, source_p);

      destination_p += 6;
      source_p += 4;
      continue;
    }

    *destination_p++ = *source_p++;

    /* There is no need to check the source_end_p
     * since the string is terminated by a quotation mark. */
    while (IS_UTF8_INTERMEDIATE_OCTET (*source_p))
    {
      *destination_p++ = *source_p++;
    }
  }

  JJS_ASSERT (destination_p == destination_start_p + literal_p->length);

  return destination_start_p;
} /* lexer_convert_literal_to_chars */

/**
 * Construct an unused literal.
 *
 * @return a newly allocated literal
 */
lexer_literal_t *
lexer_construct_unused_literal (parser_context_t *parser_context_p) /**< context */
{
  lexer_literal_t *literal_p;

  if (parser_context_p->literal_count >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
  }

  literal_p = (lexer_literal_t *) parser_list_append (parser_context_p, &parser_context_p->literal_pool);
  literal_p->type = LEXER_UNUSED_LITERAL;
  literal_p->status_flags = 0;
  return literal_p;
} /* lexer_construct_unused_literal */

/**
 * Construct a literal object from an identifier.
 */
void
lexer_construct_literal_object (parser_context_t *parser_context_p, /**< context */
                                const lexer_lit_location_t *lit_location_p, /**< literal location */
                                uint8_t literal_type) /**< final literal type */
{
  uint8_t local_byte_array[LEXER_MAX_LITERAL_LOCAL_BUFFER_SIZE];
//  ecma_context_t *context_p = parser_context_p->context_p;
  const uint8_t *char_p =
    lexer_convert_literal_to_chars (parser_context_p, lit_location_p, local_byte_array, LEXER_STRING_NO_OPTS);

  prop_length_t length = lit_location_p->length;
  parser_list_iterator_t literal_iterator;
  lexer_literal_t *literal_p;
  uint32_t literal_index = 0;
  bool search_scope_stack = (literal_type == LEXER_IDENT_LITERAL);

  if (JJS_UNLIKELY (literal_type == LEXER_NEW_IDENT_LITERAL))
  {
    literal_type = LEXER_IDENT_LITERAL;
  }

  JJS_ASSERT (literal_type == LEXER_IDENT_LITERAL || literal_type == LEXER_STRING_LITERAL);

  JJS_ASSERT (literal_type != LEXER_IDENT_LITERAL || length <= PARSER_MAXIMUM_IDENT_LENGTH);
  JJS_ASSERT (literal_type != LEXER_STRING_LITERAL || length <= PARSER_MAXIMUM_STRING_LENGTH);

  parser_list_iterator_init (&parser_context_p->literal_pool, &literal_iterator);

  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
  {
    if (literal_p->type == literal_type && literal_p->prop.length == length
        && memcmp (literal_p->u.char_p, char_p, length) == 0)
    {
      parser_context_p->lit_object.literal_p = literal_p;
      parser_context_p->lit_object.index = (uint16_t) literal_index;

      parser_free_allocated_buffer (parser_context_p);

      if (search_scope_stack)
      {
        parser_scope_stack_t *scope_stack_start_p = parser_context_p->scope_stack_p;
        parser_scope_stack_t *scope_stack_p
          = scope_stack_start_p ? scope_stack_start_p + parser_context_p->scope_stack_top : NULL;

        while (scope_stack_p > scope_stack_start_p)
        {
          scope_stack_p--;

          if (scope_stack_p->map_from == literal_index)
          {
            JJS_ASSERT (scanner_decode_map_to (scope_stack_p) >= PARSER_REGISTER_START
                          || (literal_p->status_flags & LEXER_FLAG_USED));
            parser_context_p->lit_object.index = scanner_decode_map_to (scope_stack_p);
            return;
          }
        }

        literal_p->status_flags |= LEXER_FLAG_USED;
      }
      return;
    }

    literal_index++;
  }

  JJS_ASSERT (literal_index == parser_context_p->literal_count);

  if (literal_index >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
  }

  literal_p = (lexer_literal_t *) parser_list_append (parser_context_p, &parser_context_p->literal_pool);
  literal_p->prop.length = length;
  literal_p->type = literal_type;

  uint8_t status_flags = LEXER_FLAG_SOURCE_PTR;

  if (length > 0 && char_p == local_byte_array)
  {
    literal_p->u.char_p = (uint8_t *) parser_malloc_scratch (parser_context_p, length);
    memcpy ((uint8_t *) literal_p->u.char_p, char_p, length);
    status_flags = 0;
  }
  else
  {
    literal_p->u.char_p = char_p;

    /* Buffer is taken over when a new literal is constructed. */
    if (parser_context_p->u.allocated_buffer_p != NULL)
    {
      JJS_ASSERT (char_p == parser_context_p->u.allocated_buffer_p);

      parser_context_p->u.allocated_buffer_p = NULL;
      status_flags = 0;
    }
  }

  if (search_scope_stack)
  {
    status_flags |= LEXER_FLAG_USED;
  }

  if (lit_location_p->status_flags & LEXER_LIT_LOCATION_IS_ASCII)
  {
    literal_p->status_flags |= LEXER_FLAG_ASCII;
  }

  literal_p->status_flags = status_flags;

  parser_context_p->lit_object.literal_p = literal_p;
  parser_context_p->lit_object.index = (uint16_t) literal_index;
  parser_context_p->literal_count++;

  JJS_ASSERT (parser_context_p->u.allocated_buffer_p == NULL);
} /* lexer_construct_literal_object */

/**
 * Construct a number object.
 *
 * @return true if number is small number
 */
bool
lexer_construct_number_object (parser_context_t *parser_context_p, /**< context */
                               bool is_expr, /**< expression is parsed */
                               bool is_negative_number) /**< sign is negative */
{
  parser_list_iterator_t literal_iterator;
  lexer_literal_t *literal_p;
  ecma_value_t lit_value;
  uint32_t literal_index = 0;
  prop_length_t length = parser_context_p->token.lit_location.length;
  ecma_context_t *context_p = parser_context_p->context_p;

#if JJS_BUILTIN_BIGINT
  if (JJS_LIKELY (parser_context_p->token.extra_value != LEXER_NUMBER_BIGINT))
  {
#endif /* JJS_BUILTIN_BIGINT */
    ecma_number_t num;
    uint32_t options = ECMA_CONVERSION_ALLOW_UNDERSCORE;

    if (parser_context_p->token.extra_value == LEXER_NUMBER_OCTAL)
    {
      num = ecma_utf8_string_to_number_by_radix (parser_context_p->token.lit_location.char_p, length, 8, options);
    }
    else
    {
      num = ecma_utf8_string_to_number (parser_context_p->token.lit_location.char_p, length, options);
    }

    if (is_expr && num > 0 && num <= CBC_PUSH_NUMBER_BYTE_RANGE_END)
    {
      int32_t int_num = (int32_t) num;

      if (int_num == num && int_num <= CBC_PUSH_NUMBER_BYTE_RANGE_END && (int_num != 0 || !is_negative_number))
      {
        parser_context_p->lit_object.index = (uint16_t) int_num;
        return true;
      }
    }

    if (is_negative_number)
    {
      num = -num;
    }

    lit_value = ecma_find_or_create_literal_number (context_p, num);
#if JJS_BUILTIN_BIGINT
  }
  else
  {
    uint32_t options = (ECMA_BIGINT_PARSE_DISALLOW_SYNTAX_ERROR | ECMA_BIGINT_PARSE_DISALLOW_MEMORY_ERROR
                        | ECMA_BIGINT_PARSE_ALLOW_UNDERSCORE);

    if (is_negative_number)
    {
      options |= ECMA_BIGINT_PARSE_SET_NEGATIVE;
    }

    JJS_ASSERT (length >= 2);
    lit_value =
      ecma_bigint_parse_string (context_p, parser_context_p->token.lit_location.char_p, (lit_utf8_size_t) (length - 1), options);

    JJS_ASSERT (lit_value != ECMA_VALUE_FALSE && !ECMA_IS_VALUE_ERROR (lit_value));

    if (lit_value == ECMA_VALUE_NULL)
    {
      parser_raise_error (parser_context_p, PARSER_ERR_OUT_OF_MEMORY);
    }

    lit_value = ecma_find_or_create_literal_bigint (context_p, lit_value);
  }
#endif /* JJS_BUILTIN_BIGINT */

  parser_list_iterator_init (&parser_context_p->literal_pool, &literal_iterator);

  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
  {
    if (literal_p->type == LEXER_NUMBER_LITERAL && literal_p->u.value == lit_value)
    {
      parser_context_p->lit_object.literal_p = literal_p;
      parser_context_p->lit_object.index = (uint16_t) literal_index;
      return false;
    }

    literal_index++;
  }

  JJS_ASSERT (literal_index == parser_context_p->literal_count);

  if (literal_index >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
  }

  literal_p = (lexer_literal_t *) parser_list_append (parser_context_p, &parser_context_p->literal_pool);
  literal_p->u.value = lit_value;
  literal_p->prop.length = 0; /* Unused. */
  literal_p->type = LEXER_NUMBER_LITERAL;
  literal_p->status_flags = 0;

  parser_context_p->lit_object.literal_p = literal_p;
  parser_context_p->lit_object.index = (uint16_t) literal_index;

  parser_context_p->literal_count++;
  return false;
} /* lexer_construct_number_object */

/**
 * Convert a push number opcode to push literal opcode
 */
void
lexer_convert_push_number_to_push_literal (parser_context_t *parser_context_p) /**< context */
{
  ecma_integer_value_t value;
  bool two_literals = parser_context_p->last_cbc_opcode >= CBC_PUSH_LITERAL_PUSH_NUMBER_0;

  if (parser_context_p->last_cbc_opcode == CBC_PUSH_NUMBER_0 || parser_context_p->last_cbc_opcode == CBC_PUSH_LITERAL_PUSH_NUMBER_0)
  {
    value = 0;
  }
  else if (parser_context_p->last_cbc_opcode == CBC_PUSH_NUMBER_POS_BYTE
           || parser_context_p->last_cbc_opcode == CBC_PUSH_LITERAL_PUSH_NUMBER_POS_BYTE)
  {
    value = ((ecma_integer_value_t) parser_context_p->last_cbc.value) + 1;
  }
  else
  {
    JJS_ASSERT (parser_context_p->last_cbc_opcode == CBC_PUSH_NUMBER_NEG_BYTE
                  || parser_context_p->last_cbc_opcode == CBC_PUSH_LITERAL_PUSH_NUMBER_NEG_BYTE);
    value = -((ecma_integer_value_t) parser_context_p->last_cbc.value) - 1;
  }

  ecma_value_t lit_value = ecma_make_integer_value (value);

  parser_list_iterator_t literal_iterator;
  parser_list_iterator_init (&parser_context_p->literal_pool, &literal_iterator);

  parser_context_p->last_cbc_opcode = two_literals ? CBC_PUSH_TWO_LITERALS : CBC_PUSH_LITERAL;

  uint32_t literal_index = 0;
  lexer_literal_t *literal_p;

  while ((literal_p = (lexer_literal_t *) parser_list_iterator_next (&literal_iterator)) != NULL)
  {
    if (literal_p->type == LEXER_NUMBER_LITERAL && literal_p->u.value == lit_value)
    {
      if (two_literals)
      {
        parser_context_p->last_cbc.value = (uint16_t) literal_index;
      }
      else
      {
        parser_context_p->last_cbc.literal_index = (uint16_t) literal_index;
      }
      return;
    }

    literal_index++;
  }

  JJS_ASSERT (literal_index == parser_context_p->literal_count);

  if (literal_index >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
  }

  literal_p = (lexer_literal_t *) parser_list_append (parser_context_p, &parser_context_p->literal_pool);
  literal_p->u.value = lit_value;
  literal_p->prop.length = 0; /* Unused. */
  literal_p->type = LEXER_NUMBER_LITERAL;
  literal_p->status_flags = 0;

  parser_context_p->literal_count++;

  if (two_literals)
  {
    parser_context_p->last_cbc.value = (uint16_t) literal_index;
  }
  else
  {
    parser_context_p->last_cbc.literal_index = (uint16_t) literal_index;
  }
} /* lexer_convert_push_number_to_push_literal */

/**
 * Construct a function literal object.
 *
 * @return function object literal index
 */
uint16_t
lexer_construct_function_object (parser_context_t *parser_context_p, /**< context */
                                 uint32_t extra_status_flags) /**< extra status flags */
{
  ecma_context_t *context_p = parser_context_p->context_p;

  if (context_p->vm_stack_limit != 0)
  {
    if (JJS_UNLIKELY (ecma_get_current_stack_usage (context_p) > context_p->vm_stack_limit))
    {
      parser_raise_error (parser_context_p, PARSER_ERR_STACK_OVERFLOW);
    }
  }

  ecma_compiled_code_t *compiled_code_p;
  lexer_literal_t *literal_p;
  uint16_t result_index;

  if (parser_context_p->status_flags & PARSER_INSIDE_WITH)
  {
    extra_status_flags |= PARSER_INSIDE_WITH;
  }

  literal_p = lexer_construct_unused_literal (parser_context_p);
  result_index = parser_context_p->literal_count;
  parser_context_p->literal_count++;

  parser_flush_cbc (parser_context_p);

  if (JJS_LIKELY (!(extra_status_flags & PARSER_IS_ARROW_FUNCTION)))
  {
    compiled_code_p = parser_parse_function (parser_context_p, extra_status_flags);
  }
  else
  {
    compiled_code_p = parser_parse_arrow_function (parser_context_p, extra_status_flags);
  }

  literal_p->u.bytecode_p = compiled_code_p;
  literal_p->type = LEXER_FUNCTION_LITERAL;

  return result_index;
} /* lexer_construct_function_object */

/**
 * Construct a class static block function literal object.
 *
 * @return function object literal index
 */
uint16_t
lexer_construct_class_static_block_function (parser_context_t *parser_context_p) /**< context */
{
  ecma_compiled_code_t *compiled_code_p;
  lexer_literal_t *literal_p;
  uint16_t result_index;

  literal_p = lexer_construct_unused_literal (parser_context_p);
  result_index = parser_context_p->literal_count;
  parser_context_p->literal_count++;

  parser_flush_cbc (parser_context_p);
  compiled_code_p = parser_parse_class_static_block (parser_context_p);

  literal_p->u.bytecode_p = compiled_code_p;
  literal_p->type = LEXER_FUNCTION_LITERAL;

  return result_index;
} /* lexer_construct_class_static_block_function */

/**
 * Construct a regular expression object.
 *
 * Note: In ESNEXT the constructed literal's type can be LEXER_STRING_LITERAL which represents
 * invalid pattern. In this case the lit_object's index contains the thrown error message literal.
 * Otherwise a new literal is appended to the end of the literal pool.
 */
void
lexer_construct_regexp_object (parser_context_t *parser_context_p, /**< context */
                               bool parse_only) /**< parse only */
{
#if JJS_BUILTIN_REGEXP
  const uint8_t *source_p = parser_context_p->source_p;
  const uint8_t *regex_start_p = parser_context_p->source_p;
  const uint8_t *regex_end_p = regex_start_p;
  const uint8_t *source_end_p = parser_context_p->source_end_p;
  parser_line_counter_t column = parser_context_p->column;
  bool in_class = false;
  uint16_t current_flags;
  lit_utf8_size_t length;

  JJS_ASSERT (parser_context_p->token.type == LEXER_DIVIDE || parser_context_p->token.type == LEXER_ASSIGN_DIVIDE);

  if (parser_context_p->token.type == LEXER_ASSIGN_DIVIDE)
  {
    regex_start_p--;
  }

  while (true)
  {
    if (source_p >= source_end_p)
    {
      parser_raise_error (parser_context_p, PARSER_ERR_UNTERMINATED_REGEXP);
    }

    if (!in_class && source_p[0] == LIT_CHAR_SLASH)
    {
      regex_end_p = source_p;
      source_p++;
      column++;
      break;
    }

    switch (source_p[0])
    {
      case LIT_CHAR_CR:
      case LIT_CHAR_LF:
      case LEXER_NEWLINE_LS_PS_BYTE_1:
      {
        if (source_p[0] != LEXER_NEWLINE_LS_PS_BYTE_1 || LEXER_NEWLINE_LS_PS_BYTE_23 (source_p))
        {
          parser_raise_error (parser_context_p, PARSER_ERR_NEWLINE_NOT_ALLOWED);
        }
        break;
      }
      case LIT_CHAR_TAB:
      {
        column = align_column_to_tab (column);
        /* Subtract -1 because column is increased below. */
        column--;
        break;
      }
      case LIT_CHAR_LEFT_SQUARE:
      {
        in_class = true;
        break;
      }
      case LIT_CHAR_RIGHT_SQUARE:
      {
        in_class = false;
        break;
      }
      case LIT_CHAR_BACKSLASH:
      {
        if (source_p + 1 >= source_end_p)
        {
          parser_raise_error (parser_context_p, PARSER_ERR_UNTERMINATED_REGEXP);
        }

        if (source_p[1] >= 0x20 && source_p[1] <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
        {
          source_p++;
          column++;
        }
      }
    }

    source_p++;
    column++;

    while (source_p < source_end_p && IS_UTF8_INTERMEDIATE_OCTET (source_p[0]))
    {
      source_p++;
    }
  }

  current_flags = 0;
  while (source_p < source_end_p)
  {
    uint32_t flag = 0;

    if (source_p[0] == LIT_CHAR_LOWERCASE_G)
    {
      flag = RE_FLAG_GLOBAL;
    }
    else if (source_p[0] == LIT_CHAR_LOWERCASE_I)
    {
      flag = RE_FLAG_IGNORE_CASE;
    }
    else if (source_p[0] == LIT_CHAR_LOWERCASE_M)
    {
      flag = RE_FLAG_MULTILINE;
    }
    else if (source_p[0] == LIT_CHAR_LOWERCASE_U)
    {
      flag = RE_FLAG_UNICODE;
    }
    else if (source_p[0] == LIT_CHAR_LOWERCASE_Y)
    {
      flag = RE_FLAG_STICKY;
    }
    else if (source_p[0] == LIT_CHAR_LOWERCASE_S)
    {
      flag = RE_FLAG_DOTALL;
    }

    if (flag == 0)
    {
      break;
    }

    if (current_flags & flag)
    {
      parser_raise_error (parser_context_p, PARSER_ERR_DUPLICATED_REGEXP_FLAG);
    }

    current_flags = (uint16_t) (current_flags | flag);
    source_p++;
    column++;
  }

  parser_context_p->source_p = source_p;
  parser_context_p->column = column;

  if (source_p < source_end_p && lexer_parse_identifier (parser_context_p, LEXER_PARSE_CHECK_PART_AND_RETURN))
  {
    parser_raise_error (parser_context_p, PARSER_ERR_UNKNOWN_REGEXP_FLAG);
  }

  length = (lit_utf8_size_t) (regex_end_p - regex_start_p);
  if (length > PARSER_MAXIMUM_STRING_LENGTH)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_REGEXP_TOO_LONG);
  }

  parser_context_p->column = column;
  parser_context_p->source_p = source_p;

  if (parse_only)
  {
    return;
  }

  if (parser_context_p->literal_count >= PARSER_MAXIMUM_NUMBER_OF_LITERALS)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_LITERAL_LIMIT_REACHED);
  }

  /* Compile the RegExp literal and store the RegExp bytecode pointer */
  ecma_string_t *pattern_str_p = NULL;
  ecma_context_t *context_p = parser_context_p->context_p;

  if (lit_is_valid_cesu8_string (regex_start_p, length))
  {
    pattern_str_p = ecma_new_ecma_string_from_utf8 (context_p, regex_start_p, length);
  }
  else
  {
    JJS_ASSERT (lit_is_valid_utf8_string (regex_start_p, length, false));
    pattern_str_p = ecma_new_ecma_string_from_utf8_converted_to_cesu8 (context_p, regex_start_p, length);
  }

  re_compiled_code_t *re_bytecode_p = re_compile_bytecode (parser_context_p->context_p, pattern_str_p, current_flags);
  ecma_deref_ecma_string (context_p, pattern_str_p);

  if (JJS_UNLIKELY (re_bytecode_p == NULL))
  {
    parser_raise_error (parser_context_p, PARSER_ERR_INVALID_REGEXP);
  }

  lexer_literal_t *literal_p = (lexer_literal_t *) parser_list_append (parser_context_p, &parser_context_p->literal_pool);
  literal_p->u.bytecode_p = (ecma_compiled_code_t *) re_bytecode_p;
  literal_p->type = LEXER_REGEXP_LITERAL;
  literal_p->prop.length = (prop_length_t) length;
  literal_p->status_flags = 0;

  parser_context_p->token.type = LEXER_LITERAL;
  parser_context_p->token.lit_location.type = LEXER_REGEXP_LITERAL;

  parser_context_p->lit_object.literal_p = literal_p;
  parser_context_p->lit_object.index = parser_context_p->literal_count++;
#else /* !JJS_BUILTIN_REGEXP */
  JJS_UNUSED (parse_only);
  parser_raise_error (context_p, PARSER_ERR_UNSUPPORTED_REGEXP);
#endif /* JJS_BUILTIN_REGEXP */
} /* lexer_construct_regexp_object */

/**
 * Next token must be an identifier.
 */
void
lexer_expect_identifier (parser_context_t *parser_context_p, /**< context */
                         uint8_t literal_type) /**< literal type */
{
  JJS_ASSERT (literal_type == LEXER_STRING_LITERAL || literal_type == LEXER_IDENT_LITERAL
                || literal_type == LEXER_NEW_IDENT_LITERAL);

  lexer_skip_spaces (parser_context_p);
  parser_context_p->token.keyword_type = LEXER_EOS;
  parser_context_p->token.line = parser_context_p->line;
  parser_context_p->token.column = parser_context_p->column;

  if (parser_context_p->source_p < parser_context_p->source_end_p
      && lexer_parse_identifier (
        parser_context_p,
        (literal_type != LEXER_STRING_LITERAL ? LEXER_PARSE_CHECK_KEYWORDS : LEXER_PARSE_NO_OPTS)))
  {
    if (parser_context_p->token.type == LEXER_LITERAL)
    {
      JJS_ASSERT (parser_context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

      lexer_construct_literal_object (parser_context_p, &parser_context_p->token.lit_location, literal_type);

      if (literal_type != LEXER_STRING_LITERAL && (parser_context_p->status_flags & PARSER_IS_STRICT))
      {
        if (parser_context_p->token.keyword_type == LEXER_KEYW_EVAL)
        {
          parser_raise_error (parser_context_p, PARSER_ERR_EVAL_NOT_ALLOWED);
        }
        else if (parser_context_p->token.keyword_type == LEXER_KEYW_ARGUMENTS)
        {
          parser_raise_error (parser_context_p, PARSER_ERR_ARGUMENTS_NOT_ALLOWED);
        }
      }
      return;
    }
  }
#if JJS_MODULE_SYSTEM
  else if (parser_context_p->status_flags & PARSER_MODULE_DEFAULT_CLASS_OR_FUNC)
  {
    /* When parsing default exports for modules, it is not required by functions or classes to have identifiers.
     * In this case we use a synthetic name for them. */
    parser_context_p->token.type = LEXER_LITERAL;
    parser_context_p->token.lit_location = lexer_default_literal;
    lexer_construct_literal_object (parser_context_p, &parser_context_p->token.lit_location, literal_type);
    parser_context_p->status_flags &= (uint32_t) ~(PARSER_MODULE_DEFAULT_CLASS_OR_FUNC);
    return;
  }
#endif /* JJS_MODULE_SYSTEM */
  if (parser_context_p->token.type == LEXER_KEYW_YIELD)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_YIELD_NOT_ALLOWED);
  }
  if (parser_context_p->token.type == LEXER_KEYW_AWAIT)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_AWAIT_NOT_ALLOWED);
  }
  parser_raise_error (parser_context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
} /* lexer_expect_identifier */

/**
 * Next token must be an identifier.
 */
void
lexer_expect_object_literal_id (parser_context_t *parser_context_p, /**< context */
                                uint32_t ident_opts) /**< lexer_obj_ident_opts_t option bits */
{
  lexer_skip_spaces (parser_context_p);

  if (parser_context_p->source_p >= parser_context_p->source_end_p)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_PROPERTY_IDENTIFIER_EXPECTED);
  }

  parser_context_p->token.keyword_type = LEXER_EOS;
  parser_context_p->token.line = parser_context_p->line;
  parser_context_p->token.column = parser_context_p->column;
  bool create_literal_object = false;

  JJS_ASSERT ((ident_opts & LEXER_OBJ_IDENT_CLASS_IDENTIFIER) || !(ident_opts & LEXER_OBJ_IDENT_CLASS_NO_STATIC));

#if JJS_FUNCTION_TO_STRING
  if (ident_opts & LEXER_OBJ_IDENT_SET_FUNCTION_START)
  {
    parser_context_p->function_start_p = parser_context_p->source_p;
  }
#endif /* JJS_FUNCTION_TO_STRING */

  if (lexer_parse_identifier (parser_context_p, LEXER_PARSE_NO_OPTS))
  {
    if (!(ident_opts & (LEXER_OBJ_IDENT_ONLY_IDENTIFIERS | LEXER_OBJ_IDENT_OBJECT_PATTERN)))
    {
      lexer_skip_spaces (parser_context_p);
      parser_context_p->token.flags = (uint8_t) (parser_context_p->token.flags | LEXER_NO_SKIP_SPACES);

      if (parser_context_p->source_p < parser_context_p->source_end_p && parser_context_p->source_p[0] != LIT_CHAR_COMMA
          && parser_context_p->source_p[0] != LIT_CHAR_RIGHT_BRACE && parser_context_p->source_p[0] != LIT_CHAR_LEFT_PAREN
          && parser_context_p->source_p[0] != LIT_CHAR_SEMICOLON && parser_context_p->source_p[0] != LIT_CHAR_EQUALS
          && parser_context_p->source_p[0] != LIT_CHAR_COLON)
      {
        if (lexer_compare_literal_to_string (parser_context_p, "get", 3))
        {
          parser_context_p->token.type = LEXER_PROPERTY_GETTER;
          return;
        }

        if (lexer_compare_literal_to_string (parser_context_p, "set", 3))
        {
          parser_context_p->token.type = LEXER_PROPERTY_SETTER;
          return;
        }

        if (lexer_compare_literal_to_string (parser_context_p, "async", 5))
        {
          parser_context_p->token.type = LEXER_KEYW_ASYNC;
          return;
        }

        if (ident_opts & LEXER_OBJ_IDENT_CLASS_NO_STATIC)
        {
          if (lexer_compare_literal_to_string (parser_context_p, "static", 6))
          {
            parser_context_p->token.type = LEXER_KEYW_STATIC;
          }
          return;
        }
      }
    }

    create_literal_object = true;
  }
  else if (ident_opts & LEXER_OBJ_IDENT_CLASS_PRIVATE)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_INVALID_CHARACTER);
  }
  else
  {
    switch (parser_context_p->source_p[0])
    {
      case LIT_CHAR_DOUBLE_QUOTE:
      case LIT_CHAR_SINGLE_QUOTE:
      {
        lexer_parse_string (parser_context_p, LEXER_STRING_NO_OPTS);
        create_literal_object = true;
        break;
      }
      case LIT_CHAR_LEFT_SQUARE:
      {
#if JJS_FUNCTION_TO_STRING
        const uint8_t *function_start_p = parser_context_p->function_start_p;
#endif /* JJS_FUNCTION_TO_STRING */

        lexer_consume_next_character (parser_context_p);

        lexer_next_token (parser_context_p);
        parser_parse_expression (parser_context_p, PARSE_EXPR_NO_COMMA);

        if (parser_context_p->token.type != LEXER_RIGHT_SQUARE)
        {
          parser_raise_error (parser_context_p, PARSER_ERR_RIGHT_SQUARE_EXPECTED);
        }

#if JJS_FUNCTION_TO_STRING
        parser_context_p->function_start_p = function_start_p;
#endif /* JJS_FUNCTION_TO_STRING */
        return;
      }
      case LIT_CHAR_ASTERISK:
      {
        if (ident_opts & (LEXER_OBJ_IDENT_ONLY_IDENTIFIERS | LEXER_OBJ_IDENT_OBJECT_PATTERN))
        {
          break;
        }

        parser_context_p->token.type = LEXER_MULTIPLY;
        lexer_consume_next_character (parser_context_p);
        return;
      }
      case LIT_CHAR_HASHMARK:
      {
        if (ident_opts & LEXER_OBJ_IDENT_CLASS_IDENTIFIER)
        {
          parser_context_p->token.type = LEXER_HASHMARK;
          return;
        }

        break;
      }
      case LIT_CHAR_LEFT_BRACE:
      {
        const uint32_t static_block_flags =
          (LEXER_OBJ_IDENT_CLASS_NO_STATIC | LEXER_OBJ_IDENT_CLASS_PRIVATE | LEXER_OBJ_IDENT_CLASS_IDENTIFIER);

        if ((ident_opts & static_block_flags) == LEXER_OBJ_IDENT_CLASS_IDENTIFIER)
        {
          parser_context_p->token.type = LEXER_LEFT_BRACE;
          lexer_consume_next_character (parser_context_p);
          return;
        }

        break;
      }
      case LIT_CHAR_RIGHT_BRACE:
      {
        if (ident_opts & LEXER_OBJ_IDENT_ONLY_IDENTIFIERS)
        {
          break;
        }

        parser_context_p->token.type = LEXER_RIGHT_BRACE;
        lexer_consume_next_character (parser_context_p);
        return;
      }
      case LIT_CHAR_DOT:
      {
        if (!(parser_context_p->source_p + 1 >= parser_context_p->source_end_p || lit_char_is_decimal_digit (parser_context_p->source_p[1])))
        {
          if ((ident_opts & ((uint32_t) ~(LEXER_OBJ_IDENT_OBJECT_PATTERN | LEXER_OBJ_IDENT_SET_FUNCTION_START)))
              || parser_context_p->source_p + 2 >= parser_context_p->source_end_p || parser_context_p->source_p[1] != LIT_CHAR_DOT
              || parser_context_p->source_p[2] != LIT_CHAR_DOT)
          {
            break;
          }

          parser_context_p->token.type = LEXER_THREE_DOTS;
          parser_context_p->token.flags &= (uint8_t) ~LEXER_NO_SKIP_SPACES;
          PARSER_PLUS_EQUAL_LC (parser_context_p->column, 3);
          parser_context_p->source_p += 3;
          return;
        }
        /* FALLTHRU */
      }
      default:
      {
        const uint8_t *char_p = parser_context_p->source_p;

        if (char_p[0] == LIT_CHAR_DOT)
        {
          char_p++;
        }

        if (char_p < parser_context_p->source_end_p && char_p[0] >= LIT_CHAR_0 && char_p[0] <= LIT_CHAR_9)
        {
          lexer_parse_number (parser_context_p);

          if (!(ident_opts & LEXER_OBJ_IDENT_CLASS_IDENTIFIER))
          {
            lexer_construct_number_object (parser_context_p, false, false);
          }
          return;
        }
        break;
      }
    }
  }

  if (create_literal_object)
  {
    if (ident_opts & LEXER_OBJ_IDENT_CLASS_IDENTIFIER)
    {
      return;
    }

    if (ident_opts & LEXER_OBJ_IDENT_CLASS_PRIVATE)
    {
      parser_resolve_private_identifier (parser_context_p);
      return;
    }

    lexer_construct_literal_object (parser_context_p, &parser_context_p->token.lit_location, LEXER_STRING_LITERAL);
    return;
  }

  parser_raise_error (parser_context_p, PARSER_ERR_PROPERTY_IDENTIFIER_EXPECTED);
} /* lexer_expect_object_literal_id */

/**
 * Read next token without checking keywords
 *
 * @return true if the next literal is identifier, false otherwise
 */
bool
lexer_scan_identifier (parser_context_t *parser_context_p, /**< context */
                       lexer_parse_options_t opts) /**< identifier parse options */
{
  lexer_skip_spaces (parser_context_p);
  parser_context_p->token.keyword_type = LEXER_EOS;
  parser_context_p->token.line = parser_context_p->line;
  parser_context_p->token.column = parser_context_p->column;

  if (parser_context_p->source_p < parser_context_p->source_end_p && lexer_parse_identifier (parser_context_p, opts))
  {
    return true;
  }

  parser_context_p->token.flags |= LEXER_NO_SKIP_SPACES;
  lexer_next_token (parser_context_p);
  return false;
} /* lexer_scan_identifier */

/**
 * Check whether the identifier is a modifier in a property definition.
 */
void
lexer_check_property_modifier (parser_context_t *parser_context_p) /**< context */
{
  JJS_ASSERT (!(parser_context_p->token.flags & LEXER_NO_SKIP_SPACES));
  JJS_ASSERT (parser_context_p->token.type == LEXER_LITERAL && parser_context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

  lexer_skip_spaces (parser_context_p);
  parser_context_p->token.flags = (uint8_t) (parser_context_p->token.flags | LEXER_NO_SKIP_SPACES);

  if (parser_context_p->source_p >= parser_context_p->source_end_p || parser_context_p->source_p[0] == LIT_CHAR_COMMA
      || parser_context_p->source_p[0] == LIT_CHAR_RIGHT_BRACE || parser_context_p->source_p[0] == LIT_CHAR_LEFT_PAREN
      || parser_context_p->source_p[0] == LIT_CHAR_EQUALS || parser_context_p->source_p[0] == LIT_CHAR_COLON)
  {
    return;
  }

  if (lexer_compare_literal_to_string (parser_context_p, "get", 3))
  {
    parser_context_p->token.type = LEXER_PROPERTY_GETTER;
    return;
  }

  if (lexer_compare_literal_to_string (parser_context_p, "set", 3))
  {
    parser_context_p->token.type = LEXER_PROPERTY_SETTER;
    return;
  }

  if (lexer_compare_literal_to_string (parser_context_p, "async", 5))
  {
    parser_context_p->token.type = LEXER_KEYW_ASYNC;
    return;
  }
} /* lexer_check_property_modifier */

/**
 * Compares two identifiers.
 *
 * Note:
 *   Escape sequences are allowed in the left identifier, but not in the right
 *
 * @return true if the two identifiers are the same
 */
static bool
lexer_compare_identifier_to_chars (const uint8_t *left_p, /**< left identifier */
                                   const uint8_t *right_p, /**< right identifier string */
                                   size_t size) /**< byte size of the two identifiers */
{
  uint8_t utf8_buf[6];

  do
  {
    if (*left_p == *right_p)
    {
      left_p++;
      right_p++;
      size--;
      continue;
    }

    size_t escape_size;

    if (*left_p == LIT_CHAR_BACKSLASH)
    {
      left_p += 2;
      lit_code_point_t code_point = lexer_unchecked_hex_to_character (&left_p);

      escape_size = lit_code_point_to_cesu8_bytes (utf8_buf, code_point);
    }
    else if (*left_p >= LIT_UTF8_4_BYTE_MARKER)
    {
      lit_four_byte_utf8_char_to_cesu8 (utf8_buf, left_p);
      escape_size = 3 * 2;
      left_p += 4;
    }
    else
    {
      return false;
    }

    size -= escape_size;

    uint8_t *utf8_p = utf8_buf;
    do
    {
      if (*right_p++ != *utf8_p++)
      {
        return false;
      }
    } while (--escape_size > 0);
  } while (size > 0);

  return true;
} /* lexer_compare_identifier_to_chars */

/**
 * Compares an identifier to a string.
 *
 * Note:
 *   Escape sequences are allowed in the left identifier, but not in the right
 *
 * @return true if the identifier equals to string
 */
bool
lexer_compare_identifier_to_string (const lexer_lit_location_t *left_p, /**< left literal */
                                    const uint8_t *right_p, /**< right identifier string */
                                    size_t size) /**< byte size of the right identifier */
{
  if (left_p->length != size)
  {
    return false;
  }

  if (!(left_p->status_flags & LEXER_LIT_LOCATION_HAS_ESCAPE))
  {
    return memcmp (left_p->char_p, right_p, size) == 0;
  }

  return lexer_compare_identifier_to_chars (left_p->char_p, right_p, size);
} /* lexer_compare_identifier_to_string */

/**
 * Compares two identifiers.
 *
 * Note:
 *   Escape sequences are allowed in both identifiers
 *
 * @return true if the two identifiers are the same
 */
bool
lexer_compare_identifiers (parser_context_t *parser_context_p, /**< context */
                           const lexer_lit_location_t *left_p, /**< left literal */
                           const lexer_lit_location_t *right_p) /**< right literal */
{
  prop_length_t length = left_p->length;

  if (length != right_p->length)
  {
    return false;
  }

  if (!(left_p->status_flags & LEXER_LIT_LOCATION_HAS_ESCAPE))
  {
    return lexer_compare_identifier_to_chars (right_p->char_p, left_p->char_p, length);
  }

  if (!(right_p->status_flags & LEXER_LIT_LOCATION_HAS_ESCAPE))
  {
    return lexer_compare_identifier_to_chars (left_p->char_p, right_p->char_p, length);
  }

  if (length <= 64)
  {
    uint8_t buf_p[64];
    lexer_convert_ident_to_cesu8 (buf_p, left_p->char_p, length);
    return lexer_compare_identifier_to_chars (right_p->char_p, buf_p, length);
  }

  uint8_t *dynamic_buf_p = parser_malloc_scratch (parser_context_p, length);

  lexer_convert_ident_to_cesu8 (dynamic_buf_p, left_p->char_p, length);
  bool result = lexer_compare_identifier_to_chars (right_p->char_p, dynamic_buf_p, length);
  parser_free_scratch (parser_context_p, dynamic_buf_p, length);

  return result;
} /* lexer_compare_identifiers */

/**
 * Compares the current identifier in the context to the parameter identifier
 *
 * Note:
 *   Escape sequences are allowed.
 *
 * @return true if the input identifiers are the same
 */
bool
lexer_current_is_literal (parser_context_t *parser_context_p, /**< context */
                          const lexer_lit_location_t *right_ident_p) /**< identifier */
{
  JJS_ASSERT (parser_context_p->token.type == LEXER_LITERAL && parser_context_p->token.lit_location.type == LEXER_IDENT_LITERAL);

  lexer_lit_location_t *left_ident_p = &parser_context_p->token.lit_location;

  JJS_ASSERT (left_ident_p->length > 0 && right_ident_p->length > 0);

  if (left_ident_p->length != right_ident_p->length)
  {
    return false;
  }

  if (!((left_ident_p->status_flags | right_ident_p->status_flags) & LEXER_LIT_LOCATION_HAS_ESCAPE))
  {
    return memcmp (left_ident_p->char_p, right_ident_p->char_p, left_ident_p->length) == 0;
  }

  return lexer_compare_identifiers (parser_context_p, left_ident_p, right_ident_p);
} /* lexer_current_is_literal */

/**
 * Compares the current string token to "use strict".
 *
 * Note:
 *   Escape sequences are not allowed.
 *
 * @return true if "use strict" is found, false otherwise
 */
extern inline bool JJS_ATTR_ALWAYS_INLINE
lexer_string_is_use_strict (parser_context_t *parser_context_p) /**< context */
{
  JJS_ASSERT (parser_context_p->token.type == LEXER_LITERAL && parser_context_p->token.lit_location.type == LEXER_STRING_LITERAL);

  return (parser_context_p->token.lit_location.length == 10
          && !(parser_context_p->token.lit_location.status_flags & LEXER_LIT_LOCATION_HAS_ESCAPE)
          && memcmp (parser_context_p->token.lit_location.char_p, "use strict", 10) == 0);
} /* lexer_string_is_use_strict */

/**
 * Checks whether the string before the current token is a directive or a string literal.
 *
 * @return true if the string is a directive, false otherwise
 */
extern inline bool JJS_ATTR_ALWAYS_INLINE
lexer_string_is_directive (parser_context_t *parser_context_p) /**< context */
{
  return (parser_context_p->token.type == LEXER_SEMICOLON || parser_context_p->token.type == LEXER_RIGHT_BRACE
          || parser_context_p->token.type == LEXER_EOS
          || ((parser_context_p->token.flags & LEXER_WAS_NEWLINE) && !LEXER_IS_BINARY_OP_TOKEN (parser_context_p->token.type)
              && parser_context_p->token.type != LEXER_LEFT_PAREN && parser_context_p->token.type != LEXER_LEFT_SQUARE
              && parser_context_p->token.type != LEXER_DOT));
} /* lexer_string_is_directive */

/**
 * Compares the current token to an expected identifier.
 *
 * Note:
 *   Escape sequences are not allowed.
 *
 * @return true if they are the same, false otherwise
 */
extern inline bool JJS_ATTR_ALWAYS_INLINE
lexer_token_is_identifier (parser_context_t *parser_context_p, /**< context */
                           const char *identifier_p, /**< identifier */
                           size_t identifier_length) /**< identifier length */
{
  /* Checking has_escape is unnecessary because memcmp will fail if escape sequences are present. */
  return (parser_context_p->token.type == LEXER_LITERAL && parser_context_p->token.lit_location.type == LEXER_IDENT_LITERAL
          && parser_context_p->token.lit_location.length == identifier_length
          && memcmp (parser_context_p->token.lit_location.char_p, identifier_p, identifier_length) == 0);
} /* lexer_token_is_identifier */

/**
 * Compares the current identifier token to "let".
 *
 * Note:
 *   Escape sequences are not allowed.
 *
 * @return true if "let" is found, false otherwise
 */
extern inline bool JJS_ATTR_ALWAYS_INLINE
lexer_token_is_let (parser_context_t *parser_context_p) /**< context */
{
  JJS_ASSERT (parser_context_p->token.type == LEXER_LITERAL);

  return (parser_context_p->token.keyword_type == LEXER_KEYW_LET
          && !(parser_context_p->token.lit_location.status_flags & LEXER_LIT_LOCATION_HAS_ESCAPE));
} /* lexer_token_is_let */

/**
 * Compares the current identifier token to "async".
 *
 * Note:
 *   Escape sequences are not allowed.
 *
 * @return true if "async" is found, false otherwise
 */
extern inline bool JJS_ATTR_ALWAYS_INLINE
lexer_token_is_async (parser_context_t *parser_context_p) /**< context */
{
  JJS_ASSERT (parser_context_p->token.type == LEXER_LITERAL || parser_context_p->token.type == LEXER_TEMPLATE_LITERAL);

  return (parser_context_p->token.keyword_type == LEXER_KEYW_ASYNC
          && !(parser_context_p->token.lit_location.status_flags & LEXER_LIT_LOCATION_HAS_ESCAPE));
} /* lexer_token_is_async */

/**
 * Compares the current identifier or string to an expected string.
 *
 * Note:
 *   Escape sequences are not allowed.
 *
 * @return true if they are the same, false otherwise
 */
extern inline bool JJS_ATTR_ALWAYS_INLINE
lexer_compare_literal_to_string (parser_context_t *parser_context_p, /**< context */
                                 const char *string_p, /**< string */
                                 size_t string_length) /**< string length */
{
  JJS_ASSERT (parser_context_p->token.type == LEXER_LITERAL
                && (parser_context_p->token.lit_location.type == LEXER_IDENT_LITERAL
                    || parser_context_p->token.lit_location.type == LEXER_STRING_LITERAL));

  /* Checking has_escape is unnecessary because memcmp will fail if escape sequences are present. */
  return (parser_context_p->token.lit_location.length == string_length
          && memcmp (parser_context_p->token.lit_location.char_p, string_p, string_length) == 0);
} /* lexer_compare_literal_to_string */

/**
 * Initialize line info to its default value
 */
void
lexer_init_line_info (parser_context_t *parser_context_p) /**< context */
{
  const jjs_parse_options_t *options_p = parser_context_p->options_p;

  if (options_p != NULL)
  {
    parser_context_p->line = options_p->start_line.has_value ? options_p->start_line.value : 1;
    parser_context_p->column = options_p->start_column.has_value ? options_p->start_column.value : 1;
  }
  else
  {
    parser_context_p->line = 1;
    parser_context_p->column = 1;
  }
} /* lexer_init_line_info */

/**
 * Convert binary lvalue token to binary token
 * e.g. += -> +
 *      ^= -> ^
 *
 * @return binary token
 */
uint8_t
lexer_convert_binary_lvalue_token_to_binary (uint8_t token) /**< binary lvalue token */
{
  JJS_ASSERT (LEXER_IS_BINARY_LVALUE_OP_TOKEN (token));
  JJS_ASSERT (token != LEXER_ASSIGN);

  if (token <= LEXER_ASSIGN_EXPONENTIATION)
  {
    return (uint8_t) (LEXER_ADD + (token - LEXER_ASSIGN_ADD));
  }

  if (token <= LEXER_ASSIGN_UNS_RIGHT_SHIFT)
  {
    return (uint8_t) (LEXER_LEFT_SHIFT + (token - LEXER_ASSIGN_LEFT_SHIFT));
  }

  switch (token)
  {
    case LEXER_ASSIGN_BIT_AND:
    {
      return LEXER_BIT_AND;
    }
    case LEXER_ASSIGN_BIT_OR:
    {
      return LEXER_BIT_OR;
    }
    default:
    {
      JJS_ASSERT (token == LEXER_ASSIGN_BIT_XOR);
      return LEXER_BIT_XOR;
    }
  }
} /* lexer_convert_binary_lvalue_token_to_binary */

/**
 * @}
 * @}
 * @}
 */

#endif /* JJS_PARSER */
