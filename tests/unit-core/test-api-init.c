/* Copyright Light Source Software, LLC and other contributors.
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

static void 
check_default_options (const jjs_context_options_t* options)
{
  TEST_ASSERT (options->context_flags == 0);
  TEST_ASSERT (options->gc_limit_kb == JJS_DEFAULT_GC_LIMIT);
  TEST_ASSERT (options->gc_mark_limit == JJS_DEFAULT_GC_MARK_LIMIT);
  TEST_ASSERT (options->gc_new_objects_fraction == JJS_DEFAULT_GC_NEW_OBJECTS_FRACTION);
  TEST_ASSERT (options->vm_heap_size_kb == JJS_DEFAULT_VM_HEAP_SIZE);
  TEST_ASSERT (options->vm_stack_limit_kb == JJS_DEFAULT_VM_STACK_LIMIT);
  TEST_ASSERT (options->external_heap.buffer_p == NULL);
  TEST_ASSERT (options->external_heap.buffer_size_in_bytes == 0);
  TEST_ASSERT (options->external_heap.free_cb == NULL);
  TEST_ASSERT (options->external_heap.free_user_p == NULL);
}

static void
test_context_options (void)
{
  jjs_context_options_t options = jjs_context_options ();

  check_default_options (&options);
}

static void
test_context_options_init (void)
{
  jjs_context_options_t options;

  TEST_ASSERT (jjs_context_options_init (&options) == &options);
  check_default_options (&options);

}

static void
test_init_default (void)
{
  TEST_ASSERT (jjs_init_default() == JJS_CONTEXT_STATUS_OK);
  jjs_cleanup ();
}

static void
test_init_options_null (void)
{
  TEST_ASSERT (jjs_init (NULL) == JJS_CONTEXT_STATUS_OK);
  jjs_cleanup ();
}

static void
test_init_options_stack_limit (void)
{
#if !JJS_VM_STACK_STATIC
  jjs_context_options_t opts = jjs_context_options ();

  opts.vm_stack_limit_kb = 96;

  TEST_ASSERT (jjs_init (&opts) == JJS_CONTEXT_STATUS_OK);
  jjs_cleanup ();
#endif /* !JJS_VM_STACK_STATIC */
}

static void
test_init_options_stack_limit_when_stack_static (void)
{
#if JJS_VM_STACK_STATIC
  jjs_context_options_t opts = jjs_context_options ();

  opts.vm_stack_limit_kb += 10;

  TEST_ASSERT (jjs_init (&opts) == JJS_CONTEXT_STATUS_IMMUTABLE_STACK_LIMIT);
#endif /* JJS_VM_STACK_STATIC */
}

static jjs_external_heap_options_t external_heap_options = {0};

static void
external_heap_free (void* context_heap_p, void* user_p)
{
  TEST_ASSERT (context_heap_p == external_heap_options.buffer_p);
  TEST_ASSERT (user_p == external_heap_options.free_user_p);
}

static void
test_init_options_external_heap (void)
{
#if !JJS_VM_HEAP_STATIC
  static const uint32_t external_heap_size = 512 * 1024;
  jjs_context_options_t opts = jjs_context_options ();

  external_heap_options = (jjs_external_heap_options_t) {
    .buffer_p = malloc (external_heap_size),
    .buffer_size_in_bytes = external_heap_size,
    .free_cb = external_heap_free,
    .free_user_p = (void*) (uintptr_t) 1,
  };

  opts.context_flags = JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP;
  opts.external_heap = external_heap_options;

  TEST_ASSERT (jjs_init (&opts) == JJS_CONTEXT_STATUS_OK);

  jjs_cleanup();
  free (external_heap_options.buffer_p);
#endif /* !JJS_VM_HEAP_STATIC */
}

static void
test_init_options_external_heap_invalid (void)
{
#if !JJS_VM_HEAP_STATIC
  jjs_context_options_t opts = jjs_context_options ();

  external_heap_options = (jjs_external_heap_options_t) {
    .buffer_p = NULL,
    .buffer_size_in_bytes = 512 * 1024,
  };

  opts.context_flags = JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP;
  opts.external_heap = external_heap_options;

  TEST_ASSERT (jjs_init (&opts) == JJS_CONTEXT_STATUS_INVALID_EXTERNAL_HEAP);
#endif /* !JJS_VM_HEAP_STATIC */
}

static void
test_init_options_external_heap_when_heap_static (void)
{
#if JJS_VM_HEAP_STATIC
  static const uint32_t external_heap_size = 512 * 1024;
  jjs_context_options_t opts = jjs_context_options ();

  external_heap_options = (jjs_external_heap_options_t) {
    .buffer_p = malloc (external_heap_size),
    .buffer_size_in_bytes = external_heap_size,
  };

  opts.context_flags = JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP;
  opts.external_heap = external_heap_options;

  TEST_ASSERT (jjs_init (&opts) == JJS_CONTEXT_STATUS_IMMUTABLE_HEAP_SIZE);

  free (external_heap_options.buffer_p);
#endif /* JJS_VM_HEAP_STATIC */
}

int
main (void)
{
  test_context_options ();
  test_context_options_init ();

  test_init_default ();
  test_init_options_null ();
  test_init_options_external_heap ();
  test_init_options_external_heap_invalid ();
  test_init_options_external_heap_when_heap_static ();
  test_init_options_stack_limit ();
  test_init_options_stack_limit_when_stack_static ();

  return 0;
} /* main */
