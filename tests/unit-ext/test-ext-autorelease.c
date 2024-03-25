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

/**
 * Unit test for jjs-ext/autorelease.
 */

#include "jjs.h"

#include "jjs-ext/autorelease.h"
#include "test-common.h"

static int native_free_cb_call_count;

static void
native_free_cb (void *native_p, /**< native pointer */
                const jjs_object_native_info_t *info_p) /**< native info */
{
  (void) native_p;
  (void) info_p;
  ++native_free_cb_call_count;
} /* native_free_cb */

static const jjs_object_native_info_t native_info = {
  .free_cb = native_free_cb,
  .number_of_references = 0,
  .offset_of_references = 0,
};

static jjs_value_t
create_object (void)
{
  jjs_value_t obj = jjs_object ();
  jjs_object_set_native_ptr (obj, &native_info, NULL);
  return obj;
} /* create_object */

static void
test_autorelease_val (void)
{
  JJSX_AR_VALUE_T obj = create_object ();
  (void) obj;
} /* test_autorelease_val */

int
main (void)
{
  TEST_ASSERT (jjs_init_default () == JJS_CONTEXT_STATUS_OK);

  native_free_cb_call_count = 0;
  test_autorelease_val ();
  jjs_heap_gc (JJS_GC_PRESSURE_HIGH);
  TEST_ASSERT (native_free_cb_call_count == 1);

  jjs_cleanup ();
} /* main */
