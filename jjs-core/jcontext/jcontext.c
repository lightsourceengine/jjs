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

#include "jcontext.h"

/** \addtogroup context Context
 * @{
 */

/**
 * Check the existence of the ECMA_STATUS_EXCEPTION flag.
 *
 * @return true - if the flag is set
 *         false - otherwise
 */
extern inline bool JJS_ATTR_ALWAYS_INLINE
jcontext_has_pending_exception (jjs_context_t *context_p)
{
  return context_p->status_flags & ECMA_STATUS_EXCEPTION;
} /* jcontext_has_pending_exception */

/**
 * Check the existence of the ECMA_STATUS_ABORT flag.
 *
 * @return true - if the flag is set
 *         false - otherwise
 */
extern inline bool JJS_ATTR_ALWAYS_INLINE
jcontext_has_pending_abort (jjs_context_t *context_p) /**< JJS context */
{
  return context_p->status_flags & ECMA_STATUS_ABORT;
} /* jcontext_has_pending_abort */

/**
 * Set the abort flag for the context.
 */
extern inline void JJS_ATTR_ALWAYS_INLINE
jcontext_set_abort_flag (jjs_context_t *context_p, /**< JJS context */
                         bool is_abort) /**< true - if the abort flag should be set
                                         *   false - if the abort flag should be removed */
{
  JJS_ASSERT (jcontext_has_pending_exception (context_p));

  if (is_abort)
  {
    context_p->status_flags |= ECMA_STATUS_ABORT;
  }
  else
  {
    context_p->status_flags &= (uint32_t) ~ECMA_STATUS_ABORT;
  }
} /* jcontext_set_abort_flag */

/**
 * Set the exception flag for the context.
 */
extern inline void JJS_ATTR_ALWAYS_INLINE
jcontext_set_exception_flag (jjs_context_t *context_p, /**< JJS context */
                             bool is_exception) /**< true - if the exception flag should be set
                                                 *   false - if the exception flag should be removed */
{
  if (is_exception)
  {
    context_p->status_flags |= ECMA_STATUS_EXCEPTION;
  }
  else
  {
    context_p->status_flags &= (uint32_t) ~ECMA_STATUS_EXCEPTION;
  }
} /* jcontext_set_exception_flag */

/**
 * Raise exception from the given error value.
 */
extern inline void JJS_ATTR_ALWAYS_INLINE
jcontext_raise_exception (jjs_context_t *context_p, /**< JJS context */
                          ecma_value_t error) /**< error to raise */
{
  JJS_ASSERT (!jcontext_has_pending_exception (context_p));
  JJS_ASSERT (!jcontext_has_pending_abort (context_p));

  context_p->error_value = error;
  jcontext_set_exception_flag (context_p, true);
} /* jcontext_raise_exception */

/**
 * Release the current exception/abort of the context.
 */
void
jcontext_release_exception (jjs_context_t *context_p) /**< JJS context */
{
  JJS_ASSERT (jcontext_has_pending_exception (context_p));

  ecma_free_value (jcontext_take_exception (context_p));
} /* jcontext_release_exception */

/**
 * Take the current exception/abort of context.
 *
 * @return current exception as an ecma-value
 */
ecma_value_t
jcontext_take_exception (jjs_context_t *context_p)
{
  JJS_ASSERT (jcontext_has_pending_exception (context_p));

  context_p->status_flags &= (uint32_t) ~(ECMA_STATUS_EXCEPTION
#if JJS_VM_THROW
                                               | ECMA_STATUS_ERROR_THROWN
#endif /* JJS_VM_THROW */
                                               | ECMA_STATUS_ABORT);
  return context_p->error_value;
} /* jcontext_take_exception */

/**
 * Global context.
 */
jjs_context_t jjs_global_context = {0};

/**
 * @}
 */
