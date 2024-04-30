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

#include "ecma-array-object.h"
#include "ecma-builtin-function-prototype.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtin-object.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-iterator-object.h"
#include "ecma-proxy-object.h"

#include "jcontext.h"

#if JJS_BUILTIN_REFLECT

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
  ECMA_REFLECT_OBJECT_ROUTINE_START = 0,
  ECMA_REFLECT_OBJECT_GET, /* ECMA-262 v6, 26.1.6 */
  ECMA_REFLECT_OBJECT_SET, /* ECMA-262 v6, 26.1.13 */
  ECMA_REFLECT_OBJECT_HAS, /* ECMA-262 v6, 26.1.9 */
  ECMA_REFLECT_OBJECT_DELETE_PROPERTY, /* ECMA-262 v6, 26.1.4 */
  ECMA_REFLECT_OBJECT_CONSTRUCT, /* ECMA-262, 26.1.2 */
  ECMA_REFLECT_OBJECT_OWN_KEYS, /* ECMA-262 v6, 26.1.11 */
  ECMA_REFLECT_OBJECT_GET_PROTOTYPE_OF, /* ECMA-262 v6, 26.1.8 */
  ECMA_REFLECT_OBJECT_SET_PROTOTYPE_OF, /* ECMA-262 v6, 26.1.14 */
  ECMA_REFLECT_OBJECT_APPLY, /* ECMA-262 v6, 26.1.1 */
  ECMA_REFLECT_OBJECT_DEFINE_PROPERTY, /* ECMA-262 v6, 26.1.3 */
  ECMA_REFLECT_OBJECT_GET_OWN_PROPERTY_DESCRIPTOR, /* ECMA-262 v6, 26.1.7 */
  ECMA_REFLECT_OBJECT_IS_EXTENSIBLE, /* ECMA-262 v6, 26.1.10 */
  ECMA_REFLECT_OBJECT_PREVENT_EXTENSIONS, /* ECMA-262 v6, 26.1.12 */
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-reflect.inc.h"
#define BUILTIN_UNDERSCORED_ID  reflect
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup object ECMA Reflect object built-in
 * @{
 */

/**
 * Dispatcher for the built-in's routines.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_reflect_dispatch_routine (ecma_context_t *context_p, /**< JJS context */
                                       uint8_t builtin_routine_id, /**< built-in wide routine identifier */
                                       ecma_value_t this_arg, /**< 'this' argument value */
                                       const ecma_value_t arguments_list[], /**< list of arguments
                                                                             *   passed to routine */
                                       uint32_t arguments_number) /**< length of arguments' list */
{
  JJS_UNUSED (this_arg);

  if (builtin_routine_id < ECMA_REFLECT_OBJECT_CONSTRUCT)
  {
    /* 1. */
    if (arguments_number == 0 || !ecma_is_value_object (arguments_list[0]))
    {
      return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_IS_NOT_AN_OBJECT);
    }

    /* 2. */
    ecma_string_t *name_str_p = ecma_op_to_property_key (context_p, arguments_list[1]);

    /* 3. */
    if (name_str_p == NULL)
    {
      return ECMA_VALUE_ERROR;
    }

    ecma_value_t ret_value;
    ecma_object_t *target_p = ecma_get_object_from_value (context_p, arguments_list[0]);
    switch (builtin_routine_id)
    {
      case ECMA_REFLECT_OBJECT_GET:
      {
        ecma_value_t receiver = arguments_list[0];

        /* 4. */
        if (arguments_number > 2)
        {
          receiver = arguments_list[2];
        }

        ret_value = ecma_op_object_get_with_receiver (context_p, target_p, name_str_p, receiver);
        break;
      }

      case ECMA_REFLECT_OBJECT_HAS:
      {
        ret_value = ecma_op_object_has_property (context_p, target_p, name_str_p);
        break;
      }

      case ECMA_REFLECT_OBJECT_DELETE_PROPERTY:
      {
        ret_value = ecma_op_object_delete (context_p, target_p, name_str_p, false);
        break;
      }

      default:
      {
        JJS_ASSERT (builtin_routine_id == ECMA_REFLECT_OBJECT_SET);

        ecma_value_t receiver = arguments_list[0];

        if (arguments_number > 3)
        {
          receiver = arguments_list[3];
        }

        ret_value = ecma_op_object_put_with_receiver (context_p, target_p, name_str_p, arguments_list[2], receiver, false);
        break;
      }
    }

    ecma_deref_ecma_string (context_p, name_str_p);
    return ret_value;
  }

  if (builtin_routine_id == ECMA_REFLECT_OBJECT_OWN_KEYS)
  {
    /* 1. */
    if (arguments_number == 0 || !ecma_is_value_object (arguments_list[0]))
    {
      return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_IS_NOT_AN_OBJECT);
    }

    ecma_object_t *target_p = ecma_get_object_from_value (context_p, arguments_list[0]);

    /* 2. */
    ecma_collection_t *prop_names = ecma_op_object_own_property_keys (context_p, target_p, JJS_PROPERTY_FILTER_ALL);

#if JJS_BUILTIN_PROXY
    if (prop_names == NULL)
    {
      return ECMA_VALUE_ERROR;
    }
#endif /* JJS_BUILTIN_PROXY */

    /* 3. */
    return ecma_op_new_array_object_from_collection (context_p, prop_names, false);
  }

  if (builtin_routine_id == ECMA_REFLECT_OBJECT_CONSTRUCT)
  {
    /* 1. */
    if (arguments_number < 1 || !ecma_is_constructor (context_p, arguments_list[0]))
    {
      return ecma_raise_type_error (context_p, ECMA_ERR_TARGET_IS_NOT_A_CONSTRUCTOR);
    }

    ecma_object_t *target_p = ecma_get_object_from_value (context_p, arguments_list[0]);

    /* 2. */
    ecma_object_t *new_target_p = target_p;

    if (arguments_number > 2)
    {
      /* 3. */
      if (!ecma_is_constructor (context_p, arguments_list[2]))
      {
        return ecma_raise_type_error (context_p, ECMA_ERR_TARGET_IS_NOT_A_CONSTRUCTOR);
      }

      new_target_p = ecma_get_object_from_value (context_p, arguments_list[2]);
    }

    /* 4. */
    if (arguments_number < 2)
    {
      return ecma_raise_type_error (context_p, ECMA_ERR_REFLECT_EXPECTS_AN_OBJECT_AS_SECOND_ARGUMENT);
    }

    ecma_collection_t *coll_p = ecma_op_create_list_from_array_like (context_p, arguments_list[1], false);

    if (coll_p == NULL)
    {
      return ECMA_VALUE_ERROR;
    }

    ecma_value_t ret_value = ecma_op_function_construct (context_p, target_p, new_target_p, coll_p->buffer_p, coll_p->item_count);

    ecma_collection_free (context_p, coll_p);
    return ret_value;
  }

  if (!ecma_is_value_object (arguments_list[0]))
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_IS_NOT_AN_OBJECT);
  }

  switch (builtin_routine_id)
  {
    case ECMA_REFLECT_OBJECT_GET_PROTOTYPE_OF:
    {
      return ecma_builtin_object_object_get_prototype_of (context_p, ecma_get_object_from_value (context_p, arguments_list[0]));
    }
    case ECMA_REFLECT_OBJECT_SET_PROTOTYPE_OF:
    {
      if (!ecma_is_value_object (arguments_list[1]) && !ecma_is_value_null (arguments_list[1]))
      {
        return ecma_raise_type_error (context_p, ECMA_ERR_PROTOTYPE_IS_NEITHER_OBJECT_NOR_NULL);
      }

      ecma_object_t *obj_p = ecma_get_object_from_value (context_p, arguments_list[0]);
      ecma_value_t status;

#if JJS_BUILTIN_PROXY
      if (ECMA_OBJECT_IS_PROXY (obj_p))
      {
        status = ecma_proxy_object_set_prototype_of (context_p, obj_p, arguments_list[1]);
      }
      else
#endif /* JJS_BUILTIN_PROXY */
      {
        status = ecma_op_ordinary_object_set_prototype_of (context_p, obj_p, arguments_list[1]);
      }

      return status;
    }
    case ECMA_REFLECT_OBJECT_APPLY:
    {
      if (!ecma_op_is_callable (context_p, arguments_list[0]))
      {
        return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_THIS_NOT_FUNCTION);
      }

      ecma_object_t *func_obj_p = ecma_get_object_from_value (context_p, arguments_list[0]);
      return ecma_builtin_function_prototype_object_apply (context_p, func_obj_p, arguments_list[1], arguments_list[2]);
    }
    case ECMA_REFLECT_OBJECT_DEFINE_PROPERTY:
    {
      ecma_object_t *obj_p = ecma_get_object_from_value (context_p, arguments_list[0]);
      ecma_string_t *name_str_p = ecma_op_to_property_key (context_p, arguments_list[1]);

      if (name_str_p == NULL)
      {
        return ECMA_VALUE_ERROR;
      }

      ecma_property_descriptor_t prop_desc;
      ecma_value_t conv_result = ecma_op_to_property_descriptor (context_p, arguments_list[2], &prop_desc);

      if (ECMA_IS_VALUE_ERROR (conv_result))
      {
        ecma_deref_ecma_string (context_p, name_str_p);
        return conv_result;
      }

      ecma_value_t result = ecma_op_object_define_own_property (context_p, obj_p, name_str_p, &prop_desc);

      ecma_deref_ecma_string (context_p, name_str_p);
      ecma_free_property_descriptor (context_p, &prop_desc);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        return result;
      }

      bool boolean_result = ecma_op_to_boolean (context_p, result);

      return ecma_make_boolean_value (boolean_result);
    }
    case ECMA_REFLECT_OBJECT_GET_OWN_PROPERTY_DESCRIPTOR:
    {
      ecma_object_t *obj_p = ecma_get_object_from_value (context_p, arguments_list[0]);
      ecma_string_t *name_str_p = ecma_op_to_property_key (context_p, arguments_list[1]);

      if (name_str_p == NULL)
      {
        return ECMA_VALUE_ERROR;
      }

      ecma_value_t ret_val = ecma_builtin_object_object_get_own_property_descriptor (context_p, obj_p, name_str_p);
      ecma_deref_ecma_string (context_p, name_str_p);
      return ret_val;
    }
    case ECMA_REFLECT_OBJECT_IS_EXTENSIBLE:
    {
      ecma_object_t *obj_p = ecma_get_object_from_value (context_p, arguments_list[0]);
      return ecma_builtin_object_object_is_extensible (context_p, obj_p);
    }
    default:
    {
      JJS_ASSERT (builtin_routine_id == ECMA_REFLECT_OBJECT_PREVENT_EXTENSIONS);
      ecma_object_t *obj_p = ecma_get_object_from_value (context_p, arguments_list[0]);

#if JJS_BUILTIN_PROXY
      if (ECMA_OBJECT_IS_PROXY (obj_p))
      {
        return ecma_proxy_object_prevent_extensions (context_p, obj_p);
      }
#endif /* !JJS_BUILTIN_PROXY */

      ecma_op_ordinary_object_prevent_extensions (context_p, obj_p);

      return ECMA_VALUE_TRUE;
    }
  }
} /* ecma_builtin_reflect_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* JJS_BUILTIN_REFLECT */
