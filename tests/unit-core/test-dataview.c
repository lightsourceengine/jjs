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

int
main (void)
{
  if (!jjs_feature_enabled (JJS_FEATURE_DATAVIEW))
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "DataView support is disabled!\n");
    return 0;
  }

  /* DataView builtin requires the TypedArray builtin */
  TEST_ASSERT (jjs_feature_enabled (JJS_FEATURE_TYPEDARRAY));

  TEST_CONTEXT_NEW (context_p);

  /* Test accessors */
  jjs_value_t arraybuffer = jjs_arraybuffer (16);
  jjs_value_t view1 = jjs_dataview (arraybuffer, 0, 16);
  TEST_ASSERT (!jjs_value_is_exception (view1));
  TEST_ASSERT (jjs_value_is_dataview (view1));

  jjs_length_t byteOffset = 0;
  jjs_length_t byteLength = 0;
  ;
  jjs_value_t internal_buffer = jjs_dataview_buffer (view1, &byteOffset, &byteLength);
  TEST_ASSERT (jjs_binary_op (JJS_BIN_OP_STRICT_EQUAL, internal_buffer, arraybuffer));
  TEST_ASSERT (byteOffset == 0);
  TEST_ASSERT (byteLength == 16);
  jjs_value_free (internal_buffer);

  jjs_value_t view2 = jjs_dataview (arraybuffer, 12, 4);
  TEST_ASSERT (!jjs_value_is_exception (view1));
  TEST_ASSERT (jjs_value_is_dataview (view2));
  internal_buffer = jjs_dataview_buffer (view2, &byteOffset, &byteLength);
  TEST_ASSERT (jjs_binary_op (JJS_BIN_OP_STRICT_EQUAL, internal_buffer, arraybuffer));
  TEST_ASSERT (byteOffset == 12);
  TEST_ASSERT (byteLength == 4);
  jjs_value_free (internal_buffer);

  /* Test invalid construction */
  jjs_value_t empty_object = jjs_object ();
  jjs_value_t view3 = jjs_dataview (empty_object, 20, 10);
  TEST_ASSERT (jjs_value_is_exception (view3));
  jjs_value_t error_obj = jjs_exception_value (view3, true);
  TEST_ASSERT (jjs_error_type (error_obj) == JJS_ERROR_TYPE);
  jjs_value_free (error_obj);
  jjs_value_free (empty_object);

  jjs_value_t view4 = jjs_dataview (arraybuffer, 20, 10);
  TEST_ASSERT (jjs_value_is_exception (view3));
  error_obj = jjs_exception_value (view4, true);
  TEST_ASSERT (jjs_error_type (error_obj) == JJS_ERROR_RANGE);
  jjs_value_free (error_obj);

  /* Test getting/setting values */
  jjs_value_t global_obj = jjs_current_realm ();
  jjs_value_t view1_str = jjs_string_sz ("view1");
  jjs_value_t view2_str = jjs_string_sz ("view2");
  TEST_ASSERT (jjs_object_set (global_obj, view1_str, view1));
  TEST_ASSERT (jjs_object_set (global_obj, view2_str, view2));

  jjs_value_free (view1_str);
  jjs_value_free (view2_str);
  jjs_value_free (global_obj);

  const jjs_char_t set_src[] = "view1.setInt16 (12, 255)";
  TEST_ASSERT (jjs_value_is_undefined (jjs_eval (set_src, sizeof (set_src) - 1, JJS_PARSE_NO_OPTS)));

  const jjs_char_t get_src[] = "view2.getInt16 (0)";
  TEST_ASSERT (jjs_value_as_number (jjs_eval (get_src, sizeof (get_src) - 1, JJS_PARSE_NO_OPTS)) == 255);

  const jjs_char_t get_src_little_endian[] = "view2.getInt16 (0, true)";
  TEST_ASSERT (
    jjs_value_as_number (jjs_eval (get_src_little_endian, sizeof (get_src_little_endian) - 1, JJS_PARSE_NO_OPTS))
    == -256);

  /* Cleanup */
  jjs_value_free (view2);
  jjs_value_free (view1);
  jjs_value_free (arraybuffer);

  TEST_CONTEXT_FREE (context_p);

  return 0;
} /* main */
