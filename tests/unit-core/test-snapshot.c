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

#include "jjs.h"

#include "config.h"
#include "test-common.h"

/**
 * Maximum size of snapshots buffer
 */
#define SNAPSHOT_BUFFER_SIZE (256)

/**
 * Maximum size of literal buffer
 */
#define LITERAL_BUFFER_SIZE (256)

/**
 * Magic strings
 */
static const jjs_char_t *magic_strings[] = { (const jjs_char_t *) " ",      (const jjs_char_t *) "a",
                                               (const jjs_char_t *) "b",      (const jjs_char_t *) "c",
                                               (const jjs_char_t *) "from",   (const jjs_char_t *) "func",
                                               (const jjs_char_t *) "string", (const jjs_char_t *) "snapshot" };

/**
 * Magic string lengths
 */
static const jjs_length_t magic_string_lengths[] = { 1, 1, 1, 1, 4, 4, 6, 8 };

static void
test_function_snapshot (void)
{
  /* function to snapshot */
  if (!jjs_feature_enabled (JJS_FEATURE_SNAPSHOT_SAVE) || !jjs_feature_enabled (JJS_FEATURE_SNAPSHOT_EXEC))
  {
    return;
  }

  static uint32_t function_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];

  const jjs_char_t code_to_snapshot[] = "return a + b";

  TEST_ASSERT(jjs_init_default () == JJS_STATUS_OK);

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_HAS_ARGUMENT_LIST;
  parse_options.argument_list = jjs_string_sz ("a, b");

  jjs_value_t parse_result = jjs_parse (code_to_snapshot, sizeof (code_to_snapshot) - 1, &parse_options);
  TEST_ASSERT (!jjs_value_is_exception (parse_result));

  jjs_value_t generate_result =
    jjs_generate_snapshot (parse_result, 0, function_snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
  jjs_value_free (parse_result);
  jjs_value_free (parse_options.argument_list);

  TEST_ASSERT (!jjs_value_is_exception (generate_result) && jjs_value_is_number (generate_result));

  size_t function_snapshot_size = (size_t) jjs_value_as_number (generate_result);
  jjs_value_free (generate_result);

  jjs_cleanup ();

  TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

  jjs_value_t function_obj = jjs_exec_snapshot (function_snapshot_buffer,
                                                    function_snapshot_size,
                                                    0,
                                                    JJS_SNAPSHOT_EXEC_LOAD_AS_FUNCTION,
                                                    NULL);

  TEST_ASSERT (!jjs_value_is_exception (function_obj));
  TEST_ASSERT (jjs_value_is_function (function_obj));

  jjs_value_t this_val = jjs_undefined ();
  jjs_value_t args[2];
  args[0] = jjs_number (1.0);
  args[1] = jjs_number (2.0);

  jjs_value_t res = jjs_call (function_obj, this_val, args, 2);

  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_number (res));
  double num = jjs_value_as_number (res);
  TEST_ASSERT (num == 3);

  jjs_value_free (args[0]);
  jjs_value_free (args[1]);
  jjs_value_free (res);
  jjs_value_free (function_obj);

  jjs_cleanup ();
} /* test_function_snapshot */

static void
arguments_test_exec_snapshot (uint32_t *snapshot_p, size_t snapshot_size, uint32_t exec_snapshot_flags)
{
  TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);
  jjs_value_t res = jjs_exec_snapshot (snapshot_p, snapshot_size, 0, exec_snapshot_flags, NULL);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_number (res));
  double raw_value = jjs_value_as_number (res);
  TEST_ASSERT (raw_value == 15);
  jjs_value_free (res);

  jjs_cleanup ();
} /* arguments_test_exec_snapshot */

static void
test_function_arguments_snapshot (void)
{
  if (jjs_feature_enabled (JJS_FEATURE_SNAPSHOT_SAVE) && jjs_feature_enabled (JJS_FEATURE_SNAPSHOT_EXEC))
  {
    static uint32_t arguments_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];

    const jjs_char_t code_to_snapshot[] = TEST_STRING_LITERAL ("function f(a,b,c) {"
                                                                 "  arguments[0]++;"
                                                                 "  arguments[1]++;"
                                                                 "  arguments[2]++;"
                                                                 "  return a + b + c;"
                                                                 "}"
                                                                 "f(3,4,5);");
    TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

    jjs_value_t parse_result = jjs_parse (code_to_snapshot, sizeof (code_to_snapshot) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));

    jjs_value_t generate_result =
      jjs_generate_snapshot (parse_result, 0, arguments_snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
    jjs_value_free (parse_result);

    TEST_ASSERT (!jjs_value_is_exception (generate_result) && jjs_value_is_number (generate_result));

    size_t snapshot_size = (size_t) jjs_value_as_number (generate_result);
    jjs_value_free (generate_result);

    jjs_cleanup ();

    arguments_test_exec_snapshot (arguments_snapshot_buffer, snapshot_size, 0);
    arguments_test_exec_snapshot (arguments_snapshot_buffer, snapshot_size, JJS_SNAPSHOT_EXEC_COPY_DATA);
  }
} /* test_function_arguments_snapshot */

static void
test_exec_snapshot (uint32_t *snapshot_p, size_t snapshot_size, uint32_t exec_snapshot_flags)
{
  char string_data[32];

  TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

  jjs_register_magic_strings (magic_strings,
                                sizeof (magic_string_lengths) / sizeof (jjs_length_t),
                                magic_string_lengths);

  jjs_value_t res = jjs_exec_snapshot (snapshot_p, snapshot_size, 0, exec_snapshot_flags, NULL);

  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_string (res));
  jjs_size_t sz = jjs_string_size (res, JJS_ENCODING_CESU8);
  TEST_ASSERT (sz == 20);
  sz = jjs_string_to_buffer (res, JJS_ENCODING_CESU8, (jjs_char_t *) string_data, sz);
  TEST_ASSERT (sz == 20);
  jjs_value_free (res);
  TEST_ASSERT (!strncmp (string_data, "string from snapshot", (size_t) sz));

  jjs_cleanup ();
} /* test_exec_snapshot */

static void
test_snapshot_with_user (void)
{
  if (jjs_feature_enabled (JJS_FEATURE_SNAPSHOT_SAVE) && jjs_feature_enabled (JJS_FEATURE_SNAPSHOT_EXEC))
  {
    static uint32_t snapshot_buffer[SNAPSHOT_BUFFER_SIZE];

    const jjs_char_t code_to_snapshot[] = TEST_STRING_LITERAL ("function f() {}\n"
                                                                 "f");
    TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

    jjs_value_t parse_result = jjs_parse (code_to_snapshot, sizeof (code_to_snapshot) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));

    jjs_value_t generate_result = jjs_generate_snapshot (parse_result, 0, snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
    jjs_value_free (parse_result);

    TEST_ASSERT (!jjs_value_is_exception (generate_result) && jjs_value_is_number (generate_result));

    size_t snapshot_size = (size_t) jjs_value_as_number (generate_result);
    jjs_value_free (generate_result);

    for (int i = 0; i < 3; i++)
    {
      jjs_exec_snapshot_option_values_t snapshot_exec_options;

      if (i == 0)
      {
        snapshot_exec_options.user_value = jjs_object ();
      }
      else if (i == 1)
      {
        snapshot_exec_options.user_value = jjs_number (-3.5);
      }
      else
      {
        snapshot_exec_options.user_value = jjs_string_sz ("AnyString...");
      }

      jjs_value_t result = jjs_exec_snapshot (snapshot_buffer,
                                                  snapshot_size,
                                                  0,
                                                  JJS_SNAPSHOT_EXEC_HAS_USER_VALUE,
                                                  &snapshot_exec_options);

      TEST_ASSERT (!jjs_value_is_exception (result) && jjs_value_is_function (result));

      jjs_value_t user_value = jjs_source_user_value (result);
      jjs_value_free (result);

      result = jjs_binary_op (JJS_BIN_OP_STRICT_EQUAL, user_value, snapshot_exec_options.user_value);

      TEST_ASSERT (jjs_value_is_true (result));

      jjs_value_free (result);
      jjs_value_free (user_value);
      jjs_value_free (snapshot_exec_options.user_value);
    }

    jjs_cleanup ();
  }
} /* test_snapshot_with_user */

int
main (void)
{
  TEST_INIT ();

  /* Static snapshot */
  if (jjs_feature_enabled (JJS_FEATURE_SNAPSHOT_SAVE) && jjs_feature_enabled (JJS_FEATURE_SNAPSHOT_EXEC))
  {
    static uint32_t snapshot_buffer[SNAPSHOT_BUFFER_SIZE];
    const jjs_char_t code_to_snapshot[] = TEST_STRING_LITERAL ("function func(a, b, c) {"
                                                                 "  c = 'snapshot';"
                                                                 "  return arguments[0] + ' ' + b + ' ' + arguments[2];"
                                                                 "};"
                                                                 "func('string', 'from');");

    TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);
    jjs_register_magic_strings (magic_strings,
                                  sizeof (magic_string_lengths) / sizeof (jjs_length_t),
                                  magic_string_lengths);

    jjs_value_t parse_result = jjs_parse (code_to_snapshot, sizeof (code_to_snapshot) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));

    jjs_value_t generate_result =
      jjs_generate_snapshot (parse_result, JJS_SNAPSHOT_SAVE_STATIC, snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
    jjs_value_free (parse_result);

    TEST_ASSERT (!jjs_value_is_exception (generate_result) && jjs_value_is_number (generate_result));

    size_t snapshot_size = (size_t) jjs_value_as_number (generate_result);
    jjs_value_free (generate_result);

    /* Static snapshots are not supported by default. */
    jjs_value_t exec_result = jjs_exec_snapshot (snapshot_buffer, snapshot_size, 0, 0, NULL);
    TEST_ASSERT (jjs_value_is_exception (exec_result));
    jjs_value_free (exec_result);

    jjs_cleanup ();

    test_exec_snapshot (snapshot_buffer, snapshot_size, JJS_SNAPSHOT_EXEC_ALLOW_STATIC);
  }

  /* Merge snapshot */
  if (jjs_feature_enabled (JJS_FEATURE_SNAPSHOT_SAVE) && jjs_feature_enabled (JJS_FEATURE_SNAPSHOT_EXEC))
  {
    static uint32_t snapshot_buffer_0[SNAPSHOT_BUFFER_SIZE];
    static uint32_t snapshot_buffer_1[SNAPSHOT_BUFFER_SIZE];
    size_t snapshot_sizes[2];
    static uint32_t merged_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];

    const jjs_char_t code_to_snapshot1[] = "var a = 'hello'; 123";

    TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

    jjs_value_t parse_result = jjs_parse (code_to_snapshot1, sizeof (code_to_snapshot1) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));

    jjs_value_t generate_result = jjs_generate_snapshot (parse_result, 0, snapshot_buffer_0, SNAPSHOT_BUFFER_SIZE);
    jjs_value_free (parse_result);

    TEST_ASSERT (!jjs_value_is_exception (generate_result) && jjs_value_is_number (generate_result));

    snapshot_sizes[0] = (size_t) jjs_value_as_number (generate_result);
    jjs_value_free (generate_result);

    jjs_cleanup ();

    const jjs_char_t code_to_snapshot2[] = "var b = 'hello'; 456";

    TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

    parse_result = jjs_parse (code_to_snapshot2, sizeof (code_to_snapshot2) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));

    generate_result = jjs_generate_snapshot (parse_result, 0, snapshot_buffer_1, SNAPSHOT_BUFFER_SIZE);
    jjs_value_free (parse_result);

    TEST_ASSERT (!jjs_value_is_exception (generate_result) && jjs_value_is_number (generate_result));

    snapshot_sizes[1] = (size_t) jjs_value_as_number (generate_result);
    jjs_value_free (generate_result);

    jjs_cleanup ();

    TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

    const char *error_p;
    const uint32_t *snapshot_buffers[2];

    snapshot_buffers[0] = snapshot_buffer_0;
    snapshot_buffers[1] = snapshot_buffer_1;

    static uint32_t snapshot_buffer_0_bck[SNAPSHOT_BUFFER_SIZE];
    static uint32_t snapshot_buffer_1_bck[SNAPSHOT_BUFFER_SIZE];

    memcpy (snapshot_buffer_0_bck, snapshot_buffer_0, SNAPSHOT_BUFFER_SIZE);
    memcpy (snapshot_buffer_1_bck, snapshot_buffer_1, SNAPSHOT_BUFFER_SIZE);

    size_t merged_size = jjs_merge_snapshots (snapshot_buffers,
                                                snapshot_sizes,
                                                2,
                                                merged_snapshot_buffer,
                                                SNAPSHOT_BUFFER_SIZE,
                                                &error_p);

    jjs_cleanup ();

    TEST_ASSERT (0 == memcmp (snapshot_buffer_0_bck, snapshot_buffer_0, SNAPSHOT_BUFFER_SIZE));
    TEST_ASSERT (0 == memcmp (snapshot_buffer_1_bck, snapshot_buffer_1, SNAPSHOT_BUFFER_SIZE));

    TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

    jjs_value_t res = jjs_exec_snapshot (merged_snapshot_buffer, merged_size, 0, 0, NULL);
    TEST_ASSERT (!jjs_value_is_exception (res));
    TEST_ASSERT (jjs_value_as_number (res) == 123);
    jjs_value_free (res);

    res = jjs_exec_snapshot (merged_snapshot_buffer, merged_size, 1, 0, NULL);
    TEST_ASSERT (!jjs_value_is_exception (res));
    TEST_ASSERT (jjs_value_as_number (res) == 456);
    jjs_value_free (res);

    jjs_cleanup ();
  }

  /* Save literals */
  if (jjs_feature_enabled (JJS_FEATURE_SNAPSHOT_SAVE))
  {
    /* C format generation */
    TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

    static jjs_char_t literal_buffer_c[LITERAL_BUFFER_SIZE];
    static uint32_t literal_snapshot_buffer[SNAPSHOT_BUFFER_SIZE];
    static const jjs_char_t code_for_c_format[] = "var object = { aa:'fo\" o\\n \\\\', Bb:'max', aaa:'xzy0' };";

    jjs_value_t parse_result = jjs_parse (code_for_c_format, sizeof (code_for_c_format) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));

    jjs_value_t generate_result =
      jjs_generate_snapshot (parse_result, 0, literal_snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
    jjs_value_free (parse_result);

    TEST_ASSERT (!jjs_value_is_exception (generate_result));
    TEST_ASSERT (jjs_value_is_number (generate_result));

    size_t snapshot_size = (size_t) jjs_value_as_number (generate_result);
    jjs_value_free (generate_result);

    const size_t lit_c_buf_sz = jjs_get_literals_from_snapshot (literal_snapshot_buffer,
                                                                  snapshot_size,
                                                                  literal_buffer_c,
                                                                  LITERAL_BUFFER_SIZE,
                                                                  true);
    printf ("%i\n", (int32_t)lit_c_buf_sz);
    TEST_ASSERT (lit_c_buf_sz == 233);

    static const char *expected_c_format = ("jjs_length_t literal_count = 5;\n\n"
                                            "jjs_char_t *literals[5] =\n"
                                            "{\n"
                                            "  \"Bb\",\n"
                                            "  \"aa\",\n"
                                            "  \"aaa\",\n"
                                            "  \"xzy0\",\n"
                                            "  \"fo\\\" o\\x0A \\\\\"\n"
                                            "};\n\n"
                                            "jjs_length_t literal_sizes[5] =\n"
                                            "{\n"
                                            "  2 /* Bb */,\n"
                                            "  2 /* aa */,\n"
                                            "  3 /* aaa */,\n"
                                            "  4 /* xzy0 */,\n"
                                            "  8 /* fo\" o\n \\ */\n"
                                            "};\n");

    TEST_ASSERT (!strncmp ((char *) literal_buffer_c, expected_c_format, lit_c_buf_sz));

    /* List format generation */
    static jjs_char_t literal_buffer_list[LITERAL_BUFFER_SIZE];
    const size_t lit_list_buf_sz = jjs_get_literals_from_snapshot (literal_snapshot_buffer,
                                                                     snapshot_size,
                                                                     literal_buffer_list,
                                                                     LITERAL_BUFFER_SIZE,
                                                                     false);
    TEST_ASSERT (lit_list_buf_sz == 34);
    TEST_ASSERT (
      !strncmp ((char *) literal_buffer_list, "2 Bb\n2 aa\n3 aaa\n4 xzy0\n8 fo\" o\n \\\n", lit_list_buf_sz));

    jjs_cleanup ();
  }

  test_function_snapshot ();

  test_function_arguments_snapshot ();

  test_snapshot_with_user ();

  return 0;
} /* main */
