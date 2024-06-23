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

static jjs_value_t
custom_to_json (const jjs_call_info_t *call_info_p, /**< call information */
                const jjs_value_t args_p[], /**< arguments list */
                const jjs_length_t args_cnt) /**< arguments length */
{
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_p);
  JJS_UNUSED (args_cnt);

  return jjs_throw_sz (ctx (), JJS_ERROR_URI, "Error");
} /* custom_to_json */

int
main (void)
{
  ctx_open (NULL);

  {
    /* JSON.parse check */
    const jjs_char_t data[] = "{\"name\": \"John\", \"age\": 5}";
    jjs_value_t parsed_json = jjs_json_parse (ctx (), data, sizeof (data) - 1);

    /* Check "name" property values */
    jjs_value_t name_key = jjs_string_sz (ctx (), "name");

    jjs_value_t has_name = jjs_object_has (ctx (), parsed_json, name_key);
    TEST_ASSERT (jjs_value_is_true (ctx (), has_name));
    jjs_value_free (ctx (), has_name);

    jjs_value_t name_value = jjs_object_get (ctx (), parsed_json, name_key);
    TEST_ASSERT (jjs_value_is_string (ctx (), name_value) == true);

    jjs_size_t name_size = jjs_string_size (ctx (), name_value, JJS_ENCODING_CESU8);
    TEST_ASSERT (name_size == 4);
    JJS_VLA (jjs_char_t, name_data, name_size + 1);
    jjs_size_t copied = jjs_string_to_buffer (ctx (), name_value, JJS_ENCODING_CESU8, name_data, name_size);
    name_data[name_size] = '\0';

    jjs_value_free (ctx (), name_value);

    TEST_ASSERT (copied == name_size);
    TEST_ASSERT_STR ("John", name_data);
    jjs_value_free (ctx (), name_key);

    /* Check "age" property values */
    jjs_value_t age_key = jjs_string_sz (ctx (), "age");

    jjs_value_t has_age = jjs_object_has (ctx (), parsed_json, age_key);
    TEST_ASSERT (jjs_value_is_true (ctx (), has_age));
    jjs_value_free (ctx (), has_age);

    jjs_value_t age_value = jjs_object_get (ctx (), parsed_json, age_key);
    TEST_ASSERT (jjs_value_is_number (ctx (), age_value) == true);
    TEST_ASSERT (jjs_value_as_number (ctx (), age_value) == 5.0);

    jjs_value_free (ctx (), age_value);
    jjs_value_free (ctx (), age_key);

    jjs_value_free (ctx (), parsed_json);
  }

  /* JSON.parse cesu-8 / utf-8 encoded string */
  {
    jjs_char_t cesu8[] = "{\"ch\": \"\xED\xA0\x83\xED\xB2\x9F\"}";
    jjs_char_t utf8[] = "{\"ch\": \"\xF0\x90\xB2\x9F\"}";

    jjs_value_t parsed_cesu8 = jjs_json_parse (ctx (), cesu8, sizeof (cesu8) - 1);
    jjs_value_t parsed_utf8 = jjs_json_parse (ctx (), utf8, sizeof (utf8) - 1);

    jjs_value_t key = jjs_string_sz (ctx (), "ch");
    jjs_value_t char_cesu8 = jjs_object_get (ctx (), parsed_cesu8, key);
    jjs_value_t char_utf8 = jjs_object_get (ctx (), parsed_utf8, key);
    jjs_value_free (ctx (), key);

    TEST_ASSERT (jjs_value_to_boolean (ctx (), jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, char_cesu8, JJS_MOVE, char_utf8, JJS_MOVE)));
    jjs_value_free (ctx (), parsed_cesu8);
    jjs_value_free (ctx (), parsed_utf8);
  }

  /* JSON.parse error checks */
  {
    jjs_value_t parsed_json = jjs_json_parse (ctx (), (const jjs_char_t *) "", 0);
    TEST_ASSERT (jjs_value_is_exception (ctx (), parsed_json));
    TEST_ASSERT (jjs_error_type (ctx (), parsed_json) == JJS_ERROR_SYNTAX);
    jjs_value_free (ctx (), parsed_json);
  }

  {
    jjs_value_t parsed_json = jjs_json_parse (ctx (), (const jjs_char_t *) "-", 1);
    TEST_ASSERT (jjs_value_is_exception (ctx (), parsed_json));
    TEST_ASSERT (jjs_error_type (ctx (), parsed_json) == JJS_ERROR_SYNTAX);
    jjs_value_free (ctx (), parsed_json);
  }

  /* JSON.stringify check */
  {
    jjs_value_t obj = jjs_object (ctx ());
    /* Fill "obj" with data */
    {
      jjs_value_t name_key = jjs_string_sz (ctx (), "name");
      jjs_value_t name_value = jjs_string_sz (ctx (), "John");
      jjs_value_t name_set = jjs_object_set (ctx (), obj, name_key, name_value);
      TEST_ASSERT (!jjs_value_is_exception (ctx (), name_set));
      TEST_ASSERT (jjs_value_is_boolean (ctx (), name_set));
      TEST_ASSERT (jjs_value_is_true (ctx (), name_set));
      jjs_value_free (ctx (), name_key);
      jjs_value_free (ctx (), name_value);
      jjs_value_free (ctx (), name_set);
    }
    {
      jjs_value_t age_key = jjs_string_sz (ctx (), "age");
      jjs_value_t age_value = jjs_number (ctx (), 32);
      jjs_value_t age_set = jjs_object_set (ctx (), obj, age_key, age_value);
      TEST_ASSERT (!jjs_value_is_exception (ctx (), age_set));
      TEST_ASSERT (jjs_value_is_boolean (ctx (), age_set));
      TEST_ASSERT (jjs_value_is_true (ctx (), age_set));
      jjs_value_free (ctx (), age_key);
      jjs_value_free (ctx (), age_value);
      jjs_value_free (ctx (), age_set);
    }

    jjs_value_t json_string = jjs_json_stringify (ctx (), obj);
    TEST_ASSERT (jjs_value_is_string (ctx (), json_string));

    jjs_value_free (ctx (), obj);

    const char check_value[] = "{\"name\":\"John\",\"age\":32}";
    jjs_size_t json_size = jjs_string_size (ctx (), json_string, JJS_ENCODING_CESU8);
    TEST_ASSERT (json_size == strlen (check_value));
    JJS_VLA (jjs_char_t, json_data, json_size + 1);
    jjs_string_to_buffer (ctx (), json_string, JJS_ENCODING_CESU8, json_data, json_size);
    json_data[json_size] = '\0';

    TEST_ASSERT_STR (check_value, json_data);

    jjs_value_free (ctx (), json_string);
  }

  /* Custom "toJSON" invocation test */
  {
    jjs_value_t obj = jjs_object (ctx ());
    /* Fill "obj" with data */
    {
      jjs_value_t name_key = jjs_string_sz (ctx (), "toJSON");
      jjs_value_t name_value = jjs_function_external (ctx (), custom_to_json);
      jjs_value_t name_set = jjs_object_set (ctx (), obj, name_key, name_value);
      TEST_ASSERT (!jjs_value_is_exception (ctx (), name_set));
      TEST_ASSERT (jjs_value_is_boolean (ctx (), name_set));
      TEST_ASSERT (jjs_value_is_true (ctx (), name_set));
      jjs_value_free (ctx (), name_key);
      jjs_value_free (ctx (), name_value);
      jjs_value_free (ctx (), name_set);
    }

    jjs_value_t json_string = jjs_json_stringify (ctx (), obj);
    TEST_ASSERT (jjs_value_is_exception (ctx (), json_string));
    TEST_ASSERT (jjs_error_type (ctx (), json_string) == JJS_ERROR_URI);

    jjs_value_free (ctx (), json_string);
    jjs_value_free (ctx (), obj);
  }

  ctx_close ();

  return 0;
} /* main */
