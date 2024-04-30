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

typedef struct
{
  void* start_p;
  void* next_p;
  jjs_size_t remaining;
  jjs_size_t size;
} jjs_util_arena_allocator_header_t;

static const jjs_size_t ARENA_HEADER_SIZE = (jjs_size_t) JJS_ALIGNUP (sizeof (jjs_util_arena_allocator_header_t), JMEM_ALIGNMENT);

static void*
jjs_util_arena_allocator_alloc (jjs_allocator_t* self_p, uint32_t size)
{
  jjs_util_arena_allocator_header_t* header_p = self_p->internal[0];
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
jjs_util_arena_allocator_free (jjs_allocator_t* self_p, void* p, uint32_t size)
{
  JJS_UNUSED_ALL (self_p, size, p);
}

/**
 * Init a new arena allocator.
 *
 * The arena implementation is very simple. Allocations follow each other in the buffer. If there is
 * no more room, the allocation fails. free() does nothing. All memory is "freed" when reset is called
 * to rewind the arena buffer pointer to the beginning.
 */
static bool
jjs_util_arena_allocator_init (void* block_p, jjs_size_t block_size, jjs_allocator_t* allocator_p)
{
  jjs_util_arena_allocator_header_t* header = block_p;

  header->start_p = ((uint8_t*) block_p) + ARENA_HEADER_SIZE;
  header->next_p = header->start_p;
  header->size = JJS_ALIGNUP (block_size - ARENA_HEADER_SIZE, JMEM_ALIGNMENT);
  header->remaining = header->size;

  *allocator_p = (jjs_allocator_t){
    .alloc = jjs_util_arena_allocator_alloc,
    .free = jjs_util_arena_allocator_free,
    .internal = { block_p },
  };

  return true;
}

/**
 * Drop all arena allocations.
 */
static void
jjs_util_arena_allocator_reset (jjs_allocator_t* allocator_p)
{
  jjs_util_arena_allocator_header_t* header = allocator_p->internal[0];

  header->next_p = header->start_p;
  header->remaining = header->size;
}

static const jjs_size_t COMPOUND_ALLOCATION_HEADER_SIZE = (jjs_size_t) JJS_ALIGNUP (sizeof (jjs_allocator_t*), JMEM_ALIGNMENT);

static void*
jjs_util_compound_allocator_alloc (jjs_allocator_t* self_p, uint32_t size)
{
  void* block;
  jjs_allocator_t* arena = self_p->internal[0];

  block = arena->alloc (arena, COMPOUND_ALLOCATION_HEADER_SIZE + size);

  if (block)
  {
    *((jjs_allocator_t**) block) = arena;

    return ((uint8_t*) block) + COMPOUND_ALLOCATION_HEADER_SIZE;
  }

  jjs_allocator_t* fallback = self_p->internal[1];

  block = fallback->alloc (fallback, COMPOUND_ALLOCATION_HEADER_SIZE + size);

  if (block)
  {
    *((jjs_allocator_t**) block) = fallback;
    return ((uint8_t*) block) + COMPOUND_ALLOCATION_HEADER_SIZE;
  }

  return NULL;
}

static void
jjs_util_compound_allocator_free (jjs_allocator_t* self_p, void* p, uint32_t size)
{
  jjs_allocator_t* allocator = *((jjs_allocator_t**) (((uint8_t*) p) - COMPOUND_ALLOCATION_HEADER_SIZE));

  if (self_p->internal[1] == allocator)
  {
    allocator->free (allocator, ((uint8_t*) p) - COMPOUND_ALLOCATION_HEADER_SIZE, size + COMPOUND_ALLOCATION_HEADER_SIZE);
  }
}

/**
 * Combines an arena allocator with a fallback allocator.
 *
 * The allocator tries to alloc to the arena first. If that fails, the fallback allocator
 * is used. Each allocation has an extra sizeof (uintptr_t) header indicating which
 * allocator was responsible for the alloc. On free, the pointer is adjusted so the
 * freeing allocator can be called.
 *
 * The header could probably be represented more compactly, but the compound allocator is
 * used for the scratch allocator. Right now, the usage is maybe 1-3 allocations before the
 * scratch allocator is reset, so the extra memory is not an issue.
 */
static jjs_allocator_t
jjs_util_compound_allocator (jjs_allocator_t* arena, jjs_allocator_t* fallback)
{
  if (arena == NULL)
  {
    return *fallback;
  }

  return (jjs_allocator_t){
    .alloc = jjs_util_compound_allocator_alloc,
    .free = jjs_util_compound_allocator_free,
    .internal[0] = arena,
    .internal[1] = fallback,
  };
}

static void*
jjs_util_system_allocator_alloc (jjs_allocator_t* self_p, uint32_t size)
{
  JJS_UNUSED_ALL (self_p);
  return malloc (size);
}

static void
jjs_util_system_allocator_free (jjs_allocator_t* self_p, void* p, uint32_t size)
{
  JJS_UNUSED_ALL (self_p, size);
  free (p);
}

jjs_allocator_t
jjs_util_system_allocator (void)
{
  return *jjs_util_system_allocator_ptr ();
}

jjs_allocator_t *
jjs_util_system_allocator_ptr (void)
{
  static jjs_allocator_t system_allocator = {
    .alloc = jjs_util_system_allocator_alloc,
    .free = jjs_util_system_allocator_free,
  };

  return &system_allocator;
}

static void*
jjs_util_vm_allocator_alloc (jjs_allocator_t* self_p, uint32_t size)
{
  JJS_ASSERT (self_p->internal[0]);
  return jjs_heap_alloc (self_p->internal[0], size);
}

static void
jjs_util_vm_allocator_free (jjs_allocator_t* self_p, void* p, uint32_t size)
{
  JJS_ASSERT (self_p->internal[0]);
  jjs_heap_free (self_p->internal[0], p, size);
}

jjs_allocator_t
jjs_util_vm_allocator (jjs_context_t* context_p)
{
  return (jjs_allocator_t){
    .alloc = jjs_util_vm_allocator_alloc,
    .free = jjs_util_vm_allocator_free,
    .internal[0] = context_p,
  };
}

static void*
jjs_util_arraybuffer_allocator_alloc (jjs_allocator_t* self_p, uint32_t size)
{
  if (self_p->internal[0] != NULL)
  {
    return NULL;
  }

  jjs_context_t *context_p = self_p->internal[2];
  jjs_value_t value = jjs_arraybuffer (context_p, size);

  if (jjs_value_is_exception (context_p, value))
  {
    return false;
  }

  void* value_data_p = jjs_arraybuffer_data (context_p, value);

  if (value_data_p == NULL)
  {
    jjs_value_free (context_p, value);
    return false;
  }

  self_p->internal[0] = value_data_p;
  self_p->internal[1] = (void*) (uintptr_t) value;

  return value_data_p;
}

static void
jjs_util_arraybuffer_allocator_free (jjs_allocator_t* self_p, void* p, uint32_t size)
{
  JJS_UNUSED_ALL (size);

  if (self_p->internal[0] == p)
  {
    jjs_value_free (self_p->internal[2], (jjs_value_t) (uintptr_t) self_p->internal[1]);
    self_p->internal[0] = self_p->internal[1] = NULL;
  }
}

/**
 * Special allocator for file reads.
 *
 * This allocator is for performance. Instead of allocating a buffer, reading file contents to the buffer and
 * copying the buffer to a JS ArrayBuffer, this allocator will allocate an ArrayBuffer directly and return
 * its memory pointer. The file contents can be written into this buffer, skipping an extra copy.
 *
 * This allocator assumes that the file will be read in one shot.
 *
 * jjs_util_arraybuffer_allocator_move() exists to extract the ArrayBuffer from the allocator.
 */
jjs_allocator_t
jjs_util_arraybuffer_allocator (jjs_context_t* context_p)
{
  return (jjs_allocator_t){
    .alloc = jjs_util_arraybuffer_allocator_alloc,
    .free = jjs_util_arraybuffer_allocator_free,
    .internal = { NULL, NULL, context_p },
  };
}

jjs_value_t
jjs_util_arraybuffer_allocator_move (jjs_allocator_t* arraybuffer_allocator)
{
  if (arraybuffer_allocator->internal[0] != NULL)
  {
    jjs_value_t result = (jjs_value_t) (uintptr_t) arraybuffer_allocator->internal[1];

    arraybuffer_allocator->internal[0] = arraybuffer_allocator->internal[1] = NULL;

    return result;
  }

  return ECMA_VALUE_UNDEFINED;
}

/**
 * Initialize the allocators in the context.
 */
bool
jjs_util_context_allocator_init (jjs_context_t* context_p, const jjs_allocator_t* fallback_allocator)
{
  jjs_allocator_t* scratch_arena_allocator = NULL;

#if JJS_SCRATCH_ARENA_SIZE > 0
  if (jjs_util_arena_allocator_init (&context_p->scratch_arena_block,
                                     JJS_SCRATCH_ARENA_SIZE * 1024,
                                     &context_p->scratch_arena_allocator))
  {
    scratch_arena_allocator = &context_p->scratch_arena_allocator;
  }
  else
  {
    return false;
  }
#endif /* JJS_SCRATCH_ARENA_SIZE > 0 */

  context_p->fallback_scratch_allocator = *fallback_allocator;

  if (scratch_arena_allocator)
  {
    context_p->scratch_allocator =
      jjs_util_compound_allocator (scratch_arena_allocator, &context_p->fallback_scratch_allocator);
  }

  return true;
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
          uint8_t* utf8 = allocator->alloc (allocator, allocated_size);

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
          uint16_t* utf16le = allocator->alloc (allocator, allocated_size);

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
          lit_utf8_byte_t* utf8_p = allocator->alloc (allocator, allocated_size);

          if (utf8_p == NULL)
          {
            return JJS_STATUS_BAD_ALLOC;
          }

          lit_utf8_size_t written = lit_convert_cesu8_string_to_utf8_string (source_p, source_size, utf8_p, utf8_size);

          if (written != utf8_size)
          {
            allocator->free (allocator, utf8_p, allocated_size);
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

          result = result_cursor = allocator->alloc (allocator, allocated_size);

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
