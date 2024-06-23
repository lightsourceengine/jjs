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

#include "jjs-test.h"

typedef struct
{
  jjs_object_type_t type_info;
  jjs_value_t value;
  bool active;
} test_entry_t;

#define ENTRY(TYPE, VALUE) \
  {                        \
    TYPE, VALUE, true      \
  }
#define ENTRY_IF(TYPE, VALUE, FEATURE)           \
  {                                              \
    TYPE, VALUE, jjs_feature_enabled (FEATURE) \
  }
#define EVALUATE(BUFF) (jjs_eval (ctx (), (BUFF), sizeof ((BUFF)) - 1, JJS_PARSE_NO_OPTS))
#define PARSE(OPTS)    (jjs_parse (ctx (), (const jjs_char_t *) "", 0, (OPTS)))
static jjs_value_t
test_ext_function (const jjs_call_info_t *call_info_p, /**< call information */
                   const jjs_value_t args_p[], /**< array of arguments */
                   const jjs_length_t args_cnt) /**< number of arguments */
{
  (void) call_info_p;
  (void) args_p;
  (void) args_cnt;
  return jjs_boolean (ctx (), true);
} /* test_ext_function */

static jjs_object_type_t
test_namespace (const jjs_parse_options_t module_parse_options) /** module options */
{
  jjs_value_t module = jjs_parse_sz (ctx (), "", &module_parse_options);
  jjs_value_t module_linked = jjs_module_link (ctx (), module, NULL, NULL);
  jjs_object_type_t namespace = jjs_module_namespace (ctx (), module);
  jjs_value_free (ctx (), module_linked);
  jjs_value_free (ctx (), module);
  return namespace;
} /* test_namespace */

static jjs_value_t
test_dataview (void)
{
  return jjs_dataview (ctx (), jjs_arraybuffer (ctx (), 10), JJS_MOVE, 0, 4);
} /* test_dataview */

int
main (void)
{
  ctx_open (NULL);

  const jjs_char_t proxy_object[] = "new Proxy({}, {})";
  const jjs_char_t typedarray_object[] = "new Uint8Array()";
  const jjs_char_t container_object[] = "new Map()";
  const jjs_char_t iterator_object[] = "[1, 2, 3].values()";
  const jjs_char_t arrow_function[] = "_ => 5";
  const jjs_char_t async_arrow_function[] = "async _ => 5";
  const jjs_char_t generator_function[] = "function *f() {}; f";
  const jjs_char_t async_generator_function[] = "async function *f() {}; f";
  const jjs_char_t getter_function[] = "Object.getOwnPropertyDescriptor({get a(){}}, 'a').get";
  const jjs_char_t setter_function[] = "Object.getOwnPropertyDescriptor({set a(b){}}, 'a').set";
  const jjs_char_t method_function[] = "Object.getOwnPropertyDescriptor({a(){}}, 'a').value";

  const jjs_char_t symbol_object[] = "new Object(Symbol('foo'))";
  const jjs_char_t generator_object[] = "function *f() { yield 5 }; f()";
  const jjs_char_t bigint_object[] = "Object(5n)";

  const jjs_char_t builtin_function[] = "Object";
  const jjs_char_t simple_function[] = "function f() {}; f";
  const jjs_char_t bound_function[] = "function f() {}; f.bind(1,2)";
  const jjs_char_t mapped_arguments[] = "function f(a, b) { return arguments; }; f()";
  const jjs_char_t unmapped_arguments[] = "function f(a, b) {'use strict'; return arguments; }; f()";
  const jjs_char_t boolean_object[] = "new Boolean(true)";
  const jjs_char_t date_object[] = "new Date()";
  const jjs_char_t number_object[] = "new Number(5)";
  const jjs_char_t regexp_object[] = "new RegExp()";
  const jjs_char_t string_object[] = "new String('foo')";
  const jjs_char_t weak_ref_object[] = "new WeakRef({})";
  const jjs_char_t error_object[] = "new Error()";

  jjs_parse_options_t module_parse_options = {
    .parse_module = true,
  };

  test_entry_t entries[] = {
    ENTRY (JJS_OBJECT_TYPE_NONE, jjs_number (ctx (), -33.0)),
    ENTRY (JJS_OBJECT_TYPE_NONE, jjs_boolean (ctx (), true)),
    ENTRY (JJS_OBJECT_TYPE_NONE, jjs_undefined (ctx ())),
    ENTRY (JJS_OBJECT_TYPE_NONE, jjs_null (ctx ())),
    ENTRY (JJS_OBJECT_TYPE_NONE, jjs_string_sz (ctx (), "foo")),
    ENTRY (JJS_OBJECT_TYPE_NONE, jjs_throw_sz (ctx (), JJS_ERROR_TYPE, "error")),

    ENTRY (JJS_OBJECT_TYPE_GENERIC, jjs_object (ctx ())),
    ENTRY_IF (JJS_OBJECT_TYPE_MODULE_NAMESPACE, test_namespace (module_parse_options), JJS_FEATURE_MODULE),
    ENTRY (JJS_OBJECT_TYPE_ARRAY, jjs_array (ctx (), 10)),

    ENTRY_IF (JJS_OBJECT_TYPE_PROXY, EVALUATE (proxy_object), JJS_FEATURE_PROXY),
    ENTRY_IF (JJS_OBJECT_TYPE_TYPEDARRAY, EVALUATE (typedarray_object), JJS_FEATURE_TYPEDARRAY),
    ENTRY_IF (JJS_OBJECT_TYPE_CONTAINER, EVALUATE (container_object), JJS_FEATURE_MAP),
    ENTRY_IF (JJS_OBJECT_TYPE_ITERATOR, EVALUATE (iterator_object), JJS_FEATURE_SYMBOL),

    ENTRY (JJS_OBJECT_TYPE_SCRIPT, PARSE (NULL)),
    ENTRY_IF (JJS_OBJECT_TYPE_MODULE, PARSE (&module_parse_options), JJS_FEATURE_MODULE),
    ENTRY_IF (JJS_OBJECT_TYPE_PROMISE, jjs_promise (ctx ()), JJS_FEATURE_PROMISE),
    ENTRY_IF (JJS_OBJECT_TYPE_DATAVIEW, test_dataview (), JJS_FEATURE_DATAVIEW),
    ENTRY_IF (JJS_OBJECT_TYPE_FUNCTION, EVALUATE (arrow_function), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_OBJECT_TYPE_FUNCTION, EVALUATE (async_arrow_function), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_OBJECT_TYPE_FUNCTION, EVALUATE (generator_function), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_OBJECT_TYPE_FUNCTION, EVALUATE (async_generator_function), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_OBJECT_TYPE_FUNCTION, EVALUATE (method_function), JJS_FEATURE_SYMBOL),
    ENTRY (JJS_OBJECT_TYPE_FUNCTION, EVALUATE (builtin_function)),
    ENTRY (JJS_OBJECT_TYPE_FUNCTION, EVALUATE (simple_function)),
    ENTRY (JJS_OBJECT_TYPE_FUNCTION, EVALUATE (bound_function)),
    ENTRY (JJS_OBJECT_TYPE_FUNCTION, jjs_function_external (ctx (), test_ext_function)),
    ENTRY (JJS_OBJECT_TYPE_FUNCTION, EVALUATE (getter_function)),
    ENTRY (JJS_OBJECT_TYPE_FUNCTION, EVALUATE (setter_function)),
    ENTRY_IF (JJS_OBJECT_TYPE_ERROR, EVALUATE (error_object), JJS_FEATURE_ERROR_MESSAGES),
    ENTRY_IF (JJS_OBJECT_TYPE_ARRAYBUFFER, jjs_arraybuffer (ctx (), 10), JJS_FEATURE_TYPEDARRAY),

    ENTRY (JJS_OBJECT_TYPE_ARGUMENTS, EVALUATE (mapped_arguments)),
    ENTRY (JJS_OBJECT_TYPE_ARGUMENTS, EVALUATE (unmapped_arguments)),
    ENTRY (JJS_OBJECT_TYPE_BOOLEAN, EVALUATE (boolean_object)),
    ENTRY (JJS_OBJECT_TYPE_DATE, EVALUATE (date_object)),
    ENTRY (JJS_OBJECT_TYPE_NUMBER, EVALUATE (number_object)),
    ENTRY (JJS_OBJECT_TYPE_REGEXP, EVALUATE (regexp_object)),
    ENTRY (JJS_OBJECT_TYPE_STRING, EVALUATE (string_object)),
    ENTRY_IF (JJS_OBJECT_TYPE_SYMBOL, EVALUATE (symbol_object), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_OBJECT_TYPE_GENERATOR, EVALUATE (generator_object), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_OBJECT_TYPE_BIGINT, EVALUATE (bigint_object), JJS_FEATURE_BIGINT),
    ENTRY_IF (JJS_OBJECT_TYPE_WEAKREF, EVALUATE (weak_ref_object), JJS_FEATURE_WEAKREF),
  };

  for (size_t idx = 0; idx < sizeof (entries) / sizeof (entries[0]); idx++)
  {
    jjs_object_type_t type_info = jjs_object_type (ctx (), entries[idx].value);

    TEST_ASSERT (!entries[idx].active || type_info == entries[idx].type_info);
    jjs_value_free (ctx (), entries[idx].value);
  }

  if (jjs_feature_enabled (JJS_FEATURE_REALM))
  {
    jjs_value_t new_realm = jjs_realm (ctx ());
    jjs_object_type_t new_realm_object_type = jjs_object_type (ctx (), new_realm);
    TEST_ASSERT (new_realm_object_type == JJS_OBJECT_TYPE_GENERIC);

    jjs_value_t old_realm = jjs_set_realm (ctx (), new_realm);
    jjs_object_type_t old_realm_object_type = jjs_object_type (ctx (), old_realm);
    TEST_ASSERT (old_realm_object_type == JJS_OBJECT_TYPE_GENERIC);

    jjs_set_realm (ctx (), old_realm);

    jjs_value_free (ctx (), new_realm);
  }

  ctx_close ();

  return 0;
} /* main */
