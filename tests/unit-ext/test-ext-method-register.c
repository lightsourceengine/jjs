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

/**
 * Unit test for jjs-ext/handler property registration
 */

#include <string.h>

#include "jjs.h"

#include "jjs-ext/properties.h"
#include "test-common.h"

static jjs_value_t
method_hello (const jjs_call_info_t *call_info_p, /**< call information */
              const jjs_value_t jargv[], /**< arguments */
              const jjs_length_t jargc) /**< number of arguments */
{
  (void) call_info_p;
  (void) jargv;
  return jjs_number (jargc);
} /* method_hello */

/**
 * Helper method to create a non-configurable property on an object
 */
static void
freeze_property (jjs_value_t target_obj, /**< target object */
                 const char *target_prop) /**< target property name */
{
  // "freeze" property
  jjs_property_descriptor_t prop_desc = jjs_property_descriptor ();
  prop_desc.flags |= JJS_PROP_IS_CONFIGURABLE_DEFINED;

  jjs_value_t prop_name = jjs_string_sz (target_prop);
  jjs_value_t return_value = jjs_object_define_own_prop (target_obj, prop_name, &prop_desc);
  TEST_ASSERT (jjs_value_is_boolean (return_value));
  jjs_value_free (return_value);
  jjs_value_free (prop_name);

  jjs_property_descriptor_free (&prop_desc);
} /* freeze_property */

/**
 * Test registration of various property values.
 */
static void
test_simple_registration (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  jjs_value_t target_object = jjs_object ();

  // Test simple registration
  jjsx_property_entry methods[] = {
    JJSX_PROPERTY_FUNCTION ("hello", method_hello),  JJSX_PROPERTY_NUMBER ("my_number", 42.5),
    JJSX_PROPERTY_STRING_SZ ("my_str", "super_str"), JJSX_PROPERTY_STRING ("my_str_sz", "super_str", 6),
    JJSX_PROPERTY_BOOLEAN ("my_bool", true),         JJSX_PROPERTY_BOOLEAN ("my_bool_false", false),
    JJSX_PROPERTY_UNDEFINED ("my_non_value"),        JJSX_PROPERTY_LIST_END (),
  };

  jjsx_register_result register_result = jjsx_set_properties (target_object, methods);

  TEST_ASSERT (register_result.registered == 7);
  TEST_ASSERT (jjs_value_is_undefined (register_result.result));

  jjsx_release_property_entry (methods, register_result);
  jjs_value_free (register_result.result);

  jjs_value_t global_obj = jjs_current_realm ();
  jjs_object_set_sz (global_obj, "test", target_object);
  jjs_value_free (target_object);
  jjs_value_free (global_obj);

  {
    const char *test_A = "test.my_number";
    jjs_value_t result = jjs_eval ((const jjs_char_t *) test_A, strlen (test_A), 0);
    TEST_ASSERT (jjs_value_is_number (result));
    TEST_ASSERT (jjs_value_as_number (result) == 42.5);
    jjs_value_free (result);
  }

  {
    const char *test_A = "test.my_str_sz === 'super_'";
    jjs_value_t result = jjs_eval ((const jjs_char_t *) test_A, strlen (test_A), 0);
    TEST_ASSERT (jjs_value_is_boolean (result));
    TEST_ASSERT (jjs_value_is_true (result));
    jjs_value_free (result);
  }

  {
    const char *test_A = "test.my_str === 'super_str'";
    jjs_value_t result = jjs_eval ((const jjs_char_t *) test_A, strlen (test_A), 0);
    TEST_ASSERT (jjs_value_is_boolean (result));
    TEST_ASSERT (jjs_value_is_true (result));
    jjs_value_free (result);
  }

  {
    const char *test_A = "test.my_bool";
    jjs_value_t result = jjs_eval ((const jjs_char_t *) test_A, strlen (test_A), 0);
    TEST_ASSERT (jjs_value_is_boolean (result));
    TEST_ASSERT (jjs_value_is_true (result));
    jjs_value_free (result);
  }

  {
    const char *test_A = "test.my_bool_false";
    jjs_value_t result = jjs_eval ((const jjs_char_t *) test_A, strlen (test_A), 0);
    TEST_ASSERT (jjs_value_is_boolean (result));
    TEST_ASSERT (jjs_value_is_true (result) == false);
    jjs_value_free (result);
  }

  {
    const char *test_A = "test.my_non_value";
    jjs_value_t result = jjs_eval ((const jjs_char_t *) test_A, strlen (test_A), 0);
    TEST_ASSERT (jjs_value_is_undefined (result));
    jjs_value_free (result);
  }

  {
    const char *test_A = "test.hello(33, 42, 2);";
    jjs_value_t result = jjs_eval ((const jjs_char_t *) test_A, strlen (test_A), 0);
    TEST_ASSERT (jjs_value_is_number (result));
    TEST_ASSERT ((uint32_t) jjs_value_as_number (result) == 3u);
    jjs_value_free (result);
  }

  {
    const char *test_A = "test.hello();";
    jjs_value_t result = jjs_eval ((const jjs_char_t *) test_A, strlen (test_A), 0);
    TEST_ASSERT (jjs_value_is_number (result));
    TEST_ASSERT ((uint32_t) jjs_value_as_number (result) == 0u);
    jjs_value_free (result);
  }

  jjs_cleanup ();
} /* test_simple_registration */

/**
 * Test registration error.
 *
 * Trying to register a property which is already a non-configurable property
 * should result in an error.
 */
static void
test_error_setvalue (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  const char *target_prop = "test_err";
  jjs_value_t global_obj = jjs_current_realm ();
  freeze_property (global_obj, target_prop);

  jjs_value_t new_object = jjs_object ();
  jjs_value_t set_result = jjs_object_set_sz (global_obj, target_prop, new_object);
  TEST_ASSERT (jjs_value_is_exception (set_result));

  jjs_value_free (set_result);
  jjs_value_free (new_object);
  jjs_value_free (global_obj);

  jjs_cleanup ();
} /* test_error_setvalue */

/**
 * Test registration error with jjsx_set_properties.
 *
 * Trying to register a property which is already a non-configurable property
 * should result in an error.
 */
static void
test_error_single_function (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  const char *target_prop = "test_err";
  jjs_value_t target_object = jjs_object ();
  freeze_property (target_object, target_prop);

  jjsx_property_entry methods[] = {
    JJSX_PROPERTY_FUNCTION (target_prop, method_hello), // This registration should fail
    JJSX_PROPERTY_LIST_END (),
  };

  jjsx_register_result register_result = jjsx_set_properties (target_object, methods);

  TEST_ASSERT (register_result.registered == 0);
  TEST_ASSERT (jjs_value_is_exception (register_result.result));
  jjsx_release_property_entry (methods, register_result);
  jjs_value_free (register_result.result);

  jjs_value_free (target_object);

  jjs_cleanup ();
} /* test_error_single_function */

/**
 * Test to see if jjsx_set_properties exits at the first error.
 */
static void
test_error_multiple_functions (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  const char *prop_ok = "prop_ok";
  const char *prop_err = "prop_err";
  const char *prop_not = "prop_not";
  jjs_value_t target_object = jjs_object ();
  freeze_property (target_object, prop_err);

  jjsx_property_entry methods[] = {
    JJSX_PROPERTY_FUNCTION (prop_ok, method_hello), // This registration is ok
    JJSX_PROPERTY_FUNCTION (prop_err, method_hello), // This registration should fail
    JJSX_PROPERTY_FUNCTION (prop_not, method_hello), // This registration is not done
    JJSX_PROPERTY_LIST_END (),
  };

  jjsx_register_result register_result = jjsx_set_properties (target_object, methods);

  TEST_ASSERT (register_result.registered == 1);
  TEST_ASSERT (jjs_value_is_exception (register_result.result));

  jjsx_release_property_entry (methods, register_result);
  jjs_value_free (register_result.result);

  {
    // Test if property "prop_ok" is correctly registered.
    jjs_value_t prop_ok_val = jjs_string_sz (prop_ok);
    jjs_value_t prop_ok_exists = jjs_object_has_own (target_object, prop_ok_val);
    TEST_ASSERT (jjs_value_is_true (prop_ok_exists));
    jjs_value_free (prop_ok_exists);

    // Try calling the method
    jjs_value_t prop_ok_func = jjs_object_get (target_object, prop_ok_val);
    TEST_ASSERT (jjs_value_is_function (prop_ok_func) == true);
    jjs_value_t args[2] = {
      jjs_number (22),
      jjs_number (-3),
    };
    jjs_size_t args_cnt = sizeof (args) / sizeof (jjs_value_t);
    jjs_value_t func_result = jjs_call (prop_ok_func, jjs_undefined (), args, args_cnt);
    TEST_ASSERT (jjs_value_is_number (func_result) == true);
    TEST_ASSERT ((uint32_t) jjs_value_as_number (func_result) == 2u);
    jjs_value_free (func_result);
    for (jjs_size_t idx = 0; idx < args_cnt; idx++)
    {
      jjs_value_free (args[idx]);
    }
    jjs_value_free (prop_ok_func);
    jjs_value_free (prop_ok_val);
  }

  {
    // The "prop_err" should exist - as it was "freezed" - but it should not be a function
    jjs_value_t prop_err_val = jjs_string_sz (prop_err);
    jjs_value_t prop_err_exists = jjs_object_has_own (target_object, prop_err_val);
    TEST_ASSERT (jjs_value_is_true (prop_err_exists));
    jjs_value_free (prop_err_exists);

    jjs_value_t prop_err_func = jjs_value_is_function (prop_err_val);
    TEST_ASSERT (jjs_value_is_function (prop_err_func) == false);
    jjs_value_free (prop_err_val);
  }

  { // The "prop_not" is not available on the target object
    jjs_value_t prop_not_val = jjs_string_sz (prop_not);
    jjs_value_t prop_not_exists = jjs_object_has_own (target_object, prop_not_val);
    TEST_ASSERT (jjs_value_is_true (prop_not_exists) == false);
    jjs_value_free (prop_not_exists);
    jjs_value_free (prop_not_val);
  }

  jjs_value_free (target_object);

  jjs_cleanup ();
} /* test_error_multiple_functions */

int
main (void)
{
  test_simple_registration ();
  test_error_setvalue ();
  test_error_single_function ();
  test_error_multiple_functions ();
  return 0;
} /* main */
