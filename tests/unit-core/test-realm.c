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

#include "test-common.h"

static void
create_number_property (jjs_value_t object_value, /**< object value */
                        char *name_p, /**< name */
                        double number) /**< value */
{
  jjs_value_t name_value = jjs_string_sz (name_p);
  jjs_value_t number_value = jjs_number (number);
  jjs_value_t result_value = jjs_object_set (object_value, name_value, number_value);
  TEST_ASSERT (!jjs_value_is_exception (result_value));

  jjs_value_free (result_value);
  jjs_value_free (number_value);
  jjs_value_free (name_value);
} /* create_number_property */

static double
get_number_property (jjs_value_t object_value, /**< object value */
                     char *name_p) /**< name */
{
  jjs_value_t name_value = jjs_string_sz (name_p);
  jjs_value_t result_value = jjs_object_get (object_value, name_value);
  TEST_ASSERT (!jjs_value_is_exception (result_value));
  TEST_ASSERT (jjs_value_is_number (result_value));

  double result = jjs_value_as_number (result_value);

  jjs_value_free (result_value);
  jjs_value_free (name_value);
  return result;
} /* get_number_property */

static double
eval_and_get_number (char *script_p) /**< script source */
{
  jjs_value_t result_value;
  result_value = jjs_eval ((const jjs_char_t *) script_p, strlen (script_p), JJS_PARSE_NO_OPTS);

  TEST_ASSERT (jjs_value_is_number (result_value));
  double result = jjs_value_as_number (result_value);
  jjs_value_free (result_value);
  return result;
} /* eval_and_get_number */

static void
check_type_error (jjs_value_t result_value) /**< result value */
{
  TEST_ASSERT (jjs_value_is_exception (result_value));
  result_value = jjs_exception_value (result_value, true);
  TEST_ASSERT (jjs_error_type (result_value) == JJS_ERROR_TYPE);
  jjs_value_free (result_value);
} /* check_type_error */

static void
check_array_prototype (jjs_value_t realm_value, jjs_value_t result_value)
{
  jjs_value_t name_value = jjs_string_sz ("Array");
  jjs_value_t array_value = jjs_object_get (realm_value, name_value);
  TEST_ASSERT (jjs_value_is_object (array_value));
  jjs_value_free (name_value);

  name_value = jjs_string_sz ("prototype");
  jjs_value_t prototype_value = jjs_object_get (array_value, name_value);
  TEST_ASSERT (jjs_value_is_object (prototype_value));
  jjs_value_free (name_value);
  jjs_value_free (array_value);

  jjs_value_t compare_value = jjs_binary_op (JJS_BIN_OP_STRICT_EQUAL, result_value, prototype_value);
  jjs_value_free (prototype_value);

  TEST_ASSERT (jjs_value_is_boolean (compare_value) && jjs_value_is_true (compare_value));
  jjs_value_free (compare_value);
} /* check_array_prototype */

/**
 * Unit test's main function.
 */
int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jjs_init_default () == JJS_STATUS_OK);

  jjs_value_t global_value = jjs_current_realm ();
  jjs_value_t result_value = jjs_realm_this (global_value);
  TEST_ASSERT (global_value == result_value);
  jjs_value_free (global_value);

  jjs_value_t number_value = jjs_number (3);
  check_type_error (jjs_realm_this (number_value));
  jjs_value_free (number_value);

  if (!jjs_feature_enabled (JJS_FEATURE_REALM))
  {
    printf ("Skipping test, Realms not enabled\n");
    return 0;
  }

  jjs_value_t realm_value = jjs_realm ();

  create_number_property (global_value, "a", 3.5);
  create_number_property (global_value, "b", 7.25);
  create_number_property (realm_value, "a", -1.25);
  create_number_property (realm_value, "b", -6.75);

  TEST_ASSERT (eval_and_get_number ("a") == 3.5);

  result_value = jjs_set_realm (realm_value);
  TEST_ASSERT (result_value == global_value);
  TEST_ASSERT (eval_and_get_number ("a") == -1.25);

  result_value = jjs_set_realm (global_value);
  TEST_ASSERT (result_value == realm_value);
  TEST_ASSERT (eval_and_get_number ("b") == 7.25);

  result_value = jjs_set_realm (realm_value);
  TEST_ASSERT (result_value == global_value);
  TEST_ASSERT (eval_and_get_number ("b") == -6.75);

  result_value = jjs_set_realm (global_value);
  TEST_ASSERT (result_value == realm_value);

  jjs_value_t object_value = jjs_object ();
  check_type_error (jjs_set_realm (object_value));
  jjs_value_free (object_value);

  number_value = jjs_number (5);
  check_type_error (jjs_set_realm (number_value));
  jjs_value_free (number_value);

  jjs_value_free (global_value);
  jjs_value_free (realm_value);

  realm_value = jjs_realm ();

  result_value = jjs_realm_this (realm_value);
  TEST_ASSERT (result_value == realm_value);
  jjs_value_free (result_value);

  result_value = jjs_set_realm (realm_value);
  TEST_ASSERT (!jjs_value_is_exception (result_value));
  object_value = jjs_object ();
  jjs_set_realm (result_value);

  number_value = jjs_number (7);
  check_type_error (jjs_realm_set_this (realm_value, number_value));
  check_type_error (jjs_realm_set_this (number_value, object_value));
  jjs_value_free (number_value);

  result_value = jjs_realm_set_this (realm_value, object_value);
  TEST_ASSERT (jjs_value_is_boolean (result_value) && jjs_value_is_true (result_value));
  jjs_value_free (result_value);

  create_number_property (object_value, "x", 7.25);
  create_number_property (object_value, "y", 1.25);

  result_value = jjs_set_realm (realm_value);
  TEST_ASSERT (!jjs_value_is_exception (result_value));
  TEST_ASSERT (eval_and_get_number ("var z = -5.5; x + this.y") == 8.5);
  jjs_set_realm (result_value);

  TEST_ASSERT (get_number_property (object_value, "z") == -5.5);

  result_value = jjs_realm_this (realm_value);
  TEST_ASSERT (result_value == object_value);
  jjs_value_free (result_value);

  jjs_value_free (object_value);
  jjs_value_free (realm_value);

  if (jjs_feature_enabled (JJS_FEATURE_PROXY))
  {
    /* Check property creation. */
    jjs_value_t handler_value = jjs_object ();
    jjs_value_t target_value = jjs_realm ();
    jjs_value_t proxy_value = jjs_proxy (target_value, handler_value);

    jjs_realm_set_this (target_value, proxy_value);
    jjs_value_free (proxy_value);
    jjs_value_free (handler_value);

    jjs_value_t old_realm_value = jjs_set_realm (target_value);
    TEST_ASSERT (!jjs_value_is_exception (old_realm_value));
    TEST_ASSERT (eval_and_get_number ("var z = 1.5; z") == 1.5);
    jjs_set_realm (old_realm_value);

    TEST_ASSERT (get_number_property (target_value, "z") == 1.5);
    jjs_value_free (target_value);

    /* Check isExtensible error. */

    const char *script_p = "new Proxy({}, { isExtensible: function() { throw 42.5 } })";
    proxy_value = jjs_eval ((const jjs_char_t *) script_p, strlen (script_p), JJS_PARSE_NO_OPTS);
    TEST_ASSERT (!jjs_value_is_exception (proxy_value) && jjs_value_is_object (proxy_value));

    target_value = jjs_realm ();
    jjs_realm_set_this (target_value, proxy_value);
    jjs_value_free (proxy_value);

    old_realm_value = jjs_set_realm (target_value);
    TEST_ASSERT (!jjs_value_is_exception (old_realm_value));
    script_p = "var z = 1.5";
    result_value = jjs_eval ((const jjs_char_t *) script_p, strlen (script_p), JJS_PARSE_NO_OPTS);
    jjs_set_realm (old_realm_value);
    jjs_value_free (target_value);

    TEST_ASSERT (jjs_value_is_exception (result_value));
    result_value = jjs_exception_value (result_value, true);
    TEST_ASSERT (jjs_value_is_number (result_value) && jjs_value_as_number (result_value) == 42.5);
    jjs_value_free (result_value);
  }

  realm_value = jjs_realm ();

  result_value = jjs_set_realm (realm_value);
  TEST_ASSERT (!jjs_value_is_exception (result_value));

  const char *script_p = "global2 = global1 - 1; Object.getPrototypeOf([])";
  jjs_value_t script_value = jjs_parse ((const jjs_char_t *) script_p, strlen (script_p), NULL);

  TEST_ASSERT (!jjs_value_is_exception (script_value));
  jjs_set_realm (result_value);

  /* Script is compiled in another realm. */
  create_number_property (realm_value, "global1", 7.5);
  result_value = jjs_run (script_value);
  TEST_ASSERT (!jjs_value_is_exception (result_value));

  check_array_prototype (realm_value, result_value);

  jjs_value_free (result_value);
  jjs_value_free (script_value);

  TEST_ASSERT (get_number_property (realm_value, "global2") == 6.5);

  jjs_value_free (realm_value);

  jjs_cleanup ();
  return 0;
} /* main */
