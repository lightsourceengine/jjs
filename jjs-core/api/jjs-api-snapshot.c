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

#include "jjs-api-snapshot.h"
#include "jjs.h"

#include "ecma-conversion.h"
#include "ecma-errors.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-literal-storage.h"

#include "jcontext.h"
#include "js-parser-internal.h"
#include "js-parser.h"
#include "lit-char-helpers.h"
#include "re-compiler.h"

#if JJS_SNAPSHOT_SAVE || JJS_SNAPSHOT_EXEC

/**
 * Get snapshot configuration flags.
 *
 * @return configuration flags
 */
static inline uint32_t JJS_ATTR_ALWAYS_INLINE
snapshot_get_global_flags (bool has_regex, /**< regex literal is present */
                           bool has_class) /**< class literal is present */
{
  JJS_UNUSED (has_regex);
  JJS_UNUSED (has_class);

  uint32_t flags = 0;

#if JJS_BUILTIN_REGEXP
  flags |= (has_regex ? JJS_SNAPSHOT_HAS_REGEX_LITERAL : 0);
#endif /* JJS_BUILTIN_REGEXP */

  flags |= (has_class ? JJS_SNAPSHOT_HAS_CLASS_LITERAL : 0);

  return flags;
} /* snapshot_get_global_flags */

/**
 * Checks whether the global_flags argument matches to the current feature set.
 *
 * @return true if global_flags accepted, false otherwise
 */
static inline bool JJS_ATTR_ALWAYS_INLINE
snapshot_check_global_flags (uint32_t global_flags) /**< global flags */
{
#if JJS_BUILTIN_REGEXP
  global_flags &= (uint32_t) ~JJS_SNAPSHOT_HAS_REGEX_LITERAL;
#endif /* JJS_BUILTIN_REGEXP */

  global_flags &= (uint32_t) ~JJS_SNAPSHOT_HAS_CLASS_LITERAL;

  return global_flags == snapshot_get_global_flags (false, false);
} /* snapshot_check_global_flags */

#endif /* JJS_SNAPSHOT_SAVE || JJS_SNAPSHOT_EXEC */

#if JJS_SNAPSHOT_SAVE

/**
 * Variables required to take a snapshot.
 */
typedef struct
{
  size_t snapshot_buffer_write_offset;
  ecma_value_t snapshot_error;
  bool regex_found;
  bool class_found;
} snapshot_globals_t;

/** \addtogroup jjssnapshot JJS snapshot operations
 * @{
 */

/**
 * Write data into the specified buffer.
 *
 * Note:
 *      Offset is in-out and is incremented if the write operation completes successfully.
 *
 * @return true - if write was successful, i.e. offset + data_size doesn't exceed buffer size,
 *         false - otherwise
 */
static inline bool JJS_ATTR_ALWAYS_INLINE
snapshot_write_to_buffer_by_offset (uint8_t *buffer_p, /**< buffer */
                                    size_t buffer_size, /**< size of buffer */
                                    size_t *in_out_buffer_offset_p, /**< [in,out] offset to write to
                                                                     * incremented with data_size */
                                    const void *data_p, /**< data */
                                    size_t data_size) /**< size of the writable data */
{
  if (*in_out_buffer_offset_p + data_size > buffer_size)
  {
    return false;
  }

  memcpy (buffer_p + *in_out_buffer_offset_p, data_p, data_size);
  *in_out_buffer_offset_p += data_size;

  return true;
} /* snapshot_write_to_buffer_by_offset */

/**
 * Maximum snapshot write buffer offset.
 */
#if !JJS_NUMBER_TYPE_FLOAT64
#define JJS_SNAPSHOT_MAXIMUM_WRITE_OFFSET (0x7fffff >> 1)
#else /* JJS_NUMBER_TYPE_FLOAT64 */
#define JJS_SNAPSHOT_MAXIMUM_WRITE_OFFSET (UINT32_MAX >> 1)
#endif /* !JJS_NUMBER_TYPE_FLOAT64 */

/**
 * Save snapshot helper.
 *
 * @return start offset
 */
static uint32_t
snapshot_add_compiled_code (jjs_context_t* context_p, /**< JJS context */
                            const ecma_compiled_code_t *compiled_code_p, /**< compiled code */
                            uint8_t *snapshot_buffer_p, /**< snapshot buffer */
                            size_t snapshot_buffer_size, /**< snapshot buffer size */
                            snapshot_globals_t *globals_p) /**< snapshot globals */
{
  const char *error_buffer_too_small_p = "Snapshot buffer too small";

  if (!ecma_is_value_empty (globals_p->snapshot_error))
  {
    return 0;
  }

  JJS_ASSERT ((globals_p->snapshot_buffer_write_offset & (JMEM_ALIGNMENT - 1)) == 0);

  if (globals_p->snapshot_buffer_write_offset > JJS_SNAPSHOT_MAXIMUM_WRITE_OFFSET)
  {
    globals_p->snapshot_error = jjs_throw_sz (context_p, JJS_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_MAXIMUM_SNAPSHOT_SIZE));
    return 0;
  }

  /* The snapshot generator always parses a single file,
   * so the base always starts right after the snapshot header. */
  uint32_t start_offset = (uint32_t) (globals_p->snapshot_buffer_write_offset - sizeof (jjs_snapshot_header_t));

  uint8_t *copied_code_start_p = snapshot_buffer_p + globals_p->snapshot_buffer_write_offset;
  ecma_compiled_code_t *copied_code_p = (ecma_compiled_code_t *) copied_code_start_p;

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_HAS_TAGGED_LITERALS)
  {
    globals_p->snapshot_error =
      jjs_throw_sz (context_p, JJS_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_TAGGED_TEMPLATE_LITERALS));
    return 0;
  }

  if (CBC_FUNCTION_GET_TYPE (compiled_code_p->status_flags) == CBC_FUNCTION_CONSTRUCTOR)
  {
    globals_p->class_found = true;
  }

#if JJS_BUILTIN_REGEXP
  if (!CBC_IS_FUNCTION (compiled_code_p->status_flags))
  {
    /* Regular expression. */
    if (globals_p->snapshot_buffer_write_offset + sizeof (ecma_compiled_code_t) > snapshot_buffer_size)
    {
      globals_p->snapshot_error = jjs_throw_sz (context_p, JJS_ERROR_RANGE, error_buffer_too_small_p);
      return 0;
    }

    globals_p->snapshot_buffer_write_offset += sizeof (ecma_compiled_code_t);

    ecma_value_t pattern = ((re_compiled_code_t *) compiled_code_p)->source;
    ecma_string_t *pattern_string_p = ecma_get_string_from_value (context_p, pattern);

    lit_utf8_size_t pattern_size = 0;

    ECMA_STRING_TO_UTF8_STRING (context_p, pattern_string_p, buffer_p, buffer_size);

    pattern_size = buffer_size;

    if (!snapshot_write_to_buffer_by_offset (snapshot_buffer_p,
                                             snapshot_buffer_size,
                                             &globals_p->snapshot_buffer_write_offset,
                                             buffer_p,
                                             buffer_size))
    {
      globals_p->snapshot_error = jjs_throw_sz (context_p, JJS_ERROR_RANGE, error_buffer_too_small_p);
      /* cannot return inside ECMA_FINALIZE_UTF8_STRING */
    }

    ECMA_FINALIZE_UTF8_STRING (context_p, buffer_p, buffer_size);

    if (!ecma_is_value_empty (globals_p->snapshot_error))
    {
      return 0;
    }

    globals_p->regex_found = true;
    globals_p->snapshot_buffer_write_offset = JJS_ALIGNUP (globals_p->snapshot_buffer_write_offset, JMEM_ALIGNMENT);

    /* Regexp character size is stored in refs. */
    copied_code_p->refs = (uint16_t) pattern_size;

    pattern_size += (lit_utf8_size_t) sizeof (ecma_compiled_code_t);
    copied_code_p->size = (uint16_t) ((pattern_size + JMEM_ALIGNMENT - 1) >> JMEM_ALIGNMENT_LOG);

    copied_code_p->status_flags = compiled_code_p->status_flags;

    return start_offset;
  }
#endif /* JJS_BUILTIN_REGEXP */

  JJS_ASSERT (CBC_IS_FUNCTION (compiled_code_p->status_flags));

  if (!snapshot_write_to_buffer_by_offset (snapshot_buffer_p,
                                           snapshot_buffer_size,
                                           &globals_p->snapshot_buffer_write_offset,
                                           compiled_code_p,
                                           ((size_t) compiled_code_p->size) << JMEM_ALIGNMENT_LOG))
  {
    globals_p->snapshot_error = jjs_throw_sz (context_p, JJS_ERROR_RANGE, error_buffer_too_small_p);
    return 0;
  }

  /* Sub-functions and regular expressions are stored recursively. */
  uint8_t *buffer_p = (uint8_t *) copied_code_p;
  ecma_value_t *literal_start_p;
  uint32_t const_literal_end;
  uint32_t literal_end;

#if JJS_LINE_INFO
  /* disable line info in the snapshot, but the line info slot space will remain */
  copied_code_p->status_flags &= (uint16_t) ~CBC_CODE_FLAGS_USING_LINE_INFO;
#endif /* JJS_LINE_INFO */

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    literal_start_p = (ecma_value_t *) (buffer_p + sizeof (cbc_uint16_arguments_t));

    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) buffer_p;
    literal_end = (uint32_t) (args_p->literal_end - args_p->register_end);
    const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);
  }
  else
  {
    literal_start_p = (ecma_value_t *) (buffer_p + sizeof (cbc_uint8_arguments_t));

    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) buffer_p;
    literal_end = (uint32_t) (args_p->literal_end - args_p->register_end);
    const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);
  }

  for (uint32_t i = const_literal_end; i < literal_end; i++)
  {
    ecma_compiled_code_t *bytecode_p = ECMA_GET_INTERNAL_VALUE_POINTER (context_p, ecma_compiled_code_t, literal_start_p[i]);

    if (bytecode_p == compiled_code_p)
    {
      literal_start_p[i] = 0;
    }
    else
    {
      uint32_t offset = snapshot_add_compiled_code (context_p, bytecode_p, snapshot_buffer_p, snapshot_buffer_size, globals_p);

      JJS_ASSERT (!ecma_is_value_empty (globals_p->snapshot_error) || offset > start_offset);

      literal_start_p[i] = offset - start_offset;
    }
  }

  return start_offset;
} /* snapshot_add_compiled_code */

/**
 * Create unsupported literal error.
 */
static void
static_snapshot_error_unsupported_literal (jjs_context_t *context_p, /**< JJS context */
                                           snapshot_globals_t *globals_p, /**< snapshot globals */
                                           ecma_value_t literal) /**< literal form the literal pool */
{
  lit_utf8_byte_t *str_p = (lit_utf8_byte_t *) "Unsupported static snapshot literal: ";
  ecma_stringbuilder_t builder = ecma_stringbuilder_create_raw (context_p, str_p, 37);

  JJS_ASSERT (!ECMA_IS_VALUE_ERROR (literal));

  ecma_string_t *literal_string_p = ecma_op_to_string (context_p, literal);
  JJS_ASSERT (literal_string_p != NULL);

  ecma_stringbuilder_append (&builder, literal_string_p);

  ecma_deref_ecma_string (context_p, literal_string_p);

  ecma_object_t *error_object_p = ecma_new_standard_error (context_p, JJS_ERROR_RANGE, ecma_stringbuilder_finalize (&builder));

  globals_p->snapshot_error = ecma_create_exception_from_object (context_p, error_object_p);
} /* static_snapshot_error_unsupported_literal */

/**
 * Save static snapshot helper.
 *
 * @return start offset
 */
static uint32_t
static_snapshot_add_compiled_code (jjs_context_t* context_p, /**< JJS context */
                                   const ecma_compiled_code_t *compiled_code_p, /**< compiled code */
                                   uint8_t *snapshot_buffer_p, /**< snapshot buffer */
                                   size_t snapshot_buffer_size, /**< snapshot buffer size */
                                   snapshot_globals_t *globals_p) /**< snapshot globals */
{
  if (!ecma_is_value_empty (globals_p->snapshot_error))
  {
    return 0;
  }

  JJS_ASSERT ((globals_p->snapshot_buffer_write_offset & (JMEM_ALIGNMENT - 1)) == 0);

  if (globals_p->snapshot_buffer_write_offset >= JJS_SNAPSHOT_MAXIMUM_WRITE_OFFSET)
  {
    globals_p->snapshot_error = jjs_throw_sz (context_p, JJS_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_MAXIMUM_SNAPSHOT_SIZE));
    return 0;
  }

  /* The snapshot generator always parses a single file,
   * so the base always starts right after the snapshot header. */
  uint32_t start_offset = (uint32_t) (globals_p->snapshot_buffer_write_offset - sizeof (jjs_snapshot_header_t));

  uint8_t *copied_code_start_p = snapshot_buffer_p + globals_p->snapshot_buffer_write_offset;
  ecma_compiled_code_t *copied_code_p = (ecma_compiled_code_t *) copied_code_start_p;

  if (!CBC_IS_FUNCTION (compiled_code_p->status_flags))
  {
    /* Regular expression literals are not supported. */
    globals_p->snapshot_error =
      jjs_throw_sz (context_p, JJS_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_REGULAR_EXPRESSION_NOT_SUPPORTED));
    return 0;
  }

  if (!snapshot_write_to_buffer_by_offset (snapshot_buffer_p,
                                           snapshot_buffer_size,
                                           &globals_p->snapshot_buffer_write_offset,
                                           compiled_code_p,
                                           ((size_t) compiled_code_p->size) << JMEM_ALIGNMENT_LOG))
  {
    globals_p->snapshot_error = jjs_throw_sz (context_p, JJS_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_SNAPSHOT_BUFFER_SMALL));
    return 0;
  }

  /* Sub-functions and regular expressions are stored recursively. */
  uint8_t *buffer_p = (uint8_t *) copied_code_p;
  ecma_value_t *literal_start_p;
  uint32_t const_literal_end;
  uint32_t literal_end;

  ((ecma_compiled_code_t *) copied_code_p)->status_flags |= CBC_CODE_FLAGS_STATIC_FUNCTION;

  if (compiled_code_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    literal_start_p = (ecma_value_t *) (buffer_p + sizeof (cbc_uint16_arguments_t));

    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) buffer_p;
    literal_end = (uint32_t) (args_p->literal_end - args_p->register_end);
    const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);

    args_p->script_value = JMEM_CP_NULL;
  }
  else
  {
    literal_start_p = (ecma_value_t *) (buffer_p + sizeof (cbc_uint8_arguments_t));

    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) buffer_p;
    literal_end = (uint32_t) (args_p->literal_end - args_p->register_end);
    const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);

    args_p->script_value = JMEM_CP_NULL;
  }

  for (uint32_t i = 0; i < const_literal_end; i++)
  {
    if (!ecma_is_value_direct (literal_start_p[i]) && !ecma_is_value_direct_string (literal_start_p[i]))
    {
      static_snapshot_error_unsupported_literal (context_p, globals_p, literal_start_p[i]);
      return 0;
    }
  }

  for (uint32_t i = const_literal_end; i < literal_end; i++)
  {
    ecma_compiled_code_t *bytecode_p = ECMA_GET_INTERNAL_VALUE_POINTER (context_p, ecma_compiled_code_t, literal_start_p[i]);

    if (bytecode_p == compiled_code_p)
    {
      literal_start_p[i] = 0;
    }
    else
    {
      uint32_t offset =
        static_snapshot_add_compiled_code (context_p, bytecode_p, snapshot_buffer_p, snapshot_buffer_size, globals_p);

      JJS_ASSERT (!ecma_is_value_empty (globals_p->snapshot_error) || offset > start_offset);

      literal_start_p[i] = offset - start_offset;
    }
  }

  buffer_p += ((size_t) compiled_code_p->size) << JMEM_ALIGNMENT_LOG;
  literal_start_p = ecma_snapshot_resolve_serializable_values (compiled_code_p, buffer_p);

  while (literal_start_p < (ecma_value_t *) buffer_p)
  {
    if (!ecma_is_value_direct_string (*literal_start_p) && !ecma_is_value_empty (*literal_start_p))
    {
      static_snapshot_error_unsupported_literal (context_p, globals_p, *literal_start_p);
      return 0;
    }

    literal_start_p++;
  }

  return start_offset;
} /* static_snapshot_add_compiled_code */

/**
 * Set the uint16_t offsets in the code area.
 */
static void
jjs_snapshot_set_offsets (uint32_t *buffer_p, /**< buffer */
                            uint32_t size, /**< buffer size */
                            lit_mem_to_snapshot_id_map_entry_t *lit_map_p) /**< literal map */
{
  JJS_ASSERT (size > 0);

  do
  {
    ecma_compiled_code_t *bytecode_p = (ecma_compiled_code_t *) buffer_p;
    uint32_t code_size = ((uint32_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG;

    if (CBC_IS_FUNCTION (bytecode_p->status_flags))
    {
      ecma_value_t *literal_start_p;
      uint32_t const_literal_end;

      if (bytecode_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
      {
        literal_start_p = (ecma_value_t *) (((uint8_t *) buffer_p) + sizeof (cbc_uint16_arguments_t));

        cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) buffer_p;
        const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);
      }
      else
      {
        literal_start_p = (ecma_value_t *) (((uint8_t *) buffer_p) + sizeof (cbc_uint8_arguments_t));

        cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) buffer_p;
        const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);
      }

      for (uint32_t i = 0; i < const_literal_end; i++)
      {
        if (ecma_is_value_string (literal_start_p[i])
#if JJS_BUILTIN_BIGINT
            || ecma_is_value_bigint (literal_start_p[i])
#endif /* JJS_BUILTIN_BIGINT */
            || ecma_is_value_float_number (literal_start_p[i]))
        {
          lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

          while (current_p->literal_id != literal_start_p[i])
          {
            current_p++;
          }

          literal_start_p[i] = current_p->literal_offset;
        }
      }

      uint8_t *byte_p = (uint8_t *) bytecode_p + (((size_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG);
      literal_start_p = ecma_snapshot_resolve_serializable_values (bytecode_p, byte_p);

      while (literal_start_p < (ecma_value_t *) byte_p)
      {
        if (*literal_start_p != ECMA_VALUE_EMPTY)
        {
          JJS_ASSERT (ecma_is_value_string (*literal_start_p));

          lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

          while (current_p->literal_id != *literal_start_p)
          {
            current_p++;
          }

          *literal_start_p = current_p->literal_offset;
        }

        literal_start_p++;
      }

      /* Set reference counter to 1. */
      bytecode_p->refs = 1;
    }

    JJS_ASSERT ((code_size % sizeof (uint32_t)) == 0);
    buffer_p += code_size / sizeof (uint32_t);
    size -= code_size;
  } while (size > 0);
} /* jjs_snapshot_set_offsets */

#endif /* JJS_SNAPSHOT_SAVE */

#if JJS_SNAPSHOT_EXEC

/**
 * Byte code blocks shorter than this threshold are always copied into the memory.
 * The memory / performance trade-of of byte code redirection does not worth
 * in such cases.
 */
#define BYTECODE_NO_COPY_THRESHOLD 8

/**
 * Load byte code from snapshot.
 *
 * @return byte code
 */
static ecma_compiled_code_t *
snapshot_load_compiled_code (jjs_context_t* context_p, /**< JJS context */
                             const uint8_t *base_addr_p, /**< base address of the
                                                          *   current primary function */
                             const uint8_t *literal_base_p, /**< literal start */
                             cbc_script_t *script_p, /**< script */
                             bool copy_bytecode) /**< byte code should be copied to memory */
{
  ecma_compiled_code_t *bytecode_p = (ecma_compiled_code_t *) base_addr_p;
  uint32_t code_size = ((uint32_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG;

#if JJS_BUILTIN_REGEXP
  if (!CBC_IS_FUNCTION (bytecode_p->status_flags))
  {
    const uint8_t *regex_start_p = ((const uint8_t *) bytecode_p) + sizeof (ecma_compiled_code_t);

    /* Real size is stored in refs. */
    ecma_string_t *pattern_str_p = ecma_new_ecma_string_from_utf8 (context_p, regex_start_p, bytecode_p->refs);

    const re_compiled_code_t *re_bytecode_p = re_compile_bytecode (context_p, pattern_str_p, bytecode_p->status_flags);
    ecma_deref_ecma_string (context_p, pattern_str_p);

    return (ecma_compiled_code_t *) re_bytecode_p;
  }
#else /* !JJS_BUILTIN_REGEXP */
  JJS_ASSERT (CBC_IS_FUNCTION (bytecode_p->status_flags));
#endif /* JJS_BUILTIN_REGEXP */

  size_t header_size;
  uint32_t argument_end;
  uint32_t const_literal_end;
  uint32_t literal_end;

  if (JJS_UNLIKELY (script_p->refs_and_type >= CBC_SCRIPT_REF_MAX))
  {
    /* This is probably never happens in practice. */
    jjs_fatal (JJS_FATAL_REF_COUNT_LIMIT);
  }

  script_p->refs_and_type += CBC_SCRIPT_REF_ONE;

  if (bytecode_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    uint8_t *byte_p = (uint8_t *) bytecode_p;
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) byte_p;

    argument_end = args_p->argument_end;
    const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);
    literal_end = (uint32_t) (args_p->literal_end - args_p->register_end);
    header_size = sizeof (cbc_uint16_arguments_t);

    ECMA_SET_INTERNAL_VALUE_POINTER (context_p, args_p->script_value, script_p);
  }
  else
  {
    uint8_t *byte_p = (uint8_t *) bytecode_p;
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) byte_p;

    argument_end = args_p->argument_end;
    const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);
    literal_end = (uint32_t) (args_p->literal_end - args_p->register_end);
    header_size = sizeof (cbc_uint8_arguments_t);

    ECMA_SET_INTERNAL_VALUE_POINTER (context_p, args_p->script_value, script_p);
  }

  if (copy_bytecode || (header_size + (literal_end * sizeof (uint16_t)) + BYTECODE_NO_COPY_THRESHOLD > code_size))
  {
    bytecode_p = (ecma_compiled_code_t *) jmem_heap_alloc_block (context_p, code_size);

#if JJS_MEM_STATS
    jmem_stats_allocate_byte_code_bytes (context_p, code_size);
#endif /* JJS_MEM_STATS */

    memcpy (bytecode_p, base_addr_p, code_size);
  }
  else
  {
    uint32_t start_offset = (uint32_t) (header_size + literal_end * sizeof (ecma_value_t));

    uint8_t *real_bytecode_p = ((uint8_t *) bytecode_p) + start_offset;
    uint32_t new_code_size = (uint32_t) (start_offset + 1 + sizeof (uint8_t *));
    uint32_t extra_bytes = 0;

    if (bytecode_p->status_flags & CBC_CODE_FLAGS_MAPPED_ARGUMENTS_NEEDED)
    {
      extra_bytes += (uint32_t) (argument_end * sizeof (ecma_value_t));
    }

    /* function name */
    if (CBC_FUNCTION_GET_TYPE (bytecode_p->status_flags) != CBC_FUNCTION_CONSTRUCTOR)
    {
      extra_bytes += (uint32_t) sizeof (ecma_value_t);
    }

    /* tagged template literals */
    if (bytecode_p->status_flags & CBC_CODE_FLAGS_HAS_TAGGED_LITERALS)
    {
      extra_bytes += (uint32_t) sizeof (ecma_value_t);
    }

    /* adjust for line info block */
    extra_bytes += (uint32_t) sizeof (ecma_value_t);

#if JJS_SOURCE_NAME
    /* TODO: what is this? i don't see where this is being set in parser. */
    /* source name */
    extra_bytes += (uint32_t) sizeof (ecma_value_t);
#endif /* JJS_SOURCE_NAME */

    new_code_size = JJS_ALIGNUP (new_code_size + extra_bytes, JMEM_ALIGNMENT);

    bytecode_p = (ecma_compiled_code_t *) jmem_heap_alloc_block (context_p, new_code_size);

#if JJS_MEM_STATS
    jmem_stats_allocate_byte_code_bytes (context_p, new_code_size);
#endif /* JJS_MEM_STATS */

    memcpy (bytecode_p, base_addr_p, start_offset);

    bytecode_p->size = (uint16_t) (new_code_size >> JMEM_ALIGNMENT_LOG);

    uint8_t *byte_p = (uint8_t *) bytecode_p;

    uint8_t *new_base_p = byte_p + new_code_size - extra_bytes;
    const uint8_t *base_p = base_addr_p + code_size - extra_bytes;

    if (extra_bytes != 0)
    {
      memcpy (new_base_p, base_p, extra_bytes);
    }

    byte_p[start_offset] = CBC_SET_BYTECODE_PTR;
    memcpy (byte_p + start_offset + 1, &real_bytecode_p, sizeof (uintptr_t));

    code_size = new_code_size;
  }

  JJS_ASSERT (bytecode_p->refs == 1);

#if JJS_DEBUGGER
  bytecode_p->status_flags = (uint16_t) (bytecode_p->status_flags | CBC_CODE_FLAGS_DEBUGGER_IGNORE);
#endif /* JJS_DEBUGGER */

  ecma_value_t *literal_start_p = (ecma_value_t *) (((uint8_t *) bytecode_p) + header_size);

  for (uint32_t i = 0; i < const_literal_end; i++)
  {
    if ((literal_start_p[i] & ECMA_VALUE_TYPE_MASK) == ECMA_TYPE_SNAPSHOT_OFFSET)
    {
      literal_start_p[i] = ecma_snapshot_get_literal (context_p, literal_base_p, literal_start_p[i]);
    }
  }

  for (uint32_t i = const_literal_end; i < literal_end; i++)
  {
    size_t literal_offset = (size_t) literal_start_p[i];

    if (literal_offset == 0)
    {
      /* Self reference */
      ECMA_SET_INTERNAL_VALUE_POINTER (context_p, literal_start_p[i], bytecode_p);
    }
    else
    {
      ecma_compiled_code_t *literal_bytecode_p;
      literal_bytecode_p =
        snapshot_load_compiled_code (context_p, base_addr_p + literal_offset, literal_base_p, script_p, copy_bytecode);

      ECMA_SET_INTERNAL_VALUE_POINTER (context_p, literal_start_p[i], literal_bytecode_p);
    }
  }

  uint8_t *byte_p = ((uint8_t *) bytecode_p) + code_size;
  literal_start_p = ecma_snapshot_resolve_serializable_values (bytecode_p, byte_p);

  while (literal_start_p < (ecma_value_t *) byte_p)
  {
    if ((*literal_start_p & ECMA_VALUE_TYPE_MASK) == ECMA_TYPE_SNAPSHOT_OFFSET)
    {
      *literal_start_p = ecma_snapshot_get_literal (context_p, literal_base_p, *literal_start_p);
    }

    literal_start_p++;
  }

  return bytecode_p;
} /* snapshot_load_compiled_code */

#endif /* JJS_SNAPSHOT_EXEC */

/**
 * Generate snapshot from specified source and arguments
 *
 * @return size of snapshot (a number value), if it was generated succesfully
 *          (i.e. there are no syntax errors in source code, buffer size is sufficient,
 *           and snapshot support is enabled in current configuration through JJS_SNAPSHOT_SAVE),
 *         error object otherwise
 */
jjs_value_t
jjs_generate_snapshot (jjs_context_t* context_p, /**< JJS context */
                       jjs_value_t compiled_code, /**< parsed script or function */
                       uint32_t generate_snapshot_opts, /**< jjs_generate_snapshot_opts_t option bits */
                       uint32_t *buffer_p, /**< buffer to save snapshot to */
                       size_t buffer_size) /**< the buffer's size */
{
#if JJS_SNAPSHOT_SAVE
  uint32_t allowed_options = JJS_SNAPSHOT_SAVE_STATIC;

  if ((generate_snapshot_opts & ~allowed_options) != 0)
  {
    return jjs_throw_sz (context_p, JJS_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_SNAPSHOT_FLAG_NOT_SUPPORTED));
  }

  const ecma_compiled_code_t *bytecode_data_p = NULL;

  if (ecma_is_value_object (compiled_code))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (context_p, compiled_code);

    if (ecma_object_class_is (object_p, ECMA_OBJECT_CLASS_SCRIPT))
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (context_p, ecma_compiled_code_t, ext_object_p->u.cls.u3.value);
    }
    else if (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_FUNCTION)
    {
      ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;

      bytecode_data_p = ecma_op_function_get_compiled_code (context_p, ext_func_p);

      uint16_t type = CBC_FUNCTION_GET_TYPE (bytecode_data_p->status_flags);

      if (type != CBC_FUNCTION_NORMAL)
      {
        bytecode_data_p = NULL;
      }
    }
  }

  if (JJS_UNLIKELY (bytecode_data_p == NULL))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_SNAPSHOT_UNSUPPORTED_COMPILED_CODE));
  }

  snapshot_globals_t globals;
  const uint32_t aligned_header_size = JJS_ALIGNUP (sizeof (jjs_snapshot_header_t), JMEM_ALIGNMENT);

  globals.snapshot_buffer_write_offset = aligned_header_size;
  globals.snapshot_error = ECMA_VALUE_EMPTY;
  globals.regex_found = false;
  globals.class_found = false;

  if (generate_snapshot_opts & JJS_SNAPSHOT_SAVE_STATIC)
  {
    static_snapshot_add_compiled_code (context_p, bytecode_data_p, (uint8_t *) buffer_p, buffer_size, &globals);
  }
  else
  {
    snapshot_add_compiled_code (context_p, bytecode_data_p, (uint8_t *) buffer_p, buffer_size, &globals);
  }

  if (!ecma_is_value_empty (globals.snapshot_error))
  {
    return globals.snapshot_error;
  }

  jjs_snapshot_header_t header;
  header.magic = JJS_SNAPSHOT_MAGIC;
  header.version = JJS_SNAPSHOT_VERSION;
  header.global_flags = snapshot_get_global_flags (globals.regex_found, globals.class_found);
  header.lit_table_offset = (uint32_t) globals.snapshot_buffer_write_offset;
  header.number_of_funcs = 1;
  header.func_offsets[0] = aligned_header_size;

  lit_mem_to_snapshot_id_map_entry_t *lit_map_p = NULL;
  uint32_t literals_num = 0;

  if (!(generate_snapshot_opts & JJS_SNAPSHOT_SAVE_STATIC))
  {
    ecma_collection_t *lit_pool_p = ecma_new_collection (context_p);

    ecma_save_literals_add_compiled_code (context_p, bytecode_data_p, lit_pool_p);

    if (!ecma_save_literals_for_snapshot (context_p,
                                          lit_pool_p,
                                          buffer_p,
                                          buffer_size,
                                          &globals.snapshot_buffer_write_offset,
                                          &lit_map_p,
                                          &literals_num))
    {
      JJS_ASSERT (lit_map_p == NULL);
      return jjs_throw_sz (context_p, JJS_ERROR_COMMON, ecma_get_error_msg (ECMA_ERR_CANNOT_ALLOCATE_MEMORY_LITERALS));
    }

    jjs_snapshot_set_offsets (buffer_p + (aligned_header_size / sizeof (uint32_t)),
                                (uint32_t) (header.lit_table_offset - aligned_header_size),
                                lit_map_p);
  }

  size_t header_offset = 0;

  snapshot_write_to_buffer_by_offset ((uint8_t *) buffer_p, buffer_size, &header_offset, &header, sizeof (header));

  if (lit_map_p != NULL)
  {
    jmem_heap_free_block (context_p, lit_map_p, literals_num * sizeof (lit_mem_to_snapshot_id_map_entry_t));
  }

  return ecma_make_number_value (context_p, (ecma_number_t) globals.snapshot_buffer_write_offset);
#else /* !JJS_SNAPSHOT_SAVE */
  JJS_UNUSED_ALL (compiled_code, generate_snapshot_opts, buffer_p, buffer_size);
  return jjs_throw_sz (context_p, JJS_ERROR_COMMON, ecma_get_error_msg (ECMA_ERR_SNAPSHOT_SAVE_DISABLED));
#endif /* JJS_SNAPSHOT_SAVE */
} /* jjs_generate_snapshot */

/**
 * Execute/load snapshot from specified buffer
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return result of bytecode - if run was successful
 *         thrown error - otherwise
 */
jjs_value_t
jjs_exec_snapshot (jjs_context_t* context_p, /**< JJS context */
                   const uint32_t *snapshot_p, /**< snapshot */
                   size_t snapshot_size, /**< size of snapshot */
                   size_t func_index, /**< index of primary function */
                   uint32_t exec_snapshot_opts, /**< jjs_exec_snapshot_opts_t option bits */
                   const jjs_exec_snapshot_option_values_t *option_values_p) /**< additional option values,
                                                                                  *   can be NULL if not used */
{
#if JJS_SNAPSHOT_EXEC
  JJS_ASSERT (snapshot_p != NULL);

  uint32_t allowed_opts =
    (JJS_SNAPSHOT_EXEC_COPY_DATA | JJS_SNAPSHOT_EXEC_ALLOW_STATIC | JJS_SNAPSHOT_EXEC_LOAD_AS_FUNCTION
     | JJS_SNAPSHOT_EXEC_HAS_SOURCE_NAME | JJS_SNAPSHOT_EXEC_HAS_USER_VALUE);

  if ((exec_snapshot_opts & ~(allowed_opts)) != 0)
  {
    return jjs_throw_sz (context_p, JJS_ERROR_RANGE,
                           ecma_get_error_msg (ECMA_ERR_UNSUPPORTED_SNAPSHOT_EXEC_FLAGS_ARE_SPECIFIED));
  }

  const uint8_t *snapshot_data_p = (uint8_t *) snapshot_p;

  if (snapshot_size <= sizeof (jjs_snapshot_header_t))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INVALID_SNAPSHOT_FORMAT));
  }

  const jjs_snapshot_header_t *header_p = (const jjs_snapshot_header_t *) snapshot_data_p;

  if (header_p->magic != JJS_SNAPSHOT_MAGIC || header_p->version != JJS_SNAPSHOT_VERSION
      || !snapshot_check_global_flags (header_p->global_flags))
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INVALID_SNAPSHOT_VERSION_OR_FEATURES));
  }

  if (header_p->lit_table_offset > snapshot_size)
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INVALID_SNAPSHOT_VERSION_OR_FEATURES));
  }

  if (func_index >= header_p->number_of_funcs)
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_FUNCTION_INDEX_IS_HIGHER_THAN_MAXIMUM));
  }

  JJS_ASSERT ((header_p->lit_table_offset % sizeof (uint32_t)) == 0);

  uint32_t func_offset = header_p->func_offsets[func_index];
  ecma_compiled_code_t *bytecode_p = (ecma_compiled_code_t *) (snapshot_data_p + func_offset);

  if (bytecode_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION)
  {
    if (!(exec_snapshot_opts & JJS_SNAPSHOT_EXEC_ALLOW_STATIC))
    {
      return jjs_throw_sz (context_p, JJS_ERROR_COMMON, ecma_get_error_msg (ECMA_ERR_STATIC_SNAPSHOTS_ARE_NOT_ENABLED));
    }

    if (exec_snapshot_opts & JJS_SNAPSHOT_EXEC_COPY_DATA)
    {
      return jjs_throw_sz (context_p, JJS_ERROR_COMMON,
                             ecma_get_error_msg (ECMA_ERR_STATIC_SNAPSHOTS_CANNOT_BE_COPIED_INTO_MEMORY));
    }
  }
  else
  {
    ecma_value_t user_value = ECMA_VALUE_EMPTY;

    if ((exec_snapshot_opts & JJS_SNAPSHOT_EXEC_HAS_USER_VALUE) && option_values_p != NULL)
    {
      user_value = option_values_p->user_value;
    }

    size_t script_size = sizeof (cbc_script_t);

    if (user_value != ECMA_VALUE_EMPTY)
    {
      script_size += sizeof (ecma_value_t);
    }

    cbc_script_t *script_p = jmem_heap_alloc_block (context_p, script_size);

    CBC_SCRIPT_SET_TYPE (script_p, user_value, CBC_SCRIPT_REF_ONE);

#if JJS_BUILTIN_REALMS
    script_p->realm_p = (ecma_object_t *) context_p->global_object_p;
#endif /* JJS_BUILTIN_REALMS */

#if JJS_SOURCE_NAME
    ecma_value_t source_name = ecma_make_magic_string_value (LIT_MAGIC_STRING_SOURCE_NAME_ANON);

    if ((exec_snapshot_opts & JJS_SNAPSHOT_EXEC_HAS_SOURCE_NAME) && option_values_p != NULL
        && ecma_is_value_string (option_values_p->source_name) > 0)
    {
      ecma_ref_ecma_string (ecma_get_string_from_value (context_p, option_values_p->source_name));
      source_name = option_values_p->source_name;
    }

    script_p->source_name = source_name;
#endif /* JJS_SOURCE_NAME */

#if JJS_FUNCTION_TO_STRING
    script_p->source_code = ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
#endif /* JJS_FUNCTION_TO_STRING */

    const uint8_t *literal_base_p = snapshot_data_p + header_p->lit_table_offset;

    bytecode_p = snapshot_load_compiled_code (context_p,
                                              (const uint8_t *) bytecode_p,
                                              literal_base_p,
                                              script_p,
                                              (exec_snapshot_opts & JJS_SNAPSHOT_EXEC_COPY_DATA) != 0);

    if (bytecode_p == NULL)
    {
      JJS_ASSERT (script_p->refs_and_type >= CBC_SCRIPT_REF_ONE);
      jmem_heap_free_block (context_p, script_p, script_size);
      return ecma_raise_type_error (context_p, ECMA_ERR_INVALID_SNAPSHOT_FORMAT);
    }

    script_p->refs_and_type -= CBC_SCRIPT_REF_ONE;

    if (user_value != ECMA_VALUE_EMPTY)
    {
      CBC_SCRIPT_GET_USER_VALUE (script_p) = ecma_copy_value_if_not_object (context_p, user_value);
    }
  }

#if JJS_PARSER_DUMP_BYTE_CODE
  if (context_p->context_flags & JJS_CONTEXT_FLAG_SHOW_OPCODES)
  {
    util_print_cbc (context_p, bytecode_p);
  }
#endif /* JJS_PARSER_DUMP_BYTE_CODE */

  ecma_value_t ret_val;

  if (exec_snapshot_opts & JJS_SNAPSHOT_EXEC_LOAD_AS_FUNCTION)
  {
    ecma_object_t *global_object_p = ecma_builtin_get_global (context_p);

#if JJS_BUILTIN_REALMS
    JJS_ASSERT (global_object_p == (ecma_object_t *) ecma_op_function_get_realm (context_p, bytecode_p));
#endif /* JJS_BUILTIN_REALMS */

    if (bytecode_p->status_flags & CBC_CODE_FLAGS_LEXICAL_BLOCK_NEEDED)
    {
      ecma_create_global_lexical_block (context_p, global_object_p);
    }

    ecma_object_t *lex_env_p = ecma_get_global_scope (context_p, global_object_p);
    ecma_object_t *func_obj_p = ecma_op_create_simple_function_object (context_p, lex_env_p, bytecode_p);

    if (!(bytecode_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION))
    {
      ecma_bytecode_deref (context_p, bytecode_p);
    }
    ret_val = ecma_make_object_value (context_p, func_obj_p);
  }
  else
  {
    ret_val = vm_run_global (context_p, bytecode_p, NULL);
    if (!(bytecode_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION))
    {
      ecma_bytecode_deref (context_p, bytecode_p);
    }
  }

  if (ECMA_IS_VALUE_ERROR (ret_val))
  {
    return ecma_create_exception_from_context (context_p);
  }

  return ret_val;
#else /* !JJS_SNAPSHOT_EXEC */
  JJS_UNUSED_ALL (snapshot_p, snapshot_size, func_index, exec_snapshot_opts, option_values_p);
  return jjs_throw_sz (context_p, JJS_ERROR_COMMON, ecma_get_error_msg (ECMA_ERR_SNAPSHOT_EXEC_DISABLED));
#endif /* JJS_SNAPSHOT_EXEC */
} /* jjs_exec_snapshot */

/**
 * @}
 */

#if JJS_SNAPSHOT_SAVE

/**
 * Collect all literals from a snapshot file.
 */
static void
scan_snapshot_functions (jjs_context_t *context_p, /**< JJS context */
                         const uint8_t *buffer_p, /**< snapshot buffer start */
                         const uint8_t *buffer_end_p, /**< snapshot buffer end */
                         ecma_collection_t *lit_pool_p, /**< list of known values */
                         const uint8_t *literal_base_p) /**< start of literal data */
{
  JJS_ASSERT (buffer_end_p > buffer_p);

  do
  {
    const ecma_compiled_code_t *bytecode_p = (ecma_compiled_code_t *) buffer_p;
    uint32_t code_size = ((uint32_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG;

    if (CBC_IS_FUNCTION (bytecode_p->status_flags) && !(bytecode_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION))
    {
      const ecma_value_t *literal_start_p;
      uint32_t const_literal_end;

      if (bytecode_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
      {
        literal_start_p = (ecma_value_t *) (buffer_p + sizeof (cbc_uint16_arguments_t));

        cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) buffer_p;
        const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);
      }
      else
      {
        literal_start_p = (ecma_value_t *) (buffer_p + sizeof (cbc_uint8_arguments_t));

        cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) buffer_p;
        const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);
      }

      for (uint32_t i = 0; i < const_literal_end; i++)
      {
        if ((literal_start_p[i] & ECMA_VALUE_TYPE_MASK) == ECMA_TYPE_SNAPSHOT_OFFSET)
        {
          ecma_value_t lit_value = ecma_snapshot_get_literal (context_p, literal_base_p, literal_start_p[i]);
          ecma_save_literals_append_value (context_p, lit_value, lit_pool_p);
        }
      }

      uint8_t *byte_p = (uint8_t *) bytecode_p + (((size_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG);
      literal_start_p = ecma_snapshot_resolve_serializable_values ((ecma_compiled_code_t *) bytecode_p, byte_p);

      while (literal_start_p < (ecma_value_t *) byte_p)
      {
        if ((*literal_start_p & ECMA_VALUE_TYPE_MASK) == ECMA_TYPE_SNAPSHOT_OFFSET)
        {
          ecma_value_t lit_value = ecma_snapshot_get_literal (context_p, literal_base_p, *literal_start_p);
          ecma_save_literals_append_value (context_p, lit_value, lit_pool_p);
        }

        literal_start_p++;
      }
    }

    buffer_p += code_size;
  } while (buffer_p < buffer_end_p);
} /* scan_snapshot_functions */

/**
 * Update all literal offsets in a snapshot data.
 */
static void
update_literal_offsets (jjs_context_t *context_p, /**< JJS context */
                        uint8_t *buffer_p, /**< [in,out] snapshot buffer start */
                        const uint8_t *buffer_end_p, /**< snapshot buffer end */
                        const lit_mem_to_snapshot_id_map_entry_t *lit_map_p, /**< literal map */
                        const uint8_t *literal_base_p) /**< start of literal data */
{
  JJS_ASSERT (buffer_end_p > buffer_p);

  do
  {
    const ecma_compiled_code_t *bytecode_p = (ecma_compiled_code_t *) buffer_p;
    uint32_t code_size = ((uint32_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG;

    if (CBC_IS_FUNCTION (bytecode_p->status_flags) && !(bytecode_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION))
    {
      ecma_value_t *literal_start_p;
      uint32_t const_literal_end;

      if (bytecode_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
      {
        literal_start_p = (ecma_value_t *) (buffer_p + sizeof (cbc_uint16_arguments_t));

        cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) buffer_p;
        const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);
      }
      else
      {
        literal_start_p = (ecma_value_t *) (buffer_p + sizeof (cbc_uint8_arguments_t));

        cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) buffer_p;
        const_literal_end = (uint32_t) (args_p->const_literal_end - args_p->register_end);
      }

      for (uint32_t i = 0; i < const_literal_end; i++)
      {
        if ((literal_start_p[i] & ECMA_VALUE_TYPE_MASK) == ECMA_TYPE_SNAPSHOT_OFFSET)
        {
          ecma_value_t lit_value = ecma_snapshot_get_literal (context_p, literal_base_p, literal_start_p[i]);
          const lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

          while (current_p->literal_id != lit_value)
          {
            current_p++;
          }

          literal_start_p[i] = current_p->literal_offset;
        }
      }

      uint8_t *byte_p = (uint8_t *) bytecode_p + (((size_t) bytecode_p->size) << JMEM_ALIGNMENT_LOG);
      literal_start_p = ecma_snapshot_resolve_serializable_values ((ecma_compiled_code_t *) bytecode_p, byte_p);

      while (literal_start_p < (ecma_value_t *) byte_p)
      {
        if ((*literal_start_p & ECMA_VALUE_TYPE_MASK) == ECMA_TYPE_SNAPSHOT_OFFSET)
        {
          ecma_value_t lit_value = ecma_snapshot_get_literal (context_p, literal_base_p, *literal_start_p);
          const lit_mem_to_snapshot_id_map_entry_t *current_p = lit_map_p;

          while (current_p->literal_id != lit_value)
          {
            current_p++;
          }

          *literal_start_p = current_p->literal_offset;
        }

        literal_start_p++;
      }
    }

    buffer_p += code_size;
  } while (buffer_p < buffer_end_p);
} /* update_literal_offsets */

#endif /* JJS_SNAPSHOT_SAVE */

/**
 * Merge multiple snapshots into a single buffer
 *
 * @return length of merged snapshot file
 *         0 on error
 */
size_t
jjs_merge_snapshots (jjs_context_t* context_p, /**< JJS context */
                     const uint32_t **inp_buffers_p, /**< array of (pointers to start of) input buffers */
                     size_t *inp_buffer_sizes_p, /**< array of input buffer sizes */
                     size_t number_of_snapshots, /**< number of snapshots */
                     uint32_t *out_buffer_p, /**< output buffer */
                     size_t out_buffer_size, /**< output buffer size */
                     const char **error_p) /**< error description */
{
#if JJS_SNAPSHOT_SAVE
  JJS_UNUSED (context_p);
  uint32_t number_of_funcs = 0;
  uint32_t merged_global_flags = 0;
  size_t functions_size = sizeof (jjs_snapshot_header_t);

  if (number_of_snapshots < 2)
  {
    *error_p = "at least two snapshots must be passed";
    return 0;
  }

  ecma_collection_t *lit_pool_p = ecma_new_collection (context_p);

  for (uint32_t i = 0; i < number_of_snapshots; i++)
  {
    if (inp_buffer_sizes_p[i] < sizeof (jjs_snapshot_header_t))
    {
      *error_p = "invalid snapshot file";
      ecma_collection_destroy (context_p, lit_pool_p);
      return 0;
    }

    const jjs_snapshot_header_t *header_p = (const jjs_snapshot_header_t *) inp_buffers_p[i];

    if (header_p->magic != JJS_SNAPSHOT_MAGIC || header_p->version != JJS_SNAPSHOT_VERSION
        || !snapshot_check_global_flags (header_p->global_flags))
    {
      *error_p = "invalid snapshot version or unsupported features present";
      ecma_collection_destroy (context_p, lit_pool_p);
      return 0;
    }

    merged_global_flags |= header_p->global_flags;

    uint32_t start_offset = header_p->func_offsets[0];
    const uint8_t *data_p = (const uint8_t *) inp_buffers_p[i];
    const uint8_t *literal_base_p = data_p + header_p->lit_table_offset;

    JJS_ASSERT (header_p->number_of_funcs > 0);

    number_of_funcs += header_p->number_of_funcs;
    functions_size += header_p->lit_table_offset - start_offset;

    scan_snapshot_functions (context_p, data_p + start_offset, literal_base_p, lit_pool_p, literal_base_p);
  }

  JJS_ASSERT (number_of_funcs > 0);

  functions_size += JJS_ALIGNUP ((number_of_funcs - 1) * sizeof (uint32_t), JMEM_ALIGNMENT);

  if (functions_size >= out_buffer_size)
  {
    *error_p = "output buffer is too small";
    ecma_collection_destroy (context_p, lit_pool_p);
    return 0;
  }

  jjs_snapshot_header_t *header_p = (jjs_snapshot_header_t *) out_buffer_p;

  header_p->magic = JJS_SNAPSHOT_MAGIC;
  header_p->version = JJS_SNAPSHOT_VERSION;
  header_p->global_flags = merged_global_flags;
  header_p->lit_table_offset = (uint32_t) functions_size;
  header_p->number_of_funcs = number_of_funcs;

  lit_mem_to_snapshot_id_map_entry_t *lit_map_p;
  uint32_t literals_num;

  if (!ecma_save_literals_for_snapshot (context_p,
                                        lit_pool_p,
                                        out_buffer_p,
                                        out_buffer_size,
                                        &functions_size,
                                        &lit_map_p,
                                        &literals_num))
  {
    *error_p = "buffer is too small";
    return 0;
  }

  uint32_t *func_offset_p = header_p->func_offsets;
  uint8_t *dst_p = ((uint8_t *) out_buffer_p) + sizeof (jjs_snapshot_header_t);
  dst_p += JJS_ALIGNUP ((number_of_funcs - 1) * sizeof (uint32_t), JMEM_ALIGNMENT);

  for (uint32_t i = 0; i < number_of_snapshots; i++)
  {
    const jjs_snapshot_header_t *current_header_p = (const jjs_snapshot_header_t *) inp_buffers_p[i];

    uint32_t start_offset = current_header_p->func_offsets[0];

    memcpy (dst_p,
            ((const uint8_t *) inp_buffers_p[i]) + start_offset,
            current_header_p->lit_table_offset - start_offset);

    const uint8_t *literal_base_p = ((const uint8_t *) inp_buffers_p[i]) + current_header_p->lit_table_offset;
    update_literal_offsets (context_p,
                            dst_p,
                            dst_p + current_header_p->lit_table_offset - start_offset,
                            lit_map_p,
                            literal_base_p);

    uint32_t current_offset = (uint32_t) (dst_p - (uint8_t *) out_buffer_p) - start_offset;

    for (uint32_t j = 0; j < current_header_p->number_of_funcs; j++)
    {
      /* Updating offset without changing any flags. */
      *func_offset_p++ = current_header_p->func_offsets[j] + current_offset;
    }

    dst_p += current_header_p->lit_table_offset - start_offset;
  }

  JJS_ASSERT ((uint32_t) (dst_p - (uint8_t *) out_buffer_p) == header_p->lit_table_offset);

  if (lit_map_p != NULL)
  {
    jmem_heap_free_block (context_p, lit_map_p, literals_num * sizeof (lit_mem_to_snapshot_id_map_entry_t));
  }

  *error_p = NULL;
  return functions_size;
#else /* !JJS_SNAPSHOT_SAVE */
  JJS_UNUSED_ALL (context_p, inp_buffers_p, inp_buffer_sizes_p, number_of_snapshots, out_buffer_p, out_buffer_size, error_p);

  *error_p = "snapshot merge not supported";
  return 0;
#endif /* JJS_SNAPSHOT_SAVE */
} /* jjs_merge_snapshots */

#if JJS_SNAPSHOT_SAVE

/**
 * ====================== Functions for literal saving ==========================
 */

/**
 * Compare two ecma_strings by size, then lexicographically.
 *
 * @return true - if the first string is less than the second one,
 *         false - otherwise
 */
static bool
jjs_save_literals_compare (jjs_context_t *context_p, /**< JJS context */
                           ecma_string_t *literal1, /**< first literal */
                           ecma_string_t *literal2) /**< second literal */
{
  const lit_utf8_size_t lit1_size = ecma_string_get_size (context_p, literal1);
  const lit_utf8_size_t lit2_size = ecma_string_get_size (context_p, literal2);

  if (lit1_size == lit2_size)
  {
    return ecma_compare_ecma_strings_relational (context_p, literal1, literal2);
  }

  return (lit1_size < lit2_size);
} /* jjs_save_literals_compare */

/**
 * Helper function for the heapsort algorithm.
 *
 * @return index of the maximum value
 */
static lit_utf8_size_t
jjs_save_literals_heap_max (jjs_context_t *context_p, /**< JJS context */
                            ecma_string_t *literals[], /**< array of literals */
                            lit_utf8_size_t num_of_nodes, /**< number of nodes */
                            lit_utf8_size_t node_idx, /**< index of parent node */
                            lit_utf8_size_t child_idx1, /**< index of the first child */
                            lit_utf8_size_t child_idx2) /**< index of the second child */
{
  lit_utf8_size_t max_idx = node_idx;

  if (child_idx1 < num_of_nodes && jjs_save_literals_compare (context_p, literals[max_idx], literals[child_idx1]))
  {
    max_idx = child_idx1;
  }

  if (child_idx2 < num_of_nodes && jjs_save_literals_compare (context_p, literals[max_idx], literals[child_idx2]))
  {
    max_idx = child_idx2;
  }

  return max_idx;
} /* jjs_save_literals_heap_max */

/**
 * Helper function for the heapsort algorithm.
 */
static void
jjs_save_literals_down_heap (jjs_context_t *context_p, /**< JJS context */
                             ecma_string_t *literals[], /**< array of literals */
                             lit_utf8_size_t num_of_nodes, /**< number of nodes */
                             lit_utf8_size_t node_idx) /**< index of parent node */
{
  while (true)
  {
    lit_utf8_size_t max_idx =
      jjs_save_literals_heap_max (context_p, literals, num_of_nodes, node_idx, 2 * node_idx + 1, 2 * node_idx + 2);
    if (max_idx == node_idx)
    {
      break;
    }

    ecma_string_t *tmp_str_p = literals[node_idx];
    literals[node_idx] = literals[max_idx];
    literals[max_idx] = tmp_str_p;

    node_idx = max_idx;
  }
} /* jjs_save_literals_down_heap */

/**
 * Helper function for a heapsort algorithm.
 */
static void
jjs_save_literals_sort (jjs_context_t *context_p, /**< JJS context */
                        ecma_string_t *literals[], /**< array of literals */
                        lit_utf8_size_t num_of_literals) /**< number of literals */
{
  if (num_of_literals < 2)
  {
    return;
  }

  lit_utf8_size_t lit_idx = (num_of_literals - 2) / 2;

  while (lit_idx <= (num_of_literals - 2) / 2)
  {
    jjs_save_literals_down_heap (context_p, literals, num_of_literals, lit_idx--);
  }

  for (lit_idx = 0; lit_idx < num_of_literals; lit_idx++)
  {
    const lit_utf8_size_t last_idx = num_of_literals - lit_idx - 1;

    ecma_string_t *tmp_str_p = literals[last_idx];
    literals[last_idx] = literals[0];
    literals[0] = tmp_str_p;

    jjs_save_literals_down_heap (context_p, literals, last_idx, 0);
  }
} /* jjs_save_literals_sort */

/**
 * Append characters to the specified buffer.
 *
 * @return the position of the buffer pointer after copy.
 */
static uint8_t *
jjs_append_chars_to_buffer (uint8_t *buffer_p, /**< buffer */
                              uint8_t *buffer_end_p, /**< the end of the buffer */
                              const char *chars, /**< string */
                              lit_utf8_size_t string_size) /**< string size */
{
  if (buffer_p > buffer_end_p)
  {
    return buffer_p;
  }

  if (string_size == 0)
  {
    string_size = (lit_utf8_size_t) strlen (chars);
  }

  if (buffer_p + string_size <= buffer_end_p)
  {
    memcpy ((char *) buffer_p, chars, string_size);

    return buffer_p + string_size;
  }

  /* Move the pointer behind the buffer to prevent further writes. */
  return buffer_end_p + 1;
} /* jjs_append_chars_to_buffer */

/**
 * Append an ecma-string to the specified buffer.
 *
 * @return the position of the buffer pointer after copy.
 */
static uint8_t *
jjs_append_ecma_string_to_buffer (jjs_context_t *context_p, /**< JJS context */
                                  uint8_t *buffer_p, /**< buffer */
                                  uint8_t *buffer_end_p, /**< the end of the buffer */
                                  ecma_string_t *string_p) /**< ecma-string */
{
  ECMA_STRING_TO_UTF8_STRING (context_p, string_p, str_buffer_p, str_buffer_size);

  /* Append the string to the buffer. */
  uint8_t *new_buffer_p =
    jjs_append_chars_to_buffer (buffer_p, buffer_end_p, (const char *) str_buffer_p, str_buffer_size);

  ECMA_FINALIZE_UTF8_STRING (context_p, str_buffer_p, str_buffer_size);

  return new_buffer_p;
} /* jjs_append_ecma_string_to_buffer */

/**
 * Append an unsigned number to the specified buffer.
 *
 * @return the position of the buffer pointer after copy.
 */
static uint8_t *
jjs_append_number_to_buffer (uint8_t *buffer_p, /**< buffer */
                               uint8_t *buffer_end_p, /**< the end of the buffer */
                               lit_utf8_size_t number) /**< number */
{
  lit_utf8_byte_t uint32_to_str_buffer[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
  lit_utf8_size_t utf8_str_size =
    ecma_uint32_to_utf8_string (number, uint32_to_str_buffer, ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32);

  JJS_ASSERT (utf8_str_size <= ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32);

  return jjs_append_chars_to_buffer (buffer_p, buffer_end_p, (const char *) uint32_to_str_buffer, utf8_str_size);
} /* jjs_append_number_to_buffer */

#endif /* JJS_SNAPSHOT_SAVE */

/**
 * Get the literals from a snapshot. Copies certain string literals into the given
 * buffer in a specified format.
 *
 * Note:
 *      Only valid identifiers are saved in C format.
 *
 * @return size of the literal-list in bytes, at most equal to the buffer size,
 *         if the list of the literals isn't empty,
 *         0 - otherwise.
 */
size_t
jjs_get_literals_from_snapshot (jjs_context_t *context_p, /**< JJS context */
                                const uint32_t *snapshot_p, /**< input snapshot buffer */
                                size_t snapshot_size, /**< size of the input snapshot buffer */
                                jjs_char_t *lit_buf_p, /**< [out] buffer to save literals to */
                                size_t lit_buf_size, /**< the buffer's size */
                                bool is_c_format) /**< format-flag */
{
#if JJS_SNAPSHOT_SAVE
  const uint8_t *snapshot_data_p = (uint8_t *) snapshot_p;
  const jjs_snapshot_header_t *header_p = (const jjs_snapshot_header_t *) snapshot_data_p;

  if (snapshot_size <= sizeof (jjs_snapshot_header_t) || header_p->magic != JJS_SNAPSHOT_MAGIC
      || header_p->version != JJS_SNAPSHOT_VERSION || !snapshot_check_global_flags (header_p->global_flags))
  {
    /* Invalid snapshot format */
    return 0;
  }

  JJS_ASSERT ((header_p->lit_table_offset % sizeof (uint32_t)) == 0);
  const uint8_t *literal_base_p = snapshot_data_p + header_p->lit_table_offset;

  ecma_collection_t *lit_pool_p = ecma_new_collection (context_p);
  scan_snapshot_functions (context_p, snapshot_data_p + header_p->func_offsets[0], literal_base_p, lit_pool_p, literal_base_p);

  lit_utf8_size_t literal_count = 0;
  ecma_value_t *buffer_p = lit_pool_p->buffer_p;

  /* Count the valid and non-magic identifiers in the list. */
  for (uint32_t i = 0; i < lit_pool_p->item_count; i++)
  {
    if (ecma_is_value_string (buffer_p[i]))
    {
      ecma_string_t *literal_p = ecma_get_string_from_value (context_p, buffer_p[i]);

      if (ecma_get_string_magic (literal_p) == LIT_MAGIC_STRING__COUNT)
      {
        literal_count++;
      }
    }
  }

  if (literal_count == 0)
  {
    ecma_collection_destroy (context_p, lit_pool_p);
    return 0;
  }

  jjs_char_t *const buffer_start_p = lit_buf_p;
  jjs_char_t *const buffer_end_p = lit_buf_p + lit_buf_size;

  JMEM_DEFINE_LOCAL_ARRAY (context_p, literal_array, literal_count, ecma_string_t *);
  lit_utf8_size_t literal_idx = 0;

  buffer_p = lit_pool_p->buffer_p;

  /* Count the valid and non-magic identifiers in the list. */
  for (uint32_t i = 0; i < lit_pool_p->item_count; i++)
  {
    if (ecma_is_value_string (buffer_p[i]))
    {
      ecma_string_t *literal_p = ecma_get_string_from_value (context_p, buffer_p[i]);

      if (ecma_get_string_magic (literal_p) == LIT_MAGIC_STRING__COUNT)
      {
        literal_array[literal_idx++] = literal_p;
      }
    }
  }

  ecma_collection_destroy (context_p, lit_pool_p);

  /* Sort the strings by size at first, then lexicographically. */
  jjs_save_literals_sort (context_p, literal_array, literal_count);

  if (is_c_format)
  {
    /* Save literal count. */
    lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "jjs_length_t literal_count = ", 0);

    lit_buf_p = jjs_append_number_to_buffer (lit_buf_p, buffer_end_p, literal_count);

    /* Save the array of literals. */
    lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, ";\n\njjs_char_t *literals[", 0);

    lit_buf_p = jjs_append_number_to_buffer (lit_buf_p, buffer_end_p, literal_count);
    lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "] =\n{\n", 0);

    for (lit_utf8_size_t i = 0; i < literal_count; i++)
    {
      lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "  \"", 0);
      ECMA_STRING_TO_UTF8_STRING (context_p, literal_array[i], str_buffer_p, str_buffer_size);
      for (lit_utf8_size_t j = 0; j < str_buffer_size; j++)
      {
        uint8_t byte = str_buffer_p[j];
        if (byte < 32 || byte > 127)
        {
          lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "\\x", 0);
          ecma_char_t hex_digit = (ecma_char_t) (byte >> 4);
          *lit_buf_p++ = (lit_utf8_byte_t) ((hex_digit > 9) ? (hex_digit + ('A' - 10)) : (hex_digit + '0'));
          hex_digit = (lit_utf8_byte_t) (byte & 0xf);
          *lit_buf_p++ = (lit_utf8_byte_t) ((hex_digit > 9) ? (hex_digit + ('A' - 10)) : (hex_digit + '0'));
        }
        else
        {
          if (byte == '\\' || byte == '"')
          {
            *lit_buf_p++ = '\\';
          }
          *lit_buf_p++ = byte;
        }
      }

      ECMA_FINALIZE_UTF8_STRING (context_p, str_buffer_p, str_buffer_size);
      lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "\"", 0);

      if (i < literal_count - 1)
      {
        lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, ",", 0);
      }

      lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "\n", 0);
    }

    lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "};\n\njjs_length_t literal_sizes[", 0);

    lit_buf_p = jjs_append_number_to_buffer (lit_buf_p, buffer_end_p, literal_count);
    lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "] =\n{\n", 0);
  }

  /* Save the literal sizes respectively. */
  for (lit_utf8_size_t i = 0; i < literal_count; i++)
  {
    lit_utf8_size_t str_size = ecma_string_get_size (context_p, literal_array[i]);

    if (is_c_format)
    {
      lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "  ", 0);
    }

    lit_buf_p = jjs_append_number_to_buffer (lit_buf_p, buffer_end_p, str_size);
    lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, " ", 0);

    if (is_c_format)
    {
      /* Show the given string as a comment. */
      lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "/* ", 0);
      lit_buf_p = jjs_append_ecma_string_to_buffer (context_p, lit_buf_p, buffer_end_p, literal_array[i]);
      lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, " */", 0);

      if (i < literal_count - 1)
      {
        lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, ",", 0);
      }
    }
    else
    {
      lit_buf_p = jjs_append_ecma_string_to_buffer (context_p, lit_buf_p, buffer_end_p, literal_array[i]);
    }

    lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "\n", 0);
  }

  if (is_c_format)
  {
    lit_buf_p = jjs_append_chars_to_buffer (lit_buf_p, buffer_end_p, "};\n", 0);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (context_p, literal_array);

  return lit_buf_p <= buffer_end_p ? (size_t) (lit_buf_p - buffer_start_p) : 0;
#else /* !JJS_SNAPSHOT_SAVE */
  JJS_UNUSED_ALL (context_p, snapshot_p, snapshot_size, lit_buf_p, lit_buf_size, is_c_format);
  return 0;
#endif /* JJS_SNAPSHOT_SAVE */
} /* jjs_get_literals_from_snapshot */

/**
 * Get all string literals from a snapshot.
 *
 * String literals are constant strings in the source code, such as key names, function
 * names and string constants.
 *
 * @return Array object containing an unordered list of string literals. On failure, an exception is returned.
 */
jjs_value_t
jjs_snapshot_get_string_literals (jjs_context_t* context_p, /**< JJS context */
                                  const uint32_t *snapshot_p, /**< snapshot */
                                  size_t snapshot_size) /**< size of snapshot in bytes */
{
#if JJS_SNAPSHOT_SAVE
  const uint8_t *snapshot_data_p = (uint8_t *) snapshot_p;
  const jjs_snapshot_header_t *header_p = (const jjs_snapshot_header_t *) snapshot_data_p;

  if (snapshot_size <= sizeof (jjs_snapshot_header_t) || header_p->magic != JJS_SNAPSHOT_MAGIC
      || header_p->version != JJS_SNAPSHOT_VERSION || !snapshot_check_global_flags (header_p->global_flags))
  {
    /* Invalid snapshot format */
    return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "invalid snapshot format");
  }

  JJS_ASSERT ((header_p->lit_table_offset % sizeof (uint32_t)) == 0);
  const uint8_t *literal_base_p = snapshot_data_p + header_p->lit_table_offset;

  ecma_collection_t *lit_pool_p = ecma_new_collection (context_p);
  scan_snapshot_functions (context_p, snapshot_data_p + header_p->func_offsets[0], literal_base_p, lit_pool_p, literal_base_p);

  lit_utf8_size_t literal_count = 0;
  ecma_value_t *buffer_p = lit_pool_p->buffer_p;

  /* Count the valid and non-magic identifiers in the list. */
  for (uint32_t i = 0; i < lit_pool_p->item_count; i++)
  {
    if (ecma_is_value_string (buffer_p[i]))
    {
      ecma_string_t *literal_p = ecma_get_string_from_value (context_p, buffer_p[i]);

      if (ecma_get_string_magic (literal_p) == LIT_MAGIC_STRING__COUNT)
      {
        literal_count++;
      }
    }
  }

  if (literal_count == 0)
  {
    ecma_collection_destroy (context_p, lit_pool_p);
    return jjs_array (context_p, 0);
  }

  lit_utf8_size_t literal_idx = 0;
  jjs_value_t result;

  buffer_p = lit_pool_p->buffer_p;

  JMEM_DEFINE_LOCAL_ARRAY (context_p, literal_array, literal_count, ecma_string_t *)

  /* Count the valid and non-magic identifiers in the list. */
  for (uint32_t i = 0; i < lit_pool_p->item_count; i++)
  {
    if (ecma_is_value_string (buffer_p[i]))
    {
      ecma_string_t *literal_p = ecma_get_string_from_value (context_p, buffer_p[i]);

      if (ecma_get_string_magic (literal_p) == LIT_MAGIC_STRING__COUNT)
      {
        literal_array[literal_idx++] = literal_p;
      }
    }
  }

  if (literal_idx > 0)
  {
    jjs_save_literals_sort (context_p, literal_array, literal_idx);
    result = jjs_array (context_p, literal_idx);

    for (uint32_t k = 0; k < literal_idx; k++)
    {
      ecma_value_t value = ecma_make_string_value (context_p, literal_array[k]);
      jjs_value_free (context_p, jjs_object_set_index (context_p, result, k, value, JJS_KEEP));
    }
  }
  else
  {
    result = jjs_array (context_p, 0);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (context_p, literal_array)

  ecma_collection_destroy (context_p, lit_pool_p);

  return result;
#else /* !JJS_SNAPSHOT_SAVE */
  JJS_UNUSED_ALL (context_p, snapshot_p, snapshot_size);
  return jjs_throw_sz (context_p, JJS_ERROR_COMMON, ecma_get_error_msg (ECMA_ERR_SNAPSHOT_SAVE_DISABLED));
#endif /* JJS_SNAPSHOT_SAVE */
}
