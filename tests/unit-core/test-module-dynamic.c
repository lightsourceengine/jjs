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

static int mode = 0;
static jjs_value_t global_user_value;

static jjs_value_t
global_assert (const jjs_call_info_t *call_info_p, /**< call information */
               const jjs_value_t args_p[], /**< arguments list */
               const jjs_length_t args_cnt) /**< arguments length */
{
  JJS_UNUSED (call_info_p);

  TEST_ASSERT (args_cnt == 1 && jjs_value_is_true (ctx (), args_p[0]));
  return jjs_boolean (ctx (), true);
} /* global_assert */

static void
register_assert (void)
{
  jjs_value_t global_object_value = jjs_current_realm (ctx ());

  jjs_value_t function_value = jjs_function_external (ctx (), global_assert);
  jjs_value_t function_name_value = jjs_string_sz (ctx (), "assert");
  jjs_value_t result_value = jjs_object_set (ctx (), global_object_value, function_name_value, function_value);

  jjs_value_free (ctx (), function_name_value);
  jjs_value_free (ctx (), function_value);
  jjs_value_free (ctx (), global_object_value);

  TEST_ASSERT (jjs_value_is_true (ctx (), result_value));
  jjs_value_free (ctx (), result_value);
} /* register_assert */

static void
compare_specifier (jjs_value_t specifier, /* string value */
                   int id) /* module id */
{
  jjs_char_t string[] = "XX_module.mjs";

  TEST_ASSERT (id >= 1 && id <= 99 && string[0] == 'X' && string[1] == 'X');

  string[0] = (jjs_char_t) ((id / 10) + '0');
  string[1] = (jjs_char_t) ((id % 10) + '0');

  jjs_size_t length = (jjs_size_t) (sizeof (string) - 1);
  jjs_char_t buffer[sizeof (string) - 1];

  TEST_ASSERT (jjs_value_is_string (ctx (), specifier));
  TEST_ASSERT (jjs_string_size (ctx (), specifier, JJS_ENCODING_CESU8) == length);

  TEST_ASSERT (jjs_string_to_buffer (ctx (), specifier, JJS_ENCODING_CESU8, buffer, length) == length);
  TEST_ASSERT (memcmp (buffer, string, length) == 0);
} /* compare_specifier */

static jjs_value_t
module_import_callback (jjs_context_t *context_p, /** JJS context */
                        const jjs_value_t specifier, /* string value */
                        const jjs_value_t user_value, /* user value assigned to the script */
                        void *user_p) /* user pointer */
{
  JJS_UNUSED (context_p);
  TEST_ASSERT (user_p == (void *) &mode);

  if (mode != 3)
  {
    jjs_value_t compare_value = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, user_value, global_user_value);

    TEST_ASSERT (jjs_value_is_true (ctx (), compare_value));
    jjs_value_free (ctx (), compare_value);
  }

  switch (mode)
  {
    case 0:
    {
      compare_specifier (specifier, 1);
      return jjs_throw_sz (ctx (), JJS_ERROR_RANGE, "Err01");
    }
    case 1:
    {
      compare_specifier (specifier, 2);
      return jjs_null (ctx ());
    }
    case 2:
    {
      compare_specifier (specifier, 3);

      jjs_value_t promise_value = jjs_promise (ctx ());
      /* Normally this should be a namespace object. */
      jjs_value_t object_value = jjs_object (ctx ());
      jjs_promise_resolve (ctx (), promise_value, object_value);
      jjs_value_free (ctx (), object_value);
      return promise_value;
    }
    case 3:
    {
      compare_specifier (specifier, 28);

      TEST_ASSERT (jjs_value_is_object (ctx (), user_value));
      jjs_value_t property_name = jjs_string_sz (ctx (), "MyProp1");
      jjs_value_t result = jjs_object_get (ctx (), user_value, property_name);
      TEST_ASSERT (jjs_value_is_number (ctx (), result) && jjs_value_as_number (ctx (), result) == 3.5);
      jjs_value_free (ctx (), result);
      jjs_value_free (ctx (), property_name);
      return jjs_undefined (ctx ());
    }
  }

  TEST_ASSERT (mode == 4 || mode == 5);

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_MODULE;

  jjs_value_t parse_result_value = jjs_parse (ctx (), (const jjs_char_t *) "", 0, &parse_options);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), parse_result_value));

  jjs_value_t result_value = jjs_module_link (ctx (), parse_result_value, NULL, NULL);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));
  jjs_value_free (ctx (), result_value);

  if (mode == 4)
  {
    result_value = jjs_module_evaluate (ctx (), parse_result_value);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));
    jjs_value_free (ctx (), result_value);
  }

  return parse_result_value;
} /* module_import_callback */

static void
run_script (const char *source_p, /* source code */
            jjs_parse_options_t *parse_options_p, /* parse options */
            bool release_user_value) /* release user value */
{
  jjs_value_t parse_result_value;

  parse_result_value = jjs_parse (ctx (), (const jjs_char_t *) source_p, strlen (source_p), parse_options_p);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), parse_result_value));

  if (release_user_value)
  {
    jjs_value_free (ctx (), parse_options_p->user_value);
    jjs_heap_gc (ctx (), JJS_GC_PRESSURE_HIGH);
  }

  jjs_value_t result_value;
  if (parse_options_p->options & JJS_PARSE_MODULE)
  {
    result_value = jjs_module_link (ctx (), parse_result_value, NULL, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));
    jjs_value_free (ctx (), result_value);

    result_value = jjs_module_evaluate (ctx (), parse_result_value);
  }
  else
  {
    result_value = jjs_run (ctx (), parse_result_value);
  }

  jjs_value_free (ctx (), parse_result_value);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));
  jjs_value_free (ctx (), result_value);

  result_value = jjs_run_jobs (ctx ());
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));
  jjs_value_free (ctx (), result_value);
} /* run_script */

int
main (void)
{
  if (!jjs_feature_enabled (JJS_FEATURE_MODULE))
  {
    jjs_log (ctx (), JJS_LOG_LEVEL_ERROR, "Module is disabled!\n");
    return 0;
  }

  ctx_open (NULL);

  register_assert ();
  jjs_module_on_import (ctx (), module_import_callback, (void *) &mode);

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_NO_OPTS;

  if (jjs_feature_enabled (JJS_FEATURE_ERROR_MESSAGES))
  {
    run_script ("var expected_message = 'Module cannot be instantiated'", &parse_options, false);
  }
  else
  {
    run_script ("var expected_message = ''", &parse_options, false);
  }

  global_user_value = jjs_object (ctx ());
  const char *source_p = TEST_STRING_LITERAL ("import('01_module.mjs').then(\n"
                                              "  function(resolve) { assert(false) },\n"
                                              "  function(reject) {\n"
                                              "    assert(reject instanceof RangeError\n"
                                              "           && reject.message === 'Err01')\n"
                                              "  }\n"
                                              ")");

  mode = 0;
  parse_options.options = JJS_PARSE_HAS_USER_VALUE;
  parse_options.user_value = global_user_value;
  run_script (source_p, &parse_options, false);
  jjs_value_free (ctx (), global_user_value);

  global_user_value = jjs_null (ctx ());
  source_p = TEST_STRING_LITERAL ("var src = \"import('02_module.mjs').then(\\\n"
                                  "  function(resolve) { assert(false) },\\\n"
                                  "  function(reject) {\\\n"
                                  "    assert(reject instanceof RangeError\\\n"
                                  "           && reject.message === expected_message)\\\n"
                                  "  }\\\n"
                                  ")\"\n"
                                  "eval('eval(src)')");

  mode = 1;
  parse_options.options = JJS_PARSE_HAS_USER_VALUE;
  parse_options.user_value = global_user_value;
  run_script (source_p, &parse_options, false);
  jjs_value_free (ctx (), global_user_value);

  global_user_value = jjs_number (ctx (), 5.6);
  source_p = TEST_STRING_LITERAL ("function f() {\n"
                                  "  return function () {\n"
                                  "    return import('03_module.mjs')\n"
                                  "  }\n"
                                  "}\n"
                                  "export var a = f()().then(\n"
                                  "  function(resolve) { assert(typeof resolve == 'object') },\n"
                                  "  function(reject) { assert(false) }\n"
                                  ")");

  mode = 2;
  parse_options.options = JJS_PARSE_HAS_USER_VALUE | JJS_PARSE_MODULE;
  parse_options.user_value = global_user_value;
  run_script (source_p, &parse_options, false);
  jjs_value_free (ctx (), global_user_value);

  global_user_value = jjs_string_sz (ctx (), "Any string...");
  source_p = TEST_STRING_LITERAL ("var src = \"import('02_module.mjs').then(\\\n"
                                  "  function(resolve) { assert(typeof resolve == 'object') },\\\n"
                                  "  function(reject) { assert(false) }\\\n"
                                  ")\"\n"
                                  "function f() {\n"
                                  "  eval('(function() { return eval(src) })()')\n"
                                  "}\n"
                                  "f()\n");

  for (int i = 0; i < 2; i++)
  {
    mode = 3;
    parse_options.options = JJS_PARSE_HAS_USER_VALUE | (i == 1 ? JJS_PARSE_MODULE : 0);
    parse_options.user_value = jjs_object (ctx ());
    jjs_value_t property_name = jjs_string_sz (ctx (), "MyProp1");
    jjs_value_t property_value = jjs_number (ctx (), 3.5);
    jjs_value_t result = jjs_object_set (ctx (), parse_options.user_value, property_name, property_value);
    TEST_ASSERT (jjs_value_is_true (ctx (), result));
    jjs_value_free (ctx (), result);
    jjs_value_free (ctx (), property_value);
    jjs_value_free (ctx (), property_name);

    source_p = TEST_STRING_LITERAL ("import('28_module.mjs')");
    run_script (source_p, &parse_options, true);
  }

  mode = 4;
  parse_options.options = JJS_PARSE_HAS_USER_VALUE;
  parse_options.user_value = global_user_value;
  run_script (source_p, &parse_options, false);
  jjs_value_free (ctx (), global_user_value);

  global_user_value = jjs_function_external (ctx (), global_assert);
  source_p = TEST_STRING_LITERAL ("var src = \"import('02_module.mjs').then(\\\n"
                                  "  function(resolve) { assert(false) },\\\n"
                                  "  function(reject) {\\\n"
                                  "    assert(reject instanceof RangeError\\\n"
                                  "           && reject.message === expected_message)\\\n"
                                  "  }\\\n"
                                  ")\"\n"
                                  "export function f() {\n"
                                  "  eval('(function() { return eval(src) })()')\n"
                                  "}\n"
                                  "f()\n");

  mode = 5;
  parse_options.options = JJS_PARSE_HAS_USER_VALUE | JJS_PARSE_MODULE;
  parse_options.user_value = global_user_value;
  run_script (source_p, &parse_options, false);
  jjs_value_free (ctx (), global_user_value);

  ctx_close ();
  return 0;
} /* main */
