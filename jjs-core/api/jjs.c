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

#include "jjs.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#include "jjs-annex.h"
#include "jjs-context-init.h"
#include "jjs-debugger-transport.h"
#include "jjs-platform.h"

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-arraybuffer-object.h"
#include "ecma-bigint.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-comparison.h"
#include "ecma-container-object.h"
#include "ecma-dataview-object.h"
#include "ecma-errors.h"
#include "ecma-eval.h"
#include "ecma-exceptions.h"
#include "ecma-extended-info.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-init-finalize.h"
#include "ecma-iterator-object.h"
#include "ecma-lex-env.h"
#include "ecma-line-info.h"
#include "ecma-literal-storage.h"
#include "ecma-objects-general.h"
#include "ecma-objects.h"
#include "ecma-promise-object.h"
#include "ecma-proxy-object.h"
#include "ecma-regexp-object.h"
#include "ecma-shared-arraybuffer-object.h"
#include "ecma-symbol-object.h"
#include "ecma-typedarray-object.h"

#include "annex.h"
#include "debugger.h"
#include "jcontext.h"
#include "jmem.h"
#include "jrt.h"
#include "js-parser.h"
#include "lit-char-helpers.h"
#include "opcodes.h"
#include "re-compiler.h"

#include "config.h"

JJS_STATIC_ASSERT (sizeof (jjs_value_t) == sizeof (ecma_value_t),
                     size_of_jjs_value_t_must_be_equal_to_size_of_ecma_value_t);

#if JJS_BUILTIN_REGEXP
JJS_STATIC_ASSERT ((int) RE_FLAG_GLOBAL == (int) JJS_REGEXP_FLAG_GLOBAL
                       && (int) RE_FLAG_MULTILINE == (int) JJS_REGEXP_FLAG_MULTILINE
                       && (int) RE_FLAG_IGNORE_CASE == (int) JJS_REGEXP_FLAG_IGNORE_CASE
                       && (int) RE_FLAG_STICKY == (int) JJS_REGEXP_FLAG_STICKY
                       && (int) RE_FLAG_UNICODE == (int) JJS_REGEXP_FLAG_UNICODE
                       && (int) RE_FLAG_DOTALL == (int) JJS_REGEXP_FLAG_DOTALL,
                     re_flags_t_must_be_equal_to_jjs_regexp_flags_t);
#endif /* JJS_BUILTIN_REGEXP */

/* The internal ECMA_PROMISE_STATE_* values are "one byte away" from the API values */
JJS_STATIC_ASSERT ((int) ECMA_PROMISE_IS_PENDING == (int) JJS_PROMISE_STATE_PENDING
                       && (int) ECMA_PROMISE_IS_FULFILLED == (int) JJS_PROMISE_STATE_FULFILLED,
                     promise_internal_state_matches_external);

/**
 * Offset between internal and external arithmetic operator types
 */
#define ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET (JJS_BIN_OP_SUB - NUMBER_ARITHMETIC_SUBTRACTION)

JJS_STATIC_ASSERT (((NUMBER_ARITHMETIC_SUBTRACTION + ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET) == JJS_BIN_OP_SUB)
                       && ((NUMBER_ARITHMETIC_MULTIPLICATION + ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET)
                           == JJS_BIN_OP_MUL)
                       && ((NUMBER_ARITHMETIC_DIVISION + ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET) == JJS_BIN_OP_DIV)
                       && ((NUMBER_ARITHMETIC_REMAINDER + ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET) == JJS_BIN_OP_REM),
                     number_arithmetics_operation_type_matches_external);

#if !JJS_PARSER && !JJS_SNAPSHOT_EXEC
#error "JJS_SNAPSHOT_EXEC must be enabled if JJS_PARSER is disabled!"
#endif /* !JJS_PARSER && !JJS_SNAPSHOT_EXEC */

static void jjs_init_realm (ecma_value_t global);

#define STR2(S) #S
#define STR(S) STR2(S)
static const char* JJS_VERSION = STR (JJS_API_MAJOR_VERSION) "." STR (JJS_API_MINOR_VERSION) "." STR (JJS_API_PATCH_VERSION);
#undef STR
#undef STR2

/** \addtogroup jjs JJS engine interface
 * @{
 */

/**
 * Turn on API availability
 */
static inline void JJS_ATTR_ALWAYS_INLINE
jjs_api_enable (void)
{
#ifndef JJS_NDEBUG
  JJS_CONTEXT (status_flags) |= ECMA_STATUS_API_ENABLED;
#endif /* JJS_NDEBUG */
} /* jjs_make_api_available */

/**
 * Turn off API availability
 */
static inline void JJS_ATTR_ALWAYS_INLINE
jjs_api_disable (void)
{
#ifndef JJS_NDEBUG
  JJS_CONTEXT (status_flags) &= (uint32_t) ~ECMA_STATUS_API_ENABLED;
#endif /* JJS_NDEBUG */
} /* jjs_api_disable */

jjs_context_options_t
jjs_context_options (void)
{
  jjs_context_options_t opts;

  jjs_context_options_init (&opts);

  return opts;
} /* jjs_context_options */

/**
 * Initializes a jjs_context_options_t with defaults.
 *
 * Always use this function for jjs_context_options_t, as defaults are configured that
 * may or may not be available otherwise and jjs_init assumes jjs_context_options_t has
 * these defaults.
 *
 * After this function runs, you can add or remove jjs_context_options_t configuration to
 * suit your application's needs.
 *
 * @param opts context options to initialize
 * @return the opts argument
 */
jjs_context_options_t *
jjs_context_options_init (jjs_context_options_t * opts)
{
  JJS_ASSERT (opts != NULL);
  memset(opts, 0, sizeof (jjs_context_options_t));

  opts->vm_heap_size_kb = JJS_DEFAULT_VM_HEAP_SIZE;
  opts->vm_stack_limit_kb = JJS_DEFAULT_VM_STACK_LIMIT;
  opts->gc_limit_kb = JJS_DEFAULT_GC_LIMIT;
  opts->gc_mark_limit = JJS_DEFAULT_GC_MARK_LIMIT;
  opts->gc_new_objects_fraction = JJS_DEFAULT_GC_NEW_OBJECTS_FRACTION;

  opts->platform = jjsp_defaults ();

  return opts;
} /* jjs_context_options */

/**
 * Start JJS with context options.
 *
 * Use jjs_context_options_init () to init the context options and make your changes from
 * there. jjs_init () expects the context options to be fully populated and there are
 * quite a few options. Some defaults may not be available otherwise.
 *
 * @param opts context options. if NULL, (compile time set) default context options are used
 * @return JJS_CONTEXT_STATUS_OK or an error code on failure
 */
jjs_context_status_t
jjs_init (const jjs_context_options_t * opts)
{
  jjs_context_status_t status = jjs_context_init (opts);

  if (status != JJS_CONTEXT_STATUS_OK)
  {
    return status;
  }

  jjs_api_enable ();
  jmem_init ();
  ecma_init ();
  jjs_init_realm (ecma_make_object_value (ecma_builtin_get_global ()));
  jjs_annex_init ();
  jjs_annex_init_realm (JJS_CONTEXT (global_object_p));

  return JJS_CONTEXT_STATUS_OK;
} /* jjs_init */

/**
 * Start JJS with default context options.
 *
 * @return JJS_CONTEXT_STATUS_OK or an error code on failure
 */
jjs_context_status_t
jjs_init_default (void)
{
  return jjs_init (NULL);
} /* jjs_init_default */

jjs_context_status_t
jjs_init_with_flags (uint32_t context_flags)
{
  jjs_context_options_t opts = jjs_context_options ();

  opts.context_flags = context_flags;

  return jjs_init (&opts);
}

/**
 * Terminate JJS engine
 */
void
jjs_cleanup (void)
{
  jjs_assert_api_enabled ();

#if JJS_DEBUGGER
  if (JJS_CONTEXT (debugger_flags) & JJS_DEBUGGER_CONNECTED)
  {
    jjs_debugger_send_type (JJS_DEBUGGER_CLOSE_CONNECTION);

    jjs_debugger_transport_close ();
  }
#endif /* JJS_DEBUGGER */

  for (jjs_context_data_header_t *this_p = JJS_CONTEXT (context_data_p); this_p != NULL; this_p = this_p->next_p)
  {
    if (this_p->manager_p->deinit_cb)
    {
      void *data = (this_p->manager_p->bytes_needed > 0) ? JJS_CONTEXT_DATA_HEADER_USER_DATA (this_p) : NULL;
      this_p->manager_p->deinit_cb (data);
    }
  }

  ecma_free_all_enqueued_jobs ();
  jjs_annex_finalize ();
  ecma_finalize ();
  jjs_api_disable ();

  for (jjs_context_data_header_t *this_p = JJS_CONTEXT (context_data_p), *next_p = NULL; this_p != NULL;
       this_p = next_p)
  {
    next_p = this_p->next_p;

    if (this_p->manager_p->finalize_cb)
    {
      void *data = (this_p->manager_p->bytes_needed > 0) ? JJS_CONTEXT_DATA_HEADER_USER_DATA (this_p) : NULL;
      this_p->manager_p->finalize_cb (data);
    }

    jmem_heap_free_block (this_p, sizeof (jjs_context_data_header_t) + this_p->manager_p->bytes_needed);
  }

  jjs_context_cleanup ();
} /* jjs_cleanup */

/** jjs.cwd handler */
static JJS_HANDLER (jjs_api_cwd_handler)
{
  JJS_UNUSED_ALL (call_info_p, args_count, args_p);
  return jjs_platform_cwd ();
} /* jjs_api_cwd_handler */

/** jjs.realpath handler */
static JJS_HANDLER (jjs_api_realpath_handler)
{
  JJS_UNUSED_ALL (call_info_p);
  return jjs_platform_realpath (args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED, JJS_KEEP);
} /* jjs_api_realpath_handler */

/** jjs.gc handler */
static JJS_HANDLER (jjs_api_gc_handler)
{
  JJS_UNUSED_ALL (call_info_p);

  jjs_gc_mode_t mode =
    ((args_count > 0 && jjs_value_to_boolean (args_p[0])) ? JJS_GC_PRESSURE_HIGH : JJS_GC_PRESSURE_LOW);

  jjs_heap_gc (mode);

  return ECMA_VALUE_UNDEFINED;
} /* jjs_api_gc_handler */

/** jjs.readFile handler */
static JJS_HANDLER (jjs_api_read_file_handler)
{
  JJS_UNUSED_ALL (call_info_p, args_count, args_p);

  jjs_platform_read_file_options_t options = {
    .encoding = JJS_ENCODING_NONE,
  };

  /* extract encoding: string or { encoding: string } */
  jjs_value_t encoding;

  if (args_count > 1)
  {
    if (jjs_value_is_string (args_p[1]))
    {
      encoding = jjs_value_copy (args_p[1]);
    }
    else if (jjs_value_is_object (args_p[1]))
    {
      encoding = jjs_object_get_sz (args_p[1], "encoding");
    }
    else if (jjs_value_is_undefined(args_p[1]))
    {
      encoding = ECMA_VALUE_UNDEFINED;
    }
    else
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, "readFile expects encoding string or options object for argument 2");
    }
  }
  else
  {
    encoding = ECMA_VALUE_UNDEFINED;
  }

  /* encoding == encoding type */
  char buffer[8];

  if (jjs_value_is_string (encoding))
  {
    static const jjs_size_t size = (jjs_size_t) (sizeof (buffer) / sizeof (buffer[0]));
    jjs_value_t w = jjs_string_to_buffer (encoding, JJS_ENCODING_UTF8, (lit_utf8_byte_t*) &buffer[0], size - 1);

    JJS_ASSERT(w < size);
    buffer[w] = '\0';

    lit_utf8_byte_t* cursor_p = (lit_utf8_byte_t*) buffer;
    lit_utf8_byte_t c;

    while (*cursor_p != '\0')
    {
      c = *cursor_p;

      if (c <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
      {
        *cursor_p += (lit_utf8_byte_t) lit_char_to_lower_case (c, NULL);
      }

      cursor_p++;
    }

    if (strcmp (buffer, "utf8") == 0 || strcmp (buffer, "utf-8") == 0)
    {
      options.encoding = JJS_ENCODING_UTF8;
    }
    else if (strcmp (buffer, "cesu8") == 0)
    {
      options.encoding = JJS_ENCODING_CESU8;
    }
    else if (strcmp (buffer, "none") != 0)
    {
      jjs_value_free (encoding);
      return jjs_throw_sz (JJS_ERROR_TYPE, "invalid readFile encoding");
    }
  }

  jjs_value_free (encoding);

  return jjs_platform_read_file (args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED, JJS_KEEP, &options);
} /* jjs_api_read_file_handler */

/** Initialize realm with global jjs object. */
static void
jjs_init_realm (ecma_value_t global)
{
  jjs_value_t jjs = jjs_object ();
  ecma_object_t* jjs_p = ecma_get_object_from_value (jjs);

  annex_util_define_ro_value (jjs_p, LIT_MAGIC_STRING_VERSION, ecma_string_ascii_sz (JJS_VERSION), JJS_MOVE);
  annex_util_define_ro_value (jjs_p, LIT_MAGIC_STRING_OS, jjs_platform_os (), JJS_MOVE);
  annex_util_define_ro_value (jjs_p, LIT_MAGIC_STRING_ARCH, jjs_platform_arch (), JJS_MOVE);

  if (jjs_platform_has_cwd ())
  {
    annex_util_define_function (jjs_p, LIT_MAGIC_STRING_CWD, jjs_api_cwd_handler);
  }

  if (jjs_platform_has_realpath ())
  {
    annex_util_define_function (jjs_p, LIT_MAGIC_STRING_REALPATH, jjs_api_realpath_handler);
  }

  if (jjs_platform_has_read_file ())
  {
    annex_util_define_function (jjs_p, LIT_MAGIC_STRING_READ_FILE, jjs_api_read_file_handler);
  }

  if ((JJS_CONTEXT (context_flags) & JJS_CONTEXT_FLAG_EXPOSE_GC) != 0)
  {
    annex_util_define_function (jjs_p, LIT_MAGIC_STRING_GC, jjs_api_gc_handler);
  }

  ecma_set_m (global, LIT_MAGIC_STRING_JJS, jjs);

  jjs_value_free (jjs);
} /* jjs_init_realm */

/**
 * Retrieve a context data item, or create a new one.
 *
 * @param manager_p pointer to the manager whose context data item should be returned.
 *
 * @return a pointer to the user-provided context-specific data item for the given manager, creating such a pointer if
 * none was found.
 */
void *
jjs_context_data (const jjs_context_data_manager_t *manager_p)
{
  void *ret = NULL;
  jjs_context_data_header_t *item_p;

  for (item_p = JJS_CONTEXT (context_data_p); item_p != NULL; item_p = item_p->next_p)
  {
    if (item_p->manager_p == manager_p)
    {
      return (manager_p->bytes_needed > 0) ? JJS_CONTEXT_DATA_HEADER_USER_DATA (item_p) : NULL;
    }
  }

  item_p = jmem_heap_alloc_block (sizeof (jjs_context_data_header_t) + manager_p->bytes_needed);
  item_p->manager_p = manager_p;
  item_p->next_p = JJS_CONTEXT (context_data_p);
  JJS_CONTEXT (context_data_p) = item_p;

  if (manager_p->bytes_needed > 0)
  {
    ret = JJS_CONTEXT_DATA_HEADER_USER_DATA (item_p);
    memset (ret, 0, manager_p->bytes_needed);
  }

  if (manager_p->init_cb)
  {
    manager_p->init_cb (ret);
  }

  return ret;
} /* jjs_context_data */

/**
 * Register external magic string array
 */
void
jjs_register_magic_strings (const jjs_char_t *const *ext_strings_p, /**< character arrays, representing
                                                                         *   external magic strings' contents */
                              uint32_t count, /**< number of the strings */
                              const jjs_length_t *str_lengths_p) /**< lengths of all strings */
{
  jjs_assert_api_enabled ();

  lit_magic_strings_ex_set ((const lit_utf8_byte_t *const *) ext_strings_p,
                            count,
                            (const lit_utf8_size_t *) str_lengths_p);
} /* jjs_register_magic_strings */

/**
 * Run garbage collection
 */
void
jjs_heap_gc (jjs_gc_mode_t mode) /**< operational mode */
{
  jjs_assert_api_enabled ();

  if (mode == JJS_GC_PRESSURE_LOW)
  {
    /* Call GC directly, because 'ecma_free_unused_memory' might decide it's not yet worth it. */
    ecma_gc_run ();
    return;
  }

  ecma_free_unused_memory (JMEM_PRESSURE_HIGH);
} /* jjs_heap_gc */

/**
 * Get heap memory stats.
 *
 * @return true - get the heap stats successful
 *         false - otherwise. Usually it is because the MEM_STATS feature is not enabled.
 */
bool
jjs_heap_stats (jjs_heap_stats_t *out_stats_p) /**< [out] heap memory stats */
{
#if JJS_MEM_STATS
  if (out_stats_p == NULL)
  {
    return false;
  }

  jmem_heap_stats_t jmem_heap_stats;
  memset (&jmem_heap_stats, 0, sizeof (jmem_heap_stats));
  jmem_heap_get_stats (&jmem_heap_stats);

  *out_stats_p = (jjs_heap_stats_t){ .version = 1,
                                       .size = jmem_heap_stats.size,
                                       .allocated_bytes = jmem_heap_stats.allocated_bytes,
                                       .peak_allocated_bytes = jmem_heap_stats.peak_allocated_bytes };

  return true;
#else /* !JJS_MEM_STATS */
  JJS_UNUSED (out_stats_p);
  return false;
#endif /* JJS_MEM_STATS */
} /* jjs_heap_stats */

#if JJS_PARSER
/**
 * Common code for parsing a script, module, or function.
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
static jjs_value_t
jjs_parse_common (void *source_p, /**< script source */
                    const jjs_parse_options_t *options_p, /**< parsing options, can be NULL if not used */
                    uint32_t parse_opts) /**< internal parsing options */
{
  jjs_assert_api_enabled ();

  if (options_p != NULL)
  {
    const uint32_t allowed_options =
      (JJS_PARSE_STRICT_MODE | JJS_PARSE_MODULE | JJS_PARSE_HAS_ARGUMENT_LIST | JJS_PARSE_HAS_SOURCE_NAME
       | JJS_PARSE_HAS_START | JJS_PARSE_HAS_USER_VALUE);
    uint32_t options = options_p->options;

    if ((options & ~allowed_options) != 0
        || ((options_p->options & JJS_PARSE_HAS_ARGUMENT_LIST)
            && ((options_p->options & JJS_PARSE_MODULE) || !ecma_is_value_string (options_p->argument_list)))
        || ((options_p->options & JJS_PARSE_HAS_SOURCE_NAME) && !ecma_is_value_string (options_p->source_name)))
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
    }
  }

#if JJS_DEBUGGER
  if ((JJS_CONTEXT (debugger_flags) & JJS_DEBUGGER_CONNECTED) && options_p != NULL
      && (options_p->options & JJS_PARSE_HAS_SOURCE_NAME) && ecma_is_value_string (options_p->source_name))
  {
    ECMA_STRING_TO_UTF8_STRING (ecma_get_string_from_value (options_p->source_name),
                                source_name_start_p,
                                source_name_size);
    jjs_debugger_send_string (JJS_DEBUGGER_SOURCE_CODE_NAME,
                                JJS_DEBUGGER_NO_SUBTYPE,
                                source_name_start_p,
                                source_name_size);
    ECMA_FINALIZE_UTF8_STRING (source_name_start_p, source_name_size);
  }
#endif /* JJS_DEBUGGER */

  if (options_p != NULL)
  {
    parse_opts |= options_p->options & (JJS_PARSE_STRICT_MODE | JJS_PARSE_MODULE);
  }

  if ((parse_opts & JJS_PARSE_MODULE) != 0)
  {
#if JJS_MODULE_SYSTEM
    JJS_CONTEXT (module_current_p) = ecma_module_create ();
#else /* !JJS_MODULE_SYSTEM */
    return jjs_throw_sz (JJS_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JJS_MODULE_SYSTEM */
  }

  ecma_compiled_code_t *bytecode_data_p;
  bytecode_data_p = parser_parse_script (source_p, parse_opts, options_p);

  if (JJS_UNLIKELY (bytecode_data_p == NULL))
  {
#if JJS_MODULE_SYSTEM
    if ((parse_opts & JJS_PARSE_MODULE) != 0)
    {
      ecma_module_cleanup_context ();
    }
#endif /* JJS_MODULE_SYSTEM */

    return ecma_create_exception_from_context ();
  }

#if JJS_MODULE_SYSTEM
  if (JJS_UNLIKELY (parse_opts & JJS_PARSE_MODULE))
  {
    ecma_module_t *module_p = JJS_CONTEXT (module_current_p);
    module_p->u.compiled_code_p = bytecode_data_p;

    JJS_CONTEXT (module_current_p) = NULL;

    return ecma_make_object_value ((ecma_object_t *) module_p);
  }
#endif /* JJS_MODULE_SYSTEM */

  if (JJS_UNLIKELY (options_p != NULL && (options_p->options & JJS_PARSE_HAS_ARGUMENT_LIST)))
  {
    ecma_object_t *global_object_p = ecma_builtin_get_global ();

#if JJS_BUILTIN_REALMS
    JJS_ASSERT (global_object_p == (ecma_object_t *) ecma_op_function_get_realm (bytecode_data_p));
#endif /* JJS_BUILTIN_REALMS */

    ecma_object_t *lex_env_p = ecma_get_global_environment (global_object_p);
    ecma_object_t *func_obj_p = ecma_op_create_simple_function_object (lex_env_p, bytecode_data_p);
    ecma_bytecode_deref (bytecode_data_p);

    return ecma_make_object_value (func_obj_p);
  }

  ecma_object_t *object_p = ecma_create_object (NULL, sizeof (ecma_extended_object_t), ECMA_OBJECT_TYPE_CLASS);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.cls.type = ECMA_OBJECT_CLASS_SCRIPT;
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_object_p->u.cls.u3.value, bytecode_data_p);

  return ecma_make_object_value (object_p);
} /* jjs_parse_common */

#endif /* JJS_PARSER */

/**
 * Parse a script, module, or function and create a compiled code using a character string
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
jjs_value_t
jjs_parse (const jjs_char_t *source_p, /**< script source */
             size_t source_size, /**< script source size */
             const jjs_parse_options_t *options_p) /**< parsing options, can be NULL if not used */
{
#if JJS_PARSER
  parser_source_char_t source_char;
  source_char.source_p = source_p;
  source_char.source_size = source_size;

  return jjs_parse_common ((void *) &source_char, options_p, JJS_PARSE_NO_OPTS);
#else /* !JJS_PARSER */
  JJS_UNUSED (source_p);
  JJS_UNUSED (source_size);
  JJS_UNUSED (options_p);

  return jjs_throw_sz (JJS_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_PARSER_NOT_SUPPORTED));
#endif /* JJS_PARSER */
} /* jjs_parse */

/**
 * Parse a script, module, or function and create a compiled code using a string value
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
jjs_value_t
jjs_parse_value (const jjs_value_t source, /**< script source */
                   const jjs_parse_options_t *options_p) /**< parsing options, can be NULL if not used */
{
#if JJS_PARSER
  if (!ecma_is_value_string (source))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  return jjs_parse_common ((void *) &source, options_p, ECMA_PARSE_HAS_SOURCE_VALUE);
#else /* !JJS_PARSER */
  JJS_UNUSED (source);
  JJS_UNUSED (options_p);

  return jjs_throw_sz (JJS_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_PARSER_NOT_SUPPORTED));
#endif /* JJS_PARSER */
} /* jjs_parse_value */

/**
 * Run a Script or Module created by jjs_parse.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return result of bytecode - if run was successful
 *         thrown error - otherwise
 */
jjs_value_t
jjs_run (const jjs_value_t script) /**< script or module to run */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (script))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *object_p = ecma_get_object_from_value (script);

  if (!ecma_object_class_is (object_p, ECMA_OBJECT_CLASS_SCRIPT))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

  const ecma_compiled_code_t *bytecode_data_p;
  bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_compiled_code_t, ext_object_p->u.cls.u3.value);

  JJS_ASSERT (CBC_FUNCTION_GET_TYPE (bytecode_data_p->status_flags) == CBC_FUNCTION_SCRIPT);

  return jjs_return (vm_run_global (bytecode_data_p, object_p));
} /* jjs_run */

/**
 * Perform eval
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return result of eval, may be error value.
 */
jjs_value_t
jjs_eval (const jjs_char_t *source_p, /**< source code */
            size_t source_size, /**< length of source code */
            uint32_t flags) /**< jjs_parse_opts_t flags */
{
  jjs_assert_api_enabled ();

  uint32_t allowed_parse_options = JJS_PARSE_STRICT_MODE;

  if ((flags & ~allowed_parse_options) != 0)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  parser_source_char_t source_char;
  source_char.source_p = source_p;
  source_char.source_size = source_size;

  return jjs_return (ecma_op_eval_chars_buffer ((void *) &source_char, flags));
} /* jjs_eval */

/**
 * Run enqueued microtasks created by Promise or AsyncFunction objects.
 * Tasks are executed until an exception is thrown or all tasks are executed.
 *
 * Note: returned value must be freed with jjs_value_free
 *
 * @return result of last executed job, possibly an exception.
 */
jjs_value_t
jjs_run_jobs (void)
{
  jjs_assert_api_enabled ();

  return jjs_return (ecma_process_all_enqueued_jobs ());
} /* jjs_run_jobs */

bool
jjs_has_pending_jobs (void) {
  jjs_assert_api_enabled ();

  return ecma_has_enqueued_jobs ();
} /* jjs_has_pending_jobs */

/**
 * Get global object
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return api value of global object
 */
jjs_value_t
jjs_current_realm (void)
{
  jjs_assert_api_enabled ();
  ecma_object_t *global_obj_p = ecma_builtin_get_global ();
  ecma_ref_object (global_obj_p);
  return ecma_make_object_value (global_obj_p);
} /* jjs_current_realm */

/**
 * Check if the specified value is an abort value.
 *
 * @return true  - if both the error and abort values are set,
 *         false - otherwise
 */
bool
jjs_value_is_abort (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_exception (value))
  {
    return false;
  }

  ecma_extended_primitive_t *error_ref_p = ecma_get_extended_primitive_from_value (value);

  return (error_ref_p->refs_and_type & ECMA_ERROR_API_FLAG_ABORT) != 0;
} /* jjs_value_is_abort */

/**
 * Check if the specified value is an array object value.
 *
 * @return true  - if the specified value is an array object,
 *         false - otherwise
 */
bool
jjs_value_is_array (const jjs_value_t value) /**< JJS api value */
{
  jjs_assert_api_enabled ();

  return (ecma_is_value_object (value)
          && ecma_get_object_base_type (ecma_get_object_from_value (value)) == ECMA_OBJECT_BASE_TYPE_ARRAY);
} /* jjs_value_is_array */

/**
 * Check if the specified value is boolean.
 *
 * @return true  - if the specified value is boolean,
 *         false - otherwise
 */
bool
jjs_value_is_boolean (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return ecma_is_value_boolean (value);
} /* jjs_value_is_boolean */

/**
 * Check if the specified value is true.
 *
 * @return true  - if the specified value is true
 *         false - otherwise
 */
bool
jjs_value_is_true (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return ecma_is_value_true (value);
} /* jjs_value_is_true */

/**
 * Check if the specified value is false.
 *
 * @return true  - if the specified value is false
 *         false - otherwise
 */
bool
jjs_value_is_false (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return ecma_is_value_false (value);
} /* jjs_value_is_false */

/**
 * Check if the specified value is a constructor function object value.
 *
 * @return true - if the specified value is a function value that implements [[Construct]],
 *         false - otherwise
 */
bool
jjs_value_is_constructor (const jjs_value_t value) /**< JJS api value */
{
  jjs_assert_api_enabled ();

  return ecma_is_constructor (value);
} /* jjs_value_is_constructor */

/**
 * Check if the specified value is an error or abort value.
 *
 * @return true  - if the specified value is an error value,
 *         false - otherwise
 */
bool
jjs_value_is_exception (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return ecma_is_value_exception (value);
} /* jjs_value_is_exception */

/**
 * Check if the specified value is a function object value.
 *
 * @return true - if the specified value is callable,
 *         false - otherwise
 */
bool
jjs_value_is_function (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return ecma_op_is_callable (value);
} /* jjs_value_is_function */

/**
 * Check if the specified value is an async function object value.
 *
 * @return true - if the specified value is an async function,
 *         false - otherwise
 */
bool
jjs_value_is_async_function (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_object (value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (value);

    if (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
    {
      const ecma_compiled_code_t *bytecode_data_p;
      bytecode_data_p = ecma_op_function_get_compiled_code ((ecma_extended_object_t *) obj_p);
      uint16_t type = CBC_FUNCTION_GET_TYPE (bytecode_data_p->status_flags);

      return (type == CBC_FUNCTION_ASYNC || type == CBC_FUNCTION_ASYNC_ARROW || type == CBC_FUNCTION_ASYNC_GENERATOR);
    }
  }

  return false;
} /* jjs_value_is_async_function */

/**
 * Check if the specified value is number.
 *
 * @return true  - if the specified value is number,
 *         false - otherwise
 */
bool
jjs_value_is_number (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return ecma_is_value_number (value);
} /* jjs_value_is_number */

/**
 * Check if the specified value is null.
 *
 * @return true  - if the specified value is null,
 *         false - otherwise
 */
bool
jjs_value_is_null (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return ecma_is_value_null (value);
} /* jjs_value_is_null */

/**
 * Check if the specified value is object.
 *
 * @return true  - if the specified value is object,
 *         false - otherwise
 */
bool
jjs_value_is_object (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return ecma_is_value_object (value);
} /* jjs_value_is_object */

/**
 * Check if the specified value is promise.
 *
 * @return true  - if the specified value is promise,
 *         false - otherwise
 */
bool
jjs_value_is_promise (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return (ecma_is_value_object (value) && ecma_is_promise (ecma_get_object_from_value (value)));
} /* jjs_value_is_promise */

/**
 * Check if the specified value is a proxy object.
 *
 * @return true  - if the specified value is a proxy object,
 *         false - otherwise
 */
bool
jjs_value_is_proxy (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();
#if JJS_BUILTIN_PROXY
  return (ecma_is_value_object (value) && ECMA_OBJECT_IS_PROXY (ecma_get_object_from_value (value)));
#else /* !JJS_BUILTIN_PROXY */
  JJS_UNUSED (value);
  return false;
#endif /* JJS_BUILTIN_PROXY */
} /* jjs_value_is_proxy */

/**
 * Check if the specified value is string.
 *
 * @return true  - if the specified value is string,
 *         false - otherwise
 */
bool
jjs_value_is_string (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return ecma_is_value_string (value);
} /* jjs_value_is_string */

/**
 * Check if the specified value is symbol.
 *
 * @return true  - if the specified value is symbol,
 *         false - otherwise
 */
bool
jjs_value_is_symbol (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return ecma_is_value_symbol (value);
} /* jjs_value_is_symbol */

/**
 * Check if the specified value is BigInt.
 *
 * @return true  - if the specified value is BigInt,
 *         false - otherwise
 */
bool
jjs_value_is_bigint (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_BIGINT
  return ecma_is_value_bigint (value);
#else /* !JJS_BUILTIN_BIGINT */
  JJS_UNUSED (value);
  return false;
#endif /* JJS_BUILTIN_BIGINT */
} /* jjs_value_is_bigint */

/**
 * Check if the specified value is undefined.
 *
 * @return true  - if the specified value is undefined,
 *         false - otherwise
 */
bool
jjs_value_is_undefined (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  return ecma_is_value_undefined (value);
} /* jjs_value_is_undefined */

/**
 * Perform the base type of the JavaScript value.
 *
 * @return jjs_type_t value
 */
jjs_type_t
jjs_value_type (const jjs_value_t value) /**< input value to check */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return JJS_TYPE_EXCEPTION;
  }

  lit_magic_string_id_t lit_id = ecma_get_typeof_lit_id (value);

  JJS_ASSERT (lit_id != LIT_MAGIC_STRING__EMPTY);

  switch (lit_id)
  {
    case LIT_MAGIC_STRING_UNDEFINED:
    {
      return JJS_TYPE_UNDEFINED;
    }
    case LIT_MAGIC_STRING_BOOLEAN:
    {
      return JJS_TYPE_BOOLEAN;
    }
    case LIT_MAGIC_STRING_NUMBER:
    {
      return JJS_TYPE_NUMBER;
    }
    case LIT_MAGIC_STRING_STRING:
    {
      return JJS_TYPE_STRING;
    }
    case LIT_MAGIC_STRING_SYMBOL:
    {
      return JJS_TYPE_SYMBOL;
    }
    case LIT_MAGIC_STRING_FUNCTION:
    {
      return JJS_TYPE_FUNCTION;
    }
#if JJS_BUILTIN_BIGINT
    case LIT_MAGIC_STRING_BIGINT:
    {
      return JJS_TYPE_BIGINT;
    }
#endif /* JJS_BUILTIN_BIGINT */
    default:
    {
      JJS_ASSERT (lit_id == LIT_MAGIC_STRING_OBJECT);

      /* Based on the ECMA 262 5.1 standard the 'null' value is an object.
       * Thus we'll do an extra check for 'null' here.
       */
      return ecma_is_value_null (value) ? JJS_TYPE_NULL : JJS_TYPE_OBJECT;
    }
  }
} /* jjs_value_type */

/**
 * Used by jjs_object_type to get the type of class objects
 */
static const uint8_t jjs_class_object_type[] = {
  /* These objects require custom property resolving. */
  JJS_OBJECT_TYPE_STRING, /**< type of ECMA_OBJECT_CLASS_STRING */
  JJS_OBJECT_TYPE_ARGUMENTS, /**< type of ECMA_OBJECT_CLASS_ARGUMENTS */
#if JJS_BUILTIN_TYPEDARRAY
  JJS_OBJECT_TYPE_TYPEDARRAY, /**< type of ECMA_OBJECT_CLASS_TYPEDARRAY */
#endif /* JJS_BUILTIN_TYPEDARRAY */
#if JJS_MODULE_SYSTEM
  JJS_OBJECT_TYPE_MODULE_NAMESPACE, /**< type of ECMA_OBJECT_CLASS_MODULE_NAMESPACE */
#endif /* JJS_MODULE_SYSTEM */

  /* These objects are marked by Garbage Collector. */
  JJS_OBJECT_TYPE_GENERATOR, /**< type of ECMA_OBJECT_CLASS_GENERATOR */
  JJS_OBJECT_TYPE_GENERATOR, /**< type of ECMA_OBJECT_CLASS_ASYNC_GENERATOR */
  JJS_OBJECT_TYPE_ITERATOR, /**< type of ECMA_OBJECT_CLASS_ARRAY_ITERATOR */
  JJS_OBJECT_TYPE_ITERATOR, /**< type of ECMA_OBJECT_CLASS_SET_ITERATOR */
  JJS_OBJECT_TYPE_ITERATOR, /**< type of ECMA_OBJECT_CLASS_MAP_ITERATOR */
#if JJS_BUILTIN_REGEXP
  JJS_OBJECT_TYPE_ITERATOR, /**< type of ECMA_OBJECT_CLASS_REGEXP_STRING_ITERATOR */
#endif /* JJS_BUILTIN_REGEXP */
#if JJS_MODULE_SYSTEM
  JJS_OBJECT_TYPE_MODULE, /**< type of ECMA_OBJECT_CLASS_MODULE */
#endif /* JJS_MODULE_SYSTEM */
  JJS_OBJECT_TYPE_PROMISE, /**< type of ECMA_OBJECT_CLASS_PROMISE */
  JJS_OBJECT_TYPE_GENERIC, /**< type of ECMA_OBJECT_CLASS_PROMISE_CAPABILITY */
  JJS_OBJECT_TYPE_GENERIC, /**< type of ECMA_OBJECT_CLASS_ASYNC_FROM_SYNC_ITERATOR */
#if JJS_BUILTIN_DATAVIEW
  JJS_OBJECT_TYPE_DATAVIEW, /**< type of ECMA_OBJECT_CLASS_DATAVIEW */
#endif /* JJS_BUILTIN_DATAVIEW */
#if JJS_BUILTIN_CONTAINER
  JJS_OBJECT_TYPE_CONTAINER, /**< type of ECMA_OBJECT_CLASS_CONTAINER */
#endif /* JJS_BUILTIN_CONTAINER */

  /* Normal objects. */
  JJS_OBJECT_TYPE_BOOLEAN, /**< type of ECMA_OBJECT_CLASS_BOOLEAN */
  JJS_OBJECT_TYPE_NUMBER, /**< type of ECMA_OBJECT_CLASS_NUMBER */
  JJS_OBJECT_TYPE_ERROR, /**< type of ECMA_OBJECT_CLASS_ERROR */
  JJS_OBJECT_TYPE_GENERIC, /**< type of ECMA_OBJECT_CLASS_INTERNAL_OBJECT */
#if JJS_PARSER
  JJS_OBJECT_TYPE_SCRIPT, /**< type of ECMA_OBJECT_CLASS_SCRIPT */
#endif /* JJS_PARSER */
#if JJS_BUILTIN_DATE
  JJS_OBJECT_TYPE_DATE, /**< type of ECMA_OBJECT_CLASS_DATE */
#endif /* JJS_BUILTIN_DATE */
#if JJS_BUILTIN_REGEXP
  JJS_OBJECT_TYPE_REGEXP, /**< type of ECMA_OBJECT_CLASS_REGEXP */
#endif /* JJS_BUILTIN_REGEXP */
  JJS_OBJECT_TYPE_SYMBOL, /**< type of ECMA_OBJECT_CLASS_SYMBOL */
  JJS_OBJECT_TYPE_ITERATOR, /**< type of ECMA_OBJECT_CLASS_STRING_ITERATOR */
#if JJS_BUILTIN_TYPEDARRAY
  JJS_OBJECT_TYPE_ARRAYBUFFER, /**< type of ECMA_OBJECT_CLASS_ARRAY_BUFFER */
#if JJS_BUILTIN_SHAREDARRAYBUFFER
  JJS_OBJECT_TYPE_SHARED_ARRAY_BUFFER, /**< type of ECMA_OBJECT_CLASS_SHARED_ARRAY_BUFFER */
#endif /* JJS_BUILTIN_SHAREDARRAYBUFFER */
#endif /* JJS_BUILTIN_TYPEDARRAY */
#if JJS_BUILTIN_BIGINT
  JJS_OBJECT_TYPE_BIGINT, /**< type of ECMA_OBJECT_CLASS_BIGINT */
#endif /* JJS_BUILTIN_BIGINT */
#if JJS_BUILTIN_WEAKREF
  JJS_OBJECT_TYPE_WEAKREF, /**< type of ECMA_OBJECT_CLASS_WEAKREF */
#endif /* JJS_BUILTIN_WEAKREF */
};

JJS_STATIC_ASSERT (sizeof (jjs_class_object_type) == ECMA_OBJECT_CLASS__MAX,
                     jjs_class_object_type_must_have_object_class_max_elements);

/**
 * Get the object type of the given value
 *
 * @return JJS_OBJECT_TYPE_NONE - if the given value is not an object
 *         jjs_object_type_t value - otherwise
 */
jjs_object_type_t
jjs_object_type (const jjs_value_t value) /**< input value to check */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (value))
  {
    return JJS_OBJECT_TYPE_NONE;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (value);
  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

  switch (ecma_get_object_type (obj_p))
  {
    case ECMA_OBJECT_TYPE_CLASS:
    case ECMA_OBJECT_TYPE_BUILT_IN_CLASS:
    {
      JJS_ASSERT (ext_obj_p->u.cls.type < ECMA_OBJECT_CLASS__MAX);
      return jjs_class_object_type[ext_obj_p->u.cls.type];
    }
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_BUILT_IN_ARRAY:
    {
      return JJS_OBJECT_TYPE_ARRAY;
    }
    case ECMA_OBJECT_TYPE_PROXY:
    {
      return JJS_OBJECT_TYPE_PROXY;
    }
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_NATIVE_FUNCTION:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    {
      return JJS_OBJECT_TYPE_FUNCTION;
    }
    default:
    {
      break;
    }
  }

  return JJS_OBJECT_TYPE_GENERIC;
} /* jjs_object_type */

/**
 * Get the function type of the given value
 *
 * @return JJS_FUNCTION_TYPE_NONE - if the given value is not a function object
 *         jjs_function_type_t value - otherwise
 */
jjs_function_type_t
jjs_function_type (const jjs_value_t value) /**< input value to check */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_object (value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (value);
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    switch (ecma_get_object_type (obj_p))
    {
      case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
      {
        return JJS_FUNCTION_TYPE_BOUND;
      }
      case ECMA_OBJECT_TYPE_NATIVE_FUNCTION:
      case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
      {
        return JJS_FUNCTION_TYPE_GENERIC;
      }
      case ECMA_OBJECT_TYPE_FUNCTION:
      {
        const ecma_compiled_code_t *bytecode_data_p = ecma_op_function_get_compiled_code (ext_obj_p);

        switch (CBC_FUNCTION_GET_TYPE (bytecode_data_p->status_flags))
        {
          case CBC_FUNCTION_ARROW:
          case CBC_FUNCTION_ASYNC_ARROW:
          {
            return JJS_FUNCTION_TYPE_ARROW;
          }
          case CBC_FUNCTION_GENERATOR:
          case CBC_FUNCTION_ASYNC_GENERATOR:
          {
            return JJS_FUNCTION_TYPE_GENERATOR;
          }
          case CBC_FUNCTION_ACCESSOR:
          {
            return JJS_FUNCTION_TYPE_ACCESSOR;
          }
          default:
          {
            break;
          }
        }
        return JJS_FUNCTION_TYPE_GENERIC;
      }
      default:
      {
        break;
      }
    }
  }

  return JJS_FUNCTION_TYPE_NONE;
} /* jjs_function_type */

/**
 * Get the itearator type of the given value
 *
 * @return JJS_ITERATOR_TYPE_NONE - if the given value is not an iterator object
 *         jjs_iterator_type_t value - otherwise
 */
jjs_iterator_type_t
jjs_iterator_type (const jjs_value_t value) /**< input value to check */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_object (value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (value);
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_CLASS)
    {
      switch (ext_obj_p->u.cls.type)
      {
        case ECMA_OBJECT_CLASS_ARRAY_ITERATOR:
        {
          return JJS_ITERATOR_TYPE_ARRAY;
        }
#if JJS_BUILTIN_CONTAINER
        case ECMA_OBJECT_CLASS_SET_ITERATOR:
        {
          return JJS_ITERATOR_TYPE_SET;
        }
        case ECMA_OBJECT_CLASS_MAP_ITERATOR:
        {
          return JJS_ITERATOR_TYPE_MAP;
        }
#endif /* JJS_BUILTIN_CONTAINER */
        case ECMA_OBJECT_CLASS_STRING_ITERATOR:
        {
          return JJS_ITERATOR_TYPE_STRING;
        }
        default:
        {
          break;
        }
      }
    }
  }

  return JJS_ITERATOR_TYPE_NONE;
} /* jjs_iterator_type */

/**
 * Check if the specified feature is enabled.
 *
 * @return true  - if the specified feature is enabled,
 *         false - otherwise
 */
bool
jjs_feature_enabled (const jjs_feature_t feature) /**< feature to check */
{
  // note: each feature define is statically checked to be 0 or 1 in jjs-config.h
#define IS_FEATURE_ENABLED(F) ((F) != 0)
  switch (feature)
  {
    case JJS_FEATURE_CPOINTER_32_BIT:
      return IS_FEATURE_ENABLED (JJS_CPOINTER_32_BIT);
    case JJS_FEATURE_ERROR_MESSAGES:
      return IS_FEATURE_ENABLED (JJS_ERROR_MESSAGES);
    case JJS_FEATURE_JS_PARSER:
      return IS_FEATURE_ENABLED (JJS_PARSER);
    case JJS_FEATURE_HEAP_STATS:
      return IS_FEATURE_ENABLED (JJS_MEM_STATS);
    case JJS_FEATURE_PARSER_DUMP:
      return IS_FEATURE_ENABLED (JJS_PARSER_DUMP_BYTE_CODE);
    case JJS_FEATURE_REGEXP_DUMP:
      return IS_FEATURE_ENABLED (JJS_REGEXP_DUMP_BYTE_CODE);
    case JJS_FEATURE_SNAPSHOT_SAVE:
      return IS_FEATURE_ENABLED (JJS_SNAPSHOT_SAVE);
    case JJS_FEATURE_SNAPSHOT_EXEC:
      return IS_FEATURE_ENABLED (JJS_SNAPSHOT_EXEC);
    case JJS_FEATURE_DEBUGGER:
      return IS_FEATURE_ENABLED (JJS_DEBUGGER);
    case JJS_FEATURE_VM_EXEC_STOP:
      return IS_FEATURE_ENABLED (JJS_VM_HALT);
    case JJS_FEATURE_VM_THROW:
      return IS_FEATURE_ENABLED (JJS_VM_THROW);
    case JJS_FEATURE_JSON:
      return IS_FEATURE_ENABLED (JJS_BUILTIN_JSON);
    case JJS_FEATURE_TYPEDARRAY:
      return IS_FEATURE_ENABLED (JJS_BUILTIN_TYPEDARRAY);
    case JJS_FEATURE_DATAVIEW:
      return IS_FEATURE_ENABLED (JJS_BUILTIN_DATAVIEW);
    case JJS_FEATURE_PROXY:
      return IS_FEATURE_ENABLED (JJS_BUILTIN_PROXY);
    case JJS_FEATURE_DATE:
      return IS_FEATURE_ENABLED (JJS_BUILTIN_DATE);
    case JJS_FEATURE_REGEXP:
      return IS_FEATURE_ENABLED (JJS_BUILTIN_REGEXP);
    case JJS_FEATURE_LINE_INFO:
      return IS_FEATURE_ENABLED (JJS_LINE_INFO);
    case JJS_FEATURE_LOGGING:
      return IS_FEATURE_ENABLED (JJS_LOGGING);
    case JJS_FEATURE_GLOBAL_THIS:
      return IS_FEATURE_ENABLED (JJS_BUILTIN_GLOBAL_THIS);
    case JJS_FEATURE_MAP:
    case JJS_FEATURE_SET:
    case JJS_FEATURE_WEAKMAP:
    case JJS_FEATURE_WEAKSET:
      return IS_FEATURE_ENABLED (JJS_BUILTIN_CONTAINER);
    case JJS_FEATURE_WEAKREF:
      return IS_FEATURE_ENABLED (JJS_BUILTIN_WEAKREF);
    case JJS_FEATURE_BIGINT:
      return IS_FEATURE_ENABLED (JJS_BUILTIN_BIGINT);
    case JJS_FEATURE_REALM:
      return IS_FEATURE_ENABLED (JJS_BUILTIN_REALMS);
    case JJS_FEATURE_PROMISE_CALLBACK:
      return IS_FEATURE_ENABLED (JJS_PROMISE_CALLBACK);
    case JJS_FEATURE_MODULE:
      return IS_FEATURE_ENABLED (JJS_MODULE_SYSTEM);
    case JJS_FEATURE_FUNCTION_TO_STRING:
      return IS_FEATURE_ENABLED (JJS_FUNCTION_TO_STRING);
    case JJS_FEATURE_QUEUE_MICROTASK:
      return IS_FEATURE_ENABLED (JJS_ANNEX_QUEUE_MICROTASK);
    case JJS_FEATURE_COMMONJS:
      return IS_FEATURE_ENABLED (JJS_ANNEX_COMMONJS);
    case JJS_FEATURE_ESM:
      return IS_FEATURE_ENABLED (JJS_ANNEX_ESM);
    case JJS_FEATURE_PMAP:
      return IS_FEATURE_ENABLED (JJS_ANNEX_PMAP);
    case JJS_FEATURE_PROMISE:
    case JJS_FEATURE_SYMBOL:
      return true;
    case JJS_FEATURE_VMOD:
      return IS_FEATURE_ENABLED (JJS_ANNEX_VMOD);
    case JJS_FEATURE_VM_STACK_STATIC:
      return IS_FEATURE_ENABLED (JJS_VM_STACK_STATIC);
    case JJS_FEATURE_VM_HEAP_STATIC:
      return IS_FEATURE_ENABLED (JJS_VM_HEAP_STATIC);
    default:
      JJS_ASSERT (false);
      return false;
  }
#undef IS_FEATURE_ENABLED
} /* jjs_feature_enabled */

/**
 * Perform binary operation on the given operands (==, ===, <, >, etc.).
 *
 * @return error - if argument has an error flag or operation is unsuccessful or unsupported
 *         true/false - the result of the binary operation on the given operands otherwise
 */
jjs_value_t
jjs_binary_op (jjs_binary_op_t operation, /**< operation */
                 const jjs_value_t lhs, /**< first operand */
                 const jjs_value_t rhs) /**< second operand */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (lhs) || ecma_is_value_exception (rhs))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  switch (operation)
  {
    case JJS_BIN_OP_EQUAL:
    {
      return jjs_return (ecma_op_abstract_equality_compare (lhs, rhs));
    }
    case JJS_BIN_OP_STRICT_EQUAL:
    {
      return ecma_make_boolean_value (ecma_op_strict_equality_compare (lhs, rhs));
    }
    case JJS_BIN_OP_LESS:
    {
      return jjs_return (opfunc_relation (lhs, rhs, true, false));
    }
    case JJS_BIN_OP_LESS_EQUAL:
    {
      return jjs_return (opfunc_relation (lhs, rhs, false, true));
    }
    case JJS_BIN_OP_GREATER:
    {
      return jjs_return (opfunc_relation (lhs, rhs, false, false));
    }
    case JJS_BIN_OP_GREATER_EQUAL:
    {
      return jjs_return (opfunc_relation (lhs, rhs, true, true));
    }
    case JJS_BIN_OP_INSTANCEOF:
    {
      if (!ecma_is_value_object (lhs) || !ecma_op_is_callable (rhs))
      {
        return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
      }

      ecma_object_t *proto_obj_p = ecma_get_object_from_value (rhs);
      return jjs_return (ecma_op_object_has_instance (proto_obj_p, lhs));
    }
    case JJS_BIN_OP_ADD:
    {
      return jjs_return (opfunc_addition (lhs, rhs));
    }
    case JJS_BIN_OP_SUB:
    case JJS_BIN_OP_MUL:
    case JJS_BIN_OP_DIV:
    case JJS_BIN_OP_REM:
    {
      return jjs_return (do_number_arithmetic (operation - ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET, lhs, rhs));
    }
    default:
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_UNSUPPORTED_BINARY_OPERATION));
    }
  }
} /* jjs_binary_op */

/**
 * Create an abort value containing the argument value. If the second argument is true
 * the function will take ownership ofthe input value, otherwise the value will be copied.
 *
 * @return api abort value
 */
jjs_value_t
jjs_throw_abort (jjs_value_t value, /**< api value */
                   bool take_ownership) /**< release api value */
{
  jjs_assert_api_enabled ();

  if (JJS_UNLIKELY (ecma_is_value_exception (value)))
  {
    /* This is a rare case so it is optimized for
     * binary size rather than performance. */
    if (jjs_value_is_abort (value))
    {
      return take_ownership ? value : jjs_value_copy (value);
    }

    value = jjs_exception_value (value, take_ownership);
    take_ownership = true;
  }

  if (!take_ownership)
  {
    value = ecma_copy_value (value);
  }

  return ecma_create_exception (value, ECMA_ERROR_API_FLAG_ABORT);
} /* jjs_throw_abort */

/**
 * Create an exception value containing the argument value. If the second argument is true
 * the function will take ownership ofthe input value, otherwise the value will be copied.
 *
 * @return exception value
 */
jjs_value_t
jjs_throw_value (jjs_value_t value, /**< value */
                   bool take_ownership) /**< take ownership of the value */
{
  jjs_assert_api_enabled ();

  if (JJS_UNLIKELY (ecma_is_value_exception (value)))
  {
    /* This is a rare case so it is optimized for
     * binary size rather than performance. */
    if (!jjs_value_is_abort (value))
    {
      return take_ownership ? value : jjs_value_copy (value);
    }

    value = jjs_exception_value (value, take_ownership);
    take_ownership = true;
  }

  if (!take_ownership)
  {
    value = ecma_copy_value (value);
  }

  return ecma_create_exception (value, ECMA_ERROR_API_FLAG_NONE);
} /* jjs_throw_value */

/**
 * Get the value contained in an exception. If the second argument is true
 * it will release the argument exception value in the process.
 *
 * @return value in exception
 */
jjs_value_t
jjs_exception_value (jjs_value_t value, /**< api value */
                       bool free_exception) /**< release api value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_exception (value))
  {
    return free_exception ? value : ecma_copy_value (value);
  }

  jjs_value_t ret_val = jjs_value_copy (ecma_get_extended_primitive_from_value (value)->u.value);

  if (free_exception)
  {
    jjs_value_free (value);
  }
  return ret_val;
} /* jjs_exception_value */

/**
 * Set new decorator callback for Error objects. The decorator can
 * create or update any properties of the newly created Error object.
 */
void
jjs_error_on_created (jjs_error_object_created_cb_t callback, /**< new callback */
                        void *user_p) /**< user pointer passed to the callback */
{
  jjs_assert_api_enabled ();

  JJS_CONTEXT (error_object_created_callback_p) = callback;
  JJS_CONTEXT (error_object_created_callback_user_p) = user_p;
} /* jjs_error_on_created */

/**
 * When JJS_VM_THROW is enabled, the callback passed to this
 * function is called when an error is thrown in ECMAScript code.
 */
void
jjs_on_throw (jjs_throw_cb_t callback, /**< callback which is called on throws */
                void *user_p) /**< pointer passed to the function */
{
#if JJS_VM_THROW
  JJS_CONTEXT (vm_throw_callback_p) = callback;
  JJS_CONTEXT (vm_throw_callback_user_p) = user_p;
#else /* !JJS_VM_THROW */
  JJS_UNUSED (callback);
  JJS_UNUSED (user_p);
#endif /* JJS_VM_THROW */
} /* jjs_on_throw */

/**
 * Checks whether the callback set by jjs_on_throw captured the error
 *
 * @return true, if the vm throw callback captured the error
 *         false, otherwise
 */
bool
jjs_exception_is_captured (const jjs_value_t value) /**< exception value */
{
  jjs_assert_api_enabled ();

#if JJS_VM_THROW
  if (!ecma_is_value_exception (value))
  {
    return false;
  }

  ecma_extended_primitive_t *error_ref_p = ecma_get_extended_primitive_from_value (value);

  return (error_ref_p->refs_and_type & ECMA_ERROR_API_FLAG_THROW_CAPTURED) != 0;
#else /* !JJS_VM_THROW */
  JJS_UNUSED (value);
  return false;
#endif /* JJS_VM_THROW */
} /* jjs_exception_is_captured */

/**
 * Sets whether the callback set by jjs_on_throw should capture the exception or not
 */
void
jjs_exception_allow_capture (jjs_value_t value, /**< exception value */
                               bool should_capture) /**< callback should capture this error */
{
  jjs_assert_api_enabled ();

#if JJS_VM_THROW
  if (!ecma_is_value_exception (value))
  {
    return;
  }

  ecma_extended_primitive_t *error_ref_p = ecma_get_extended_primitive_from_value (value);

  if (should_capture)
  {
    error_ref_p->refs_and_type &= ~(uint32_t) ECMA_ERROR_API_FLAG_THROW_CAPTURED;
    return;
  }

  error_ref_p->refs_and_type |= ECMA_ERROR_API_FLAG_THROW_CAPTURED;
#else /* !JJS_VM_THROW */
  JJS_UNUSED (value);
  JJS_UNUSED (should_capture);
#endif /* JJS_VM_THROW */
} /* jjs_exception_allow_capture */

/**
 * Check if the given value is an Error object.
 *
 * @return true - if it is an Error object
 *         false - otherwise
 */
bool
jjs_value_is_error (const jjs_value_t value) /**< api value */
{
  return ecma_is_value_object (value)
         && ecma_object_class_is (ecma_get_object_from_value (value), ECMA_OBJECT_CLASS_ERROR);
} /* jjs_value_is_error */

/**
 * Return the type of the Error object if possible.
 *
 * @return one of the jjs_error_t value as the type of the Error object
 *         JJS_ERROR_NONE - if the input value is not an Error object
 */
jjs_error_t
jjs_error_type (jjs_value_t value) /**< api value */
{
  if (JJS_UNLIKELY (ecma_is_value_exception (value)))
  {
    value = ecma_get_extended_primitive_from_value (value)->u.value;
  }

  if (!ecma_is_value_object (value))
  {
    return JJS_ERROR_NONE;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (value);
  /* TODO(check if error object) */
  jjs_error_t error_type = ecma_get_error_type (object_p);

  return (jjs_error_t) error_type;
} /* jjs_error_type */

/**
 * Get number from the specified value as a double.
 *
 * @return stored number as double
 */
double
jjs_value_as_number (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_number (value))
  {
    return 0;
  }

  return (double) ecma_get_number_from_value (value);
} /* jjs_value_as_number */

/**
 * Call ToBoolean operation on the api value.
 *
 * @return true  - if the logical value is true
 *         false - otherwise
 */
bool
jjs_value_to_boolean (const jjs_value_t value) /**< input value */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return false;
  }

  return ecma_op_to_boolean (value);
} /* jjs_value_to_boolean */

/**
 * Call ToNumber operation on the api value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return converted number value - if success
 *         thrown error - otherwise
 */
jjs_value_t
jjs_value_to_number (const jjs_value_t value) /**< input value */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  ecma_number_t num;
  ecma_value_t ret_value = ecma_op_to_number (value, &num);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ecma_create_exception_from_context ();
  }

  return ecma_make_number_value (num);
} /* jjs_value_to_number */

/**
 * Call ToObject operation on the api value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return converted object value - if success
 *         thrown error - otherwise
 */
jjs_value_t
jjs_value_to_object (const jjs_value_t value) /**< input value */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  return jjs_return (ecma_op_to_object (value));
} /* jjs_value_to_object */

/**
 * Call ToPrimitive operation on the api value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return converted primitive value - if success
 *         thrown error - otherwise
 */
jjs_value_t
jjs_value_to_primitive (const jjs_value_t value) /**< input value */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  return jjs_return (ecma_op_to_primitive (value, ECMA_PREFERRED_TYPE_NO));
} /* jjs_value_to_primitive */

/**
 * Call the ToString ecma builtin operation on the api value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return converted string value - if success
 *         thrown error - otherwise
 */
jjs_value_t
jjs_value_to_string (const jjs_value_t value) /**< input value */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  ecma_string_t *str_p = ecma_op_to_string (value);
  if (JJS_UNLIKELY (str_p == NULL))
  {
    return ecma_create_exception_from_context ();
  }

  return ecma_make_string_value (str_p);
} /* jjs_value_to_string */

/**
 * Call the BigInt constructor ecma builtin operation on the api value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return BigInt value - if success
 *         thrown error - otherwise
 */
jjs_value_t
jjs_value_to_bigint (const jjs_value_t value) /**< input value */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_BIGINT
  if (ecma_is_value_exception (value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  return jjs_return (ecma_bigint_to_bigint (value, true));
#else /* !JJS_BUILTIN_BIGINT */
  JJS_UNUSED (value);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_BIGINT_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_BIGINT */
} /* jjs_value_to_bigint */

/**
 * Convert any number to integer number.
 *
 * Note:
 *      For non-number values 0 is returned.
 *
 * @return integer representation of the number.
 */
double
jjs_value_as_integer (const jjs_value_t value) /**< input value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_number (value))
  {
    return 0;
  }

  double number = ecma_get_number_from_value (value);

  if (ecma_number_is_nan (number))
  {
    return ECMA_NUMBER_ZERO;
  }

  if (ecma_number_is_zero (number) || ecma_number_is_infinity (number))
  {
    return number;
  }

  ecma_number_t floor_fabs = (ecma_number_t) floor (fabs (number));

  return ecma_number_is_negative (number) ? -floor_fabs : floor_fabs;
} /* jjs_value_as_integer */

/**
 * Convert any number to int32 number.
 *
 * Note:
 *      For non-number values 0 is returned.
 *
 * @return int32 representation of the number.
 */
int32_t
jjs_value_as_int32 (const jjs_value_t value) /**< input value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_number (value))
  {
    return 0;
  }

  return ecma_number_to_int32 (ecma_get_number_from_value (value));
} /* jjs_value_as_int32 */

/**
 * Convert any number to uint32 number.
 *
 * Note:
 *      For non-number values 0 is returned.
 *
 * @return uint32 representation of the number.
 */
uint32_t
jjs_value_as_uint32 (const jjs_value_t value) /**< input value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_number (value))
  {
    return 0;
  }

  return ecma_number_to_uint32 (ecma_get_number_from_value (value));
} /* jjs_value_as_uint32 */

/**
 * Get number from the specified value as a float.
 *
 * @return stored number as float
 */
float
jjs_value_as_float (const jjs_value_t value)  /**< api value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_number (value))
  {
    return 0;
  }

#if JJS_NUMBER_TYPE_FLOAT64
  return (float) ecma_get_number_from_value (value);
#else
  return ecma_get_number_from_value (value);
#endif
} /* jjs_value_as_float */

/**
 * Get number from the specified value as a double.
 *
 * @return stored number as double
 */
double
jjs_value_as_double (const jjs_value_t value)  /**< api value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_number (value))
  {
    return 0;
  }

#if JJS_NUMBER_TYPE_FLOAT64
  return ecma_get_number_from_value (value);
#else
  return (double) ecma_get_number_from_value (value);
#endif
} /* jjs_value_as_double */

/**
 * Take additional ownership over the argument value.
 * The value will be copied by reference when possible, changes made to the new value will be reflected
 * in the original.
 *
 * @return copied value
 */
jjs_value_t
jjs_value_copy (const jjs_value_t value) /**< value */
{
  jjs_assert_api_enabled ();

  if (JJS_UNLIKELY (ecma_is_value_exception (value)))
  {
    ecma_ref_extended_primitive (ecma_get_extended_primitive_from_value (value));
    return value;
  }

  return ecma_copy_value (value);
} /* jjs_value_copy */

/**
 * Release ownership of the argument value
 */
void
jjs_value_free (jjs_value_t value) /**< value */
{
  jjs_assert_api_enabled ();

  if (JJS_UNLIKELY (ecma_is_value_exception (value)))
  {
    ecma_deref_exception (ecma_get_extended_primitive_from_value (value));
    return;
  }

  ecma_free_value (value);
} /* jjs_value_free */

/**
 * Release ownership of the argument value, unless the condition function
 * return true.
 *
 * This function is for a common pattern of api usage. The value should be
 * released unless the value satisfies a condition, like jjs_value_is_exception. If
 * the condition is satisfied, you may want to take additional steps like
 * logging or chaning control flow before releasing the value.
 *
 * @param value the value to release
 * @param condition_fn condition function to apply to the value
 * @return true: value was released; false: value was not released
 */
bool jjs_value_free_unless (jjs_value_t value, jjs_value_condition_fn_t condition_fn)
{
  jjs_assert_api_enabled ();
  JJS_ASSERT (condition_fn);

  if (condition_fn (value))
  {
    return false;
  }

  jjs_value_free (value);

  return true;
} /* jjs_value_free_unless */

/**
 * Create an array object value
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return value of the constructed array object
 */
jjs_value_t
jjs_array (jjs_length_t length) /**< length of array */
{
  jjs_assert_api_enabled ();

  ecma_object_t *array_p = ecma_op_new_array_object (length);
  return ecma_make_object_value (array_p);
} /* jjs_array */

/**
 * Create a jjs_value_t representing a boolean value from the given boolean parameter.
 *
 * @return value of the created boolean
 */
jjs_value_t
jjs_boolean (bool value) /**< bool value from which a jjs_value_t will be created */
{
  jjs_assert_api_enabled ();

  return ecma_make_boolean_value (value);
} /* jjs_boolean */

/**
 * Create an Error object with the provided string value as the error message.
 * If the message value is not a string, the created error will not have a message property.
 *
 * @return Error object
 */
jjs_value_t
jjs_error (jjs_error_t error_type, /**< type of error */
             const jjs_value_t message, /**< message of the error */
             const jjs_value_t options)  /**< options */
{
  jjs_assert_api_enabled ();

  ecma_string_t *message_p = NULL;
  if (ecma_is_value_string (message))
  {
    message_p = ecma_get_string_from_value (message);
  }

  ecma_object_t *error_object_p = ecma_new_standard_error_with_options((jjs_error_t) error_type, message_p, options);

  return ecma_make_object_value (error_object_p);
} /* jjs_error */

/**
 * Create an Error object with a zero-terminated string as a message. If the message string is NULL, the created error
 * will not have a message property.
 *
 * @return Error object
 */
jjs_value_t
jjs_error_sz (jjs_error_t error_type, /**< type of error */
                const char *message_p, /**< message of the error */
                const jjs_value_t options)  /**< options */
{
  jjs_value_t message = ECMA_VALUE_UNDEFINED;

  if (message_p != NULL)
  {
    message = jjs_string_sz (message_p);
  }

  ecma_value_t error = jjs_error (error_type, message, options);
  ecma_free_value (message);

  return error;
} /* jjs_error_sz */

/**
 * Create an AggregateError object.
 *
 * If the errors argument is not iterable, an exception will be returned.
 *
 * The message argument will be toString()'d.
 *
 * If the options argument is an object containing a "cause" property, this cause property will be
 * copied to the new error object. Otherwise, the options argument is ignored.
 *
 * @param errors iterable value, like an Array or Set
 * @param message_p value to set as message
 * @param options optional options object containing "cause"
 * @return AggregateError object or exception if errors is not iterable
 */
jjs_value_t jjs_aggregate_error (const jjs_value_t errors, const jjs_value_t message, const jjs_value_t options) {
  jjs_assert_api_enabled ();

  return ecma_new_aggregate_error(errors, message, options);
} /* jjs_aggregate_error */

/**
 * Create an AggregateError object with a zero-terminated string as a message.
 *
 * If the errors argument is not iterable, an exception will be returned.
 *
 * If the options argument is an object containing a "cause" property, this cause property will be
 * copied to the new error object. Otherwise, the options argument is ignored.
 *
 * @param errors iterable value, like an Array or Set
 * @param message_p null terminated string message
 * @param options optional options object containing "cause"
 * @return AggregateError object or exception if errors is not iterable
 */
jjs_value_t jjs_aggregate_error_sz (const jjs_value_t errors, const char *message_p, const jjs_value_t options) {
  jjs_value_t message = ECMA_VALUE_UNDEFINED;

  if (message_p != NULL)
  {
    message = jjs_string_sz (message_p);
  }

  ecma_value_t error = jjs_aggregate_error (errors, message, options);
  ecma_free_value (message);

  return error;
} /* jjs_aggregate_error_sz */

/**
 * Create an exception by constructing an Error object with the specified type and the provided string value as the
 * error message.  If the message value is not a string, the created error will not have a message property.
 *
 * @return exception value
 */
jjs_value_t
jjs_throw (jjs_error_t error_type, /**< type of error */
             const jjs_value_t message) /**< message value */
{
  return jjs_throw_value (jjs_error (error_type, message, ECMA_VALUE_UNDEFINED), true);
} /* jjs_throw */

/**
 * Create an exception by constructing an Error object with the specified type and the provided zero-terminated ASCII
 * string as the error message.  If the message string is NULL, the created error will not have a message property.
 *
 * @return exception value
 */
jjs_value_t
jjs_throw_sz (jjs_error_t error_type, /**< type of error */
                const char *message_p) /**< value of 'message' property
                                        *   of constructed error object */
{
  return jjs_throw_value (jjs_error_sz (error_type, message_p, ECMA_VALUE_UNDEFINED), true);
} /* jjs_throw_sz */

/**
 * Create an external function object
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return value of the constructed function object
 */
jjs_value_t
jjs_function_external (jjs_external_handler_t handler) /**< native handler
                                                            *   for the function */
{
  jjs_assert_api_enabled ();

  ecma_object_t *func_obj_p = ecma_op_create_external_function_object (handler);
  return ecma_make_object_value (func_obj_p);
} /* jjs_function_external */

/**
 * Creates a jjs_value_t representing a number value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return jjs_value_t created from the given double argument.
 */
jjs_value_t
jjs_number (double value) /**< double value from which a jjs_value_t will be created */
{
  jjs_assert_api_enabled ();

  return ecma_make_number_value ((ecma_number_t) value);
} /* jjs_number */

/**
 * Creates a jjs_value_t representing a positive or negative infinity value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return jjs_value_t representing an infinity value.
 */
jjs_value_t
jjs_infinity (bool sign) /**< true for negative Infinity
                            *   false for positive Infinity */
{
  jjs_assert_api_enabled ();

  return ecma_make_number_value (ecma_number_make_infinity (sign));
} /* jjs_infinity */

/**
 * Creates a jjs_value_t representing a not-a-number value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return jjs_value_t representing a not-a-number value.
 */
jjs_value_t
jjs_nan (void)
{
  jjs_assert_api_enabled ();

  return ecma_make_nan_value ();
} /* jjs_nan */

/**
 * Creates a jjs_value_t representing a number value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return jjs_value_t created from the given float argument.
 */
jjs_value_t jjs_number_from_float (float value) /**< float value from which a jjs_value_t will be created */
{
  jjs_assert_api_enabled ();

#if JJS_NUMBER_TYPE_FLOAT64
  return ecma_make_number_value ((ecma_number_t) value);
#else
  return ecma_make_number_value (value);
#endif
} /* jjs_number_from_float */

/**
 * Creates a jjs_value_t representing a number value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return jjs_value_t created from the given double argument.
 */
jjs_value_t jjs_number_from_double (double value) /**< double value from which a jjs_value_t will be created */
{
  jjs_assert_api_enabled ();

#if JJS_NUMBER_TYPE_FLOAT64
  return ecma_make_number_value (value);
#else
  return ecma_make_number_value ((ecma_number_t) value);
#endif
} /* jjs_number_from_double */

/**
 * Creates a jjs_value_t representing a number value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return jjs_value_t created from the given integer argument.
 */
jjs_value_t jjs_number_from_int32 (int32_t value) /**< int value from which a jjs_value_t will be created */
{
  jjs_assert_api_enabled ();

  return ecma_make_int32_value (value);
} /* jjs_number_from_int32 */

/**
 * Creates a jjs_value_t representing a number value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return jjs_value_t created from the given unsigned argument.
 */
jjs_value_t jjs_number_from_uint32 (uint32_t value) /**< unsigned value from which a jjs_value_t will be created */
{
  jjs_assert_api_enabled ();

  return ecma_make_uint32_value (value);
} /* jjs_number_from_uint32 */

/**
 * Creates a jjs_value_t representing an undefined value.
 *
 * @return value of undefined
 */
jjs_value_t
jjs_undefined (void)
{
  jjs_assert_api_enabled ();

  return ECMA_VALUE_UNDEFINED;
} /* jjs_undefined */

/**
 * Creates and returns a jjs_value_t with type null object.
 *
 * @return jjs_value_t representing null
 */
jjs_value_t
jjs_null (void)
{
  jjs_assert_api_enabled ();

  return ECMA_VALUE_NULL;
} /* jjs_null */

/**
 * Create new JavaScript object, like with new Object().
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return value of the created object
 */
jjs_value_t
jjs_object (void)
{
  jjs_assert_api_enabled ();

  return ecma_make_object_value (ecma_op_create_object_object_noarg ());
} /* jjs_object */

/**
 * Create an empty Promise object which can be resolved/rejected later
 * by calling jjs_promise_resolve or jjs_promise_reject.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return value of the created object
 */
jjs_value_t
jjs_promise (void)
{
  jjs_assert_api_enabled ();

  return jjs_return (ecma_op_create_promise_object (ECMA_VALUE_EMPTY, ECMA_VALUE_UNDEFINED, NULL));
} /* jjs_create_promise */

/**
 * Create a new Proxy object with the given target and handler
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return value of the created Proxy object
 */
jjs_value_t
jjs_proxy (const jjs_value_t target, /**< target argument */
             const jjs_value_t handler) /**< handler argument */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (target) || ecma_is_value_exception (handler))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

#if JJS_BUILTIN_PROXY
  ecma_object_t *proxy_p = ecma_proxy_create (target, handler, 0);

  if (proxy_p == NULL)
  {
    return ecma_create_exception_from_context ();
  }

  return ecma_make_object_value (proxy_p);
#else /* !JJS_BUILTIN_PROXY */
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PROXY_IS_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_PROXY */
} /* jjs_proxy */

#if JJS_BUILTIN_PROXY

JJS_STATIC_ASSERT ((int) JJS_PROXY_SKIP_RESULT_VALIDATION == (int) ECMA_PROXY_SKIP_RESULT_VALIDATION,
                     jjs_and_ecma_proxy_skip_result_validation_must_be_equal);

#endif /* JJS_BUILTIN_PROXY */

/**
 * Create a new Proxy object with the given target, handler, and special options
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return value of the created Proxy object
 */
jjs_value_t
jjs_proxy_custom (const jjs_value_t target, /**< target argument */
                    const jjs_value_t handler, /**< handler argument */
                    uint32_t flags) /**< jjs_proxy_custom_behavior_t option bits */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (target) || ecma_is_value_exception (handler))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

#if JJS_BUILTIN_PROXY
  flags &= JJS_PROXY_SKIP_RESULT_VALIDATION;

  ecma_object_t *proxy_p = ecma_proxy_create (target, handler, flags);

  if (proxy_p == NULL)
  {
    return ecma_create_exception_from_context ();
  }

  return ecma_make_object_value (proxy_p);
#else /* !JJS_BUILTIN_PROXY */
  JJS_UNUSED (flags);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PROXY_IS_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_PROXY */
} /* jjs_proxy_custom */

/**
 * Create string value from the input zero-terminated ASCII string.
 *
 * @return created string
 */
jjs_value_t
jjs_string_sz (const char *str_p) /**< pointer to string */
{
  const jjs_char_t *data_p = (const jjs_char_t *) str_p;
  return jjs_string (data_p, lit_zt_utf8_string_size (data_p), JJS_ENCODING_CESU8);
} /* jjs_string_sz */

/**
 * Creates JJS string from a null-terminated, UTF-8 encoded C string.
 *
 * Shorthand for jjs_string with JJS_ENCODING_UTF8 encoding.
 *
 * @return created string
 */
jjs_value_t
jjs_string_utf8_sz (const char *str_p) /**< pointer to string */
{
  return jjs_string ((const jjs_char_t *) str_p, (jjs_size_t) strlen (str_p), JJS_ENCODING_UTF8);
} /* jjs_string_utf8_sz */

/**
 * Creates JJS string from a null-terminated, CESU8 encoded C string.
 *
 * Shorthand for jjs_string with JJS_ENCODING_CESU8 encoding.
 *
 * @return created string
 */
jjs_value_t
jjs_string_cesu8_sz (const char *str_p) /**< pointer to string */
{
  return jjs_string ((const jjs_char_t *) str_p, (jjs_size_t) strlen (str_p), JJS_ENCODING_CESU8);
} /* jjs_string_cesu8_sz */

/**
 * Create a string value from the input buffer using the specified encoding.
 * The content of the buffer is assumed to be valid in the specified encoding, it's the callers responsibility to
 * validate the input.
 *
 * See also: jjs_validate_string
 *
 * @return created string
 */
jjs_value_t
jjs_string (const jjs_char_t *buffer_p, /**< pointer to buffer */
              jjs_size_t buffer_size, /**< buffer size */
              jjs_encoding_t encoding) /**< buffer encoding */
{
  jjs_assert_api_enabled ();
  ecma_string_t *ecma_str_p = NULL;
  JJS_ASSERT (jjs_validate_string (buffer_p, buffer_size, encoding));

  switch (encoding)
  {
    case JJS_ENCODING_CESU8:
    {
      ecma_str_p = ecma_new_ecma_string_from_utf8 (buffer_p, buffer_size);
      break;
    }
    case JJS_ENCODING_UTF8:
    {
      ecma_str_p = ecma_new_ecma_string_from_utf8_converted_to_cesu8 (buffer_p, buffer_size);
      break;
    }
    default:
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INVALID_ENCODING));
    }
  }

  return ecma_make_string_value (ecma_str_p);
} /* jjs_string */

/**
 * Create external string from input zero-terminated ASCII string.
 *
 * @return created external string
 */
jjs_value_t
jjs_string_external_sz (const char *str_p, /**< pointer to string */
                          void *user_p) /**< user pointer passed to the callback when the string is freed */
{
  const jjs_char_t *data_p = (const jjs_char_t *) str_p;
  return jjs_string_external (data_p, lit_zt_utf8_string_size (data_p), user_p);
} /* jjs_string_external_sz */

/**
 * Create external string from a valid CESU-8 encoded string.
 * The content of the buffer is assumed be encoded correctly, it's the callers responsibility to
 * validate the input.
 *
 * See also: jjs_validate_string
 *
 * @return created external string
 */
jjs_value_t
jjs_string_external (const jjs_char_t *buffer_p, /**< pointer to string */
                       jjs_size_t buffer_size, /**< string size */
                       void *user_p) /**< user pointer passed to the callback when the string is freed */
{
  jjs_assert_api_enabled ();

  JJS_ASSERT (jjs_validate_string (buffer_p, buffer_size, JJS_ENCODING_CESU8));
  ecma_string_t *ecma_str_p = ecma_new_ecma_external_string_from_cesu8 (buffer_p, buffer_size, user_p);
  return ecma_make_string_value (ecma_str_p);
} /* jjs_string_external_sz_sz */

/**
 * Create symbol with a description value
 *
 * Note: The given argument is converted to string. This operation can throw an exception.
 *
 * @return created symbol,
 *         or thrown exception
 */
jjs_value_t
jjs_symbol_with_description (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  return jjs_return (ecma_op_create_symbol (&value, 1));
} /* jjs_create_symbol */

/**
 * Create BigInt from a sequence of uint64 digits.
 *
 * Note: This operation can throw an exception.
 *
 * @return created bigint,
 *         or thrown exception
 */
jjs_value_t
jjs_bigint (const uint64_t *digits_p, /**< BigInt digits (lowest digit first) */
              uint32_t digit_count, /**< number of BigInt digits */
              bool sign) /**< sign bit, true if the result should be negative */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_BIGINT
  return jjs_return (ecma_bigint_create_from_digits (digits_p, digit_count, sign));
#else /* !JJS_BUILTIN_BIGINT */
  JJS_UNUSED (digits_p);
  JJS_UNUSED (digit_count);
  JJS_UNUSED (sign);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_BIGINT_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_BIGINT */
} /* jjs_bigint */

/**
 * Creates a RegExp object with the given ASCII pattern and flags.
 *
 * @return value of the constructed RegExp object.
 */
jjs_value_t
jjs_regexp_sz (const char *pattern_p, /**< RegExp pattern as zero-terminated ASCII string */
                 uint16_t flags) /**< RegExp flags */
{
  jjs_assert_api_enabled ();

  jjs_value_t pattern = jjs_string_sz (pattern_p);
  jjs_value_t result = jjs_regexp (pattern, flags);

  jjs_value_free (pattern);
  return jjs_return (result);
} /* jjs_regexp_sz */

/**
 * Creates a RegExp object with the given pattern and flags.
 *
 * @return value of the constructed RegExp object.
 */
jjs_value_t
jjs_regexp (const jjs_value_t pattern, /**< pattern string */
              uint16_t flags) /**< RegExp flags */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_REGEXP
  if (!ecma_is_value_string (pattern))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *regexp_obj_p = ecma_op_regexp_alloc (NULL);

  if (JJS_UNLIKELY (regexp_obj_p == NULL))
  {
    return ecma_create_exception_from_context ();
  }

  jjs_value_t result = ecma_op_create_regexp_with_flags (regexp_obj_p, pattern, flags);

  return jjs_return (result);

#else /* !JJS_BUILTIN_REGEXP */
  JJS_UNUSED (pattern);
  JJS_UNUSED (flags);

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_REGEXP_IS_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_REGEXP */
} /* jjs_regexp */

/**
 * Creates a new realm (global object).
 *
 * @return new realm object
 */
jjs_value_t
jjs_realm (void)
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_REALMS
  ecma_global_object_t *global_object_p = ecma_builtin_create_global_object ();
  ecma_value_t global = ecma_make_object_value ((ecma_object_t *) global_object_p);

  jjs_init_realm (global);
  jjs_annex_init_realm (global_object_p);

  return global;
#else /* !JJS_BUILTIN_REALMS */
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_REALMS_ARE_DISABLED));
#endif /* JJS_BUILTIN_REALMS */
} /* jjs_realm */

/**
 * Get length of an array object
 *
 * Note:
 *      Returns 0, if the value parameter is not an array object.
 *
 * @return length of the given array
 */
jjs_length_t
jjs_array_length (const jjs_value_t value) /**< api value */
{
  jjs_assert_api_enabled ();

  if (!jjs_value_is_object (value))
  {
    return 0;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (value);

  if (JJS_LIKELY (ecma_get_object_base_type (object_p) == ECMA_OBJECT_BASE_TYPE_ARRAY))
  {
    return ecma_array_get_length (object_p);
  }

  return 0;
} /* jjs_array_length */

/**
 * Get the size of a string value in the specified encoding.
 *
 * @return number of bytes required by the string,
 *         0 - if value is not a string
 */
jjs_size_t
jjs_string_size (const jjs_value_t value, /**< input string */
                   jjs_encoding_t encoding) /**< encoding */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  switch (encoding)
  {
    case JJS_ENCODING_CESU8:
    {
      return ecma_string_get_size (ecma_get_string_from_value (value));
    }
    case JJS_ENCODING_UTF8:
    {
      return ecma_string_get_utf8_size (ecma_get_string_from_value (value));
    }
    default:
    {
      return 0;
    }
  }
} /* jjs_string_size */

/**
 * Get length of a string value
 *
 * @return number of characters in the string
 *         0 - if value is not a string
 */
jjs_length_t
jjs_string_length (const jjs_value_t value) /**< input string */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  return ecma_string_get_length (ecma_get_string_from_value (value));
} /* jjs_string_length */

/**
 * Copy the characters of a string into the specified buffer using the specified encoding.  The string is truncated to
 * fit the buffer. If the value is not a string, nothing will be copied to the buffer.
 *
 * @return number of bytes copied to the buffer
 */
jjs_size_t
jjs_string_to_buffer (const jjs_value_t value, /**< input string value */
                        jjs_encoding_t encoding, /**< output encoding */
                        jjs_char_t *buffer_p, /**< [out] output characters buffer */
                        jjs_size_t buffer_size) /**< size of output buffer */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_string (value) || buffer_p == NULL)
  {
    return 0;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (value);

  return ecma_string_copy_to_buffer (str_p, (lit_utf8_byte_t *) buffer_p, buffer_size, encoding);
} /* jjs_string_to_char_buffer */

/**
 * Create a substring of the input string value.
 * Return an empty string if input value is not a string.
 *
 * @param value  the input string value
 * @param start  start position of the substring
 * @param end    end position of the substring
 *
 * @return created string
 */
jjs_value_t
jjs_string_substr (const jjs_value_t value, jjs_length_t start, jjs_length_t end)
{
  if (!ecma_is_value_string (value))
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  return ecma_make_string_value (ecma_string_substr (ecma_get_string_from_value (value), start, end));
} /* jjs_string_substr */

/**
 * Iterate over the input string value in the specified encoding, visiting each unit of the encoded string once. If
 * the input value is not a string, the function will do nothing.
 *
 * @param value     the input string value
 * @param callback  callback function called for each byte of the encoded string.
 * @param encoding  the requested encoding for the string
 * @param user_p    User pointer passed to the callback function
 */
void
jjs_string_iterate (const jjs_value_t value,
                      jjs_encoding_t encoding,
                      jjs_string_iterate_cb_t callback,
                      void *user_p)
{
  if (!ecma_is_value_string (value))
  {
    return;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (value);
  ECMA_STRING_TO_UTF8_STRING (str_p, buffer_p, buffer_size);

  const lit_utf8_byte_t *current_p = buffer_p;
  const lit_utf8_byte_t *end_p = buffer_p + buffer_size;

  switch (encoding)
  {
    case JJS_ENCODING_UTF8:
    {
      while (current_p < end_p)
      {
        if (JJS_UNLIKELY (*current_p >= LIT_UTF8_3_BYTE_MARKER))
        {
          lit_code_point_t cp;
          lit_utf8_size_t read_size = lit_read_code_point_from_cesu8 (current_p, end_p, &cp);

          lit_utf8_byte_t bytes[LIT_UTF8_MAX_BYTES_IN_CODE_POINT];
          lit_utf8_size_t encoded_size = lit_code_point_to_utf8 (cp, bytes);

          for (uint32_t i = 0; i < encoded_size; i++)
          {
            callback (bytes[i], user_p);
          }

          current_p += read_size;
          continue;
        }

        callback (*current_p++, user_p);
      }

      break;
    }
    case JJS_ENCODING_CESU8:
    {
      while (current_p < end_p)
      {
        callback (*current_p++, user_p);
      }

      break;
    }
    default:
    {
      break;
    }
  }
  ECMA_FINALIZE_UTF8_STRING (buffer_p, buffer_size);
} /* jjs_string_iterate */

/**
 * Sets the global callback which is called when an external string is freed.
 */
void
jjs_string_external_on_free (jjs_external_string_free_cb_t callback) /**< free callback */
{
  JJS_CONTEXT (external_string_free_callback_p) = callback;
} /* jjs_string_external_on_free */

/**
 * Returns the user pointer assigned to an external string.
 *
 * @return user pointer, if value is an external string
 *         NULL, otherwise
 */
void *
jjs_string_user_ptr (const jjs_value_t value, /**< string value */
                       bool *is_external) /**< [out] true - if value is an external string,
                                           *         false - otherwise */
{
  if (is_external != NULL)
  {
    *is_external = false;
  }

  if (!ecma_is_value_string (value))
  {
    return NULL;
  }

  ecma_string_t *string_p = ecma_get_string_from_value (value);

  if (ECMA_IS_DIRECT_STRING (string_p)
      || ECMA_STRING_GET_CONTAINER (string_p) != ECMA_STRING_CONTAINER_LONG_OR_EXTERNAL_STRING)
  {
    return NULL;
  }

  ecma_long_string_t *long_string_p = (ecma_long_string_t *) string_p;

  if (long_string_p->string_p == ECMA_LONG_STRING_BUFFER_START (long_string_p))
  {
    return NULL;
  }

  if (is_external != NULL)
  {
    *is_external = true;
  }

  return ((ecma_external_string_t *) string_p)->user_p;
} /* jjs_string_user_ptr */

/**
 * Checks whether the object or it's prototype objects have the given property.
 *
 * @return raised error - if the operation fail
 *         true/false API value  - depend on whether the property exists
 */
jjs_value_t
jjs_object_has (const jjs_value_t object, /**< object value */
                  const jjs_value_t key) /**< property name (string value) */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);
  ecma_string_t *prop_name_p = ecma_get_prop_name_from_value (key);

  return jjs_return (ecma_op_object_has_property (obj_p, prop_name_p));
} /* jjs_object_has */

/**
 * Checks whether the object or it's prototype objects have the given property.
 *
 * @return raised error - if the operation fail
 *         true/false API value  - depend on whether the property exists
 */
jjs_value_t
jjs_object_has_sz (const jjs_value_t object, /**< object value */
                     const char *key_p) /**< property key */
{
  jjs_assert_api_enabled ();

  jjs_value_t key_str = jjs_string_sz (key_p);
  jjs_value_t result = jjs_object_has (object, key_str);
  ecma_free_value (key_str);

  return result;
} /* jjs_object_has */

/**
 * Checks whether the object has the given property.
 *
 * @return ECMA_VALUE_ERROR - if the operation raises error
 *         ECMA_VALUE_{TRUE, FALSE} - based on whether the property exists
 */
jjs_value_t
jjs_object_has_own (const jjs_value_t object, /**< object value */
                      const jjs_value_t key) /**< property name (string value) */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);
  ecma_string_t *prop_name_p = ecma_get_prop_name_from_value (key);

  return jjs_return (ecma_op_object_has_own_property (obj_p, prop_name_p));
} /* jjs_has_own_property */

/**
 * Checks whether the object has the given internal property.
 *
 * @return true  - if the internal property exists
 *         false - otherwise
 */
bool
jjs_object_has_internal (const jjs_value_t object, /**< object value */
                           const jjs_value_t key) /**< property name value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);

  ecma_string_t *internal_string_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_API_INTERNAL);

  if (ecma_op_object_is_fast_array (obj_p))
  {
    return false;
  }

  ecma_property_t *property_p = ecma_find_named_property (obj_p, internal_string_p);

  if (property_p == NULL)
  {
    return false;
  }

  ecma_object_t *internal_object_p = ecma_get_object_from_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
  property_p = ecma_find_named_property (internal_object_p, ecma_get_prop_name_from_value (key));

  return property_p != NULL;
} /* jjs_object_has_internal */

/**
 * Delete a property from an object.
 *
 * @return boolean value - wether the property was deleted successfully
 *         exception - otherwise
 */
jjs_value_t
jjs_object_delete (jjs_value_t object, /**< object value */
                     const jjs_value_t key) /**< property name (string value) */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return false;
  }

  return ecma_op_object_delete (ecma_get_object_from_value (object), ecma_get_prop_name_from_value (key), false);
} /* jjs_object_delete */

/**
 * Delete a property from an object.
 *
 * @return boolean value - wether the property was deleted successfully
 *         exception - otherwise
 */
jjs_value_t
jjs_object_delete_sz (jjs_value_t object, /**< object value */
                        const char *key_p) /**< property key */
{
  jjs_assert_api_enabled ();

  jjs_value_t key_str = jjs_string_sz (key_p);
  jjs_value_t result = jjs_object_delete (object, key_str);
  ecma_free_value (key_str);

  return result;
} /* jjs_object_delete */

/**
 * Delete indexed property from the specified object.
 *
 * @return boolean value - wether the property was deleted successfully
 *         false - otherwise
 */
jjs_value_t
jjs_object_delete_index (jjs_value_t object, /**< object value */
                           uint32_t index) /**< index to be written */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return false;
  }

  ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 (index);
  ecma_value_t ret_value = ecma_op_object_delete (ecma_get_object_from_value (object), str_idx_p, false);
  ecma_deref_ecma_string (str_idx_p);

  return ret_value;
} /* jjs_object_delete_index */

/**
 * Delete an internal property from an object.
 *
 * @return true  - if property was deleted successfully
 *         false - otherwise
 */
bool
jjs_object_delete_internal (jjs_value_t object, /**< object value */
                              const jjs_value_t key) /**< property name value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);

  ecma_string_t *internal_string_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_API_INTERNAL);

  if (ecma_op_object_is_fast_array (obj_p))
  {
    return true;
  }

  ecma_property_t *property_p = ecma_find_named_property (obj_p, internal_string_p);

  if (property_p == NULL)
  {
    return true;
  }

  ecma_object_t *internal_object_p = ecma_get_object_from_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
  property_p = ecma_find_named_property (internal_object_p, ecma_get_prop_name_from_value (key));

  if (property_p == NULL)
  {
    return true;
  }

  ecma_delete_property (internal_object_p, ECMA_PROPERTY_VALUE_PTR (property_p));

  return true;
} /* jjs_object_delete_internal */

/**
 * Get value of a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return value of the property - if success
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_get (const jjs_value_t object, /**< object value */
                  const jjs_value_t key) /**< property name (string value) */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  jjs_value_t ret_value =
    ecma_op_object_get (ecma_get_object_from_value (object), ecma_get_prop_name_from_value (key));
  return jjs_return (ret_value);
} /* jjs_object_get */

/**
 * Get value of a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return value of the property - if success
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_get_sz (const jjs_value_t object, /**< object value */
                     const char *key_p) /**< property key */
{
  jjs_assert_api_enabled ();

  jjs_value_t key_str = jjs_string_sz (key_p);
  jjs_value_t result = jjs_object_get (object, key_str);
  ecma_free_value (key_str);

  return result;
} /* jjs_object_get */

/**
 * Get value by an index from the specified object.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return value of the property specified by the index - if success
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_get_index (const jjs_value_t object, /**< object value */
                        uint32_t index) /**< index to be written */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_value_t ret_value = ecma_op_object_get_by_index (ecma_get_object_from_value (object), index);

  return jjs_return (ret_value);
} /* jjs_object_get_index */

/**
 * Get the own property value of an object with the given name.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return value of the property - if success
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_find_own (const jjs_value_t object, /**< object value */
                       const jjs_value_t key, /**< property name (string value) */
                       const jjs_value_t receiver, /**< receiver object value */
                       bool *found_p) /**< [out] true, if the property is found
                                       *   or object is a Proxy object, false otherwise */
{
  jjs_assert_api_enabled ();

  if (found_p != NULL)
  {
    *found_p = false;
  }

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key) || !ecma_is_value_object (receiver))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *object_p = ecma_get_object_from_value (object);
  ecma_string_t *property_name_p = ecma_get_prop_name_from_value (key);

#if JJS_BUILTIN_PROXY
  if (ECMA_OBJECT_IS_PROXY (object_p))
  {
    if (found_p != NULL)
    {
      *found_p = true;
    }

    return jjs_return (ecma_proxy_object_get (object_p, property_name_p, receiver));
  }
#endif /* JJS_BUILTIN_PROXY */

  ecma_value_t ret_value = ecma_op_object_find_own (receiver, object_p, property_name_p);

  if (ecma_is_value_found (ret_value))
  {
    if (found_p != NULL)
    {
      *found_p = true;
    }

    return jjs_return (ret_value);
  }

  return ECMA_VALUE_UNDEFINED;
} /* jjs_object_find_own */

/**
 * Get value of an internal property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return value of the internal property - if the internal property exists
 *         undefined value - if the internal does not property exists
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_get_internal (const jjs_value_t object, /**< object value */
                           const jjs_value_t key) /**< property name value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);

  ecma_string_t *internal_string_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_API_INTERNAL);

  if (ecma_op_object_is_fast_array (obj_p))
  {
    return jjs_return (ECMA_VALUE_UNDEFINED);
  }

  ecma_property_t *property_p = ecma_find_named_property (obj_p, internal_string_p);

  if (property_p == NULL)
  {
    return jjs_return (ECMA_VALUE_UNDEFINED);
  }

  ecma_object_t *internal_object_p = ecma_get_object_from_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
  property_p = ecma_find_named_property (internal_object_p, ecma_get_prop_name_from_value (key));

  if (property_p == NULL)
  {
    return jjs_return (ECMA_VALUE_UNDEFINED);
  }

  return jjs_return (ecma_copy_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value));
} /* jjs_object_get_internal */

/**
 * Set a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_set (jjs_value_t object, /**< object value */
                  const jjs_value_t key, /**< property name (string value) */
                  const jjs_value_t value) /**< value to set */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (value) || !ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  return jjs_return (
    ecma_op_object_put (ecma_get_object_from_value (object), ecma_get_prop_name_from_value (key), value, true));
} /* jjs_object_set */

/**
 * Set a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_set_sz (jjs_value_t object, /**< object value */
                     const char *key_p, /**< property key */
                     const jjs_value_t value) /**< value to set */
{
  jjs_assert_api_enabled ();

  jjs_value_t key_str = jjs_string_sz (key_p);
  jjs_value_t result = jjs_object_set (object, key_str, value);
  ecma_free_value (key_str);

  return result;
} /* jjs_object_set */

/**
 * Set indexed value in the specified object
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_set_index (jjs_value_t object, /**< object value */
                        uint32_t index, /**< index to be written */
                        const jjs_value_t value) /**< value to set */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (value) || !ecma_is_value_object (object))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_value_t ret_value = ecma_op_object_put_by_index (ecma_get_object_from_value (object), index, value, true);

  return jjs_return (ret_value);
} /* jjs_object_set_index */

/**
 * Set an internal property to the specified object with the given name.
 *
 * Note:
 *      - the property cannot be accessed from the JavaScript context, only from the public API
 *      - returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
bool
jjs_object_set_internal (jjs_value_t object, /**< object value */
                           const jjs_value_t key, /**< property name value */
                           const jjs_value_t value) /**< value to set */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (value) || !ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);

  ecma_string_t *internal_string_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_API_INTERNAL);

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_fast_array_convert_to_normal (obj_p);
  }

  ecma_property_t *property_p = ecma_find_named_property (obj_p, internal_string_p);
  ecma_object_t *internal_object_p;

  if (property_p == NULL)
  {
    ecma_property_value_t *value_p =
      ecma_create_named_data_property (obj_p, internal_string_p, ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE, NULL);

    internal_object_p = ecma_create_object (NULL, sizeof (ecma_extended_object_t), ECMA_OBJECT_TYPE_CLASS);
    {
      ecma_extended_object_t *container_p = (ecma_extended_object_t *) internal_object_p;
      container_p->u.cls.type = ECMA_OBJECT_CLASS_INTERNAL_OBJECT;
    }

    value_p->value = ecma_make_object_value (internal_object_p);
    ecma_deref_object (internal_object_p);
  }
  else
  {
    internal_object_p = ecma_get_object_from_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
  }

  ecma_string_t *prop_name_p = ecma_get_prop_name_from_value (key);
  property_p = ecma_find_named_property (internal_object_p, prop_name_p);

  if (property_p == NULL)
  {
    ecma_property_value_t *value_p = ecma_create_named_data_property (internal_object_p,
                                                                      prop_name_p,
                                                                      ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                                      NULL);

    value_p->value = ecma_copy_value_if_not_object (value);
  }
  else
  {
    ecma_named_data_property_assign_value (internal_object_p, ECMA_PROPERTY_VALUE_PTR (property_p), value);
  }

  return true;
} /* jjs_object_set_internal */

/**
 * Construct empty property descriptor, i.e.:
 *  property descriptor with all is_defined flags set to false and the rest - to default value.
 *
 * @return empty property descriptor
 */
jjs_property_descriptor_t
jjs_property_descriptor (void)
{
  jjs_property_descriptor_t prop_desc;

  prop_desc.flags = JJS_PROP_NO_OPTS;
  prop_desc.value = ECMA_VALUE_UNDEFINED;
  prop_desc.getter = ECMA_VALUE_UNDEFINED;
  prop_desc.setter = ECMA_VALUE_UNDEFINED;

  return prop_desc;
} /* jjs_property_descriptor */

/**
 * Convert a ecma_property_descriptor_t to a jjs_property_descriptor_t
 *
 * if error occurs the property descriptor's value field is filled with ECMA_VALUE_ERROR
 *
 * @return jjs_property_descriptor_t
 */
static jjs_property_descriptor_t
jjs_property_descriptor_from_ecma (const ecma_property_descriptor_t *prop_desc_p) /**<[out] property_descriptor */
{
  jjs_property_descriptor_t prop_desc = jjs_property_descriptor ();

  prop_desc.flags = prop_desc_p->flags;

  if (prop_desc.flags & (JJS_PROP_IS_VALUE_DEFINED))
  {
    prop_desc.value = prop_desc_p->value;
  }

  if (prop_desc_p->flags & JJS_PROP_IS_GET_DEFINED)
  {
    prop_desc.getter = ECMA_VALUE_NULL;

    if (prop_desc_p->get_p != NULL)
    {
      prop_desc.getter = ecma_make_object_value (prop_desc_p->get_p);
      JJS_ASSERT (ecma_op_is_callable (prop_desc.getter));
    }
  }

  if (prop_desc_p->flags & JJS_PROP_IS_SET_DEFINED)
  {
    prop_desc.setter = ECMA_VALUE_NULL;

    if (prop_desc_p->set_p != NULL)
    {
      prop_desc.setter = ecma_make_object_value (prop_desc_p->set_p);
      JJS_ASSERT (ecma_op_is_callable (prop_desc.setter));
    }
  }

  return prop_desc;
} /* jjs_property_descriptor_from_ecma */

/**
 * Convert a jjs_property_descriptor_t to a ecma_property_descriptor_t
 *
 * Note:
 *     if error occurs the property descriptor's value field
 *     is set to ECMA_VALUE_ERROR, but no error is thrown
 *
 * @return ecma_property_descriptor_t
 */
static ecma_property_descriptor_t
jjs_property_descriptor_to_ecma (const jjs_property_descriptor_t *prop_desc_p) /**< input property_descriptor */
{
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  prop_desc.flags = prop_desc_p->flags;

  /* Copy data property info. */
  if (prop_desc_p->flags & JJS_PROP_IS_VALUE_DEFINED)
  {
    if (ecma_is_value_exception (prop_desc_p->value)
        || (prop_desc_p->flags & (JJS_PROP_IS_GET_DEFINED | JJS_PROP_IS_SET_DEFINED)))
    {
      prop_desc.value = ECMA_VALUE_ERROR;
      return prop_desc;
    }

    prop_desc.value = prop_desc_p->value;
  }

  /* Copy accessor property info. */
  if (prop_desc_p->flags & JJS_PROP_IS_GET_DEFINED)
  {
    ecma_value_t getter = prop_desc_p->getter;

    if (ecma_is_value_exception (getter))
    {
      prop_desc.value = ECMA_VALUE_ERROR;
      return prop_desc;
    }

    if (ecma_op_is_callable (getter))
    {
      prop_desc.get_p = ecma_get_object_from_value (getter);
    }
    else if (!ecma_is_value_null (getter))
    {
      prop_desc.value = ECMA_VALUE_ERROR;
      return prop_desc;
    }
  }

  if (prop_desc_p->flags & JJS_PROP_IS_SET_DEFINED)
  {
    ecma_value_t setter = prop_desc_p->setter;

    if (ecma_is_value_exception (setter))
    {
      prop_desc.value = ECMA_VALUE_ERROR;
      return prop_desc;
    }

    if (ecma_op_is_callable (setter))
    {
      prop_desc.set_p = ecma_get_object_from_value (setter);
    }
    else if (!ecma_is_value_null (setter))
    {
      prop_desc.value = ECMA_VALUE_ERROR;
      return prop_desc;
    }
  }

  const uint16_t configurable_mask = JJS_PROP_IS_CONFIGURABLE | JJS_PROP_IS_CONFIGURABLE_DEFINED;
  const uint16_t enumerable_mask = JJS_PROP_IS_ENUMERABLE | JJS_PROP_IS_ENUMERABLE_DEFINED;
  const uint16_t writable_mask = JJS_PROP_IS_WRITABLE | JJS_PROP_IS_WRITABLE_DEFINED;

  if ((prop_desc_p->flags & configurable_mask) == JJS_PROP_IS_CONFIGURABLE
      || (prop_desc_p->flags & enumerable_mask) == JJS_PROP_IS_ENUMERABLE
      || (prop_desc_p->flags & writable_mask) == JJS_PROP_IS_WRITABLE)
  {
    prop_desc.value = ECMA_VALUE_ERROR;
    return prop_desc;
  }

  prop_desc.flags |= (uint16_t) (prop_desc_p->flags | JJS_PROP_SHOULD_THROW);

  return prop_desc;
} /* jjs_property_descriptor_to_ecma */

/** Helper function to return false value or error depending on the given flag.
 *
 * @return value marked with error flag - if is_throw is true
 *         false value - otherwise
 */
static jjs_value_t
jjs_type_error_or_false (ecma_error_msg_t msg, /**< message */
                           uint16_t flags) /**< property descriptor flags */
{
  if (!(flags & JJS_PROP_SHOULD_THROW))
  {
    return ECMA_VALUE_FALSE;
  }

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (msg));
} /* jjs_type_error_or_false */

/**
 * Define a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         false value - if the property cannot be defined and JJS_PROP_SHOULD_THROW is not set
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_define_own_prop (jjs_value_t object, /**< object value */
                              const jjs_value_t key, /**< property name (string value) */
                              const jjs_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return jjs_type_error_or_false (ECMA_ERR_WRONG_ARGS_MSG, prop_desc_p->flags);
  }

  if (prop_desc_p->flags & (JJS_PROP_IS_WRITABLE_DEFINED | JJS_PROP_IS_VALUE_DEFINED)
      && prop_desc_p->flags & (JJS_PROP_IS_GET_DEFINED | JJS_PROP_IS_SET_DEFINED))
  {
    return jjs_type_error_or_false (ECMA_ERR_WRONG_ARGS_MSG, prop_desc_p->flags);
  }

  ecma_property_descriptor_t prop_desc = jjs_property_descriptor_to_ecma (prop_desc_p);

  if (ECMA_IS_VALUE_ERROR (prop_desc.value))
  {
    return jjs_type_error_or_false (ECMA_ERR_WRONG_ARGS_MSG, prop_desc_p->flags);
  }

  return jjs_return (ecma_op_object_define_own_property (ecma_get_object_from_value (object),
                                                           ecma_get_prop_name_from_value (key),
                                                           &prop_desc));
} /* jjs_object_define_own_prop */

/**
 * Construct property descriptor from specified property.
 *
 * @return true - if success, the prop_desc_p fields contains the property info
 *         false - otherwise, the prop_desc_p is unchanged
 */
jjs_value_t
jjs_object_get_own_prop (const jjs_value_t object, /**< object value */
                           const jjs_value_t key, /**< property name (string value) */
                           jjs_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_property_descriptor_t prop_desc;

  ecma_value_t status = ecma_op_object_get_own_property_descriptor (ecma_get_object_from_value (object),
                                                                    ecma_get_prop_name_from_value (key),
                                                                    &prop_desc);

#if JJS_BUILTIN_PROXY
  if (ECMA_IS_VALUE_ERROR (status))
  {
    return ecma_create_exception_from_context ();
  }
#endif /* JJS_BUILTIN_PROXY */

  if (!ecma_is_value_true (status))
  {
    return ECMA_VALUE_FALSE;
  }

  /* The flags are always filled in the returned descriptor. */
  JJS_ASSERT (
    (prop_desc.flags & JJS_PROP_IS_CONFIGURABLE_DEFINED) && (prop_desc.flags & JJS_PROP_IS_ENUMERABLE_DEFINED)
    && ((prop_desc.flags & JJS_PROP_IS_WRITABLE_DEFINED) || !(prop_desc.flags & JJS_PROP_IS_VALUE_DEFINED)));

  prop_desc_p->flags = prop_desc.flags;
  prop_desc_p->value = ECMA_VALUE_UNDEFINED;
  prop_desc_p->getter = ECMA_VALUE_UNDEFINED;
  prop_desc_p->setter = ECMA_VALUE_UNDEFINED;

  if (prop_desc_p->flags & JJS_PROP_IS_VALUE_DEFINED)
  {
    prop_desc_p->value = prop_desc.value;
  }

  if (prop_desc_p->flags & JJS_PROP_IS_GET_DEFINED)
  {
    if (prop_desc.get_p != NULL)
    {
      prop_desc_p->getter = ecma_make_object_value (prop_desc.get_p);
    }
    else
    {
      prop_desc_p->getter = ECMA_VALUE_NULL;
    }
  }

  if (prop_desc_p->flags & JJS_PROP_IS_SET_DEFINED)
  {
    if (prop_desc.set_p != NULL)
    {
      prop_desc_p->setter = ecma_make_object_value (prop_desc.set_p);
    }
    else
    {
      prop_desc_p->setter = ECMA_VALUE_NULL;
    }
  }

  return ECMA_VALUE_TRUE;
} /* jjs_object_get_own_prop */

/**
 * Free fields of property descriptor (setter, getter and value).
 */
void
jjs_property_descriptor_free (jjs_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  if (prop_desc_p->flags & JJS_PROP_IS_VALUE_DEFINED)
  {
    jjs_value_free (prop_desc_p->value);
  }

  if (prop_desc_p->flags & JJS_PROP_IS_GET_DEFINED)
  {
    jjs_value_free (prop_desc_p->getter);
  }

  if (prop_desc_p->flags & JJS_PROP_IS_SET_DEFINED)
  {
    jjs_value_free (prop_desc_p->setter);
  }
} /* jjs_property_descriptor_free */

/**
 * Call function specified by a function value
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *      error flag must not be set for any arguments of this function.
 *
 * @return returned JJS value of the called function
 */
jjs_value_t
jjs_call (const jjs_value_t func_object, /**< function object to call */
            const jjs_value_t this_value, /**< object for 'this' binding */
            const jjs_value_t *args_p, /**< function's call arguments */
            jjs_size_t args_count) /**< number of the arguments */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_exception (this_value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  for (jjs_size_t i = 0; i < args_count; i++)
  {
    if (ecma_is_value_exception (args_p[i]))
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
    }
  }

  return jjs_return (ecma_op_function_validated_call (func_object, this_value, args_p, args_count));
} /* jjs_call */

/**
 * Construct object value invoking specified function value as a constructor
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *      error flag must not be set for any arguments of this function.
 *
 * @return returned JJS value of the invoked constructor
 */
jjs_value_t
jjs_construct (const jjs_value_t func_object, /**< function object to call */
                 const jjs_value_t *args_p, /**< function's call arguments
                                               *   (NULL if arguments number is zero) */
                 jjs_size_t args_count) /**< number of the arguments */
{
  jjs_assert_api_enabled ();

  if (!jjs_value_is_constructor (func_object))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  for (jjs_size_t i = 0; i < args_count; i++)
  {
    if (ecma_is_value_exception (args_p[i]))
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
    }
  }

  return jjs_return (ecma_op_function_construct (ecma_get_object_from_value (func_object),
                                                   ecma_get_object_from_value (func_object),
                                                   args_p,
                                                   args_count));
} /* jjs_construct */

/**
 * Get keys of the specified object value
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return array object value - if success
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_keys (const jjs_value_t object) /**< object value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_collection_t *prop_names =
    ecma_op_object_get_enumerable_property_names (ecma_get_object_from_value (object), ECMA_ENUMERABLE_PROPERTY_KEYS);

#if JJS_BUILTIN_PROXY
  if (JJS_UNLIKELY (prop_names == NULL))
  {
    return ecma_create_exception_from_context ();
  }
#endif /* JJS_BUILTIN_PROXY */

  return ecma_op_new_array_object_from_collection (prop_names, false);
} /* jjs_object_keys */

/**
 * Get the prototype of the specified object
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return prototype object or null value - if success
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_proto (const jjs_value_t object) /**< object value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);

#if JJS_BUILTIN_PROXY
  if (ECMA_OBJECT_IS_PROXY (obj_p))
  {
    return jjs_return (ecma_proxy_object_get_prototype_of (obj_p));
  }
#endif /* JJS_BUILTIN_PROXY */

  if (obj_p->u2.prototype_cp == JMEM_CP_NULL)
  {
    return ECMA_VALUE_NULL;
  }

  ecma_object_t *proto_obj_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, obj_p->u2.prototype_cp);
  ecma_ref_object (proto_obj_p);

  return ecma_make_object_value (proto_obj_p);
} /* jjs_object_proto */

/**
 * Set the prototype of the specified object
 *
 * @return true value - if success
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_set_proto (jjs_value_t object, /**< object value */
                        const jjs_value_t proto) /**< prototype object value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object) || ecma_is_value_exception (proto)
      || (!ecma_is_value_object (proto) && !ecma_is_value_null (proto)))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }
  ecma_object_t *obj_p = ecma_get_object_from_value (object);

#if JJS_BUILTIN_PROXY
  if (ECMA_OBJECT_IS_PROXY (obj_p))
  {
    return jjs_return (ecma_proxy_object_set_prototype_of (obj_p, proto));
  }
#endif /* JJS_BUILTIN_PROXY */

  return ecma_op_ordinary_object_set_prototype_of (obj_p, proto);
} /* jjs_object_set_proto */

/**
 * Utility to check if a given object can be used for the foreach api calls.
 *
 * Some objects/classes uses extra internal objects to correctly store data.
 * These extre object should never be exposed externally to the API user.
 *
 * @returns true - if the user can access the object in the callback.
 *          false - if the object is an internal object which should no be accessed by the user.
 */
static bool
jjs_object_is_valid_foreach (ecma_object_t *object_p) /**< object to test */
{
  if (ecma_is_lexical_environment (object_p))
  {
    return false;
  }

  ecma_object_type_t object_type = ecma_get_object_type (object_p);

  if (object_type == ECMA_OBJECT_TYPE_CLASS)
  {
    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
    switch (ext_object_p->u.cls.type)
    {
      /* An object's internal property object should not be iterable by foreach. */
      case ECMA_OBJECT_CLASS_INTERNAL_OBJECT:
      {
        return false;
      }
    }
  }

  return true;
} /* jjs_object_is_valid_foreach */

/**
 * Traverse objects.
 *
 * @return true - traversal was interrupted by the callback.
 *         false - otherwise - traversal visited all objects.
 */
bool
jjs_foreach_live_object (jjs_foreach_live_object_cb_t callback, /**< function pointer of the iterator function */
                           void *user_data_p) /**< pointer to user data */
{
  jjs_assert_api_enabled ();

  JJS_ASSERT (callback != NULL);

  jmem_cpointer_t iter_cp = JJS_CONTEXT (ecma_gc_objects_cp);

  while (iter_cp != JMEM_CP_NULL)
  {
    ecma_object_t *iter_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, iter_cp);

    if (jjs_object_is_valid_foreach (iter_p) && !callback (ecma_make_object_value (iter_p), user_data_p))
    {
      return true;
    }

    iter_cp = iter_p->gc_next_cp;
  }

  return false;
} /* jjs_foreach_live_object */

/**
 * Traverse objects having a given native type info.
 *
 * @return true - traversal was interrupted by the callback.
 *         false - otherwise - traversal visited all objects.
 */
bool
jjs_foreach_live_object_with_info (const jjs_object_native_info_t *native_info_p, /**< the type info
                                                                                       *   of the native pointer */
                                     jjs_foreach_live_object_with_info_cb_t callback, /**< function to apply for
                                                                                         *   each matching object */
                                     void *user_data_p) /**< pointer to user data */
{
  jjs_assert_api_enabled ();

  JJS_ASSERT (native_info_p != NULL);
  JJS_ASSERT (callback != NULL);

  ecma_native_pointer_t *native_pointer_p;

  jmem_cpointer_t iter_cp = JJS_CONTEXT (ecma_gc_objects_cp);

  while (iter_cp != JMEM_CP_NULL)
  {
    ecma_object_t *iter_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, iter_cp);

    if (jjs_object_is_valid_foreach (iter_p))
    {
      native_pointer_p = ecma_get_native_pointer_value (iter_p, (void *) native_info_p);
      if (native_pointer_p && !callback (ecma_make_object_value (iter_p), native_pointer_p->native_p, user_data_p))
      {
        return true;
      }
    }

    iter_cp = iter_p->gc_next_cp;
  }

  return false;
} /* jjs_foreach_live_object_with_info */

/**
 * Get native pointer and its type information, associated with the given native type info.
 *
 * Note:
 *  If native pointer is present, its type information is returned in out_native_pointer_p
 *
 * @return found native pointer,
 *         or NULL
 */
void *
jjs_object_get_native_ptr (const jjs_value_t object, /**< object to get native pointer from */
                             const jjs_object_native_info_t *native_info_p) /**< the type info
                                                                               *   of the native pointer */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return NULL;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);
  ecma_native_pointer_t *native_pointer_p = ecma_get_native_pointer_value (obj_p, (void *) native_info_p);

  if (native_pointer_p == NULL)
  {
    return NULL;
  }

  return native_pointer_p->native_p;
} /* jjs_object_get_native_ptr */

/**
 * Set native pointer and an optional type info for the specified object.
 *
 *
 * Note:
 *      If native pointer was already set for the object, its value is updated.
 *
 * Note:
 *      If a non-NULL free callback is specified in the native type info,
 *      it will be called by the garbage collector when the object is freed.
 *      Referred values by this method must have at least 1 reference. (Correct API usage satisfies this condition)
 *      The type info always overwrites the previous value, so passing
 *      a NULL value deletes the current type info.
 */
void
jjs_object_set_native_ptr (jjs_value_t object, /**< object to set native pointer in */
                             const jjs_object_native_info_t *native_info_p, /**< object's native type info */
                             void *native_pointer_p) /**< native pointer */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_object (object))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (object);

    ecma_create_native_pointer_property (object_p, native_pointer_p, native_info_p);
  }
} /* jjs_object_set_native_ptr */

/**
 * Checks wether the argument object has a native poitner set for the specified native type info.
 *
 * @return true if the native pointer has been set,
 *         false otherwise
 */
bool
jjs_object_has_native_ptr (const jjs_value_t object, /**< object to set native pointer in */
                             const jjs_object_native_info_t *native_info_p) /**< object's native type info */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);
  ecma_native_pointer_t *native_pointer_p = ecma_get_native_pointer_value (obj_p, (void *) native_info_p);

  return native_pointer_p != NULL;
} /* jjs_object_has_native_ptr */

/**
 * Delete the previously set native pointer by the native type info from the specified object.
 *
 * Note:
 *      If the specified object has no matching native pointer for the given native type info
 *      the function has no effect.
 *
 * Note:
 *      This operation cannot throw an exception.
 *
 * @return true - if the native pointer has been deleted succesfully
 *         false - otherwise
 */
bool
jjs_object_delete_native_ptr (jjs_value_t object, /**< object to delete native pointer from */
                                const jjs_object_native_info_t *native_info_p) /**< object's native type info */
{
  jjs_assert_api_enabled ();

  if (ecma_is_value_object (object))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (object);

    return ecma_delete_native_pointer_property (object_p, (void *) native_info_p);
  }

  return false;
} /* jjs_object_delete_native_ptr */

/**
 * Initialize the references stored in a buffer pointed by a native pointer.
 * The references are initialized to undefined.
 */
void
jjs_native_ptr_init (void *native_pointer_p, /**< a valid non-NULL pointer to a native buffer */
                       const jjs_object_native_info_t *native_info_p) /**< the type info of
                                                                         *   the native pointer */
{
  jjs_assert_api_enabled ();

  if (native_pointer_p == NULL || native_info_p == NULL)
  {
    return;
  }

  ecma_value_t *value_p = (ecma_value_t *) (((uint8_t *) native_pointer_p) + native_info_p->offset_of_references);
  ecma_value_t *end_p = value_p + native_info_p->number_of_references;

  while (value_p < end_p)
  {
    *value_p++ = ECMA_VALUE_UNDEFINED;
  }
} /* jjs_native_ptr_init */

/**
 * Release the value references after a buffer pointed by a native pointer
 * is not attached to an object anymore. All references are set to undefined
 * similar to jjs_native_ptr_init.
 */
void
jjs_native_ptr_free (void *native_pointer_p, /**< a valid non-NULL pointer to a native buffer */
                       const jjs_object_native_info_t *native_info_p) /**< the type info of
                                                                         *   the native pointer */
{
  jjs_assert_api_enabled ();

  if (native_pointer_p == NULL || native_info_p == NULL)
  {
    return;
  }

  ecma_value_t *value_p = (ecma_value_t *) (((uint8_t *) native_pointer_p) + native_info_p->offset_of_references);
  ecma_value_t *end_p = value_p + native_info_p->number_of_references;

  while (value_p < end_p)
  {
    ecma_free_value_if_not_object (*value_p);
    *value_p++ = ECMA_VALUE_UNDEFINED;
  }
} /* jjs_native_ptr_free */

/**
 * Updates a value reference inside the area specified by the number_of_references and
 * offset_of_references fields in its corresponding jjs_object_native_info_t data.
 * The area must be part of a buffer which is currently assigned to an object.
 *
 * Note:
 *      Error references are not supported, they are replaced by undefined values.
 */
void
jjs_native_ptr_set (jjs_value_t *reference_p, /**< a valid non-NULL pointer to
                                                   *   a reference in a native buffer. */
                      const jjs_value_t value) /**< new value of the reference */
{
  jjs_assert_api_enabled ();

  if (reference_p == NULL)
  {
    return;
  }

  ecma_free_value_if_not_object (*reference_p);

  if (ecma_is_value_exception (value))
  {
    *reference_p = ECMA_VALUE_UNDEFINED;
    return;
  }

  *reference_p = ecma_copy_value_if_not_object (value);
} /* jjs_native_ptr_set */

/**
 * Applies the given function to the every property in the object.
 *
 * @return true - if object fields traversal was performed successfully, i.e.:
 *                - no unhandled exceptions were thrown in object fields traversal;
 *                - object fields traversal was stopped on callback that returned false;
 *         false - otherwise,
 *                 if getter of field threw a exception or unhandled exceptions were thrown during traversal;
 */
bool
jjs_object_foreach (const jjs_value_t object, /**< object value */
                      jjs_object_property_foreach_cb_t foreach_p, /**< foreach function */
                      void *user_data_p) /**< user data for foreach function */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return false;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (object);
  ecma_collection_t *names_p = ecma_op_object_enumerate (object_p);

#if JJS_BUILTIN_PROXY
  if (names_p == NULL)
  {
    // TODO: Due to Proxies the return value must be changed to jjs_value_t on next release
    jcontext_release_exception ();
    return false;
  }
#endif /* JJS_BUILTIN_PROXY */

  ecma_value_t *buffer_p = names_p->buffer_p;

  ecma_value_t property_value = ECMA_VALUE_EMPTY;

  bool continuous = true;

  for (uint32_t i = 0; continuous && (i < names_p->item_count); i++)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (buffer_p[i]);

    property_value = ecma_op_object_get (object_p, property_name_p);

    if (ECMA_IS_VALUE_ERROR (property_value))
    {
      break;
    }

    continuous = foreach_p (buffer_p[i], property_value, user_data_p);
    ecma_free_value (property_value);
  }

  ecma_collection_free (names_p);

  if (!ECMA_IS_VALUE_ERROR (property_value))
  {
    return true;
  }

  jcontext_release_exception ();
  return false;
} /* jjs_object_foreach */

/**
 * Gets the property keys for the given object using the selected filters.
 *
 * @return array containing the filtered property keys in successful operation
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_object_property_names (const jjs_value_t object, /**< object */
                             jjs_property_filter_t filter) /**< property filter options */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);
  ecma_object_t *obj_iter_p = obj_p;
  ecma_collection_t *result_p = ecma_new_collection ();

  ecma_ref_object (obj_iter_p);

  while (true)
  {
    /* Step 1. Get Object.[[OwnKeys]] */
    ecma_collection_t *prop_names_p = ecma_op_object_own_property_keys (obj_iter_p, filter);

#if JJS_BUILTIN_PROXY
    if (prop_names_p == NULL)
    {
      ecma_deref_object (obj_iter_p);
      return ecma_create_exception_from_context ();
    }
#endif /* JJS_BUILTIN_PROXY */

    for (uint32_t i = 0; i < prop_names_p->item_count; i++)
    {
      ecma_value_t key = prop_names_p->buffer_p[i];
      ecma_string_t *key_p = ecma_get_prop_name_from_value (key);
      uint32_t index = ecma_string_get_array_index (key_p);

      /* Step 2. Filter by key type */
      if (filter
          & (JJS_PROPERTY_FILTER_EXCLUDE_STRINGS | JJS_PROPERTY_FILTER_EXCLUDE_SYMBOLS
             | JJS_PROPERTY_FILTER_EXCLUDE_INTEGER_INDICES))
      {
        if (ecma_is_value_symbol (key))
        {
          if (filter & JJS_PROPERTY_FILTER_EXCLUDE_SYMBOLS)
          {
            continue;
          }
        }
        else if (index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          if ((filter & JJS_PROPERTY_FILTER_EXCLUDE_INTEGER_INDICES)
              || ((filter & JJS_PROPERTY_FILTER_EXCLUDE_STRINGS)
                  && !(filter & JJS_PROPERTY_FILTER_INTEGER_INDICES_AS_NUMBER)))
          {
            continue;
          }
        }
        else if (filter & JJS_PROPERTY_FILTER_EXCLUDE_STRINGS)
        {
          continue;
        }
      }

      /* Step 3. Filter property attributes */
      if (filter
          & (JJS_PROPERTY_FILTER_EXCLUDE_NON_CONFIGURABLE | JJS_PROPERTY_FILTER_EXCLUDE_NON_ENUMERABLE
             | JJS_PROPERTY_FILTER_EXCLUDE_NON_WRITABLE))
      {
        ecma_property_descriptor_t prop_desc;
        ecma_value_t status = ecma_op_object_get_own_property_descriptor (obj_iter_p, key_p, &prop_desc);

#if JJS_BUILTIN_PROXY
        if (ECMA_IS_VALUE_ERROR (status))
        {
          ecma_collection_free (prop_names_p);
          ecma_collection_free (result_p);
          ecma_deref_object (obj_iter_p);
          return ecma_create_exception_from_context ();
        }
#endif /* JJS_BUILTIN_PROXY */

        JJS_ASSERT (ecma_is_value_true (status));
        uint16_t flags = prop_desc.flags;
        ecma_free_property_descriptor (&prop_desc);

        if ((!(flags & JJS_PROP_IS_CONFIGURABLE) && (filter & JJS_PROPERTY_FILTER_EXCLUDE_NON_CONFIGURABLE))
            || (!(flags & JJS_PROP_IS_ENUMERABLE) && (filter & JJS_PROPERTY_FILTER_EXCLUDE_NON_ENUMERABLE))
            || (!(flags & JJS_PROP_IS_WRITABLE) && (filter & JJS_PROPERTY_FILTER_EXCLUDE_NON_WRITABLE)))
        {
          continue;
        }
      }

      if (index != ECMA_STRING_NOT_ARRAY_INDEX && (filter & JJS_PROPERTY_FILTER_INTEGER_INDICES_AS_NUMBER))
      {
        ecma_deref_ecma_string (key_p);
        key = ecma_make_uint32_value (index);
      }
      else
      {
        ecma_ref_ecma_string (key_p);
      }

      if ((filter & JJS_PROPERTY_FILTER_TRAVERSE_PROTOTYPE_CHAIN) && obj_iter_p != obj_p)
      {
        uint32_t duplicate_idx = 0;
        while (duplicate_idx < result_p->item_count)
        {
          ecma_value_t value = result_p->buffer_p[duplicate_idx];
          JJS_ASSERT (ecma_is_value_prop_name (value) || ecma_is_value_number (value));
          if (JJS_UNLIKELY (ecma_is_value_number (value)))
          {
            if (ecma_get_number_from_value (value) == ecma_get_number_from_value (key))
            {
              break;
            }
          }
          else if (ecma_compare_ecma_strings (ecma_get_prop_name_from_value (value), key_p))
          {
            break;
          }

          duplicate_idx++;
        }

        if (duplicate_idx == result_p->item_count)
        {
          ecma_collection_push_back (result_p, key);
        }
      }
      else
      {
        ecma_collection_push_back (result_p, key);
      }
    }

    ecma_collection_free (prop_names_p);

    /* Step 4: Traverse prototype chain */

    if ((filter & JJS_PROPERTY_FILTER_TRAVERSE_PROTOTYPE_CHAIN) != JJS_PROPERTY_FILTER_TRAVERSE_PROTOTYPE_CHAIN)
    {
      break;
    }

    ecma_object_t *proto_p = ecma_op_object_get_prototype_of (obj_iter_p);

    if (proto_p == NULL)
    {
      break;
    }

    ecma_deref_object (obj_iter_p);

    if (JJS_UNLIKELY (proto_p == ECMA_OBJECT_POINTER_ERROR))
    {
      ecma_collection_free (result_p);
      return ecma_create_exception_from_context ();
    }

    obj_iter_p = proto_p;
  }

  ecma_deref_object (obj_iter_p);

  return ecma_op_new_array_object_from_collection (result_p, false);
} /* jjs_object_property_names */

/**
 * FromPropertyDescriptor abstract operation.
 *
 * @return new jjs_value_t - if success
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_property_descriptor_to_object (const jjs_property_descriptor_t *src_prop_desc_p) /**< property descriptor */
{
  jjs_assert_api_enabled ();

  ecma_property_descriptor_t prop_desc = jjs_property_descriptor_to_ecma (src_prop_desc_p);

  if (ECMA_IS_VALUE_ERROR (prop_desc.value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *desc_obj_p = ecma_op_from_property_descriptor (&prop_desc);

  return ecma_make_object_value (desc_obj_p);
} /* jjs_property_descriptor_to_object */

/**
 * ToPropertyDescriptor abstract operation.
 *
 * @return true - if the conversion is successful
 *         thrown error - otherwise
 */
jjs_value_t
jjs_property_descriptor_from_object (const jjs_value_t object, /**< object value */
                                       jjs_property_descriptor_t *out_prop_desc_p) /**< [out] filled property
                                                                                      * descriptor if return value is
                                                                                      * true, unmodified otherwise */
{
  jjs_assert_api_enabled ();

  ecma_property_descriptor_t prop_desc;
  jjs_value_t result = ecma_op_to_property_descriptor (object, &prop_desc);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    return ecma_create_exception_from_context ();
  }

  JJS_ASSERT (result == ECMA_VALUE_EMPTY);

  *out_prop_desc_p = jjs_property_descriptor_from_ecma (&prop_desc);
  return ECMA_VALUE_TRUE;
} /* jjs_property_descriptor_from_object */

/**
 * Resolve a promise value with an argument.
 *
 * @return undefined - if success,
 *         exception - otherwise
 */
jjs_value_t
jjs_promise_resolve (jjs_value_t promise, /**< the promise value */
                       const jjs_value_t argument) /**< the argument */
{
  jjs_assert_api_enabled ();

  if (!jjs_value_is_promise (promise))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  if (ecma_is_value_exception (argument))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  return ecma_fulfill_promise_with_checks (promise, argument);
} /* jjs_promise_resolve */

/**
 * Reject a promise value with an argument.
 *
 * @return undefined - if success,
 *         exception - otherwise
 */
jjs_value_t
jjs_promise_reject (jjs_value_t promise, /**< the promise value */
                      const jjs_value_t argument) /**< the argument */
{
  jjs_assert_api_enabled ();

  if (!jjs_value_is_promise (promise))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  if (ecma_is_value_exception (argument))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  return ecma_reject_promise_with_checks (promise, argument);
} /* jjs_promise_reject */

/**
 * Get the result of a promise.
 *
 * @return - Promise result
 *         - Type error if the promise support was not enabled or the input was not a promise object
 */
jjs_value_t
jjs_promise_result (const jjs_value_t promise) /**< promise object to get the result from */
{
  jjs_assert_api_enabled ();

  if (!jjs_value_is_promise (promise))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  return ecma_promise_get_result (ecma_get_object_from_value (promise));
} /* jjs_get_promise_result */

/**
 * Get the state of a promise object.
 *
 * @return - the state of the promise (one of the jjs_promise_state_t enum values)
 *         - JJS_PROMISE_STATE_NONE is only returned if the input is not a promise object
 *           or the promise support was not enabled.
 */
jjs_promise_state_t
jjs_promise_state (const jjs_value_t promise) /**< promise object to get the state from */
{
  jjs_assert_api_enabled ();

  if (!jjs_value_is_promise (promise))
  {
    return JJS_PROMISE_STATE_NONE;
  }

  uint16_t flags = ecma_promise_get_flags (ecma_get_object_from_value (promise));
  flags &= (ECMA_PROMISE_IS_PENDING | ECMA_PROMISE_IS_FULFILLED);

  return (flags ? flags : JJS_PROMISE_STATE_REJECTED);
} /* jjs_get_promise_state */

/**
 * Sets a callback for tracking Promise and async operations.
 *
 * Note:
 *     the previous callback is overwritten
 */
void
jjs_promise_on_event (jjs_promise_event_filter_t filters, /**< combination of event filters */
                        jjs_promise_event_cb_t callback, /**< notification callback */
                        void *user_p) /**< user pointer passed to the callback */
{
  jjs_assert_api_enabled ();

#if JJS_PROMISE_CALLBACK
  if (filters == JJS_PROMISE_EVENT_FILTER_DISABLE || callback == NULL)
  {
    JJS_CONTEXT (promise_callback_filters) = JJS_PROMISE_EVENT_FILTER_DISABLE;
    return;
  }

  JJS_CONTEXT (promise_callback_filters) = (uint32_t) filters;
  JJS_CONTEXT (promise_callback) = callback;
  JJS_CONTEXT (promise_callback_user_p) = user_p;
#else /* !JJS_PROMISE_CALLBACK */
  JJS_UNUSED (filters);
  JJS_UNUSED (callback);
  JJS_UNUSED (user_p);
#endif /* JJS_PROMISE_CALLBACK */
} /* jjs_promise_set_callback */

/**
 * Get the well-knwon symbol represented by the given `symbol` enum value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return undefined value - if invalid well-known symbol was requested
 *         well-known symbol value - otherwise
 */
jjs_value_t
jjs_symbol (jjs_well_known_symbol_t symbol) /**< jjs_well_known_symbol_t enum value */
{
  jjs_assert_api_enabled ();

  lit_magic_string_id_t id = (lit_magic_string_id_t) (LIT_GLOBAL_SYMBOL__FIRST + symbol);

  if (!LIT_IS_GLOBAL_SYMBOL (id))
  {
    return ECMA_VALUE_UNDEFINED;
  }

  return ecma_make_symbol_value (ecma_op_get_global_symbol (id));
} /* jjs_get_well_known_symbol */

/**
 * Returns the description internal property of a symbol.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return string or undefined value containing the symbol's description - if success
 *         thrown error - otherwise
 */
jjs_value_t
jjs_symbol_description (const jjs_value_t symbol) /**< symbol value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_symbol (symbol))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  /* Note: This operation cannot throw an error */
  return ecma_copy_value (ecma_get_symbol_description (ecma_get_symbol_from_value (symbol)));
} /* jjs_get_symbol_description */

/**
 * Call the SymbolDescriptiveString ecma builtin operation on the symbol value.
 *
 * Note:
 *      returned value must be freed with jjs_value_free, when it is no longer needed.
 *
 * @return string value containing the symbol's descriptive string - if success
 *         thrown error - otherwise
 */
jjs_value_t
jjs_symbol_descriptive_string (const jjs_value_t symbol) /**< symbol value */
{
  jjs_assert_api_enabled ();

  if (!ecma_is_value_symbol (symbol))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  /* Note: This operation cannot throw an error */
  return ecma_get_symbol_descriptive_string (symbol);
} /* jjs_get_symbol_descriptive_string */

/**
 * Get the number of uint64 digits of a BigInt value
 *
 * @return number of uint64 digits
 */
uint32_t
jjs_bigint_digit_count (const jjs_value_t value) /**< BigInt value */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_BIGINT
  if (!ecma_is_value_bigint (value))
  {
    return 0;
  }

  return ecma_bigint_get_size_in_digits (value);
#else /* !JJS_BUILTIN_BIGINT */
  JJS_UNUSED (value);
  return 0;
#endif /* JJS_BUILTIN_BIGINT */
} /* jjs_bigint_digit_count */

/**
 * Get the uint64 digits of a BigInt value (lowest digit first)
 */
void
jjs_bigint_to_digits (const jjs_value_t value, /**< BigInt value */
                        uint64_t *digits_p, /**< [out] buffer for digits */
                        uint32_t digit_count, /**< buffer size in digits */
                        bool *sign_p) /**< [out] sign of BigInt */
{
#if JJS_BUILTIN_BIGINT
  if (!ecma_is_value_bigint (value))
  {
    if (sign_p != NULL)
    {
      *sign_p = false;
    }
    memset (digits_p, 0, digit_count * sizeof (uint64_t));
  }

  ecma_bigint_get_digits_and_sign (value, digits_p, digit_count, sign_p);
#else /* !JJS_BUILTIN_BIGINT */
  JJS_UNUSED (value);

  if (sign_p != NULL)
  {
    *sign_p = false;
  }
  memset (digits_p, 0, digit_count * sizeof (uint64_t));
#endif /* JJS_BUILTIN_BIGINT */
} /* jjs_bigint_to_digits */

/**
 * Get the target object of a Proxy object
 *
 * @return type error - if proxy_value is not a Proxy object
 *         target object - otherwise
 */
jjs_value_t
jjs_proxy_target (const jjs_value_t proxy_value) /**< proxy value */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_PROXY
  if (ecma_is_value_object (proxy_value))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (proxy_value);

    if (ECMA_OBJECT_IS_PROXY (object_p))
    {
      ecma_proxy_object_t *proxy_object_p = (ecma_proxy_object_t *) object_p;

      if (!ecma_is_value_null (proxy_object_p->target))
      {
        ecma_ref_object (ecma_get_object_from_value (proxy_object_p->target));
      }
      return proxy_object_p->target;
    }
  }
#else /* !JJS_BUILTIN_PROXY */
  JJS_UNUSED (proxy_value);
#endif /* JJS_BUILTIN_PROXY */

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ARGUMENT_IS_NOT_A_PROXY));
} /* jjs_proxy_target */

/**
 * Get the handler object of a Proxy object
 *
 * @return type error - if proxy_value is not a Proxy object
 *         handler object - otherwise
 */
jjs_value_t
jjs_proxy_handler (const jjs_value_t proxy_value) /**< proxy value */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_PROXY
  if (ecma_is_value_object (proxy_value))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (proxy_value);

    if (ECMA_OBJECT_IS_PROXY (object_p))
    {
      ecma_proxy_object_t *proxy_object_p = (ecma_proxy_object_t *) object_p;

      if (!ecma_is_value_null (proxy_object_p->handler))
      {
        ecma_ref_object (ecma_get_object_from_value (proxy_object_p->handler));
      }
      return proxy_object_p->handler;
    }
  }
#else /* !JJS_BUILTIN_PROXY */
  JJS_UNUSED (proxy_value);
#endif /* JJS_BUILTIN_PROXY */

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ARGUMENT_IS_NOT_A_PROXY));
} /* jjs_proxy_handler */

/**
 * Validate string buffer for the specified encoding
 *
 * @return true - if string is well-formed
 *         false - otherwise
 */
bool
jjs_validate_string (const jjs_char_t *buffer_p, /**< string buffer */
                       jjs_size_t buffer_size, /**< buffer size */
                       jjs_encoding_t encoding) /**< buffer encoding */
{
  switch (encoding)
  {
    case JJS_ENCODING_CESU8:
    {
      return lit_is_valid_cesu8_string (buffer_p, buffer_size);
    }
    case JJS_ENCODING_UTF8:
    {
      return lit_is_valid_utf8_string (buffer_p, buffer_size, true);
    }
    default:
    {
      return false;
    }
  }
} /* jjs_validate_string */

/**
 * Set the log level of the engine.
 *
 * Log messages with lower significance than the current log level will be ignored by `jjs_log`.
 *
 * @param level: requested log level
 */
void
jjs_log_set_level (jjs_log_level_t level)
{
  jjs_jrt_set_log_level (level);
} /* jjs_log_set_level */

/**
 * Log buffer size
 */
#define JJS_LOG_BUFFER_SIZE 64

/**
 * Log a zero-terminated string message.
 *
 * @param str_p: message
 */
static void
jjs_log_string (const char *str_p, jjs_size_t size)
{
  jjs_platform_io_log_fn_t log = JJS_CONTEXT (platform_api).io_log;

  if (log) {
    log ((const uint8_t *) str_p, size);
  }

#if JJS_DEBUGGER
  if (jjs_debugger_is_connected ())
  {
    jjs_debugger_send_string (JJS_DEBUGGER_OUTPUT_RESULT, JJS_DEBUGGER_OUTPUT_LOG, (const uint8_t *) str_p, size);
  }
#endif /* JJS_DEBUGGER */
} /* jjs_log_string */

/**
 * Log a fixed-size string message.
 *
 * @param str_p: message
 * @param size: size
 * @param buffer_p: buffer to use
 */
static void
jjs_log_string_fixed (const char *str_p, size_t size, char *buffer_p)
{
  const size_t batch_size = JJS_LOG_BUFFER_SIZE - 1;

  while (size > batch_size)
  {
    memcpy (buffer_p, str_p, batch_size);
    jjs_log_string (buffer_p, (jjs_size_t) batch_size);

    str_p += batch_size;
    size -= batch_size;
  }

  memcpy (buffer_p, str_p, size);
  jjs_log_string (buffer_p, (jjs_size_t) size);
} /* jjs_log_string_fixed */

/**
 * Format an unsigned number.
 *
 * @param num: number
 * @param width: minimum width of the number
 * @param padding: padding to be used when extending to minimum width
 * @param radix: number radix
 * @param buffer_p: buffer used to construct the number string
 *
 * @return formatted number as string
 */
static char *
jjs_format_unsigned (unsigned int num, uint8_t width, char padding, uint8_t radix, char *buffer_p)
{
  static const char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  char *cursor_p = buffer_p + JJS_LOG_BUFFER_SIZE;
  *(--cursor_p) = '\0';
  uint8_t count = 0;

  do
  {
    *(--cursor_p) = digits[num % radix];
    num /= radix;
    count++;
  } while (num > 0);

  while (count < width)
  {
    *(--cursor_p) = padding;
    count++;
  }

  return cursor_p;
} /* jjs_format_unsigned */

/**
 * Format an integer number.
 *
 * @param num: number
 * @param width: minimum width of the number
 * @param padding: padding to be used when extending to minimum width
 * @param buffer_p: buffer used to construct the number string
 *
 * @return formatted number as string
 */
static char *
jjs_format_int (int num, uint8_t width, char padding, char *buffer_p)
{
  if (num >= 0)
  {
    return jjs_format_unsigned ((unsigned) num, width, padding, 10, buffer_p);
  }

  num = -num;

  if (padding == '0' && width > 0)
  {
    char *cursor_p = jjs_format_unsigned ((unsigned) num, --width, padding, 10, buffer_p);
    *(--cursor_p) = '-';
    return cursor_p;
  }

  char *cursor_p = jjs_format_unsigned ((unsigned) num, 0, ' ', 10, buffer_p);
  *(--cursor_p) = '-';

  char *indent_p = buffer_p + JJS_LOG_BUFFER_SIZE - width - 1;

  while (cursor_p > indent_p)
  {
    *(--cursor_p) = ' ';
  }

  return cursor_p;
} /* jjs_format_int */

/**
 * Log a zero-terminated formatted message with the specified log level.
 *
 * Supported format specifiers:
 *   %s: zero-terminated string
 *   %c: character
 *   %u: unsigned integer
 *   %d: decimal integer
 *   %x: unsigned hexadecimal
 *
 * Width and padding sub-modifiers are also supported.
 *
 * @param format_p: format string
 * @param level: message log level
 */
void
jjs_log (jjs_log_level_t level, const char *format_p, ...)
{
  if (level > jjs_jrt_get_log_level ())
  {
    return;
  }

  va_list vl;
  char buffer_p[JJS_LOG_BUFFER_SIZE];
  uint32_t buffer_index = 0;
  const char *cursor_p = format_p;
  va_start (vl, format_p);

  while (*cursor_p != '\0')
  {
    if (*cursor_p == '%' || buffer_index > JJS_LOG_BUFFER_SIZE - 2)
    {
      buffer_p[buffer_index] = '\0';
      jjs_log_string (buffer_p, buffer_index);
      buffer_index = 0;
    }

    if (*cursor_p != '%')
    {
      buffer_p[buffer_index++] = *cursor_p++;
      continue;
    }

    ++cursor_p;
    uint8_t width = 0;
    size_t precision = 0;
    char padding = ' ';

    if (*cursor_p == '0')
    {
      padding = '0';
      cursor_p++;
    }

    if (lit_char_is_decimal_digit ((ecma_char_t) *cursor_p))
    {
      width = (uint8_t) (*cursor_p - '0');
      cursor_p++;
    }
    else if (*cursor_p == '*')
    {
      width = (uint8_t) JJS_MAX (va_arg (vl, int), 10);
      cursor_p++;
    }
    else if (*cursor_p == '.' && *(cursor_p + 1) == '*')
    {
      precision = (size_t) va_arg (vl, int);
      cursor_p += 2;
    }

    if (*cursor_p == '\0')
    {
      buffer_p[buffer_index++] = '%';
      break;
    }

    /* The buffer is always flushed before a substitution, and can be reused to for formatting. */
    switch (*cursor_p++)
    {
      case 's':
      {
        char *str_p = va_arg (vl, char *);

        if (precision == 0)
        {
          jjs_log_string (str_p, (jjs_size_t) strlen (str_p));
          break;
        }

        jjs_log_string_fixed (str_p, precision, buffer_p);
        break;
      }
      case 'c':
      {
        /* Arguments of types narrower than int are promoted to int for variadic functions */
        buffer_p[buffer_index++] = (char) va_arg (vl, int);
        break;
      }
      case 'd':
      {
        char *f = jjs_format_int (va_arg (vl, int), width, padding, buffer_p);
        jjs_log_string (f, (jjs_size_t) strlen (f));
        break;
      }
      case 'u':
      {
        char *f = jjs_format_unsigned (va_arg (vl, unsigned int), width, padding, 10, buffer_p);
        jjs_log_string (f, (jjs_size_t) strlen (f));
        break;
      }
      case 'x':
      {
        char *f = jjs_format_unsigned (va_arg (vl, unsigned int), width, padding, 16, buffer_p);
        jjs_log_string (f, (jjs_size_t) strlen (f));
        break;
      }
      default:
      {
        buffer_p[buffer_index++] = '%';
        break;
      }
    }
  }

  if (buffer_index > 0)
  {
    jjs_log_string (buffer_p, buffer_index);
  }

  va_end (vl);
} /* jjs_log */

/**
 * Stream write implementation that writes bytes to the platform log function and/or the debugger.
 */
static void
fmt_stream_write_io_log (const jjs_fmt_stream_t *self_p, const uint8_t *data_p, uint32_t data_size)
{
  JJS_UNUSED (self_p);

  if (JJS_CONTEXT (platform_api).io_log)
  {
    JJS_CONTEXT (platform_api).io_log (data_p, data_size);
  }

#if JJS_DEBUGGER
  if (jjs_debugger_is_connected ())
  {
    jjs_debugger_send_string (JJS_DEBUGGER_OUTPUT_RESULT, JJS_DEBUGGER_OUTPUT_LOG, data_p, data_size);
  }
#endif /* JJS_DEBUGGER */
}

/**
 * Log JS values in a fmt-like format.
 *
 * Only the {} marker is supported and the values can only be JS values.
 *
 * In C, va args do not report size and going out of bounds is undefined. printf relies on the
 * compiler to check args match the format string needs. jjs_log relies on this compiler check
 * and it cannot be extended to support JS values. The safe choice is to take a user defined
 * array of substitutions, rather than use va args.
 *
 * If the number {} markers do not match the substitution array length, an undefined will be
 * substituted or the extra substitutions will not be printed.
 *
 * jjs_log_fmt macro is available to hide the creation of a temporary values array, giving a
 * var args like experience.
 *
 * @param level log level. if greater than the current log level, the message will not be printed
 * @param format_p format string. only {} substitutions are supported.
 * @param values list of values. if values_length == 0, this can be NULL.
 * @param values_length value count. if < number of {}, undefined will be substituted. if > {}, extra values will not be
 * printed
 */
void
jjs_log_fmt_v (jjs_log_level_t level, const char *format_p, const jjs_value_t *values, jjs_size_t values_length)
{
  JJS_ASSERT (format_p);

#if JJS_DEBUGGER
  bool is_debugger_connected = jjs_debugger_is_connected ();
#else /* !JJS_DEBUGGER */
  bool is_debugger_connected = false;
#endif /* JJS_DEBUGGER */

  if (level > jjs_jrt_get_log_level () || (JJS_CONTEXT (platform_api).io_log == NULL && !is_debugger_connected)
      || format_p == NULL)
  {
    return;
  }

  jjs_fmt_stream_t stream = {
    .write = fmt_stream_write_io_log,
    .encoding = JJS_ENCODING_UTF8,
    .state_p = NULL,
  };

  jjs_fmt_v (&stream, format_p, values, values_length);
} /* jjs_log_fmt */

/**
 * Allocate memory on the engine's heap.
 *
 * Note:
 *      This function may take away memory from the executed JavaScript code.
 *      If any other dynamic memory allocation API is available (e.g., libc
 *      malloc), it should be used instead.
 *
 * @return allocated memory on success
 *         NULL otherwise
 */
void *
jjs_heap_alloc (jjs_size_t size) /**< size of the memory block */
{
  jjs_assert_api_enabled ();

  return jmem_heap_alloc_block_null_on_error (size);
} /* jjs_heap_alloc */

/**
 * Free memory allocated on the engine's heap.
 */
void
jjs_heap_free (void *mem_p, /**< value returned by jjs_heap_alloc */
                 jjs_size_t size) /**< same size as passed to jjs_heap_alloc */
{
  jjs_assert_api_enabled ();

  jmem_heap_free_block (mem_p, size);
} /* jjs_heap_free */

/**
 * When JJS_VM_HALT is enabled, the callback passed to this function
 * is periodically called with the user_p argument. If interval is greater
 * than 1, the callback is only called at every interval ticks.
 */
void
jjs_halt_handler (uint32_t interval, /**< interval of the function call */
                    jjs_halt_cb_t callback, /**< periodically called user function */
                    void *user_p) /**< pointer passed to the function */
{
#if JJS_VM_HALT
  if (interval == 0)
  {
    interval = 1;
  }

  JJS_CONTEXT (vm_exec_stop_frequency) = interval;
  JJS_CONTEXT (vm_exec_stop_counter) = interval;
  JJS_CONTEXT (vm_exec_stop_cb) = callback;
  JJS_CONTEXT (vm_exec_stop_user_p) = user_p;
#else /* !JJS_VM_HALT */
  JJS_UNUSED (interval);
  JJS_UNUSED (callback);
  JJS_UNUSED (user_p);
#endif /* JJS_VM_HALT */
} /* jjs_halt_handler */

/**
 * Get backtrace. The backtrace is an array of strings where
 * each string contains the position of the corresponding frame.
 * The array length is zero if the backtrace is not available.
 *
 * @return array value
 */
jjs_value_t
jjs_backtrace (uint32_t max_depth) /**< depth limit of the backtrace */
{
  return vm_get_backtrace (max_depth);
} /* jjs_backtrace */

/**
 * Low-level function to capture each backtrace frame.
 * The captured frame data is passed to a callback function.
 */
void
jjs_backtrace_capture (jjs_backtrace_cb_t callback, /**< callback function */
                         void *user_p) /**< user pointer passed to the callback function */
{
  jjs_frame_t frame;
  vm_frame_ctx_t *context_p = JJS_CONTEXT (vm_top_context_p);

  while (context_p != NULL)
  {
    frame.context_p = context_p;
    frame.frame_type = JJS_BACKTRACE_FRAME_JS;

    if (!callback (&frame, user_p))
    {
      return;
    }

    context_p = context_p->prev_context_p;
  }
} /* jjs_backtrace */

/**
 * Returns with the type of the backtrace frame.
 *
 * @return frame type listed in jjs_frame_type_t
 */
jjs_frame_type_t
jjs_frame_type (const jjs_frame_t *frame_p) /**< frame pointer */
{
  return (jjs_frame_type_t) frame_p->frame_type;
} /* jjs_frame_type */

/**
 * Initialize and return with the location private field of a backtrace frame.
 *
 * @return pointer to the location private field - if the location is available,
 *         NULL - otherwise
 */
const jjs_frame_location_t *
jjs_frame_location (jjs_frame_t *frame_p) /**< frame pointer */
{
  JJS_UNUSED (frame_p);

#if JJS_LINE_INFO
  if (frame_p->frame_type == JJS_BACKTRACE_FRAME_JS)
  {
    vm_frame_ctx_t *context_p = frame_p->context_p;
    const ecma_compiled_code_t *bytecode_header_p = context_p->shared_p->bytecode_header_p;

    if (!(bytecode_header_p->status_flags & CBC_CODE_FLAGS_USING_LINE_INFO))
    {
      return NULL;
    }

    frame_p->location.source_name = ecma_get_source_name (bytecode_header_p);

    ecma_line_info_get (ecma_compiled_code_get_line_info (bytecode_header_p),
                        (uint32_t) (context_p->byte_code_p - context_p->byte_code_start_p),
                        &frame_p->location);

    return &frame_p->location;
  }
#endif /* JJS_LINE_INFO */

  return NULL;
} /* jjs_frame_location */

/**
 * Initialize and return with the called function private field of a backtrace frame.
 * The backtrace frame is created for running the code bound to this function.
 *
 * @return pointer to the called function - if the function is available,
 *         NULL - otherwise
 */
const jjs_value_t *
jjs_frame_callee (jjs_frame_t *frame_p) /**< frame pointer */
{
  if (frame_p->frame_type == JJS_BACKTRACE_FRAME_JS)
  {
    vm_frame_ctx_t *context_p = frame_p->context_p;

    if (context_p->shared_p->function_object_p != NULL)
    {
      frame_p->function = ecma_make_object_value (context_p->shared_p->function_object_p);
      return &frame_p->function;
    }
  }

  return NULL;
} /* jjs_frame_callee */

/**
 * Initialize and return with the 'this' binding private field of a backtrace frame.
 * The 'this' binding is a hidden value passed to the called function. As for arrow
 * functions, the 'this' binding is assigned at function creation.
 *
 * @return pointer to the 'this' binding - if the binding is available,
 *         NULL - otherwise
 */
const jjs_value_t *
jjs_frame_this (jjs_frame_t *frame_p) /**< frame pointer */
{
  if (frame_p->frame_type == JJS_BACKTRACE_FRAME_JS)
  {
    frame_p->this_binding = frame_p->context_p->this_binding;
    return &frame_p->this_binding;
  }

  return NULL;
} /* jjs_frame_this */

/**
 * Returns true, if the code bound to the backtrace frame is strict mode code.
 *
 * @return true - if strict mode code is bound to the frame,
 *         false - otherwise
 */
bool
jjs_frame_is_strict (jjs_frame_t *frame_p) /**< frame pointer */
{
  return (frame_p->frame_type == JJS_BACKTRACE_FRAME_JS
          && (frame_p->context_p->status_flags & VM_FRAME_CTX_IS_STRICT) != 0);
} /* jjs_frame_is_strict */

/**
 * Get the source name (usually a file name) of the currently executed script or the given function object
 *
 * Note: returned value must be freed with jjs_value_free, when it is no longer needed
 *
 * @return JS string constructed from
 *         - the currently executed function object's source name, if the given value is undefined
 *         - source name of the function object, if the given value is a function object
 *         - "<anonymous>", otherwise
 */
jjs_value_t
jjs_source_name (const jjs_value_t value) /**< JJS api value */
{
#if JJS_SOURCE_NAME
  if (ecma_is_value_undefined (value) && JJS_CONTEXT (vm_top_context_p) != NULL)
  {
    return ecma_copy_value (ecma_get_source_name (JJS_CONTEXT (vm_top_context_p)->shared_p->bytecode_header_p));
  }

  ecma_value_t script_value = ecma_script_get_from_value (value);

  if (script_value == JMEM_CP_NULL)
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING_SOURCE_NAME_ANON);
  }

  const cbc_script_t *script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);

  return ecma_copy_value (script_p->source_name);
#else /* !JJS_SOURCE_NAME */
  JJS_UNUSED (value);
  return ecma_make_magic_string_value (LIT_MAGIC_STRING_SOURCE_NAME_ANON);
#endif /* JJS_SOURCE_NAME */
} /* jjs_source_name */

/**
 * Returns the user value assigned to a script / module / function.
 *
 * Note:
 *    This value is usually set by the parser when
 *    the JJS_PARSE_HAS_USER_VALUE flag is passed.
 *
 * @return user value
 */
jjs_value_t
jjs_source_user_value (const jjs_value_t value) /**< JJS api value */
{
  ecma_value_t script_value = ecma_script_get_from_value (value);

  if (script_value == JMEM_CP_NULL)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  const cbc_script_t *script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);

  if (!(script_p->refs_and_type & CBC_SCRIPT_HAS_USER_VALUE))
  {
    return ECMA_VALUE_UNDEFINED;
  }

  return ecma_copy_value (CBC_SCRIPT_GET_USER_VALUE (script_p));
} /* jjs_source_user_value */

/**
 * Checks whether an ECMAScript code is compiled by eval
 * like (eval, new Function, jjs_eval, etc.) command.
 *
 * @return true, if code is compiled by eval like command
 *         false, otherwise
 */
bool
jjs_function_is_dynamic (const jjs_value_t value) /**< JJS api value */
{
  ecma_value_t script_value = ecma_script_get_from_value (value);

  if (script_value == JMEM_CP_NULL)
  {
    return false;
  }

  const cbc_script_t *script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);

  return (script_p->refs_and_type & CBC_SCRIPT_IS_EVAL_CODE) != 0;
} /* jjs_function_is_dynamic */

/**
 * Returns a newly created source info structure corresponding to the passed script/module/function.
 *
 * @return a newly created source info, if at least one field is available, NULL otherwise
 */
jjs_source_info_t *
jjs_source_info (const jjs_value_t value) /**< JJS api value */
{
  jjs_assert_api_enabled ();

#if JJS_FUNCTION_TO_STRING
  if (!ecma_is_value_object (value))
  {
    return NULL;
  }

  jjs_source_info_t source_info;

  source_info.enabled_fields = 0;
  source_info.source_code = ECMA_VALUE_UNDEFINED;
  source_info.function_arguments = ECMA_VALUE_UNDEFINED;
  source_info.source_range_start = 0;
  source_info.source_range_length = 0;

  ecma_object_t *object_p = ecma_get_object_from_value (value);
  cbc_script_t *script_p = NULL;

  while (true)
  {
    switch (ecma_get_object_type (object_p))
    {
      case ECMA_OBJECT_TYPE_CLASS:
      {
        ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
        const ecma_compiled_code_t *bytecode_p = NULL;

        if (ext_object_p->u.cls.type == ECMA_OBJECT_CLASS_SCRIPT)
        {
          bytecode_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_compiled_code_t, ext_object_p->u.cls.u3.value);
        }
#if JJS_MODULE_SYSTEM
        else if (ext_object_p->u.cls.type == ECMA_OBJECT_CLASS_MODULE)
        {
          ecma_module_t *module_p = (ecma_module_t *) object_p;

          if (!(module_p->header.u.cls.u2.module_flags & ECMA_MODULE_IS_SYNTHETIC))
          {
            bytecode_p = module_p->u.compiled_code_p;
          }
        }
#endif /* JJS_MODULE_SYSTEM */

        if (bytecode_p == NULL)
        {
          return NULL;
        }

        ecma_value_t script_value = ((cbc_uint8_arguments_t *) bytecode_p)->script_value;
        script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);
        break;
      }
      case ECMA_OBJECT_TYPE_FUNCTION:
      {
        const ecma_compiled_code_t *bytecode_p;
        bytecode_p = ecma_op_function_get_compiled_code ((ecma_extended_object_t *) object_p);

        ecma_value_t script_value = ((cbc_uint8_arguments_t *) bytecode_p)->script_value;
        script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);

        if (bytecode_p->status_flags & CBC_CODE_FLAGS_HAS_EXTENDED_INFO)
        {
          uint8_t *extended_info_p = ecma_compiled_code_resolve_extended_info (bytecode_p);
          uint8_t extended_info = *extended_info_p;

          if (extended_info & CBC_EXTENDED_CODE_FLAGS_HAS_ARGUMENT_LENGTH)
          {
            ecma_extended_info_decode_vlq (&extended_info_p);
          }

          if (extended_info & CBC_EXTENDED_CODE_FLAGS_SOURCE_CODE_IN_ARGUMENTS)
          {
            ecma_value_t function_arguments = CBC_SCRIPT_GET_FUNCTION_ARGUMENTS (script_p, script_p->refs_and_type);

            ecma_ref_ecma_string (ecma_get_string_from_value (function_arguments));

            source_info.enabled_fields |= JJS_SOURCE_INFO_HAS_SOURCE_CODE;
            source_info.source_code = function_arguments;
            script_p = NULL;
          }

          source_info.enabled_fields |= JJS_SOURCE_INFO_HAS_SOURCE_RANGE;
          source_info.source_range_start = ecma_extended_info_decode_vlq (&extended_info_p);
          source_info.source_range_length = ecma_extended_info_decode_vlq (&extended_info_p);
        }

        JJS_ASSERT (script_p != NULL || (source_info.enabled_fields & JJS_SOURCE_INFO_HAS_SOURCE_CODE));

        if (source_info.enabled_fields == 0 && (script_p->refs_and_type & CBC_SCRIPT_HAS_FUNCTION_ARGUMENTS))
        {
          ecma_value_t function_arguments = CBC_SCRIPT_GET_FUNCTION_ARGUMENTS (script_p, script_p->refs_and_type);

          ecma_ref_ecma_string (ecma_get_string_from_value (function_arguments));

          source_info.enabled_fields |= JJS_SOURCE_INFO_HAS_FUNCTION_ARGUMENTS;
          source_info.function_arguments = function_arguments;
        }
        break;
      }
      case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
      {
        ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

        object_p =
          ECMA_GET_NON_NULL_POINTER_FROM_POINTER_TAG (ecma_object_t, ext_object_p->u.bound_function.target_function);
        continue;
      }
      case ECMA_OBJECT_TYPE_CONSTRUCTOR_FUNCTION:
      {
        ecma_value_t script_value = ((ecma_extended_object_t *) object_p)->u.constructor_function.script_value;
        script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);
        break;
      }
      default:
      {
        return NULL;
      }
    }

    break;
  }

  jjs_source_info_t *source_info_p = jmem_heap_alloc_block_null_on_error (sizeof (jjs_source_info_t));

  if (source_info_p == NULL)
  {
    return NULL;
  }

  if (script_p != NULL)
  {
    ecma_ref_ecma_string (ecma_get_string_from_value (script_p->source_code));

    source_info.enabled_fields |= JJS_SOURCE_INFO_HAS_SOURCE_CODE;
    source_info.source_code = script_p->source_code;
  }

  JJS_ASSERT (source_info.enabled_fields != 0);

  *source_info_p = source_info;
  return source_info_p;
#else /* !JJS_FUNCTION_TO_STRING */
  JJS_UNUSED (value);
  return NULL;
#endif /* JJS_FUNCTION_TO_STRING */
} /* jjs_source_info */

/**
 * Frees the the source info structure returned by jjs_source_info.
 */
void
jjs_source_info_free (jjs_source_info_t *source_info_p) /**< source info block */
{
  jjs_assert_api_enabled ();

#if JJS_FUNCTION_TO_STRING
  if (source_info_p != NULL)
  {
    ecma_free_value (source_info_p->source_code);
    ecma_free_value (source_info_p->function_arguments);
    jmem_heap_free_block (source_info_p, sizeof (jjs_source_info_t));
  }
#else /* !JJS_FUNCTION_TO_STRING */
  JJS_UNUSED (source_info_p);
#endif /* JJS_FUNCTION_TO_STRING */
} /* jjs_source_info_free */

/**
 * Replaces the currently active realm with another realm.
 *
 * The replacement should be temporary, and the original realm must be
 * restored after the tasks are completed. During the replacement, the
 * realm must be referenced by the application (i.e. the gc must not
 * reclaim it). This is also true to the returned previously active
 * realm, so there is no need to free the value after the restoration.
 *
 * @return previous realm value - if the passed value is a realm
 *         exception - otherwise
 */
jjs_value_t
jjs_set_realm (jjs_value_t realm_value) /**< JJS api value */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_REALMS
  if (ecma_is_value_object (realm_value))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (realm_value);

    if (ecma_builtin_is_global (object_p))
    {
      ecma_global_object_t *previous_global_object_p = JJS_CONTEXT (global_object_p);
      JJS_CONTEXT (global_object_p) = (ecma_global_object_t *) object_p;
      return ecma_make_object_value ((ecma_object_t *) previous_global_object_p);
    }
  }

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PASSED_ARGUMENT_IS_NOT_A_REALM));
#else /* !JJS_BUILTIN_REALMS */
  JJS_UNUSED (realm_value);
  return jjs_throw_sz (JJS_ERROR_REFERENCE, ecma_get_error_msg (ECMA_ERR_REALM_IS_NOT_AVAILABLE));
#endif /* JJS_BUILTIN_REALMS */
} /* jjs_set_realm */

/**
 * Gets the 'this' binding of a realm
 *
 * @return type error - if realm_value is not a realm
 *         this value - otherwise
 */
jjs_value_t
jjs_realm_this (jjs_value_t realm) /**< realm value */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_REALMS
  if (ecma_is_value_object (realm))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (realm);

    if (ecma_builtin_is_global (object_p))
    {
      ecma_global_object_t *global_object_p = (ecma_global_object_t *) object_p;

      ecma_ref_object (ecma_get_object_from_value (global_object_p->this_binding));
      return global_object_p->this_binding;
    }
  }

#else /* !JJS_BUILTIN_REALMS */
  ecma_object_t *global_object_p = ecma_builtin_get_global ();

  if (realm == ecma_make_object_value (global_object_p))
  {
    ecma_ref_object (global_object_p);
    return realm;
  }
#endif /* JJS_BUILTIN_REALMS */

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PASSED_ARGUMENT_IS_NOT_A_REALM));
} /* jjs_realm_this */

/**
 * Sets the 'this' binding of a realm
 *
 * This function must be called before executing any script on the realm.
 * Otherwise the operation is undefined.
 *
 * @return type error - if realm_value is not a realm or this_value is not object
 *         true - otherwise
 */
jjs_value_t
jjs_realm_set_this (jjs_value_t realm, /**< realm value */
                      jjs_value_t this_value) /**< this value */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_REALMS
  if (!ecma_is_value_object (this_value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_SECOND_ARGUMENT_MUST_BE_AN_OBJECT));
  }

  if (ecma_is_value_object (realm))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (realm);

    if (ecma_builtin_is_global (object_p))
    {
      ecma_global_object_t *global_object_p = (ecma_global_object_t *) object_p;
      global_object_p->this_binding = this_value;

      ecma_object_t *global_lex_env_p = ecma_create_object_lex_env (NULL, ecma_get_object_from_value (this_value));

      ECMA_SET_NON_NULL_POINTER (global_object_p->global_env_cp, global_lex_env_p);
      global_object_p->global_scope_cp = global_object_p->global_env_cp;

      ecma_deref_object (global_lex_env_p);
      return ECMA_VALUE_TRUE;
    }
  }

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_FIRST_ARGUMENT_IS_NOT_A_REALM));
#else /* !JJS_BUILTIN_REALMS */
  JJS_UNUSED (realm);
  JJS_UNUSED (this_value);
  return jjs_throw_sz (JJS_ERROR_REFERENCE, ecma_get_error_msg (ECMA_ERR_REALM_IS_NOT_AVAILABLE));
#endif /* JJS_BUILTIN_REALMS */
} /* jjs_realm_set_this */

/**
 * Check if the given value is an ArrayBuffer object.
 *
 * @return true - if it is an ArrayBuffer object
 *         false - otherwise
 */
bool
jjs_value_is_arraybuffer (const jjs_value_t value) /**< value to check if it is an ArrayBuffer */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  return ecma_is_arraybuffer (value);
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (value);
  return false;
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_value_is_arraybuffer */

/**
 * Creates an ArrayBuffer object with the given length (size).
 *
 * Notes:
 *      * the length is specified in bytes.
 *      * returned value must be freed with jjs_value_free, when it is no longer needed.
 *      * if the typed arrays are disabled this will return a TypeError.
 *
 * @return value of the constructed ArrayBuffer object
 */
jjs_value_t
jjs_arraybuffer (const jjs_length_t size) /**< size of the backing store allocated
                                               *   for the array buffer in bytes */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  return jjs_return (ecma_make_object_value (ecma_arraybuffer_new_object (size)));
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (size);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_arraybuffer */

/**
 * Creates an ArrayBuffer object with user specified buffer.
 *
 * Notes:
 *     * the size is specified in bytes.
 *     * the buffer passed should be at least the specified bytes big.
 *     * if the typed arrays are disabled this will return a TypeError.
 *     * if the size is zero or buffer_p is a null pointer this will return an empty ArrayBuffer.
 *
 * @return value of the newly constructed array buffer object
 */
jjs_value_t
jjs_arraybuffer_external (uint8_t *buffer_p, /**< the backing store used by the array buffer object */
                            jjs_length_t size, /**< size of the buffer in bytes */
                            void *user_p) /**< user pointer assigned to the array buffer object */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  ecma_object_t *arraybuffer_p;

  if (JJS_UNLIKELY (size == 0))
  {
    arraybuffer_p = ecma_arraybuffer_new_object (0);
  }
  else
  {
    arraybuffer_p = ecma_arraybuffer_create_object_with_buffer (ECMA_OBJECT_CLASS_ARRAY_BUFFER, size);

    ecma_arraybuffer_pointer_t *arraybuffer_pointer_p = (ecma_arraybuffer_pointer_t *) arraybuffer_p;
    arraybuffer_pointer_p->arraybuffer_user_p = user_p;

    if (buffer_p != NULL)
    {
      arraybuffer_pointer_p->extended_object.u.cls.u1.array_buffer_flags |= ECMA_ARRAYBUFFER_ALLOCATED;
      arraybuffer_pointer_p->buffer_p = buffer_p;
    }
  }

  return jjs_return (ecma_make_object_value (arraybuffer_p));
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (size);
  JJS_UNUSED (buffer_p);
  JJS_UNUSED (user_p);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_arraybuffer_external */

/**
 * Check if the given value is a SharedArrayBuffer object.
 *
 * @return true - if it is a SharedArrayBuffer object
 *         false - otherwise
 */
bool
jjs_value_is_shared_arraybuffer (const jjs_value_t value) /**< value to check if it is a SharedArrayBuffer */
{
  jjs_assert_api_enabled ();

  return ecma_is_shared_arraybuffer (value);
} /* jjs_value_is_shared_arraybuffer */

/**
 * Creates a SharedArrayBuffer object with the given length (size).
 *
 * Notes:
 *      * the length is specified in bytes.
 *      * returned value must be freed with jjs_value_free, when it is no longer needed.
 *      * if the typed arrays are disabled this will return a TypeError.
 *
 * @return value of the constructed SharedArrayBuffer object
 */
jjs_value_t
jjs_shared_arraybuffer (jjs_length_t size) /**< size of the backing store allocated
                                                *   for the shared array buffer in bytes */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_SHAREDARRAYBUFFER
  return jjs_return (ecma_make_object_value (ecma_shared_arraybuffer_new_object (size)));
#else /* !JJS_BUILTIN_SHAREDARRAYBUFFER */
  JJS_UNUSED (size);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_SHARED_ARRAYBUFFER_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_SHAREDARRAYBUFFER */
} /* jjs_shared_arraybuffer */

/**
 * Creates a SharedArrayBuffer object with user specified buffer.
 *
 * Notes:
 *     * the size is specified in bytes.
 *     * the buffer passed should be at least the specified bytes big.
 *     * if the typed arrays are disabled this will return a TypeError.
 *     * if the size is zero or buffer_p is a null pointer this will return an empty SharedArrayBuffer.
 *
 * @return value of the newly constructed shared array buffer object
 */
jjs_value_t
jjs_shared_arraybuffer_external (uint8_t *buffer_p, /**< the backing store used by the
                                                       *   shared array buffer object */
                                   jjs_length_t size, /**< size of the buffer in bytes */
                                   void *user_p) /**< user pointer assigned to the
                                                  *   shared array buffer object */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_SHAREDARRAYBUFFER
  ecma_object_t *shared_arraybuffer_p;

  if (JJS_UNLIKELY (size == 0))
  {
    shared_arraybuffer_p = ecma_shared_arraybuffer_new_object (0);
  }
  else
  {
    shared_arraybuffer_p = ecma_arraybuffer_create_object_with_buffer (ECMA_OBJECT_CLASS_SHARED_ARRAY_BUFFER, size);

    ecma_arraybuffer_pointer_t *shared_arraybuffer_pointer_p = (ecma_arraybuffer_pointer_t *) shared_arraybuffer_p;
    shared_arraybuffer_pointer_p->arraybuffer_user_p = user_p;

    if (buffer_p != NULL)
    {
      shared_arraybuffer_pointer_p->extended_object.u.cls.u1.array_buffer_flags |= ECMA_ARRAYBUFFER_ALLOCATED;
      shared_arraybuffer_pointer_p->buffer_p = buffer_p;
    }
  }

  return ecma_make_object_value (shared_arraybuffer_p);
#else /* !JJS_BUILTIN_SHAREDARRAYBUFFER */
  JJS_UNUSED (size);
  JJS_UNUSED (buffer_p);
  JJS_UNUSED (user_p);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_SHARED_ARRAYBUFFER_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_SHAREDARRAYBUFFER */
} /* jjs_shared_arraybuffer_external */

#if JJS_BUILTIN_TYPEDARRAY

/**
 * Allocate a backing store for an array buffer, ignores allocation fails.
 *
 * @return true on success,
 *         false otherwise
 */
static bool
jjs_arraybuffer_allocate_buffer_no_throw (ecma_object_t *arraybuffer_p) /**< ArrayBuffer object */
{
  JJS_ASSERT (!(ECMA_ARRAYBUFFER_GET_FLAGS (arraybuffer_p) & ECMA_ARRAYBUFFER_ALLOCATED));

  if (ECMA_ARRAYBUFFER_GET_FLAGS (arraybuffer_p) & ECMA_ARRAYBUFFER_DETACHED)
  {
    return false;
  }

  return ecma_arraybuffer_allocate_buffer (arraybuffer_p) != ECMA_VALUE_ERROR;
} /* jjs_arraybuffer_allocate_buffer_no_throw */

#endif /* JJS_BUILTIN_TYPEDARRAY */

/**
 * Copy bytes into the ArrayBuffer or SharedArrayBuffer from a buffer.
 *
 * Note:
 *     * returns 0, if the passed object is not an ArrayBuffer or SharedArrayBuffer
 *
 * @return number of bytes copied into the ArrayBuffer or SharedArrayBuffer.
 */
jjs_length_t
jjs_arraybuffer_write (jjs_value_t value, /**< target ArrayBuffer or SharedArrayBuffer */
                         jjs_length_t offset, /**< start offset of the ArrayBuffer */
                         const uint8_t *buf_p, /**< buffer to copy from */
                         jjs_length_t buf_size) /**< number of bytes to copy from the buffer */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (!(ecma_is_arraybuffer (value) || ecma_is_shared_arraybuffer (value)))
  {
    return 0;
  }

  ecma_object_t *arraybuffer_p = ecma_get_object_from_value (value);

  if (!(ECMA_ARRAYBUFFER_GET_FLAGS (arraybuffer_p) & ECMA_ARRAYBUFFER_ALLOCATED)
      && !jjs_arraybuffer_allocate_buffer_no_throw (arraybuffer_p))
  {
    return 0;
  }

  jjs_length_t length = ecma_arraybuffer_get_length (arraybuffer_p);

  if (offset >= length)
  {
    return 0;
  }

  jjs_length_t copy_count = JJS_MIN (length - offset, buf_size);

  if (copy_count > 0)
  {
    lit_utf8_byte_t *buffer_p = ecma_arraybuffer_get_buffer (arraybuffer_p);

    memcpy ((void *) (buffer_p + offset), (void *) buf_p, copy_count);
  }

  return copy_count;
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (value);
  JJS_UNUSED (offset);
  JJS_UNUSED (buf_p);
  JJS_UNUSED (buf_size);
  return 0;
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_arraybuffer_write */

/**
 * Copy bytes from a buffer into an ArrayBuffer or SharedArrayBuffer.
 *
 * Note:
 *     * if the object passed is not an ArrayBuffer or SharedArrayBuffer will return 0.
 *
 * @return number of bytes read from the ArrayBuffer.
 */
jjs_length_t
jjs_arraybuffer_read (const jjs_value_t value, /**< ArrayBuffer or SharedArrayBuffer to read from */
                        jjs_length_t offset, /**< start offset of the ArrayBuffer or SharedArrayBuffer */
                        uint8_t *buf_p, /**< destination buffer to copy to */
                        jjs_length_t buf_size) /**< number of bytes to copy into the buffer */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (!(ecma_is_arraybuffer (value) || ecma_is_shared_arraybuffer (value)))
  {
    return 0;
  }

  ecma_object_t *arraybuffer_p = ecma_get_object_from_value (value);

  if (!(ECMA_ARRAYBUFFER_GET_FLAGS (arraybuffer_p) & ECMA_ARRAYBUFFER_ALLOCATED)
      && !jjs_arraybuffer_allocate_buffer_no_throw (arraybuffer_p))
  {
    return 0;
  }

  jjs_length_t length = ecma_arraybuffer_get_length (arraybuffer_p);

  if (offset >= length)
  {
    return 0;
  }

  jjs_length_t copy_count = JJS_MIN (length - offset, buf_size);

  if (copy_count > 0)
  {
    lit_utf8_byte_t *buffer_p = ecma_arraybuffer_get_buffer (arraybuffer_p);

    memcpy ((void *) buf_p, (void *) (buffer_p + offset), copy_count);
  }

  return copy_count;
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (value);
  JJS_UNUSED (offset);
  JJS_UNUSED (buf_p);
  JJS_UNUSED (buf_size);
  return 0;
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_arraybuffer_read */

/**
 * Get the length (size) of the ArrayBuffer or SharedArrayBuffer in bytes.
 *
 * Note:
 *     This is the 'byteLength' property of an ArrayBuffer or SharedArrayBuffer.
 *
 * @return the length of the ArrayBuffer in bytes.
 */
jjs_length_t
jjs_arraybuffer_size (const jjs_value_t value) /**< ArrayBuffer or SharedArrayBuffer */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (ecma_is_arraybuffer (value) || ecma_is_shared_arraybuffer (value))
  {
    ecma_object_t *arraybuffer_p = ecma_get_object_from_value (value);
    return ecma_arraybuffer_get_length (arraybuffer_p);
  }
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (value);
#endif /* JJS_BUILTIN_TYPEDARRAY */
  return 0;
} /* jjs_arraybuffer_size */

/**
 * Get a pointer for the start of the ArrayBuffer.
 *
 * Note:
 *    * This is a high-risk operation as the bounds are not checked
 *      when accessing the pointer elements.
 *
 * @return pointer to the back-buffer of the ArrayBuffer.
 *         pointer is NULL if:
 *            - the parameter is not an ArrayBuffer
 *            - an external ArrayBuffer has been detached
 */
uint8_t *
jjs_arraybuffer_data (const jjs_value_t array_buffer) /**< Array Buffer to use */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (!(ecma_is_arraybuffer (array_buffer) || ecma_is_shared_arraybuffer (array_buffer)))
  {
    return NULL;
  }

  ecma_object_t *arraybuffer_p = ecma_get_object_from_value (array_buffer);

  if (!(ECMA_ARRAYBUFFER_GET_FLAGS (arraybuffer_p) & ECMA_ARRAYBUFFER_ALLOCATED)
      && !jjs_arraybuffer_allocate_buffer_no_throw (arraybuffer_p))
  {
    return NULL;
  }

  return (uint8_t *) ecma_arraybuffer_get_buffer (arraybuffer_p);
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (array_buffer);
  return NULL;
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_arraybuffer_data */

/**
 * Get if the ArrayBuffer is detachable.
 *
 * @return boolean value - if success
 *         value marked with error flag - otherwise
 */
bool
jjs_arraybuffer_is_detachable (const jjs_value_t value) /**< ArrayBuffer */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (ecma_is_arraybuffer (value))
  {
    ecma_object_t *buffer_p = ecma_get_object_from_value (value);
    return !ecma_arraybuffer_is_detached (buffer_p);
  }
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (value);
#endif /* JJS_BUILTIN_TYPEDARRAY */
  return false;
} /* jjs_arraybuffer_is_detachable */

/**
 * Detach the underlying data block from ArrayBuffer and set its bytelength to 0.
 *
 * Note: if the ArrayBuffer has a separate data buffer, the free callback set by
 *       jjs_arraybuffer_set_allocation_callbacks is called for this buffer
 *
 * @return null value - if success
 *         value marked with error flag - otherwise
 */
jjs_value_t
jjs_arraybuffer_detach (jjs_value_t value) /**< ArrayBuffer */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (ecma_is_arraybuffer (value))
  {
    ecma_object_t *buffer_p = ecma_get_object_from_value (value);
    if (ecma_arraybuffer_detach (buffer_p))
    {
      return ECMA_VALUE_NULL;
    }
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ARRAY_BUFFER_DETACHED));
  }
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (value);
#endif /* JJS_BUILTIN_TYPEDARRAY */
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_EXPECTED_AN_ARRAYBUFFER));
} /* jjs_arraybuffer_detach */

/**
 * Checks whether a buffer is currently allocated for an array buffer or typed array.
 *
 * @return true, if a buffer is allocated for an array buffer or typed array
 *         false, otherwise
 */
bool
jjs_arraybuffer_has_buffer (const jjs_value_t value) /**< array buffer or typed array value */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (!ecma_is_value_object (value))
  {
    return false;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (value);

  if (ecma_object_is_typedarray (object_p))
  {
    object_p = ecma_typedarray_get_arraybuffer (object_p);
  }
  else if (!(ecma_object_class_is (object_p, ECMA_OBJECT_CLASS_ARRAY_BUFFER)
             || ecma_object_is_shared_arraybuffer (object_p)))
  {
    return false;
  }

  return (ECMA_ARRAYBUFFER_GET_FLAGS (object_p) & ECMA_ARRAYBUFFER_ALLOCATED) != 0;
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (value);
  return false;
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_arraybuffer_has_buffer */

/**
 * Array buffers which size is less or equal than the limit passed to this function are allocated in
 * a single memory block. The allocator callbacks set by jjs_arraybuffer_set_allocation_callbacks
 * are not called for these array buffers. The default limit is 256 bytes.
 */
void
jjs_arraybuffer_heap_allocation_limit (jjs_length_t allocation_limit) /**< maximum size of
                                                                           *   compact allocation */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  JJS_CONTEXT (arraybuffer_compact_allocation_limit) = allocation_limit;
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (allocation_limit);
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_arraybuffer_heap_allocation_limit */

/**
 * Set callbacks for allocating and freeing backing stores for array buffer objects.
 */
void
jjs_arraybuffer_allocator (jjs_arraybuffer_allocate_cb_t allocate_callback, /**< callback for allocating
                                                                                 *   array buffer memory */
                             jjs_arraybuffer_free_cb_t free_callback, /**< callback for freeing
                                                                         *   array buffer memory */
                             void *user_p) /**< user pointer passed to the callbacks */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  JJS_CONTEXT (arraybuffer_allocate_callback) = allocate_callback;
  JJS_CONTEXT (arraybuffer_free_callback) = free_callback;
  JJS_CONTEXT (arraybuffer_allocate_callback_user_p) = user_p;
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (allocate_callback);
  JJS_UNUSED (free_callback);
  JJS_UNUSED (user_p);
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_arraybuffer_allocator */

/**
 * DataView related functions
 */

/**
 * Creates a DataView object with the given ArrayBuffer, ByteOffset and ByteLength arguments.
 *
 * Notes:
 *      * returned value must be freed with jjs_value_free, when it is no longer needed.
 *      * if the DataView bulitin is disabled this will return a TypeError.
 *
 * @return value of the constructed DataView object - if success
 *         created error - otherwise
 */
jjs_value_t
jjs_dataview (const jjs_value_t array_buffer, /**< arraybuffer to create DataView from */
                jjs_length_t byte_offset, /**< offset in bytes, to the first byte in the buffer */
                jjs_length_t byte_length) /**< number of elements in the byte array */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_DATAVIEW
  if (ecma_is_value_exception (array_buffer))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_value_t arguments_p[3] = { array_buffer,
                                  ecma_make_uint32_value (byte_offset),
                                  ecma_make_uint32_value (byte_length) };
  ecma_object_t *old_new_target_p = JJS_CONTEXT (current_new_target_p);
  if (old_new_target_p == NULL)
  {
    JJS_CONTEXT (current_new_target_p) = ecma_builtin_get (ECMA_BUILTIN_ID_DATAVIEW);
  }

  ecma_value_t dataview_value = ecma_op_dataview_create (arguments_p, 3);
  JJS_CONTEXT (current_new_target_p) = old_new_target_p;
  return jjs_return (dataview_value);
#else /* !JJS_BUILTIN_DATAVIEW */
  JJS_UNUSED (array_buffer);
  JJS_UNUSED (byte_offset);
  JJS_UNUSED (byte_length);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_DATA_VIEW_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_DATAVIEW */
} /* jjs_dataview */

/**
 * Check if the given value is a DataView object.
 *
 * @return true - if it is a DataView object
 *         false - otherwise
 */
bool
jjs_value_is_dataview (const jjs_value_t value) /**< value to check if it is a DataView object */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_DATAVIEW
  return ecma_is_dataview (value);
#else /* !JJS_BUILTIN_DATAVIEW */
  JJS_UNUSED (value);
  return false;
#endif /* JJS_BUILTIN_DATAVIEW */
} /* jjs_value_is_dataview */

/**
 * Get the underlying ArrayBuffer from a DataView.
 *
 * Additionally the byteLength and byteOffset properties are also returned
 * which were specified when the DataView was created.
 *
 * Note:
 *     the returned value must be freed with a jjs_value_free call
 *
 * @return ArrayBuffer of a DataView
 *         TypeError if the object is not a DataView.
 */
jjs_value_t
jjs_dataview_buffer (const jjs_value_t value, /**< DataView to get the arraybuffer from */
                       jjs_length_t *byte_offset, /**< [out] byteOffset property */
                       jjs_length_t *byte_length) /**< [out] byteLength property */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_DATAVIEW
  if (ecma_is_value_exception (value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_dataview_object_t *dataview_p = ecma_op_dataview_get_object (value);

  if (JJS_UNLIKELY (dataview_p == NULL))
  {
    return ecma_create_exception_from_context ();
  }

  if (byte_offset != NULL)
  {
    *byte_offset = dataview_p->byte_offset;
  }

  if (byte_length != NULL)
  {
    *byte_length = dataview_p->header.u.cls.u3.length;
  }

  ecma_object_t *arraybuffer_p = dataview_p->buffer_p;
  ecma_ref_object (arraybuffer_p);

  return ecma_make_object_value (arraybuffer_p);
#else /* !JJS_BUILTIN_DATAVIEW */
  JJS_UNUSED (value);
  JJS_UNUSED (byte_offset);
  JJS_UNUSED (byte_length);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_DATA_VIEW_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_DATAVIEW */
} /* jjs_dataview_buffer */

/**
 * TypedArray related functions
 */

/**
 * Check if the given value is a TypedArray object.
 *
 * @return true - if it is a TypedArray object
 *         false - otherwise
 */
bool
jjs_value_is_typedarray (const jjs_value_t value) /**< value to check if it is a TypedArray */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  return ecma_is_typedarray (value);
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (value);
  return false;
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_value_is_typedarray */

#if JJS_BUILTIN_TYPEDARRAY
/**
 * TypedArray mapping type
 */
typedef struct
{
  jjs_typedarray_type_t api_type; /**< api type */
  ecma_builtin_id_t prototype_id; /**< prototype ID */
  ecma_typedarray_type_t id; /**< typedArray ID */
  uint8_t element_size_shift; /**< element size shift */
} jjs_typedarray_mapping_t;

/**
 * List of TypedArray mappings
 */
static jjs_typedarray_mapping_t jjs_typedarray_mappings[] = {
#define TYPEDARRAY_ENTRY(NAME, LIT_NAME, SIZE_SHIFT)                                                      \
  {                                                                                                       \
    JJS_TYPEDARRAY_##NAME, ECMA_BUILTIN_ID_##NAME##ARRAY_PROTOTYPE, ECMA_##LIT_NAME##_ARRAY, SIZE_SHIFT \
  }

  TYPEDARRAY_ENTRY (UINT8, UINT8, 0),       TYPEDARRAY_ENTRY (UINT8CLAMPED, UINT8_CLAMPED, 0),
  TYPEDARRAY_ENTRY (INT8, INT8, 0),         TYPEDARRAY_ENTRY (UINT16, UINT16, 1),
  TYPEDARRAY_ENTRY (INT16, INT16, 1),       TYPEDARRAY_ENTRY (UINT32, UINT32, 2),
  TYPEDARRAY_ENTRY (INT32, INT32, 2),       TYPEDARRAY_ENTRY (FLOAT32, FLOAT32, 2),
#if JJS_NUMBER_TYPE_FLOAT64
  TYPEDARRAY_ENTRY (FLOAT64, FLOAT64, 3),
#endif /* JJS_NUMBER_TYPE_FLOAT64 */
#if JJS_BUILTIN_BIGINT
  TYPEDARRAY_ENTRY (BIGINT64, BIGINT64, 3), TYPEDARRAY_ENTRY (BIGUINT64, BIGUINT64, 3),
#endif /* JJS_BUILTIN_BIGINT */
#undef TYPEDARRAY_ENTRY
};

/**
 * Helper function to get the TypedArray prototype, typedArray id, and element size shift
 * information.
 *
 * @return true - if the TypedArray information was found
 *         false - if there is no such TypedArray type
 */
static bool
jjs_typedarray_find_by_type (jjs_typedarray_type_t type_name, /**< type of the TypedArray */
                               ecma_builtin_id_t *prototype_id, /**< [out] found prototype object id */
                               ecma_typedarray_type_t *id, /**< [out] found typedArray id */
                               uint8_t *element_size_shift) /**< [out] found element size shift value */
{
  JJS_ASSERT (prototype_id != NULL);
  JJS_ASSERT (id != NULL);
  JJS_ASSERT (element_size_shift != NULL);

  for (uint32_t i = 0; i < sizeof (jjs_typedarray_mappings) / sizeof (jjs_typedarray_mappings[0]); i++)
  {
    if (type_name == jjs_typedarray_mappings[i].api_type)
    {
      *prototype_id = jjs_typedarray_mappings[i].prototype_id;
      *id = jjs_typedarray_mappings[i].id;
      *element_size_shift = jjs_typedarray_mappings[i].element_size_shift;
      return true;
    }
  }

  return false;
} /* jjs_typedarray_find_by_type */

#endif /* JJS_BUILTIN_TYPEDARRAY */

/**
 * Create a TypedArray object with a given type and length.
 *
 * Notes:
 *      * returns TypeError if an incorrect type (type_name) is specified.
 *      * byteOffset property will be set to 0.
 *      * byteLength property will be a multiple of the length parameter (based on the type).
 *
 * @return - new TypedArray object
 */
jjs_value_t
jjs_typedarray (jjs_typedarray_type_t type_name, /**< type of TypedArray to create */
                  jjs_length_t length) /**< element count of the new TypedArray */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  ecma_builtin_id_t prototype_id = 0;
  ecma_typedarray_type_t id = 0;
  uint8_t element_size_shift = 0;

  if (!jjs_typedarray_find_by_type (type_name, &prototype_id, &id, &element_size_shift))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_FOR_TYPEDARRAY));
  }

  ecma_object_t *prototype_obj_p = ecma_builtin_get (prototype_id);

  ecma_value_t array_value =
    ecma_typedarray_create_object_with_length (length, NULL, prototype_obj_p, element_size_shift, id);

  JJS_ASSERT (!ECMA_IS_VALUE_ERROR (array_value));

  return array_value;
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (type_name);
  JJS_UNUSED (length);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_typedarray */

/**
 * Create a TypedArray object using the given arraybuffer and size information.
 *
 * Notes:
 *      * returns TypeError if an incorrect type (type_name) is specified.
 *      * this is the 'new %TypedArray%(arraybuffer, byteOffset, length)' equivalent call.
 *
 * @return - new TypedArray object
 */
jjs_value_t
jjs_typedarray_with_buffer_span (jjs_typedarray_type_t type, /**< type of TypedArray to create */
                                   const jjs_value_t arraybuffer, /**< ArrayBuffer to use */
                                   jjs_length_t byte_offset, /**< offset for the ArrayBuffer */
                                   jjs_length_t length) /**< number of elements to use from ArrayBuffer */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (ecma_is_value_exception (arraybuffer))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  ecma_builtin_id_t prototype_id = 0;
  ecma_typedarray_type_t id = 0;
  uint8_t element_size_shift = 0;

  if (!jjs_typedarray_find_by_type (type, &prototype_id, &id, &element_size_shift))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_FOR_TYPEDARRAY));
  }

  if (!ecma_is_arraybuffer (arraybuffer))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ARGUMENT_NOT_ARRAY_BUFFER));
  }

  ecma_object_t *prototype_obj_p = ecma_builtin_get (prototype_id);
  ecma_value_t arguments_p[3] = { arraybuffer, ecma_make_uint32_value (byte_offset), ecma_make_uint32_value (length) };

  ecma_value_t array_value = ecma_op_create_typedarray (arguments_p, 3, prototype_obj_p, element_size_shift, id);
  ecma_free_value (arguments_p[1]);
  ecma_free_value (arguments_p[2]);

  return jjs_return (array_value);
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (type);
  JJS_UNUSED (arraybuffer);
  JJS_UNUSED (byte_offset);
  JJS_UNUSED (length);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_typedarray_with_buffer_span */

/**
 * Create a TypedArray object using the given arraybuffer and size information.
 *
 * Notes:
 *      * returns TypeError if an incorrect type (type_name) is specified.
 *      * this is the 'new %TypedArray%(arraybuffer)' equivalent call.
 *
 * @return - new TypedArray object
 */
jjs_value_t
jjs_typedarray_with_buffer (jjs_typedarray_type_t type, /**< type of TypedArray to create */
                              const jjs_value_t arraybuffer) /**< ArrayBuffer to use */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (ecma_is_value_exception (arraybuffer))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  jjs_length_t byte_length = jjs_arraybuffer_size (arraybuffer);
  return jjs_typedarray_with_buffer_span (type, arraybuffer, 0, byte_length);
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (type);
  JJS_UNUSED (arraybuffer);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_typedarray_with_buffer */

/**
 * Get the type of the TypedArray.
 *
 * @return - type of the TypedArray
 *         - JJS_TYPEDARRAY_INVALID if the argument is not a TypedArray
 */
jjs_typedarray_type_t
jjs_typedarray_type (const jjs_value_t value) /**< object to get the TypedArray type */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (!ecma_is_typedarray (value))
  {
    return JJS_TYPEDARRAY_INVALID;
  }

  ecma_object_t *array_p = ecma_get_object_from_value (value);
  ecma_typedarray_type_t class_type = ecma_get_typedarray_id (array_p);

  for (uint32_t i = 0; i < sizeof (jjs_typedarray_mappings) / sizeof (jjs_typedarray_mappings[0]); i++)
  {
    if (class_type == jjs_typedarray_mappings[i].id)
    {
      return jjs_typedarray_mappings[i].api_type;
    }
  }
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (value);
#endif /* JJS_BUILTIN_TYPEDARRAY */

  return JJS_TYPEDARRAY_INVALID;
} /* jjs_typedarray_type */

/**
 * Get the element count of the TypedArray.
 *
 * @return length of the TypedArray.
 */
jjs_length_t
jjs_typedarray_length (const jjs_value_t value) /**< TypedArray to query */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (ecma_is_typedarray (value))
  {
    ecma_object_t *array_p = ecma_get_object_from_value (value);
    return ecma_typedarray_get_length (array_p);
  }
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (value);
#endif /* JJS_BUILTIN_TYPEDARRAY */

  return 0;
} /* jjs_typedarray_length */

/**
 * Get the underlying ArrayBuffer from a TypedArray.
 *
 * Additionally the byteLength and byteOffset properties are also returned
 * which were specified when the TypedArray was created.
 *
 * Note:
 *     the returned value must be freed with a jjs_value_free call
 *
 * @return ArrayBuffer of a TypedArray
 *         TypeError if the object is not a TypedArray.
 */
jjs_value_t
jjs_typedarray_buffer (const jjs_value_t value, /**< TypedArray to get the arraybuffer from */
                         jjs_length_t *byte_offset, /**< [out] byteOffset property */
                         jjs_length_t *byte_length) /**< [out] byteLength property */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_TYPEDARRAY
  if (!ecma_is_typedarray (value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_OBJECT_IS_NOT_A_TYPEDARRAY));
  }

  ecma_object_t *array_p = ecma_get_object_from_value (value);
  uint8_t shift = ecma_typedarray_get_element_size_shift (array_p);

  if (byte_length != NULL)
  {
    *byte_length = (jjs_length_t) (ecma_typedarray_get_length (array_p) << shift);
  }

  if (byte_offset != NULL)
  {
    *byte_offset = (jjs_length_t) ecma_typedarray_get_offset (array_p);
  }

  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (array_p);
  ecma_ref_object (arraybuffer_p);
  return jjs_return (ecma_make_object_value (arraybuffer_p));
#else /* !JJS_BUILTIN_TYPEDARRAY */
  JJS_UNUSED (value);
  JJS_UNUSED (byte_length);
  JJS_UNUSED (byte_offset);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_TYPEDARRAY */
} /* jjs_typedarray_buffer */

/**
 * Parse the given input buffer as a JSON string. The behaviour is equivalent with the "JSON.parse(string)" JS
 * call. The input buffer can be encoded as either cesu-8 or utf-8, but it is the callers responsibility to make sure
 * the encoding is valid.
 *
 * @return object value, or exception
 */
jjs_value_t
jjs_json_parse (const jjs_char_t *string_p, /**< json string */
                  jjs_size_t string_size) /**< json string size */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_JSON
  ecma_value_t ret_value = ecma_builtin_json_parse_buffer (string_p, string_size);

  if (ecma_is_value_undefined (ret_value))
  {
    ret_value = jjs_throw_sz (JJS_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_JSON_STRING_PARSE_ERROR));
  }

  return jjs_return (ret_value);
#else /* !JJS_BUILTIN_JSON */
  JJS_UNUSED (string_p);
  JJS_UNUSED (string_size);

  return jjs_throw_sz (JJS_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_JSON_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_JSON */
} /* jjs_json_parse */

/**
 * Parse the given input buffer as a JSON string. The behaviour is equivalent with the "JSON.parse(string)" JS
 * call. The input buffer can be encoded as either cesu-8 or utf-8, but it is the callers responsibility to make sure
 * the encoding is valid.
 *
 * @return object value, or exception
 */
jjs_value_t
jjs_json_parse_sz (const char* string_p) /**< null-terminated json string */
{
  jjs_assert_api_enabled ();
  return jjs_json_parse ((const jjs_char_t*) string_p, string_p ? (jjs_size_t) strlen (string_p) : 0);
} /* jjs_json_parse_sz */

/**
 * Load a JSON object from file.
 *
 * @param filename path to JSON file
 * @param filename_o filename reference ownership
 * @return object value, or exception
 */
jjs_value_t
jjs_json_parse_file (jjs_value_t filename, jjs_value_ownership_t filename_o)
{
  jjs_assert_api_enabled ();
#if JJS_BUILTIN_JSON
  jjs_value_t buffer = jjs_platform_read_file (filename, filename_o, NULL);

  if (jjs_value_is_exception (buffer))
  {
    return buffer;
  }

  jjs_char_t* json_p = jjs_arraybuffer_data (buffer);
  jjs_size_t len = jjs_arraybuffer_size (buffer);

  jjs_value_t result = jjs_json_parse (json_p, len);

  jjs_value_free (buffer);

  return result;
#else /* !JJS_BUILTIN_JSON */
  JJS_DISOWN (filename, filename_o);
  return jjs_throw_sz (JJS_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_JSON_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_JSON */
} /* jjs_json_parse_file */

/**
 * Create a JSON string from a JavaScript value.
 *
 * The behaviour is equivalent with the "JSON.stringify(input_value)" JS call.
 *
 * Note:
 *      The returned value must be freed with jjs_value_free,
 *
 * @return - jjs_value_t containing a JSON string.
 *         - Error value if there was a problem during the stringification.
 */
jjs_value_t
jjs_json_stringify (const jjs_value_t input_value) /**< a value to stringify */
{
  jjs_assert_api_enabled ();
#if JJS_BUILTIN_JSON
  if (ecma_is_value_exception (input_value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  ecma_value_t ret_value = ecma_builtin_json_stringify_no_opts (input_value);

  if (ecma_is_value_undefined (ret_value))
  {
    ret_value = jjs_throw_sz (JJS_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_JSON_STRINGIFY_ERROR));
  }

  return jjs_return (ret_value);
#else /* JJS_BUILTIN_JSON */
  JJS_UNUSED (input_value);

  return jjs_throw_sz (JJS_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_JSON_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_JSON */
} /* jjs_json_stringify */

/**
 * Create a container type specified in jjs_container_type_t.
 * The container can be created with a list of arguments, which will be passed to the container constructor to be
 * inserted to the container.
 *
 * Note:
 *      The returned value must be freed with jjs_value_free
 * @return jjs_value_t representing a container with the given type.
 */
jjs_value_t
jjs_container (jjs_container_type_t container_type, /**< Type of the container */
                 const jjs_value_t *arguments_list_p, /**< arguments list */
                 jjs_length_t arguments_list_len) /**< Length of arguments list */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_CONTAINER
  for (jjs_length_t i = 0; i < arguments_list_len; i++)
  {
    if (ecma_is_value_exception (arguments_list_p[i]))
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
    }
  }

  lit_magic_string_id_t lit_id;
  ecma_builtin_id_t proto_id;
  ecma_builtin_id_t ctor_id;

  switch (container_type)
  {
    case JJS_CONTAINER_TYPE_MAP:
    {
      lit_id = LIT_MAGIC_STRING_MAP_UL;
      proto_id = ECMA_BUILTIN_ID_MAP_PROTOTYPE;
      ctor_id = ECMA_BUILTIN_ID_MAP;
      break;
    }
    case JJS_CONTAINER_TYPE_SET:
    {
      lit_id = LIT_MAGIC_STRING_SET_UL;
      proto_id = ECMA_BUILTIN_ID_SET_PROTOTYPE;
      ctor_id = ECMA_BUILTIN_ID_SET;
      break;
    }
    case JJS_CONTAINER_TYPE_WEAKMAP:
    {
      lit_id = LIT_MAGIC_STRING_WEAKMAP_UL;
      proto_id = ECMA_BUILTIN_ID_WEAKMAP_PROTOTYPE;
      ctor_id = ECMA_BUILTIN_ID_WEAKMAP;
      break;
    }
    case JJS_CONTAINER_TYPE_WEAKSET:
    {
      lit_id = LIT_MAGIC_STRING_WEAKSET_UL;
      proto_id = ECMA_BUILTIN_ID_WEAKSET_PROTOTYPE;
      ctor_id = ECMA_BUILTIN_ID_WEAKSET;
      break;
    }
    default:
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INVALID_CONTAINER_TYPE));
    }
  }
  ecma_object_t *old_new_target_p = JJS_CONTEXT (current_new_target_p);

  if (old_new_target_p == NULL)
  {
    JJS_CONTEXT (current_new_target_p) = ecma_builtin_get (ctor_id);
  }

  ecma_value_t container_value = ecma_op_container_create (arguments_list_p, arguments_list_len, lit_id, proto_id);

  JJS_CONTEXT (current_new_target_p) = old_new_target_p;
  return jjs_return (container_value);
#else /* !JJS_BUILTIN_CONTAINER */
  JJS_UNUSED (arguments_list_p);
  JJS_UNUSED (arguments_list_len);
  JJS_UNUSED (container_type);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_CONTAINER */
} /* jjs_container */

/**
 * Get the type of the given container object.
 *
 * @return Corresponding type to the given container object.
 */
jjs_container_type_t
jjs_container_type (const jjs_value_t value) /**< the container object */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_CONTAINER
  if (ecma_is_value_object (value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (value);

    if (ecma_object_class_is (obj_p, ECMA_OBJECT_CLASS_CONTAINER))
    {
      switch (((ecma_extended_object_t *) obj_p)->u.cls.u2.container_id)
      {
        case LIT_MAGIC_STRING_MAP_UL:
        {
          return JJS_CONTAINER_TYPE_MAP;
        }
        case LIT_MAGIC_STRING_SET_UL:
        {
          return JJS_CONTAINER_TYPE_SET;
        }
        case LIT_MAGIC_STRING_WEAKMAP_UL:
        {
          return JJS_CONTAINER_TYPE_WEAKMAP;
        }
        case LIT_MAGIC_STRING_WEAKSET_UL:
        {
          return JJS_CONTAINER_TYPE_WEAKSET;
        }
        default:
        {
          return JJS_CONTAINER_TYPE_INVALID;
        }
      }
    }
  }

#else /* !JJS_BUILTIN_CONTAINER */
  JJS_UNUSED (value);
#endif /* JJS_BUILTIN_CONTAINER */
  return JJS_CONTAINER_TYPE_INVALID;
} /* jjs_container_type */

/**
 * Return a new array containing elements from a Container or a Container Iterator.
 * Sets the boolean input value to `true` if the container object has key/value pairs.
 *
 * Note:
 *     the returned value must be freed with a jjs_value_free call
 *
 * @return an array of items for maps/sets or their iterators, error otherwise
 */
jjs_value_t
jjs_container_to_array (const jjs_value_t value, /**< the container or iterator object */
                          bool *is_key_value_p) /**< [out] is key-value structure */
{
  jjs_assert_api_enabled ();

#if JJS_BUILTIN_CONTAINER
  if (!ecma_is_value_object (value))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NEEDED));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (value);

  if (ecma_get_object_type (obj_p) != ECMA_OBJECT_TYPE_CLASS)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NEEDED));
  }

  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

  uint32_t entry_count;
  uint8_t entry_size;

  uint32_t index = 0;
  uint8_t iterator_kind = ECMA_ITERATOR__COUNT;
  ecma_value_t *start_p;

  *is_key_value_p = false;

  if (ext_obj_p->u.cls.type == ECMA_OBJECT_CLASS_MAP_ITERATOR
      || ext_obj_p->u.cls.type == ECMA_OBJECT_CLASS_SET_ITERATOR)
  {
    ecma_value_t iterated_value = ext_obj_p->u.cls.u3.iterated_value;

    if (ecma_is_value_empty (iterated_value))
    {
      return ecma_op_new_array_object_from_collection (ecma_new_collection (), false);
    }

    ecma_extended_object_t *map_object_p = (ecma_extended_object_t *) (ecma_get_object_from_value (iterated_value));

    ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t, map_object_p->u.cls.u3.value);
    entry_count = ECMA_CONTAINER_ENTRY_COUNT (container_p);
    index = ext_obj_p->u.cls.u2.iterator_index;

    entry_size = ecma_op_container_entry_size (map_object_p->u.cls.u2.container_id);
    start_p = ECMA_CONTAINER_START (container_p);

    iterator_kind = ext_obj_p->u.cls.u1.iterator_kind;
  }
  else if (jjs_container_type (value) != JJS_CONTAINER_TYPE_INVALID)
  {
    ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t, ext_obj_p->u.cls.u3.value);
    entry_count = ECMA_CONTAINER_ENTRY_COUNT (container_p);
    entry_size = ecma_op_container_entry_size (ext_obj_p->u.cls.u2.container_id);

    index = 0;
    iterator_kind = ECMA_ITERATOR_KEYS;
    start_p = ECMA_CONTAINER_START (container_p);

    if (ext_obj_p->u.cls.u2.container_id == LIT_MAGIC_STRING_MAP_UL
        || ext_obj_p->u.cls.u2.container_id == LIT_MAGIC_STRING_WEAKMAP_UL)
    {
      iterator_kind = ECMA_ITERATOR_ENTRIES;
    }
  }
  else
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NEEDED));
  }

  *is_key_value_p = (iterator_kind == ECMA_ITERATOR_ENTRIES);
  ecma_collection_t *collection_buffer = ecma_new_collection ();

  for (uint32_t i = index; i < entry_count; i += entry_size)
  {
    ecma_value_t *entry_p = start_p + i;

    if (ecma_is_value_empty (*entry_p))
    {
      continue;
    }

    if (iterator_kind != ECMA_ITERATOR_VALUES)
    {
      ecma_collection_push_back (collection_buffer, ecma_copy_value_if_not_object (entry_p[0]));
    }

    if (iterator_kind != ECMA_ITERATOR_KEYS)
    {
      ecma_collection_push_back (collection_buffer, ecma_copy_value_if_not_object (entry_p[1]));
    }
  }
  return ecma_op_new_array_object_from_collection (collection_buffer, false);
#else /* !JJS_BUILTIN_CONTAINER */
  JJS_UNUSED (value);
  JJS_UNUSED (is_key_value_p);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_CONTAINER */
} /* jjs_container_to_array */

/**
 * Perform container operation on the given operands (add, get, set, has, delete, size, clear).
 *
 * @return error - if argument is invalid or operation is unsuccessful or unsupported
 *                 result of the container operation - otherwise.
 */
jjs_value_t
jjs_container_op (jjs_container_op_t operation, /**< container operation */
                    jjs_value_t container, /**< container */
                    const jjs_value_t *arguments, /**< list of arguments */
                    uint32_t arguments_number) /**< number of arguments */
{
  jjs_assert_api_enabled ();
#if JJS_BUILTIN_CONTAINER
  if (!ecma_is_value_object (container))
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_IS_NOT_AN_OBJECT));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (container);

  if (ecma_get_object_type (obj_p) != ECMA_OBJECT_TYPE_CLASS)
  {
    return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_IS_NOT_A_CONTAINER_OBJECT));
  }
  uint16_t type = ((ecma_extended_object_t *) obj_p)->u.cls.u2.container_id;
  ecma_extended_object_t *container_object_p = ecma_op_container_get_object (container, type);

  if (container_object_p == NULL)
  {
    return ecma_create_exception_from_context ();
  }

  switch (operation)
  {
    case JJS_CONTAINER_OP_ADD:
    case JJS_CONTAINER_OP_DELETE:
    case JJS_CONTAINER_OP_GET:
    case JJS_CONTAINER_OP_HAS:
    {
      if (arguments_number != 1 || ecma_is_value_exception (arguments[0]))
      {
        return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
      }
      break;
    }
    case JJS_CONTAINER_OP_SET:
    {
      if (arguments_number != 2 || ecma_is_value_exception (arguments[0]) || ecma_is_value_exception (arguments[1]))
      {
        return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
      }
      break;
    }
    case JJS_CONTAINER_OP_CLEAR:
    case JJS_CONTAINER_OP_SIZE:
    {
      if (arguments_number != 0)
      {
        return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
      }
      break;
    }
    default:
    {
      return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
    }
  }

  jjs_value_t result;

  switch (operation)
  {
    case JJS_CONTAINER_OP_ADD:
    {
      if (type == LIT_MAGIC_STRING_MAP_UL || type == LIT_MAGIC_STRING_WEAKMAP_UL)
      {
        return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_CALL));
      }
      result = ecma_op_container_set (container_object_p, arguments[0], arguments[0], type);
      break;
    }
    case JJS_CONTAINER_OP_GET:
    {
      if (type == LIT_MAGIC_STRING_SET_UL || type == LIT_MAGIC_STRING_WEAKSET_UL)
      {
        return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_CALL));
      }
      result = ecma_op_container_get (container_object_p, arguments[0], type);
      break;
    }
    case JJS_CONTAINER_OP_SET:
    {
      if (type == LIT_MAGIC_STRING_SET_UL || type == LIT_MAGIC_STRING_WEAKSET_UL)
      {
        return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_CALL));
      }
      result = ecma_op_container_set (container_object_p, arguments[0], arguments[1], type);
      break;
    }
    case JJS_CONTAINER_OP_HAS:
    {
      result = ecma_op_container_has (container_object_p, arguments[0], type);
      break;
    }
    case JJS_CONTAINER_OP_DELETE:
    {
      if (type == LIT_MAGIC_STRING_WEAKMAP_UL || type == LIT_MAGIC_STRING_WEAKSET_UL)
      {
        result = ecma_op_container_delete_weak (container_object_p, arguments[0], type);
        break;
      }
      result = ecma_op_container_delete (container_object_p, arguments[0], type);
      break;
    }
    case JJS_CONTAINER_OP_SIZE:
    {
      result = ecma_op_container_size (container_object_p);
      break;
    }
    case JJS_CONTAINER_OP_CLEAR:
    {
      if (type == LIT_MAGIC_STRING_WEAKSET_UL || type == LIT_MAGIC_STRING_WEAKMAP_UL)
      {
        return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_CALL));
      }
      result = ecma_op_container_clear (container_object_p);
      break;
    }
    default:
    {
      result = jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_UNSUPPORTED_CONTAINER_OPERATION));
      break;
    }
  }
  return jjs_return (result);
#else /* !JJS_BUILTIN_CONTAINER */
  JJS_UNUSED (operation);
  JJS_UNUSED (container);
  JJS_UNUSED (arguments);
  JJS_UNUSED (arguments_number);
  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NOT_SUPPORTED));
#endif /* JJS_BUILTIN_CONTAINER */
} /* jjs_container_op */

/**
 * Write a JS string to a fmt stream.
 */
static void
fmt_write_string (const jjs_fmt_stream_t *stream_p, jjs_value_t value, jjs_value_ownership_t value_o)
{
  if (!ecma_is_value_string (value))
  {
    JJS_DISOWN (value, value_o);
    stream_p->write (stream_p, (const jjs_char_t *) "undefined", 9);
    return;
  }

  ecma_string_t *string_p = ecma_get_string_from_value (value);

  ECMA_STRING_TO_UTF8_STRING (string_p, string_bytes_p, string_bytes_len);

  if (ecma_string_get_length (string_p) == string_bytes_len || stream_p->encoding == JJS_ENCODING_CESU8)
  {
    stream_p->write (stream_p, string_bytes_p, string_bytes_len);
  }
  else if (stream_p->encoding == JJS_ENCODING_UTF8)
  {
    const lit_utf8_byte_t *cesu8_cursor_p = string_bytes_p;
    const lit_utf8_byte_t *cesu8_end_p = string_bytes_p + string_bytes_len;
    lit_utf8_byte_t utf8_buf_p[4];
    lit_code_point_t cp;
    lit_utf8_size_t read_size;
    lit_utf8_size_t encoded_size;

    while (cesu8_cursor_p < cesu8_end_p)
    {
      read_size = lit_read_code_point_from_cesu8 (cesu8_cursor_p, cesu8_end_p, &cp);
      encoded_size = (cp >= LIT_UTF16_FIRST_SURROGATE_CODE_POINT) ? 4 : read_size;

      if (cp >= LIT_UTF16_FIRST_SURROGATE_CODE_POINT)
      {
        stream_p->write (stream_p, utf8_buf_p, lit_code_point_to_utf8 (cp, utf8_buf_p));
      }
      else
      {
        stream_p->write (stream_p, cesu8_cursor_p, encoded_size);
      }

      cesu8_cursor_p += read_size;
    }

    JJS_ASSERT (cesu8_cursor_p <= cesu8_end_p);
  }
  else
  {
    JJS_ASSERT (stream_p->encoding == JJS_ENCODING_UTF8 || stream_p->encoding == JJS_ENCODING_CESU8);
    goto done;
  }

done:
  ECMA_FINALIZE_UTF8_STRING (string_bytes_p, string_bytes_len);

  JJS_DISOWN (value, value_o);
} /* fmt_write_string */

/**
 * Write a JS value to a fmt stream.
 */
static void
fmt_write_value (const jjs_fmt_stream_t *stream_p, jjs_value_t value, jjs_value_ownership_t value_o)
{
  if (jjs_value_is_exception (value))
  {
    fmt_write_string (stream_p, jjs_undefined (), JJS_MOVE);
    goto done;
  }

  if (jjs_value_is_symbol (value))
  {
    fmt_write_string (stream_p, jjs_symbol_descriptive_string (value), JJS_MOVE);
    goto done;
  }

  if (jjs_value_is_string (value))
  {
    fmt_write_string (stream_p, value, JJS_KEEP);
    goto done;
  }

  if (jjs_value_is_array (value))
  {
    static const uint8_t LBRACKET = '[';
    static const uint8_t RBRACKET = ']';

    stream_p->write (stream_p, &LBRACKET, 1);
    fmt_write_string (stream_p, jjs_value_to_string (value), JJS_MOVE);
    stream_p->write (stream_p, &RBRACKET, 1);
    goto done;
  }

  fmt_write_string (stream_p, jjs_value_to_string (value), JJS_MOVE);

  if (jjs_value_is_error (value))
  {
    /* TODO: print cause and AggregateError errors */
    ecma_value_t stack = ecma_make_magic_string_value (LIT_MAGIC_STRING_STACK);
    jjs_value_t backtrace_val = jjs_object_get (value, stack);

    ecma_fast_free_value (stack);

    if (jjs_value_is_array (backtrace_val))
    {
      uint32_t length = jjs_array_length (backtrace_val);

      for (uint32_t i = 0; i < length; i++)
      {
        jjs_value_t item_val = jjs_object_get_index (backtrace_val, i);

        if (jjs_value_is_string (item_val))
        {
          fmt_write_string (stream_p, item_val, JJS_KEEP);

          if (i != length - 1)
          {
            static const uint8_t NEWLINE = '\n';
            stream_p->write (stream_p, &NEWLINE, 1);
          }
        }

        jjs_value_free (item_val);
      }
    }

    jjs_value_free (backtrace_val);
  }

done:
  JJS_DISOWN (value, value_o);
} /* fmt_write_value */

/**
 * Formats a string using fmt substitution identifiers and writes the result to the
 * given stream. The usage is intended for logging, exception messages and general
 * debugging.
 *
 * {} is the only supported substitution identifier and the only substitution type is
 * any JS value. This function does not format C primitives or structs.
 *
 * The compiler does not validate {} and given value array size. If there are more
 * occurrences of {} than values, undefined will be substituted. If there are
 * fewer occurrences of {} than values, the extra values will be ignored.
 *
 * Each substitution values is to toString()'d with the following exceptions:
 * - Symbol: description is used
 * - Error: Error class + message is printed and stack trace, if available, is
 *          printed on subsequent lines
 * - Array: toString() already prints the contents of the array delimited by ,
 * - If an exception is thrown while attempting to toString, the substitution
 *   values will be undefined.
 *
 * stream_p write will receive characters in arbitrary batches.
 */
void
jjs_fmt_v (const jjs_fmt_stream_t *stream_p, /**< target stream object */
           const char *format_p, /**< format string */
           const jjs_value_t *values_p, /**< list of JS values for substitution */
           jjs_size_t values_length) /**< number of values */
{
  jjs_size_t values_index = 0;
  const char *cursor_p = format_p;
  bool found_left_brace = false;

  while (*cursor_p != '\0')
  {
    if (*cursor_p == '{')
    {
      if (found_left_brace)
      {
        stream_p->write (stream_p, (const uint8_t *) cursor_p, 1);
      }
      else
      {
        found_left_brace = true;
      }
    }
    else if (found_left_brace && *cursor_p == '}')
    {
      jjs_value_t value = values_index < values_length ? values_p[values_index++] : jjs_undefined ();

      if (jjs_value_is_exception (value))
      {
        fmt_write_value (stream_p, jjs_exception_value (value, false), JJS_MOVE);
      }
      else
      {
        fmt_write_value (stream_p, value, JJS_KEEP);
      }

      found_left_brace = false;
    }
    else
    {
      if (found_left_brace)
      {
        stream_p->write (stream_p, (const uint8_t *) (cursor_p - 1), 1);
        found_left_brace = false;
      }

      stream_p->write (stream_p, (const uint8_t *) cursor_p, 1);
    }

    cursor_p++;
  }
} /* jjs_fmt_v */

/**
 * Stream implementation that writes to an ecma string builder. Supports CESU8 encoding only.
 */
static void
fmt_stringbuilder_stream_write (const jjs_fmt_stream_t *self_p, const uint8_t *buffer_p, jjs_size_t size)
{
  ecma_stringbuilder_t *builder = self_p->state_p;

  /* user of this stream is using CESU8, so we can just copy to the builder */
  ecma_stringbuilder_append_raw (builder, buffer_p, size);
} /* fmt_stringbuilder_stream_write */

/**
 * Simple buffer object for in-memory buffer stream.
 */
typedef struct
{
  uint8_t *buffer;
  jjs_size_t buffer_index;
  jjs_size_t buffer_size;
} fmt_buffer_t;

/**
 * Stream implementation that writes to an in-memory buffer. Supports UTF8 and CESU8 encodings.
 */
static void
fmt_buffer_stream_write (const jjs_fmt_stream_t *self_p, const uint8_t *buffer_p, jjs_size_t size)
{
  fmt_buffer_t *target_p = self_p->state_p;

  if (target_p->buffer_index < target_p->buffer_size)
  {
    if (target_p->buffer_index + size < target_p->buffer_size)
    {
      memcpy (target_p->buffer + target_p->buffer_index, buffer_p, size);
      target_p->buffer_index += size;
    }
    else
    {
      jjs_size_t write_size = target_p->buffer_size - target_p->buffer_index;
      memcpy (target_p->buffer + target_p->buffer_index, buffer_p, write_size);
      target_p->buffer_index += write_size;
    }
  }
} /* fmt_buffer_stream_write */

/**
 * Formats to a JS string.
 *
 * @see jjs_fmt_v
 *
 * @return JS string. must be released with jjs_value_free
 */
jjs_value_t
jjs_fmt_to_string_v (const char *format_p, /**< format string */
                     const jjs_value_t *values_p, /**< substitution values */
                     jjs_size_t values_length) /**< number of substitution values */
{
  JJS_ASSERT (format_p != NULL);

  if (format_p == NULL || values_length == 0)
  {
    return jjs_string_utf8_sz (format_p);
  }

  ecma_stringbuilder_t builder = ecma_stringbuilder_create ();

  jjs_fmt_stream_t writer = {
    .write = fmt_stringbuilder_stream_write,
    .state_p = &builder,
    .encoding = JJS_ENCODING_CESU8,
  };

  jjs_fmt_v (&writer, format_p, values_p, values_length);

  return ecma_make_string_value (ecma_stringbuilder_finalize (&builder));
} /* jjs_fmt_to_string_v */

/**
 * Formats to a native character buffer.
 *
 * @see jjs_fmt_v
 *
 * @return the number of bytes written to buffer_p; if there is a problem 0 is returned.
 */
jjs_size_t
jjs_fmt_to_buffer_v (jjs_char_t *buffer_p, /**< target buffer */
                     jjs_size_t buffer_size, /**< target buffer size */
                     jjs_encoding_t encoding, /**< encoding used to write character to buffer */
                     const char *format_p, /**< format string */
                     const jjs_value_t *values_p, /**< substitution values */
                     jjs_size_t values_length) /**< number of substitution values */
{
  JJS_ASSERT (buffer_p != NULL);
  JJS_ASSERT (format_p != NULL);

  if (buffer_p == NULL || buffer_size == 0 || format_p == NULL || values_length == 0)
  {
    return 0;
  }

  fmt_buffer_t target = {
    .buffer = buffer_p,
    .buffer_size = buffer_size,
    .buffer_index = 0,
  };

  jjs_fmt_stream_t writer = {
    .write = fmt_buffer_stream_write,
    .state_p = &target,
    .encoding = encoding,
  };

  jjs_fmt_v (&writer, format_p, values_p, values_length);

  return target.buffer_index;
} /* jjs_fmt_to_buffer_v */

/**
 * Join JS values with a delimiter.
 *
 * This function is not equivalent to String.prototype.join. This function just
 * toString's the values and merges the result with the delimiter. No care is taken
 * for empty strings of undefined.
 *
 * @see jjs_fmt_v
 *
 * @return JS string (empty string if something goes wrong). value must be
 * released with jjs_value_free
 */
jjs_value_t
jjs_fmt_join_v (jjs_value_t delimiter, /**< string delimiter */
                jjs_value_ownership_t delimiter_o, /**< delimiter reference ownership */
                const jjs_value_t *values_p, /**< substitution values */
                jjs_size_t values_length) /**< number of substitution values */
{
  if (!jjs_value_is_string (delimiter) || values_length == 0)
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_stringbuilder_t builder = ecma_stringbuilder_create ();

  jjs_fmt_stream_t writer = {
    .write = fmt_stringbuilder_stream_write,
    .state_p = &builder,
    .encoding = JJS_ENCODING_CESU8,
  };

  for (jjs_size_t i = 0; i < values_length; i++)
  {
    fmt_write_value (&writer, values_p[i], JJS_KEEP);

    if (i < values_length - 1)
    {
      fmt_write_value (&writer, delimiter, JJS_KEEP);
    }
  }

  JJS_DISOWN (delimiter, delimiter_o);

  return ecma_make_string_value (ecma_stringbuilder_finalize (&builder));
} /* jjs_fmt_join_v */

/**
 * @}
 */
