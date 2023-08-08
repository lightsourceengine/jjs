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

/* This file is automatically generated by the gen-strings.py script
 * from parser-error-messages.ini. Do not edit! */

#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_DUPLICATED_LABEL, "Duplicated label")
PARSER_ERROR_DEF (PARSER_ERR_LEFT_PAREN_EXPECTED, "Expected '(' token")
PARSER_ERROR_DEF (PARSER_ERR_RIGHT_PAREN_EXPECTED, "Expected ')' token")
PARSER_ERROR_DEF (PARSER_ERR_COLON_EXPECTED, "Expected ':' token")
PARSER_ERROR_DEF (PARSER_ERR_SEMICOLON_EXPECTED, "Expected ';' token")
PARSER_ERROR_DEF (PARSER_ERR_RIGHT_SQUARE_EXPECTED, "Expected ']' token")
PARSER_ERROR_DEF (PARSER_ERR_LEFT_BRACE_EXPECTED, "Expected '{' token")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM || JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_RIGHT_BRACE_EXPECTED, "Expected '}' token")
#endif /* JJS_MODULE_SYSTEM \
|| JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_NUMBER_TOO_LONG, "Number is too long")
#endif /* JJS_PARSER */
#if JJS_BUILTIN_REGEXP && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_REGEXP_TOO_LONG, "Regexp is too long")
#endif /* JJS_BUILTIN_REGEXP && JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_STATEMENT_EXPECTED, "Statement expected")
PARSER_ERROR_DEF (PARSER_ERR_STRING_TOO_LONG, "String is too long")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_AS_EXPECTED, "Expected 'as' token")
#endif /* JJS_MODULE_SYSTEM && JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_IN_EXPECTED, "Expected 'in' token")
PARSER_ERROR_DEF (PARSER_ERR_OF_EXPECTED, "Expected 'of' token")
PARSER_ERROR_DEF (PARSER_ERR_EXPRESSION_EXPECTED, "Expression expected")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM || JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_IDENTIFIER_EXPECTED, "Identifier expected")
#endif /* JJS_MODULE_SYSTEM \
|| JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_INVALID_OCTAL_DIGIT, "Invalid octal digit")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_SWITCH, "Invalid switch body")
#endif /* JJS_PARSER */
#if JJS_BUILTIN_REGEXP && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_UNKNOWN_REGEXP_FLAG, "Unknown regexp flag")
#endif /* JJS_BUILTIN_REGEXP && JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_INVALID_BIN_DIGIT, "Invalid binary digit")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_RIGHT_SQUARE, "Unexpected '}' token")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_FROM_EXPECTED, "Expected 'from' token")
#endif /* JJS_MODULE_SYSTEM && JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_MISSING_EXPONENT, "Missing exponent part")
#endif /* JJS_PARSER */
#if JJS_BUILTIN_REGEXP && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_DUPLICATED_REGEXP_FLAG, "Duplicated regexp flag")
#endif /* JJS_BUILTIN_REGEXP && JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_ARGUMENT_LIST_EXPECTED, "Expected argument list")
PARSER_ERROR_DEF (PARSER_ERR_IDENTIFIER_TOO_LONG, "Identifier is too long")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_META_EXPECTED, "Expected 'meta' keyword")
#endif /* JJS_MODULE_SYSTEM && JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_UNEXPECTED_END, "Unexpected end of input")
PARSER_ERROR_DEF (PARSER_ERR_UNEXPECTED_PRIVATE_FIELD, "Unexpected private field")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_LEFT_BRACE_MULTIPLY_EXPECTED, "Expected '{' or '*' token")
#endif /* JJS_MODULE_SYSTEM && JJS_PARSER */
#if JJS_MODULE_SYSTEM
PARSER_ERROR_DEF (PARSER_ERR_RIGHT_BRACE_COMMA_EXPECTED, "Expected '}' or ',' token")
PARSER_ERROR_DEF (PARSER_ERR_STRING_EXPECTED, "Expected a string literal")
#endif /* JJS_MODULE_SYSTEM */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_INVALID_HEX_DIGIT, "Invalid hexadecimal digit")
#endif /* JJS_PARSER */
#if JJS_BUILTIN_REGEXP && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_UNTERMINATED_REGEXP, "Unterminated regexp literal")
#endif /* JJS_BUILTIN_REGEXP && JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_UNTERMINATED_STRING, "Unterminated string literal")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_FROM_COMMA_EXPECTED, "Expected 'from' or ',' token")
#endif /* JJS_MODULE_SYSTEM && JJS_PARSER */
#if JJS_MODULE_SYSTEM
PARSER_ERROR_DEF (PARSER_ERR_EXPORT_NOT_DEFINED, "Export not defined in module")
#endif /* JJS_MODULE_SYSTEM */
#if JJS_MODULE_SYSTEM || JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_VARIABLE_REDECLARED, "Local variable is redeclared")
#endif /* JJS_MODULE_SYSTEM \
|| JJS_PARSER */
#if JJS_BUILTIN_BIGINT && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_INVALID_BIGINT, "Number is not a valid BigInt")
#endif /* JJS_BUILTIN_BIGINT && JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_PROPERTY_IDENTIFIER_EXPECTED, "Property identifier expected")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM
PARSER_ERROR_DEF (PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER, "Duplicate exported identifier")
#endif /* JJS_MODULE_SYSTEM */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_NEW_TARGET_EXPECTED, "Expected new.target expression")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_CHARACTER, "Invalid (unexpected) character")
PARSER_ERROR_DEF (PARSER_ERR_NON_STRICT_ARG_DEFINITION, "Non-strict argument definition")
PARSER_ERROR_DEF (PARSER_ERR_UNTERMINATED_MULTILINE_COMMENT, "Unterminated multiline comment")
PARSER_ERROR_DEF (PARSER_ERR_CATCH_FINALLY_EXPECTED, "Catch or finally block expected")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE, "Invalid unicode escape sequence")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM
PARSER_ERROR_DEF (PARSER_ERR_DUPLICATED_IMPORT_BINDING, "Duplicated imported binding name")
#endif /* JJS_MODULE_SYSTEM */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_WHILE_EXPECTED, "While expected for do-while loop")
PARSER_ERROR_DEF (PARSER_ERR_DELETE_PRIVATE_FIELD, "Private fields can not be deleted")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_LHS_FOR_LOOP, "Invalid left-hand-side in for-loop")
PARSER_ERROR_DEF (PARSER_ERR_LEFT_HAND_SIDE_EXP_EXPECTED, "Left-hand-side expression expected")
PARSER_ERROR_DEF (PARSER_ERR_LITERAL_LIMIT_REACHED, "Maximum number of literals reached")
PARSER_ERROR_DEF (PARSER_ERR_STACK_LIMIT_REACHED, "Maximum function stack size reached")
PARSER_ERROR_DEF (PARSER_ERR_AWAIT_NOT_ALLOWED, "Await expression is not allowed here")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_LEFT_BRACE_MULTIPLY_LITERAL_EXPECTED, "Expected '{' or '*' or literal token")
#endif /* JJS_MODULE_SYSTEM && JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_INVALID_LHS_ASSIGNMENT, "Invalid left-hand side in assignment")
PARSER_ERROR_DEF (PARSER_ERR_SCOPE_STACK_LIMIT_REACHED, "Maximum depth of scope stack reached")
PARSER_ERROR_DEF (PARSER_ERR_UNEXPECTED_SUPER_KEYWORD, "Super is not allowed to be used here")
PARSER_ERROR_DEF (PARSER_ERR_YIELD_NOT_ALLOWED, "Yield expression is not allowed here")
PARSER_ERROR_DEF (PARSER_ERR_MULTIPLE_CLASS_CONSTRUCTORS, "Multiple constructors are not allowed")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM
PARSER_ERROR_DEF (PARSER_ERR_MODULE_UNEXPECTED, "Unexpected import or export statement")
#endif /* JJS_MODULE_SYSTEM */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_IDENTIFIER_AFTER_NUMBER, "Identifier cannot start after a number")
PARSER_ERROR_DEF (PARSER_ERR_MULTIPLE_DEFAULTS_NOT_ALLOWED, "Multiple default cases are not allowed")
PARSER_ERROR_DEF (PARSER_ERR_ARRAY_ITEM_SEPARATOR_EXPECTED, "Expected ',' or ']' after an array item")
PARSER_ERROR_DEF (PARSER_ERR_ILLEGAL_PROPERTY_IN_DECLARATION, "Illegal property in declaration context")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_DESTRUCTURING_PATTERN, "Invalid destructuring assignment target")
PARSER_ERROR_DEF (PARSER_ERR_DUPLICATED_PRIVATE_FIELD, "Private field has already been declared")
PARSER_ERROR_DEF (PARSER_ERR_NO_ARGUMENTS_EXPECTED, "Property getters must have no arguments")
PARSER_ERROR_DEF (PARSER_ERR_ONE_ARGUMENT_EXPECTED, "Property setters must have one argument")
PARSER_ERROR_DEF (PARSER_ERR_CASE_NOT_IN_SWITCH, "Case statement must be in a switch block")
PARSER_ERROR_DEF (PARSER_ERR_CLASS_CONSTRUCTOR_AS_ACCESSOR, "Class constructor may not be an accessor")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_CONTINUE, "Continue statement must be inside a loop")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_IMPORT_META_REQUIRE_MODULE, "Cannot use 'import.meta' outside a module")
#endif /* JJS_MODULE_SYSTEM && JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_INVALID_IDENTIFIER_PART, "Character cannot be part of an identifier")
PARSER_ERROR_DEF (PARSER_ERR_EVAL_CANNOT_ASSIGNED, "Eval cannot be assigned to in strict mode")
PARSER_ERROR_DEF (PARSER_ERR_WITH_NOT_ALLOWED, "With statement not allowed in strict mode")
PARSER_ERROR_DEF (PARSER_ERR_NEW_TARGET_NOT_ALLOWED, "new.target expression is not allowed here")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_IDENTIFIER_START, "Character cannot be start of an identifier")
PARSER_ERROR_DEF (PARSER_ERR_STRICT_IDENT_NOT_ALLOWED, "Identifier name is reserved in strict mode")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_NULLISH_COALESCING, "Cannot chain nullish with logical AND or OR")
PARSER_ERROR_DEF (PARSER_ERR_DEFAULT_NOT_IN_SWITCH, "Default statement must be in a switch block")
#endif /* JJS_PARSER */
#if JJS_MODULE_SYSTEM && JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_IMPORT_AFTER_NEW, "Module import call is not allowed after new")
#endif /* JJS_MODULE_SYSTEM && JJS_PARSER */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_TOO_MANY_CLASS_FIELDS, "Too many computed class fields are declared")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_KEYWORD, "Escape sequences are not allowed in keywords")
PARSER_ERROR_DEF (PARSER_ERR_ARGUMENT_LIMIT_REACHED, "Maximum number of function arguments reached")
PARSER_ERROR_DEF (PARSER_ERR_NEWLINE_NOT_ALLOWED, "Newline is not allowed in strings or regexps")
PARSER_ERROR_DEF (PARSER_ERR_OCTAL_NUMBER_NOT_ALLOWED, "Octal numbers are not allowed in strict mode")
PARSER_ERROR_DEF (PARSER_ERR_CLASS_PRIVATE_CONSTRUCTOR, "Class constructor may not be a private method")
PARSER_ERROR_DEF (PARSER_ERR_FOR_AWAIT_NO_OF, "only 'of' form is allowed for for-await loops")
PARSER_ERROR_DEF (PARSER_ERR_ARGUMENTS_CANNOT_ASSIGNED, "Arguments cannot be assigned to in strict mode")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_BREAK, "Break statement must be inside a loop or switch")
PARSER_ERROR_DEF (PARSER_ERR_OBJECT_ITEM_SEPARATOR_EXPECTED, "Expected ',' or '}' after a property definition")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_BREAK_LABEL, "Labeled statement targeted by a break not found")
#endif /* JJS_PARSER */
#if JJS_PARSER && !(JJS_BUILTIN_REGEXP)
PARSER_ERROR_DEF (PARSER_ERR_UNSUPPORTED_REGEXP, "Regexp is not supported in the selected profile")
#endif /* JJS_PARSER && !(JJS_BUILTIN_REGEXP) */
#if JJS_PARSER
PARSER_ERROR_DEF (PARSER_ERR_INVALID_RETURN, "Return statement must be inside a function body")
PARSER_ERROR_DEF (PARSER_ERR_COLON_FOR_CONDITIONAL_EXPECTED, "Expected ':' token for ?: conditional expression")
PARSER_ERROR_DEF (PARSER_ERR_FORMAL_PARAM_AFTER_REST_PARAMETER, "Rest parameter must be the last formal parameter")
PARSER_ERROR_DEF (PARSER_ERR_DELETE_IDENT_NOT_ALLOWED, "Deleting identifier is not allowed in strict mode")
PARSER_ERROR_DEF (PARSER_ERR_LABELLED_FUNC_NOT_IN_BLOCK, "Labelled functions are only allowed inside blocks")
PARSER_ERROR_DEF (PARSER_ERR_REST_PARAMETER_DEFAULT_INITIALIZER, "Rest parameter may not have a default initializer")
PARSER_ERROR_DEF (PARSER_ERR_EVAL_NOT_ALLOWED, "Eval is not allowed to be used here in strict mode")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_CONTINUE_LABEL, "Labeled statement targeted by a continue not found")
PARSER_ERROR_DEF (PARSER_ERR_LEXICAL_LET_BINDING, "Let binding cannot appear in let/const declarations")
PARSER_ERROR_DEF (PARSER_ERR_UNDECLARED_PRIVATE_FIELD, "Private field must be declared in an enclosing class")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_LHS_PREFIX_OP, "Invalid left-hand side expression in prefix operation")
PARSER_ERROR_DEF (PARSER_ERR_OCTAL_ESCAPE_NOT_ALLOWED, "Octal escape sequences are not allowed in strict mode")
PARSER_ERROR_DEF (PARSER_ERR_SETTER_REST_PARAMETER, "Setter function argument must not be a rest parameter")
PARSER_ERROR_DEF (PARSER_ERR_ARGUMENTS_IN_CLASS_FIELD, "In class field declarations 'arguments' is not allowed")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_LHS_POSTFIX_OP, "Invalid left-hand side expression in postfix operation")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_UNDERSCORE_IN_NUMBER, "Invalid use of underscore character in number literals")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_EXPONENTIATION, "Left operand of ** operator cannot be unary expression")
PARSER_ERROR_DEF (PARSER_ERR_MISSING_ASSIGN_AFTER_CONST, "Value assignment is expected after a const declaration")
PARSER_ERROR_DEF (PARSER_ERR_ARGUMENTS_NOT_ALLOWED, "Arguments is not allowed to be used here in strict mode")
PARSER_ERROR_DEF (PARSER_ERR_DUPLICATED_ARGUMENT_NAMES, "Duplicated function argument names are not allowed here")
PARSER_ERROR_DEF (PARSER_ERR_CLASS_STATIC_PROTOTYPE, "Classes may not have a static property called 'prototype'")
PARSER_ERROR_DEF (PARSER_ERR_INVALID_CLASS_CONSTRUCTOR, "Class constructor may not be a generator or async function")
PARSER_ERROR_DEF (PARSER_ERR_TEMPLATE_STR_OCTAL_ESCAPE, "Octal escape sequences are not allowed in template strings")
PARSER_ERROR_DEF (PARSER_ERR_DUPLICATED_PROTO, "Duplicate __proto__ fields are not allowed in object literals")
PARSER_ERROR_DEF (PARSER_ERR_GENERATOR_IN_SINGLE_STATEMENT_POS,
                  "Generator function cannot appear in a single-statement context")
PARSER_ERROR_DEF (PARSER_ERR_LEXICAL_SINGLE_STATEMENT,
                  "Lexical declaration cannot appear in a single-statement context")
PARSER_ERROR_DEF (PARSER_ERR_FOR_IN_OF_DECLARATION, "for in-of loop variable declaration may not have an initializer")
PARSER_ERROR_DEF (PARSER_ERR_FOR_AWAIT_NO_ASYNC, "for-await-of is only allowed inside async functions and generators")
PARSER_ERROR_DEF (PARSER_ERR_USE_STRICT_NOT_ALLOWED,
                  "The 'use strict' directive is not allowed for functions with non-simple arguments")
PARSER_ERROR_DEF (PARSER_ERR_ASSIGNMENT_EXPECTED,
                  "Unexpected arrow function or yield expression (parentheses around the expression may help)")
#endif /* JJS_PARSER */