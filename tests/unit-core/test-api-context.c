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
#include "config.h"

static void
test_context_new_default (void)
{
  jjs_context_options_t options = {0};
  jjs_context_t *context_p;
  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
  jjs_context_free (context_p);
}

static void
test_context_new_null (void)
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

static bool unhandled_rejection_called = false;

static void unhandled_rejection (jjs_context_t *context_p, jjs_value_t promise, jjs_value_t reason, void *user_p)
{
  unhandled_rejection_called = true;

  TEST_ASSERT (jjs_value_is_promise (context_p, promise));
  TEST_ASSERT (jjs_value_is_error (context_p, reason));
  TEST_ASSERT (((uintptr_t) user_p) == 1);
}

static void
test_context_unhandled_rejection_handler (void)
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

static void
test_context_jjs_namespace (void)
{
  ctx_open (NULL);
  
  jjs_value_t global = ctx_global ();
  jjs_value_t jjs = ctx_value (jjs_object_get_sz (ctx (), global, "jjs"));

  TEST_ASSERT (jjs_value_is_object (ctx (), jjs));
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "version"))));
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "os"))));
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "arch"))));
  TEST_ASSERT (jjs_value_is_object (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "stdout"))));
  TEST_ASSERT (jjs_value_is_object (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "stderr"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "pmap"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "vmod"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "readFile"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "realpath"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "cwd"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "gc"))));

  ctx_close ();
}

static void
check_jjs_namespace_exclusion (jjs_namespace_exclusion_t exclusion, const char* api_name_sz)
{
  jjs_context_options_t options = {
    .jjs_namespace_exclusions = exclusion,
  };

  ctx_open (&options);

  jjs_value_t global = ctx_global();
  jjs_value_t jjs = ctx_value (jjs_object_get_sz (ctx (), global, "jjs"));

  /* sanity check that jjs object exists and basics are populated */
  TEST_ASSERT (jjs_value_is_object (ctx (), jjs));
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "version"))));
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "os"))));
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_value (jjs_object_get_sz (ctx (), jjs, "arch"))));
  TEST_ASSERT (jjs_value_is_false (ctx (), ctx_value (jjs_object_has_sz (ctx (), jjs, api_name_sz))));

  ctx_close ();
}

static void
test_context_jjs_namespace_exclusions (void)
{
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_READ_FILE, "readFile");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_STDOUT, "stdout");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_STDERR, "stderr");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_PMAP, "pmap");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_VMOD, "vmod");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_REALPATH, "realpath");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_CWD, "cwd");
  check_jjs_namespace_exclusion (JJS_NAMESPACE_EXCLUSION_GC, "gc");
}

static bool stdlib_alloc_called = false;
static jjs_size_t stdlib_alloc_called_with = 0;
static bool stdlib_free_called = false;

static void*
stdlib_alloc (void *internal_p, uint32_t size)
{
  JJS_UNUSED (internal_p);
  TEST_ASSERT(stdlib_alloc_called == false);
  stdlib_alloc_called = true;
  stdlib_alloc_called_with = size;
  return malloc (size);
}

static void
stdlib_free (void *internal_p, void* p, uint32_t size)
{
  JJS_UNUSED (internal_p);
  JJS_UNUSED (size);
  TEST_ASSERT (stdlib_alloc_called == true);
  TEST_ASSERT (stdlib_free_called == false);
  stdlib_free_called = true;
  free (p);
}

static const jjs_allocator_vtab_t stblib_allocator_vtab = {
  .alloc = stdlib_alloc,
  .free = stdlib_free,
};

static jjs_allocator_t stdlib_allocator = {
  .vtab_p = &stblib_allocator_vtab,
};

static void
reset_stdlib_allocator_tracking (void)
{
  stdlib_alloc_called = false;
  stdlib_alloc_called_with = 0;
  stdlib_free_called = false;
}

static void
test_context_allocator (void)
{
  jjs_context_options_t options = {
    .allocator = &stdlib_allocator,
  };

  jjs_context_t *context_p;

  reset_stdlib_allocator_tracking ();

  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (stdlib_alloc_called == true);
  TEST_ASSERT (stdlib_alloc_called_with > JJS_DEFAULT_VM_HEAP_SIZE_KB + JJS_DEFAULT_SCRATCH_ARENA_KB);
  TEST_ASSERT (stdlib_free_called == false);
  jjs_context_free (context_p);
  TEST_ASSERT (stdlib_free_called == true);

  options = (jjs_context_options_t){
    .allocator = &stdlib_allocator,
    .vm_heap_size_kb = jjs_optional_u32 (2048),
    .scratch_size_kb = jjs_optional_u32 (64),
  };

  reset_stdlib_allocator_tracking ();

  TEST_ASSERT (jjs_context_new (&options, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (stdlib_alloc_called == true);
  TEST_ASSERT (stdlib_alloc_called_with > (2048 + 64) * 1024);
  TEST_ASSERT (stdlib_free_called == false);
  jjs_context_free (context_p);
  TEST_ASSERT (stdlib_free_called == true);
}

int
main (void)
{
  test_context_new_default ();
  test_context_new_null ();
  
  test_context_jjs_namespace ();
  test_context_jjs_namespace_exclusions ();
  
  test_init_options_stack_limit ();
  test_init_options_stack_limit_when_stack_static ();

  test_context_unhandled_rejection_handler ();

  test_context_allocator ();

  return 0;
} /* main */
