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

#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"

#include "jcontext.h"
#include "opcodes.h"
#include "vm-defines.h"

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
  ECMA_GENERATOR_PROTOTYPE_ROUTINE_START = 0,
  ECMA_GENERATOR_PROTOTYPE_ROUTINE_NEXT,
  ECMA_GENERATOR_PROTOTYPE_ROUTINE_THROW,
  ECMA_GENERATOR_PROTOTYPE_ROUTINE_RETURN
} ecma_generator_operation_type_t;

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-generator-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID  generator_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup generatorprototype ECMA Generator.prototype object built-in
 * @{
 */

/**
 * Convert routine type to operation type.
 */
#define ECMA_GENERATOR_ROUTINE_TO_OPERATION(type) \
  ((ecma_iterator_command_type_t) ((type) -ECMA_GENERATOR_PROTOTYPE_ROUTINE_NEXT))

JJS_STATIC_ASSERT (ECMA_GENERATOR_ROUTINE_TO_OPERATION (ECMA_GENERATOR_PROTOTYPE_ROUTINE_NEXT) == ECMA_ITERATOR_NEXT,
                     convert_ecma_generator_routine_next_to_ecma_iterator_next_failed);

JJS_STATIC_ASSERT (ECMA_GENERATOR_ROUTINE_TO_OPERATION (ECMA_GENERATOR_PROTOTYPE_ROUTINE_THROW)
                       == ECMA_ITERATOR_THROW,
                     convert_ecma_generator_routine_throw_to_ecma_iterator_throw_failed);

JJS_STATIC_ASSERT (ECMA_GENERATOR_ROUTINE_TO_OPERATION (ECMA_GENERATOR_PROTOTYPE_ROUTINE_RETURN)
                       == ECMA_ITERATOR_RETURN,
                     convert_ecma_generator_routine_return_to_ecma_iterator_return_failed);

/**
 * Helper function for next / return / throw
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_generator_prototype_object_do (ecma_context_t *context_p, /**< JJS context */
                                            vm_executable_object_t *generator_object_p, /**< generator object */
                                            ecma_value_t arg, /**< argument */
                                            ecma_iterator_command_type_t resume_mode) /**< resume mode */
{
  arg = ecma_copy_value (context_p, arg);

  while (true)
  {
    if (generator_object_p->extended_object.u.cls.u2.executable_obj_flags & ECMA_EXECUTABLE_OBJECT_DO_AWAIT_OR_YIELD)
    {
      if (generator_object_p->extended_object.u.cls.u2.executable_obj_flags & ECMA_EXECUTABLE_OBJECT_RUNNING)
      {
        return ecma_raise_type_error (context_p, ECMA_ERR_GENERATOR_IS_CURRENTLY_UNDER_EXECUTION);
      }

      ecma_value_t iterator = generator_object_p->iterator;
      ecma_value_t next_method = generator_object_p->frame_ctx.stack_top_p[-1];

      bool done = false;

      generator_object_p->extended_object.u.cls.u2.executable_obj_flags |= ECMA_EXECUTABLE_OBJECT_RUNNING;
      ecma_value_t result = ecma_op_iterator_do (context_p, resume_mode, iterator, next_method, arg, &done);
      ecma_free_value (context_p, arg);
      generator_object_p->extended_object.u.cls.u2.executable_obj_flags &= (uint8_t) ~ECMA_EXECUTABLE_OBJECT_RUNNING;

      if (ECMA_IS_VALUE_ERROR (result))
      {
        arg = result;
      }
      else if (done)
      {
        arg = ecma_op_iterator_value (context_p, result);
        ecma_free_value (context_p, result);

        if (resume_mode == ECMA_ITERATOR_THROW)
        {
          resume_mode = ECMA_ITERATOR_NEXT;
        }
      }
      else
      {
        return result;
      }

      ECMA_EXECUTABLE_OBJECT_RESUME_EXEC (generator_object_p);
      generator_object_p->iterator = ECMA_VALUE_UNDEFINED;

      JJS_ASSERT (generator_object_p->frame_ctx.stack_top_p[-1] == ECMA_VALUE_UNDEFINED
                    || ecma_is_value_object (generator_object_p->frame_ctx.stack_top_p[-1]));
      generator_object_p->frame_ctx.stack_top_p--;

      if (ECMA_IS_VALUE_ERROR (arg))
      {
        arg = jcontext_take_exception (context_p);
        resume_mode = ECMA_ITERATOR_THROW;
      }
    }

    if (resume_mode == ECMA_ITERATOR_RETURN)
    {
      generator_object_p->frame_ctx.byte_code_p = opfunc_resume_executable_object_with_return;
    }
    else if (resume_mode == ECMA_ITERATOR_THROW)
    {
      generator_object_p->frame_ctx.byte_code_p = opfunc_resume_executable_object_with_throw;
    }

    ecma_value_t value = opfunc_resume_executable_object (generator_object_p, arg);

    if (ECMA_IS_VALUE_ERROR (value))
    {
      return value;
    }

    bool done;
    done = (generator_object_p->extended_object.u.cls.u2.executable_obj_flags & ECMA_EXECUTABLE_OBJECT_COMPLETED);

    if (!done)
    {
      const uint8_t *byte_code_p = generator_object_p->frame_ctx.byte_code_p;

      JJS_ASSERT (byte_code_p[-2] == CBC_EXT_OPCODE
                    && (byte_code_p[-1] == CBC_EXT_YIELD || byte_code_p[-1] == CBC_EXT_YIELD_ITERATOR));

      if (byte_code_p[-1] == CBC_EXT_YIELD_ITERATOR)
      {
        ecma_value_t iterator =
          ecma_op_get_iterator (context_p, value, ECMA_VALUE_SYNC_ITERATOR, generator_object_p->frame_ctx.stack_top_p);
        ecma_free_value (context_p, value);

        if (ECMA_IS_VALUE_ERROR (iterator))
        {
          resume_mode = ECMA_ITERATOR_THROW;
          arg = jcontext_take_exception (context_p);
          continue;
        }

        ecma_deref_object (ecma_get_object_from_value (context_p, iterator));
        generator_object_p->extended_object.u.cls.u2.executable_obj_flags |= ECMA_EXECUTABLE_OBJECT_DO_AWAIT_OR_YIELD;
        generator_object_p->iterator = iterator;

        if (generator_object_p->frame_ctx.stack_top_p[0] != ECMA_VALUE_UNDEFINED)
        {
          ecma_deref_object (ecma_get_object_from_value (context_p, generator_object_p->frame_ctx.stack_top_p[0]));
        }

        generator_object_p->frame_ctx.stack_top_p++;
        arg = ECMA_VALUE_UNDEFINED;
        continue;
      }
    }

    ecma_value_t result = ecma_create_iter_result_object (context_p, value, ecma_make_boolean_value (done));
    ecma_fast_free_value (context_p, value);
    return result;
  }
} /* ecma_builtin_generator_prototype_object_do */

/**
 * Dispatcher of the Generator built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_generator_prototype_dispatch_routine (ecma_context_t *context_p, /**< JJS context */
                                                   uint8_t builtin_routine_id, /**< built-in wide routine
                                                                                *   identifier */
                                                   ecma_value_t this_arg, /**< 'this' argument value */
                                                   const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                                           *   passed to routine */
                                                   uint32_t arguments_number) /**< length of arguments' list */
{
  JJS_UNUSED (arguments_number);

  vm_executable_object_t *executable_object_p = NULL;

  if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (context_p, this_arg);

    if (ecma_object_class_is (object_p, ECMA_OBJECT_CLASS_GENERATOR))
    {
      executable_object_p = (vm_executable_object_t *) object_p;
    }
  }

  if (executable_object_p == NULL)
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_ARGUMENT_THIS_NOT_GENERATOR_OBJECT);
  }

  if (executable_object_p->extended_object.u.cls.u2.executable_obj_flags & ECMA_EXECUTABLE_OBJECT_RUNNING)
  {
    return ecma_raise_type_error (context_p, ECMA_ERR_GENERATOR_IS_CURRENTLY_UNDER_EXECUTION);
  }

  if (executable_object_p->extended_object.u.cls.u2.executable_obj_flags & ECMA_EXECUTABLE_OBJECT_COMPLETED)
  {
    if (builtin_routine_id != ECMA_GENERATOR_PROTOTYPE_ROUTINE_THROW)
    {
      return ecma_create_iter_result_object (context_p, ECMA_VALUE_UNDEFINED, ECMA_VALUE_TRUE);
    }

    jcontext_raise_exception (context_p, ecma_copy_value (context_p, arguments_list_p[0]));
    return ECMA_VALUE_ERROR;
  }

  return ecma_builtin_generator_prototype_object_do (context_p,
                                                     executable_object_p,
                                                     arguments_list_p[0],
                                                     ECMA_GENERATOR_ROUTINE_TO_OPERATION (builtin_routine_id));
} /* ecma_builtin_generator_prototype_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */
