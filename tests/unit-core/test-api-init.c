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

#include "jjs-test.h"

static void
test_context_options_init (void)
{
  jjs_context_options_t options = {0};
  jjs_context_t *context_p;
  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
  jjs_context_free (context_p);

}

static void
test_init_options_null (void)
{
  jjs_context_t *context_p;
  TEST_ASSERT (jjs_context_new (NULL, &context_p) == JJS_STATUS_OK);
  jjs_context_free (context_p);
}

static void
test_init_options_stack_limit (void)
{
//  if (!jjs_feature_enabled(JJS_FEATURE_VM_STACK_STATIC))
//  {
//    jjs_context_options_t opts = jjs_context_options ();
//
//    opts.vm_stack_limit_kb = 96;
//
//    TEST_CONTEXT_NEW_OPTS (context_p, &opts);
//    TEST_CONTEXT_FREE (context_p);
//  }
}

static void
test_init_options_stack_limit_when_stack_static (void)
{
//  if (jjs_feature_enabled (JJS_FEATURE_VM_STACK_STATIC))
//  {
//    jjs_context_t *context_p = NULL;
//    jjs_context_options_t opts = jjs_context_options ();
//
//    opts.vm_stack_limit_kb += 10;
//
//    TEST_ASSERT (jjs_context_new (&opts, &context_p) == JJS_STATUS_CONTEXT_IMMUTABLE_STACK_LIMIT);
//  }
}

//static jjs_external_heap_options_t external_heap_options = {0};
//
//static void
//external_heap_free (void* context_heap_p, void* user_p)
//{
//  TEST_ASSERT (context_heap_p == external_heap_options.buffer_p);
//  TEST_ASSERT (user_p == external_heap_options.free_user_p);
//}

static void
test_init_options_external_heap (void)
{
//  if (!jjs_feature_enabled(JJS_FEATURE_VM_HEAP_STATIC))
//  {
//    static const uint32_t external_heap_size = 512 * 1024;
//    jjs_context_options_t opts = jjs_context_options ();
//
//    external_heap_options = (jjs_external_heap_options_t){
//      .buffer_p = malloc (external_heap_size),
//      .buffer_size_in_bytes = external_heap_size,
//      .free_cb = external_heap_free,
//      .free_user_p = (void*) (uintptr_t) 1,
//    };
//
//    opts.context_flags = JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP;
//    opts.external_heap = external_heap_options;
//
//    TEST_CONTEXT_NEW_OPTS (context_p, &opts);
//    TEST_CONTEXT_FREE (context_p);
//
//    free (external_heap_options.buffer_p);
//  }
}

static void
test_init_options_external_heap_invalid (void)
{
//  if (!jjs_feature_enabled(JJS_FEATURE_VM_HEAP_STATIC))
//  {
//    jjs_context_t *context_p = NULL;
//    jjs_context_options_t opts = jjs_context_options ();
//
//    external_heap_options = (jjs_external_heap_options_t){
//      .buffer_p = NULL,
//      .buffer_size_in_bytes = 512 * 1024,
//    };
//
//    opts.context_flags = JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP;
//    opts.external_heap = external_heap_options;
//
//    TEST_ASSERT (jjs_context_new (&opts, &context_p) == JJS_STATUS_CONTEXT_INVALID_EXTERNAL_HEAP);
//  }
}

static void
test_init_options_external_heap_when_heap_static (void)
{
//  if (jjs_feature_enabled(JJS_FEATURE_VM_HEAP_STATIC))
//  {
//    static const uint32_t external_heap_size = 512 * 1024;
//    jjs_context_t *context_p = NULL;
//    jjs_context_options_t opts = jjs_context_options ();
//
//    external_heap_options = (jjs_external_heap_options_t){
//      .buffer_p = malloc (external_heap_size),
//      .buffer_size_in_bytes = external_heap_size,
//    };
//
//    opts.context_flags = JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP;
//    opts.external_heap = external_heap_options;
//
//    TEST_ASSERT (jjs_context_new (&opts, &context_p) == JJS_STATUS_CONTEXT_INVALID_EXTERNAL_HEAP);
//
//    free (external_heap_options.buffer_p);
//  }
}

static bool unhandled_rejection_called = false;

static void unhandled_rejection (jjs_context_t *context_p, jjs_value_t promise, jjs_value_t reason, void *user_p)
{
  unhandled_rejection_called = true;

  TEST_ASSERT (jjs_value_is_promise (context_p, promise));
  TEST_ASSERT (jjs_value_is_error (context_p, reason));
  TEST_ASSERT (((uintptr_t) user_p) == 1);
}

static void
test_init_unhandled_rejection_handler (void)
{
  jjs_context_options_t options = {
    .unhandled_rejection_cb = unhandled_rejection,
    .unhandled_rejection_user_p = (void *) (uintptr_t) 1,
  };

  jjs_context_t *context_p = ctx_open (&options);

  jjs_esm_source_t source = {
    .source_sz = "import('blah')",
  };

  jjs_value_t result = jjs_esm_evaluate_source (context_p, &source, JJS_MOVE);
  TEST_ASSERT (!jjs_value_is_exception (context_p, result));
  jjs_value_free (ctx (), result);

  result = jjs_run_jobs (context_p);
  TEST_ASSERT (!jjs_value_is_exception (context_p, result));
  jjs_value_free (ctx (), result);

  TEST_ASSERT (unhandled_rejection_called);

  ctx_close ();
}

int
main (void)
{
  test_context_options_init ();

  test_init_options_null ();
  test_init_options_external_heap ();
  test_init_options_external_heap_invalid ();
  test_init_options_external_heap_when_heap_static ();
  test_init_options_stack_limit ();
  test_init_options_stack_limit_when_stack_static ();

  test_init_unhandled_rejection_handler ();

  return 0;
} /* main */
