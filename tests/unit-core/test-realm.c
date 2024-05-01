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

static void
create_number_property (jjs_value_t object_value, /**< object value */
                        char *name_p, /**< name */
                        double number) /**< value */
{
  jjs_value_t name_value = jjs_string_sz (ctx (), name_p);
  jjs_value_t number_value = jjs_number (ctx (), number);
  jjs_value_t result_value = jjs_object_set (ctx (), object_value, name_value, number_value);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));

  jjs_value_free (ctx (), result_value);
  jjs_value_free (ctx (), number_value);
  jjs_value_free (ctx (), name_value);
} /* create_number_property */

static double
get_number_property (jjs_value_t object_value, /**< object value */
                     char *name_p) /**< name */
{
  jjs_value_t name_value = jjs_string_sz (ctx (), name_p);
  jjs_value_t result_value = jjs_object_get (ctx (), object_value, name_value);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));
  TEST_ASSERT (jjs_value_is_number (ctx (), result_value));

  double result = jjs_value_as_number (ctx (), result_value);

  jjs_value_free (ctx (), result_value);
  jjs_value_free (ctx (), name_value);
  return result;
} /* get_number_property */

static double
eval_and_get_number (char *script_p) /**< script source */
{
  jjs_value_t result_value;
  result_value = jjs_eval (ctx (), (const jjs_char_t *) script_p, strlen (script_p), JJS_PARSE_NO_OPTS);

  TEST_ASSERT (jjs_value_is_number (ctx (), result_value));
  double result = jjs_value_as_number (ctx (), result_value);
  jjs_value_free (ctx (), result_value);
  return result;
} /* eval_and_get_number */

static void
check_type_error (jjs_value_t result_value) /**< result value */
{
  TEST_ASSERT (jjs_value_is_exception (ctx (), result_value));
  result_value = jjs_exception_value (ctx (), result_value, true);
  TEST_ASSERT (jjs_error_type (ctx (), result_value) == JJS_ERROR_TYPE);
  jjs_value_free (ctx (), result_value);
} /* check_type_error */

static void
check_array_prototype (jjs_value_t realm_value, jjs_value_t result_value)
{
  jjs_value_t name_value = jjs_string_sz (ctx (), "Array");
  jjs_value_t array_value = jjs_object_get (ctx (), realm_value, name_value);
  TEST_ASSERT (jjs_value_is_object (ctx (), array_value));
  jjs_value_free (ctx (), name_value);

  name_value = jjs_string_sz (ctx (), "prototype");
  jjs_value_t prototype_value = jjs_object_get (ctx (), array_value, name_value);
  TEST_ASSERT (jjs_value_is_object (ctx (), prototype_value));
  jjs_value_free (ctx (), name_value);
  jjs_value_free (ctx (), array_value);

  jjs_value_t compare_value = jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, result_value, prototype_value);
  jjs_value_free (ctx (), prototype_value);

  TEST_ASSERT (jjs_value_is_boolean (ctx (), compare_value) && jjs_value_is_true (ctx (), compare_value));
  jjs_value_free (ctx (), compare_value);
} /* check_array_prototype */

/**
 * Unit test's main function.
 */
int
main (void)
{
  ctx_open (NULL);

  jjs_value_t global_value = jjs_current_realm (ctx ());
  jjs_value_t result_value = jjs_realm_this (ctx (), global_value);
  TEST_ASSERT (global_value == result_value);
  jjs_value_free (ctx (), global_value);

  jjs_value_t number_value = jjs_number (ctx (), 3);
  check_type_error (jjs_realm_this (ctx (), number_value));
  jjs_value_free (ctx (), number_value);

  if (!jjs_feature_enabled (JJS_FEATURE_REALM))
  {
    printf ("Skipping test, Realms not enabled\n");
    return 0;
  }

  jjs_value_t realm_value = jjs_realm (ctx ());

  create_number_property (global_value, "a", 3.5);
  create_number_property (global_value, "b", 7.25);
  create_number_property (realm_value, "a", -1.25);
  create_number_property (realm_value, "b", -6.75);

  TEST_ASSERT (eval_and_get_number ("a") == 3.5);

  result_value = jjs_set_realm (ctx (), realm_value);
  TEST_ASSERT (result_value == global_value);
  TEST_ASSERT (eval_and_get_number ("a") == -1.25);

  result_value = jjs_set_realm (ctx (), global_value);
  TEST_ASSERT (result_value == realm_value);
  TEST_ASSERT (eval_and_get_number ("b") == 7.25);

  result_value = jjs_set_realm (ctx (), realm_value);
  TEST_ASSERT (result_value == global_value);
  TEST_ASSERT (eval_and_get_number ("b") == -6.75);

  result_value = jjs_set_realm (ctx (), global_value);
  TEST_ASSERT (result_value == realm_value);

  jjs_value_t object_value = jjs_object (ctx ());
  check_type_error (jjs_set_realm (ctx (), object_value));
  jjs_value_free (ctx (), object_value);

  number_value = jjs_number (ctx (), 5);
  check_type_error (jjs_set_realm (ctx (), number_value));
  jjs_value_free (ctx (), number_value);

  jjs_value_free (ctx (), global_value);
  jjs_value_free (ctx (), realm_value);

  realm_value = jjs_realm (ctx ());

  result_value = jjs_realm_this (ctx (), realm_value);
  TEST_ASSERT (result_value == realm_value);
  jjs_value_free (ctx (), result_value);

  result_value = jjs_set_realm (ctx (), realm_value);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));
  object_value = jjs_object (ctx ());
  jjs_set_realm (ctx (), result_value);

  number_value = jjs_number (ctx (), 7);
  check_type_error (jjs_realm_set_this (ctx (), realm_value, number_value));
  check_type_error (jjs_realm_set_this (ctx (), number_value, object_value));
  jjs_value_free (ctx (), number_value);

  result_value = jjs_realm_set_this (ctx (), realm_value, object_value);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result_value) && jjs_value_is_true (ctx (), result_value));
  jjs_value_free (ctx (), result_value);

  create_number_property (object_value, "x", 7.25);
  create_number_property (object_value, "y", 1.25);

  result_value = jjs_set_realm (ctx (), realm_value);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));
  TEST_ASSERT (eval_and_get_number ("var z = -5.5; x + this.y") == 8.5);
  jjs_set_realm (ctx (), result_value);

  TEST_ASSERT (get_number_property (object_value, "z") == -5.5);

  result_value = jjs_realm_this (ctx (), realm_value);
  TEST_ASSERT (result_value == object_value);
  jjs_value_free (ctx (), result_value);

  jjs_value_free (ctx (), object_value);
  jjs_value_free (ctx (), realm_value);

  if (jjs_feature_enabled (JJS_FEATURE_PROXY))
  {
    /* Check property creation. */
    jjs_value_t handler_value = jjs_object (ctx ());
    jjs_value_t target_value = jjs_realm (ctx ());
    jjs_value_t proxy_value = jjs_proxy (ctx (), target_value, handler_value);

    jjs_realm_set_this (ctx (), target_value, proxy_value);
    jjs_value_free (ctx (), proxy_value);
    jjs_value_free (ctx (), handler_value);

    jjs_value_t old_realm_value = jjs_set_realm (ctx (), target_value);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), old_realm_value));
    TEST_ASSERT (eval_and_get_number ("var z = 1.5; z") == 1.5);
    jjs_set_realm (ctx (), old_realm_value);

    TEST_ASSERT (get_number_property (target_value, "z") == 1.5);
    jjs_value_free (ctx (), target_value);

    /* Check isExtensible error. */

    const char *script_p = "new Proxy({}, { isExtensible: function() { throw 42.5 } })";
    proxy_value = jjs_eval (ctx (), (const jjs_char_t *) script_p, strlen (script_p), JJS_PARSE_NO_OPTS);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), proxy_value) && jjs_value_is_object (ctx (), proxy_value));

    target_value = jjs_realm (ctx ());
    jjs_realm_set_this (ctx (), target_value, proxy_value);
    jjs_value_free (ctx (), proxy_value);

    old_realm_value = jjs_set_realm (ctx (), target_value);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), old_realm_value));
    script_p = "var z = 1.5";
    result_value = jjs_eval (ctx (), (const jjs_char_t *) script_p, strlen (script_p), JJS_PARSE_NO_OPTS);
    jjs_set_realm (ctx (), old_realm_value);
    jjs_value_free (ctx (), target_value);

    TEST_ASSERT (jjs_value_is_exception (ctx (), result_value));
    result_value = jjs_exception_value (ctx (), result_value, true);
    TEST_ASSERT (jjs_value_is_number (ctx (), result_value) && jjs_value_as_number (ctx (), result_value) == 42.5);
    jjs_value_free (ctx (), result_value);
  }

  realm_value = jjs_realm (ctx ());

  result_value = jjs_set_realm (ctx (), realm_value);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));

  const char *script_p = "global2 = global1 - 1; Object.getPrototypeOf([])";
  jjs_value_t script_value = jjs_parse (ctx (), (const jjs_char_t *) script_p, strlen (script_p), NULL);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), script_value));
  jjs_set_realm (ctx (), result_value);

  /* Script is compiled in another realm. */
  create_number_property (realm_value, "global1", 7.5);
  result_value = jjs_run (ctx (), script_value);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_value));

  check_array_prototype (realm_value, result_value);

  jjs_value_free (ctx (), result_value);
  jjs_value_free (ctx (), script_value);

  TEST_ASSERT (get_number_property (realm_value, "global2") == 6.5);

  jjs_value_free (ctx (), realm_value);

  ctx_close ();
  return 0;
} /* main */
