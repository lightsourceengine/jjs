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

#include "ecma-alloc.h"
#include "ecma-conversion.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"

#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

/**
 * Allocate a collection of ecma values.
 *
 * @return pointer to the collection
 */
ecma_collection_t *
ecma_new_collection (ecma_context_t *context_p) /**< JJS context */
{
  ecma_collection_t *collection_p;
  collection_p = (ecma_collection_t *) jmem_heap_alloc_block (context_p, sizeof (ecma_collection_t));

  collection_p->item_count = 0;
  collection_p->capacity = ECMA_COLLECTION_INITIAL_CAPACITY;
  const uint32_t size = ECMA_COLLECTION_ALLOCATED_SIZE (ECMA_COLLECTION_INITIAL_CAPACITY);
  collection_p->buffer_p = (ecma_value_t *) jmem_heap_alloc_block (context_p, size);

  return collection_p;
} /* ecma_new_collection */

/**
 * Deallocate a collection of ecma values without freeing it's values
 */
extern inline void JJS_ATTR_ALWAYS_INLINE
ecma_collection_destroy (ecma_context_t *context_p, /**< JJS context */
                         ecma_collection_t *collection_p) /**< value collection */
{
  JJS_ASSERT (collection_p != NULL);

  jmem_heap_free_block (context_p, collection_p->buffer_p, ECMA_COLLECTION_ALLOCATED_SIZE (collection_p->capacity));
  jmem_heap_free_block (context_p, collection_p, sizeof (ecma_collection_t));
} /* ecma_collection_destroy */

/**
 * Free the template literal objects and deallocate the collection
 */
void
ecma_collection_free_template_literal (ecma_context_t *context_p, /**< JJS context */
                                       ecma_collection_t *collection_p) /**< value collection */
{
  for (uint32_t i = 0; i < collection_p->item_count; i++)
  {
    ecma_object_t *object_p = ecma_get_object_from_value (context_p, collection_p->buffer_p[i]);

    JJS_ASSERT (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_ARRAY);

    ecma_extended_object_t *array_object_p = (ecma_extended_object_t *) object_p;

    JJS_ASSERT (array_object_p->u.array.length_prop_and_hole_count & ECMA_ARRAY_TEMPLATE_LITERAL);
    array_object_p->u.array.length_prop_and_hole_count &= (uint32_t) ~ECMA_ARRAY_TEMPLATE_LITERAL;

    ecma_property_value_t *property_value_p;

    property_value_p = ecma_get_named_data_property (context_p, object_p, ecma_get_magic_string (LIT_MAGIC_STRING_RAW));
    ecma_object_t *raw_object_p = ecma_get_object_from_value (context_p, property_value_p->value);

    JJS_ASSERT (ecma_get_object_type (raw_object_p) == ECMA_OBJECT_TYPE_ARRAY);

    array_object_p = (ecma_extended_object_t *) raw_object_p;

    JJS_ASSERT (array_object_p->u.array.length_prop_and_hole_count & ECMA_ARRAY_TEMPLATE_LITERAL);
    array_object_p->u.array.length_prop_and_hole_count &= (uint32_t) ~ECMA_ARRAY_TEMPLATE_LITERAL;

    ecma_deref_object (raw_object_p);
    ecma_deref_object (object_p);
  }

  ecma_collection_destroy (context_p, collection_p);
} /* ecma_collection_free_template_literal */

/**
 * Free the collection elements and deallocate the collection
 */
void
ecma_collection_free (ecma_context_t *context_p, /**< JJS context */
                      ecma_collection_t *collection_p) /**< value collection */
{
  JJS_ASSERT (collection_p != NULL);

  ecma_value_t *buffer_p = collection_p->buffer_p;

  for (uint32_t i = 0; i < collection_p->item_count; i++)
  {
    ecma_free_value (context_p, buffer_p[i]);
  }

  ecma_collection_destroy (context_p, collection_p);
} /* ecma_collection_free */

/**
 * Append new value to ecma values collection
 *
 * Note: The reference count of the values are not increased
 */
void
ecma_collection_push_back (ecma_context_t *context_p, /**< JJS context */
                           ecma_collection_t *collection_p, /**< value collection */
                           ecma_value_t value) /**< ecma value to append */
{
  JJS_ASSERT (collection_p != NULL);

  ecma_value_t *buffer_p = collection_p->buffer_p;

  if (JJS_LIKELY (collection_p->item_count < collection_p->capacity))
  {
    buffer_p[collection_p->item_count++] = value;
    return;
  }

  const uint32_t new_capacity = collection_p->capacity + ECMA_COLLECTION_GROW_FACTOR;
  const uint32_t old_size = ECMA_COLLECTION_ALLOCATED_SIZE (collection_p->capacity);
  const uint32_t new_size = ECMA_COLLECTION_ALLOCATED_SIZE (new_capacity);

  buffer_p = jmem_heap_realloc_block (context_p, buffer_p, old_size, new_size);
  buffer_p[collection_p->item_count++] = value;
  collection_p->capacity = new_capacity;

  collection_p->buffer_p = buffer_p;
} /* ecma_collection_push_back */

/**
 * Reserve space for the given amount of ecma_values in the collection
 */
void
ecma_collection_reserve (ecma_context_t *context_p, /**< JJS context */
                         ecma_collection_t *collection_p, /**< value collection */
                         uint32_t count) /**< number of ecma values to reserve */
{
  JJS_ASSERT (collection_p != NULL);
  JJS_ASSERT (UINT32_MAX - count > collection_p->capacity);

  const uint32_t new_capacity = collection_p->capacity + count;
  const uint32_t old_size = ECMA_COLLECTION_ALLOCATED_SIZE (collection_p->capacity);
  const uint32_t new_size = ECMA_COLLECTION_ALLOCATED_SIZE (new_capacity);

  ecma_value_t *buffer_p = collection_p->buffer_p;
  buffer_p = jmem_heap_realloc_block (context_p, buffer_p, old_size, new_size);

  collection_p->capacity = new_capacity;
  collection_p->buffer_p = buffer_p;
} /* ecma_collection_reserve */

/**
 * Append a list of values to the end of the collection
 */
void
ecma_collection_append (ecma_context_t *context_p, /**< JJS context */
                        ecma_collection_t *collection_p, /**< value collection */
                        const ecma_value_t *buffer_p, /**< values to append */
                        uint32_t count) /**< number of ecma values to append */
{
  JJS_ASSERT (collection_p != NULL);
  JJS_ASSERT (collection_p->capacity >= collection_p->item_count);

  uint32_t free_count = collection_p->capacity - collection_p->item_count;

  if (free_count < count)
  {
    ecma_collection_reserve (context_p, collection_p, count - free_count);
  }

  memcpy (collection_p->buffer_p + collection_p->item_count, buffer_p, count * sizeof (ecma_value_t));
  collection_p->item_count += count;
} /* ecma_collection_append */

/**
 * Helper function to check if a given collection have duplicated properties or not
 *
 * @return true - if there are duplicated properties in the collection
 *         false - otherwise
 */
bool
ecma_collection_check_duplicated_entries (ecma_context_t *context_p, /**< JJS context */
                                          ecma_collection_t *collection_p) /**< prop name collection */
{
  if (collection_p->item_count == 0)
  {
    return false;
  }

  ecma_value_t *buffer_p = collection_p->buffer_p;

  for (uint32_t i = 0; i < collection_p->item_count - 1; i++)
  {
    ecma_string_t *current_name_p = ecma_get_prop_name_from_value (context_p, buffer_p[i]);

    for (uint32_t j = i + 1; j < collection_p->item_count; j++)
    {
      if (ecma_compare_ecma_strings (current_name_p, ecma_get_prop_name_from_value (context_p, buffer_p[j])))
      {
        return true;
      }
    }
  }

  return false;
} /* ecma_collection_check_duplicated_entries */

/**
 * Check the string value existance in the collection.
 *
 * Used by:
 *         - ecma_builtin_json_stringify step 4.b.ii.5
 *         - ecma_op_object_enumerate
 *
 * @return true, if the string is already in the collection.
 */
bool
ecma_collection_has_string_value (ecma_context_t *context_p, /**< JJS context */
                                  ecma_collection_t *collection_p, /**< collection */
                                  ecma_string_t *string_p) /**< string */
{
  ecma_value_t *buffer_p = collection_p->buffer_p;

  for (uint32_t i = 0; i < collection_p->item_count; i++)
  {
    ecma_string_t *current_p = ecma_get_string_from_value (context_p, buffer_p[i]);

    if (ecma_compare_ecma_strings (current_p, string_p))
    {
      return true;
    }
  }

  return false;
} /* ecma_collection_has_string_value */

/**
 * Initial capacity of an ecma-collection
 */
#define ECMA_COMPACT_COLLECTION_GROWTH 8

/**
 * Set the size of the compact collection
 */
#define ECMA_COMPACT_COLLECTION_SET_SIZE(compact_collection_p, item_count, unused_items) \
  ((compact_collection_p)[0] = (((item_count) << ECMA_COMPACT_COLLECTION_SIZE_SHIFT) | (unused_items)))

/**
 * Set the size of the compact collection
 */
#define ECMA_COMPACT_COLLECTION_GET_UNUSED_ITEM_COUNT(compact_collection_p) \
  ((compact_collection_p)[0] & ((1 << ECMA_COMPACT_COLLECTION_SIZE_SHIFT) - 1))

/**
 * Allocate a compact collection of ecma values
 *
 * @return pointer to the compact collection
 */
ecma_value_t *
ecma_new_compact_collection (ecma_context_t *context_p) /**< JJS context */
{
  size_t size = (ECMA_COMPACT_COLLECTION_GROWTH / 2) * sizeof (ecma_value_t);
  ecma_value_t *compact_collection_p = (ecma_value_t *) jmem_heap_alloc_block (context_p, size);

  ECMA_COMPACT_COLLECTION_SET_SIZE (compact_collection_p,
                                    ECMA_COMPACT_COLLECTION_GROWTH / 2,
                                    (ECMA_COMPACT_COLLECTION_GROWTH / 2) - 1);
  return compact_collection_p;
} /* ecma_new_compact_collection */

/**
 * Append a value to the compact collection
 *
 * @return updated pointer to the compact collection
 */
ecma_value_t *
ecma_compact_collection_push_back (ecma_context_t *context_p, /**< JJS context */
                                   ecma_value_t *compact_collection_p, /**< compact collection */
                                   ecma_value_t value) /**< ecma value to append */
{
  ecma_value_t size = ECMA_COMPACT_COLLECTION_GET_SIZE (compact_collection_p);
  ecma_value_t unused_items = ECMA_COMPACT_COLLECTION_GET_UNUSED_ITEM_COUNT (compact_collection_p);

  if (unused_items > 0)
  {
    compact_collection_p[size - unused_items] = value;
    (*compact_collection_p)--;
    return compact_collection_p;
  }

  if (size == ECMA_COMPACT_COLLECTION_GROWTH / 2)
  {
    size_t old_size = (ECMA_COMPACT_COLLECTION_GROWTH / 2) * sizeof (ecma_value_t);
    size_t new_size = ECMA_COMPACT_COLLECTION_GROWTH * sizeof (ecma_value_t);
    compact_collection_p = jmem_heap_realloc_block (context_p, compact_collection_p, old_size, new_size);

    compact_collection_p[ECMA_COMPACT_COLLECTION_GROWTH / 2] = value;

    ECMA_COMPACT_COLLECTION_SET_SIZE (compact_collection_p,
                                      ECMA_COMPACT_COLLECTION_GROWTH,
                                      (ECMA_COMPACT_COLLECTION_GROWTH / 2) - 1);
    return compact_collection_p;
  }

  size_t old_size = size * sizeof (ecma_value_t);
  size_t new_size = old_size + (ECMA_COMPACT_COLLECTION_GROWTH * sizeof (ecma_value_t));

  compact_collection_p = jmem_heap_realloc_block (context_p, compact_collection_p, old_size, new_size);
  compact_collection_p[size] = value;

  ECMA_COMPACT_COLLECTION_SET_SIZE (compact_collection_p,
                                    size + ECMA_COMPACT_COLLECTION_GROWTH,
                                    ECMA_COMPACT_COLLECTION_GROWTH - 1);
  return compact_collection_p;
} /* ecma_compact_collection_push_back */

/**
 * Discard the unused elements of a compact collection
 *
 * Note:
 *     further items should not be added after this call
 *
 * @return updated pointer to the compact collection
 */
ecma_value_t *
ecma_compact_collection_shrink (ecma_context_t *context_p, /**< JJS context */
                                ecma_value_t *compact_collection_p) /**< compact collection */
{
  ecma_value_t unused_items = ECMA_COMPACT_COLLECTION_GET_UNUSED_ITEM_COUNT (compact_collection_p);

  if (unused_items == 0)
  {
    return compact_collection_p;
  }

  ecma_value_t size = ECMA_COMPACT_COLLECTION_GET_SIZE (compact_collection_p);

  size_t old_size = size * sizeof (ecma_value_t);
  size_t new_size = (size - unused_items) * sizeof (ecma_value_t);

  compact_collection_p = jmem_heap_realloc_block (context_p, compact_collection_p, old_size, new_size);

  ECMA_COMPACT_COLLECTION_SET_SIZE (compact_collection_p, size - unused_items, 0);
  return compact_collection_p;
} /* ecma_compact_collection_shrink */

/**
 * Free a compact collection
 */
void
ecma_compact_collection_free (ecma_context_t *context_p, /**< JJS context */
                              ecma_value_t *compact_collection_p) /**< compact collection */
{
  ecma_value_t size = ECMA_COMPACT_COLLECTION_GET_SIZE (compact_collection_p);
  ecma_value_t unused_items = ECMA_COMPACT_COLLECTION_GET_UNUSED_ITEM_COUNT (compact_collection_p);

  ecma_value_t *end_p = compact_collection_p + size - unused_items;
  ecma_value_t *current_p = compact_collection_p + 1;

  while (current_p < end_p)
  {
    ecma_free_value (context_p, *current_p++);
  }

  jmem_heap_free_block (context_p, compact_collection_p, size * sizeof (ecma_value_t));
} /* ecma_compact_collection_free */

/**
 * Get the end of a compact collection
 *
 * @return pointer to the compact collection end
 */
ecma_value_t *
ecma_compact_collection_end (ecma_value_t *compact_collection_p) /**< compact collection */
{
  ecma_value_t size = ECMA_COMPACT_COLLECTION_GET_SIZE (compact_collection_p);
  ecma_value_t unused_items = ECMA_COMPACT_COLLECTION_GET_UNUSED_ITEM_COUNT (compact_collection_p);

  return compact_collection_p + size - unused_items;
} /* ecma_compact_collection_end */

/**
 * Destroy a compact collection
 */
void
ecma_compact_collection_destroy (ecma_context_t *context_p, /**< JJS context */
                                 ecma_value_t *compact_collection_p) /**< compact collection */
{
  ecma_value_t size = ECMA_COMPACT_COLLECTION_GET_SIZE (compact_collection_p);

  jmem_heap_free_block (context_p, compact_collection_p, size * sizeof (ecma_value_t));
} /* ecma_compact_collection_destroy */

static bool
ecma_hashset_put_internal (ecma_hashset_t *self, /**< this hashset */
                  ecma_value_t string_value, /**< ecma string to insert */
                  bool move_string_value) /**< transfer ownership of string_value to the hashset.
                                           *   otherwise, string_value ref is copied. */
{
  lit_string_hash_t hash = ecma_string_hash (ecma_get_string_from_value (self->context_p, string_value));
  ecma_hashset_node_t *node = &self->buckets[hash % self->capacity];

  if (node->item == ECMA_VALUE_EMPTY)
  {
    node->item = move_string_value ? string_value : ecma_copy_value (self->context_p, string_value);
  }
  else
  {
    ecma_hashset_node_t *new_node = jmem_pools_alloc (self->context_p, (jjs_size_t) sizeof (*new_node));

    if (new_node == NULL)
    {
      return false;
    }

    *new_node = (ecma_hashset_node_t) {
      .item = move_string_value ? string_value : ecma_copy_value (self->context_p, string_value),
      .next_p = node->next_p,
    };

    node->next_p = new_node;
  }

  self->size++;

  return true;
} /* ecma_hashset_put_internal */

/**
 * If load factor is exceeded, the hashset will be rebuilt with capacity * ECMA_HASHSET_GROW_BY buckets.
 */
#define ECMA_HASHSET_GROW_BY (2)

/**
 * If size is exceeds capacity * ECMA_HASHSET_LOAD_FACTOR, the hashset will be rebuilt with
 * capacity * ECMA_HASHSET_GROW_BY buckets.
 */
#define ECMA_HASHSET_LOAD_FACTOR (2)

static bool
ecma_hashset_respec (ecma_hashset_t *self)
{
  if (self->capacity == UINT32_MAX)
  {
    return true;
  }

  jjs_size_t limit;

  if (self->capacity > UINT32_MAX / ECMA_HASHSET_LOAD_FACTOR)
  {
    limit = UINT32_MAX;
  }
  else
  {
    limit = self->capacity * ECMA_HASHSET_GROW_BY;
  }

  if (self->size < limit)
  {
    return true;
  }

  ecma_hashset_t new_hashset;
  jjs_size_t new_capacity;

  if (self->capacity > UINT32_MAX / ECMA_HASHSET_GROW_BY)
  {
    new_capacity = UINT32_MAX;
  }
  else
  {
    new_capacity = self->capacity * ECMA_HASHSET_GROW_BY;
  }

  if (!ecma_hashset_init (&new_hashset,
                          self->context_p,
                          self->allocator_p,
                          new_capacity))
  {
    return false;
  }

  for (jjs_size_t i = 0; i < self->capacity; i++)
  {
    ecma_hashset_node_t *walker = self->buckets[i].next_p;
    ecma_hashset_node_t *t;

    if (self->buckets[i].item != ECMA_VALUE_EMPTY)
    {
      if (!ecma_hashset_put_internal (&new_hashset, self->buckets[i].item, true))
      {
        jjs_platform_fatal (self->context_p, JJS_FATAL_OUT_OF_MEMORY);
      }
      self->buckets[i].item = ECMA_VALUE_EMPTY;
    }

    while (walker)
    {
      t = walker;
      walker = walker->next_p;

      ecma_value_t item = t->item;

      jmem_pools_free (self->context_p, t, (jjs_size_t) sizeof (*t));

      if (!ecma_hashset_put_internal (&new_hashset, item, true))
      {
        jjs_platform_fatal (self->context_p, JJS_FATAL_OUT_OF_MEMORY);
      }
    }

    self->buckets[i].next_p = NULL;
  }

  ecma_hashset_free (self);

  *self = new_hashset;

  return true;
}

/**
 * Specialized string hashset for internal use.
 */
bool
ecma_hashset_init (ecma_hashset_t *self, /**< this hashset */
                   ecma_context_t *context_p, /**< JJS context */
                   const jjs_allocator_t *allocator_p, /**< allocator for bucket array */
                   jjs_size_t capacity) /**< number of initial buckets */
{
  const jjs_size_t size = (jjs_size_t) sizeof (*self->buckets) * capacity;

  *self = (ecma_hashset_t) {
    .allocator_p = allocator_p,
    .buckets = jjs_allocator_alloc (allocator_p, size),
    .capacity = capacity,
    .context_p = context_p
  };

  JJS_ASSERT (self->buckets);

  if (!self->buckets)
  {
    return false;
  }

  for (jjs_size_t i = 0; i < capacity; i++)
  {
    self->buckets[i] = (ecma_hashset_node_t) {
      .item = ECMA_VALUE_EMPTY,
    };
  }

  return true;
} /* ecma_hashset_init */

/**
 * Free the hashset and release all held string values.
 */
void
ecma_hashset_free (ecma_hashset_t *self) /**< this hashset */
{
  const jjs_size_t capacity = self->capacity;

  for (jjs_size_t i = 0; i < capacity; i++)
  {
    ecma_hashset_node_t *walker = self->buckets[i].next_p;
    ecma_hashset_node_t *t;

    ecma_free_value (self->context_p, self->buckets[i].item);

    while (walker)
    {
      t = walker;
      walker = walker->next_p;

      ecma_free_value (self->context_p, t->item);
      jmem_pools_free (self->context_p, t, (jjs_size_t) sizeof (*t));
    }
  }

  if (self->buckets)
  {
    jjs_allocator_free (self->allocator_p, self->buckets, (jjs_size_t) sizeof (*self->buckets) * self->capacity);
  }
} /* ecma_hashset_free */

/**
 * Checks that all string references held by the hashset have exactly one reference.
 *
 * During context shutdown, the final GC is run. If there are no leaks, all strings should
 * have exactly one ref or there is a leak somewhere. This audit is done in debug builds
 * only. This function is not intended for use outside of the context shutdown use case.
 */
void
ecma_hash_set_audit_finalize (ecma_hashset_t *self) /**< this hashset */
{
#ifndef JJS_NDEBUG
  const jjs_size_t capacity = self->capacity;

  for (jjs_size_t i = 0; i < capacity; i++)
  {
    ecma_hashset_node_t *walker = self->buckets[i].next_p;
    ecma_hashset_node_t *t;

    if (self->buckets[i].item != ECMA_VALUE_EMPTY)
    {
      ecma_string_t *string_p = ecma_get_string_from_value (self->context_p, self->buckets[i].item);
      JJS_ASSERT (ECMA_STRING_IS_REF_EQUALS_TO_ONE (string_p));
    }

    while (walker)
    {
      t = walker;
      walker = walker->next_p;

      if (t->item != ECMA_VALUE_EMPTY)
      {
        ecma_string_t *string_p = ecma_get_string_from_value (self->context_p, t->item);
        JJS_ASSERT (ECMA_STRING_IS_REF_EQUALS_TO_ONE (string_p));
      }
    }
  }
#else /* JJS_NDEBUG */
  (void) self;
#endif /* !JJS_NDEBUG */
} /* ecma_hash_set_audit_finalize */

/**
 * Find a string value by raw string.
 *
 * Note: to avoid copy/free calls in lit storage, the hashset's reference is returned. do not free.
 *
 * @return success: string value, failure: ECMA_VALUE_NOT_FOUND; the returned value should not be
 * freed. use ecma_copy_value if you need a reference.
 */
ecma_value_t
ecma_hashset_get_raw (ecma_hashset_t *self, /**< this hashset */
                  const lit_utf8_byte_t *key_p, /**< raw string key */
                  lit_utf8_size_t key_size) /**< raw string size in bytes */
{
  /* use the existing string hash, but it is not the best. */
  lit_string_hash_t hash = lit_utf8_string_calc_hash (key_p, key_size);
  ecma_hashset_node_t *node = &self->buckets[hash % self->capacity];

  if (node->item != ECMA_VALUE_EMPTY)
  {
    lit_utf8_size_t size;
    const lit_utf8_byte_t *chars;
    /* avoid alloc path in ecma_string_get_chars by providing buffer */
    lit_utf8_byte_t uint32_to_string_buffer[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
    ecma_context_t *context_p = self->context_p;
    uint8_t flags = 0;
    ecma_hashset_node_t *walker = node;

    while (walker)
    {
      chars = ecma_string_get_chars (context_p,
                                     ecma_get_string_from_value (context_p, walker->item),
                                     &size,
                                     NULL,
                                     uint32_to_string_buffer,
                                     &flags);

      /* item is always a non-direct string. no path through ecma_string_get_chars should allocate memory */
      JJS_ASSERT ((flags & ECMA_STRING_FLAG_MUST_BE_FREED) == 0);

      if (size == key_size && *key_p == *chars && memcmp (key_p, chars, key_size) == 0)
      {
        return walker->item;
      }

      walker = walker->next_p;
    }
  }

  return ECMA_VALUE_NOT_FOUND;
} /* ecma_hashset_get_raw */

/**
 * Find a string value by ecma string value.
 *
 * Note: to avoid copy/free calls in lit storage, the hashset's reference is returned. do not free.
 *
 * @return success: string value, failure: ECMA_VALUE_NOT_FOUND; the returned value should not be
 * freed. use ecma_copy_value if you need a reference.
 */
ecma_value_t
ecma_hashset_get (ecma_hashset_t *self, /**< this hashset */
                  ecma_value_t key) /**< ecma string key */
{
  ecma_context_t *context_p = self->context_p;
  JJS_ASSERT (ecma_is_value_string (key));
  ecma_string_t *key_string_p = ecma_get_string_from_value (self->context_p, key);
  lit_string_hash_t hash = ecma_string_hash (key_string_p);
  ecma_hashset_node_t *node = &self->buckets[hash % self->capacity];

  if (node->item != ECMA_VALUE_EMPTY)
  {
    ecma_hashset_node_t *walker = node;

    while (walker)
    {
      if (ecma_compare_ecma_strings (ecma_get_string_from_value (context_p, walker->item), key_string_p))
      {
        return walker->item;
      }

      walker = walker->next_p;
    }
  }

  return ECMA_VALUE_NOT_FOUND;
}

/**
 * Insert a string into the set.
 *
 * Note: for performance reasons, the caller must ensure that string is not already in the set.
 *
 * @return true: successful insert; false: out of memory.
 */
bool
ecma_hashset_put (ecma_hashset_t *self, /**< this hashset */
                  ecma_value_t string_value, /**< ecma string to insert */
                  bool move_string_value) /**< transfer ownership of string_value to the hashset.
                                           *   otherwise, string_value ref is copied. */
{
  /* assume string_value is not in the set */
//  if (ecma_hashset_get (self, string_value) != ECMA_VALUE_NOT_FOUND)
//  {
//    ecma_hashset_get (self, string_value);
//  }
  JJS_ASSERT (ecma_hashset_get (self, string_value) == ECMA_VALUE_NOT_FOUND);

  if (!ecma_hashset_respec (self))
  {
    if (move_string_value)
    {
      ecma_free_value (self->context_p, string_value);
    }
    return false;
  }

  return ecma_hashset_put_internal (self, string_value, move_string_value);
} /* ecma_hashset_put */

/**
 * @}
 * @}
 */
