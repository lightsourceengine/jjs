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

#include "js-parser-internal.h"

#if JJS_MODULE_SYSTEM
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-module.h"

#include "jcontext.h"

/**
 * Description of "*default*" literal string.
 */
const lexer_lit_location_t lexer_default_literal = { (const uint8_t *) "*default*",
                                                     9,
                                                     LEXER_IDENT_LITERAL,
                                                     LEXER_LIT_LOCATION_IS_ASCII };

const lexer_lit_location_t lexer_export_as_default_literal = { (const uint8_t *) "default",
                                                     7,
                                                     LEXER_IDENT_LITERAL,
                                                     LEXER_LIT_LOCATION_IS_ASCII };

/**
 * Check for duplicated imported binding names.
 *
 * @return true - if the given name is a duplicate
 *         false - otherwise
 */
bool
parser_module_check_duplicate_import (parser_context_t *parser_context_p, /**< parser context */
                                      ecma_string_t *local_name_p) /**< newly imported name */
{
  ecma_module_names_t *module_names_p = parser_context_p->module_names_p;

  while (module_names_p != NULL)
  {
    if (ecma_compare_ecma_strings (module_names_p->local_name_p, local_name_p))
    {
      return true;
    }

    module_names_p = module_names_p->next_p;
  }

  ecma_module_node_t *module_node_p = parser_context_p->context_p->module_current_p->imports_p;

  while (module_node_p != NULL)
  {
    module_names_p = module_node_p->module_names_p;

    while (module_names_p != NULL)
    {
      if (ecma_compare_ecma_strings (module_names_p->local_name_p, local_name_p))
      {
        return true;
      }

      module_names_p = module_names_p->next_p;
    }

    module_node_p = module_node_p->next_p;
  }

  return false;
} /* parser_module_check_duplicate_import */

/**
 * Append an identifier to the exported bindings.
 */
void
parser_module_append_export_name (parser_context_t *parser_context_p) /**< parser context */
{
  if (!(parser_context_p->status_flags & PARSER_MODULE_STORE_IDENT))
  {
    return;
  }

  parser_context_p->module_identifier_lit_p = parser_context_p->lit_object.literal_p;
  ecma_string_t *name_p = parser_new_ecma_string_from_literal (parser_context_p, parser_context_p->lit_object.literal_p);
  ecma_context_t *context_p = parser_context_p->context_p;

  if (parser_module_check_duplicate_export (parser_context_p, name_p))
  {
    ecma_deref_ecma_string (context_p, name_p);
    parser_raise_error (parser_context_p, PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER);
  }

  parser_module_add_names_to_node (parser_context_p, name_p, name_p);
  ecma_deref_ecma_string (context_p, name_p);
} /* parser_module_append_export_name */

/**
 * Check for duplicated exported bindings.
 * @return - true - if the exported name is a duplicate
 *           false - otherwise
 */
bool
parser_module_check_duplicate_export (parser_context_t *parser_context_p, /**< parser context */
                                      ecma_string_t *export_name_p) /**< exported identifier */
{
  /* We have to check in the currently constructed node, as well as all of the already added nodes. */
  ecma_module_names_t *current_names_p = parser_context_p->module_names_p;

  while (current_names_p != NULL)
  {
    if (ecma_compare_ecma_strings (current_names_p->imex_name_p, export_name_p))
    {
      return true;
    }
    current_names_p = current_names_p->next_p;
  }

  ecma_context_t *context_p = parser_context_p->context_p;
  ecma_module_names_t *name_p = context_p->module_current_p->local_exports_p;

  while (name_p != NULL)
  {
    if (ecma_compare_ecma_strings (name_p->imex_name_p, export_name_p))
    {
      return true;
    }

    name_p = name_p->next_p;
  }

  ecma_module_node_t *export_node_p = context_p->module_current_p->indirect_exports_p;

  while (export_node_p != NULL)
  {
    name_p = export_node_p->module_names_p;

    while (name_p != NULL)
    {
      if (ecma_compare_ecma_strings (name_p->imex_name_p, export_name_p))
      {
        return true;
      }

      name_p = name_p->next_p;
    }

    export_node_p = export_node_p->next_p;
  }

  /* Star exports don't have any names associated with them, so no need to check those. */
  return false;
} /* parser_module_check_duplicate_export */

/**
 * Add module names to current module node.
 */
void
parser_module_add_names_to_node (parser_context_t *parser_context_p, /**< parser context */
                                 ecma_string_t *imex_name_p, /**< import/export name */
                                 ecma_string_t *local_name_p) /**< local name */
{
  ecma_module_names_t *new_name_p = (ecma_module_names_t *) parser_malloc_vm (parser_context_p, sizeof (ecma_module_names_t));

  new_name_p->next_p = parser_context_p->module_names_p;
  parser_context_p->module_names_p = new_name_p;

  JJS_ASSERT (imex_name_p != NULL);
  ecma_ref_ecma_string (imex_name_p);
  new_name_p->imex_name_p = imex_name_p;

  JJS_ASSERT (local_name_p != NULL);
  ecma_ref_ecma_string (local_name_p);
  new_name_p->local_name_p = local_name_p;
} /* parser_module_add_names_to_node */

/**
 * Parse an ExportClause.
 */
void
parser_module_parse_export_clause (parser_context_t *parser_context_p) /**< parser context */
{
  bool has_module_specifier = false;
  ecma_context_t *context_p = parser_context_p->context_p;

  if (parser_context_p->source_p == parser_context_p->next_scanner_info_p->source_p)
  {
    has_module_specifier = true;
    JJS_ASSERT (parser_context_p->next_scanner_info_p->type == SCANNER_TYPE_EXPORT_MODULE_SPECIFIER);
    scanner_release_next (parser_context_p, sizeof (scanner_info_t));
  }

  JJS_ASSERT (parser_context_p->token.type == LEXER_LEFT_BRACE);
  lexer_next_token (parser_context_p);

  while (true)
  {
    if (parser_context_p->token.type == LEXER_RIGHT_BRACE)
    {
      lexer_next_token (parser_context_p);
      break;
    }

    /* 15.2.3.1 The referenced binding cannot be a reserved word. */
    if (parser_context_p->token.type != LEXER_LITERAL || parser_context_p->token.lit_location.type != LEXER_IDENT_LITERAL
        || parser_context_p->token.keyword_type >= LEXER_FIRST_FUTURE_STRICT_RESERVED_WORD)
    {
      parser_raise_error (parser_context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
    }

    ecma_string_t *export_name_p = NULL;
    ecma_string_t *local_name_p = NULL;

    lexer_construct_literal_object (parser_context_p, &parser_context_p->token.lit_location, LEXER_NEW_IDENT_LITERAL);

    if (!has_module_specifier && !scanner_literal_exists (parser_context_p, parser_context_p->lit_object.index))
    {
      parser_raise_error (parser_context_p, PARSER_ERR_EXPORT_NOT_DEFINED);
    }

    uint16_t local_name_index = parser_context_p->lit_object.index;
    uint16_t export_name_index = PARSER_MAXIMUM_NUMBER_OF_LITERALS;

    lexer_next_token (parser_context_p);
    if (lexer_token_is_identifier (parser_context_p, "as", 2))
    {
      lexer_next_token (parser_context_p);

      /*
       * XXX
       *
       * The ECMA spec says "export as IdentifierName", where IdentifierName excludes reserved words.
       * However, V8 allows export as default. Tools, such as babel, terser, sucrase, etc, all will
       * compact export statements using "as default". With the tools, there is no way to turn off or
       * change the compacting of exports (due to the as default being ubiquitous). My feeling is that
       * "as default" is not to spec, as it is a reserved word. However, in the context of exports,
       * default is also valid identifier. Either way, if the rest of the eco system, tools and node,
       * honor as default, JJS should as well.
       */
      if (parser_context_p->token.type == LEXER_KEYW_DEFAULT) {
        // XXX: it would be better to replace the keyword token with a literal token
        lexer_construct_literal_object (parser_context_p, &lexer_export_as_default_literal, lexer_export_as_default_literal.type);
      }
      else if (parser_context_p->token.type != LEXER_LITERAL || parser_context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
      {
        parser_raise_error (parser_context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
      }
      else
      {
        lexer_construct_literal_object (parser_context_p, &parser_context_p->token.lit_location, LEXER_NEW_IDENT_LITERAL);
      }

      export_name_index = parser_context_p->lit_object.index;

      lexer_next_token (parser_context_p);
    }

    local_name_p = parser_new_ecma_string_from_literal (parser_context_p, PARSER_GET_LITERAL (local_name_index));

    if (export_name_index != PARSER_MAXIMUM_NUMBER_OF_LITERALS)
    {
      export_name_p = parser_new_ecma_string_from_literal (parser_context_p, PARSER_GET_LITERAL (export_name_index));
    }
    else
    {
      export_name_p = local_name_p;
      ecma_ref_ecma_string (local_name_p);
    }

    if (parser_module_check_duplicate_export (parser_context_p, export_name_p))
    {
      ecma_deref_ecma_string (context_p, local_name_p);
      ecma_deref_ecma_string (context_p, export_name_p);
      parser_raise_error (parser_context_p, PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER);
    }

    parser_module_add_names_to_node (parser_context_p, export_name_p, local_name_p);
    ecma_deref_ecma_string (context_p, local_name_p);
    ecma_deref_ecma_string (context_p, export_name_p);

    if (parser_context_p->token.type != LEXER_COMMA && parser_context_p->token.type != LEXER_RIGHT_BRACE)
    {
      parser_raise_error (parser_context_p, PARSER_ERR_RIGHT_BRACE_COMMA_EXPECTED);
    }
    else if (parser_context_p->token.type == LEXER_COMMA)
    {
      lexer_next_token (parser_context_p);
    }

    if (lexer_token_is_identifier (parser_context_p, "from", 4))
    {
      parser_raise_error (parser_context_p, PARSER_ERR_RIGHT_BRACE_EXPECTED);
    }
  }
} /* parser_module_parse_export_clause */

/**
 * Parse an ImportClause
 */
void
parser_module_parse_import_clause (parser_context_t *parser_context_p) /**< parser context */
{
  ecma_context_t *context_p = parser_context_p->context_p;
  JJS_ASSERT (parser_context_p->token.type == LEXER_LEFT_BRACE);
  lexer_next_token (parser_context_p);

  while (true)
  {
    if (parser_context_p->token.type == LEXER_RIGHT_BRACE)
    {
      lexer_next_token (parser_context_p);
      break;
    }

    if (parser_context_p->token.type != LEXER_LITERAL || parser_context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
    {
      parser_raise_error (parser_context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
    }

    if (parser_context_p->next_scanner_info_p->source_p == parser_context_p->source_p)
    {
      JJS_ASSERT (parser_context_p->next_scanner_info_p->type == SCANNER_TYPE_ERR_REDECLARED);
      parser_raise_error (parser_context_p, PARSER_ERR_VARIABLE_REDECLARED);
    }

    ecma_string_t *import_name_p = NULL;
    ecma_string_t *local_name_p = NULL;

    lexer_construct_literal_object (parser_context_p, &parser_context_p->token.lit_location, LEXER_NEW_IDENT_LITERAL);

    uint16_t import_name_index = parser_context_p->lit_object.index;
    uint16_t local_name_index = PARSER_MAXIMUM_NUMBER_OF_LITERALS;

    lexer_next_token (parser_context_p);
    if (lexer_token_is_identifier (parser_context_p, "as", 2))
    {
      lexer_next_token (parser_context_p);

      if (parser_context_p->token.type != LEXER_LITERAL || parser_context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
      {
        parser_raise_error (parser_context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
      }

      if (parser_context_p->next_scanner_info_p->source_p == parser_context_p->source_p)
      {
        JJS_ASSERT (parser_context_p->next_scanner_info_p->type == SCANNER_TYPE_ERR_REDECLARED);
        parser_raise_error (parser_context_p, PARSER_ERR_VARIABLE_REDECLARED);
      }

      lexer_construct_literal_object (parser_context_p, &parser_context_p->token.lit_location, LEXER_NEW_IDENT_LITERAL);

      local_name_index = parser_context_p->lit_object.index;

      lexer_next_token (parser_context_p);
    }

    import_name_p = parser_new_ecma_string_from_literal (parser_context_p, PARSER_GET_LITERAL (import_name_index));

    if (local_name_index != PARSER_MAXIMUM_NUMBER_OF_LITERALS)
    {
      local_name_p = parser_new_ecma_string_from_literal (parser_context_p, PARSER_GET_LITERAL (local_name_index));
    }
    else
    {
      local_name_p = import_name_p;
      ecma_ref_ecma_string (local_name_p);
    }

    if (parser_module_check_duplicate_import (parser_context_p, local_name_p))
    {
      ecma_deref_ecma_string (context_p, local_name_p);
      ecma_deref_ecma_string (context_p, import_name_p);
      parser_raise_error (parser_context_p, PARSER_ERR_DUPLICATED_IMPORT_BINDING);
    }

    parser_module_add_names_to_node (parser_context_p, import_name_p, local_name_p);
    ecma_deref_ecma_string (context_p, local_name_p);
    ecma_deref_ecma_string (context_p, import_name_p);

    if (parser_context_p->token.type != LEXER_COMMA && (parser_context_p->token.type != LEXER_RIGHT_BRACE))
    {
      parser_raise_error (parser_context_p, PARSER_ERR_RIGHT_BRACE_COMMA_EXPECTED);
    }
    else if (parser_context_p->token.type == LEXER_COMMA)
    {
      lexer_next_token (parser_context_p);
    }

    if (lexer_token_is_identifier (parser_context_p, "from", 4))
    {
      parser_raise_error (parser_context_p, PARSER_ERR_RIGHT_BRACE_EXPECTED);
    }
  }
} /* parser_module_parse_import_clause */

/**
 * Raises parser error if the import or export statement is not in the global scope.
 */
void
parser_module_check_request_place (parser_context_t *parser_context_p) /**< parser context */
{
  if (parser_context_p->last_context_p != NULL || parser_context_p->stack_top_uint8 != 0
      || (parser_context_p->status_flags & PARSER_IS_FUNCTION) || (parser_context_p->global_status_flags & ECMA_PARSE_EVAL)
      || (parser_context_p->global_status_flags & ECMA_PARSE_MODULE) == 0)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_MODULE_UNEXPECTED);
  }
} /* parser_module_check_request_place */

/**
 * Append names to the names list.
 */
void
parser_module_append_names (parser_context_t *parser_context_p, /**< parser context */
                            ecma_module_names_t **module_names_p) /**< target names */
{
  ecma_module_names_t *last_name_p = parser_context_p->module_names_p;

  if (last_name_p == NULL)
  {
    return;
  }

  if (*module_names_p != NULL)
  {
    while (last_name_p->next_p != NULL)
    {
      last_name_p = last_name_p->next_p;
    }

    last_name_p->next_p = *module_names_p;
  }

  *module_names_p = parser_context_p->module_names_p;
  parser_context_p->module_names_p = NULL;
} /* parser_module_append_names */

/**
 * Handle module specifier at the end of the import / export statement.
 */
void
parser_module_handle_module_specifier (parser_context_t *parser_context_p, /**< parser context */
                                       ecma_module_node_t **node_list_p) /**< target node list */
{
  if (parser_context_p->token.type != LEXER_LITERAL || parser_context_p->token.lit_location.type != LEXER_STRING_LITERAL
      || parser_context_p->token.lit_location.length == 0)
  {
    parser_raise_error (parser_context_p, PARSER_ERR_STRING_EXPECTED);
  }

  lexer_construct_literal_object (parser_context_p, &parser_context_p->token.lit_location, LEXER_STRING_LITERAL);

  ecma_context_t *context_p = parser_context_p->context_p;
  lexer_literal_t *path_p = parser_context_p->lit_object.literal_p;

  lexer_next_token (parser_context_p);

  /* The lexer_next_token may throw an error, so the path is constructed after its call. */
  ecma_string_t *path_string_p = parser_new_ecma_string_from_literal (parser_context_p, path_p);

  ecma_module_node_t *node_p = parser_context_p->context_p->module_current_p->imports_p;
  ecma_module_node_t *last_node_p = NULL;

  /* Check if we have an import node with the same module request. */

  while (node_p != NULL)
  {
    if (ecma_compare_ecma_strings (ecma_get_string_from_value (context_p, node_p->u.path_or_module), path_string_p))
    {
      ecma_deref_ecma_string (context_p, path_string_p);
      break;
    }

    last_node_p = node_p;
    node_p = node_p->next_p;
  }

  if (node_p == NULL)
  {
    node_p = (ecma_module_node_t *) jmem_heap_alloc_block_null_on_error (context_p, sizeof (ecma_module_node_t));

    if (node_p == NULL)
    {
      ecma_deref_ecma_string (context_p, path_string_p);
      parser_raise_error (parser_context_p, PARSER_ERR_OUT_OF_MEMORY);
    }

    if (last_node_p == NULL)
    {
      parser_context_p->context_p->module_current_p->imports_p = node_p;
    }
    else
    {
      last_node_p->next_p = node_p;
    }

    node_p->next_p = NULL;
    node_p->module_names_p = NULL;
    node_p->u.path_or_module = ecma_make_string_value (context_p, path_string_p);
  }

  /* Append to imports. */
  if (node_list_p == NULL)
  {
    parser_module_append_names (parser_context_p, &node_p->module_names_p);
    return;
  }

  ecma_value_t *module_object_p = &node_p->u.path_or_module;

  node_p = *node_list_p;
  last_node_p = NULL;

  while (node_p != NULL)
  {
    if (node_p->u.module_object_p == module_object_p)
    {
      parser_module_append_names (parser_context_p, &node_p->module_names_p);
      return;
    }

    last_node_p = node_p;
    node_p = node_p->next_p;
  }

  node_p = (ecma_module_node_t *) parser_malloc_vm (parser_context_p, sizeof (ecma_module_node_t));

  if (last_node_p == NULL)
  {
    *node_list_p = node_p;
  }
  else
  {
    last_node_p->next_p = node_p;
  }

  node_p->next_p = NULL;
  node_p->module_names_p = parser_context_p->module_names_p;
  node_p->u.module_object_p = module_object_p;

  parser_context_p->module_names_p = NULL;
} /* parser_module_handle_module_specifier */

#endif /* JJS_MODULE_SYSTEM */
