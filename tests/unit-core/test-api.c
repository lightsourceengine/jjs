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

const jjs_char_t test_source[] =
  TEST_STRING_LITERAL ("function assert (arg) { "
                       "  if (!arg) { "
                       "    throw Error('Assert failed');"
                       "  } "
                       "} "
                       "this.t = 1; "
                       "function f () { "
                       "return this.t; "
                       "} "
                       "this.foo = f; "
                       "this.bar = function (a) { "
                       "return a + t; "
                       "}; "
                       "function A () { "
                       "this.t = 12; "
                       "} "
                       "this.A = A; "
                       "this.a = new A (); "
                       "function call_external () { "
                       "  return this.external ('1', true); "
                       "} "
                       "function call_throw_test() { "
                       "  var catched = false; "
                       "  try { "
                       "    this.throw_test(); "
                       "  } catch (e) { "
                       "    catched = true; "
                       "    assert(e.name == 'TypeError'); "
                       "    assert(e.message == 'error'); "
                       "  } "
                       "  assert(catched); "
                       "} "
                       "function throw_reference_error() { "
                       " throw new ReferenceError ();"
                       "} "
                       "p = {'alpha':32, 'bravo':false, 'charlie':{}, 'delta':123.45, 'echo':'foobar'};"
                       "np = {}; Object.defineProperty (np, 'foxtrot', { "
                       "get: function() { throw 'error'; }, enumerable: true }) ");

bool test_api_is_free_callback_was_called = false;

static jjs_value_t
handler (const jjs_call_info_t *call_info_p, /**< call information */
         const jjs_value_t args_p[], /**< arguments list */
         const jjs_length_t args_cnt) /**< arguments length */
{
  JJS_UNUSED (call_info_p);
  char buffer[32];
  jjs_size_t sz;

  TEST_ASSERT (args_cnt == 2);

  TEST_ASSERT (jjs_value_is_string (args_p[0]));
  sz = jjs_string_size (args_p[0], JJS_ENCODING_CESU8);
  TEST_ASSERT (sz == 1);
  sz = jjs_string_to_buffer (args_p[0], JJS_ENCODING_CESU8, (jjs_char_t *) buffer, sz);
  TEST_ASSERT (sz == 1);
  TEST_ASSERT (!strncmp (buffer, "1", (size_t) sz));

  TEST_ASSERT (jjs_value_is_boolean (args_p[1]));

  return jjs_string_sz ("string from handler");
} /* handler */

static jjs_value_t
handler_throw_test (const jjs_call_info_t *call_info_p, /**< call information */
                    const jjs_value_t args_p[], /**< arguments list */
                    const jjs_length_t args_cnt) /**< arguments length */
{
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_p);
  JJS_UNUSED (args_cnt);
  return jjs_throw_sz (JJS_ERROR_TYPE, "error");
} /* handler_throw_test */

static void
handler_construct_1_freecb (void *native_p, /**< native pointer */
                            const jjs_object_native_info_t *info_p) /**< native info */
{
  TEST_ASSERT ((uintptr_t) native_p == (uintptr_t) 0x0000000000000000ull);
  TEST_ASSERT (info_p->free_cb == handler_construct_1_freecb);

  test_api_is_free_callback_was_called = true;
} /* handler_construct_1_freecb */

static void
handler_construct_2_freecb (void *native_p, /**< native pointer */
                            const jjs_object_native_info_t *info_p) /**< native info */
{
  TEST_ASSERT ((uintptr_t) native_p == (uintptr_t) 0x0012345678abcdefull);
  TEST_ASSERT (info_p->free_cb == handler_construct_2_freecb);

  test_api_is_free_callback_was_called = true;
} /* handler_construct_2_freecb */

/**
 * The name of the jjs_object_native_info_t struct.
 */
#define JJS_NATIVE_HANDLE_INFO_FOR_CTYPE(c_type) _jjs_object_native_info_##c_type

/**
 * Define a native pointer's type based on the C type and free callback.
 */
#define JJS_DEFINE_NATIVE_HANDLE_INFO(c_type, native_free_cb)                           \
  static const jjs_object_native_info_t JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (c_type) = { \
    .free_cb = (jjs_object_native_free_cb_t) native_free_cb,                            \
    .number_of_references = 0,                                                            \
    .offset_of_references = 0,                                                            \
  }

JJS_DEFINE_NATIVE_HANDLE_INFO (bind1, handler_construct_1_freecb);
JJS_DEFINE_NATIVE_HANDLE_INFO (bind2, handler_construct_2_freecb);
JJS_DEFINE_NATIVE_HANDLE_INFO (bind3, NULL);

static jjs_value_t
handler_construct (const jjs_call_info_t *call_info_p, /**< call information */
                   const jjs_value_t args_p[], /**< arguments list */
                   const jjs_length_t args_cnt) /**< arguments length */
{
  TEST_ASSERT (jjs_value_is_object (call_info_p->this_value));

  TEST_ASSERT (args_cnt == 1);
  TEST_ASSERT (jjs_value_is_boolean (args_p[0]));
  TEST_ASSERT (jjs_value_is_true (args_p[0]));

  jjs_value_t this_value = call_info_p->this_value;
  jjs_value_t field_name = jjs_string_sz ("value_field");
  jjs_value_t res = jjs_object_set (this_value, field_name, args_p[0]);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_true (res));
  jjs_value_free (res);
  jjs_value_free (field_name);

  /* Set a native pointer. */
  jjs_object_set_native_ptr (this_value, &JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (bind1), NULL);

  /* Check that the native pointer was set. */
  TEST_ASSERT (jjs_object_has_native_ptr (this_value, &JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (bind1)));
  void *ptr = jjs_object_get_native_ptr (this_value, &JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (bind1));
  TEST_ASSERT (ptr == NULL);

  /* Set a second native pointer. */
  jjs_object_set_native_ptr (this_value, &JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (bind2), (void *) 0x0012345678abcdefull);

  /* Check that a second native pointer was set. */
  TEST_ASSERT (jjs_object_has_native_ptr (this_value, &JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (bind2)));
  ptr = jjs_object_get_native_ptr (this_value, &JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (bind2));
  TEST_ASSERT ((uintptr_t) ptr == (uintptr_t) 0x0012345678abcdefull);

  /* Check that the first native pointer is still set. */
  TEST_ASSERT (jjs_object_has_native_ptr (this_value, &JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (bind1)));
  ptr = jjs_object_get_native_ptr (this_value, &JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (bind1));
  TEST_ASSERT (ptr == NULL);
  return jjs_boolean (true);
} /* handler_construct */

/**
 * Extended Magic Strings
 */
#define JJS_MAGIC_STRING_ITEMS                                     \
  JJS_MAGIC_STRING_DEF (GLOBAL, global)                            \
  JJS_MAGIC_STRING_DEF (GREEK_ZERO_SIGN, \xed\xa0\x80\xed\xb6\x8a) \
  JJS_MAGIC_STRING_DEF (CONSOLE, console)

#define JJS_MAGIC_STRING_DEF(NAME, STRING) static const char jjs_magic_string_ex_##NAME[] = #STRING;

JJS_MAGIC_STRING_ITEMS

#undef JJS_MAGIC_STRING_DEF

const jjs_length_t magic_string_lengths[] = {
#define JJS_MAGIC_STRING_DEF(NAME, STRING) (jjs_length_t) (sizeof (jjs_magic_string_ex_##NAME) - 1u),

  JJS_MAGIC_STRING_ITEMS

#undef JJS_MAGIC_STRING_DEF
};

const jjs_char_t *magic_string_items[] = {
#define JJS_MAGIC_STRING_DEF(NAME, STRING) (const jjs_char_t *) jjs_magic_string_ex_##NAME,

  JJS_MAGIC_STRING_ITEMS

#undef JJS_MAGIC_STRING_DEF
};

static bool foreach (const jjs_value_t name, /**< field name */
                     const jjs_value_t value, /**< field value */
                     void *user_data) /**< user data */
{
  char str_buf_p[128];
  jjs_size_t sz =
    jjs_string_to_buffer (name, JJS_ENCODING_CESU8, (jjs_char_t *) str_buf_p, sizeof (str_buf_p) - 1);
  str_buf_p[sz] = '\0';

  TEST_ASSERT (!strncmp ((const char *) user_data, "user_data", 9));
  TEST_ASSERT (sz > 0);

  if (!strncmp (str_buf_p, "alpha", (size_t) sz))
  {
    TEST_ASSERT (jjs_value_is_number (value));
    TEST_ASSERT (jjs_value_as_number (value) == 32.0);
    return true;
  }
  else if (!strncmp (str_buf_p, "bravo", (size_t) sz))
  {
    TEST_ASSERT (jjs_value_is_boolean (value));
    TEST_ASSERT (jjs_value_is_true (value) == false);
    TEST_ASSERT (jjs_value_is_false (value));
    return true;
  }
  else if (!strncmp (str_buf_p, "charlie", (size_t) sz))
  {
    TEST_ASSERT (jjs_value_is_object (value));
    return true;
  }
  else if (!strncmp (str_buf_p, "delta", (size_t) sz))
  {
    TEST_ASSERT (jjs_value_is_number (value));
    TEST_ASSERT_DOUBLE_EQUALS (jjs_value_as_number (value), 123.45);
    return true;
  }
  else if (!strncmp (str_buf_p, "echo", (size_t) sz))
  {
    TEST_ASSERT (jjs_value_is_string (value));
    jjs_size_t echo_sz =
      jjs_string_to_buffer (value, JJS_ENCODING_CESU8, (jjs_char_t *) str_buf_p, sizeof (str_buf_p) - 1);
    str_buf_p[echo_sz] = '\0';
    TEST_ASSERT (!strncmp (str_buf_p, "foobar", (size_t) echo_sz));
    return true;
  }

  TEST_ASSERT (false);
  return false;
} /* foreach */

static bool
foreach_exception (const jjs_value_t name, /**< field name */
                   const jjs_value_t value, /**< field value */
                   void *user_data) /**< user data */
{
  JJS_UNUSED (value);
  JJS_UNUSED (user_data);
  char str_buf_p[128];
  jjs_size_t sz =
    jjs_string_to_buffer (name, JJS_ENCODING_CESU8, (jjs_char_t *) str_buf_p, sizeof (str_buf_p) - 1);
  str_buf_p[sz] = '\0';

  if (!strncmp (str_buf_p, "foxtrot", (size_t) sz))
  {
    TEST_ASSERT (false);
  }

  return true;
} /* foreach_exception */

static bool
foreach_subset (const jjs_value_t name, /**< field name */
                const jjs_value_t value, /**< field value */
                void *user_data) /**< user data */
{
  JJS_UNUSED (name);
  JJS_UNUSED (value);
  int *count_p = (int *) (user_data);

  if (*count_p == 3)
  {
    return false;
  }
  (*count_p)++;
  return true;
} /* foreach_subset */

static jjs_value_t
get_property (const jjs_value_t obj_val, /**< object value */
              const char *str_p) /**< property name */
{
  jjs_value_t prop_name_val = jjs_string_sz (str_p);
  jjs_value_t ret_val = jjs_object_get (obj_val, prop_name_val);
  jjs_value_free (prop_name_val);
  return ret_val;
} /* get_property */

static jjs_value_t
set_property (const jjs_value_t obj_val, /**< object value */
              const char *str_p, /**< property name */
              const jjs_value_t val) /**< value to set */
{
  jjs_value_t prop_name_val = jjs_string_sz (str_p);
  jjs_value_t ret_val = jjs_object_set (obj_val, prop_name_val, val);
  jjs_value_free (prop_name_val);
  return ret_val;
} /* set_property */

static void
test_syntax_error (const char *script_p, /**< source code to run */
                   const jjs_parse_options_t *options_p, /**< additional parsing options */
                   const char *error_message_p, /**< error message */
                   bool run_script) /**< run script before checking the error message */
{
  jjs_value_t result_val = jjs_parse ((const jjs_char_t *) script_p, strlen (script_p), options_p);

  if (run_script)
  {
    TEST_ASSERT (!jjs_value_is_exception (result_val));
    jjs_value_t script_val = result_val;

    result_val = jjs_run (script_val);
    jjs_value_free (script_val);
  }

  TEST_ASSERT (jjs_value_is_exception (result_val));
  result_val = jjs_exception_value (result_val, true);

  jjs_value_t err_str_val = jjs_value_to_string (result_val);
  jjs_size_t err_str_size = jjs_string_size (err_str_val, JJS_ENCODING_CESU8);
  jjs_char_t err_str_buf[256];

  TEST_ASSERT (err_str_size <= sizeof (err_str_buf));
  TEST_ASSERT (err_str_size == strlen (error_message_p));

  TEST_ASSERT (jjs_string_to_buffer (err_str_val, JJS_ENCODING_CESU8, err_str_buf, err_str_size) == err_str_size);

  jjs_value_free (err_str_val);
  jjs_value_free (result_val);
  TEST_ASSERT (memcmp ((char *) err_str_buf, error_message_p, err_str_size) == 0);
} /* test_syntax_error */

int
main (void)
{
  TEST_INIT ();

  bool is_ok;
  jjs_size_t sz, cesu8_sz;
  jjs_length_t cesu8_length;
  jjs_value_t val_t, val_foo, val_bar, val_A, val_A_prototype, val_a, val_a_foo, val_value_field, val_p, val_np;
  jjs_value_t val_call_external;
  jjs_value_t global_obj_val, obj_val;
  jjs_value_t external_func_val, external_construct_val;
  jjs_value_t throw_test_handler_val;
  jjs_value_t parsed_code_val, proto_val, prim_val;
  jjs_value_t res, args[2];
  double number_val;
  char buffer[32];

  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  parsed_code_val = jjs_parse (test_source, sizeof (test_source) - 1, NULL);
  TEST_ASSERT (!jjs_value_is_exception (parsed_code_val));

  res = jjs_run (parsed_code_val);
  TEST_ASSERT (!jjs_value_is_exception (res));
  jjs_value_free (res);
  jjs_value_free (parsed_code_val);

  global_obj_val = jjs_current_realm ();

  /* Get global.boo (non-existing field) */
  val_t = get_property (global_obj_val, "boo");
  TEST_ASSERT (!jjs_value_is_exception (val_t));
  TEST_ASSERT (jjs_value_is_undefined (val_t));

  /* Get global.t */
  val_t = get_property (global_obj_val, "t");
  TEST_ASSERT (!jjs_value_is_exception (val_t));
  TEST_ASSERT (jjs_value_is_number (val_t) && jjs_value_as_number (val_t) == 1.0);
  jjs_value_free (val_t);

  /* Get global.foo */
  val_foo = get_property (global_obj_val, "foo");
  TEST_ASSERT (!jjs_value_is_exception (val_foo));
  TEST_ASSERT (jjs_value_is_object (val_foo));

  /* Call foo (4, 2) */
  args[0] = jjs_number (4);
  args[1] = jjs_number (2);
  res = jjs_call (val_foo, jjs_undefined (), args, 2);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_number (res) && jjs_value_as_number (res) == 1.0);
  jjs_value_free (res);

  /* Get global.bar */
  val_bar = get_property (global_obj_val, "bar");
  TEST_ASSERT (!jjs_value_is_exception (val_bar));
  TEST_ASSERT (jjs_value_is_object (val_bar));

  /* Call bar (4, 2) */
  res = jjs_call (val_bar, jjs_undefined (), args, 2);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_number (res) && jjs_value_as_number (res) == 5.0);
  jjs_value_free (res);
  jjs_value_free (val_bar);

  /* Set global.t = "abcd" */
  jjs_value_free (args[0]);
  args[0] = jjs_string_sz ("abcd");
  res = set_property (global_obj_val, "t", args[0]);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_true (res));
  jjs_value_free (res);

  /* Call foo (4, 2) */
  res = jjs_call (val_foo, jjs_undefined (), args, 2);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_string (res));
  sz = jjs_string_size (res, JJS_ENCODING_CESU8);
  TEST_ASSERT (sz == 4);
  sz = jjs_string_to_buffer (res, JJS_ENCODING_CESU8, (jjs_char_t *) buffer, sz);
  TEST_ASSERT (sz == 4);
  jjs_value_free (res);
  TEST_ASSERT (!strncmp (buffer, "abcd", (size_t) sz));
  jjs_value_free (args[0]);
  jjs_value_free (args[1]);

  /* Get global.A */
  val_A = get_property (global_obj_val, "A");
  TEST_ASSERT (!jjs_value_is_exception (val_A));
  TEST_ASSERT (jjs_value_is_object (val_A));

  /* Get A.prototype */
  is_ok = jjs_value_is_constructor (val_A);
  TEST_ASSERT (is_ok);
  val_A_prototype = get_property (val_A, "prototype");
  TEST_ASSERT (!jjs_value_is_exception (val_A_prototype));
  TEST_ASSERT (jjs_value_is_object (val_A_prototype));
  jjs_value_free (val_A);

  /* Set A.prototype.foo = global.foo */
  res = set_property (val_A_prototype, "foo", val_foo);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_true (res));
  jjs_value_free (res);
  jjs_value_free (val_A_prototype);
  jjs_value_free (val_foo);

  /* Get global.a */
  val_a = get_property (global_obj_val, "a");
  TEST_ASSERT (!jjs_value_is_exception (val_a));
  TEST_ASSERT (jjs_value_is_object (val_a));

  /* Get a.t */
  res = get_property (val_a, "t");
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_number (res) && jjs_value_as_number (res) == 12.0);
  jjs_value_free (res);

  /* foreach properties */
  val_p = get_property (global_obj_val, "p");
  is_ok = jjs_object_foreach (val_p, foreach, (void *) "user_data");
  TEST_ASSERT (is_ok);

  /* break foreach at third element */
  int count = 0;
  is_ok = jjs_object_foreach (val_p, foreach_subset, &count);
  TEST_ASSERT (is_ok);
  TEST_ASSERT (count == 3);
  jjs_value_free (val_p);

  /* foreach with throw test */
  val_np = get_property (global_obj_val, "np");
  is_ok = !jjs_object_foreach (val_np, foreach_exception, NULL);
  TEST_ASSERT (is_ok);
  jjs_value_free (val_np);

  /* Get a.foo */
  val_a_foo = get_property (val_a, "foo");
  TEST_ASSERT (!jjs_value_is_exception (val_a_foo));
  TEST_ASSERT (jjs_value_is_object (val_a_foo));

  /* Call a.foo () */
  res = jjs_call (val_a_foo, val_a, NULL, 0);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_number (res) && jjs_value_as_number (res) == 12.0);
  jjs_value_free (res);
  jjs_value_free (val_a_foo);

  jjs_value_free (val_a);

  /* Create native handler bound function object and set it to 'external' variable */
  external_func_val = jjs_function_external (handler);
  TEST_ASSERT (jjs_value_is_function (external_func_val) && jjs_value_is_constructor (external_func_val));

  res = set_property (global_obj_val, "external", external_func_val);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_true (res));
  jjs_value_free (external_func_val);

  /* Call 'call_external' function that should call external function created above */
  val_call_external = get_property (global_obj_val, "call_external");
  TEST_ASSERT (!jjs_value_is_exception (val_call_external));
  TEST_ASSERT (jjs_value_is_object (val_call_external));
  res = jjs_call (val_call_external, global_obj_val, NULL, 0);
  jjs_value_free (val_call_external);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_string (res));
  sz = jjs_string_size (res, JJS_ENCODING_CESU8);
  TEST_ASSERT (sz == 19);
  sz = jjs_string_to_buffer (res, JJS_ENCODING_CESU8, (jjs_char_t *) buffer, sz);
  TEST_ASSERT (sz == 19);
  jjs_value_free (res);
  TEST_ASSERT (!strncmp (buffer, "string from handler", (size_t) sz));

  /* Create native handler bound function object and set it to 'external_construct' variable */
  external_construct_val = jjs_function_external (handler_construct);
  TEST_ASSERT (jjs_value_is_function (external_construct_val) && jjs_value_is_constructor (external_construct_val));

  res = set_property (global_obj_val, "external_construct", external_construct_val);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_true (res));
  jjs_value_free (res);

  /* Call external function created above, as constructor */
  args[0] = jjs_boolean (true);
  res = jjs_construct (external_construct_val, args, 1);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_object (res));
  val_value_field = get_property (res, "value_field");

  /* Get 'value_field' of constructed object */
  TEST_ASSERT (!jjs_value_is_exception (val_value_field));
  TEST_ASSERT (jjs_value_is_boolean (val_value_field) && jjs_value_is_true (val_value_field));
  jjs_value_free (val_value_field);
  jjs_value_free (external_construct_val);

  TEST_ASSERT (jjs_object_has_native_ptr (res, &JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (bind2)));
  void *ptr = jjs_object_get_native_ptr (res, &JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (bind2));
  TEST_ASSERT ((uintptr_t) ptr == (uintptr_t) 0x0012345678abcdefull);

  /* Passing NULL for info_p is allowed. */
  TEST_ASSERT (!jjs_object_has_native_ptr (res, NULL));
  jjs_object_set_native_ptr (res, NULL, (void *) 0x0012345678abcdefull);

  TEST_ASSERT (jjs_object_has_native_ptr (res, NULL));
  ptr = jjs_object_get_native_ptr (res, NULL);
  TEST_ASSERT ((uintptr_t) ptr == (uintptr_t) 0x0012345678abcdefull);

  jjs_value_free (res);

  /* Test: It is ok to set native pointer's free callback as NULL. */
  jjs_value_t obj_freecb = jjs_object ();
  jjs_object_set_native_ptr (obj_freecb, &JJS_NATIVE_HANDLE_INFO_FOR_CTYPE (bind3), (void *) 0x1234);

  jjs_value_free (obj_freecb);

  /* Test: Throwing exception from native handler. */
  throw_test_handler_val = jjs_function_external (handler_throw_test);
  TEST_ASSERT (jjs_value_is_function (throw_test_handler_val));

  res = set_property (global_obj_val, "throw_test", throw_test_handler_val);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_true (res));
  jjs_value_free (res);
  jjs_value_free (throw_test_handler_val);

  val_t = get_property (global_obj_val, "call_throw_test");
  TEST_ASSERT (!jjs_value_is_exception (val_t));
  TEST_ASSERT (jjs_value_is_object (val_t));

  res = jjs_call (val_t, global_obj_val, NULL, 0);
  TEST_ASSERT (!jjs_value_is_exception (res));
  jjs_value_free (val_t);
  jjs_value_free (res);

  /* Test: Unhandled exception in called function */
  val_t = get_property (global_obj_val, "throw_reference_error");
  TEST_ASSERT (!jjs_value_is_exception (val_t));
  TEST_ASSERT (jjs_value_is_object (val_t));

  res = jjs_call (val_t, global_obj_val, NULL, 0);

  TEST_ASSERT (jjs_value_is_exception (res));
  jjs_value_free (val_t);

  /* 'res' should contain exception object */
  res = jjs_exception_value (res, true);
  TEST_ASSERT (jjs_value_is_object (res));
  jjs_value_free (res);

  /* Test: Call of non-function */
  obj_val = jjs_object ();
  res = jjs_call (obj_val, global_obj_val, NULL, 0);
  TEST_ASSERT (jjs_value_is_exception (res));

  /* 'res' should contain exception object */
  res = jjs_exception_value (res, true);
  TEST_ASSERT (jjs_value_is_object (res));
  jjs_value_free (res);

  jjs_value_free (obj_val);

  /* Test: Unhandled exception in function called, as constructor */
  val_t = get_property (global_obj_val, "throw_reference_error");
  TEST_ASSERT (!jjs_value_is_exception (val_t));
  TEST_ASSERT (jjs_value_is_object (val_t));

  res = jjs_construct (val_t, NULL, 0);
  TEST_ASSERT (jjs_value_is_exception (res));
  jjs_value_free (val_t);

  /* 'res' should contain exception object */
  res = jjs_exception_value (res, true);
  TEST_ASSERT (jjs_value_is_object (res));
  jjs_value_free (res);

  /* Test: Call of non-function as constructor */
  obj_val = jjs_object ();
  res = jjs_construct (obj_val, NULL, 0);
  TEST_ASSERT (jjs_value_is_exception (res));

  /* 'res' should contain exception object */
  res = jjs_exception_value (res, true);
  TEST_ASSERT (jjs_value_is_object (res));
  jjs_value_free (res);

  jjs_value_free (obj_val);

  /* Test: Array Object API */
  jjs_value_t array_obj_val = jjs_array (10);
  TEST_ASSERT (jjs_value_is_array (array_obj_val));
  TEST_ASSERT (jjs_array_length (array_obj_val) == 10);

  jjs_value_t v_in = jjs_number (10.5);
  res = jjs_object_set_index (array_obj_val, 5, v_in);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_boolean (res) && jjs_value_is_true (res));
  jjs_value_free (res);
  jjs_value_t v_out = jjs_object_get_index (array_obj_val, 5);

  TEST_ASSERT (jjs_value_is_number (v_out) && jjs_value_as_number (v_out) == 10.5);

  jjs_object_delete_index (array_obj_val, 5);
  jjs_value_t v_und = jjs_object_get_index (array_obj_val, 5);

  TEST_ASSERT (jjs_value_is_undefined (v_und));

  jjs_value_free (v_in);
  jjs_value_free (v_out);
  jjs_value_free (v_und);
  jjs_value_free (array_obj_val);

  /* Test: object keys */
  res = jjs_object_keys (global_obj_val);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_array (res));
  TEST_ASSERT (jjs_array_length (res) == 19);
  jjs_value_free (res);

  /* Test: jjs_value_to_primitive */
  obj_val = jjs_eval ((jjs_char_t *) "new String ('hello')", 20, JJS_PARSE_NO_OPTS);
  TEST_ASSERT (!jjs_value_is_exception (obj_val));
  TEST_ASSERT (jjs_value_is_object (obj_val));
  TEST_ASSERT (!jjs_value_is_string (obj_val));
  prim_val = jjs_value_to_primitive (obj_val);
  TEST_ASSERT (!jjs_value_is_exception (prim_val));
  TEST_ASSERT (jjs_value_is_string (prim_val));
  jjs_value_free (prim_val);

  /* Test: jjs_object_proto */
  proto_val = jjs_object_proto (jjs_undefined ());
  TEST_ASSERT (jjs_value_is_exception (proto_val));
  jjs_value_t error = jjs_exception_value (proto_val, true);
  TEST_ASSERT (jjs_error_type (error) == JJS_ERROR_TYPE);
  jjs_value_free (error);

  proto_val = jjs_object_proto (obj_val);
  TEST_ASSERT (!jjs_value_is_exception (proto_val));
  TEST_ASSERT (jjs_value_is_object (proto_val));
  jjs_value_free (proto_val);
  jjs_value_free (obj_val);

  if (jjs_feature_enabled (JJS_FEATURE_PROXY))
  {
    jjs_value_t target = jjs_object ();
    jjs_value_t handler = jjs_object ();
    jjs_value_t proxy = jjs_proxy (target, handler);
    jjs_value_t obj_proto = jjs_eval ((jjs_char_t *) "Object.prototype", 16, JJS_PARSE_NO_OPTS);

    jjs_value_free (target);
    jjs_value_free (handler);
    proto_val = jjs_object_proto (proxy);
    TEST_ASSERT (!jjs_value_is_exception (proto_val));
    TEST_ASSERT (proto_val == obj_proto);
    jjs_value_free (proto_val);
    jjs_value_free (obj_proto);
    jjs_value_free (proxy);
  }

  /* Test: jjs_object_set_proto */
  obj_val = jjs_object ();
  res = jjs_object_set_proto (obj_val, jjs_null ());
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_boolean (res));
  TEST_ASSERT (jjs_value_is_true (res));

  jjs_value_t new_proto = jjs_object ();
  res = jjs_object_set_proto (obj_val, new_proto);
  jjs_value_free (new_proto);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_boolean (res));
  TEST_ASSERT (jjs_value_is_true (res));
  proto_val = jjs_object_proto (obj_val);
  TEST_ASSERT (!jjs_value_is_exception (proto_val));
  TEST_ASSERT (jjs_value_is_object (proto_val));
  jjs_value_free (proto_val);
  jjs_value_free (obj_val);

  if (jjs_feature_enabled (JJS_FEATURE_PROXY))
  {
    jjs_value_t target = jjs_object ();
    jjs_value_t handler = jjs_object ();
    jjs_value_t proxy = jjs_proxy (target, handler);
    new_proto = jjs_eval ((jjs_char_t *) "Function.prototype", 18, JJS_PARSE_NO_OPTS);

    res = jjs_object_set_proto (proxy, new_proto);
    TEST_ASSERT (!jjs_value_is_exception (res));
    jjs_value_t target_proto = jjs_object_proto (target);
    TEST_ASSERT (target_proto == new_proto);

    jjs_value_free (target);
    jjs_value_free (handler);
    jjs_value_free (proxy);
    jjs_value_free (new_proto);
    jjs_value_free (target_proto);
  }

  /* Test: eval */
  const jjs_char_t eval_code_src1[] = "(function () { return 123; })";
  val_t = jjs_eval (eval_code_src1, sizeof (eval_code_src1) - 1, JJS_PARSE_STRICT_MODE);
  TEST_ASSERT (!jjs_value_is_exception (val_t));
  TEST_ASSERT (jjs_value_is_object (val_t));
  TEST_ASSERT (jjs_value_is_function (val_t));

  res = jjs_call (val_t, jjs_undefined (), NULL, 0);
  TEST_ASSERT (!jjs_value_is_exception (res));
  TEST_ASSERT (jjs_value_is_number (res) && jjs_value_as_number (res) == 123.0);
  jjs_value_free (res);

  jjs_value_free (val_t);

  /* cleanup. */
  jjs_value_free (global_obj_val);

  /* Test: run gc. */
  jjs_heap_gc (JJS_GC_PRESSURE_LOW);

  /* Test: spaces */
  const jjs_char_t eval_code_src2[] = "\x0a \x0b \x0c \xc2\xa0 \xe2\x80\xa8 \xe2\x80\xa9 \xef\xbb\xbf 4321";
  val_t = jjs_eval (eval_code_src2, sizeof (eval_code_src2) - 1, JJS_PARSE_STRICT_MODE);
  TEST_ASSERT (!jjs_value_is_exception (val_t));
  TEST_ASSERT (jjs_value_is_number (val_t) && jjs_value_as_number (val_t) == 4321.0);
  jjs_value_free (val_t);

  /* Test: number */
  val_t = jjs_number (6.25);
  number_val = jjs_value_as_number (val_t);
  TEST_ASSERT (number_val * 3 == 18.75);
  jjs_value_free (val_t);

  val_t = jjs_infinity (true);
  number_val = jjs_value_as_number (val_t);
  TEST_ASSERT (number_val * 3 == number_val && number_val != 0.0);
  jjs_value_free (val_t);

  val_t = jjs_nan ();
  number_val = jjs_value_as_number (val_t);
  TEST_ASSERT (number_val != number_val);
  jjs_value_free (val_t);

  /* Test: create function */
  jjs_value_t script_source = jjs_string_sz ("  return 5 +  a+\nb+c");

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_HAS_ARGUMENT_LIST;
  parse_options.argument_list = jjs_string_sz ("a , b,c");

  jjs_value_t func_val = jjs_parse_value (script_source, &parse_options);

  TEST_ASSERT (!jjs_value_is_exception (func_val));

  jjs_value_free (parse_options.argument_list);
  jjs_value_free (script_source);

  jjs_value_t func_args[3] = { jjs_number (4), jjs_number (6), jjs_number (-2) };

  val_t = jjs_call (func_val, func_args[0], func_args, 3);
  number_val = jjs_value_as_number (val_t);
  TEST_ASSERT (number_val == 13.0);

  jjs_value_free (val_t);
  jjs_value_free (func_val);

  parse_options.options = JJS_PARSE_HAS_ARGUMENT_LIST;
  parse_options.argument_list = jjs_null ();

  func_val = jjs_parse ((const jjs_char_t *) "", 0, &parse_options);
  jjs_value_free (parse_options.argument_list);

  TEST_ASSERT (jjs_value_is_exception (func_val) && jjs_error_type (func_val) == JJS_ERROR_TYPE);
  jjs_value_free (func_val);

  script_source = jjs_number (4.5);
  func_val = jjs_parse_value (script_source, NULL);
  jjs_value_free (script_source);

  TEST_ASSERT (jjs_value_is_exception (func_val) && jjs_error_type (func_val) == JJS_ERROR_TYPE);
  jjs_value_free (func_val);

  jjs_cleanup ();

  TEST_ASSERT (test_api_is_free_callback_was_called);

  /* Test: jjs_exception_value */
  {
    TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);
    jjs_value_t num_val = jjs_number (123);
    num_val = jjs_throw_value (num_val, true);
    TEST_ASSERT (jjs_value_is_exception (num_val));
    jjs_value_t num2_val = jjs_exception_value (num_val, false);
    TEST_ASSERT (jjs_value_is_exception (num_val));
    TEST_ASSERT (!jjs_value_is_exception (num2_val));
    double num = jjs_value_as_number (num2_val);
    TEST_ASSERT (num == 123);
    num2_val = jjs_exception_value (num_val, true);
    TEST_ASSERT (!jjs_value_is_exception (num2_val));
    num = jjs_value_as_number (num2_val);
    TEST_ASSERT (num == 123);
    jjs_value_free (num2_val);
    jjs_cleanup ();
  }

  {
    TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);
    const jjs_char_t scoped_src_p[] = "let a; this.b = 5";
    jjs_value_t parse_result = jjs_parse (scoped_src_p, sizeof (scoped_src_p) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));
    jjs_value_free (parse_result);

    parse_result = jjs_parse (scoped_src_p, sizeof (scoped_src_p) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));

    jjs_value_t run_result = jjs_run (parse_result);
    TEST_ASSERT (!jjs_value_is_exception (run_result));
    jjs_value_free (run_result);

    /* Should be a syntax error due to redeclaration. */
    run_result = jjs_run (parse_result);
    TEST_ASSERT (jjs_value_is_exception (run_result));
    jjs_value_free (run_result);
    jjs_value_free (parse_result);

    /* The variable should have no effect on parsing. */
    parse_result = jjs_parse (scoped_src_p, sizeof (scoped_src_p) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));
    jjs_value_free (parse_result);

    /* The already existing global binding should not affect a new lexical binding */
    const jjs_char_t scoped_src2_p[] = "let b = 6; this.b + b";
    parse_result = jjs_parse (scoped_src2_p, sizeof (scoped_src2_p) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));
    run_result = jjs_run (parse_result);
    TEST_ASSERT (jjs_value_is_number (run_result));
    TEST_ASSERT (jjs_value_as_number (run_result) == 11);
    jjs_value_free (run_result);
    jjs_value_free (parse_result);

    /* Check restricted global property */
    const jjs_char_t scoped_src3_p[] = "let undefined;";
    parse_result = jjs_parse (scoped_src3_p, sizeof (scoped_src3_p) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));
    run_result = jjs_run (parse_result);
    TEST_ASSERT (jjs_value_is_exception (run_result));
    TEST_ASSERT (jjs_error_type (run_result) == JJS_ERROR_SYNTAX);
    jjs_value_free (run_result);
    jjs_value_free (parse_result);

    jjs_value_t global_obj = jjs_current_realm ();
    jjs_value_t prop_name = jjs_string_sz ("foo");

    jjs_property_descriptor_t prop_desc = jjs_property_descriptor ();
    prop_desc.flags |= JJS_PROP_IS_VALUE_DEFINED;
    prop_desc.value = jjs_number (5.2);

    jjs_value_t define_result = jjs_object_define_own_prop (global_obj, prop_name, &prop_desc);
    TEST_ASSERT (jjs_value_is_boolean (define_result) && jjs_value_is_true (define_result));
    jjs_value_free (define_result);

    jjs_property_descriptor_free (&prop_desc);
    jjs_value_free (prop_name);
    jjs_value_free (global_obj);

    const jjs_char_t scoped_src4_p[] = "let foo;";
    parse_result = jjs_parse (scoped_src4_p, sizeof (scoped_src4_p) - 1, NULL);
    TEST_ASSERT (!jjs_value_is_exception (parse_result));
    run_result = jjs_run (parse_result);
    TEST_ASSERT (jjs_value_is_exception (run_result));
    TEST_ASSERT (jjs_error_type (run_result) == JJS_ERROR_SYNTAX);
    jjs_value_free (run_result);
    jjs_value_free (parse_result);

    if (jjs_feature_enabled (JJS_FEATURE_REALM))
    {
      const jjs_char_t proxy_src_p[] = "new Proxy({}, { getOwnPropertyDescriptor() { throw 42.1 }})";
      jjs_value_t proxy = jjs_eval (proxy_src_p, sizeof (proxy_src_p) - 1, JJS_PARSE_NO_OPTS);
      TEST_ASSERT (jjs_value_is_object (proxy));
      jjs_value_t new_realm_value = jjs_realm ();

      jjs_value_t set_realm_this_result = jjs_realm_set_this (new_realm_value, proxy);
      TEST_ASSERT (jjs_value_is_boolean (set_realm_this_result) && jjs_value_is_true (set_realm_this_result));
      jjs_value_free (set_realm_this_result);

      jjs_value_t old_realm = jjs_set_realm (new_realm_value);

      const jjs_char_t scoped_src5_p[] = "let a;";
      parse_result = jjs_parse (scoped_src5_p, sizeof (scoped_src5_p) - 1, NULL);
      TEST_ASSERT (!jjs_value_is_exception (parse_result));
      run_result = jjs_run (parse_result);
      TEST_ASSERT (jjs_value_is_exception (run_result));
      jjs_value_t error_value = jjs_exception_value (run_result, false);
      TEST_ASSERT (jjs_value_is_number (error_value));
      TEST_ASSERT_DOUBLE_EQUALS(jjs_value_as_number (error_value), 42.1);
      jjs_value_free (error_value);
      jjs_value_free (run_result);
      jjs_value_free (parse_result);

      jjs_set_realm (old_realm);

      jjs_value_free (new_realm_value);
      jjs_value_free (proxy);

      const jjs_char_t proxy_src2_p[] = "new Proxy(Object.defineProperty({}, 'b', {value: 5.2}), {})";
      proxy = jjs_eval (proxy_src2_p, sizeof (proxy_src2_p) - 1, JJS_PARSE_NO_OPTS);
      TEST_ASSERT (jjs_value_is_object (proxy));
      new_realm_value = jjs_realm ();

      set_realm_this_result = jjs_realm_set_this (new_realm_value, proxy);
      TEST_ASSERT (jjs_value_is_boolean (set_realm_this_result) && jjs_value_is_true (set_realm_this_result));
      jjs_value_free (set_realm_this_result);

      old_realm = jjs_set_realm (new_realm_value);

      const jjs_char_t scoped_src6_p[] = "let b;";
      parse_result = jjs_parse (scoped_src6_p, sizeof (scoped_src6_p) - 1, NULL);
      TEST_ASSERT (!jjs_value_is_exception (parse_result));
      run_result = jjs_run (parse_result);
      TEST_ASSERT (jjs_value_is_exception (run_result));
      TEST_ASSERT (jjs_value_is_exception (run_result));
      TEST_ASSERT (jjs_error_type (run_result) == JJS_ERROR_SYNTAX);
      jjs_value_free (run_result);
      jjs_value_free (parse_result);

      jjs_set_realm (old_realm);

      jjs_value_free (new_realm_value);
      jjs_value_free (proxy);
    }

    jjs_cleanup ();
  }

  /* Test: parser error location */
  if (jjs_feature_enabled (JJS_FEATURE_ERROR_MESSAGES))
  {
    TEST_ASSERT (jjs_init_with_flags (JJS_CONTEXT_FLAG_SHOW_OPCODES) == JJS_CONTEXT_STATUS_OK);

    test_syntax_error ("b = 'hello';\nvar a = (;",
                       NULL,
                       "SyntaxError: Unexpected end of input [<anonymous>:2:10]",
                       false);

    parse_options.options = JJS_PARSE_HAS_SOURCE_NAME;
    parse_options.source_name = jjs_string_sz ("filename.js");

    test_syntax_error ("b = 'hello';\nvar a = (;",
                       &parse_options,
                       "SyntaxError: Unexpected end of input [filename.js:2:10]",
                       false);

    test_syntax_error ("eval(\"var b;\\nfor (,); \");",
                       &parse_options,
                       "SyntaxError: Unexpected end of input [<eval>:2:6]",
                       true);

    parse_options.options |= JJS_PARSE_HAS_START;
    parse_options.start_line = 10;
    parse_options.start_column = 20;

    test_syntax_error ("for (var a in []",
                       &parse_options,
                       "SyntaxError: Expected ')' token [filename.js:10:36]",
                       false);

    jjs_value_free (parse_options.source_name);
    jjs_cleanup ();
  }

  /* External Magic String */
  TEST_ASSERT (jjs_init_with_flags (JJS_CONTEXT_FLAG_SHOW_OPCODES) == JJS_CONTEXT_STATUS_OK);

  uint32_t num_magic_string_items = (uint32_t) (sizeof (magic_string_items) / sizeof (jjs_char_t *));
  jjs_register_magic_strings (magic_string_items, num_magic_string_items, magic_string_lengths);

  const jjs_char_t ms_code_src[] = "var global = {}; var console = [1]; var process = 1;";
  parsed_code_val = jjs_parse (ms_code_src, sizeof (ms_code_src) - 1, NULL);
  TEST_ASSERT (!jjs_value_is_exception (parsed_code_val));

  res = jjs_run (parsed_code_val);
  TEST_ASSERT (!jjs_value_is_exception (res));
  jjs_value_free (res);
  jjs_value_free (parsed_code_val);

  /* call jjs_string_sz functions which will returns with the registered external magic strings */
  args[0] = jjs_string_sz ("console");
  args[1] = jjs_string_sz ("\xed\xa0\x80\xed\xb6\x8a"); /**< greek zero sign */

  cesu8_length = jjs_string_length (args[0]);
  cesu8_sz = jjs_string_size (args[0], JJS_ENCODING_CESU8);

  JJS_VLA (char, string_console, cesu8_sz);
  jjs_string_to_buffer (args[0], JJS_ENCODING_CESU8, (jjs_char_t *) string_console, cesu8_sz);

  TEST_ASSERT (!strncmp (string_console, "console", cesu8_sz));
  TEST_ASSERT (cesu8_length == 7);
  TEST_ASSERT (cesu8_length == cesu8_sz);

  jjs_value_free (args[0]);

  const jjs_char_t test_magic_str_access_src[] = "'console'.charAt(6) == 'e'";
  res = jjs_eval (test_magic_str_access_src, sizeof (test_magic_str_access_src) - 1, JJS_PARSE_NO_OPTS);
  TEST_ASSERT (jjs_value_is_boolean (res));
  TEST_ASSERT (jjs_value_is_true (res));

  jjs_value_free (res);

  cesu8_length = jjs_string_length (args[1]);
  cesu8_sz = jjs_string_size (args[1], JJS_ENCODING_CESU8);

  JJS_VLA (char, string_greek_zero_sign, cesu8_sz);
  jjs_string_to_buffer (args[1], JJS_ENCODING_CESU8, (jjs_char_t *) string_greek_zero_sign, cesu8_sz);

  TEST_ASSERT (!strncmp (string_greek_zero_sign, "\xed\xa0\x80\xed\xb6\x8a", cesu8_sz));
  TEST_ASSERT (cesu8_length == 2);
  TEST_ASSERT (cesu8_sz == 6);

  jjs_value_free (args[1]);

  jjs_cleanup ();

  return 0;
} /* main */
