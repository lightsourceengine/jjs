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

#include <stdio.h>

/**
 * Type to describe test cases.
 */
typedef struct
{
  jjs_typedarray_type_t typedarray_type; /**< what kind of TypedArray */
  char *constructor_name; /**< JS constructor name for TypedArray */
  uint32_t element_count; /**< number of elements for the TypedArray */
  uint32_t bytes_per_element; /**< bytes per elment of the given typedarray_type */
} test_entry_t;

/**
 * Register a JavaScript value in the global object.
 */
static void
register_js_value (const char *name_p, /**< name of the function */
                   jjs_value_t value) /**< function callback */
{
  jjs_value_t global_obj_val = jjs_current_realm (ctx ());

  jjs_value_t name_val = jjs_string_sz (ctx (), name_p);
  jjs_value_t result_val = jjs_object_set (ctx (), global_obj_val, name_val, value);

  jjs_value_free (ctx (), name_val);
  jjs_value_free (ctx (), global_obj_val);

  jjs_value_free (ctx (), result_val);
} /* register_js_value */

static jjs_value_t
assert_handler (const jjs_call_info_t *call_info_p, /**< call information */
                const jjs_value_t args_p[], /**< function arguments */
                const jjs_length_t args_cnt) /**< number of function arguments */
{
  JJS_UNUSED (call_info_p);

  if (jjs_value_is_true (ctx (), args_p[0]))
  {
    return jjs_boolean (ctx (), true);
  }
  else
  {
    if (args_cnt > 1 && jjs_value_is_string (ctx (), args_p[1]))
    {
      jjs_char_t utf8_string[128];
      jjs_size_t copied =
        jjs_string_to_buffer (ctx (), args_p[1], JJS_ENCODING_UTF8, utf8_string, sizeof (utf8_string) - 1);
      utf8_string[copied] = '\0';

      printf ("JS assert: %s\n", utf8_string);
    }
    TEST_ASSERT (false);
    return jjs_undefined (ctx ());
  }
} /* assert_handler */

/**
 * Do simple TypedArray property validation.
 */
static void
test_typedarray_info (jjs_value_t typedarray, /**< target TypedArray to query */
                      jjs_typedarray_type_t typedarray_type, /**< expected TypedArray type */
                      jjs_length_t element_count, /**< expected element count */
                      jjs_length_t bytes_per_element) /**< bytes per element for the given type */
{
  TEST_ASSERT (!jjs_value_is_exception (ctx (), typedarray));
  TEST_ASSERT (jjs_value_is_typedarray (ctx (), typedarray));
  TEST_ASSERT (jjs_typedarray_type (ctx (), typedarray) == typedarray_type);
  TEST_ASSERT (jjs_typedarray_length (ctx (), typedarray) == element_count);

  jjs_length_t byte_length = (uint32_t) -1;
  jjs_length_t byte_offset = (uint32_t) -1;
  jjs_value_t arraybuffer = jjs_typedarray_buffer (ctx (), typedarray, &byte_offset, &byte_length);
  TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));

  TEST_ASSERT (byte_length == element_count * bytes_per_element);
  TEST_ASSERT (byte_offset == 0);

  jjs_value_free (ctx (), arraybuffer);
} /* test_typedarray_info */

/**
 * Test construction of TypedArrays and validate properties.
 */
static void
test_typedarray_queries (test_entry_t test_entries[]) /**< test cases */
{
  jjs_value_t global_obj_val = jjs_current_realm (ctx ());

  for (uint32_t i = 0; test_entries[i].constructor_name != NULL; i++)
  {
    /* Create TypedArray via construct call */
    {
      jjs_value_t prop_name = jjs_string_sz (ctx (), test_entries[i].constructor_name);
      jjs_value_t prop_value = jjs_object_get (ctx (), global_obj_val, prop_name);
      TEST_ASSERT (!jjs_value_is_exception (ctx (), prop_value));
      jjs_value_t length_arg = jjs_number (ctx (), test_entries[i].element_count);

      jjs_value_t typedarray = jjs_construct (ctx (), prop_value, &length_arg, 1);

      jjs_value_free (ctx (), prop_name);
      jjs_value_free (ctx (), prop_value);
      jjs_value_free (ctx (), length_arg);

      test_typedarray_info (typedarray,
                            test_entries[i].typedarray_type,
                            test_entries[i].element_count,
                            test_entries[i].bytes_per_element);
      jjs_value_free (ctx (), typedarray);
    }

    /* Create TypedArray via api call */
    {
      jjs_value_t typedarray = jjs_typedarray (ctx (), test_entries[i].typedarray_type, test_entries[i].element_count);
      test_typedarray_info (typedarray,
                            test_entries[i].typedarray_type,
                            test_entries[i].element_count,
                            test_entries[i].bytes_per_element);
      jjs_value_free (ctx (), typedarray);
    }
  }

  jjs_value_free (ctx (), global_obj_val);
} /* test_typedarray_queries */

/**
 * Test value at given position in the buffer based on TypedArray type.
 */
static void
test_buffer_value (uint64_t value, /**< value to test for */
                   const void *buffer, /**< buffer to read value from */
                   uint32_t start_offset, /**< start offset of the value */
                   jjs_typedarray_type_t typedarray_type, /**< type of TypedArray */
                   uint32_t bytes_per_element) /**< bytes per element for the given type */
{
  uint32_t offset = start_offset / bytes_per_element;

#define TEST_VALUE_AT(TYPE, BUFFER, OFFSET, VALUE) TEST_ASSERT (((TYPE *) BUFFER)[OFFSET] == (TYPE) (VALUE))

  switch (typedarray_type)
  {
    case JJS_TYPEDARRAY_UINT8:
      TEST_VALUE_AT (uint8_t, buffer, offset, value);
      break;
    case JJS_TYPEDARRAY_INT8:
      TEST_VALUE_AT (int8_t, buffer, offset, value);
      break;
    case JJS_TYPEDARRAY_UINT16:
      TEST_VALUE_AT (uint16_t, buffer, offset, value);
      break;
    case JJS_TYPEDARRAY_INT16:
      TEST_VALUE_AT (int16_t, buffer, offset, value);
      break;
    case JJS_TYPEDARRAY_UINT32:
      TEST_VALUE_AT (uint32_t, buffer, offset, value);
      break;
    case JJS_TYPEDARRAY_INT32:
      TEST_VALUE_AT (int32_t, buffer, offset, value);
      break;
    case JJS_TYPEDARRAY_FLOAT32:
      TEST_VALUE_AT (float, buffer, offset, value);
      break;
    case JJS_TYPEDARRAY_FLOAT64:
      TEST_VALUE_AT (double, buffer, offset, value);
      break;
    case JJS_TYPEDARRAY_BIGINT64:
      TEST_VALUE_AT (int64_t, buffer, offset, value);
      break;
    case JJS_TYPEDARRAY_BIGUINT64:
      TEST_VALUE_AT (uint64_t, buffer, offset, value);
      break;

    case JJS_TYPEDARRAY_UINT8CLAMPED:
    {
      int64_t signed_value = (int64_t) value;
      uint8_t expected = (uint8_t) value;

      /* clamp the value if required*/
      if (signed_value > 0xFF)
      {
        expected = 0xFF;
      }
      else if (signed_value < 0)
      {
        expected = 0;
      }

      TEST_VALUE_AT (uint8_t, buffer, offset, expected);
      break;
    }
    default:
      TEST_ASSERT (false);
      break;
  }

#undef TEST_VALUE_AT
} /* test_buffer_value */

static void
test_typedarray_complex_creation (test_entry_t test_entries[], /**< test cases */
                                  bool use_external_buffer) /**< run tests using arraybuffer with external memory */
{
  const uint32_t arraybuffer_size = 256;

  for (uint32_t i = 0; test_entries[i].constructor_name != NULL; i++)
  {
    const uint32_t offset = 8;
    uint32_t element_count = test_entries[i].element_count;
    uint32_t bytes_per_element = test_entries[i].bytes_per_element;
    uint8_t *buffer_p = NULL;

    /* new %TypedArray% (buffer, offset, length); */
    jjs_value_t typedarray;
    {
      jjs_value_t arraybuffer;

      if (use_external_buffer)
      {
        buffer_p = (uint8_t *) jjs_heap_alloc (ctx (), arraybuffer_size);
        arraybuffer = jjs_arraybuffer_external (ctx (), buffer_p, arraybuffer_size, NULL);
      }
      else
      {
        arraybuffer = jjs_arraybuffer (ctx (), arraybuffer_size);
      }

      jjs_value_t js_offset = jjs_number (ctx (), offset);
      jjs_value_t js_element_count = jjs_number (ctx (), element_count);

      register_js_value ("expected_offset", js_offset);
      register_js_value ("expected_length", js_element_count);

      typedarray =
        jjs_typedarray_with_buffer_span (ctx (), test_entries[i].typedarray_type, arraybuffer, offset, element_count);
      TEST_ASSERT (!jjs_value_is_exception (ctx (), typedarray));

      jjs_value_free (ctx (), js_offset);
      jjs_value_free (ctx (), js_element_count);
      jjs_value_free (ctx (), arraybuffer);
    }

    register_js_value ("array", typedarray);

    const jjs_char_t test_exptected_src[] =
      TEST_STRING_LITERAL ("assert (array.length == expected_length,"
                           "        'expected length: ' + expected_length + ' got: ' + array.length);"
                           "assert (array.byteOffset == expected_offset);");
    jjs_value_t result = jjs_eval (ctx (), test_exptected_src, sizeof (test_exptected_src) - 1, JJS_PARSE_STRICT_MODE);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
    jjs_value_free (ctx (), result);

    const jjs_char_t set_element_src[] = TEST_STRING_LITERAL ("array[0] = 0x11223344n");

    /* crop the last 'n' character */
    size_t src_length = sizeof (set_element_src) - 2;

    if (test_entries[i].typedarray_type >= JJS_TYPEDARRAY_BIGINT64)
    {
      /* use the last 'n' character */
      src_length++;
    }

    result = jjs_eval (ctx (), set_element_src, src_length, JJS_PARSE_STRICT_MODE);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
    jjs_value_free (ctx (), result);

    {
      jjs_length_t byte_length = 0;
      jjs_length_t byte_offset = 0;
      jjs_value_t buffer = jjs_typedarray_buffer (ctx (), typedarray, &byte_offset, &byte_length);
      TEST_ASSERT (byte_length == element_count * bytes_per_element);
      TEST_ASSERT (byte_offset == offset);

      JJS_VLA (uint8_t, test_buffer, arraybuffer_size);

      jjs_typedarray_type_t type = jjs_typedarray_type (ctx (), typedarray);
      jjs_value_t read_count = jjs_arraybuffer_read (ctx (), buffer, 0, test_buffer, offset + byte_length);
      TEST_ASSERT (read_count == offset + byte_length);
      test_buffer_value (0x11223344, test_buffer, offset, type, bytes_per_element);

      if (use_external_buffer)
      {
        test_buffer_value (0x11223344, buffer_p, offset, type, bytes_per_element);
        TEST_ASSERT (memcmp (buffer_p, test_buffer, offset + byte_length) == 0);
      }

      jjs_value_free (ctx (), buffer);
    }

    jjs_value_free (ctx (), typedarray);
  }
} /* test_typedarray_complex_creation */

/**
 * Test get/set/delete property by index.
 */
static void
test_property_by_index (test_entry_t test_entries[])
{
  int test_int_numbers[5] = { -5, -70, 13, 0, 56 };
  double test_double_numbers[5] = { -83.153, -35.15, 0, 13.1, 89.8975 };
  uint8_t test_uint_numbers[5] = { 83, 15, 36, 0, 43 };
  uint64_t test_uint64_numbers[5] = { 83, 0, 1, UINT32_MAX, UINT64_MAX };
  int64_t test_int64_numbers[5] = { INT64_MAX, INT64_MIN, 0, INT32_MAX, INT32_MIN };

  for (uint32_t i = 0; test_entries[i].constructor_name != NULL; i++)
  {
    jjs_value_t test_number;
    uint32_t test_numbers_length = sizeof (test_int_numbers) / sizeof (int);
    jjs_value_t typedarray = jjs_typedarray (ctx (), test_entries[i].typedarray_type, test_numbers_length);
    jjs_typedarray_type_t type = jjs_typedarray_type (ctx (), typedarray);

    jjs_value_t set_result;
    jjs_value_t get_result;

    switch (type)
    {
      case JJS_TYPEDARRAY_INT8:
      case JJS_TYPEDARRAY_INT16:
      case JJS_TYPEDARRAY_INT32:
      {
        for (uint8_t j = 0; j < test_numbers_length; j++)
        {
          test_number = jjs_number (ctx (), test_int_numbers[j]);
          TEST_ASSERT (jjs_value_is_false (ctx (), jjs_object_delete_index (ctx (), typedarray, j)));
          set_result = jjs_object_set_index (ctx (), typedarray, j, test_number);
          get_result = jjs_object_get_index (ctx (), typedarray, j);

          TEST_ASSERT (jjs_value_is_boolean (ctx (), set_result));
          TEST_ASSERT (jjs_value_is_true (ctx (), set_result));
          TEST_ASSERT (jjs_value_is_false (ctx (), jjs_object_delete_index (ctx (), typedarray, j)));
          TEST_ASSERT (jjs_value_as_number (ctx (), get_result) == test_int_numbers[j]);

          jjs_value_free (ctx (), test_number);
          jjs_value_free (ctx (), set_result);
          jjs_value_free (ctx (), get_result);
        }
        break;
      }
      case JJS_TYPEDARRAY_FLOAT32:
      case JJS_TYPEDARRAY_FLOAT64:
      {
        for (uint8_t j = 0; j < test_numbers_length; j++)
        {
          test_number = jjs_number (ctx (), test_double_numbers[j]);
          TEST_ASSERT (jjs_value_is_false (ctx (), jjs_object_delete_index (ctx (), typedarray, j)));
          set_result = jjs_object_set_index (ctx (), typedarray, j, test_number);
          get_result = jjs_object_get_index (ctx (), typedarray, j);

          TEST_ASSERT (jjs_value_is_boolean (ctx (), set_result));
          TEST_ASSERT (jjs_value_is_true (ctx (), set_result));
          TEST_ASSERT (jjs_value_is_false (ctx (), jjs_object_delete_index (ctx (), typedarray, j)));

          double epsilon = pow (10, -5);
          double get_abs = fabs (jjs_value_as_number (ctx (), get_result) - test_double_numbers[j]);
          TEST_ASSERT (get_abs < epsilon);

          jjs_value_free (ctx (), test_number);
          jjs_value_free (ctx (), set_result);
          jjs_value_free (ctx (), get_result);

          /* Testing positive and negative infinity */
          for (uint8_t k = 0; k < 2; k++)
          {
            jjs_value_t inf = jjs_infinity (ctx (), k);
            jjs_value_t set_inf = jjs_object_set_index (ctx (), typedarray, 0, inf);
            TEST_ASSERT (jjs_value_is_boolean (ctx (), set_inf));
            TEST_ASSERT (jjs_value_is_true (ctx (), set_inf));
            jjs_value_t get_inf = jjs_object_get_index (ctx (), typedarray, 0);
            TEST_ASSERT (isinf (jjs_value_as_number (ctx (), get_inf)));

            jjs_value_free (ctx (), inf);
            jjs_value_free (ctx (), set_inf);
            jjs_value_free (ctx (), get_inf);
          }
        }
        break;
      }
      case JJS_TYPEDARRAY_BIGINT64:
      {
        for (uint8_t j = 0; j < test_numbers_length; j++)
        {
          test_number = jjs_bigint (ctx (), (uint64_t *) &test_int64_numbers[j], 1, true);
          TEST_ASSERT (jjs_value_is_false (ctx (), jjs_object_delete_index (ctx (), typedarray, j)));
          set_result = jjs_object_set_index (ctx (), typedarray, j, test_number);
          get_result = jjs_object_get_index (ctx (), typedarray, j);

          TEST_ASSERT (jjs_value_is_boolean (ctx (), set_result));
          TEST_ASSERT (jjs_value_is_true (ctx (), set_result));
          TEST_ASSERT (jjs_value_is_false (ctx (), jjs_object_delete_index (ctx (), typedarray, j)));
          int64_t get_number;
          bool sign = jjs_bigint_sign (ctx (), get_result);
          jjs_bigint_to_digits (ctx (), get_result, (uint64_t *) &get_number, 1);

          TEST_ASSERT (sign ? get_number : -get_number == test_int64_numbers[j]);

          jjs_value_free (ctx (), test_number);
          jjs_value_free (ctx (), set_result);
          jjs_value_free (ctx (), get_result);
        }
        break;
      }
      case JJS_TYPEDARRAY_BIGUINT64:
      {
        for (uint32_t j = 0; j < test_numbers_length; j++)
        {
          test_number = jjs_bigint (ctx (), &test_uint64_numbers[j], 1, false);
          TEST_ASSERT (jjs_value_is_false (ctx (), jjs_object_delete_index (ctx (), typedarray, j)));
          set_result = jjs_object_set_index (ctx (), typedarray, j, test_number);
          get_result = jjs_object_get_index (ctx (), typedarray, j);

          TEST_ASSERT (jjs_value_is_boolean (ctx (), set_result));
          TEST_ASSERT (jjs_value_is_true (ctx (), set_result));
          TEST_ASSERT (jjs_value_is_false (ctx (), jjs_object_delete_index (ctx (), typedarray, j)));
          uint64_t get_number;

          jjs_bigint_to_digits (ctx (), get_result, &get_number, 1);

          TEST_ASSERT (get_number == test_uint64_numbers[j]);

          jjs_value_free (ctx (), test_number);
          jjs_value_free (ctx (), set_result);
          jjs_value_free (ctx (), get_result);
        }
        break;
      }
      default:
      {
        for (uint8_t j = 0; j < test_numbers_length; j++)
        {
          test_number = jjs_number (ctx (), test_uint_numbers[j]);
          TEST_ASSERT (jjs_value_is_false (ctx (), jjs_object_delete_index (ctx (), typedarray, j)));
          set_result = jjs_object_set_index (ctx (), typedarray, j, test_number);
          get_result = jjs_object_get_index (ctx (), typedarray, j);

          TEST_ASSERT (jjs_value_is_boolean (ctx (), set_result));
          TEST_ASSERT (jjs_value_is_true (ctx (), set_result));
          TEST_ASSERT (jjs_value_is_false (ctx (), jjs_object_delete_index (ctx (), typedarray, j)));
          TEST_ASSERT (jjs_value_as_number (ctx (), get_result) == test_uint_numbers[j]);

          jjs_value_free (ctx (), test_number);
          jjs_value_free (ctx (), set_result);
          jjs_value_free (ctx (), get_result);
        }
        break;
      }
    }

    jjs_value_t set_undefined = jjs_object_set_index (ctx (), typedarray, 100, jjs_number (ctx (), 50));

    if (type == JJS_TYPEDARRAY_BIGINT64 || type == JJS_TYPEDARRAY_BIGUINT64)
    {
      TEST_ASSERT (jjs_value_is_exception (ctx (), set_undefined));
    }
    else
    {
      TEST_ASSERT (jjs_value_is_boolean (ctx (), set_undefined) && !jjs_value_is_true (ctx (), set_undefined));
    }

    jjs_value_t get_undefined = jjs_object_get_index (ctx (), typedarray, 100);

    if (type == JJS_TYPEDARRAY_BIGINT64 || type == JJS_TYPEDARRAY_BIGUINT64)
    {
      TEST_ASSERT (jjs_value_is_exception (ctx (), set_undefined));
    }
    else
    {
      TEST_ASSERT (jjs_value_is_undefined (ctx (), get_undefined));
    }

    TEST_ASSERT (jjs_value_is_undefined (ctx (), get_undefined));
    jjs_value_free (ctx (), set_undefined);
    jjs_value_free (ctx (), get_undefined);
    jjs_value_free (ctx (), typedarray);
  }
} /* test_property_by_index */

static void
test_detached_arraybuffer (void)
{
  static jjs_typedarray_type_t types[] = {
    JJS_TYPEDARRAY_UINT8,   JJS_TYPEDARRAY_UINT8CLAMPED, JJS_TYPEDARRAY_INT8,      JJS_TYPEDARRAY_UINT16,
    JJS_TYPEDARRAY_INT16,   JJS_TYPEDARRAY_UINT32,       JJS_TYPEDARRAY_INT32,     JJS_TYPEDARRAY_FLOAT32,
    JJS_TYPEDARRAY_FLOAT64, JJS_TYPEDARRAY_BIGINT64,     JJS_TYPEDARRAY_BIGUINT64,
  };

  /* Creating an TypedArray for a detached array buffer with a given length/offset is invalid */
  {
    const uint32_t length = 1;
    uint8_t *buffer_p = (uint8_t *) jjs_heap_alloc (ctx (), length);
    jjs_value_t arraybuffer = jjs_arraybuffer_external (ctx (), buffer_p, length, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), arraybuffer));
    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == length);

    TEST_ASSERT (jjs_arraybuffer_is_detachable (ctx (), arraybuffer));

    jjs_value_t res = jjs_arraybuffer_detach (ctx (), arraybuffer);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), res));
    jjs_value_free (ctx (), res);

    TEST_ASSERT (!jjs_arraybuffer_is_detachable (ctx (), arraybuffer));

    for (size_t idx = 0; idx < (sizeof (types) / sizeof (types[0])); idx++)
    {
      jjs_value_t typedarray = jjs_typedarray_with_buffer_span (ctx (), types[idx], arraybuffer, 0, 4);
      TEST_ASSERT (jjs_value_is_exception (ctx (), typedarray));
      TEST_ASSERT (jjs_error_type (ctx (), typedarray) == JJS_ERROR_TYPE);
      jjs_value_free (ctx (), typedarray);
    }

    jjs_value_free (ctx (), arraybuffer);
  }

  /* Creating an TypedArray for a detached array buffer without length/offset is valid */
  {
    const uint32_t length = 1;
    uint8_t *buffer_p = (uint8_t *) jjs_heap_alloc (ctx (), length);
    jjs_value_t arraybuffer = jjs_arraybuffer_external (ctx (), buffer_p, length, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), arraybuffer));
    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == length);

    TEST_ASSERT (jjs_arraybuffer_is_detachable (ctx (), arraybuffer));

    jjs_value_t res = jjs_arraybuffer_detach (ctx (), arraybuffer);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), res));
    jjs_value_free (ctx (), res);

    TEST_ASSERT (!jjs_arraybuffer_is_detachable (ctx (), arraybuffer));

    for (size_t idx = 0; idx < (sizeof (types) / sizeof (types[0])); idx++)
    {
      jjs_value_t typedarray = jjs_typedarray_with_buffer (ctx (), types[idx], arraybuffer);
      TEST_ASSERT (jjs_value_is_exception (ctx (), typedarray));
      TEST_ASSERT (jjs_error_type (ctx (), typedarray) == JJS_ERROR_TYPE);
      jjs_value_free (ctx (), typedarray);
    }

    jjs_value_free (ctx (), arraybuffer);
  }
} /* test_detached_arraybuffer */

int
main (void)
{
  ctx_open (NULL);

  if (!jjs_feature_enabled (JJS_FEATURE_TYPEDARRAY))
  {
    jjs_log (ctx (), JJS_LOG_LEVEL_ERROR, "TypedArray is disabled!\n");
    ctx_close ();
    return 0;
  }

  jjs_value_t function_val = jjs_function_external (ctx (), assert_handler);
  register_js_value ("assert", function_val);
  jjs_value_free (ctx (), function_val);

  test_entry_t test_entries[] = {
#define TEST_ENTRY(TYPE, CONSTRUCTOR, COUNT, BYTES_PER_ELEMENT) { TYPE, CONSTRUCTOR, COUNT, BYTES_PER_ELEMENT }

    TEST_ENTRY (JJS_TYPEDARRAY_UINT8, "Uint8Array", 12, 1),
    TEST_ENTRY (JJS_TYPEDARRAY_UINT8CLAMPED, "Uint8ClampedArray", 12, 1),
    TEST_ENTRY (JJS_TYPEDARRAY_INT8, "Int8Array", 12, 1),
    TEST_ENTRY (JJS_TYPEDARRAY_UINT16, "Uint16Array", 12, 2),
    TEST_ENTRY (JJS_TYPEDARRAY_INT16, "Int16Array", 12, 2),
    TEST_ENTRY (JJS_TYPEDARRAY_UINT16, "Uint16Array", 12, 2),
    TEST_ENTRY (JJS_TYPEDARRAY_INT32, "Int32Array", 12, 4),
    TEST_ENTRY (JJS_TYPEDARRAY_UINT32, "Uint32Array", 12, 4),
    TEST_ENTRY (JJS_TYPEDARRAY_FLOAT32, "Float32Array", 12, 4),
    /* TODO: add check if the float64 is supported */
    TEST_ENTRY (JJS_TYPEDARRAY_FLOAT64, "Float64Array", 12, 8),
    TEST_ENTRY (JJS_TYPEDARRAY_BIGINT64, "BigInt64Array", 12, 8),
    TEST_ENTRY (JJS_TYPEDARRAY_BIGUINT64, "BigUint64Array", 12, 8),

    TEST_ENTRY (JJS_TYPEDARRAY_INVALID, NULL, 0, 0)
#undef TEST_ENTRY
  };

  /* Test TypedArray queries */
  test_typedarray_queries (test_entries);

  /* Test TypedArray operations in js */
  {
    const uint32_t element_count = 14;

    jjs_value_t array = jjs_typedarray (ctx (), JJS_TYPEDARRAY_UINT8, element_count);

    {
      uint8_t expected_value = 42;
      JJS_VLA (uint8_t, expected_data, element_count);
      memset (expected_data, expected_value, element_count);

      jjs_length_t byte_length;
      jjs_length_t offset;
      jjs_value_t buffer = jjs_typedarray_buffer (ctx (), array, &offset, &byte_length);
      TEST_ASSERT (byte_length == element_count);
      jjs_length_t written = jjs_arraybuffer_write (ctx (), buffer, offset, expected_data, element_count);
      TEST_ASSERT (written == element_count);
      jjs_value_free (ctx (), buffer);

      jjs_value_t js_element_count = jjs_number (ctx (), element_count);
      jjs_value_t js_expected_value = jjs_number (ctx (), expected_value);

      register_js_value ("array", array);
      register_js_value ("expected_length", js_element_count);
      register_js_value ("expected_value", js_expected_value);

      jjs_value_free (ctx (), js_element_count);
      jjs_value_free (ctx (), js_expected_value);
    }

    /* Check read and to write */
    const jjs_char_t eval_src[] = TEST_STRING_LITERAL (
      "assert (array.length == expected_length, 'expected length: ' + expected_length + ' got: ' + array.length);"
      "for (var i = 0; i < array.length; i++)"
      "{"
      "  assert (array[i] == expected_value);"
      "  array[i] = i;"
      "};");
    jjs_value_t result = jjs_eval (ctx (), eval_src, sizeof (eval_src) - 1, JJS_PARSE_STRICT_MODE);

    TEST_ASSERT (!jjs_value_is_exception (ctx (), result));
    jjs_value_free (ctx (), result);

    /* Check write results */
    {
      jjs_length_t byte_length;
      jjs_length_t offset;
      jjs_value_t buffer = jjs_typedarray_buffer (ctx (), array, &offset, &byte_length);
      TEST_ASSERT (byte_length == element_count);
      TEST_ASSERT (byte_length == jjs_typedarray_length (ctx (), array));
      TEST_ASSERT (offset == jjs_typedarray_offset (ctx (), array));

      JJS_VLA (uint8_t, result_data, element_count);

      jjs_length_t read_count = jjs_arraybuffer_read (ctx (), buffer, offset, result_data, byte_length);
      TEST_ASSERT (read_count == byte_length);

      for (uint32_t i = 0; i < read_count; i++)
      {
        TEST_ASSERT (result_data[i] == i);
      }

      jjs_value_free (ctx (), buffer);
    }

    jjs_value_free (ctx (), array);
  }

  test_typedarray_complex_creation (test_entries, false);
  test_typedarray_complex_creation (test_entries, true);

  test_property_by_index (test_entries);

  /* test invalid things */
  {
    jjs_value_t values[] = {
      jjs_number (ctx (), 11),
      jjs_boolean (ctx (), false),
      jjs_string_sz (ctx (), "test"),
      jjs_object (ctx ()),
      jjs_null (ctx ()),
      jjs_arraybuffer (ctx (), 16),
      jjs_error_sz (ctx (), JJS_ERROR_TYPE, "error", jjs_undefined (ctx ())),
      jjs_undefined (ctx ()),
      jjs_promise (ctx ()),
    };

    for (size_t idx = 0; idx < sizeof (values) / sizeof (values[0]); idx++)
    {
      /* A non-TypedArray object should not be regarded a TypedArray. */
      bool is_typedarray = jjs_value_is_typedarray (ctx (), values[idx]);
      TEST_ASSERT (is_typedarray == false);

      /* JJS_TYPEDARRAY_INVALID should be returned for non-TypedArray objects */
      jjs_typedarray_type_t type = jjs_typedarray_type (ctx (), values[idx]);
      TEST_ASSERT (type == JJS_TYPEDARRAY_INVALID);

      /* Zero should be returned for non-TypedArray objects */
      jjs_length_t length = jjs_typedarray_length (ctx (), values[idx]);
      TEST_ASSERT (length == 0);

      /**
       * Getting the ArrayBuffer from a non-TypedArray object(s) should return an error
       * and should not modify the output parameter values.
       */
      {
        jjs_length_t offset = 22;
        jjs_length_t byte_count = 23;
        jjs_value_t error = jjs_typedarray_buffer (ctx (), values[idx], &offset, &byte_count);
        TEST_ASSERT (jjs_value_is_exception (ctx (), error));
        TEST_ASSERT (offset == 22);
        TEST_ASSERT (byte_count == 23);
        jjs_value_free (ctx (), error);
      }

      /**
       * Creating a TypedArray from a non-ArrayBuffer should result an error.
       */
      if (!jjs_value_is_arraybuffer (ctx (), values[idx]))
      {
        jjs_value_t error = jjs_typedarray_with_buffer (ctx (), JJS_TYPEDARRAY_UINT8, values[idx]);
        TEST_ASSERT (jjs_value_is_exception (ctx (), error));
        jjs_value_free (ctx (), error);
      }

      jjs_value_free (ctx (), values[idx]);
    }
  }

  test_detached_arraybuffer ();

  ctx_close ();

  return 0;
} /* main */
