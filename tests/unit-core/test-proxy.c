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

/** Test in Proxy on C side. Equivalent test code in JS:

var demo = 0.0;

var target = {};
var handler = {
    get: function (target, name, recv) {
        assert (typeof (target) === 'object');
        assert (name === 'value');
        assert (typeof (recv) === 'object');
        return demo++;
    }

    set: function (target, name, value, recv) {
        assert (typeof (target) === 'object');
        assert (name === 'value');
        assert (typeof (recv) === 'object');
        demo = 55;
        return demo;
    }
};

var pdemo = new Proxy(target, handler);

assert (pdemo.value === 1.0);
assert (pdemo.value === 1.0);
assert (pdemo.value === 2.0);

pdemo.value = 55;

assert (pdemo.value === 56);

pdemo.value = 12;

assert (pdemo.value === 13);
 */

static int demo_value = 0;

static jjs_value_t
handler_get (const jjs_call_info_t *call_info_p, /**< call information */
             const jjs_value_t args_p[], /**< function arguments */
             const jjs_length_t args_count) /**< number of function arguments */
{
  JJS_UNUSED (call_info_p);

  TEST_ASSERT (args_count == 3);
  TEST_ASSERT (jjs_value_is_object (ctx (), args_p[0])); /* target */
  TEST_ASSERT (jjs_value_is_string (ctx (), args_p[1])); /* P */
  TEST_ASSERT (jjs_value_is_object (ctx (), args_p[2])); /* receiver */

  const char expected[] = "value";
  char buffer[10];
  jjs_size_t copied = jjs_string_to_buffer (ctx (), args_p[1], JJS_ENCODING_CESU8, (jjs_char_t *) buffer, 10);

  TEST_ASSERT (copied == 5);
  TEST_ASSERT (strncmp (expected, buffer, 5) == 0);

  demo_value++;

  return jjs_number (ctx (), demo_value);
} /* handler_get */

static jjs_value_t
handler_set (const jjs_call_info_t *call_info_p, /**< call information */
             const jjs_value_t args_p[], /**< function arguments */
             const jjs_length_t args_count) /**< number of function arguments */
{
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_p);
  JJS_UNUSED (args_count);

  TEST_ASSERT (args_count == 4);
  TEST_ASSERT (jjs_value_is_object (ctx (), args_p[0])); /* target */
  TEST_ASSERT (jjs_value_is_string (ctx (), args_p[1])); /* P */
  TEST_ASSERT (jjs_value_is_number (ctx (), args_p[2])); /* V */
  TEST_ASSERT (jjs_value_is_object (ctx (), args_p[3])); /* receiver */

  const char expected[] = "value";
  char buffer[10];
  jjs_size_t copied = jjs_string_to_buffer (ctx (), args_p[1], JJS_ENCODING_CESU8, (jjs_char_t *) buffer, 10);

  TEST_ASSERT (copied == 5);
  TEST_ASSERT (strncmp (expected, buffer, 5) == 0);

  TEST_ASSERT (jjs_value_is_number (ctx (), args_p[2]));
  demo_value = (int) jjs_value_as_number (ctx (), args_p[2]);

  return jjs_number (ctx (), demo_value);
} /* handler_set */

static void
set_property (jjs_value_t target, /**< target object */
              const char *name_p, /**< name of the property */
              jjs_value_t value) /**< value of the property */
{
  jjs_value_t name_val = jjs_string_sz (ctx (), name_p);
  jjs_value_t result_val = jjs_object_set (ctx (), target, name_val, value);

  TEST_ASSERT (jjs_value_is_boolean (ctx (), result_val));
  TEST_ASSERT (jjs_value_is_true (ctx (), result_val));
  jjs_value_free (ctx (), name_val);
} /* set_property */

static jjs_value_t
get_property (jjs_value_t target, /**< target object */
              const char *name_p) /**< name of the property */
{
  jjs_value_t name_val = jjs_string_sz (ctx (), name_p);
  jjs_value_t result_val = jjs_object_get (ctx (), target, name_val);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), result_val));
  jjs_value_free (ctx (), name_val);
  return result_val;
} /* get_property */

static void
set_function (jjs_value_t target, /**< target object */
              const char *name_p, /**< name of the function */
              jjs_external_handler_t handler_p) /**< function callback */
{
  jjs_value_t function_val = jjs_function_external (ctx (), handler_p);
  set_property (target, name_p, function_val);
  jjs_value_free (ctx (), function_val);
} /* set_function */

struct test_data
{
  int value;
};

static void
proxy_native_freecb (jjs_context_t *context_p, /** JJS context */
                     void *native_p, /**< native pointer */
                     const jjs_object_native_info_t *info_p) /**< native info */
{
  JJS_UNUSED (context_p);
  TEST_ASSERT (native_p != NULL);
  TEST_ASSERT (info_p->free_cb == proxy_native_freecb);
  free (native_p);
} /* proxy_native_freecb */

static const jjs_object_native_info_t proxy_native_info = {
  .free_cb = proxy_native_freecb,
  .number_of_references = 0,
  .offset_of_references = 0,
};

static jjs_value_t
proxy_native_handler_get (const jjs_call_info_t *call_info_p, /**< call information */
                          const jjs_value_t args_p[], /**< function arguments */
                          const jjs_length_t args_count) /**< number of function arguments */
{
  JJS_UNUSED (call_info_p);
  TEST_ASSERT (args_count == 3);

  /* 3rd argument (Receiver) should be the Proxy here. */
  jjs_value_t receiver = args_p[2];
  TEST_ASSERT (jjs_value_is_proxy (ctx (), receiver));

  /* Check if proxy has the native ptr. */
  TEST_ASSERT (jjs_object_has_native_ptr (ctx (), receiver, &proxy_native_info));
  struct test_data *native_p = jjs_object_get_native_ptr (ctx (), receiver, &proxy_native_info);
  TEST_ASSERT (native_p != NULL);

  native_p->value <<= 1;
  return jjs_number (ctx (), native_p->value);
} /* proxy_native_handler_get */

/**
 * Test Proxy with added native object.
 */
static void
test_proxy_native (void)
{
  jjs_value_t handler = jjs_object (ctx ());
  set_function (handler, "get", proxy_native_handler_get);

  jjs_value_t target = jjs_object (ctx ());
  jjs_value_t proxy = jjs_proxy (ctx (), target, handler);

  struct test_data *data = (struct test_data *) malloc (sizeof (struct test_data));
  data->value = 2;
  jjs_object_set_native_ptr (ctx (), proxy, &proxy_native_info, data);

  /* Call: proxy[10] */
  jjs_value_t result_for_10 = jjs_object_get_index (ctx (), proxy, 10);
  TEST_ASSERT (jjs_value_is_number (ctx (), result_for_10));
  TEST_ASSERT (jjs_value_as_number (ctx (), result_for_10) == 4.0);

  /* Call: proxy[5] */
  data->value = 8;
  jjs_value_t result_for_5 = jjs_object_get_index (ctx (), proxy, 5);
  TEST_ASSERT (jjs_value_is_number (ctx (), result_for_5));
  TEST_ASSERT (jjs_value_as_number (ctx (), result_for_5) == 16.0);

  jjs_value_free (ctx (), handler);
  jjs_value_free (ctx (), target);
  jjs_value_free (ctx (), proxy);
} /* test_proxy_native */

int
main (void)
{
  if (!jjs_feature_enabled (JJS_FEATURE_PROXY))
  {
    printf ("Skipping test, Proxy not enabled\n");
    return 0;
  }

  ctx_open (NULL);

  jjs_value_t handler = jjs_object (ctx ());
  {
    set_function (handler, "get", handler_get);
    set_function (handler, "set", handler_set);
  }

  jjs_value_t target = jjs_object (ctx ());
  jjs_value_t proxy = jjs_proxy (ctx (), target, handler);
  {
    jjs_value_t global = jjs_current_realm (ctx ());
    set_property (global, "pdemo", proxy);
    jjs_value_free (ctx (), global);
  }

  const char get_value_src[] = TEST_STRING_LITERAL ("pdemo.value");
  jjs_value_t parsed_get_code_val = jjs_parse_sz (ctx (), get_value_src, NULL);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), parsed_get_code_val));

  {
    jjs_value_t res = jjs_run (ctx (), parsed_get_code_val, JJS_KEEP);
    TEST_ASSERT (jjs_value_is_number (ctx (), res));
    TEST_ASSERT (jjs_value_as_number (ctx (), res) == 1.0);
    jjs_value_free (ctx (), res);
  }

  {
    jjs_value_t res = get_property (proxy, "value");
    TEST_ASSERT (jjs_value_is_number (ctx (), res));
    TEST_ASSERT (jjs_value_as_number (ctx (), res) == 2.0);
    jjs_value_free (ctx (), res);
  }

  {
    jjs_value_t res = jjs_run (ctx (), parsed_get_code_val, JJS_KEEP);
    TEST_ASSERT (jjs_value_is_number (ctx (), res));
    TEST_ASSERT (jjs_value_as_number (ctx (), res) == 3.0);
    jjs_value_free (ctx (), res);
  }

  const char set_value_src[] = TEST_STRING_LITERAL ("pdemo.value = 55");
  jjs_value_t parsed_set_code_val = jjs_parse_sz (ctx (), set_value_src, NULL);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), parsed_set_code_val));

  {
    jjs_value_t res = jjs_run (ctx (), parsed_set_code_val, JJS_KEEP);
    TEST_ASSERT (jjs_value_is_number (ctx (), res));
    TEST_ASSERT (jjs_value_as_number (ctx (), res) == 55);
    jjs_value_free (ctx (), res);
  }

  {
    jjs_value_t res = jjs_run (ctx (), parsed_get_code_val, JJS_KEEP);
    TEST_ASSERT (jjs_value_is_number (ctx (), res));
    TEST_ASSERT (jjs_value_as_number (ctx (), res) == 56);
    jjs_value_free (ctx (), res);
  }

  {
    jjs_value_t new_value = jjs_number (ctx (), 12);
    set_property (proxy, "value", new_value);
    jjs_value_free (ctx (), new_value);
  }

  {
    jjs_value_t res = get_property (proxy, "value");
    TEST_ASSERT (jjs_value_is_number (ctx (), res));
    TEST_ASSERT (jjs_value_as_number (ctx (), res) == 13.0);
    jjs_value_free (ctx (), res);
  }

  jjs_value_free (ctx (), parsed_set_code_val);
  jjs_value_free (ctx (), parsed_get_code_val);
  jjs_value_free (ctx (), proxy);
  jjs_value_free (ctx (), target);
  jjs_value_free (ctx (), handler);

  {
    const char has_value_src[] = TEST_STRING_LITERAL ("new Proxy({}, {\n"
                                                              "  has: function(target, key) { throw 33 }\n"
                                                              "})");
    jjs_value_t parsed_has_code_val = jjs_parse_sz (ctx (), has_value_src, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), parsed_has_code_val));

    jjs_value_t res = jjs_run (ctx (), parsed_has_code_val, JJS_MOVE);
    TEST_ASSERT (jjs_value_is_proxy (ctx (), res));

    jjs_value_t name = jjs_string_sz (ctx (), "key");
    TEST_ASSERT (jjs_value_is_string (ctx (), name));
    jjs_value_t property = jjs_object_has (ctx (), res, name);
    jjs_value_free (ctx (), name);
    jjs_value_free (ctx (), res);

    TEST_ASSERT (jjs_value_is_exception (ctx (), property));
    property = jjs_exception_value (ctx (), property, true);
    TEST_ASSERT (jjs_value_as_number (ctx (), property) == 33);
    jjs_value_free (ctx (), property);
  }

  target = jjs_object (ctx ());
  handler = jjs_object (ctx ());
  proxy = jjs_proxy (ctx (), target, handler);

  {
    jjs_value_t res = jjs_proxy_target (ctx (), proxy);
    TEST_ASSERT (res == target);
    jjs_value_free (ctx (), res);

    res = jjs_proxy_handler (ctx (), proxy);
    TEST_ASSERT (res == handler);
    jjs_value_free (ctx (), res);

    res = jjs_proxy_target (ctx (), target);
    TEST_ASSERT (jjs_value_is_exception (ctx (), res));
    res = jjs_exception_value (ctx (), res, true);
    TEST_ASSERT (jjs_error_type (ctx (), res) == JJS_ERROR_TYPE);
    jjs_value_free (ctx (), res);

    res = jjs_proxy_handler (ctx (), handler);
    TEST_ASSERT (jjs_value_is_exception (ctx (), res));
    res = jjs_exception_value (ctx (), res, true);
    TEST_ASSERT (jjs_error_type (ctx (), res) == JJS_ERROR_TYPE);
    jjs_value_free (ctx (), res);
  }

  jjs_value_free (ctx (), proxy);
  jjs_value_free (ctx (), handler);
  jjs_value_free (ctx (), target);

  test_proxy_native ();

  ctx_close ();
  return 0;
} /* main */
