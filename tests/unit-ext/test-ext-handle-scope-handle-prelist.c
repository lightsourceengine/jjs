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
 * Unit test for jjs-ext/handle-scope-handle-prelist.
 *
 * Tests escaping JJS value that holds on scope's handle heap.
 */

#include "jjs.h"

#include "jjs-ext/handle-scope.h"
#include "test-common.h"

static size_t native_free_cb_call_count;
static const size_t handle_count = JJSX_HANDLE_PRELIST_SIZE * 2;

static void
native_free_cb (void *native_p, /**< native pointer */
                jjs_object_native_info_t *info_p) /**< native info */
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
  jjsx_escapable_handle_scope scope;
  jjsx_open_escapable_handle_scope (&scope);

  jjs_value_t obj;
  for (size_t idx = 0; idx < handle_count; idx++)
  {
    obj = jjsx_create_handle (jjs_object ());
    jjs_object_set_native_ptr (obj, &native_info, NULL);
  }

  // If leaves `escaped` uninitialized, there will be a style error on linux thrown by compiler
  jjs_value_t escaped = 0;
  jjsx_escape_handle (scope, obj, &escaped);
  TEST_ASSERT (scope->prelist_handle_count == JJSX_HANDLE_PRELIST_SIZE);

  jjsx_close_handle_scope (scope);
  return escaped;
} /* create_object */

static void
test_handle_scope_val (void)
{
  jjsx_handle_scope scope;
  jjsx_open_handle_scope (&scope);
  jjs_value_t obj = create_object ();
  (void) obj;

  jjs_heap_gc (JJS_GC_PRESSURE_LOW);
  TEST_ASSERT (native_free_cb_call_count == (handle_count - 1));

  jjsx_close_handle_scope (scope);
} /* test_handle_scope_val */

int
main (void)
{
  jjs_init (JJS_INIT_EMPTY);

  native_free_cb_call_count = 0;
  test_handle_scope_val ();

  jjs_heap_gc (JJS_GC_PRESSURE_LOW);
  TEST_ASSERT (native_free_cb_call_count == handle_count);

  jjs_cleanup ();
} /* main */
