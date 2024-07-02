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
test_context_new_empty_options (void)
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
test_context_new_no_arena (void)
{
  jjs_context_options_t options = {
    /* size of 0 disables the arena */
    .scratch_size_kb = jjs_optional_u32 (0),
  };
  jjs_context_t *context_p = ctx_open (&options);

  /* do some vm work */
  for (jjs_size_t i = 0; i < 5; i++)
  {
    ctx_defer_free (jjs_binary_op (context_p, JJS_BIN_OP_ADD, jjs_string_sz(ctx (), "x"), JJS_MOVE, jjs_string_sz(ctx (), "y"), JJS_MOVE));
  }

  ctx_close ();
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

static void
test_context_jjs_namespace (void)
{
  ctx_open (NULL);
  
  jjs_value_t global = ctx_global ();
  jjs_value_t jjs = ctx_defer_free (jjs_object_get_sz (ctx (), global, "jjs"));

  TEST_ASSERT (jjs_value_is_object (ctx (), jjs));
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), jjs, "version"))));
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), jjs, "os"))));
  TEST_ASSERT (jjs_value_is_string (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), jjs, "arch"))));
  TEST_ASSERT (jjs_value_is_object (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), jjs, "stdout"))));
  TEST_ASSERT (jjs_value_is_object (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), jjs, "stderr"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), jjs, "pmap"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), jjs, "vmod"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), jjs, "readFile"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), jjs, "realpath"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), jjs, "cwd"))));
  TEST_ASSERT (jjs_value_is_function (ctx (), ctx_defer_free (jjs_object_get_sz (ctx (), jjs, "gc"))));

  ctx_close ();
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
  jjs_context_t *context_p;

  reset_stdlib_allocator_tracking ();

  TEST_ASSERT (jjs_context_new_with_allocator (NULL, &stdlib_allocator, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (stdlib_alloc_called == true);
  TEST_ASSERT (stdlib_alloc_called_with > JJS_DEFAULT_VM_HEAP_SIZE_KB + JJS_DEFAULT_SCRATCH_SIZE_KB);
  TEST_ASSERT (stdlib_free_called == false);
  jjs_context_free (context_p);
  TEST_ASSERT (stdlib_free_called == true);

  jjs_context_options_t options = {
    .vm_heap_size_kb = jjs_optional_u32 (2048),
    .scratch_size_kb = jjs_optional_u32 (64),
  };

  reset_stdlib_allocator_tracking ();

  TEST_ASSERT (jjs_context_new_with_allocator (&options, &stdlib_allocator, &context_p) == JJS_STATUS_OK);
  TEST_ASSERT (stdlib_alloc_called == true);
  TEST_ASSERT (stdlib_alloc_called_with > (2048 + 64) * 1024);
  TEST_ASSERT (stdlib_free_called == false);
  jjs_context_free (context_p);
  TEST_ASSERT (stdlib_free_called == true);
}

static void
test_context_data_init (void)
{
  jjs_context_t *context_p;
  jjs_context_data_key_t key;
  char id[JJS_CONTEXT_DATA_ID_LIMIT + 1];
  memset(id, 'x', JJS_ARRAY_SIZE (id));
  id[JJS_CONTEXT_DATA_ID_LIMIT] = '\0';

  context_p = ctx_open (NULL);

  /* expect successful registration of data */
  TEST_ASSERT (jjs_context_data_init (context_p, "test", NULL, &key) == JJS_STATUS_OK);
  TEST_ASSERT (key == 0);

  /* expect error if id already registered */
  TEST_ASSERT (jjs_context_data_init (context_p, "test", NULL, NULL) == JJS_STATUS_CONTEXT_DATA_EXISTS);

  /* expect error if id is too long */
  TEST_ASSERT (jjs_context_data_init (context_p, id, NULL, NULL) == JJS_STATUS_CONTEXT_DATA_ID_SIZE);

  ctx_close ();

  /* expect error if all data slots are in use */
  context_p = ctx_open (NULL);

  for (int32_t i = 0; i < JJS_CONTEXT_DATA_LIMIT; i++)
  {
    snprintf (id, JJS_ARRAY_SIZE (id), "%i", i);
    TEST_ASSERT (jjs_context_data_init (context_p, id, NULL, NULL) == JJS_STATUS_OK);
  }

  TEST_ASSERT (jjs_context_data_init (context_p, "test", NULL, NULL) == JJS_STATUS_CONTEXT_DATA_FULL);

  ctx_close ();
}

static void
test_context_data_key (void)
{
  jjs_context_t *context_p;
  jjs_context_data_key_t key;

  context_p = ctx_open (NULL);
  TEST_ASSERT (jjs_context_data_init (context_p, "test", NULL, &key) == JJS_STATUS_OK);

  /* expect valid key for test */
  TEST_ASSERT (jjs_context_data_key (context_p, "test") == key);

  /* expect -1 for unregistered key */
  TEST_ASSERT (jjs_context_data_key (context_p, "xxx") == -1);

  ctx_close ();
}

static void
test_context_data_get (void)
{
  jjs_context_t *context_p;
  void* data_p;
  jjs_context_data_key_t key;

  context_p = ctx_open (NULL);
  TEST_ASSERT (jjs_context_data_init (context_p, "test", context_p, &key) == JJS_STATUS_OK);

  /* expect to retrieve data by key */
  TEST_ASSERT (jjs_context_data_get (context_p, key, &data_p) == JJS_STATUS_OK);
  TEST_ASSERT (data_p == context_p);

  /* expect error for invalid key */
  TEST_ASSERT (jjs_context_data_get (context_p, 1024, &data_p) == JJS_STATUS_CONTEXT_DATA_NOT_FOUND);

  ctx_close ();
}

static void
test_context_data_set (void)
{
  jjs_context_t *context_p;
  void *data_p;
  jjs_context_data_key_t key;

  context_p = ctx_open (NULL);
  TEST_ASSERT (jjs_context_data_init (context_p, "test", NULL, &key) == JJS_STATUS_OK);

  /* expect data to be successfully set */
  TEST_ASSERT (jjs_context_data_set (context_p, key, context_p) == JJS_STATUS_OK);
  TEST_ASSERT (jjs_context_data_get (context_p, key, &data_p) == JJS_STATUS_OK);
  TEST_ASSERT (data_p == context_p);

  /* expect error if key not registered */
  TEST_ASSERT (jjs_context_data_set (context_p, 1024, context_p) == JJS_STATUS_CONTEXT_DATA_NOT_FOUND);

  ctx_close ();
}

int
main (void)
{
  test_context_new_empty_options ();
  test_context_new_null ();
  test_context_new_no_arena ();
  
  test_context_jjs_namespace ();
  
  test_init_options_stack_limit ();
  test_init_options_stack_limit_when_stack_static ();

  test_context_allocator ();

  test_context_data_init ();
  test_context_data_key ();
  test_context_data_get ();
  test_context_data_set ();

  return 0;
} /* main */
