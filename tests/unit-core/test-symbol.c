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

/* foo string */
#define STRING_FOO ("foo")

/* bar string */
#define STRING_BAR ("bar")

/* Symbol(bar) desciptive string */
#define SYMBOL_DESCIPTIVE_STRING_BAR "Symbol(bar)"

/* bar string desciption */
#define SYMBOL_DESCIPTION_BAR "bar"

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t object = jjs_object (ctx ());

  jjs_value_t empty_symbol_1 = ctx_defer_free (jjs_symbol (ctx ()));
  jjs_value_t empty_symbol_2 = ctx_defer_free (jjs_symbol (ctx ()));

  TEST_ASSERT (jjs_value_is_symbol (ctx (), empty_symbol_1));
  TEST_ASSERT (jjs_value_is_symbol (ctx (), empty_symbol_2));

  jjs_value_t empty_symbol_cmp = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, empty_symbol_1, JJS_KEEP, empty_symbol_2, JJS_KEEP);
  TEST_ASSERT (jjs_value_is_false (ctx (), ctx_defer_free (empty_symbol_cmp)));

  /* Test for that each symbol is unique independently from their descriptor strings */
  jjs_value_t symbol_1 = jjs_symbol_with_description_sz (ctx (), STRING_FOO);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), symbol_1));
  TEST_ASSERT (jjs_value_is_symbol (ctx (), symbol_1));

  jjs_value_t symbol_2 = jjs_symbol_with_description_sz (ctx (), STRING_FOO);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), symbol_2));
  TEST_ASSERT (jjs_value_is_symbol (ctx (), symbol_2));

  jjs_value_t value_1 = jjs_number (ctx (), 1);
  jjs_value_t value_2 = jjs_number (ctx (), 2);

  jjs_value_t result_val = jjs_object_set (ctx (), object, symbol_1, value_1, JJS_KEEP);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result_val));
  TEST_ASSERT (jjs_value_is_true (ctx (), jjs_object_has (ctx (), object, symbol_1)));
  TEST_ASSERT (jjs_value_is_true (ctx (), jjs_object_has_own (ctx (), object, symbol_1)));
  jjs_value_free (ctx (), result_val);

  result_val = jjs_object_set (ctx (), object, symbol_2, value_2, JJS_KEEP);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result_val));
  TEST_ASSERT (jjs_value_is_true (ctx (), jjs_object_has (ctx (), object, symbol_2)));
  TEST_ASSERT (jjs_value_is_true (ctx (), jjs_object_has_own (ctx (), object, symbol_2)));
  jjs_value_free (ctx (), result_val);

  jjs_value_t get_value_1 = jjs_object_get (ctx (), object, symbol_1);
  TEST_ASSERT (jjs_value_as_number (ctx (), get_value_1) == jjs_value_as_number (ctx (), value_1));
  jjs_value_free (ctx (), get_value_1);

  jjs_value_t get_value_2 = jjs_object_get (ctx (), object, symbol_2);
  TEST_ASSERT (jjs_value_as_number (ctx (), get_value_2) == jjs_value_as_number (ctx (), value_2));
  jjs_value_free (ctx (), get_value_2);

  /* Test delete / has_{own}_property */
  jjs_value_t delete_result = jjs_object_delete (ctx (), object, symbol_1);
  TEST_ASSERT (jjs_value_is_true (ctx (), delete_result));
  TEST_ASSERT (!jjs_value_is_true (ctx (), jjs_object_has (ctx (), object, symbol_1)));
  TEST_ASSERT (!jjs_value_is_true (ctx (), jjs_object_has_own (ctx (), object, symbol_1)));

  jjs_value_free (ctx (), delete_result);
  jjs_value_free (ctx (), value_1);
  jjs_value_free (ctx (), symbol_1);

  /* Test {get, define}_own_property_descriptor */
  jjs_property_descriptor_t prop_desc;
  TEST_ASSERT (jjs_object_get_own_prop (ctx (), object, symbol_2, &prop_desc));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_VALUE_DEFINED);
  TEST_ASSERT (value_2 == prop_desc.value);
  TEST_ASSERT (jjs_value_as_number (ctx (), value_2) == jjs_value_as_number (ctx (), prop_desc.value));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_WRITABLE);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_ENUMERABLE);
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_CONFIGURABLE);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_GET_DEFINED));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), prop_desc.getter));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_SET_DEFINED));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), prop_desc.setter));
  jjs_property_descriptor_free (ctx (), &prop_desc);

  /* Modify the descriptor fields */
  prop_desc = jjs_property_descriptor ();
  jjs_value_t value_3 = jjs_string_sz (ctx (), STRING_BAR);

  prop_desc.flags |= JJS_PROP_IS_VALUE_DEFINED | JJS_PROP_IS_WRITABLE_DEFINED | JJS_PROP_IS_ENUMERABLE_DEFINED
                     | JJS_PROP_IS_CONFIGURABLE_DEFINED;
  prop_desc.value = jjs_value_copy (ctx (), value_3);
  jjs_value_t define_own_property_result = jjs_object_define_own_prop (ctx (), object, symbol_2, &prop_desc);
  TEST_ASSERT (jjs_value_is_true (ctx (), define_own_property_result));
  jjs_property_descriptor_free (ctx (), &prop_desc);
  jjs_value_free (ctx (), define_own_property_result);

  /* Check the modified fields */
  TEST_ASSERT (ctx_defer_free (jjs_object_get_own_prop (ctx (), object, symbol_2, &prop_desc)));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_VALUE_DEFINED);
  TEST_ASSERT (value_3 == prop_desc.value);
  TEST_ASSERT (jjs_value_is_string (ctx (), prop_desc.value));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_WRITABLE_DEFINED);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_WRITABLE));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_ENUMERABLE_DEFINED);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_ENUMERABLE));
  TEST_ASSERT (prop_desc.flags & JJS_PROP_IS_CONFIGURABLE_DEFINED);
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_CONFIGURABLE));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_GET_DEFINED));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), prop_desc.getter));
  TEST_ASSERT (!(prop_desc.flags & JJS_PROP_IS_SET_DEFINED));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), prop_desc.setter));
  jjs_property_descriptor_free (ctx (), &prop_desc);

  jjs_value_free (ctx (), value_3);
  jjs_value_free (ctx (), value_2);
  jjs_value_free (ctx (), symbol_2);
  jjs_value_free (ctx (), object);

  /* Test creating symbol with a symbol description */

  jjs_value_t empty_symbol = jjs_symbol_with_description_sz (ctx (), "");
  TEST_ASSERT (!jjs_value_is_exception (ctx (), empty_symbol));
  TEST_ASSERT (jjs_value_is_symbol (ctx (), empty_symbol));

  jjs_value_t symbol_symbol = jjs_symbol_with_description (ctx (), empty_symbol, JJS_KEEP);
  TEST_ASSERT (!jjs_value_is_symbol (ctx (), symbol_symbol));
  TEST_ASSERT (jjs_value_is_exception (ctx (), symbol_symbol));

  jjs_value_t error_obj = jjs_exception_value (ctx (), symbol_symbol, JJS_MOVE);

  TEST_ASSERT (jjs_error_type (ctx (), error_obj) == JJS_ERROR_TYPE);

  jjs_value_free (ctx (), error_obj);
  jjs_value_free (ctx (), empty_symbol);

  /* Test symbol to string operation with symbol argument */

  jjs_value_t bar_symbol = jjs_symbol_with_description_sz (ctx (), STRING_BAR);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), bar_symbol));
  TEST_ASSERT (jjs_value_is_symbol (ctx (), bar_symbol));

  jjs_value_t bar_symbol_string = jjs_symbol_descriptive_string (ctx (), bar_symbol);
  TEST_ASSERT (jjs_value_is_string (ctx (), bar_symbol_string));

  jjs_size_t bar_symbol_string_size = jjs_string_size (ctx (), bar_symbol_string, JJS_ENCODING_CESU8);
  TEST_ASSERT (bar_symbol_string_size == (sizeof (SYMBOL_DESCIPTIVE_STRING_BAR) - 1));
  JJS_VLA (jjs_char_t, str_buff, bar_symbol_string_size);

  jjs_string_to_buffer (ctx (), bar_symbol_string, JJS_ENCODING_CESU8, str_buff, bar_symbol_string_size);
  TEST_ASSERT (memcmp (str_buff, SYMBOL_DESCIPTIVE_STRING_BAR, sizeof (SYMBOL_DESCIPTIVE_STRING_BAR) - 1) == 0);

  jjs_value_free (ctx (), bar_symbol_string);

  /* Test symbol get description operation with string description */
  bar_symbol_string = jjs_symbol_description (ctx (), bar_symbol);
  TEST_ASSERT (jjs_value_is_string (ctx (), bar_symbol_string));

  bar_symbol_string_size = jjs_string_size (ctx (), bar_symbol_string, JJS_ENCODING_CESU8);
  TEST_ASSERT (bar_symbol_string_size == (sizeof (SYMBOL_DESCIPTION_BAR) - 1));

  jjs_string_to_buffer (ctx (), bar_symbol_string, JJS_ENCODING_CESU8, str_buff, bar_symbol_string_size);
  TEST_ASSERT (memcmp (str_buff, STRING_BAR, sizeof (SYMBOL_DESCIPTION_BAR) - 1) == 0);

  jjs_value_free (ctx (), bar_symbol_string);
  jjs_value_free (ctx (), bar_symbol);

  /* Test symbol get description operation with undefined description */
  jjs_value_t undefined_symbol = jjs_symbol_with_description (ctx (), jjs_undefined (ctx ()), JJS_MOVE);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), bar_symbol));
  TEST_ASSERT (jjs_value_is_symbol (ctx (), bar_symbol));

  jjs_value_t undefined_description = jjs_symbol_description (ctx (), undefined_symbol);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), undefined_description));
  jjs_value_free (ctx (), undefined_description);

  /* Test symbol to string operation with non-symbol argument */
  jjs_value_t null_value = jjs_null (ctx ());
  jjs_value_t to_string_value = jjs_symbol_descriptive_string (ctx (), null_value);
  TEST_ASSERT (jjs_value_is_exception (ctx (), to_string_value));

  error_obj = jjs_exception_value (ctx (), to_string_value, JJS_MOVE);

  TEST_ASSERT (jjs_error_type (ctx (), error_obj) == JJS_ERROR_TYPE);

  jjs_value_free (ctx (), error_obj);
  jjs_value_free (ctx (), null_value);

  const jjs_char_t obj_src[] = ""
                                 "({"
                                 "  [Symbol.asyncIterator]: 1,"
                                 "  [Symbol.hasInstance]: 2,"
                                 "  [Symbol.isConcatSpreadable]: 3,"
                                 "  [Symbol.iterator]: 4,"
                                 "  [Symbol.match]: 5,"
                                 "  [Symbol.replace]: 6,"
                                 "  [Symbol.search]: 7,"
                                 "  [Symbol.species]: 8,"
                                 "  [Symbol.split]: 9,"
                                 "  [Symbol.toPrimitive]: 10,"
                                 "  [Symbol.toStringTag]: 11,"
                                 "  [Symbol.unscopables]: 12,"
                                 "  [Symbol.matchAll]: 13,"
                                 "})";

  const char *symbols[] = {
    "asyncIterator", "hasInstance", "isConcatSpreadable", "iterator",    "match",       "replace",  "search",
    "species",       "split",       "toPrimitive",        "toStringTag", "unscopables", "matchAll",
  };

  jjs_value_t obj = jjs_eval (ctx (), obj_src, sizeof (obj_src) - 1, JJS_PARSE_NO_OPTS);
  TEST_ASSERT (jjs_value_is_object (ctx (), obj));

  jjs_value_t global_obj = jjs_current_realm (ctx ());
  jjs_value_t symbol_str = jjs_string_sz (ctx (), "Symbol");
  jjs_value_t builtin_symbol = jjs_object_get (ctx (), global_obj, symbol_str);
  TEST_ASSERT (jjs_value_is_object (ctx (), builtin_symbol));

  double expected = 1.0;
  uint32_t prop_index = 0;

  for (jjs_well_known_symbol_t id = JJS_SYMBOL_ASYNC_ITERATOR; id <= JJS_SYMBOL_MATCH_ALL;
       id++, expected++, prop_index++)
  {
    jjs_value_t well_known_symbol = jjs_symbol_get_well_known (ctx (), id);

    jjs_value_t prop_str = jjs_string_sz (ctx (), symbols[prop_index]);
    jjs_value_t current_global_symbol = jjs_object_get (ctx (), builtin_symbol, prop_str);
    jjs_value_free (ctx (), prop_str);

    jjs_value_t relation = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, well_known_symbol, JJS_KEEP, current_global_symbol, JJS_KEEP);

    TEST_ASSERT (jjs_value_is_boolean (ctx (), relation) && jjs_value_is_true (ctx (), relation));

    jjs_value_free (ctx (), relation);

    jjs_value_t prop_result_wn = jjs_object_get (ctx (), obj, well_known_symbol);
    jjs_value_t prop_result_global = jjs_object_get (ctx (), obj, current_global_symbol);

    TEST_ASSERT (jjs_value_is_number (ctx (), prop_result_wn));
    double number_wn = jjs_value_as_number (ctx (), prop_result_wn);
    TEST_ASSERT (number_wn == expected);

    TEST_ASSERT (jjs_value_is_number (ctx (), prop_result_global));
    double number_global = jjs_value_as_number (ctx (), prop_result_global);
    TEST_ASSERT (number_global == expected);

    jjs_value_free (ctx (), prop_result_global);
    jjs_value_free (ctx (), prop_result_wn);
    jjs_value_free (ctx (), current_global_symbol);
    jjs_value_free (ctx (), well_known_symbol);
  }

  jjs_value_free (ctx (), builtin_symbol);

  /* Deletion of the 'Symbol' builtin makes the well-known symbols unaccessible from JS context
     but the symbols still can be obtained via 'jjs_symbol' */
  const jjs_char_t deleter_src[] = "delete Symbol";

  jjs_value_t deleter = jjs_eval (ctx (), deleter_src, sizeof (deleter_src) - 1, JJS_PARSE_NO_OPTS);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), deleter) && jjs_value_is_true (ctx (), deleter));
  jjs_value_free (ctx (), deleter);

  builtin_symbol = jjs_object_get (ctx (), global_obj, symbol_str);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), builtin_symbol));
  jjs_value_free (ctx (), builtin_symbol);

  expected = 1.0;
  prop_index = 0;

  for (jjs_well_known_symbol_t id = JJS_SYMBOL_ASYNC_ITERATOR; id <= JJS_SYMBOL_MATCH_ALL;
       id++, expected++, prop_index++)
  {
    jjs_value_t well_known_symbol = jjs_symbol_get_well_known (ctx (), id);
    jjs_value_t prop_result_wn = jjs_object_get (ctx (), obj, well_known_symbol);

    TEST_ASSERT (jjs_value_is_number (ctx (), prop_result_wn));
    double number_wn = jjs_value_as_number (ctx (), prop_result_wn);
    TEST_ASSERT (number_wn == expected);

    jjs_value_free (ctx (), prop_result_wn);
    jjs_value_free (ctx (), well_known_symbol);
  }

  jjs_well_known_symbol_t invalid_symbol = (jjs_well_known_symbol_t) (JJS_SYMBOL_MATCH_ALL + 1);
  jjs_value_t invalid_well_known_symbol = jjs_symbol_get_well_known (ctx (), invalid_symbol);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), invalid_well_known_symbol));
  jjs_value_free (ctx (), invalid_well_known_symbol);

  invalid_symbol = (jjs_well_known_symbol_t) (JJS_SYMBOL_ASYNC_ITERATOR - 1);
  invalid_well_known_symbol = jjs_symbol_get_well_known (ctx (), invalid_symbol);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), invalid_well_known_symbol));
  jjs_value_free (ctx (), invalid_well_known_symbol);

  jjs_value_free (ctx (), symbol_str);
  jjs_value_free (ctx (), global_obj);
  jjs_value_free (ctx (), obj);

  ctx_close ();

  return 0;
} /* main */
