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

static int global_counter;

static void
native_free_callback (jjs_context_t *context_p, /** JJS context */
                      void *native_p, /**< native pointer */
                      const jjs_object_native_info_t *info_p) /**< native info */
{
  JJS_UNUSED (context_p);
  TEST_ASSERT (native_p == (void *) &global_counter);
  TEST_ASSERT (info_p->free_cb == native_free_callback);
  global_counter++;
} /* native_free_callback */

static const jjs_object_native_info_t native_info = {
  .free_cb = native_free_callback,
  .number_of_references = 0,
  .offset_of_references = 0,
};

static jjs_value_t
create_array_from_container_handler (const jjs_call_info_t *call_info_p,
                                     const jjs_value_t args_p[],
                                     const jjs_length_t args_count)
{
  JJS_UNUSED (call_info_p);

  if (args_count < 2)
  {
    return jjs_undefined (ctx ());
  }

  bool is_key_value_pairs = false;
  jjs_value_t result = jjs_container_to_array (ctx (), args_p[0], &is_key_value_pairs);

  TEST_ASSERT (is_key_value_pairs == jjs_value_is_true (ctx (), args_p[1]));
  return result;
} /* create_array_from_container_handler */

static void
run_eval (const char *source_p)
{
  jjs_value_t result = jjs_eval_sz (ctx (), source_p, 0);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);
} /* run_eval */

static void
run_eval_error (const char *source_p)
{
  jjs_value_t result = jjs_eval_sz (ctx (), source_p, 0);
  jjs_value_free (ctx (), result);
} /* run_eval_error */

int
main (void)
{
  if (!jjs_feature_enabled (JJS_FEATURE_MAP) || !jjs_feature_enabled (JJS_FEATURE_SET)
      || !jjs_feature_enabled (JJS_FEATURE_WEAKMAP) || !jjs_feature_enabled (JJS_FEATURE_WEAKSET))
  {
    jjs_log (ctx (), JJS_LOG_LEVEL_ERROR, "Containers are disabled!\n");
    return 0;
  }

  ctx_open (NULL);

  jjs_value_t instance_check;
  jjs_value_t global = jjs_current_realm (ctx ());
  jjs_value_t map_str = jjs_string_sz (ctx (), "Map");
  jjs_value_t set_str = jjs_string_sz (ctx (), "Set");
  jjs_value_t weakmap_str = jjs_string_sz (ctx (), "WeakMap");
  jjs_value_t weakset_str = jjs_string_sz (ctx (), "WeakSet");
  jjs_value_t global_map = jjs_object_get (ctx (), global, map_str);
  jjs_value_t global_set = jjs_object_get (ctx (), global, set_str);
  jjs_value_t global_weakmap = jjs_object_get (ctx (), global, weakmap_str);
  jjs_value_t global_weakset = jjs_object_get (ctx (), global, weakset_str);

  jjs_value_t function = jjs_function_external (ctx (), create_array_from_container_handler);
  jjs_value_t name = jjs_string_sz (ctx (), "create_array_from_container");
  jjs_value_t res = jjs_object_set (ctx (), global, name, function);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), res));

  jjs_value_free (ctx (), res);
  jjs_value_free (ctx (), name);
  jjs_value_free (ctx (), function);

  jjs_value_free (ctx (), global);

  jjs_value_free (ctx (), map_str);
  jjs_value_free (ctx (), set_str);
  jjs_value_free (ctx (), weakmap_str);
  jjs_value_free (ctx (), weakset_str);

  jjs_value_t empty_map = jjs_container (ctx (), JJS_CONTAINER_TYPE_MAP, NULL, 0);
  TEST_ASSERT (jjs_container_type (ctx (), empty_map) == JJS_CONTAINER_TYPE_MAP);
  instance_check = jjs_binary_op (ctx (), JJS_BIN_OP_INSTANCEOF, empty_map, JJS_MOVE, global_map, JJS_KEEP);
  TEST_ASSERT (jjs_value_is_true (ctx (), instance_check));
  jjs_value_free (ctx (), instance_check);

  jjs_value_t empty_set = jjs_container (ctx (), JJS_CONTAINER_TYPE_SET, NULL, 0);
  TEST_ASSERT (jjs_container_type (ctx (), empty_set) == JJS_CONTAINER_TYPE_SET);
  instance_check = jjs_binary_op (ctx (), JJS_BIN_OP_INSTANCEOF, empty_set, JJS_MOVE, global_set, JJS_KEEP);
  TEST_ASSERT (jjs_value_is_true (ctx (), instance_check));
  jjs_value_free (ctx (), instance_check);

  jjs_value_t empty_weakmap = jjs_container (ctx (), JJS_CONTAINER_TYPE_WEAKMAP, NULL, 0);
  TEST_ASSERT (jjs_container_type (ctx (), empty_weakmap) == JJS_CONTAINER_TYPE_WEAKMAP);
  instance_check = jjs_binary_op (ctx (), JJS_BIN_OP_INSTANCEOF, empty_weakmap, JJS_MOVE, global_weakmap, JJS_KEEP);
  TEST_ASSERT (jjs_value_is_true (ctx (), instance_check));
  jjs_value_free (ctx (), instance_check);

  jjs_value_t empty_weakset = jjs_container (ctx (), JJS_CONTAINER_TYPE_WEAKSET, NULL, 0);
  TEST_ASSERT (jjs_container_type (ctx (), empty_weakset) == JJS_CONTAINER_TYPE_WEAKSET);
  instance_check = jjs_binary_op (ctx (), JJS_BIN_OP_INSTANCEOF, empty_weakset, JJS_MOVE, global_weakset, JJS_KEEP);
  TEST_ASSERT (jjs_value_is_true (ctx (), instance_check));
  jjs_value_free (ctx (), instance_check);

  jjs_value_free (ctx (), global_map);
  jjs_value_free (ctx (), global_set);
  jjs_value_free (ctx (), global_weakmap);
  jjs_value_free (ctx (), global_weakset);

  const jjs_char_t source[] = TEST_STRING_LITERAL ("(function () {\n"
                                                     "  var o1 = {}\n"
                                                     "  var o2 = {}\n"
                                                     "  var o3 = {}\n"
                                                     "  var wm = new WeakMap()\n"
                                                     "  wm.set(o1, o2)\n"
                                                     "  wm.set(o2, o3)\n"
                                                     "  return o3\n"
                                                     "})()\n");
  jjs_value_t result = jjs_eval (ctx (), source, sizeof (source) - 1, JJS_PARSE_NO_OPTS);
  TEST_ASSERT (jjs_value_is_object (ctx (), result));

  jjs_object_set_native_ptr (ctx (), result, &native_info, (void *) &global_counter);
  jjs_value_free (ctx (), result);

  global_counter = 0;
  jjs_heap_gc (ctx (), JJS_GC_PRESSURE_LOW);
  TEST_ASSERT (global_counter == 1);

  run_eval ("function assert(v) {\n"
            "  if(v !== true)\n"
            "    throw 'Assertion failed!'\n"
            "}");

  run_eval ("function test_values(arr1, arr2) {\n"
            "  assert(Array.isArray(arr1));\n"
            "  assert(arr1.length == arr2.length);\n"
            "  for(let i = 0; i < arr1.length; i++) {\n"
            "    assert(arr1[i] === arr2[i]);\n"
            "  }\n"
            "}\n");

  run_eval ("var map = new Map();\n"
            "map.set(1, 3.14);\n"
            "map.set(2, true);\n"
            "map.set(3, 'foo');\n"
            "var set = new Set();\n"
            "set.add(3.14);\n"
            "set.add(true);\n"
            "set.add('foo');\n"
            "var obj = { x:3, y:'foo'};\n"
            "var b_int = 1n;\n"
            "var obj_bint_map = new Map();\n"
            "obj_bint_map.set(1, obj);\n"
            "obj_bint_map.set(2, b_int);\n");

  run_eval ("var result = create_array_from_container(map, true);\n"
            "test_values(result, [1, 3.14, 2, true, 3, 'foo']);");

  run_eval ("var result = create_array_from_container(set, false);\n"
            "test_values(result, [3.14, true, 'foo']);");

  run_eval ("var result = create_array_from_container(map.entries(), true);\n"
            "test_values(result, [1, 3.14, 2, true, 3, 'foo']);");

  run_eval ("var result = create_array_from_container(map.keys(), false);\n"
            "test_values(result, [1, 2, 3,]);");

  run_eval ("var result = create_array_from_container(map.values(), false);\n"
            "test_values(result, [3.14, true, 'foo']);");

  run_eval ("var result = create_array_from_container(obj_bint_map, true)\n"
            "test_values(result, [1, obj, 2, b_int]);");

  run_eval ("var map = new Map();\n"
            "map.set(1, 1);\n"
            "var iter = map.entries();\n"
            "iter.next();\n"
            "var result = create_array_from_container(iter, true);\n"
            "assert(Array.isArray(result));\n"
            "assert(result.length == 0);");

  run_eval ("var ws = new WeakSet();\n"
            "var foo = {};\n"
            "var bar = {};\n"
            "ws.add(foo);\n"
            "ws.add(bar);\n"
            "var result = create_array_from_container(ws, false);\n"
            "test_values(result, [foo, bar]);\n");

  run_eval ("var ws = new WeakMap();\n"
            "var foo = {};\n"
            "var bar = {};\n"
            "ws.set(foo, 37);\n"
            "ws.set(bar, 'asd');\n"
            "var result = create_array_from_container(ws, true);\n"
            "test_values(result, [foo, 37, bar, 'asd']);\n");

  run_eval_error ("var iter = null;\n"
                  "var result = create_array_from_container(iter, false);\n"
                  "assert(result instanceof Error);");

  run_eval_error ("var iter = 3;\n"
                  "var result = create_array_from_container(iter, false);\n"
                  "assert(result instanceof Error);");

  run_eval_error ("var iter = [3.14, true, 'foo'].entries();\n"
                  "var result = create_array_from_container(iter, false);\n"
                  "assert(result instanceof Error);");

  ctx_close ();
  return 0;
} /* main */
