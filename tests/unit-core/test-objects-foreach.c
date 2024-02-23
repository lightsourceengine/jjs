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

static bool
count_objects (jjs_value_t object, void *user_arg)
{
  (void) object;
  TEST_ASSERT (user_arg != NULL);

  int *counter = (int *) user_arg;

  (*counter)++;
  return true;
} /* count_objects */

static void
test_container (void)
{
  jjs_value_t global = jjs_current_realm ();
  jjs_value_t map_str = jjs_string_sz ("Map");
  jjs_value_t map_result = jjs_object_get (global, map_str);
  jjs_type_t type = jjs_value_type (map_result);

  jjs_value_free (map_result);
  jjs_value_free (map_str);
  jjs_value_free (global);

  /* If there is no Map function this is not an es.next profile build, skip this test case. */
  if (type != JJS_TYPE_FUNCTION)
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Container based test is disabled!\n");
    return;
  }

  {
    /* Create a "DEMO" array which will be used for the Map below. */
    const char array_str[] = "var DEMO = [[1, 2], [3, 4]]; DEMO";
    jjs_value_t array = jjs_eval ((const jjs_char_t *) array_str, sizeof (array_str) - 1, 0);
    TEST_ASSERT (jjs_value_is_object (array));
    TEST_ASSERT (!jjs_value_is_exception (array));
    jjs_value_free (array);
  }

  const char eval_str[] = "new Map (DEMO)";
  {
    /* Make sure that the Map and it's prototype object/function is initialized. */
    jjs_value_t result = jjs_eval ((const jjs_char_t *) eval_str, sizeof (eval_str) - 1, 0);
    TEST_ASSERT (jjs_value_is_object (result));
    TEST_ASSERT (!jjs_value_is_exception (result));
    jjs_value_free (result);
  }

  /* Do a bit of cleaning to clear up old objects. */
  jjs_heap_gc (JJS_GC_PRESSURE_LOW);

  /* Get the number of iterable objects. */
  int start_count = 0;
  jjs_foreach_live_object (count_objects, &start_count);

  /* Create another map. */
  jjs_value_t result = jjs_eval ((const jjs_char_t *) eval_str, sizeof (eval_str) - 1, 0);

  /* Remove any old/unused objects. */
  jjs_heap_gc (JJS_GC_PRESSURE_LOW);

  /* Get the current number of objects. */
  int end_count = 0;
  jjs_foreach_live_object (count_objects, &end_count);

  /* As only one Map was created the number of available iterable objects should be incremented only by one. */
  TEST_ASSERT (end_count > start_count);
  TEST_ASSERT ((end_count - start_count) == 1);

  jjs_value_free (result);
} /* test_container */

static void
test_internal_prop (void)
{
  /* Make sure that the object is initialized in the engine. */
  jjs_value_t object_dummy = jjs_object ();

  /* Get the number of iterable objects. */
  int before_object_count = 0;
  jjs_foreach_live_object (count_objects, &before_object_count);

  jjs_value_t object = jjs_object ();

  /* After creating the object, the number of objects is incremented by one. */
  int after_object_count = 0;
  {
    jjs_foreach_live_object (count_objects, &after_object_count);

    TEST_ASSERT (after_object_count > before_object_count);
    TEST_ASSERT ((after_object_count - before_object_count) == 1);
  }

  jjs_value_t internal_prop_name = jjs_string_sz ("hidden_foo");
  jjs_value_t internal_prop_object = jjs_object ();
  bool internal_result = jjs_object_set_internal (object, internal_prop_name, internal_prop_object);
  TEST_ASSERT (internal_result == true);
  jjs_value_free (internal_prop_name);
  jjs_value_free (internal_prop_object);

  /* After adding an internal property object, the number of object is incremented by one. */
  {
    int after_internal_count = 0;
    jjs_foreach_live_object (count_objects, &after_internal_count);

    TEST_ASSERT (after_internal_count > after_object_count);
    TEST_ASSERT ((after_internal_count - after_object_count) == 1);
  }

  jjs_value_free (object);
  jjs_value_free (object_dummy);
} /* test_internal_prop */

static int test_data = 1;

static void
free_test_data (void *native_p, /**< native pointer */
                const jjs_object_native_info_t *info_p) /**< native info */
{
  TEST_ASSERT ((int *) native_p == &test_data);
  TEST_ASSERT (info_p->free_cb == free_test_data);
} /* free_test_data */

static const jjs_object_native_info_t test_info = {
  .free_cb = free_test_data,
  .number_of_references = 0,
  .offset_of_references = 0,
};

static const jjs_char_t strict_equal_source[] = "var x = function(a, b) {return a === b;}; x";

static bool
find_test_object_by_data (const jjs_value_t candidate, void *object_data_p, void *context_p)
{
  if (object_data_p == &test_data)
  {
    *((jjs_value_t *) context_p) = jjs_value_copy (candidate);
    return false;
  }
  return true;
} /* find_test_object_by_data */

static bool
find_test_object_by_property (const jjs_value_t candidate, void *context_p)
{
  jjs_value_t *args_p = (jjs_value_t *) context_p;
  jjs_value_t result = jjs_object_has (candidate, args_p[0]);

  bool has_property = (!jjs_value_is_exception (result) && jjs_value_is_true (result));

  /* If the object has the desired property, store a new reference to it in args_p[1]. */
  if (has_property)
  {
    args_p[1] = jjs_value_copy (candidate);
  }

  jjs_value_free (result);

  /* Stop iterating if we've found our object. */
  return !has_property;
} /* find_test_object_by_property */

int
main (void)
{
  jjs_init (JJS_INIT_EMPTY);

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_STRICT_MODE;

  /* Render strict-equal as a function. */
  jjs_value_t parse_result = jjs_parse (strict_equal_source, sizeof (strict_equal_source) - 1, &parse_options);
  TEST_ASSERT (!jjs_value_is_exception (parse_result));
  jjs_value_t strict_equal = jjs_run (parse_result);
  TEST_ASSERT (!jjs_value_is_exception (strict_equal));
  jjs_value_free (parse_result);

  /* Create an object and associate some native data with it. */
  jjs_value_t object = jjs_object ();
  jjs_object_set_native_ptr (object, &test_info, &test_data);

  /* Retrieve the object by its native pointer. */

  jjs_value_t found_object;
  TEST_ASSERT (jjs_foreach_live_object_with_info (&test_info, find_test_object_by_data, &found_object));
  jjs_value_t args[2] = { object, found_object };

  /* Assert that the correct object was retrieved. */
  jjs_value_t undefined = jjs_undefined ();
  jjs_value_t strict_equal_result = jjs_call (strict_equal, undefined, args, 2);
  TEST_ASSERT (jjs_value_is_boolean (strict_equal_result) && jjs_value_is_true (strict_equal_result));
  jjs_value_free (strict_equal_result);
  jjs_value_free (found_object);
  jjs_value_free (object);

  /* Collect garbage. */
  jjs_heap_gc (JJS_GC_PRESSURE_LOW);

  /* Attempt to retrieve the object by its native pointer again. */
  TEST_ASSERT (!jjs_foreach_live_object_with_info (&test_info, find_test_object_by_data, &found_object));

  /* Create an object and set a property on it. */
  object = jjs_object ();
  jjs_value_t property_name = jjs_string_sz ("xyzzy");
  jjs_value_t property_value = jjs_number (42);
  jjs_value_free (jjs_object_set (object, property_name, property_value));
  jjs_value_free (property_value);

  /* Retrieve the object by the presence of its property, placing it at args[1]. */
  args[0] = property_name;
  TEST_ASSERT (jjs_foreach_live_object (find_test_object_by_property, args));

  /* Assert that the right object was retrieved and release both the original reference to it and the retrieved one. */
  args[0] = object;
  strict_equal_result = jjs_call (strict_equal, undefined, args, 2);
  TEST_ASSERT (jjs_value_is_boolean (strict_equal_result) && jjs_value_is_true (strict_equal_result));
  jjs_value_free (strict_equal_result);
  jjs_value_free (args[0]);
  jjs_value_free (args[1]);

  /* Collect garbage. */
  jjs_heap_gc (JJS_GC_PRESSURE_LOW);

  /* Attempt to retrieve the object by the presence of its property again. */
  args[0] = property_name;
  TEST_ASSERT (!jjs_foreach_live_object (find_test_object_by_property, args));

  jjs_value_free (property_name);
  jjs_value_free (undefined);
  jjs_value_free (strict_equal);

  test_container ();
  test_internal_prop ();

  jjs_cleanup ();

  return 0;
} /* main */
