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

#include "ecma-symbol-object.h"

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects-general.h"
#include "ecma-objects.h"

#include "lit-char-helpers.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmasymbolobject ECMA Symbol object related routines
 * @{
 */

/**
 * Symbol creation operation.
 *
 * See also: ECMA-262 v6, 6.1.5.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_symbol (ecma_context_t *context_p, /**< JJS context */
                       const ecma_value_t *arguments_list_p, /**< list of arguments */
                       uint32_t arguments_list_len) /**< length of the arguments' list */
{
  JJS_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_value_t string_desc;

  /* 1-3. */
  if (arguments_list_len == 0 || ecma_is_value_undefined (arguments_list_p[0]))
  {
    string_desc = ECMA_VALUE_UNDEFINED;
  }
  else
  {
    ecma_string_t *str_p = ecma_op_to_string (context_p, arguments_list_p[0]);

    /* 4. */
    if (JJS_UNLIKELY (str_p == NULL))
    {
      return ECMA_VALUE_ERROR;
    }

    string_desc = ecma_make_string_value (context_p, str_p);
  }

  /* 5. */
  return ecma_make_symbol_value (context_p, ecma_new_symbol_from_descriptor_string (context_p, string_desc));
} /* ecma_op_create_symbol */

/**
 * Symbol object creation operation.
 *
 * See also: ECMA-262 v6, 19.4.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_symbol_object (ecma_context_t *context_p, /**< JJS context */
                              const ecma_value_t value) /**< symbol value */
{
  JJS_ASSERT (ecma_is_value_symbol (value));

  ecma_object_t *prototype_obj_p = ecma_builtin_get (context_p, ECMA_BUILTIN_ID_SYMBOL_PROTOTYPE);
  ecma_object_t *object_p =
    ecma_create_object (context_p, prototype_obj_p, sizeof (ecma_extended_object_t), ECMA_OBJECT_TYPE_CLASS);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.cls.type = ECMA_OBJECT_CLASS_SYMBOL;
  ext_object_p->u.cls.u3.value = ecma_copy_value (context_p, value);

  return ecma_make_object_value (context_p, object_p);
} /* ecma_op_create_symbol_object */

/**
 * Get the symbol descriptor ecma-string from an ecma-symbol
 *
 * @return pointer to ecma-string descriptor
 */
ecma_value_t
ecma_get_symbol_description (ecma_context_t *context_p, /**< JJS context */
                             ecma_string_t *symbol_p) /**< ecma-symbol */
{
  JJS_UNUSED (context_p);
  JJS_ASSERT (symbol_p != NULL);
  JJS_ASSERT (ecma_prop_name_is_symbol (symbol_p));

  return ((ecma_extended_string_t *) symbol_p)->u.symbol_descriptor;
} /* ecma_get_symbol_description */

/**
 * Get the descriptive string of the Symbol.
 *
 * See also: ECMA-262 v6, 19.4.3.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_get_symbol_descriptive_string (ecma_context_t *context_p, /**< JJS context */
                                    ecma_value_t symbol_value) /**< symbol to stringify */
{
  /* 1. */
  JJS_ASSERT (ecma_is_value_symbol (symbol_value));

  /* 2 - 3. */
  ecma_string_t *symbol_p = ecma_get_symbol_from_value (context_p, symbol_value);
  ecma_value_t string_desc = ecma_get_symbol_description (context_p, symbol_p);
  ecma_stringbuilder_t builder = ecma_stringbuilder_create_raw (context_p, (lit_utf8_byte_t *) ("Symbol("), 7);

  if (!ecma_is_value_undefined (string_desc))
  {
    ecma_string_t *string_desc_p = ecma_get_string_from_value (context_p, string_desc);
    ecma_stringbuilder_append (&builder, string_desc_p);
  }

  ecma_stringbuilder_append_byte (&builder, LIT_CHAR_RIGHT_PAREN);
  return ecma_make_string_value (context_p, ecma_stringbuilder_finalize (&builder));
} /* ecma_get_symbol_descriptive_string */

/**
 * thisSymbolValue abstract operation
 *
 * See also:
 *          ECMA-262 v11, 19.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_symbol_this_value (ecma_context_t *context_p, /**< JJS context */
                        ecma_value_t this_arg) /**< this argument value */
{
  /* 1. */
  if (ecma_is_value_symbol (this_arg))
  {
    return this_arg;
  }

  /* 2. */
  if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (context_p, this_arg);

    if (ecma_object_class_is (object_p, ECMA_OBJECT_CLASS_SYMBOL))
    {
      return ((ecma_extended_object_t *) object_p)->u.cls.u3.value;
    }
  }

  /* 3. */
  return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_THIS_NOT_SYMBOL);
} /* ecma_symbol_this_value */

/**
 * @}
 * @}
 */
