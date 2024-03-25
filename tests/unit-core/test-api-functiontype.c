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

#include "jjs-port.h"
#include "jjs.h"

#include "test-common.h"

typedef struct
{
  jjs_function_type_t type_info;
  jjs_value_t value;
  bool active;
  bool is_async;
} test_entry_t;

#define ENTRY(TYPE, VALUE)   \
  {                          \
    TYPE, VALUE, true, false \
  }
#define ENTRY_IF(TYPE, VALUE, FEATURE, ASYNC)           \
  {                                                     \
    TYPE, VALUE, jjs_feature_enabled (FEATURE), ASYNC \
  }
#define EVALUATE(BUFF) (jjs_eval ((BUFF), sizeof ((BUFF)) - 1, JJS_PARSE_NO_OPTS))
static jjs_value_t
test_ext_function (const jjs_call_info_t *call_info_p, /**< call information */
                   const jjs_value_t args_p[], /**< array of arguments */
                   const jjs_length_t args_cnt) /**< number of arguments */
{
  (void) call_info_p;
  (void) args_p;
  (void) args_cnt;
  return jjs_boolean (true);
} /* test_ext_function */

int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  const jjs_char_t arrow_function[] = "_ => 5";
  const jjs_char_t async_arrow_function[] = "async _ => 5";
  const jjs_char_t generator_function[] = "function *f() {}; f";
  const jjs_char_t async_generator_function[] = "async function *f() {}; f";
  const jjs_char_t getter_function[] = "Object.getOwnPropertyDescriptor({get a(){}}, 'a').get";
  const jjs_char_t setter_function[] = "Object.getOwnPropertyDescriptor({set a(b){}}, 'a').set";
  const jjs_char_t method_function[] = "Object.getOwnPropertyDescriptor({a(){}}, 'a').value";

  const jjs_char_t builtin_function[] = "Object";
  const jjs_char_t simple_function[] = "function f() {}; f";
  const jjs_char_t bound_function[] = "function f() {}; f.bind(1,2)";

  test_entry_t entries[] = {
    ENTRY (JJS_FUNCTION_TYPE_NONE, jjs_number (-33.0)),
    ENTRY (JJS_FUNCTION_TYPE_NONE, jjs_boolean (true)),
    ENTRY (JJS_FUNCTION_TYPE_NONE, jjs_undefined ()),
    ENTRY (JJS_FUNCTION_TYPE_NONE, jjs_null ()),
    ENTRY (JJS_FUNCTION_TYPE_NONE, jjs_string_sz ("foo")),
    ENTRY (JJS_FUNCTION_TYPE_NONE, jjs_throw_sz (JJS_ERROR_TYPE, "error")),

    ENTRY (JJS_FUNCTION_TYPE_NONE, jjs_object ()),
    ENTRY (JJS_FUNCTION_TYPE_NONE, jjs_array (10)),

    ENTRY_IF (JJS_FUNCTION_TYPE_ARROW, EVALUATE (arrow_function), JJS_FEATURE_SYMBOL, false),
    ENTRY_IF (JJS_FUNCTION_TYPE_ARROW, EVALUATE (async_arrow_function), JJS_FEATURE_SYMBOL, true),
    ENTRY_IF (JJS_FUNCTION_TYPE_GENERATOR, EVALUATE (generator_function), JJS_FEATURE_SYMBOL, false),
    ENTRY_IF (JJS_FUNCTION_TYPE_GENERATOR, EVALUATE (async_generator_function), JJS_FEATURE_SYMBOL, true),
    ENTRY_IF (JJS_FUNCTION_TYPE_GENERIC, EVALUATE (method_function), JJS_FEATURE_SYMBOL, false),
    ENTRY (JJS_FUNCTION_TYPE_GENERIC, EVALUATE (builtin_function)),
    ENTRY (JJS_FUNCTION_TYPE_GENERIC, EVALUATE (simple_function)),
    ENTRY (JJS_FUNCTION_TYPE_BOUND, EVALUATE (bound_function)),
    ENTRY (JJS_FUNCTION_TYPE_GENERIC, jjs_function_external (test_ext_function)),
    ENTRY (JJS_FUNCTION_TYPE_ACCESSOR, EVALUATE (getter_function)),
    ENTRY (JJS_FUNCTION_TYPE_ACCESSOR, EVALUATE (setter_function)),
  };

  for (size_t idx = 0; idx < sizeof (entries) / sizeof (entries[0]); idx++)
  {
    jjs_function_type_t type_info = jjs_function_type (entries[idx].value);
    TEST_ASSERT (!entries[idx].active
                 || (type_info == entries[idx].type_info
                     && jjs_value_is_async_function (entries[idx].value) == entries[idx].is_async));
    jjs_value_free (entries[idx].value);
  }

  jjs_cleanup ();

  return 0;
} /* main */
