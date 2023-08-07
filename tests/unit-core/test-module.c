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

#include <string.h>

#include "jjs.h"

#include "test-common.h"

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

  TEST_ASSERT (jjs_value_is_string (specifier));
  TEST_ASSERT (jjs_string_size (specifier, JJS_ENCODING_CESU8) == length);

  TEST_ASSERT (jjs_string_to_buffer (specifier, JJS_ENCODING_CESU8, buffer, length) == length);
  TEST_ASSERT (memcmp (buffer, string, length) == 0);
} /* compare_specifier */

static void
compare_property (jjs_value_t namespace_object, /**< namespace object */
                  const char *name_p, /**< property name */
                  double expected_value) /**< property value (number for simplicity) */
{
  jjs_value_t name = jjs_string_sz (name_p);
  jjs_value_t result = jjs_object_get (namespace_object, name);

  TEST_ASSERT (jjs_value_is_number (result));
  TEST_ASSERT (jjs_value_as_number (result) == expected_value);

  jjs_value_free (result);
  jjs_value_free (name);
} /* compare_property */

static jjs_value_t
create_module (int id) /**< module id */
{
  jjs_parse_options_t module_parse_options;
  module_parse_options.options = JJS_PARSE_MODULE;

  jjs_value_t result;

  if (id == 0)
  {
    jjs_char_t source[] = "export var a = 7";

    result = jjs_parse (source, sizeof (source) - 1, &module_parse_options);
  }
  else
  {
    jjs_char_t source[] = "export {a} from 'XX_module.mjs'";

    TEST_ASSERT (id >= 1 && id <= 99 && source[17] == 'X' && source[18] == 'X');

    source[17] = (jjs_char_t) ((id / 10) + '0');
    source[18] = (jjs_char_t) ((id % 10) + '0');

    result = jjs_parse (source, sizeof (source) - 1, &module_parse_options);
  }

  TEST_ASSERT (!jjs_value_is_exception (result));
  return result;
} /* create_module */

static int counter = 0;
static jjs_value_t module;

static jjs_value_t
resolve_callback1 (const jjs_value_t specifier, /**< module specifier */
                   const jjs_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  TEST_ASSERT (user_p == (void *) &module);
  TEST_ASSERT (referrer == module);
  compare_specifier (specifier, 1);

  counter++;
  return counter == 1 ? jjs_number (7) : jjs_object ();
} /* resolve_callback1 */

static jjs_value_t prev_module;
static bool terminate_with_error;

static jjs_value_t
resolve_callback2 (const jjs_value_t specifier, /**< module specifier */
                   const jjs_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  TEST_ASSERT (prev_module == referrer);
  TEST_ASSERT (user_p == NULL);

  compare_specifier (specifier, ++counter);

  if (counter >= 32)
  {
    if (terminate_with_error)
    {
      return jjs_throw_sz (JJS_ERROR_RANGE, "Module not found");
    }

    return create_module (0);
  }

  prev_module = create_module (counter + 1);
  return prev_module;
} /* resolve_callback2 */

static jjs_value_t
resolve_callback3 (const jjs_value_t specifier, /**< module specifier */
                   const jjs_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  (void) specifier;
  (void) referrer;
  (void) user_p;

  TEST_ASSERT (false);
} /* resolve_callback3 */

static jjs_value_t
native_module_evaluate (const jjs_value_t native_module) /**< native module */
{
  ++counter;

  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_EVALUATING);

  jjs_value_t exp_val = jjs_string_sz ("exp");
  jjs_value_t other_exp_val = jjs_string_sz ("other_exp");
  /* The native module has no such export. */
  jjs_value_t no_exp_val = jjs_string_sz ("no_exp");

  jjs_value_t result = jjs_native_module_get (native_module, exp_val);
  TEST_ASSERT (jjs_value_is_undefined (result));
  jjs_value_free (result);

  result = jjs_native_module_get (native_module, other_exp_val);
  TEST_ASSERT (jjs_value_is_undefined (result));
  jjs_value_free (result);

  result = jjs_native_module_get (native_module, no_exp_val);
  TEST_ASSERT (jjs_value_is_exception (result));
  TEST_ASSERT (jjs_error_type (result) == JJS_ERROR_REFERENCE);
  jjs_value_free (result);

  jjs_value_t export = jjs_number (3.5);
  result = jjs_native_module_set (native_module, exp_val, export);
  TEST_ASSERT (jjs_value_is_boolean (result) && jjs_value_is_true (result));
  jjs_value_free (result);
  jjs_value_free (export);

  export = jjs_string_sz ("str");
  result = jjs_native_module_set (native_module, other_exp_val, export);
  TEST_ASSERT (jjs_value_is_boolean (result) && jjs_value_is_true (result));
  jjs_value_free (result);
  jjs_value_free (export);

  result = jjs_native_module_set (native_module, no_exp_val, no_exp_val);
  TEST_ASSERT (jjs_value_is_exception (result));
  TEST_ASSERT (jjs_error_type (result) == JJS_ERROR_REFERENCE);
  jjs_value_free (result);

  result = jjs_native_module_get (native_module, exp_val);
  TEST_ASSERT (jjs_value_is_number (result) && jjs_value_as_number (result) == 3.5);
  jjs_value_free (result);

  result = jjs_native_module_get (native_module, other_exp_val);
  TEST_ASSERT (jjs_value_is_string (result));
  jjs_value_free (result);

  jjs_value_free (exp_val);
  jjs_value_free (other_exp_val);
  jjs_value_free (no_exp_val);

  if (counter == 4)
  {
    ++counter;
    return jjs_throw_sz (JJS_ERROR_COMMON, "Ooops!");
  }

  return jjs_undefined ();
} /* native_module_evaluate */

static jjs_value_t
resolve_callback4 (const jjs_value_t specifier, /**< module specifier */
                   const jjs_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  (void) specifier;
  (void) referrer;

  ++counter;

  jjs_value_t exports[2] = { jjs_string_sz ("exp"), jjs_string_sz ("other_exp") };

  jjs_value_t native_module = jjs_native_module (native_module_evaluate, exports, 2);
  TEST_ASSERT (!jjs_value_is_exception (native_module));

  jjs_value_free (exports[0]);
  jjs_value_free (exports[1]);

  *((jjs_value_t *) user_p) = jjs_value_copy (native_module);
  return native_module;
} /* resolve_callback4 */

static void
module_state_changed (jjs_module_state_t new_state, /**< new state of the module */
                      const jjs_value_t module_val, /**< a module whose state is changed */
                      const jjs_value_t value, /**< value argument */
                      void *user_p) /**< user pointer */
{
  TEST_ASSERT (jjs_module_state (module_val) == new_state);
  TEST_ASSERT (module_val == module);
  TEST_ASSERT (user_p == (void *) &counter);

  ++counter;

  switch (counter)
  {
    case 1:
    case 3:
    {
      TEST_ASSERT (new_state == JJS_MODULE_STATE_LINKED);
      TEST_ASSERT (jjs_value_is_undefined (value));
      break;
    }
    case 2:
    {
      TEST_ASSERT (new_state == JJS_MODULE_STATE_EVALUATED);
      TEST_ASSERT (jjs_value_is_number (value) && jjs_value_as_number (value) == 33.5);
      break;
    }
    default:
    {
      TEST_ASSERT (counter == 4);
      TEST_ASSERT (new_state == JJS_MODULE_STATE_ERROR);
      TEST_ASSERT (jjs_value_is_number (value) && jjs_value_as_number (value) == -5.5);
      break;
    }
  }
} /* module_state_changed */

static jjs_value_t
resolve_callback5 (const jjs_value_t specifier, /**< module specifier */
                   const jjs_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  (void) specifier;
  (void) user_p;

  /* This circular reference is valid. However, import resolving triggers
   * a SyntaxError, because the module does not export a default binding. */
  return referrer;
} /* resolve_callback5 */

int
main (void)
{
  jjs_init (JJS_INIT_EMPTY);

  if (!jjs_feature_enabled (JJS_FEATURE_MODULE))
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Module is disabled!\n");
    jjs_cleanup ();
    return 0;
  }

  jjs_value_t number = jjs_number (5);
  jjs_value_t object = jjs_object ();

  jjs_value_t result = jjs_module_link (number, resolve_callback1, NULL);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_module_link (object, resolve_callback1, NULL);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  module = create_module (1);

  /* After an error, module must remain in unlinked mode. */
  result = jjs_module_link (module, resolve_callback1, (void *) &module);
  TEST_ASSERT (jjs_value_is_exception (result));
  TEST_ASSERT (counter == 1);
  jjs_value_free (result);

  result = jjs_module_link (module, resolve_callback1, (void *) &module);
  TEST_ASSERT (jjs_value_is_exception (result));
  TEST_ASSERT (counter == 2);
  jjs_value_free (result);

  prev_module = module;
  counter = 0;
  terminate_with_error = true;
  result = jjs_module_link (module, resolve_callback2, NULL);
  TEST_ASSERT (jjs_value_is_exception (result));
  TEST_ASSERT (counter == 32);
  jjs_value_free (result);

  /* The successfully resolved modules is kept around in unlinked state. */
  jjs_heap_gc (JJS_GC_PRESSURE_HIGH);

  counter = 31;
  terminate_with_error = false;
  result = jjs_module_link (module, resolve_callback2, NULL);
  TEST_ASSERT (jjs_value_is_boolean (result) && jjs_value_is_true (result));
  TEST_ASSERT (counter == 32);
  jjs_value_free (result);

  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_LINKED);
  TEST_ASSERT (jjs_module_request_count (module) == 1);
  result = jjs_module_request (module, 0);
  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_LINKED);
  jjs_value_free (result);

  jjs_value_free (module);

  module = create_module (1);

  prev_module = module;
  counter = 0;
  terminate_with_error = false;
  result = jjs_module_link (module, resolve_callback2, NULL);
  TEST_ASSERT (jjs_value_is_boolean (result) && jjs_value_is_true (result));
  TEST_ASSERT (counter == 32);
  jjs_value_free (result);
  jjs_value_free (module);

  TEST_ASSERT (jjs_module_state (number) == JJS_MODULE_STATE_INVALID);

  jjs_parse_options_t module_parse_options;
  module_parse_options.options = JJS_PARSE_MODULE;

  jjs_char_t source1[] = TEST_STRING_LITERAL ("import a from '16_module.mjs'\n"
                                                "export * from '07_module.mjs'\n"
                                                "export * from '44_module.mjs'\n"
                                                "import * as b from '36_module.mjs'\n");
  module = jjs_parse (source1, sizeof (source1) - 1, &module_parse_options);
  TEST_ASSERT (!jjs_value_is_exception (module));
  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_UNLINKED);

  TEST_ASSERT (jjs_module_request_count (number) == 0);
  TEST_ASSERT (jjs_module_request_count (module) == 4);

  result = jjs_module_request (object, 0);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_module_request (module, 0);
  compare_specifier (result, 16);
  jjs_value_free (result);

  result = jjs_module_request (module, 1);
  compare_specifier (result, 7);
  jjs_value_free (result);

  result = jjs_module_request (module, 2);
  compare_specifier (result, 44);
  jjs_value_free (result);

  result = jjs_module_request (module, 3);
  compare_specifier (result, 36);
  jjs_value_free (result);

  result = jjs_module_request (module, 4);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  jjs_value_free (module);

  result = jjs_module_namespace (number);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  jjs_char_t source2[] = TEST_STRING_LITERAL ("export let a = 6\n"
                                                "export let b = 8.5\n");
  module = jjs_parse (source2, sizeof (source2) - 1, &module_parse_options);
  TEST_ASSERT (!jjs_value_is_exception (module));
  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_UNLINKED);

  result = jjs_module_link (module, resolve_callback3, NULL);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_LINKED);

  result = jjs_module_evaluate (module);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_EVALUATED);

  result = jjs_module_namespace (module);
  TEST_ASSERT (jjs_value_is_object (result));
  compare_property (result, "a", 6);
  compare_property (result, "b", 8.5);
  jjs_value_free (result);

  jjs_value_free (module);

  module = jjs_native_module (NULL, &object, 1);
  TEST_ASSERT (jjs_value_is_exception (module));
  jjs_value_free (module);

  module = jjs_native_module (NULL, NULL, 0);
  TEST_ASSERT (!jjs_value_is_exception (module));
  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_UNLINKED);

  result = jjs_native_module_get (object, number);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_native_module_set (module, number, number);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  jjs_value_free (module);

  /* Valid identifier. */
  jjs_value_t export = jjs_string_sz ("\xed\xa0\x83\xed\xb2\x80");

  module = jjs_native_module (NULL, &export, 1);
  TEST_ASSERT (!jjs_value_is_exception (module));
  TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_UNLINKED);

  result = jjs_module_link (module, NULL, NULL);
  TEST_ASSERT (jjs_value_is_boolean (result) && jjs_value_is_true (result));
  jjs_value_free (result);

  result = jjs_module_evaluate (module);
  TEST_ASSERT (jjs_value_is_undefined (result));
  jjs_value_free (result);

  jjs_value_free (module);
  jjs_value_free (export);

  /* Invalid identifiers. */
  export = jjs_string_sz ("a+");
  module = jjs_native_module (NULL, &export, 1);
  TEST_ASSERT (jjs_value_is_exception (module));
  jjs_value_free (module);
  jjs_value_free (export);

  export = jjs_string_sz ("\xed\xa0\x80");
  module = jjs_native_module (NULL, &export, 1);
  TEST_ASSERT (jjs_value_is_exception (module));
  jjs_value_free (module);
  jjs_value_free (export);

  counter = 0;

  for (int i = 0; i < 2; i++)
  {
    jjs_char_t source3[] = TEST_STRING_LITERAL (
      "import {exp, other_exp as other} from 'native.js'\n"
      "import * as namespace from 'native.js'\n"
      "if (exp !== 3.5 || other !== 'str') { throw 'Assertion failed!' }\n"
      "if (namespace.exp !== 3.5 || namespace.other_exp !== 'str') { throw 'Assertion failed!' }\n");
    module = jjs_parse (source3, sizeof (source3) - 1, &module_parse_options);
    TEST_ASSERT (!jjs_value_is_exception (module));
    TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_UNLINKED);

    jjs_value_t native_module;

    result = jjs_module_link (module, resolve_callback4, (void *) &native_module);
    TEST_ASSERT (!jjs_value_is_exception (result));
    jjs_value_free (result);

    TEST_ASSERT (counter == i * 2 + 1);
    TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_LINKED);
    TEST_ASSERT (jjs_module_state (native_module) == JJS_MODULE_STATE_LINKED);

    result = jjs_module_evaluate (module);

    if (i == 0)
    {
      TEST_ASSERT (!jjs_value_is_exception (result));
      TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_EVALUATED);
      TEST_ASSERT (jjs_module_state (native_module) == JJS_MODULE_STATE_EVALUATED);
      TEST_ASSERT (counter == 2);
    }
    else
    {
      TEST_ASSERT (jjs_value_is_exception (result));
      TEST_ASSERT (jjs_module_state (module) == JJS_MODULE_STATE_ERROR);
      TEST_ASSERT (jjs_module_state (native_module) == JJS_MODULE_STATE_ERROR);
      TEST_ASSERT (counter == 5);
    }

    jjs_value_free (result);
    jjs_value_free (module);
    jjs_value_free (native_module);
  }

  jjs_value_free (object);
  jjs_value_free (number);

  counter = 0;
  jjs_module_on_state_changed (module_state_changed, (void *) &counter);

  jjs_char_t source4[] = TEST_STRING_LITERAL ("33.5\n");
  module = jjs_parse (source4, sizeof (source4) - 1, &module_parse_options);

  result = jjs_module_link (module, NULL, NULL);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_module_evaluate (module);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  jjs_value_free (module);

  jjs_char_t source5[] = TEST_STRING_LITERAL ("throw -5.5\n");
  module = jjs_parse (source5, sizeof (source5) - 1, &module_parse_options);

  result = jjs_module_link (module, NULL, NULL);
  TEST_ASSERT (!jjs_value_is_exception (result));
  jjs_value_free (result);

  result = jjs_module_evaluate (module);
  TEST_ASSERT (jjs_value_is_exception (result));
  jjs_value_free (result);

  jjs_value_free (module);

  jjs_module_on_state_changed (NULL, NULL);

  TEST_ASSERT (counter == 4);

  jjs_char_t source6[] = TEST_STRING_LITERAL ("import a from 'self'\n");
  module = jjs_parse (source6, sizeof (source6) - 1, &module_parse_options);

  result = jjs_module_link (module, resolve_callback5, NULL);
  TEST_ASSERT (jjs_value_is_exception (result) && jjs_error_type (result) == JJS_ERROR_SYNTAX);
  jjs_value_free (result);

  jjs_cleanup ();

  return 0;
} /* main */
