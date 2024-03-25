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

static jjs_value_t
backtrace_handler (const jjs_call_info_t *call_info_p, /**< call information */
                   const jjs_value_t args_p[], /**< argument list */
                   const jjs_length_t args_count) /**< argument count */
{
  JJS_UNUSED (call_info_p);

  uint32_t max_depth = 0;

  if (args_count >= 1 && jjs_value_is_number (args_p[0]))
  {
    max_depth = (uint32_t) jjs_value_as_number (args_p[0]);
  }

  return jjs_backtrace (max_depth);
} /* backtrace_handler */

static void
compare_string (jjs_value_t left_value, /* string value */
                const char *right_p) /* string to compare */
{
  jjs_char_t buffer[64];
  size_t length = strlen (right_p);

  TEST_ASSERT (length <= sizeof (buffer));
  TEST_ASSERT (jjs_value_is_string (left_value));
  TEST_ASSERT (jjs_string_size (left_value, JJS_ENCODING_CESU8) == length);

  TEST_ASSERT (jjs_string_to_buffer (left_value, JJS_ENCODING_CESU8, buffer, sizeof (buffer)) == length);
  TEST_ASSERT (memcmp (buffer, right_p, length) == 0);
} /* compare_string */

static const jjs_value_t *handler_args_p;
static int frame_index;

static bool
backtrace_callback (jjs_frame_t *frame_p, /* frame information */
                    void *user_p) /* user data */
{
  TEST_ASSERT ((void *) handler_args_p == user_p);
  TEST_ASSERT (jjs_frame_type (frame_p) == JJS_BACKTRACE_FRAME_JS);

  const jjs_frame_location_t *location_p = jjs_frame_location (frame_p);
  const jjs_value_t *function_p = jjs_frame_callee (frame_p);
  const jjs_value_t *this_p = jjs_frame_this (frame_p);

  TEST_ASSERT (location_p != NULL);
  TEST_ASSERT (function_p != NULL);
  TEST_ASSERT (this_p != NULL);

  compare_string (location_p->source_name, "capture_test.js");

  ++frame_index;

  if (frame_index == 1)
  {
    TEST_ASSERT (!jjs_frame_is_strict (frame_p));
    TEST_ASSERT (location_p->line == 2);
    TEST_ASSERT (location_p->column == 3);
    TEST_ASSERT (handler_args_p[0] == *function_p);
    TEST_ASSERT (handler_args_p[1] == *this_p);
    return true;
  }

  if (frame_index == 2)
  {
    TEST_ASSERT (jjs_frame_is_strict (frame_p));
    TEST_ASSERT (location_p->line == 7);
    TEST_ASSERT (location_p->column == 6);
    TEST_ASSERT (handler_args_p[2] == *function_p);
    TEST_ASSERT (jjs_value_is_undefined (*this_p));
    return true;
  }

  jjs_value_t global = jjs_current_realm ();

  TEST_ASSERT (frame_index == 3);
  TEST_ASSERT (!jjs_frame_is_strict (frame_p));
  TEST_ASSERT (location_p->line == 11);
  TEST_ASSERT (location_p->column == 3);
  TEST_ASSERT (handler_args_p[3] == *function_p);
  TEST_ASSERT (global == *this_p);

  jjs_value_free (global);
  return false;
} /* backtrace_callback */

static bool
async_backtrace_callback (jjs_frame_t *frame_p, /* frame information */
                          void *user_p) /* user data */
{
  TEST_ASSERT ((void *) handler_args_p == user_p);
  TEST_ASSERT (jjs_frame_type (frame_p) == JJS_BACKTRACE_FRAME_JS);

  const jjs_frame_location_t *location_p = jjs_frame_location (frame_p);
  const jjs_value_t *function_p = jjs_frame_callee (frame_p);

  TEST_ASSERT (location_p != NULL);
  TEST_ASSERT (function_p != NULL);

  compare_string (location_p->source_name, "async_capture_test.js");

  ++frame_index;

  if (frame_index == 1)
  {
    TEST_ASSERT (jjs_frame_is_strict (frame_p));
    TEST_ASSERT (location_p->line == 3);
    TEST_ASSERT (location_p->column == 3);
    TEST_ASSERT (handler_args_p[0] == *function_p);
    return true;
  }

  TEST_ASSERT (frame_index == 2);
  TEST_ASSERT (!jjs_frame_is_strict (frame_p));
  TEST_ASSERT (location_p->line == 8);
  TEST_ASSERT (location_p->column == 3);
  TEST_ASSERT (handler_args_p[1] == *function_p);
  return true;
} /* async_backtrace_callback */

static bool
class_backtrace_callback (jjs_frame_t *frame_p, /* frame information */
                          void *user_p) /* user data */
{
  TEST_ASSERT ((void *) handler_args_p == user_p);
  TEST_ASSERT (jjs_frame_type (frame_p) == JJS_BACKTRACE_FRAME_JS);

  const jjs_frame_location_t *location_p = jjs_frame_location (frame_p);
  const jjs_value_t *function_p = jjs_frame_callee (frame_p);

  TEST_ASSERT (location_p != NULL);
  TEST_ASSERT (function_p != NULL);

  compare_string (location_p->source_name, "class_capture_test.js");

  ++frame_index;

  if (frame_index == 1)
  {
    TEST_ASSERT (jjs_frame_is_strict (frame_p));
    TEST_ASSERT (location_p->line == 3);
    TEST_ASSERT (location_p->column == 14);
    return false;
  }

  TEST_ASSERT (frame_index == 2);
  TEST_ASSERT (jjs_frame_is_strict (frame_p));
  TEST_ASSERT (location_p->line == 2);
  TEST_ASSERT (location_p->column == 7);
  return false;
} /* class_backtrace_callback */

static jjs_value_t
capture_handler (const jjs_call_info_t *call_info_p, /**< call information */
                 const jjs_value_t args_p[], /**< argument list */
                 const jjs_length_t args_count) /**< argument count */
{
  JJS_UNUSED (call_info_p);

  TEST_ASSERT (args_count == 0 || args_count == 2 || args_count == 4);
  TEST_ASSERT (args_count == 0 || frame_index == 0);

  jjs_backtrace_cb_t callback = backtrace_callback;

  if (args_count == 0)
  {
    callback = class_backtrace_callback;
  }
  else if (args_count == 2)
  {
    callback = async_backtrace_callback;
  }

  handler_args_p = args_p;
  jjs_backtrace_capture (callback, (void *) args_p);

  TEST_ASSERT (args_count == 0 || frame_index == (args_count == 4 ? 3 : 2));

  return jjs_undefined ();
} /* capture_handler */

static bool
global_backtrace_callback (jjs_frame_t *frame_p, /* frame information */
                           void *user_p) /* user data */
{
  TEST_ASSERT (user_p != NULL && frame_index == 0);
  frame_index++;

  const jjs_value_t *function_p = jjs_frame_callee (frame_p);
  jjs_value_t *result_p = ((jjs_value_t *) user_p);

  TEST_ASSERT (function_p != NULL);
  jjs_value_free (*result_p);
  *result_p = jjs_value_copy (*function_p);
  return true;
} /* global_backtrace_callback */

static jjs_value_t
global_capture_handler (const jjs_call_info_t *call_info_p, /**< call information */
                        const jjs_value_t args_p[], /**< argument list */
                        const jjs_length_t args_count) /**< argument count */
{
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_p);
  JJS_UNUSED (args_count);

  jjs_value_t result = jjs_undefined ();
  jjs_backtrace_capture (global_backtrace_callback, &result);

  TEST_ASSERT (jjs_value_is_object (result));
  return result;
} /* global_capture_handler */

static void
register_callback (jjs_external_handler_t handler_p, /**< callback function */
                   char *name_p) /**< name of the function */
{
  jjs_value_t global = jjs_current_realm ();

  jjs_value_t func = jjs_function_external (handler_p);
  jjs_value_t name = jjs_string_sz (name_p);
  jjs_value_t result = jjs_object_set (global, name, func);
  TEST_ASSERT (!jjs_value_is_exception (result));

  jjs_value_free (result);
  jjs_value_free (name);
  jjs_value_free (func);

  jjs_value_free (global);
} /* register_callback */

static jjs_value_t
run (const char *source_name_p, /**< source name */
     const char *source_p) /**< source code */
{
  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jjs_string_sz (source_name_p);

  jjs_value_t code = jjs_parse ((const jjs_char_t *) source_p, strlen (source_p), &parse_options);
  jjs_value_free (parse_options.source_name);
  TEST_ASSERT (!jjs_value_is_exception (code));

  jjs_value_t result = jjs_run (code);
  jjs_value_free (code);

  return result;
} /* run */

static void
compare (jjs_value_t array, /**< array */
         uint32_t index, /**< item index */
         const char *str) /**< string to compare */
{
  jjs_char_t buf[64];

  size_t len = strlen (str);

  TEST_ASSERT (len < sizeof (buf));

  jjs_value_t value = jjs_object_get_index (array, index);

  TEST_ASSERT (!jjs_value_is_exception (value) && jjs_value_is_string (value));

  TEST_ASSERT (jjs_string_size (value, JJS_ENCODING_CESU8) == len);

  jjs_size_t str_len = jjs_string_to_buffer (value, JJS_ENCODING_CESU8, buf, (jjs_size_t) len);
  TEST_ASSERT (str_len == len);

  jjs_value_free (value);

  TEST_ASSERT (memcmp (buf, str, len) == 0);
} /* compare */

static void
test_get_backtrace_api_call (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  register_callback (backtrace_handler, "backtrace");
  register_callback (capture_handler, "capture");

  const char *source_p = ("function f() {\n"
                          "  return backtrace(0);\n"
                          "}\n"
                          "\n"
                          "function g() {\n"
                          "  return f();\n"
                          "}\n"
                          "\n"
                          "function h() {\n"
                          "  return g();\n"
                          "}\n"
                          "\n"
                          "h();\n");

  jjs_value_t backtrace = run ("something.js", source_p);

  TEST_ASSERT (!jjs_value_is_exception (backtrace) && jjs_value_is_array (backtrace));

  TEST_ASSERT (jjs_array_length (backtrace) == 4);

  compare (backtrace, 0, "something.js:2:3");
  compare (backtrace, 1, "something.js:6:3");
  compare (backtrace, 2, "something.js:10:3");
  compare (backtrace, 3, "something.js:13:1");

  jjs_value_free (backtrace);

  /* Depth set to 2 this time. */

  source_p = ("function f() {\n"
              "  1; return backtrace(2);\n"
              "}\n"
              "\n"
              "function g() {\n"
              "  return f();\n"
              "}\n"
              "\n"
              "function h() {\n"
              "  return g();\n"
              "}\n"
              "\n"
              "h();\n");

  backtrace = run ("something_else.js", source_p);

  TEST_ASSERT (!jjs_value_is_exception (backtrace) && jjs_value_is_array (backtrace));

  TEST_ASSERT (jjs_array_length (backtrace) == 2);

  compare (backtrace, 0, "something_else.js:2:6");
  compare (backtrace, 1, "something_else.js:6:3");

  jjs_value_free (backtrace);

  /* Test frame capturing. */

  frame_index = 0;
  source_p = ("var o = { f:function() {\n"
              "  return capture(o.f, o, g, h);\n"
              "} }\n"
              "\n"
              "function g() {\n"
              "  'use strict';\n"
              "  1; return o.f();\n"
              "}\n"
              "\n"
              "function h() {\n"
              "  return g();\n"
              "}\n"
              "\n"
              "h();\n");

  jjs_value_t result = run ("capture_test.js", source_p);

  TEST_ASSERT (jjs_value_is_undefined (result));
  jjs_value_free (result);

  TEST_ASSERT (frame_index == 3);

  /* Test async frame capturing. */
  source_p = "async function f() {}";
  result = jjs_eval ((const jjs_char_t *) source_p, strlen (source_p), JJS_PARSE_NO_OPTS);

  if (!jjs_value_is_exception (result))
  {
    jjs_value_free (result);

    frame_index = 0;
    source_p = ("function f() {\n"
                "  'use strict';\n"
                "  return capture(f, g);\n"
                "}\n"
                "\n"
                "async function g() {\n"
                "  await 0;\n"
                "  return f();\n"
                "}\n"
                "\n"
                "g();\n");

    result = run ("async_capture_test.js", source_p);

    TEST_ASSERT (jjs_value_is_promise (result));
    jjs_value_free (result);

    TEST_ASSERT (frame_index == 0);

    result = jjs_run_jobs ();
    TEST_ASSERT (!jjs_value_is_exception (result));

    TEST_ASSERT (frame_index == 2);
  }
  else
  {
    TEST_ASSERT (jjs_error_type (result) == JJS_ERROR_SYNTAX);
  }

  jjs_value_free (result);

  /* Test class initializer frame capturing. */
  source_p = "class C {}";
  result = jjs_eval ((const jjs_char_t *) source_p, strlen (source_p), JJS_PARSE_NO_OPTS);

  if (!jjs_value_is_exception (result))
  {
    jjs_value_free (result);

    frame_index = 0;
    source_p = ("class C {\n"
                "  a = capture();\n"
                "  static b = capture();\n"
                "}\n"
                "new C;\n");

    result = run ("class_capture_test.js", source_p);

    TEST_ASSERT (!jjs_value_is_exception (result));
    TEST_ASSERT (frame_index == 2);
  }
  else
  {
    TEST_ASSERT (jjs_error_type (result) == JJS_ERROR_SYNTAX);
  }

  jjs_value_free (result);

  register_callback (global_capture_handler, "global_capture");

  frame_index = 0;

  source_p = "global_capture()";

  jjs_value_t code = jjs_parse ((const jjs_char_t *) source_p, strlen (source_p), NULL);
  TEST_ASSERT (!jjs_value_is_exception (code));

  result = jjs_run (code);

  jjs_value_t compare_value = jjs_binary_op (JJS_BIN_OP_STRICT_EQUAL, result, code);
  TEST_ASSERT (jjs_value_is_true (compare_value));

  jjs_value_free (compare_value);
  jjs_value_free (result);
  jjs_value_free (code);

  jjs_cleanup ();
} /* test_get_backtrace_api_call */

static void
test_exception_backtrace (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  const char *source = ("function f() {\n"
                        "  undef_reference;\n"
                        "}\n"
                        "\n"
                        "function g() {\n"
                        "  return f();\n"
                        "}\n"
                        "\n"
                        "g();\n");

  jjs_value_t error = run ("bad.js", source);

  TEST_ASSERT (jjs_value_is_exception (error));

  error = jjs_exception_value (error, true);

  TEST_ASSERT (jjs_value_is_object (error));

  jjs_value_t name = jjs_string_sz ("stack");
  jjs_value_t backtrace = jjs_object_get (error, name);

  jjs_value_free (name);
  jjs_value_free (error);

  TEST_ASSERT (!jjs_value_is_exception (backtrace) && jjs_value_is_array (backtrace));

  TEST_ASSERT (jjs_array_length (backtrace) == 3);

  compare (backtrace, 0, "bad.js:2:3");
  compare (backtrace, 1, "bad.js:6:3");
  compare (backtrace, 2, "bad.js:9:1");

  jjs_value_free (backtrace);

  jjs_cleanup ();
} /* test_exception_backtrace */

static void
test_large_line_count (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  const char *source = ("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "g();\n");

  jjs_value_t error = run ("bad.js", source);

  TEST_ASSERT (jjs_value_is_exception (error));

  error = jjs_exception_value (error, true);

  TEST_ASSERT (jjs_value_is_object (error));

  jjs_value_t name = jjs_string_sz ("stack");
  jjs_value_t backtrace = jjs_object_get (error, name);

  jjs_value_free (name);
  jjs_value_free (error);

  TEST_ASSERT (!jjs_value_is_exception (backtrace) && jjs_value_is_array (backtrace));

  TEST_ASSERT (jjs_array_length (backtrace) == 1);

  compare (backtrace, 0, "bad.js:385:1");

  jjs_value_free (backtrace);

  jjs_cleanup ();
} /* test_large_line_count */

int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jjs_feature_enabled (JJS_FEATURE_LINE_INFO));

  test_get_backtrace_api_call ();
  test_exception_backtrace ();
  test_large_line_count ();

  return 0;
} /* main */
