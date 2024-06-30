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

#include <string.h>

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

static void
compare_property (jjs_value_t namespace_object, /**< namespace object */
                  const char *name_p, /**< property name */
                  double expected_value) /**< property value (number for simplicity) */
{
  jjs_value_t name = jjs_string_sz (ctx (), name_p);
  jjs_value_t result = jjs_object_get (ctx (), namespace_object, name);

  TEST_ASSERT (jjs_value_is_number (ctx (), result));
  TEST_ASSERT (jjs_value_as_number (ctx (), result) == expected_value);

  jjs_value_free (ctx (), result);
  jjs_value_free (ctx (), name);
} /* compare_property */

static jjs_value_t
create_module (jjs_context_t *context_p, /**< context */
               int id) /**< module id */
{
  jjs_parse_options_t module_parse_options = {
    .parse_module = true,
  };

  jjs_value_t result;

  if (id == 0)
  {
    const char source[] = "export var a = 7";

    result = jjs_parse_sz (context_p, source, &module_parse_options);
  }
  else
  {
    char source[] = "export {a} from 'XX_module.mjs'";

    TEST_ASSERT (id >= 1 && id <= 99 && source[17] == 'X' && source[18] == 'X');

    source[17] = (char) ((id / 10) + '0');
    source[18] = (char) ((id % 10) + '0');

    result = jjs_parse_sz (context_p, source, &module_parse_options);
  }

  TEST_ASSERT (!jjs_value_is_exception (context_p, result));
  return result;
} /* create_module */

static int counter = 0;
static jjs_value_t module;

static jjs_value_t
resolve_callback1 (jjs_context_t *context_p, /** JJS context */
                   const jjs_value_t specifier, /**< module specifier */
                   const jjs_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  JJS_UNUSED (context_p);
  TEST_ASSERT (user_p == (void *) &module);
  TEST_ASSERT (referrer == module);
  compare_specifier (specifier, 1);

  counter++;
  return counter == 1 ? jjs_number (ctx (), 7) : jjs_object (ctx ());
} /* resolve_callback1 */

static jjs_value_t prev_module;
static bool terminate_with_error;

static jjs_value_t
resolve_callback2 (jjs_context_t *context_p, /** JJS context */
                   const jjs_value_t specifier, /**< module specifier */
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
      return jjs_throw_sz (context_p, JJS_ERROR_RANGE, "Module not found");
    }

    return create_module (context_p, 0);
  }

  prev_module = create_module (context_p, counter + 1);
  return prev_module;
} /* resolve_callback2 */

static jjs_value_t
resolve_callback3 (jjs_context_t *context_p, /** JJS context */
                   const jjs_value_t specifier, /**< module specifier */
                   const jjs_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  JJS_UNUSED (specifier);
  JJS_UNUSED (referrer);
  JJS_UNUSED (user_p);

  TEST_ASSERT (false);

  return jjs_undefined (context_p);
} /* resolve_callback3 */

static jjs_value_t
synthetic_module_evaluate (jjs_context_t *context_p, /** JJS context */
                           const jjs_value_t synthetic_module) /**< synthetic module */
{
  ++counter;

  TEST_ASSERT (jjs_module_state (context_p, module) == JJS_MODULE_STATE_EVALUATING);

  jjs_value_t exp_val = jjs_string_sz (context_p, "exp");
  jjs_value_t other_exp_val = jjs_string_sz (context_p, "other_exp");
  /* The synthetic module has no such export. */
  jjs_value_t no_exp_val = jjs_string_sz (context_p, "no_exp");

  jjs_value_t result;
  jjs_value_t export = jjs_number (context_p, 3.5);
  result = jjs_synthetic_module_set_export (context_p, synthetic_module, exp_val, export, JJS_MOVE);
  TEST_ASSERT (jjs_value_is_boolean (context_p, result) && jjs_value_is_true (ctx (), result));
  jjs_value_free (context_p, result);

  export = jjs_string_sz (context_p, "str");
  result = jjs_synthetic_module_set_export (context_p, synthetic_module, other_exp_val, export, JJS_MOVE);
  TEST_ASSERT (jjs_value_is_boolean (context_p, result) && jjs_value_is_true (ctx (), result));
  jjs_value_free (context_p, result);

  result = jjs_synthetic_module_set_export (context_p, synthetic_module, no_exp_val, no_exp_val, JJS_KEEP);
  TEST_ASSERT (jjs_value_is_exception (context_p, result));
  TEST_ASSERT (jjs_error_type (context_p, result) == JJS_ERROR_REFERENCE);
  jjs_value_free (context_p, result);

  jjs_value_free (context_p, exp_val);
  jjs_value_free (context_p, other_exp_val);
  jjs_value_free (context_p, no_exp_val);

  if (counter == 4)
  {
    ++counter;
    return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "Ooops!");
  }

  return jjs_undefined (context_p);
} /* synthetic_module_evaluate */

static jjs_value_t
resolve_callback4 (jjs_context_t *context_p, /** JJS context */
                   const jjs_value_t specifier, /**< module specifier */
                   const jjs_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  JJS_UNUSED (specifier);
  JJS_UNUSED (referrer);

  ++counter;

  jjs_value_t exports[2] = { jjs_string_sz (context_p, "exp"), jjs_string_sz (context_p, "other_exp") };

  jjs_value_t synthetic_module = jjs_synthetic_module (context_p, synthetic_module_evaluate, exports, 2, JJS_MOVE);
  TEST_ASSERT (!jjs_value_is_exception (context_p, synthetic_module));

  *((jjs_value_t *) user_p) = jjs_value_copy (context_p, synthetic_module);
  return synthetic_module;
} /* resolve_callback4 */

static void
module_state_changed (jjs_context_t *context_p, /**< context */
                      jjs_module_state_t new_state, /**< new state of the module */
                      const jjs_value_t module_val, /**< a module whose state is changed */
                      const jjs_value_t value, /**< value argument */
                      void *user_p) /**< user pointer */
{
  TEST_ASSERT (jjs_module_state (context_p, module_val) == new_state);
  TEST_ASSERT (module_val == module);
  TEST_ASSERT (user_p == (void *) &counter);

  ++counter;

  switch (counter)
  {
    case 1:
    case 3:
    {
      TEST_ASSERT (new_state == JJS_MODULE_STATE_LINKED);
      TEST_ASSERT (jjs_value_is_undefined (context_p, value));
      break;
    }
    case 2:
    {
      TEST_ASSERT (new_state == JJS_MODULE_STATE_EVALUATED);
      TEST_ASSERT (jjs_value_is_number (context_p, value) && jjs_value_as_number (context_p, value) == 33.5);
      break;
    }
    default:
    {
      TEST_ASSERT (counter == 4);
      TEST_ASSERT (new_state == JJS_MODULE_STATE_ERROR);
      TEST_ASSERT (jjs_value_is_number (context_p, value) && jjs_value_as_number (context_p, value) == -5.5);
      break;
    }
  }
} /* module_state_changed */

static jjs_value_t
resolve_callback5 (jjs_context_t *context_p, /** JJS context */
                   const jjs_value_t specifier, /**< module specifier */
                   const jjs_value_t referrer, /**< parent module */
                   void *user_p) /**< user data */
{
  JJS_UNUSED (specifier);
  JJS_UNUSED (user_p);

  /* This circular reference is valid. However, import resolving triggers
   * a SyntaxError, because the module does not export a default binding. */
  return jjs_value_copy (context_p, referrer);
} /* resolve_callback5 */

int
main (void)
{
  if (!jjs_feature_enabled (JJS_FEATURE_MODULE))
  {
    jjs_log (ctx (), JJS_LOG_LEVEL_ERROR, "Module is disabled!\n");
    return 0;
  }

  ctx_open (NULL);

  jjs_value_t number = jjs_number (ctx (), 5);
  jjs_value_t object = jjs_object (ctx ());

  jjs_value_t result = jjs_module_link (ctx (), number, resolve_callback1, NULL);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_module_link (ctx (), object, resolve_callback1, NULL);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  module = create_module (ctx (), 1);

  /* After an error, module must remain in unlinked mode. */
  result = jjs_module_link (ctx (), module, resolve_callback1, (void *) &module);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  TEST_ASSERT (counter == 1);
  jjs_value_free (ctx (), result);

  result = jjs_module_link (ctx (), module, resolve_callback1, (void *) &module);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  TEST_ASSERT (counter == 2);
  jjs_value_free (ctx (), result);

  prev_module = module;
  counter = 0;
  terminate_with_error = true;
  result = jjs_module_link (ctx (), module, resolve_callback2, NULL);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  TEST_ASSERT (counter == 32);
  jjs_value_free (ctx (), result);

  /* The successfully resolved modules is kept around in unlinked state. */
  jjs_heap_gc (ctx (), JJS_GC_PRESSURE_HIGH);

  counter = 31;
  terminate_with_error = false;
  result = jjs_module_link (ctx (), module, resolve_callback2, NULL);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result) && jjs_value_is_true (ctx (), result));
  TEST_ASSERT (counter == 32);
  jjs_value_free (ctx (), result);

  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_LINKED);
  TEST_ASSERT (jjs_module_request_count (ctx (), module) == 1);
  result = jjs_module_request (ctx (), module, 0);
  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_LINKED);
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), module);

  module = create_module (ctx (), 1);

  prev_module = module;
  counter = 0;
  terminate_with_error = false;
  result = jjs_module_link (ctx (), module, resolve_callback2, NULL);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result) && jjs_value_is_true (ctx (), result));
  TEST_ASSERT (counter == 32);
  jjs_value_free (ctx (), result);
  jjs_value_free (ctx (), module);

  TEST_ASSERT (jjs_module_state (ctx (), number) == JJS_MODULE_STATE_INVALID);

  jjs_parse_options_t module_parse_options = {
    .parse_module = true,
  };

  const char source1[] = TEST_STRING_LITERAL ("import a from '16_module.mjs'\n"
                                                "export * from '07_module.mjs'\n"
                                                "export * from '44_module.mjs'\n"
                                                "import * as b from '36_module.mjs'\n");
  module = jjs_parse_sz (ctx (), source1, &module_parse_options);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), module));
  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_UNLINKED);

  TEST_ASSERT (jjs_module_request_count (ctx (), number) == 0);
  TEST_ASSERT (jjs_module_request_count (ctx (), module) == 4);

  result = jjs_module_request (ctx (), object, 0);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_module_request (ctx (), module, 0);
  compare_specifier (result, 16);
  jjs_value_free (ctx (), result);

  result = jjs_module_request (ctx (), module, 1);
  compare_specifier (result, 7);
  jjs_value_free (ctx (), result);

  result = jjs_module_request (ctx (), module, 2);
  compare_specifier (result, 44);
  jjs_value_free (ctx (), result);

  result = jjs_module_request (ctx (), module, 3);
  compare_specifier (result, 36);
  jjs_value_free (ctx (), result);

  result = jjs_module_request (ctx (), module, 4);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), module);

  result = jjs_module_namespace (ctx (), number);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  const char source2[] = TEST_STRING_LITERAL ("export let a = 6\n"
                                                "export let b = 8.5\n");
  module = jjs_parse_sz (ctx (), source2, &module_parse_options);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), module));
  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_UNLINKED);

  result = jjs_module_link (ctx (), module, resolve_callback3, NULL);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_LINKED);

  result = jjs_module_evaluate (ctx (), module);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_EVALUATED);

  result = jjs_module_namespace (ctx (), module);
  TEST_ASSERT (jjs_value_is_object (ctx (), result));
  compare_property (result, "a", 6);
  compare_property (result, "b", 8.5);
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), module);

  module = jjs_synthetic_module (ctx (), NULL, &object, 1, JJS_KEEP);
  TEST_ASSERT (jjs_value_is_exception (ctx (), module));
  jjs_value_free (ctx (), module);

  module = jjs_synthetic_module (ctx (), NULL, NULL, 0, JJS_KEEP);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), module));
  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_UNLINKED);

  result = jjs_synthetic_module_set_export (ctx (), module, number, number, JJS_KEEP);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), module);

  /* Valid identifier. */
  jjs_value_t export = jjs_string_sz (ctx (), "\xed\xa0\x83\xed\xb2\x80");

  module = jjs_synthetic_module (ctx (), NULL, &export, 1, JJS_KEEP);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), module));
  TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_UNLINKED);

  result = jjs_module_link (ctx (), module, NULL, NULL);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result) && jjs_value_is_true (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_module_evaluate (ctx (), module);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), module);
  jjs_value_free (ctx (), export);

  /* Invalid identifiers. */
  export = jjs_string_sz (ctx (), "a+");
  module = jjs_synthetic_module (ctx (), NULL, &export, 1, JJS_MOVE);
  TEST_ASSERT (jjs_value_is_exception (ctx (), module));
  jjs_value_free (ctx (), module);

  export = jjs_string_sz (ctx (), "\xed\xa0\x80");
  module = jjs_synthetic_module (ctx (), NULL, &export, 1, JJS_MOVE);
  TEST_ASSERT (jjs_value_is_exception (ctx (), module));
  jjs_value_free (ctx (), module);

  counter = 0;

  for (int i = 0; i < 2; i++)
  {
    const char source3[] = TEST_STRING_LITERAL (
      "import {exp, other_exp as other} from 'native.js'\n"
      "import * as namespace from 'native.js'\n"
      "if (exp !== 3.5 || other !== 'str') { throw `Assertion failed: exp = ${exp}, other = ${other}` }\n"
      "if (namespace.exp !== 3.5 || namespace.other_exp !== 'str') { throw `Assertion failed: namespace.exp = ${namespace.exp}, namespace.other_exp = ${namespace.other_exp}` }\n");
    module = jjs_parse_sz (ctx (), source3, &module_parse_options);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), module));
    TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_UNLINKED);

    jjs_value_t synthetic_module;

    result = jjs_module_link (ctx (), module, resolve_callback4, (void *) &synthetic_module);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
    jjs_value_free (ctx (), result);

    TEST_ASSERT (counter == i * 2 + 1);
    TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_LINKED);
    TEST_ASSERT (jjs_module_state (ctx (), synthetic_module) == JJS_MODULE_STATE_LINKED);

    result = jjs_module_evaluate (ctx (), module);

    if (i == 0)
    {
      TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
      TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_EVALUATED);
      TEST_ASSERT (jjs_module_state (ctx (), synthetic_module) == JJS_MODULE_STATE_EVALUATED);
      TEST_ASSERT (counter == 2);
    }
    else
    {
      TEST_ASSERT (jjs_value_is_exception (ctx (), result));
      TEST_ASSERT (jjs_module_state (ctx (), module) == JJS_MODULE_STATE_ERROR);
      TEST_ASSERT (jjs_module_state (ctx (), synthetic_module) == JJS_MODULE_STATE_ERROR);
      TEST_ASSERT (counter == 5);
    }

    jjs_value_free (ctx (), result);
    jjs_value_free (ctx (), module);
    jjs_value_free (ctx (), synthetic_module);
  }

  jjs_value_free (ctx (), object);
  jjs_value_free (ctx (), number);

  counter = 0;
  jjs_module_on_state_changed (ctx (), module_state_changed, (void *) &counter);

  const char source4[] = TEST_STRING_LITERAL ("33.5\n");
  module = jjs_parse_sz (ctx (), source4, &module_parse_options);

  result = jjs_module_link (ctx (), module, NULL, NULL);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_module_evaluate (ctx (), module);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), module);

  const char source5[] = TEST_STRING_LITERAL ("throw -5.5\n");
  module = jjs_parse_sz (ctx (), source5, &module_parse_options);

  result = jjs_module_link (ctx (), module, NULL, NULL);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  result = jjs_module_evaluate (ctx (), module);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), module);

  jjs_module_on_state_changed (ctx (), NULL, NULL);

  TEST_ASSERT (counter == 4);

  const char source6[] = TEST_STRING_LITERAL ("import a from 'self'\n");
  module = jjs_parse_sz (ctx (), source6, &module_parse_options);

  result = jjs_module_link (ctx (), module, resolve_callback5, NULL);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result) && jjs_error_type (ctx (), result) == JJS_ERROR_SYNTAX);
  jjs_value_free (ctx (), result);
  jjs_value_free (ctx (), module);

  ctx_close ();

  return 0;
} /* main */
