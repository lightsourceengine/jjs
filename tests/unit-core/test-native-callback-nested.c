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

#include "jjs-port.h"
#include "jjs.h"

#include "test-common.h"

static void
native_cb2 (void)
{
  jjs_value_t array = jjs_array (100);
  jjs_value_free (array);
} /* native_cb2 */

static const jjs_object_native_info_t native_info2 = {
  .free_cb = (jjs_object_native_free_cb_t) native_cb2,
  .number_of_references = 0,
  .offset_of_references = 0,
};

static void
native_cb (void)
{
  jjs_value_t array = jjs_array (100);

  jjs_object_set_native_ptr (array, &native_info2, NULL);

  jjs_value_free (array);
} /* native_cb */

static const jjs_object_native_info_t native_info = {
  .free_cb = (jjs_object_native_free_cb_t) native_cb,
  .number_of_references = 0,
  .offset_of_references = 0,
};

int
main (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  jjs_value_t obj = jjs_object ();

  jjs_object_set_native_ptr (obj, &native_info, NULL);
  jjs_value_free (obj);

  jjs_cleanup ();
  return 0;
} /* main */
