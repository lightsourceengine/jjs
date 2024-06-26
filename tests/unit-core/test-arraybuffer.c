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

/**
 * Register a JavaScript value in the global object.
 */
static void
register_js_value (const char *name_p, /**< name of the function */
                   jjs_value_t value) /**< JS value */
{
  jjs_value_t global_obj_val = jjs_current_realm (ctx ());

  jjs_value_t result_val = jjs_object_set_sz (ctx (), global_obj_val, name_p, value, JJS_KEEP);
  TEST_ASSERT (jjs_value_is_boolean (ctx (), result_val));

  jjs_value_free (ctx (), global_obj_val);

  jjs_value_free (ctx (), result_val);
} /* register_js_value */

static jjs_value_t
assert_handler (const jjs_call_info_t *call_info_p, /**< call information */
                const jjs_value_t args_p[], /**< function arguments */
                const jjs_length_t args_cnt) /**< number of function arguments */
{
  JJS_UNUSED (call_info_p);

  if (args_cnt > 0 && jjs_value_is_true (ctx (), args_p[0]))
  {
    return jjs_boolean (ctx (), true);
  }

  if (args_cnt > 1 && jjs_value_is_string (ctx (), args_p[1]))
  {
    jjs_length_t utf8_sz = jjs_string_size (ctx (), args_p[1], JJS_ENCODING_CESU8);
    TEST_ASSERT (utf8_sz <= 127); /* 127 is the expected max assert fail message size. */
    JJS_VLA (char, string_from_utf8, utf8_sz + 1);
    string_from_utf8[utf8_sz] = 0;

    jjs_string_to_buffer (ctx (), args_p[1], JJS_ENCODING_CESU8, (jjs_char_t *) string_from_utf8, utf8_sz);

    printf ("JS assert: %s\n", string_from_utf8);
  }

  TEST_ASSERT (false);

  return jjs_undefined (ctx ());
} /* assert_handler */

/**
 * Test ArrayBuffer 'read' api call with various offset values.
 */
static void
test_read_with_offset (uint8_t offset) /**< offset for buffer read. */
{
  const jjs_char_t eval_arraybuffer_src[] =
    TEST_STRING_LITERAL ("var array = new Uint8Array (15);"
                         "for (var i = 0; i < array.length; i++) { array[i] = i * 2; };"
                         "array.buffer");
  jjs_value_t arraybuffer =
    jjs_eval (ctx (), eval_arraybuffer_src, sizeof (eval_arraybuffer_src) - 1, JJS_PARSE_STRICT_MODE);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), arraybuffer));
  TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));
  TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == 15);

  uint8_t buffer[20];
  memset (buffer, 120, 20);

  /* Try to copy more than the target buffer size. */
  jjs_length_t copied = jjs_arraybuffer_read (ctx (), arraybuffer, offset, buffer, 20);
  TEST_ASSERT (copied == (jjs_length_t) (15 - offset));

  for (uint8_t i = 0; i < copied; i++)
  {
    TEST_ASSERT (buffer[i] == (i + offset) * 2);
  }
  TEST_ASSERT (buffer[15 - offset] == 120);

  jjs_value_free (ctx (), arraybuffer);
} /* test_read_with_offset */

/**
 * Test ArrayBuffer 'write' api call with various offset values.
 */
static void
test_write_with_offset (uint8_t offset) /**< offset for buffer write. */
{
  {
    jjs_value_t offset_val = jjs_number (ctx (), offset);
    register_js_value ("offset", offset_val);
    jjs_value_free (ctx (), offset_val);
  }

  const jjs_char_t eval_arraybuffer_src[] = "var array = new Uint8Array (15); array.buffer";
  jjs_value_t arraybuffer =
    jjs_eval (ctx (), eval_arraybuffer_src, sizeof (eval_arraybuffer_src) - 1, JJS_PARSE_STRICT_MODE);

  TEST_ASSERT (!jjs_value_is_exception (ctx (), arraybuffer));
  TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));
  TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == 15);

  uint8_t buffer[20];

  for (uint8_t i = 0; i < 20; i++)
  {
    buffer[i] = (uint8_t) (i * 3);
  }

  /* Intentionally copy more than the allowed space. */
  jjs_length_t copied = jjs_arraybuffer_write (ctx (), arraybuffer, offset, buffer, 20);
  TEST_ASSERT (copied == (jjs_length_t) (15 - offset));

  const jjs_char_t eval_test_arraybuffer[] = TEST_STRING_LITERAL (
    "for (var i = 0; i < offset; i++)"
    "{"
    "  assert (array[i] == 0, 'offset check for: ' + i + ' was: ' + array[i] + ' should be: 0');"
    "};"
    "for (var i = offset; i < array.length; i++)"
    "{"
    "  var expected = (i - offset) * 3;"
    "  assert (array[i] == expected, 'calc check for: ' + i + ' was: ' + array[i] + ' should be: ' + expected);"
    "};"
    "assert (array[15] === undefined, 'ArrayBuffer out of bounds index should return undefined value');");
  jjs_value_t res = jjs_eval (ctx (), eval_test_arraybuffer, sizeof (eval_test_arraybuffer) - 1, JJS_PARSE_STRICT_MODE);
  jjs_value_free (ctx (), res);
  jjs_value_free (ctx (), arraybuffer);
} /* test_write_with_offset */

static int allocate_mode = 0;
static int allocate_count = 0;
static int free_count = 0;

static uint8_t *
test_allocate_cb (jjs_context_t *context_p, /**< context */
                  jjs_arraybuffer_type_t buffer_type, /**< type of the array buffer object */
                  uint32_t buffer_size, /**< size of the requested buffer */
                  void **buffer_user_p, /**< [in/out] user pointer assigned to the array buffer object */
                  void *user_p) /**< user pointer passed to jjs_arraybuffer_set_allocation_callbacks */
{
  JJS_UNUSED (context_p);
  TEST_ASSERT (buffer_type == JJS_ARRAYBUFFER_TYPE_ARRAYBUFFER);
  TEST_ASSERT (user_p == (void *) &allocate_mode);

  if (*buffer_user_p != NULL)
  {
    TEST_ASSERT (*buffer_user_p == (void *) &allocate_count);
    TEST_ASSERT (buffer_size == 20);
    allocate_count++;
    *buffer_user_p = (void *) &free_count;
  }
  else
  {
    *buffer_user_p = (void *) &allocate_mode;
  }
  return (uint8_t *) malloc (buffer_size);
} /* test_allocate_cb */

static void
test_free_cb (jjs_context_t *context_p, /**< context */
              jjs_arraybuffer_type_t buffer_type, /**< type of the array buffer object */
              uint8_t *buffer_p, /**< pointer to the allocated buffer */
              uint32_t buffer_size, /**< size of the allocated buffer */
              void *buffer_user_p, /**< user pointer assigned to the array buffer object */
              void *user_p) /**< user pointer passed to jjs_arraybuffer_set_allocation_callbacks */
{
  JJS_UNUSED (context_p);
  TEST_ASSERT (buffer_type == JJS_ARRAYBUFFER_TYPE_ARRAYBUFFER);
  TEST_ASSERT (user_p == (void *) &allocate_mode);

  if (buffer_user_p == NULL)
  {
    TEST_ASSERT (buffer_size == 15);
    free_count++;
  }
  else if (buffer_user_p == (void *) &free_count)
  {
    TEST_ASSERT (buffer_size == 20);
    free_count++;
  }
  else
  {
    TEST_ASSERT (buffer_user_p == (void *) &allocate_mode);
  }

  free (buffer_p);
} /* test_free_cb */

int
main (void)
{
  if (!jjs_feature_enabled (JJS_FEATURE_TYPEDARRAY))
  {
    jjs_log (ctx (), JJS_LOG_LEVEL_ERROR, "ArrayBuffer is disabled!\n");
    return 0;
  }

  ctx_open (NULL);

  jjs_arraybuffer_heap_allocation_limit (ctx (), 4);
  jjs_arraybuffer_allocator (ctx (), test_allocate_cb, test_free_cb, (void *) &allocate_mode);

  jjs_value_t function_val = jjs_function_external (ctx (), assert_handler);
  register_js_value ("assert", function_val);
  jjs_value_free (ctx (), function_val);

  /* Test array buffer queries */
  {
    const jjs_char_t eval_arraybuffer_src[] = "new ArrayBuffer (10)";
    jjs_value_t eval_arraybuffer =
      jjs_eval (ctx (), eval_arraybuffer_src, sizeof (eval_arraybuffer_src) - 1, JJS_PARSE_STRICT_MODE);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), eval_arraybuffer));
    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), eval_arraybuffer));
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), eval_arraybuffer) == 10);
    jjs_value_free (ctx (), eval_arraybuffer);
  }

  /* Test array buffer creation */
  {
    const uint32_t length = 15;
    jjs_value_t arraybuffer = jjs_arraybuffer (ctx (), length);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), arraybuffer));
    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == length);
    jjs_value_free (ctx (), arraybuffer);
  }

  /* Test array buffer read operations */
  for (uint8_t i = 0; i < 15; i++)
  {
    test_read_with_offset (i);
  }

  /* Test zero length ArrayBuffer read */
  {
    const uint32_t length = 0;
    jjs_value_t arraybuffer = jjs_arraybuffer (ctx (), length);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), arraybuffer));
    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == length);

    uint8_t data[20];
    memset (data, 11, 20);

    jjs_length_t bytes_read = jjs_arraybuffer_read (ctx (), arraybuffer, 0, data, 20);
    TEST_ASSERT (bytes_read == 0);

    for (int i = 0; i < 20; i++)
    {
      TEST_ASSERT (data[i] == 11);
    }

    jjs_value_free (ctx (), arraybuffer);
  }

  /* Test array buffer write operations */
  for (uint8_t i = 0; i < 15; i++)
  {
    test_write_with_offset (i);
  }

  /* Test zero length ArrayBuffer write */
  {
    const uint32_t length = 0;
    jjs_value_t arraybuffer = jjs_arraybuffer (ctx (), length);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), arraybuffer));
    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == length);

    uint8_t data[20];
    memset (data, 11, 20);

    jjs_length_t bytes_written = jjs_arraybuffer_write (ctx (), arraybuffer, 0, data, 20);
    TEST_ASSERT (bytes_written == 0);

    jjs_value_free (ctx (), arraybuffer);
  }

  /* Test zero length external ArrayBuffer */
  {
    const uint32_t length = 0;
    jjs_value_t arraybuffer = jjs_arraybuffer_external (ctx (), NULL, length, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), arraybuffer));
    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));
    TEST_ASSERT (jjs_arraybuffer_is_detachable (ctx (), arraybuffer));
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == length);

    uint8_t data[20];
    memset (data, 11, 20);

    jjs_length_t bytes_written = jjs_arraybuffer_write (ctx (), arraybuffer, 0, data, 20);
    TEST_ASSERT (bytes_written == 0);

    jjs_value_free (ctx (), arraybuffer);
  }

  /* Test ArrayBuffer with buffer allocated externally */
  {
    const uint32_t buffer_size = 15;
    const uint8_t base_value = 51;

    uint8_t *buffer_p = (uint8_t *) malloc (buffer_size);
    memset (buffer_p, base_value, buffer_size);

    jjs_value_t arrayb = jjs_arraybuffer_external (ctx (), buffer_p, buffer_size, NULL);
    uint8_t new_value = 123;
    jjs_length_t copied = jjs_arraybuffer_write (ctx (), arrayb, 0, &new_value, 1);
    TEST_ASSERT (copied == 1);
    TEST_ASSERT (buffer_p[0] == new_value);
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arrayb) == buffer_size);

    for (uint32_t i = 1; i < buffer_size; i++)
    {
      TEST_ASSERT (buffer_p[i] == base_value);
    }

    JJS_VLA (uint8_t, test_buffer, buffer_size);
    jjs_length_t read = jjs_arraybuffer_read (ctx (), arrayb, 0, test_buffer, buffer_size);
    TEST_ASSERT (read == buffer_size);
    TEST_ASSERT (test_buffer[0] == new_value);

    for (uint32_t i = 1; i < buffer_size; i++)
    {
      TEST_ASSERT (test_buffer[i] == base_value);
    }

    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arrayb));
    jjs_value_free (ctx (), arrayb);
  }

  /* Test ArrayBuffer external memory map/unmap */
  {
    const uint32_t buffer_size = 20;

    jjs_value_t input_buffer = jjs_arraybuffer_external (ctx (), NULL, buffer_size, (void *) &allocate_count);
    register_js_value ("input_buffer", input_buffer);
    jjs_value_free (ctx (), input_buffer);

    const jjs_char_t eval_arraybuffer_src[] = TEST_STRING_LITERAL ("var array = new Uint8Array(input_buffer);"
                                                                     "for (var i = 0; i < array.length; i++)"
                                                                     "{"
                                                                     "  array[i] = i * 2;"
                                                                     "};"
                                                                     "array.buffer");
    jjs_value_t buffer =
      jjs_eval (ctx (), eval_arraybuffer_src, sizeof (eval_arraybuffer_src) - 1, JJS_PARSE_STRICT_MODE);

    TEST_ASSERT (!jjs_value_is_exception (ctx (), buffer));
    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), buffer));
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), buffer) == 20);

    uint8_t *const data = jjs_arraybuffer_data (ctx (), buffer);

    TEST_ASSERT (data != NULL);

    /* test memory read */
    for (int i = 0; i < 20; i++)
    {
      TEST_ASSERT (data[i] == (uint8_t) (i * 2));
    }

    /* "upload" new data */
    double sum = 0;
    for (int i = 0; i < 20; i++)
    {
      data[i] = (uint8_t) (i * 3);
      sum += data[i];
    }

    const jjs_char_t eval_test_arraybuffer[] = TEST_STRING_LITERAL (
      "var sum = 0;"
      "for (var i = 0; i < array.length; i++)"
      "{"
      "  var expected = i * 3;"
      "  assert(array[i] == expected, 'Array at index ' + i + ' was: ' + array[i] + ' should be: ' + expected);"
      "  sum += array[i]"
      "};"
      "sum");
    jjs_value_t res = jjs_eval (ctx (), eval_test_arraybuffer, sizeof (eval_test_arraybuffer) - 1, JJS_PARSE_STRICT_MODE);
    TEST_ASSERT (jjs_value_is_number (ctx (), res));
    TEST_ASSERT (jjs_value_as_number (ctx (), res) == sum);
    jjs_value_free (ctx (), res);

    jjs_value_free (ctx (), buffer);
  }

  /* Test internal ArrayBuffer detach */
  {
    const uint32_t length = 4;
    jjs_value_t arraybuffer = jjs_arraybuffer (ctx (), length);
    TEST_ASSERT (jjs_arraybuffer_has_buffer (ctx (), arraybuffer));
    TEST_ASSERT (!jjs_value_is_exception (ctx (), arraybuffer));
    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == length);
    TEST_ASSERT (jjs_arraybuffer_is_detachable (ctx (), arraybuffer));

    jjs_value_t res = jjs_arraybuffer_detach (ctx (), arraybuffer);
    TEST_ASSERT (!jjs_arraybuffer_has_buffer (ctx (), arraybuffer));
    TEST_ASSERT (!jjs_value_is_exception (ctx (), res));
    TEST_ASSERT (jjs_arraybuffer_data (ctx (), arraybuffer) == NULL);
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == 0);
    TEST_ASSERT (!jjs_arraybuffer_is_detachable (ctx (), arraybuffer));

    jjs_value_free (ctx (), res);
    jjs_value_free (ctx (), arraybuffer);
  }

  /* Test external ArrayBuffer detach */
  {
    const uint32_t length = 64;
    jjs_value_t arraybuffer = jjs_arraybuffer_external (ctx (), NULL, length, NULL);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), arraybuffer));
    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == length);
    TEST_ASSERT (!jjs_arraybuffer_has_buffer (ctx (), arraybuffer));

    uint8_t buf[1] = { 1 };
    TEST_ASSERT (jjs_arraybuffer_write (ctx (), arraybuffer, 0, buf, 1) == 1);
    TEST_ASSERT (jjs_arraybuffer_has_buffer (ctx (), arraybuffer));
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == length);
    TEST_ASSERT (jjs_arraybuffer_is_detachable (ctx (), arraybuffer));

    jjs_value_t res = jjs_arraybuffer_detach (ctx (), arraybuffer);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), res));
    TEST_ASSERT (jjs_arraybuffer_data (ctx (), arraybuffer) == NULL);
    TEST_ASSERT (jjs_arraybuffer_size (ctx (), arraybuffer) == 0);
    TEST_ASSERT (!jjs_arraybuffer_has_buffer (ctx (), arraybuffer));
    TEST_ASSERT (!jjs_arraybuffer_is_detachable (ctx (), arraybuffer));

    jjs_value_free (ctx (), res);
    jjs_value_free (ctx (), arraybuffer);
  }

  /* Test ArrayBuffer created in ECMAScript */
  for (int i = 0; i < 3; i++)
  {
    const jjs_char_t source[] = TEST_STRING_LITERAL ("new ArrayBuffer(64)");
    jjs_value_t arraybuffer = jjs_eval (ctx (), source, sizeof (source) - 1, JJS_PARSE_NO_OPTS);
    TEST_ASSERT (!jjs_value_is_exception (ctx (), arraybuffer));
    TEST_ASSERT (jjs_value_is_arraybuffer (ctx (), arraybuffer));
    TEST_ASSERT (!jjs_arraybuffer_has_buffer (ctx (), arraybuffer));

    if (i == 0)
    {
      uint8_t buf[2] = { 2, 3 };
      TEST_ASSERT (jjs_arraybuffer_write (ctx (), arraybuffer, 63, buf, 2) == 1);
    }
    else if (i == 1)
    {
      uint8_t buf[2] = { 1, 1 };
      TEST_ASSERT (jjs_arraybuffer_read (ctx (), arraybuffer, 63, buf, 2) == 1);
      TEST_ASSERT (buf[0] == 0 && buf[1] == 1);
    }
    else
    {
      uint8_t *buffer_p = jjs_arraybuffer_data (ctx (), arraybuffer);
      TEST_ASSERT (buffer_p != NULL);
    }

    TEST_ASSERT (jjs_arraybuffer_has_buffer (ctx (), arraybuffer));

    jjs_value_free (ctx (), arraybuffer);
  }

  ctx_close ();

  TEST_ASSERT (allocate_count == 1);
  TEST_ASSERT (free_count == 2);

  return 0;
} /* main */
