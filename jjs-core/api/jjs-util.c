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

#include "jjs-util.h"

#include <stdlib.h>

#include "ecma-helpers.h"

#include "jcontext.h"
#include "lit-char-helpers.h"

/**
 * Create an API compatible return value.
 *
 * @return return value for JJS API functions
 */
jjs_value_t
jjs_return (jjs_context_t *context_p, const jjs_value_t value) /**< return value */
{
  if (ECMA_IS_VALUE_ERROR (value))
  {
    return ecma_create_exception_from_context (context_p);
  }

  return value;
} /* jjs_return */


void
jjs_util_value_with_context_free (jjs_value_with_context_t* container_p)
{
  jjs_value_free (container_p->context_p, container_p->value);
}

/**
 * Maps a JS string option argument to an enum.
 *
 * The pattern in JS is fn('option') or fn({ key: 'option') or fn(). The string is
 * extracted and looked up in option_mappings_p. If found, return true. If not found,
 * return false. If the option is undefined, true is returned with default_mapped_value.
 */
bool
jjs_util_map_option (jjs_context_t* context_p,
                     jjs_value_t option,
                     jjs_value_ownership_t option_o,
                     jjs_value_t key,
                     jjs_value_ownership_t key_o,
                     const jjs_util_option_pair_t* option_mappings_p,
                     jjs_size_t len,
                     uint32_t default_mapped_value,
                     uint32_t* out_p)
{
  if (jjs_value_is_undefined (context_p, option))
  {
    JJS_DISOWN (context_p, option, option_o);
    JJS_DISOWN (context_p, key, key_o);
    *out_p = default_mapped_value;
    return true;
  }

  /* option_value = option.key or option */
  jjs_value_t option_value;

  if (jjs_value_is_string (context_p, option))
  {
    option_value = jjs_value_copy (context_p, option);
  }
  else if (jjs_value_is_string (context_p, key) && jjs_value_is_object (context_p, option))
  {
    option_value = jjs_object_get (context_p, option, key);

    if (jjs_value_is_undefined (context_p, option_value))
    {
      JJS_DISOWN (context_p, option, option_o);
      JJS_DISOWN (context_p, key, key_o);
      jjs_value_free (context_p, option_value);
      *out_p = default_mapped_value;
      return true;
    }
  }
  else
  {
    option_value = ECMA_VALUE_EMPTY;
  }

  JJS_DISOWN (context_p, option, option_o);
  JJS_DISOWN (context_p, key, key_o);

  if (!jjs_value_is_string (context_p, option_value))
  {
    jjs_value_free (context_p, option_value);
    return false;
  }

  /* tolower option_value */
  char buffer[32];

  static const jjs_size_t size = (jjs_size_t) (sizeof (buffer) / sizeof (buffer[0]));
  jjs_value_t w = jjs_string_to_buffer (context_p, option_value, JJS_ENCODING_CESU8, (lit_utf8_byte_t*) &buffer[0], size - 1);

  jjs_value_free (context_p, option_value);

  JJS_ASSERT (w < size);
  buffer[w] = '\0';

  lit_utf8_byte_t* cursor_p = (lit_utf8_byte_t*) buffer;
  lit_utf8_byte_t c;

  while (*cursor_p != '\0')
  {
    c = *cursor_p;

    if (c <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
    {
      *cursor_p = (lit_utf8_byte_t) lit_char_to_lower_case (c, NULL);
    }

    cursor_p++;
  }

  for (jjs_size_t i = 0; i < len; i++)
  {
    if (strcmp (buffer, option_mappings_p[i].name_sz) == 0)
    {
      *out_p = option_mappings_p[i].value;
      return true;
    }
  }

  return false;
} /* jjs_util_map_option */

#if JJS_SCRATCH_ARENA

typedef struct
{
  void* start_p;
  void* next_p;
  jjs_size_t remaining;
  jjs_size_t size;
} jjs_util_arena_allocator_header_t;

static const jjs_size_t ARENA_HEADER_SIZE = (jjs_size_t) JJS_ALIGNUP (sizeof (jjs_util_arena_allocator_header_t), JMEM_ALIGNMENT);

static void*
jjs_util_arena_allocator_alloc (void *internal_p, uint32_t size)
{
  jjs_util_arena_allocator_header_t* header_p = internal_p;
  jjs_size_t aligned_size = JJS_ALIGNUP (size, JMEM_ALIGNMENT);

  if (aligned_size == 0 || aligned_size >= header_p->remaining)
  {
    return NULL;
  }

  void* result = header_p->next_p;

  header_p->next_p = ((uint8_t*) result) + aligned_size;
  header_p->remaining -= aligned_size;

  return result;
}

static void
jjs_util_arena_allocator_free (void *internal_p, void* p, uint32_t size)
{
  JJS_UNUSED_ALL (internal_p, size, p);
}

/**
 * Create a new arena allocator.
 *
 * The arena implementation is backed by a single block of memory. Allocation requests use the block until
 * the available space is exhausted. The allocator has a free api, but it is a no-op. The expected usage
 * is to make allocations and at the end of the operation, reset the arena. This implementation is not
 * intended to be a general purpose arena allocator.
 */
jjs_allocator_t
jjs_util_arena_allocator (void* block_p, jjs_size_t block_size)
{
  static const jjs_allocator_vtab_t vtab = {
    .alloc = jjs_util_arena_allocator_alloc,
    .free = jjs_util_arena_allocator_free,
    .free_self = NULL,
  };

  jjs_util_arena_allocator_header_t* header = block_p;

  header->start_p = ((uint8_t*) block_p) + ARENA_HEADER_SIZE;
  header->next_p = header->start_p;
  header->size = JJS_ALIGNUP (block_size - ARENA_HEADER_SIZE, JMEM_ALIGNMENT);
  header->remaining = header->size;

  return (jjs_allocator_t){
    .vtab_p = &vtab,
    .internal_p = header,
  };
}

/**
 * Drop all arena allocations.
 */
static void
jjs_util_arena_allocator_reset (jjs_allocator_t* allocator_p)
{
  jjs_util_arena_allocator_header_t* header = allocator_p->internal_p;

  header->next_p = header->start_p;
  header->remaining = header->size;
}

static const jjs_size_t SCRATCH_ALLOCATION_HEADER_SIZE = (jjs_size_t) JJS_ALIGNUP (sizeof (jjs_allocator_t*), JMEM_ALIGNMENT);

static void*
jjs_util_scratch_allocator_alloc (void *internal_p, uint32_t size)
{
  void* block;
  jjs_context_t *context_p = internal_p;
  jjs_allocator_t* arena = &context_p->scratch_arena_allocator;

  block = jjs_allocator_alloc (arena, SCRATCH_ALLOCATION_HEADER_SIZE + size);

  if (block)
  {
    *((jjs_allocator_t**) block) = NULL;
    return ((uint8_t*) block) + SCRATCH_ALLOCATION_HEADER_SIZE;
  }

  jjs_allocator_t* fallback = &context_p->scratch_fallback_allocator;

  block = jjs_allocator_alloc (fallback, SCRATCH_ALLOCATION_HEADER_SIZE + size);

  if (block)
  {
    *((jjs_allocator_t**) block) = fallback;
    return ((uint8_t*) block) + SCRATCH_ALLOCATION_HEADER_SIZE;
  }

  return NULL;
}

static void
jjs_util_scratch_allocator_free (void *internal_p, void* p, uint32_t size)
{
  jjs_allocator_t* allocator_p = *((jjs_allocator_t**) (((uint8_t*) p) - SCRATCH_ALLOCATION_HEADER_SIZE));

#if !defined (JJS_NDEBUG)
  jjs_context_t *context_p = internal_p;
  JJS_ASSERT (allocator_p == NULL || allocator_p == &context_p->scratch_fallback_allocator);
#else /* defined (JJS_NDEBUG) */
  JJS_UNUSED (internal_p);
#endif /* !defined (JJS_NDEBUG) */

  if (allocator_p)
  {
    jjs_allocator_free (allocator_p,
                        ((uint8_t*) p) - SCRATCH_ALLOCATION_HEADER_SIZE,
                        size + SCRATCH_ALLOCATION_HEADER_SIZE);
  }
}

/**
 * Create a new scratch allocator for internal temporary allocations.
 *
 * The scratch allocator multiplexes jjs_context_t.scratch_arena_allocator
 * and jjs_context_t.scratch_fallback_allocator. For an allocation, the arena allocator will be tried. If that
 * fails, the fallback allocator is used.
 *
 * Each allocated block has a pointer size header indicating the allocator that made the allocation. The header is
 * used to determine which allocator gets a free call.
 */
jjs_allocator_t
jjs_util_scratch_allocator (jjs_context_t *context_p)
{
  static const jjs_allocator_vtab_t vtab = {
    .alloc = jjs_util_scratch_allocator_alloc,
    .free = jjs_util_scratch_allocator_free,
    .free_self = NULL,
  };

  return (jjs_allocator_t) {
    .vtab_p = &vtab,
    .internal_p = context_p,
  };
}

#endif /* JJS_SCRATCH_ARENA */

static void*
jjs_util_system_allocator_alloc (void* internal_p, uint32_t size)
{
  JJS_UNUSED_ALL (internal_p);
  return malloc (size);
}

static void
jjs_util_system_allocator_free (void* internal_p, void* p, uint32_t size)
{
  JJS_UNUSED_ALL (internal_p, size);
  free (p);
}

jjs_allocator_t
jjs_util_system_allocator (void)
{
  static const jjs_allocator_vtab_t vtab = {
    .alloc = jjs_util_system_allocator_alloc,
    .free = jjs_util_system_allocator_free,
    .free_self = NULL,
  };

  return (jjs_allocator_t) {
    .vtab_p = &vtab,
    .internal_p = NULL,
  };
}

static void*
jjs_util_vm_allocator_alloc (void *internal_p, uint32_t size)
{
  JJS_ASSERT (internal_p);
  return jjs_heap_alloc (internal_p, size);
}

static void
jjs_util_vm_allocator_free (void *internal_p, void* p, uint32_t size)
{
  JJS_ASSERT (internal_p);
  jjs_heap_free (internal_p, p, size);
}

jjs_allocator_t
jjs_util_vm_allocator (jjs_context_t* context_p)
{
  static const jjs_allocator_vtab_t vtab = {
    .alloc = jjs_util_vm_allocator_alloc,
    .free = jjs_util_vm_allocator_free,
    .free_self = NULL,
  };

  return (jjs_allocator_t) {
    .vtab_p = &vtab,
    .internal_p = context_p,
  };
}

static void*
jjs_util_arraybuffer_allocator_alloc (void* internal_p, uint32_t size)
{
  jjs_value_with_context_t *buffer_container_p = internal_p;
  JJS_ASSERT (buffer_container_p->value == 0 || buffer_container_p->value == ECMA_VALUE_UNDEFINED);
  jjs_context_t *context_p = buffer_container_p->context_p;
  JJS_ASSERT (context_p);
  jjs_value_t value = jjs_arraybuffer (context_p, size);
  void* value_data_p;

  if (jjs_value_is_exception (context_p, value))
  {
    return false;
  }

  value_data_p = jjs_arraybuffer_data (context_p, value);

  if (value_data_p == NULL)
  {
    jjs_value_free (context_p, value);
    return false;
  }

  buffer_container_p->value = value;

  return value_data_p;
}

static void
jjs_util_arraybuffer_allocator_free (void *internal_p, void* p, uint32_t size)
{
  JJS_UNUSED_ALL (size);

  jjs_value_with_context_t *buffer_container_p = internal_p;
  jjs_context_t *context_p = buffer_container_p->context_p;

  JJS_ASSERT (context_p);

  if (jjs_arraybuffer_data (context_p, buffer_container_p->value) == p)
  {
    jjs_value_free (context_p, buffer_container_p->value);
    buffer_container_p->value = ECMA_VALUE_UNDEFINED;
  }
}

/**
 * Special allocator for file reads.
 *
 * This allocator is for performance. Instead of allocating a buffer, reading file contents to the buffer and
 * copying the buffer to a JS ArrayBuffer, this allocator will allocate an ArrayBuffer directly and return
 * its memory pointer. The file contents can be written into this buffer, skipping an extra copy.
 *
 * The allocator assumes it will get a single alloc() call. On this call, the JS ArrayBuffer is created
 * and allocated in the VM. A subsequent call to alloc() will fail. The user can retrieve the buffer from
 * the jjs_value_with_context object.
 */
jjs_allocator_t
jjs_util_arraybuffer_allocator (jjs_value_with_context_t *value_p)
{
  static const jjs_allocator_vtab_t vtab = {
    .alloc = jjs_util_arraybuffer_allocator_alloc,
    .free = jjs_util_arraybuffer_allocator_free,
    .free_self = NULL,
  };

  return (jjs_allocator_t) {
    .vtab_p = &vtab,
    .internal_p = value_p,
  };
}

/**
 * Acquire exclusive access to the scratch allocator.
 */
jjs_allocator_t*
jjs_util_context_acquire_scratch_allocator (jjs_context_t* context_p)
{
  return &context_p->scratch_allocator;
}

/**
 * Release the scratch allocator. If the scratch arena allocator is enabled, all
 * of its allocations are dropped.
 */
void
jjs_util_context_release_scratch_allocator (jjs_context_t* context_p)
{
#if JJS_SCRATCH_ARENA_SIZE > 0
  jjs_util_arena_allocator_reset (&context_p->scratch_arena_allocator);
#else /* !(JJS_SCRATCH_ARENA_SIZE > 0) */
  JJS_UNUSED (context_p);
#endif /* JJS_SCRATCH_ARENA_SIZE > 0 */
}

/**
 * Convert from one text encoding to another.
 */
jjs_status_t
jjs_util_convert (jjs_allocator_t* allocator,
                  const uint8_t* source_p,
                  jjs_size_t source_size,
                  jjs_encoding_t source_encoding,
                  void** dest_p,
                  jjs_size_t* dest_size,
                  jjs_encoding_t dest_encoding,
                  bool add_null_terminator,
                  bool add_windows_long_filename_prefix)
{
  JJS_UNUSED_ALL (allocator, source_p, source_size, source_encoding, dest_encoding, dest_p, dest_size);
  jjs_size_t extra_bytes = 0;

  if (add_null_terminator)
  {
    extra_bytes++;
  }

  if (add_windows_long_filename_prefix)
  {
    //    extra_bytes += WINDOWS_LONG_FILENAME_PREFIX_LEN;
  }

  switch (source_encoding)
  {
    case JJS_ENCODING_ASCII:
    {
      switch (dest_encoding)
      {
        case JJS_ENCODING_UTF8:
        case JJS_ENCODING_ASCII:
        {
          jjs_size_t allocated_size = (source_size + extra_bytes);
          uint8_t* utf8 = jjs_allocator_alloc (allocator, allocated_size);

          if (!utf8)
          {
            return JJS_STATUS_BAD_ALLOC;
          }

          memcpy (utf8, source_p, source_size);

          if (add_null_terminator)
          {
            utf8[source_size] = '\0';
          }

          *dest_p = utf8;
          *dest_size = allocated_size;
          return JJS_STATUS_OK;
        }
        case JJS_ENCODING_UTF16:
        {
          jjs_size_t allocated_size = (source_size + extra_bytes) * sizeof (uint16_t);
          uint16_t* utf16le = jjs_allocator_alloc (allocator, allocated_size);

          if (!utf16le)
          {
            return JJS_STATUS_BAD_ALLOC;
          }

          for (jjs_size_t i = 0; i < source_size; i++)
          {
            utf16le[i] = source_p[i];
          }

          if (add_null_terminator)
          {
            utf16le[source_size] = L'\0';
          }

          *dest_p = utf16le;
          *dest_size = allocated_size;
          return JJS_STATUS_OK;
        }
        default:
        {
          break;
        }
      }
      break;
    }
    case JJS_ENCODING_CESU8:
    {
      switch (dest_encoding)
      {
        case JJS_ENCODING_UTF8:
        {
          lit_utf8_size_t utf8_size = lit_get_utf8_size_of_cesu8_string (source_p, source_size);

          if (utf8_size == 0)
          {
            return JJS_STATUS_MALFORMED_CESU8;
          }

          lit_utf8_size_t allocated_size = source_size + extra_bytes;
          lit_utf8_byte_t* utf8_p = jjs_allocator_alloc (allocator, allocated_size);

          if (utf8_p == NULL)
          {
            return JJS_STATUS_BAD_ALLOC;
          }

          lit_utf8_size_t written = lit_convert_cesu8_string_to_utf8_string (source_p, source_size, utf8_p, utf8_size);

          if (written != utf8_size)
          {
            jjs_allocator_free (allocator, utf8_p, allocated_size);
            return JJS_STATUS_MALFORMED_CESU8;
          }

          if (add_null_terminator)
          {
            utf8_p[utf8_size] = '\0';
          }

          *dest_p = utf8_p;
          *dest_size = allocated_size;

          return JJS_STATUS_OK;
        }
        case JJS_ENCODING_UTF16:
        {
          lit_utf8_size_t advance;
          ecma_char_t ch;
          ecma_char_t* result;
          ecma_char_t* result_cursor;
          lit_utf8_size_t result_size = 0;
          lit_utf8_size_t index = 0;

          while (lit_peek_wchar_from_cesu8 (source_p, source_size, index, &advance, &ch))
          {
            result_size++;
            index += advance;
          }

          jjs_size_t allocated_size = (result_size + extra_bytes) * sizeof (ecma_char_t);

          result = result_cursor = jjs_allocator_alloc (allocator, allocated_size);

          if (result == NULL)
          {
            return JJS_STATUS_BAD_ALLOC;
          }

          index = 0;

          while (lit_peek_wchar_from_cesu8 (source_p, source_size, index, &advance, &ch))
          {
            index += advance;
            *result_cursor++ = ch;
          }

          if (add_null_terminator)
          {
            *result_cursor = L'\0';
          }

          *dest_p = result;
          *dest_size = allocated_size;

          return JJS_STATUS_OK;
        }
        default:
        {
          break;
        }
      }
      break;
    }
    default:
      break;
  }

  return JJS_STATUS_UNSUPPORTED_ENCODING;
}

void
jjs_util_promise_unhandled_rejection_default (jjs_context_t* context_p,
                                              jjs_value_t promise,
                                              jjs_value_t reason,
                                              void *user_p)
{
  JJS_UNUSED_ALL (context_p, promise, reason, user_p);
#if JJS_LOGGING
  jjs_log_fmt (context_p, JJS_LOG_LEVEL_ERROR, "Uncaught:\n{}\n", reason);
#endif
}
