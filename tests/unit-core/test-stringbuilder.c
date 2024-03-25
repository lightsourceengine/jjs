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

#include "ecma-helpers.h"
#include "ecma-init-finalize.h"

#include "jjs-context-init.h"
#include "lit-char-helpers.h"
#include "lit-strings.h"
#include "test-common.h"

int
main (void)
{
  TEST_INIT ();
  TEST_CONTEXT_INIT ();

  jmem_init ();
  ecma_init ();

  {
    static const lit_utf8_byte_t string_data[] = "A simple string";

    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    ecma_stringbuilder_append_raw (&builder, string_data, sizeof (string_data) - 1);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    ecma_string_t *str_p = ecma_new_ecma_string_from_ascii (string_data, sizeof (string_data) - 1);
    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
  }

  {
    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    ecma_stringbuilder_append_magic (&builder, LIT_MAGIC_STRING_STRING);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    ecma_string_t *str_p = ecma_get_magic_string (LIT_MAGIC_STRING_STRING);
    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
  }

  {
    static const lit_utf8_byte_t string_data[] = "a";

    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    ecma_stringbuilder_append_char (&builder, LIT_CHAR_LOWERCASE_A);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    ecma_string_t *str_p = ecma_new_ecma_string_from_ascii (string_data, sizeof (string_data) - 1);
    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
  }

  {
    static const lit_utf8_byte_t string_data[] = "A simple string";
    ecma_string_t *str_p = ecma_new_ecma_string_from_ascii (string_data, sizeof (string_data) - 1);

    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    ecma_stringbuilder_append (&builder, str_p);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
  }

  {
    ecma_string_t *str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
  }

  {
    static const lit_utf8_byte_t string_data[] = "abc";

    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    ecma_stringbuilder_append_char (&builder, LIT_CHAR_LOWERCASE_A);
    ecma_stringbuilder_append_char (&builder, LIT_CHAR_LOWERCASE_B);
    ecma_stringbuilder_append_char (&builder, LIT_CHAR_LOWERCASE_C);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    ecma_string_t *str_p = ecma_new_ecma_string_from_ascii (string_data, sizeof (string_data) - 1);
    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
  }

  {
    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    ecma_stringbuilder_append_char (&builder, LIT_CHAR_1);
    ecma_stringbuilder_append_char (&builder, LIT_CHAR_2);
    ecma_stringbuilder_append_char (&builder, LIT_CHAR_3);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    ecma_string_t *str_p = ecma_new_ecma_string_from_uint32 (123);
    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
  }

  {
    static const lit_utf8_byte_t string_data[] = "abc";
    ecma_string_t *uint_str_p = ecma_new_ecma_string_from_uint32 (234);

    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    ecma_stringbuilder_append_char (&builder, LIT_CHAR_1);
    ecma_stringbuilder_append_raw (&builder, string_data, sizeof (string_data) - 1);
    ecma_stringbuilder_append (&builder, uint_str_p);
    ecma_stringbuilder_append_magic (&builder, LIT_MAGIC_STRING_STRING);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    static const lit_utf8_byte_t expected_data[] = "1abc234string";
    ecma_string_t *str_p = ecma_new_ecma_string_from_ascii (expected_data, sizeof (expected_data) - 1);
    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
  }

  {
    static const lit_utf8_byte_t string_data[] = "abc";
    ecma_string_t *uint_str_p = ecma_new_ecma_string_from_uint32 (234);

    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    ecma_stringbuilder_append_char (&builder, LIT_CHAR_1);
    ecma_stringbuilder_append_raw (&builder, string_data, sizeof (string_data) - 1);
    ecma_stringbuilder_append (&builder, uint_str_p);
    ecma_stringbuilder_append_magic (&builder, LIT_MAGIC_STRING_STRING);
    /* Test that we do not leak. */
    ecma_stringbuilder_destroy (&builder);
  }

  {
    static const lit_utf8_byte_t string_data[] = "abcdefghijklmnop";
    const size_t count = UINT16_MAX / (sizeof (string_data) - 1) + 1;

    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    for (size_t i = 0; i < count; i++)
    {
      ecma_stringbuilder_append_raw (&builder, string_data, sizeof (string_data) - 1);
    }
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    ecma_string_t *expected_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
    for (size_t i = 0; i < count; i++)
    {
      expected_p =
        ecma_append_chars_to_string (expected_p, string_data, sizeof (string_data) - 1, sizeof (string_data) - 1);
    }

    TEST_ASSERT (ecma_compare_ecma_strings (result_p, expected_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (expected_p);
  }

  {
    static const lit_utf8_byte_t string_data[] = "abc";
    ecma_string_t *uint_str_p = ecma_new_ecma_string_from_uint32 (234);

    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    ecma_stringbuilder_append_char (&builder, LIT_CHAR_1);
    ecma_stringbuilder_append_raw (&builder, string_data, sizeof (string_data) - 1);

    ecma_string_t *another_string = ecma_new_ecma_string_from_ascii (string_data, sizeof (string_data) - 1);

    ecma_stringbuilder_append (&builder, uint_str_p);
    ecma_stringbuilder_append_magic (&builder, LIT_MAGIC_STRING_STRING);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    static const lit_utf8_byte_t expected_data[] = "1abc234string";
    ecma_string_t *str_p = ecma_new_ecma_string_from_ascii (expected_data, sizeof (expected_data) - 1);
    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
    ecma_deref_ecma_string (another_string);
  }

  {
    static const lit_utf8_byte_t string_data[] = "abc";
    ecma_string_t *uint_str_p = ecma_new_ecma_string_from_uint32 (234);

    ecma_stringbuilder_t builder = ecma_stringbuilder_create_from (uint_str_p);
    ecma_stringbuilder_append_raw (&builder, string_data, sizeof (string_data) - 1);
    ecma_stringbuilder_append_magic (&builder, LIT_MAGIC_STRING_STRING);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    static const lit_utf8_byte_t expected_data[] = "234abcstring";
    ecma_string_t *str_p = ecma_new_ecma_string_from_ascii (expected_data, sizeof (expected_data) - 1);
    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
  }

  {
    ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    ecma_string_t *str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
  }

  {
    ecma_string_t *str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
    ecma_stringbuilder_t builder = ecma_stringbuilder_create_from (str_p);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
  }

  {
    ecma_string_t *str_p = ecma_get_magic_string (LIT_MAGIC_STRING_STRING);
    ecma_stringbuilder_t builder = ecma_stringbuilder_create_from (str_p);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    TEST_ASSERT (ecma_compare_ecma_strings (result_p, str_p));
    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
  }

  {
    // should build a string from a single item array
    ecma_string_t *str_p = ecma_get_magic_string (LIT_MAGIC_STRING_STRING);
    lit_utf8_size_t str_size = ecma_string_get_size (str_p);
    ecma_stringbuilder_t builder = ecma_stringbuilder_create_from_array (&str_p, &str_size, 1);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    TEST_ASSERT (ecma_compare_ecma_strings (str_p, result_p));

    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (str_p);
  }

  {
    // should build a string from a multi-item array
    #define STRING_COUNT 3
    static const char* EXPECTED = "string,exports";

    ecma_string_t* strings_p [STRING_COUNT] = {
      ecma_get_magic_string(LIT_MAGIC_STRING_STRING),
      ecma_new_ecma_string_from_utf8((const lit_utf8_byte_t*)",", 1),
      ecma_get_magic_string(LIT_MAGIC_STRING_EXPORTS),
    };

    lit_utf8_size_t string_sizes_p [STRING_COUNT] = {
      ecma_string_get_size(strings_p[0]),
      ecma_string_get_size(strings_p[1]),
      ecma_string_get_size(strings_p[2]),
    };

    ecma_stringbuilder_t builder = ecma_stringbuilder_create_from_array (strings_p, string_sizes_p, STRING_COUNT);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);
    ecma_string_t *expected_p = ecma_new_ecma_string_from_utf8((const lit_utf8_byte_t*)EXPECTED, (lit_utf8_size_t)strlen(EXPECTED));

    TEST_ASSERT (ecma_compare_ecma_strings (result_p, expected_p));

    ecma_deref_ecma_string (result_p);
    ecma_deref_ecma_string (expected_p);

    for (lit_utf8_size_t i = 0; i < STRING_COUNT; i++)
    {
      ecma_deref_ecma_string(strings_p[i]);
    }

    #undef STRING_COUNT
  }

  {
    // should build an empty string from an empty array
    ecma_stringbuilder_t builder = ecma_stringbuilder_create_from_array (NULL, NULL, 0);
    ecma_string_t *result_p = ecma_stringbuilder_finalize (&builder);

    TEST_ASSERT (ecma_string_is_empty (result_p));

    ecma_deref_ecma_string (result_p);
  }

  ecma_finalize ();
  jmem_finalize ();
  TEST_CONTEXT_CLEANUP ();

  return 0;
} /* main */
