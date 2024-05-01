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
  jjs_iterator_type_t type_info;
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

int
main (void)
{
  ctx_open (NULL);

  const jjs_char_t array_iterator_keys[] = "[1, 2, 3].keys()";
  const jjs_char_t array_iterator_values[] = "[1, 2, 3].values()";
  const jjs_char_t array_iterator_entries[] = "[1, 2, 3].entries()";
  const jjs_char_t array_iterator_symbol_iterator[] = "([1, 2, 3])[Symbol.iterator]()";

  const jjs_char_t typedarray_iterator_keys[] = "new Uint8Array([1, 2, 3]).keys()";
  const jjs_char_t typedarray_iterator_values[] = "new Uint8Array([1, 2, 3]).values()";
  const jjs_char_t typedarray_iterator_entries[] = "new Uint8Array([1, 2, 3]).entries()";
  const jjs_char_t typedarray_iterator_symbol_iterator[] = "new Uint8Array([1, 2, 3])[Symbol.iterator]()";

  const jjs_char_t string_symbol_iterator[] = "('foo')[Symbol.iterator]()";

  const jjs_char_t map_iterator_keys[] = "new Map([1, 2, 3].entries()).keys()";
  const jjs_char_t map_iterator_values[] = "new Map([1, 2, 3].entries()).values()";
  const jjs_char_t map_iterator_entries[] = "new Map([1, 2, 3].entries()).entries()";
  const jjs_char_t map_iterator_symbol_iterator[] = "new Map([1, 2, 3].entries())[Symbol.iterator]()";

  const jjs_char_t set_iterator_keys[] = "new Set([1, 2, 3]).keys()";
  const jjs_char_t set_iterator_values[] = "new Set([1, 2, 3]).values()";
  const jjs_char_t set_iterator_entries[] = "new Set([1, 2, 3]).entries()";
  const jjs_char_t set_iterator_symbol_iterator[] = "new Set([1, 2, 3])[Symbol.iterator]()";

  test_entry_t entries[] = {
    ENTRY (JJS_ITERATOR_TYPE_NONE, jjs_number (ctx (), -33.0)),
    ENTRY (JJS_ITERATOR_TYPE_NONE, jjs_boolean (ctx (), true)),
    ENTRY (JJS_ITERATOR_TYPE_NONE, jjs_undefined (ctx ())),
    ENTRY (JJS_ITERATOR_TYPE_NONE, jjs_null (ctx ())),
    ENTRY (JJS_ITERATOR_TYPE_NONE, jjs_string_sz (ctx (), "foo")),
    ENTRY (JJS_ITERATOR_TYPE_NONE, jjs_throw_sz (ctx (), JJS_ERROR_TYPE, "error")),

    ENTRY (JJS_ITERATOR_TYPE_NONE, jjs_object (ctx ())),
    ENTRY (JJS_ITERATOR_TYPE_NONE, jjs_array (ctx (), 10)),

    ENTRY_IF (JJS_ITERATOR_TYPE_ARRAY, EVALUATE (array_iterator_keys), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_ITERATOR_TYPE_ARRAY, EVALUATE (array_iterator_values), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_ITERATOR_TYPE_ARRAY, EVALUATE (array_iterator_entries), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_ITERATOR_TYPE_ARRAY, EVALUATE (array_iterator_symbol_iterator), JJS_FEATURE_SYMBOL),

    ENTRY_IF (JJS_ITERATOR_TYPE_ARRAY, EVALUATE (typedarray_iterator_keys), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_ITERATOR_TYPE_ARRAY, EVALUATE (typedarray_iterator_values), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_ITERATOR_TYPE_ARRAY, EVALUATE (typedarray_iterator_entries), JJS_FEATURE_SYMBOL),
    ENTRY_IF (JJS_ITERATOR_TYPE_ARRAY, EVALUATE (typedarray_iterator_symbol_iterator), JJS_FEATURE_SYMBOL),

    ENTRY_IF (JJS_ITERATOR_TYPE_STRING, EVALUATE (string_symbol_iterator), JJS_FEATURE_SYMBOL),

    ENTRY_IF (JJS_ITERATOR_TYPE_MAP, EVALUATE (map_iterator_keys), JJS_FEATURE_MAP),
    ENTRY_IF (JJS_ITERATOR_TYPE_MAP, EVALUATE (map_iterator_values), JJS_FEATURE_MAP),
    ENTRY_IF (JJS_ITERATOR_TYPE_MAP, EVALUATE (map_iterator_entries), JJS_FEATURE_MAP),
    ENTRY_IF (JJS_ITERATOR_TYPE_MAP, EVALUATE (map_iterator_symbol_iterator), JJS_FEATURE_MAP),

    ENTRY_IF (JJS_ITERATOR_TYPE_SET, EVALUATE (set_iterator_keys), JJS_FEATURE_SET),
    ENTRY_IF (JJS_ITERATOR_TYPE_SET, EVALUATE (set_iterator_values), JJS_FEATURE_SET),
    ENTRY_IF (JJS_ITERATOR_TYPE_SET, EVALUATE (set_iterator_entries), JJS_FEATURE_SET),
    ENTRY_IF (JJS_ITERATOR_TYPE_SET, EVALUATE (set_iterator_symbol_iterator), JJS_FEATURE_SET),
  };

  for (size_t idx = 0; idx < sizeof (entries) / sizeof (entries[0]); idx++)
  {
    jjs_iterator_type_t type_info = jjs_iterator_type (ctx (), entries[idx].value);
    TEST_ASSERT (!entries[idx].active || type_info == entries[idx].type_info);
    jjs_value_free (ctx (), entries[idx].value);
  }

  ctx_close ();

  return 0;
} /* main */
