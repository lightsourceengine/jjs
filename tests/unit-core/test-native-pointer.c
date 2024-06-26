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

static int global_int = 4;
static void *global_p = (void *) &global_int;
static int global_counter = 0;

static void
native_free_callback (jjs_context_t *context_p, /** JJS context */
                      void *native_p, /**< native pointer */
                      const jjs_object_native_info_t *info_p) /**< native info */
{
  JJS_UNUSED (context_p);
  JJS_UNUSED (native_p);
  TEST_ASSERT (info_p->free_cb == native_free_callback);
  global_counter++;
} /* native_free_callback */

static const jjs_object_native_info_t native_info_1 = {
  .free_cb = native_free_callback,
  .number_of_references = 0,
  .offset_of_references = 0,
};

static const jjs_object_native_info_t native_info_2 = {
  .free_cb = NULL,
  .number_of_references = 0,
  .offset_of_references = 0,
};

static const jjs_object_native_info_t native_info_3 = {
  .free_cb = NULL,
  .number_of_references = 0,
  .offset_of_references = 0,
};

typedef struct
{
  uint32_t check_before;
  jjs_value_t a;
  jjs_value_t b;
  jjs_value_t c;
  uint32_t check_after;
} test_references_t;

static test_references_t test_references1;
static test_references_t test_references2;
static test_references_t test_references3;
static test_references_t test_references4;
static int call_count = 0;

static void
native_references_free_callback (jjs_context_t *context_p, /** JJS context */
                                 void *native_p, /**< native pointer */
                                 const jjs_object_native_info_t *info_p) /**< native info */
{
  JJS_UNUSED (context_p);
  test_references_t *refs_p = (test_references_t *) native_p;

  TEST_ASSERT ((refs_p == &test_references1 && test_references1.check_before == 0x12345678)
               || (refs_p == &test_references2 && test_references2.check_before == 0x87654321)
               || (refs_p == &test_references3 && test_references3.check_before == 0x12344321));
  TEST_ASSERT (refs_p->check_before == refs_p->check_after);

  uint32_t check = refs_p->check_before;

  jjs_native_ptr_free (ctx (), native_p, info_p);

  TEST_ASSERT (jjs_value_is_undefined (ctx (), refs_p->a));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), refs_p->b));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), refs_p->c));
  TEST_ASSERT (refs_p->check_before == check);
  TEST_ASSERT (refs_p->check_after == check);

  call_count++;
} /* native_references_free_callback */

static const jjs_object_native_info_t native_info_4 = {
  .free_cb = native_references_free_callback,
  .number_of_references = 3,
  .offset_of_references = (uint16_t) offsetof (test_references_t, a),
};

static void
init_references (test_references_t *refs_p, /**< native pointer */
                 uint32_t check) /**< value for memory check */
{
  /* Memory garbage */
  refs_p->check_before = check;
  refs_p->a = 1;
  refs_p->b = 2;
  refs_p->c = 3;
  refs_p->check_after = check;

  jjs_native_ptr_init (ctx (), (void *) refs_p, &native_info_4);

  TEST_ASSERT (jjs_value_is_undefined (ctx (), refs_p->a));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), refs_p->b));
  TEST_ASSERT (jjs_value_is_undefined (ctx (), refs_p->c));
  TEST_ASSERT (refs_p->check_before == check);
  TEST_ASSERT (refs_p->check_after == check);
} /* init_references */

static void
set_references (test_references_t *refs_p, /**< native pointer */
                jjs_value_t value1, /**< first value to be set */
                jjs_value_t value2, /**< second value to be set */
                jjs_value_t value3) /**< third value to be set */
{
  jjs_native_ptr_set (ctx (), &refs_p->a, value1);
  jjs_native_ptr_set (ctx (), &refs_p->b, value2);
  jjs_native_ptr_set (ctx (), &refs_p->c, value3);

  TEST_ASSERT (jjs_value_is_object (ctx (), value1) ? jjs_value_is_object (ctx (), refs_p->a) : jjs_value_is_string (ctx (), refs_p->a));
  TEST_ASSERT (jjs_value_is_object (ctx (), value2) ? jjs_value_is_object (ctx (), refs_p->b) : jjs_value_is_string (ctx (), refs_p->b));
  TEST_ASSERT (jjs_value_is_object (ctx (), value3) ? jjs_value_is_object (ctx (), refs_p->c) : jjs_value_is_string (ctx (), refs_p->c));
} /* set_references */

static void
check_native_info (jjs_value_t object_value, /**< object value */
                   const jjs_object_native_info_t *native_info_p, /**< native info */
                   void *expected_pointer_p) /**< expected pointer */
{
  TEST_ASSERT (jjs_object_has_native_ptr (ctx (), object_value, native_info_p));
  void *native_pointer_p = jjs_object_get_native_ptr (ctx (), object_value, native_info_p);
  TEST_ASSERT (native_pointer_p == expected_pointer_p);
} /* check_native_info */

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t object_value = jjs_object (ctx ());

  jjs_object_set_native_ptr (ctx (), object_value, &native_info_1, global_p);
  jjs_object_set_native_ptr (ctx (), object_value, &native_info_2, NULL);

  check_native_info (object_value, &native_info_1, global_p);
  check_native_info (object_value, &native_info_2, NULL);

  jjs_value_free (ctx (), object_value);

  jjs_heap_gc (ctx (), JJS_GC_PRESSURE_HIGH);
  TEST_ASSERT (global_counter == 1);
  global_counter = 0;

  object_value = jjs_object (ctx ());

  jjs_object_set_native_ptr (ctx (), object_value, &native_info_1, global_p);
  jjs_object_set_native_ptr (ctx (), object_value, &native_info_2, NULL);

  TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object_value, &native_info_1));

  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_1));
  check_native_info (object_value, &native_info_2, NULL);

  TEST_ASSERT (!jjs_object_delete_native_ptr (ctx (), object_value, &native_info_1));

  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_1));
  check_native_info (object_value, &native_info_2, NULL);

  TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object_value, &native_info_2));

  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_1));
  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_2));

  jjs_object_set_native_ptr (ctx (), object_value, &native_info_1, NULL);

  check_native_info (object_value, &native_info_1, NULL);
  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_2));

  jjs_object_set_native_ptr (ctx (), object_value, &native_info_2, global_p);

  check_native_info (object_value, &native_info_1, NULL);
  check_native_info (object_value, &native_info_2, global_p);

  jjs_object_set_native_ptr (ctx (), object_value, &native_info_1, global_p);

  check_native_info (object_value, &native_info_1, global_p);
  check_native_info (object_value, &native_info_2, global_p);

  TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object_value, &native_info_1));
  TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object_value, &native_info_2));

  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_1));
  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_2));

  jjs_object_set_native_ptr (ctx (), object_value, &native_info_1, global_p);
  jjs_object_set_native_ptr (ctx (), object_value, &native_info_2, NULL);
  jjs_object_set_native_ptr (ctx (), object_value, &native_info_3, global_p);

  check_native_info (object_value, &native_info_1, global_p);
  check_native_info (object_value, &native_info_2, NULL);
  check_native_info (object_value, &native_info_3, global_p);

  TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object_value, &native_info_1));
  TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object_value, &native_info_2));
  TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object_value, &native_info_3));

  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_1));
  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_2));
  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_3));

  jjs_object_set_native_ptr (ctx (), object_value, &native_info_1, NULL);
  jjs_object_set_native_ptr (ctx (), object_value, &native_info_2, global_p);
  jjs_object_set_native_ptr (ctx (), object_value, &native_info_3, NULL);

  check_native_info (object_value, &native_info_1, NULL);
  check_native_info (object_value, &native_info_2, global_p);
  check_native_info (object_value, &native_info_3, NULL);

  /* Reversed delete order. */
  TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object_value, &native_info_3));
  TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object_value, &native_info_2));
  TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object_value, &native_info_1));

  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_1));
  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_2));
  TEST_ASSERT (!jjs_object_has_native_ptr (ctx (), object_value, &native_info_3));

  /* Test value references */
  jjs_value_t string1_value = jjs_string_sz (ctx (), "String1");
  jjs_value_t string2_value = jjs_string_sz (ctx (), "String2");

  jjs_value_t object1_value = jjs_object (ctx ());
  jjs_value_t object2_value = jjs_object (ctx ());

  init_references (&test_references1, 0x12345678);
  init_references (&test_references2, 0x87654321);

  jjs_object_set_native_ptr (ctx (), object1_value, &native_info_4, (void *) &test_references1);
  jjs_object_set_native_ptr (ctx (), object2_value, &native_info_4, (void *) &test_references2);

  /* Assign values (cross reference between object1 and object2). */
  set_references (&test_references1, string1_value, object2_value, string2_value);
  set_references (&test_references2, string2_value, object1_value, string1_value);

  jjs_heap_gc (ctx (), JJS_GC_PRESSURE_HIGH);

  /* Reassign values. */
  set_references (&test_references1, object2_value, string2_value, string1_value);
  set_references (&test_references2, object1_value, string1_value, string2_value);

  jjs_heap_gc (ctx (), JJS_GC_PRESSURE_HIGH);

  jjs_value_free (ctx (), object1_value);
  jjs_value_free (ctx (), object2_value);

  object1_value = jjs_object (ctx ());
  object2_value = jjs_object (ctx ());

  init_references (&test_references3, 0x12344321);

  /* Assign the same native pointer to multiple objects. */
  jjs_object_set_native_ptr (ctx (), object1_value, &native_info_4, (void *) &test_references3);
  jjs_object_set_native_ptr (ctx (), object2_value, &native_info_4, (void *) &test_references3);

  set_references (&test_references3, object1_value, object2_value, string1_value);

  jjs_heap_gc (ctx (), JJS_GC_PRESSURE_HIGH);

  init_references (&test_references4, 0x87655678);

  /* Re-assign reference */
  jjs_object_set_native_ptr (ctx (), object1_value, &native_info_4, (void *) &test_references4);

  set_references (&test_references4, string1_value, string2_value, string1_value);

  jjs_object_set_native_ptr (ctx (), object1_value, &native_info_4, NULL);

  jjs_native_ptr_free (ctx (), (void *) &test_references4, &native_info_4);

  /* Calling jjs_native_ptr_init with test_references4 is optional here. */

  jjs_object_set_native_ptr (ctx (), object1_value, &native_info_4, (void *) &test_references4);

  set_references (&test_references4, string2_value, string1_value, string2_value);

  TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object1_value, &native_info_4));

  jjs_native_ptr_free (ctx (), (void *) &test_references4, &native_info_4);

  jjs_value_free (ctx (), object1_value);
  jjs_value_free (ctx (), object2_value);

  /* Delete references */
  for (int i = 0; i < 3; i++)
  {
    object1_value = jjs_object (ctx ());

    jjs_object_set_native_ptr (ctx (), object1_value, NULL, global_p);
    jjs_object_set_native_ptr (ctx (), object1_value, &native_info_4, (void *) &test_references4);
    jjs_object_set_native_ptr (ctx (), object1_value, &native_info_2, global_p);
    set_references (&test_references4, string1_value, string2_value, object1_value);

    jjs_heap_gc (ctx (), JJS_GC_PRESSURE_HIGH);

    if (i == 1)
    {
      TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object1_value, NULL));
    }
    else if (i == 2)
    {
      TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object1_value, &native_info_2));
    }

    TEST_ASSERT (jjs_object_delete_native_ptr (ctx (), object1_value, &native_info_4));
    jjs_native_ptr_free (ctx (), (void *) &test_references4, &native_info_4);
    jjs_value_free (ctx (), object1_value);
  }

  jjs_value_free (ctx (), string1_value);
  jjs_value_free (ctx (), string2_value);

  jjs_value_free (ctx (), object_value);

  ctx_close ();

  TEST_ASSERT (global_counter == 0);
  TEST_ASSERT (call_count == 3);
  return 0;
} /* main */
