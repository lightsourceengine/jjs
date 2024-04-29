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

#include "ecma-helpers.h"

#include "js-parser-internal.h"

#if JJS_PARSER

#if JJS_LINE_INFO
#include "jcontext.h"
#endif /* JJS_LINE_INFO */

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_utils Utility
 * @{
 */

/**********************************************************************/
/* Emitting byte codes                                                */
/**********************************************************************/

/**
 * Append two bytes to the cbc stream.
 */
static void
parser_emit_two_bytes (parser_context_t *context_p, /**< context */
                       uint8_t first_byte, /**< first byte */
                       uint8_t second_byte) /**< second byte */
{
  uint32_t last_position = context_p->byte_code.last_position;

  if (last_position + 2 <= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    parser_mem_page_t *page_p = context_p->byte_code.last_p;

    page_p->bytes[last_position] = first_byte;
    page_p->bytes[last_position + 1] = second_byte;
    context_p->byte_code.last_position = last_position + 2;
  }
  else if (last_position >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    parser_mem_page_t *page_p;

    parser_cbc_stream_alloc_page (context_p, &context_p->byte_code);
    page_p = context_p->byte_code.last_p;
    page_p->bytes[0] = first_byte;
    page_p->bytes[1] = second_byte;
    context_p->byte_code.last_position = 2;
  }
  else
  {
    context_p->byte_code.last_p->bytes[PARSER_CBC_STREAM_PAGE_SIZE - 1] = first_byte;
    parser_cbc_stream_alloc_page (context_p, &context_p->byte_code);
    context_p->byte_code.last_p->bytes[0] = second_byte;
    context_p->byte_code.last_position = 1;
  }
} /* parser_emit_two_bytes */

/**
 * Append byte to the end of the current byte code stream.
 *
 * @param context_p parser context
 * @param byte byte
 */
#define PARSER_APPEND_TO_BYTE_CODE(context_p, byte)                        \
  if ((context_p)->byte_code.last_position >= PARSER_CBC_STREAM_PAGE_SIZE) \
  {                                                                        \
    parser_cbc_stream_alloc_page ((context_p), &(context_p)->byte_code);   \
  }                                                                        \
  (context_p)->byte_code.last_p->bytes[(context_p)->byte_code.last_position++] = (uint8_t) (byte)

#if JJS_PARSER_DUMP_BYTE_CODE

/**
 * Print literal corresponding to the current index
 */
static void
parser_print_literal (parser_context_t *context_p, /**< context */
                      uint16_t literal_index) /**< index of literal */
{
  parser_scope_stack_t *scope_stack_p = context_p->scope_stack_p;
  parser_scope_stack_t *scope_stack_end_p = scope_stack_p + context_p->scope_stack_top;
  bool in_scope_literal = false;

  while (scope_stack_p < scope_stack_end_p)
  {
    scope_stack_end_p--;

    if (scope_stack_end_p->map_from == PARSER_SCOPE_STACK_FUNC)
    {
      if (literal_index == scope_stack_end_p->map_to)
      {
        in_scope_literal = true;
        break;
      }
    }
    else if (literal_index == scanner_decode_map_to (scope_stack_end_p))
    {
      in_scope_literal = true;
      break;
    }
  }

  if (literal_index < PARSER_REGISTER_START)
  {
    JJS_DEBUG_MSG (in_scope_literal ? " IDX:%d->" : " idx:%d->", literal_index);
    lexer_literal_t *literal_p = PARSER_GET_LITERAL (literal_index);
    util_print_literal (literal_p);
    return;
  }

  if (!in_scope_literal)
  {
    JJS_DEBUG_MSG (" reg:%d", (int) (literal_index - PARSER_REGISTER_START));
    return;
  }

  JJS_DEBUG_MSG (" REG:%d->", (int) (literal_index - PARSER_REGISTER_START));

  lexer_literal_t *literal_p = PARSER_GET_LITERAL (scope_stack_end_p->map_from);
  util_print_literal (literal_p);
} /* parser_print_literal */

#endif /* JJS_PARSER_DUMP_BYTE_CODE */

/**
 * Append the current byte code to the stream
 */
void
parser_flush_cbc (parser_context_t *parser_context_p) /**< context */
{
  uint8_t flags;
  uint16_t last_opcode = parser_context_p->last_cbc_opcode;

  if (last_opcode == PARSER_CBC_UNAVAILABLE)
  {
    return;
  }

  parser_context_p->status_flags |= PARSER_NO_END_LABEL;

  if (PARSER_IS_BASIC_OPCODE (last_opcode))
  {
    cbc_opcode_t opcode = (cbc_opcode_t) last_opcode;

    JJS_ASSERT (opcode < CBC_END);
    flags = cbc_flags[opcode];

    PARSER_APPEND_TO_BYTE_CODE (parser_context_p, opcode);
    parser_context_p->byte_code_size++;
  }
  else
  {
    cbc_ext_opcode_t opcode = (cbc_ext_opcode_t) PARSER_GET_EXT_OPCODE (last_opcode);

    JJS_ASSERT (opcode < CBC_EXT_END);
    flags = cbc_ext_flags[opcode];
    parser_emit_two_bytes (parser_context_p, CBC_EXT_OPCODE, (uint8_t) opcode);
    parser_context_p->byte_code_size += 2;
  }

  JJS_ASSERT ((flags >> CBC_STACK_ADJUST_SHIFT) >= CBC_STACK_ADJUST_BASE
                || (CBC_STACK_ADJUST_BASE - (flags >> CBC_STACK_ADJUST_SHIFT)) <= parser_context_p->stack_depth);
  PARSER_PLUS_EQUAL_U16 (parser_context_p->stack_depth, CBC_STACK_ADJUST_VALUE (flags));

  if (flags & (CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2))
  {
    uint16_t literal_index = parser_context_p->last_cbc.literal_index;

    parser_emit_two_bytes (parser_context_p, (uint8_t) (literal_index & 0xff), (uint8_t) (literal_index >> 8));
    parser_context_p->byte_code_size += 2;
  }

  if (flags & CBC_HAS_LITERAL_ARG2)
  {
    uint16_t literal_index = parser_context_p->last_cbc.value;

    parser_emit_two_bytes (parser_context_p, (uint8_t) (literal_index & 0xff), (uint8_t) (literal_index >> 8));
    parser_context_p->byte_code_size += 2;

    if (!(flags & CBC_HAS_LITERAL_ARG))
    {
      literal_index = parser_context_p->last_cbc.third_literal_index;

      parser_emit_two_bytes (parser_context_p, (uint8_t) (literal_index & 0xff), (uint8_t) (literal_index >> 8));
      parser_context_p->byte_code_size += 2;
    }
  }

  if (flags & CBC_HAS_BYTE_ARG)
  {
    uint8_t byte_argument = (uint8_t) parser_context_p->last_cbc.value;

    JJS_ASSERT (parser_context_p->last_cbc.value <= CBC_MAXIMUM_BYTE_VALUE);

    if (flags & CBC_POP_STACK_BYTE_ARG)
    {
      JJS_ASSERT (parser_context_p->stack_depth >= byte_argument);
      PARSER_MINUS_EQUAL_U16 (parser_context_p->stack_depth, byte_argument);
    }

    PARSER_APPEND_TO_BYTE_CODE (parser_context_p, byte_argument);
    parser_context_p->byte_code_size++;
  }

#if JJS_PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes)
  {
    JJS_DEBUG_MSG ("  [%3d] %s",
                     (int) context_p->stack_depth,
                     PARSER_IS_BASIC_OPCODE (last_opcode) ? cbc_names[last_opcode]
                                                          : cbc_ext_names[PARSER_GET_EXT_OPCODE (last_opcode)]);

    if (flags & (CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2))
    {
      parser_print_literal (context_p, context_p->last_cbc.literal_index);
    }

    if (flags & CBC_HAS_LITERAL_ARG2)
    {
      parser_print_literal (context_p, context_p->last_cbc.value);

      if (!(flags & CBC_HAS_LITERAL_ARG))
      {
        parser_print_literal (context_p, context_p->last_cbc.third_literal_index);
      }
    }

    if (flags & CBC_HAS_BYTE_ARG)
    {
      if (last_opcode == CBC_PUSH_NUMBER_POS_BYTE || last_opcode == CBC_PUSH_LITERAL_PUSH_NUMBER_POS_BYTE)
      {
        JJS_DEBUG_MSG (" number:%d", (int) context_p->last_cbc.value + 1);
      }
      else if (last_opcode == CBC_PUSH_NUMBER_NEG_BYTE || last_opcode == CBC_PUSH_LITERAL_PUSH_NUMBER_NEG_BYTE)
      {
        JJS_DEBUG_MSG (" number:%d", -((int) context_p->last_cbc.value + 1));
      }
      else
      {
        JJS_DEBUG_MSG (" byte_arg:%d", (int) context_p->last_cbc.value);
      }
    }

    JJS_DEBUG_MSG ("\n");
  }
#endif /* JJS_PARSER_DUMP_BYTE_CODE */

  if (parser_context_p->stack_depth > parser_context_p->stack_limit)
  {
    parser_context_p->stack_limit = parser_context_p->stack_depth;
    if (parser_context_p->stack_limit > PARSER_MAXIMUM_STACK_LIMIT)
    {
      parser_raise_error (parser_context_p, PARSER_ERR_STACK_LIMIT_REACHED);
    }
  }

  parser_context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
} /* parser_flush_cbc */

/**
 * Append a byte code
 */
void
parser_emit_cbc (parser_context_t *parser_context_p, /**< context */
                 uint16_t opcode) /**< opcode */
{
  JJS_ASSERT (PARSER_ARGS_EQ (opcode, 0));

  if (parser_context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (parser_context_p);
  }

  parser_context_p->last_cbc_opcode = opcode;
} /* parser_emit_cbc */

/**
 * Append a byte code with a literal argument
 */
void
parser_emit_cbc_literal (parser_context_t *parser_context_p, /**< context */
                         uint16_t opcode, /**< opcode */
                         uint16_t literal_index) /**< literal index */
{
  JJS_ASSERT (PARSER_ARGS_EQ (opcode, CBC_HAS_LITERAL_ARG));

  if (parser_context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (parser_context_p);
  }

  parser_context_p->last_cbc_opcode = opcode;
  parser_context_p->last_cbc.literal_index = literal_index;
  parser_context_p->last_cbc.literal_type = LEXER_UNUSED_LITERAL;
  parser_context_p->last_cbc.literal_keyword_type = LEXER_EOS;
} /* parser_emit_cbc_literal */

/**
 * Append a byte code with a literal and value argument
 */
void
parser_emit_cbc_literal_value (parser_context_t *parser_context_p, /**< context */
                               uint16_t opcode, /**< opcode */
                               uint16_t literal_index, /**< literal index */
                               uint16_t value) /**< value */
{
  JJS_ASSERT (PARSER_ARGS_EQ (opcode, CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2));

  if (parser_context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (parser_context_p);
  }

  parser_context_p->last_cbc_opcode = opcode;
  parser_context_p->last_cbc.literal_index = literal_index;
  parser_context_p->last_cbc.literal_type = LEXER_UNUSED_LITERAL;
  parser_context_p->last_cbc.literal_keyword_type = LEXER_EOS;
  parser_context_p->last_cbc.value = value;
} /* parser_emit_cbc_literal_value */

/**
 * Append a byte code with the current literal argument
 */
void
parser_emit_cbc_literal_from_token (parser_context_t *parser_context_p, /**< context */
                                    uint16_t opcode) /**< opcode */
{
  JJS_ASSERT (PARSER_ARGS_EQ (opcode, CBC_HAS_LITERAL_ARG));

  if (parser_context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (parser_context_p);
  }

  parser_context_p->last_cbc_opcode = opcode;
  parser_context_p->last_cbc.literal_index = parser_context_p->lit_object.index;
  parser_context_p->last_cbc.literal_type = parser_context_p->token.lit_location.type;
  parser_context_p->last_cbc.literal_keyword_type = parser_context_p->token.keyword_type;
} /* parser_emit_cbc_literal_from_token */

/**
 * Append a byte code with a call argument
 */
void
parser_emit_cbc_call (parser_context_t *parser_context_p, /**< context */
                      uint16_t opcode, /**< opcode */
                      size_t call_arguments) /**< number of arguments */
{
  JJS_ASSERT (PARSER_ARGS_EQ (opcode, CBC_HAS_BYTE_ARG));
  JJS_ASSERT (call_arguments <= CBC_MAXIMUM_BYTE_VALUE);

  if (parser_context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (parser_context_p);
  }

  parser_context_p->last_cbc_opcode = opcode;
  parser_context_p->last_cbc.value = (uint16_t) call_arguments;
} /* parser_emit_cbc_call */

/**
 * Append a push number 1/2 byte code
 */
void
parser_emit_cbc_push_number (parser_context_t *parser_context_p, /**< context */
                             bool is_negative_number) /**< sign is negative */
{
  uint16_t value = parser_context_p->lit_object.index;
  uint16_t lit_value = PARSER_INVALID_LITERAL_INDEX;

  if (parser_context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    if (parser_context_p->last_cbc_opcode == CBC_PUSH_LITERAL)
    {
      lit_value = parser_context_p->last_cbc.literal_index;
    }
    else
    {
      if (parser_context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
      {
        parser_context_p->last_cbc_opcode = CBC_PUSH_LITERAL;
        lit_value = parser_context_p->last_cbc.value;
      }
      else if (parser_context_p->last_cbc_opcode == CBC_PUSH_THREE_LITERALS)
      {
        parser_context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
        lit_value = parser_context_p->last_cbc.third_literal_index;
      }

      parser_flush_cbc (parser_context_p);
    }
  }

  if (value == 0)
  {
    if (lit_value == PARSER_INVALID_LITERAL_INDEX)
    {
      parser_context_p->last_cbc_opcode = CBC_PUSH_NUMBER_0;
      return;
    }

    parser_context_p->last_cbc_opcode = CBC_PUSH_LITERAL_PUSH_NUMBER_0;
    parser_context_p->last_cbc.literal_index = lit_value;
    return;
  }

  uint16_t opcode;

  if (lit_value == PARSER_INVALID_LITERAL_INDEX)
  {
    opcode = (is_negative_number ? CBC_PUSH_NUMBER_NEG_BYTE : CBC_PUSH_NUMBER_POS_BYTE);

    JJS_ASSERT (CBC_STACK_ADJUST_VALUE (PARSER_GET_FLAGS (opcode)) == 1);
  }
  else
  {
    opcode = (is_negative_number ? CBC_PUSH_LITERAL_PUSH_NUMBER_NEG_BYTE : CBC_PUSH_LITERAL_PUSH_NUMBER_POS_BYTE);
    JJS_ASSERT (CBC_STACK_ADJUST_VALUE (PARSER_GET_FLAGS (opcode)) == 2);

    parser_context_p->last_cbc.literal_index = lit_value;
  }

  JJS_ASSERT (value > 0 && value <= CBC_PUSH_NUMBER_BYTE_RANGE_END);

  parser_context_p->last_cbc_opcode = opcode;
  parser_context_p->last_cbc.value = (uint16_t) (value - 1);
} /* parser_emit_cbc_push_number */

/**
 * Append a byte code with a branch argument
 */
void
parser_emit_cbc_forward_branch (parser_context_t *parser_context_p, /**< context */
                                uint16_t opcode, /**< opcode */
                                parser_branch_t *branch_p) /**< branch result */
{
  uint8_t flags;
  uint32_t extra_byte_code_increase;

  if (parser_context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (parser_context_p);
  }

  parser_context_p->status_flags |= PARSER_NO_END_LABEL;

  if (PARSER_IS_BASIC_OPCODE (opcode))
  {
    JJS_ASSERT (opcode < CBC_END);
    flags = cbc_flags[opcode];
    extra_byte_code_increase = 0;
  }
  else
  {
    PARSER_APPEND_TO_BYTE_CODE (parser_context_p, CBC_EXT_OPCODE);
    opcode = (uint16_t) PARSER_GET_EXT_OPCODE (opcode);

    JJS_ASSERT (opcode < CBC_EXT_END);
    flags = cbc_ext_flags[opcode];
    extra_byte_code_increase = 1;
  }

  JJS_ASSERT (flags & CBC_HAS_BRANCH_ARG);
  JJS_ASSERT (CBC_BRANCH_IS_FORWARD (flags));
  JJS_ASSERT (CBC_BRANCH_OFFSET_LENGTH (opcode) == 1);

  /* Branch opcodes never push anything onto the stack. */
  JJS_ASSERT ((flags >> CBC_STACK_ADJUST_SHIFT) >= CBC_STACK_ADJUST_BASE
                || (CBC_STACK_ADJUST_BASE - (flags >> CBC_STACK_ADJUST_SHIFT)) <= parser_context_p->stack_depth);
  PARSER_PLUS_EQUAL_U16 (parser_context_p->stack_depth, CBC_STACK_ADJUST_VALUE (flags));

#if JJS_PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes)
  {
    JJS_DEBUG_MSG ("  [%3d] %s\n",
                     (int) context_p->stack_depth,
                     extra_byte_code_increase == 0 ? cbc_names[opcode] : cbc_ext_names[opcode]);
  }
#endif /* JJS_PARSER_DUMP_BYTE_CODE */

  PARSER_PLUS_EQUAL_U16 (opcode, PARSER_MAX_BRANCH_LENGTH - 1);

  parser_emit_two_bytes (parser_context_p, (uint8_t) opcode, 0);
  branch_p->page_p = parser_context_p->byte_code.last_p;
  branch_p->offset = (parser_context_p->byte_code.last_position - 1) | (parser_context_p->byte_code_size << 8);

  parser_context_p->byte_code_size += extra_byte_code_increase;

#if PARSER_MAXIMUM_CODE_SIZE <= UINT16_MAX
  PARSER_APPEND_TO_BYTE_CODE (context_p, 0);
#else /* PARSER_MAXIMUM_CODE_SIZE > UINT16_MAX */
  parser_emit_two_bytes (parser_context_p, 0, 0);
#endif /* PARSER_MAXIMUM_CODE_SIZE <= UINT16_MAX */

  parser_context_p->byte_code_size += PARSER_MAX_BRANCH_LENGTH + 1;

  if (parser_context_p->stack_depth > parser_context_p->stack_limit)
  {
    parser_context_p->stack_limit = parser_context_p->stack_depth;
    if (parser_context_p->stack_limit > PARSER_MAXIMUM_STACK_LIMIT)
    {
      parser_raise_error (parser_context_p, PARSER_ERR_STACK_LIMIT_REACHED);
    }
  }
} /* parser_emit_cbc_forward_branch */

/**
 * Append a branch byte code and create an item.
 *
 * @return newly created parser branch node
 */
parser_branch_node_t *
parser_emit_cbc_forward_branch_item (parser_context_t *parser_context_p, /**< context */
                                     uint16_t opcode, /**< opcode */
                                     parser_branch_node_t *next_p) /**< next branch */
{
  parser_branch_t branch;
  parser_branch_node_t *new_item;

  /* Since byte code insertion may throw an out-of-memory error,
   * the branch is constructed locally, and copied later. */
  parser_emit_cbc_forward_branch (parser_context_p, opcode, &branch);

  new_item = (parser_branch_node_t *) parser_malloc (parser_context_p, sizeof (parser_branch_node_t));
  new_item->branch = branch;
  new_item->next_p = next_p;
  return new_item;
} /* parser_emit_cbc_forward_branch_item */

/**
 * Append a byte code with a branch argument
 */
void
parser_emit_cbc_backward_branch (parser_context_t *parser_context_p, /**< context */
                                 uint16_t opcode, /**< opcode */
                                 uint32_t offset) /**< destination offset */
{
  uint8_t flags;
#if JJS_PARSER_DUMP_BYTE_CODE
  const char *name;
#endif /* JJS_PARSER_DUMP_BYTE_CODE */

  if (parser_context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (parser_context_p);
  }

  parser_context_p->status_flags |= PARSER_NO_END_LABEL;
  offset = parser_context_p->byte_code_size - offset;

  if (PARSER_IS_BASIC_OPCODE (opcode))
  {
    JJS_ASSERT (opcode < CBC_END);
    flags = cbc_flags[opcode];

#if JJS_PARSER_DUMP_BYTE_CODE
    name = cbc_names[opcode];
#endif /* JJS_PARSER_DUMP_BYTE_CODE */
  }
  else
  {
    PARSER_APPEND_TO_BYTE_CODE (parser_context_p, CBC_EXT_OPCODE);
    opcode = (uint16_t) PARSER_GET_EXT_OPCODE (opcode);

    JJS_ASSERT (opcode < CBC_EXT_END);
    flags = cbc_ext_flags[opcode];
    parser_context_p->byte_code_size++;

#if JJS_PARSER_DUMP_BYTE_CODE
    name = cbc_ext_names[opcode];
#endif /* JJS_PARSER_DUMP_BYTE_CODE */
  }

  JJS_ASSERT (flags & CBC_HAS_BRANCH_ARG);
  JJS_ASSERT (CBC_BRANCH_IS_BACKWARD (flags));
  JJS_ASSERT (CBC_BRANCH_OFFSET_LENGTH (opcode) == 1);
  JJS_ASSERT (offset <= parser_context_p->byte_code_size);

  /* Branch opcodes never push anything onto the stack. */
  JJS_ASSERT ((flags >> CBC_STACK_ADJUST_SHIFT) >= CBC_STACK_ADJUST_BASE
                || (CBC_STACK_ADJUST_BASE - (flags >> CBC_STACK_ADJUST_SHIFT)) <= parser_context_p->stack_depth);
  PARSER_PLUS_EQUAL_U16 (parser_context_p->stack_depth, CBC_STACK_ADJUST_VALUE (flags));

#if JJS_PARSER_DUMP_BYTE_CODE
  if (context_p->is_show_opcodes)
  {
    JJS_DEBUG_MSG ("  [%3d] %s\n", (int) context_p->stack_depth, name);
  }
#endif /* JJS_PARSER_DUMP_BYTE_CODE */

  parser_context_p->byte_code_size += 2;
#if PARSER_MAXIMUM_CODE_SIZE > UINT16_MAX
  if (offset > UINT16_MAX)
  {
    opcode++;
    parser_context_p->byte_code_size++;
  }
#endif /* PARSER_MAXIMUM_CODE_SIZE > UINT16_MAX */

  if (offset > UINT8_MAX)
  {
    opcode++;
    parser_context_p->byte_code_size++;
  }

  PARSER_APPEND_TO_BYTE_CODE (parser_context_p, (uint8_t) opcode);

#if PARSER_MAXIMUM_CODE_SIZE > UINT16_MAX
  if (offset > UINT16_MAX)
  {
    PARSER_APPEND_TO_BYTE_CODE (parser_context_p, offset >> 16);
  }
#endif /* PARSER_MAXIMUM_CODE_SIZE > UINT16_MAX */

  if (offset > UINT8_MAX)
  {
    PARSER_APPEND_TO_BYTE_CODE (parser_context_p, (offset >> 8) & 0xff);
  }

  PARSER_APPEND_TO_BYTE_CODE (parser_context_p, offset & 0xff);
} /* parser_emit_cbc_backward_branch */

#undef PARSER_CHECK_LAST_POSITION
#undef PARSER_APPEND_TO_BYTE_CODE

/**
 * Helper function for parser.
 *
 * @return a new string based on encode.
 */
ecma_string_t *
parser_new_ecma_string_from_literal (lexer_literal_t *literal_p) /**< literal */
{
  JJS_ASSERT (literal_p != NULL);

  ecma_string_t *new_string = NULL;

  if (literal_p->status_flags & LEXER_FLAG_ASCII)
  {
    new_string = ecma_new_ecma_string_from_ascii (literal_p->u.char_p, literal_p->prop.length);
  }
  else
  {
    new_string = ecma_new_ecma_string_from_utf8 (literal_p->u.char_p, literal_p->prop.length);
  }

  return new_string;
} /* parser_new_ecma_string_from_literal */

/**
 * Set a branch to the current byte code position
 */
void
parser_set_branch_to_current_position (parser_context_t *parser_context_p, /**< context */
                                       parser_branch_t *branch_p) /**< branch result */
{
  uint32_t delta;
  size_t offset;
  parser_mem_page_t *page_p = branch_p->page_p;

  if (parser_context_p->last_cbc_opcode != PARSER_CBC_UNAVAILABLE)
  {
    parser_flush_cbc (parser_context_p);
  }

  parser_context_p->status_flags &= (uint32_t) ~PARSER_NO_END_LABEL;

  JJS_ASSERT (parser_context_p->byte_code_size > (branch_p->offset >> 8));

  delta = parser_context_p->byte_code_size - (branch_p->offset >> 8);
  offset = (branch_p->offset & CBC_LOWER_SEVEN_BIT_MASK);

  JJS_ASSERT (delta <= PARSER_MAXIMUM_CODE_SIZE);

#if PARSER_MAXIMUM_CODE_SIZE <= UINT16_MAX
  page_p->bytes[offset++] = (uint8_t) (delta >> 8);
  if (offset >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    page_p = page_p->next_p;
    offset = 0;
  }
#else /* PARSER_MAXIMUM_CODE_SIZE > UINT16_MAX */
  page_p->bytes[offset++] = (uint8_t) (delta >> 16);
  if (offset >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    page_p = page_p->next_p;
    offset = 0;
  }
  page_p->bytes[offset++] = (uint8_t) ((delta >> 8) & 0xff);
  if (offset >= PARSER_CBC_STREAM_PAGE_SIZE)
  {
    page_p = page_p->next_p;
    offset = 0;
  }
#endif /* PARSER_MAXIMUM_CODE_SIZE <= UINT16_MAX */
  page_p->bytes[offset] = delta & 0xff;
} /* parser_set_branch_to_current_position */

/**
 * Set breaks to the current byte code position
 */
void
parser_set_breaks_to_current_position (parser_context_t *parser_context_p, /**< context */
                                       parser_branch_node_t *current_p) /**< branch list */
{
  while (current_p != NULL)
  {
    parser_branch_node_t *next_p = current_p->next_p;

    if (!(current_p->branch.offset & CBC_HIGHEST_BIT_MASK))
    {
      parser_set_branch_to_current_position (parser_context_p, &current_p->branch);
    }
    parser_free (current_p, sizeof (parser_branch_node_t));
    current_p = next_p;
  }
} /* parser_set_breaks_to_current_position */

/**
 * Set continues to the current byte code position
 */
void
parser_set_continues_to_current_position (parser_context_t *parser_context_p, /**< context */
                                          parser_branch_node_t *current_p) /**< branch list */
{
  while (current_p != NULL)
  {
    if (current_p->branch.offset & CBC_HIGHEST_BIT_MASK)
    {
      parser_set_branch_to_current_position (parser_context_p, &current_p->branch);
    }
    current_p = current_p->next_p;
  }
} /* parser_set_continues_to_current_position */

/**
 * Return the size of internal record corresponding to a class field
 *
 * @return internal record size
 */
static size_t
parser_get_class_field_info_size (uint8_t class_field_type) /**< class field type */
{
  if (class_field_type & PARSER_CLASS_FIELD_INITIALIZED)
  {
    return sizeof (scanner_range_t) + 1;
  }

  if (class_field_type & PARSER_CLASS_FIELD_NORMAL)
  {
    return sizeof (scanner_location_t) + 1;
  }

  return 1;
} /* parser_get_class_field_info_size */

/**
 * Reverse the field list of a class
 */
void
parser_reverse_class_fields (parser_context_t *parser_context_p, /**< context */
                             size_t fields_size) /**< size of consumed memory */
{
  uint8_t *data_p = (uint8_t *) parser_malloc (parser_context_p, fields_size);
  uint8_t *data_end_p = data_p + fields_size;
  uint8_t *current_p = data_p;
  bool has_fields = false;
  parser_stack_iterator_t iterator;

  JJS_ASSERT (!(parser_context_p->stack_top_uint8 & PARSER_CLASS_FIELD_END));

  parser_stack_iterator_init (parser_context_p, &iterator);

  do
  {
    uint8_t class_field_type = parser_stack_iterator_read_uint8 (&iterator);
    size_t info_size = parser_get_class_field_info_size (class_field_type);

    parser_stack_iterator_read (&iterator, current_p, info_size);
    parser_stack_iterator_skip (&iterator, info_size);
    current_p += info_size;

    if (!(class_field_type & PARSER_CLASS_FIELD_STATIC))
    {
      has_fields = true;
      parser_context_p->stack_top_uint8 = class_field_type;
    }
  } while (current_p < data_end_p);

  parser_stack_iterator_init (parser_context_p, &iterator);
  current_p = data_end_p;

  bool has_static_fields = false;

  if (has_fields)
  {
    do
    {
      uint8_t class_field_type = current_p[-1];

      size_t info_size = parser_get_class_field_info_size (class_field_type);

      if (!(class_field_type & PARSER_CLASS_FIELD_STATIC))
      {
        current_p -= info_size;
        parser_stack_iterator_write (&iterator, current_p, info_size);
        parser_stack_iterator_skip (&iterator, info_size);
        continue;
      }

      if (!has_static_fields)
      {
        has_static_fields = true;
        current_p[-1] |= PARSER_CLASS_FIELD_END;
      }
      current_p -= info_size;
    } while (current_p > data_p);
  }
  else
  {
    /* All class fields are static. */
    has_static_fields = true;
    JJS_ASSERT (data_end_p[-1] & PARSER_CLASS_FIELD_STATIC);
    parser_context_p->stack_top_uint8 = data_end_p[-1];
  }

  if (has_static_fields)
  {
    current_p = data_end_p;

    do
    {
      uint8_t class_field_type = current_p[-1];

      size_t info_size = parser_get_class_field_info_size (class_field_type);
      current_p -= info_size;

      if (class_field_type & PARSER_CLASS_FIELD_STATIC)
      {
        parser_stack_iterator_write (&iterator, current_p, info_size);
        parser_stack_iterator_skip (&iterator, info_size);
      }
    } while (current_p > data_p);
  }

  parser_free (data_p, fields_size);
} /* parser_reverse_class_fields */

/**
 * @}
 * @}
 * @}
 */

#endif /* JJS_PARSER */
