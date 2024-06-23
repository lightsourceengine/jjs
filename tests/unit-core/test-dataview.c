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

int
main (void)
{
  if (!jjs_feature_enabled (JJS_FEATURE_DATAVIEW))
  {
    jjs_log (ctx (), JJS_LOG_LEVEL_ERROR, "DataView support is disabled!\n");
    return 0;
  }

  /* DataView builtin requires the TypedArray builtin */
  TEST_ASSERT (jjs_feature_enabled (JJS_FEATURE_TYPEDARRAY));

  ctx_open (NULL);

  /* Test accessors */
  jjs_value_t arraybuffer = jjs_arraybuffer (ctx (), 16);
  jjs_value_t view1 = jjs_dataview (ctx (), arraybuffer, JJS_KEEP, 0, 16);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), view1));
  TEST_ASSERT (jjs_value_is_dataview (ctx (), view1));

  jjs_length_t byteOffset;
  jjs_length_t byteLength;
  jjs_value_t internal_buffer = jjs_dataview_buffer (ctx (), view1, &byteOffset, &byteLength);

  TEST_ASSERT (jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, internal_buffer, JJS_MOVE, arraybuffer, JJS_KEEP));
  TEST_ASSERT (byteOffset == 0);
  TEST_ASSERT (byteOffset == jjs_dataview_byte_offset (ctx (), view1));
  TEST_ASSERT (byteLength == 16);
  TEST_ASSERT (byteLength == jjs_dataview_byte_length (ctx (), view1));

  jjs_value_t view2 = jjs_dataview (ctx (), arraybuffer, JJS_KEEP, 12, 4);
  TEST_ASSERT (!jjs_value_is_exception (ctx (), view1));
  TEST_ASSERT (jjs_value_is_dataview (ctx (), view2));
  internal_buffer = jjs_dataview_buffer (ctx (), view2, &byteOffset, &byteLength);

  TEST_ASSERT (jjs_binary_op (ctx (), JJS_BIN_OP_STRICT_EQUAL, internal_buffer, JJS_MOVE, arraybuffer, JJS_KEEP));
  TEST_ASSERT (byteOffset == 12);
  TEST_ASSERT (byteOffset == jjs_dataview_byte_offset (ctx (), view2));
  TEST_ASSERT (byteLength == 4);
  TEST_ASSERT (byteLength == jjs_dataview_byte_length (ctx (), view2));

  /* Test invalid construction */
  jjs_value_t view3 = jjs_dataview (ctx (), jjs_object (ctx ()), JJS_MOVE, 20, 10);
  TEST_ASSERT (jjs_value_is_exception (ctx (), view3));
  jjs_value_t error_obj = jjs_exception_value (ctx (), view3, true);
  TEST_ASSERT (jjs_error_type (ctx (), error_obj) == JJS_ERROR_TYPE);
  jjs_value_free (ctx (), error_obj);

  jjs_value_t view4 = jjs_dataview (ctx (), arraybuffer, JJS_KEEP, 20, 10);
  TEST_ASSERT (jjs_value_is_exception (ctx (), view3));
  error_obj = jjs_exception_value (ctx (), view4, true);
  TEST_ASSERT (jjs_error_type (ctx (), error_obj) == JJS_ERROR_RANGE);
  jjs_value_free (ctx (), error_obj);

  /* Test getting/setting values */
  jjs_value_t global_obj = jjs_current_realm (ctx ());
  TEST_ASSERT (jjs_object_set_sz (ctx (), global_obj, "view1", view1, JJS_KEEP));
  TEST_ASSERT (jjs_object_set_sz (ctx (), global_obj, "view2", view2, JJS_KEEP));

  jjs_value_free (ctx (), global_obj);

  const jjs_char_t set_src[] = "view1.setInt16 (12, 255)";
  TEST_ASSERT (jjs_value_is_undefined (ctx (), jjs_eval (ctx (), set_src, sizeof (set_src) - 1, JJS_PARSE_NO_OPTS)));

  const jjs_char_t get_src[] = "view2.getInt16 (0)";
  TEST_ASSERT (jjs_value_as_number (ctx (), jjs_eval (ctx (), get_src, sizeof (get_src) - 1, JJS_PARSE_NO_OPTS)) == 255);

  const jjs_char_t get_src_little_endian[] = "view2.getInt16 (0, true)";
  TEST_ASSERT (
    jjs_value_as_number (ctx (), jjs_eval (ctx (), get_src_little_endian, sizeof (get_src_little_endian) - 1, JJS_PARSE_NO_OPTS))
    == -256);

  /* Cleanup */
  jjs_value_free (ctx (), view2);
  jjs_value_free (ctx (), view1);
  jjs_value_free (ctx (), arraybuffer);

  ctx_close ();

  return 0;
} /* main */
