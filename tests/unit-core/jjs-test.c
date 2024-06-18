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

#include "jjs-test.h"

#include "jjs-context-init.h"
#include "jjs-util.h"

#include "ecma-objects.h"

typedef struct
{
  jjs_context_t *context_p;
  jjs_value_t stored_values[1024];
  size_t stored_values_index;
} ctx_stack_entry;

static ctx_stack_entry context_stack[3] = { 0 };
static int32_t context_stack_index = -1;

static void
test_failure (void)
{
  //  exit (1);
  abort ();
}

jjs_context_t *
ctx_open (const jjs_context_options_t *options)
{
  int32_t next = context_stack_index + 1;

  TEST_ASSERT (next < (int32_t) JJS_ARRAY_SIZE (context_stack));
  TEST_ASSERT (jjs_context_new (options, &context_stack[next].context_p) == JJS_STATUS_OK);

  context_stack[next].stored_values_index = 0;

  context_stack_index = next;

  return context_stack[context_stack_index].context_p;
}
void
ctx_close (void)
{
  TEST_ASSERT (context_stack_index >= 0);

  ctx_stack_entry *e = &context_stack[context_stack_index];

  for (size_t i = 0; i < e->stored_values_index; i++)
  {
    jjs_value_free (e->context_p, e->stored_values[i]);
  }

  jjs_context_free (e->context_p);
  context_stack_index--;
}

jjs_context_t *
ctx (void)
{
  TEST_ASSERT (context_stack_index >= 0);
  return context_stack[context_stack_index].context_p;
}

jjs_value_t
ctx_defer_free (jjs_value_t value)
{
  TEST_ASSERT (context_stack_index >= 0);

  ctx_stack_entry *e = &context_stack[context_stack_index];

  TEST_ASSERT (e->stored_values_index < JJS_ARRAY_SIZE (context_stack->stored_values));
  e->stored_values[e->stored_values_index++] = value;

  return value;
}

jjs_value_t
ctx_global (void)
{
  return ctx_defer_free (jjs_current_realm (ctx ()));
}

jjs_value_t
ctx_cstr (const char *s)
{
  return ctx_defer_free (jjs_string_utf8_sz (ctx (), s));
}

jjs_value_t
ctx_number (double n)
{
  return ctx_defer_free (jjs_number (ctx (), n));
}

jjs_value_t
ctx_null (void)
{
  return jjs_null (ctx ());
}

jjs_value_t
ctx_undefined (void)
{
  return jjs_undefined (ctx ());
}

jjs_value_t
ctx_object (void)
{
  return ctx_defer_free (jjs_object (ctx ()));
}

jjs_value_t
ctx_array (jjs_length_t len)
{
  return ctx_defer_free (jjs_array (ctx (), len));
}

jjs_value_t
ctx_boolean (bool value)
{
  return jjs_boolean (ctx (), value);
}

jjs_value_t
ctx_symbol (const char *description)
{
  return ctx_defer_free (jjs_symbol_with_description (ctx (), ctx_cstr (description)));
}

jjs_context_t *
ctx_bootstrap (const jjs_context_options_t *options)
{
  jjs_context_t *context_p;

  TEST_ASSERT (jjs_context_init (options, jjs_util_system_allocator_ptr (), &context_p) == JJS_STATUS_OK);

  return context_p;
}

void
ctx_bootstrap_cleanup (jjs_context_t *context_p)
{
  jjs_context_cleanup (context_p);
}

void
ctx_assert_strict_equals (jjs_value_t actual, jjs_value_t expected)
{
  if (expected == ECMA_VALUE_EMPTY)
  {
    if (expected != actual)
    {
      jjs_log_fmt (ctx (), JJS_LOG_LEVEL_ERROR, "expected ECMA_VALUE_EMPTY got: {}\n", actual);
      test_failure ();
    }

    return;
  }

  if (jjs_value_is_exception (ctx (), actual))
  {
    jjs_log_fmt (ctx (), JJS_LOG_LEVEL_ERROR, "Uncaught exception: {}\n", actual);
    test_failure ();
    return;
  }

  jjs_value_t op_result = ctx_defer_free (jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, expected, actual));

  if (jjs_value_is_exception (ctx (), op_result))
  {
    jjs_log_fmt (ctx (), JJS_LOG_LEVEL_ERROR, "strict equals exception: {}\n", expected, actual);
    test_failure ();
  }

  if (!jjs_value_is_true (ctx (), op_result))
  {
    jjs_log_fmt (ctx (),
                 JJS_LOG_LEVEL_ERROR,
                 "strict equals assertion failed: expected {} to equal {}\n",
                 expected,
                 actual);
    test_failure ();
  }
}

bool
strict_equals (jjs_context_t *context_p, jjs_value_t a, jjs_value_t b)
{
  jjs_value_t op_result = jjs_binary_op (context_p, JJS_BIN_OP_STRICT_EQUAL, a, b);
  bool result = jjs_value_is_true (context_p, op_result);

  jjs_value_free (context_p, op_result);

  return result;
}

bool
strict_equals_cstr (jjs_context_t *context_p, jjs_value_t a, const char *b)
{
  jjs_value_t b_value = jjs_string_sz (context_p, b);
  bool result = strict_equals (context_p, a, b_value);

  jjs_value_free (context_p, b_value);

  return result;
}

bool
strict_equals_int32 (jjs_context_t *context_p, jjs_value_t a, int32_t b)
{
  jjs_value_t b_value = jjs_number_from_int32 (context_p, b);
  bool result = strict_equals (context_p, a, b_value);

  jjs_value_free (context_p, b_value);

  return result;
}
