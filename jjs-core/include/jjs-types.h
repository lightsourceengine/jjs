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

#ifndef JJS_TYPES_H
#define JJS_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "jjs-compiler.h"

JJS_C_API_BEGIN

/**
 * @defgroup jjs-api-types JJS public API types
 * @{
 */

/**
 * Error codes that can be passed by the engine when calling jjs_platform_fatal.
 */
typedef enum
{
  JJS_FATAL_OUT_OF_MEMORY = 10, /**< Out of memory */
  JJS_FATAL_REF_COUNT_LIMIT = 12, /**< Reference count limit reached */
  JJS_FATAL_DISABLED_BYTE_CODE = 13, /**< Executed disabled instruction */
  JJS_FATAL_UNTERMINATED_GC_LOOPS = 14, /**< Garbage collection loop limit reached */
  JJS_FATAL_FAILED_ASSERTION = 120 /**< Assertion failed */
} jjs_fatal_code_t;

/**
 * Encoding types.
 *
 * ECMA calls for UTF16 encoding for internal strings. For embedded systems, CESU8 is a common
 * replacement for UTF16. JJS has opted to use CESU8. Most JJS apis are encoding and decoding is
 * between UTF8 and CESU8. Not all apis support UTF16.
 */
typedef enum
{
  JJS_ENCODING_NONE, /**< indicates no encoding selected. api dependent on how this is interpreted. */
  JJS_ENCODING_CESU8, /**< cesu-8 encoding */
  JJS_ENCODING_UTF8, /**< utf-8 encoding */
  JJS_ENCODING_UTF16, /**< utf-16 encoding (endianness of platform) */
} jjs_encoding_t;

/**
 * Status code for platform api calls.
 */
typedef enum
{
  JJS_PLATFORM_STATUS_OK,
  JJS_PLATFORM_STATUS_ERR,
} jjs_platform_status_t;

/**
 * Buffer object used by platform api.
 *
 * The structure of the buffer object is generic. It is up to the platform api that is using it
 * to define the data type, how to free and whether to use encoding.
 */
typedef struct jjs_platform_buffer_t
{
  void* data_p; /**< pointer to the buffer data, type is chosen by the platform api using jjs_platform_buffer_t */
  uint32_t length; /**< size of the buffer data in bytes */
  jjs_encoding_t encoding; /**< encoding type of the data */
  void (*free) (struct jjs_platform_buffer_t*); /**< when done, this function should be called to release buffer memory */
} jjs_platform_buffer_t;

// Platform API Signatures
typedef jjs_platform_status_t (*jjs_platform_cwd_fn_t) (jjs_platform_buffer_t*);
typedef void (*jjs_platform_fatal_fn_t) (jjs_fatal_code_t);

typedef void (*jjs_platform_io_log_fn_t) (const char*);

typedef jjs_platform_status_t (*jjs_platform_fs_read_file_fn_t) (const uint8_t*, uint32_t, jjs_platform_buffer_t*);

typedef jjs_platform_status_t (*jjs_platform_time_sleep_fn_t) (uint32_t);
typedef jjs_platform_status_t (*jjs_platform_time_local_tza_fn_t) (double, int32_t*);
typedef jjs_platform_status_t (*jjs_platform_time_now_ms_fn_t) (double*);

typedef jjs_platform_status_t (*jjs_platform_path_realpath_fn_t) (const uint8_t*, uint32_t, jjs_platform_buffer_t*);

/**
 * Contains platform specific data and functions used internally by JJS. The
 * structure is exposed publicly so that the embedding application can inject
 * their own platform specific implementations.
 *
 * Depending on compile settings and the platform, all platform functionality
 * might not be available. API should be checked for NULL before using.
 */
typedef struct
{
  /**
   * Get the system os platform identifier.
   * 
   * JJS determines the platform identifier based on compiler flags with 'unknown'
   * as a fallback. Node's platform identifiers are used as a baseline. Here are
   * the possible values JJS can produce:
   * 
   * [ aix, darwin, freebsd, linux, openbsd, sunos, win32, unknown ]
   *
   * The identifier picker can be overridden at compile time by defining JJS_PLATFORM_OS.
   */
  char os_sz[16];

  /**
   * Get the system arch platform identifier.
   *
   * JJS determines the platform identifier based on compiler flags with 'unknown'
   * as a fallback. Node's platform identifiers are used as a baseline. Here are
   * the possible values JJS can produce:
   *
   * [ arm, arm64, ia32, loong64, mips, mipsel, ppc, ppc64, riscv64, s390, s390x, x64, unknown ]
   *
   * The identifier picker can be overridden at compile time by defining JJS_PLATFORM_ARCH.
   */
  char arch_sz[16];

  /**
   * Engine calls this platform function when the process experiences a fatal failure
   * which it cannot recover.
   *
   * A libc implementation will implement this with exit(), abort() or both.
   *
   * This platform function is required by the engine.
   */
  jjs_platform_fatal_fn_t fatal;

  /**
   * Get the current working directory of the process.
   *
   * The return values is a jjs_platform_status_t code. If OK, the pointer to a jjs_platform_buffer_t
   * is populated. The implementation will set the data pointer, length and provide a free function
   * that should be called to cleanup the buffer resources. If the implementation does not have a
   * need for a free operation, a dummy free function should be set. The returned buffer also
   * can have a UTF8 or UTF16 encoding. The caller must honor the encoding when converting the
   * buffer to string.
   *
   * This platform function is required for commonjs and esm modules to construct paths
   * from relative specifiers.
   */
  jjs_platform_cwd_fn_t path_cwd;

  /**
   * Display or log a debug/error message.
   *
   * The message is passed as a zero-terminated string. Messages may be logged in parts, which
   * will result in multiple calls to this functions. The implementation should consider
   * this before appending or prepending strings to the argument.
   *
   * This platform function is called with messages coming from the JJS engine as
   * the result of some abnormal operation or describing its internal operations
   * (e.g., data structure dumps or tracing info).
   *
   * The implementation can decide whether error and debug messages are logged to
   * the console, or saved to a database or to a file.
   */
  jjs_platform_io_log_fn_t io_log;

  /**
   * Get local time zone adjustment in milliseconds for the given input time.
   *
   * The argument is a time value representing milliseconds since unix epoch.
   *
   * Ideally, this function should satisfy the stipulations applied to LocalTZA
   * in section 21.4.1.7 of the ECMAScript version 12.0, as if called with isUTC true.
   *
   * This platform function is required by jjs-core when JJS_BUILTIN_DATE is enabled.
   */
  jjs_platform_time_local_tza_fn_t time_local_tza;

  /**
   * Get the current system time in UTC time or milliseconds since the unix epoch.
   *
   * This platform function is required by jjs-core when JJS_BUILTIN_DATE is enabled.
   */
  jjs_platform_time_now_ms_fn_t time_now_ms;

  /**
   * Put the current thread to sleep for the given number of milliseconds.
   *
   * This platform function is required by jjs-core when JJS_DEBUGGER is enabled.
   */
  jjs_platform_time_sleep_fn_t time_sleep;

  /**
   * Returns an absolute path with symlinks resolved to their real path names.
   *
   * The input path must be encoded in CESU8.
   *
   * The implementation should use realpath or GetFinalPathNameByHandle
   * (on windows). The primary use case for resolving module path names
   * so that they can be consistent cache keys, preventing modules from being
   * reloaded.
   */
  jjs_platform_path_realpath_fn_t path_realpath;

  /**
   * Reads the entire contents of a file into a (system) memory buffer. This
   * read is intended for JS source files, JSON files and snapshot. It is not
   * intended to be a general purpose read.
   *
   * The input path must be encoded in CESU8.
   *
   * The returned buffer will have a NONE encoding. On success, the caller must
   * call the buffer's free function to cleanup memory.
   *
   * The intent is to put the file contents in system memory, but the implementation
   * can decide to use vm memory or static memory. In that case, the implementation
   * should provide an appropriate buffer free function.
   */
  jjs_platform_fs_read_file_fn_t fs_read_file;
} jjs_platform_t;

/**
 * Status values returned by jjs_init functions.
 */
typedef enum
{
  JJS_CONTEXT_STATUS_OK = 0, /**< context successfully initialized */
  JJS_CONTEXT_STATUS_ALREADY_INITIALIZED, /**< context already in an initialized state */
  JJS_CONTEXT_STATUS_INVALID_EXTERNAL_HEAP, /**< external heap is invalid or not aligned */
  JJS_CONTEXT_STATUS_IMMUTABLE_HEAP_SIZE, /**< heap size was configured at compile time and cannot be changed  */
  JJS_CONTEXT_STATUS_IMMUTABLE_STACK_LIMIT, /**< stack limit was configured at compile time and cannot be changed  */
  JJS_CONTEXT_STATUS_CHAOS, /**< context is in a horrible state */
  JJS_CONTEXT_STATUS_REQUIRES_FATAL, /**< platform.fatal function is required by the engine */
  JJS_CONTEXT_STATUS_REQUIRES_TIME_SLEEP, /**< platform.time_sleep function is required by the engine */
  JJS_CONTEXT_STATUS_REQUIRES_TIME_LOCAL_TZA, /**< platform.time_local_tza function is required by the engine */
  JJS_CONTEXT_STATUS_REQUIRES_TIME_NOW_MS, /**< platform.time_now_ms function is required by the engine */
} jjs_context_status_t;

/**
 * JJS init flags.
 */
typedef enum
{
  JJS_CONTEXT_FLAG_NONE = (0u), /**< empty flag set */
  JJS_CONTEXT_FLAG_SHOW_OPCODES = (1u << 0), /**< dump byte-code to log after parse */
  JJS_CONTEXT_FLAG_SHOW_REGEXP_OPCODES = (1u << 1), /**< dump regexp byte-code to log after compilation */
  JJS_CONTEXT_FLAG_MEM_STATS = (1u << 2), /**< dump memory statistics */
  JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP = (1u << 3),
} jjs_context_flag_t;

/**
 * Callback for cleaning up an externally set context vm heap.
 */
typedef void (*jjs_context_heap_free_cb_t) (void* context_heap, void* user_p);

/**
 * Context initialization settings.
 *
 * Use jjs_context_options_init() to initialize this structure, as that function fills
 * in configured defaults.
 */
typedef struct
{
  /**
   * Context configuration flags. Bits enumerated in jjs_context_flag_t.
   */
  uint32_t context_flags;

  /**
   * Externally provided heap.
   *
   * It is necessary for the caller to ensure that the heap pointer's lifetime outlasts
   * the lifetime of JJS. The pointer lifetime should end with the callback to
   * external_heap_free_cb or after jjs_cleanup has been called.
   *
   * If set, add JJS_CONTEXT_FLAG_USING_EXTERNAL_HEAP to the context_flags.
   *
   * If the pointer is not aligned (address and vm_heap_size) or JJS_VM_HEAP_STATIC is set,
   * jjs_init will return an error status code.
   */
  uint8_t* external_heap;

  /**
   * Cleanup callback when the context is destroyed and the external heap can be freed.
   */
  jjs_context_heap_free_cb_t external_heap_free_cb;

  /**
   * Optional user defined token passed to external_heap_free_cb.
   */
  void* external_heap_free_user_p;

  /**
   * Maximum VM heap size.
   *
   * Use JJS_TO_KB() or JJS_TO_MB() macros to set this field. The field format is in short
   * hand kilobytes. The macros make setup code easier to read.
   *
   * If JJS_VM_HEAP_STATIC is set, jjs_init will return an error status if you attempt to
   * set this field.
   *
   * Default: JJS_DEFAULT_VM_HEAP_SIZE
   */
  uint32_t vm_heap_size;

  /**
   * VM stack size limit.
   *
   * Use JJS_TO_KB() or JJS_TO_MB() macros to set this field. The field format is in short
   * hand kilobytes. The macros make setup code easier to read.
   *
   * If JJS_VM_STACK_STATIC is set, jjs_init will return an error status if you attempt to
   * set this field.
   *
   * WARNING: This feature will not work across platforms, compilers and build configurations!
   * 
   * Default: JJS_DEFAULT_VM_STACK_LIMIT
   */
  uint32_t vm_stack_limit;

  /**
   * Allowed heap usage limit until next garbage collection.
   *
   * Use JJS_TO_KB() or JJS_TO_MB() macros to set this field. The field format is in short
   * hand kilobytes. The macros make setup code easier to read.
   *
   * If 0, the computed value is min (1/32 of heap size, 8K).
   *
   * Default: JJS_DEFAULT_GC_LIMIT
   */
  uint32_t gc_limit;

  /**
   * GC mark depth.
   *
   * if 0, mark depth is unlimited.
   *
   * Default: JJS_DEFAULT_GC_MARK_LIMIT
   */
  uint32_t gc_mark_limit;

  /**
   * Amount of newly allocated objects since the last GC run, represented as
   * a fraction of all allocated objects.
   *
   * If 0, 16 is used.
   *
   * Default: JJS_DEFAULT_GC_NEW_OBJECTS_FRACTION
   */
  uint32_t gc_new_objects_fraction;

  /**
   * Platform object.
   *
   * When jjs_context_options or jjs_context_options_init is called, this is filled in with all
   * the platform specific information and functions JJS will use by default. Depending on
   * compile time configuration some fields will be filled in and other not.
   *
   * As part of context configuration, you can modify anything in platform with your own data
   * of behavior. When the options are committed to the context through jjs_init, the modified
   * platform is used by JJS.
   */
  jjs_platform_t platform;
} jjs_context_options_t;

/**
 * Enum that is associated with a value passed to an api. It indicates how to handle
 * the lifetime of the JJS value.
 */
typedef enum
{
  /**
   * Caller owns the lifetime of the JJS value. If an api needs its own reference, it
   * must use jjs_value_copy.
   */
  JJS_KEEP,
  /**
   * Caller is transferring ownership of the JJS value to an api. The api is responsible
   * for free'ing the reference. The transfer is absolute. If the api has an error, it
   * is still responsible for the JS value reference.
   */
  JJS_MOVE,
} jjs_value_ownership_t;

/**
 * JJS log levels. The levels are in severity order
 * where the most serious levels come first.
 */
typedef enum
{
  JJS_LOG_LEVEL_ERROR = 0u, /**< the engine will terminate after the message is printed */
  JJS_LOG_LEVEL_WARNING = 1u, /**< a request is aborted, but the engine continues its operation */
  JJS_LOG_LEVEL_DEBUG = 2u, /**< debug messages from the engine, low volume */
  JJS_LOG_LEVEL_TRACE = 3u /**< detailed info about engine internals, potentially high volume */
} jjs_log_level_t;

/**
 * JJS API Error object types.
 */
typedef enum
{
  JJS_ERROR_NONE = 0, /**< No Error */

  JJS_ERROR_COMMON, /**< Error */
  JJS_ERROR_EVAL, /**< EvalError */
  JJS_ERROR_RANGE, /**< RangeError */
  JJS_ERROR_REFERENCE, /**< ReferenceError */
  JJS_ERROR_SYNTAX, /**< SyntaxError */
  JJS_ERROR_TYPE, /**< TypeError */
  JJS_ERROR_URI, /**< URIError */
  JJS_ERROR_AGGREGATE /**< AggregateError */
} jjs_error_t;

/**
 * JJS feature types.
 */
typedef enum
{
  JJS_FEATURE_CPOINTER_32_BIT, /**< 32 bit compressed pointers */
  JJS_FEATURE_ERROR_MESSAGES, /**< error messages */
  JJS_FEATURE_JS_PARSER, /**< js-parser */
  JJS_FEATURE_HEAP_STATS, /**< memory statistics */
  JJS_FEATURE_PARSER_DUMP, /**< parser byte-code dumps */
  JJS_FEATURE_REGEXP_DUMP, /**< regexp byte-code dumps */
  JJS_FEATURE_SNAPSHOT_SAVE, /**< saving snapshot files */
  JJS_FEATURE_SNAPSHOT_EXEC, /**< executing snapshot files */
  JJS_FEATURE_DEBUGGER, /**< debugging */
  JJS_FEATURE_VM_EXEC_STOP, /**< stopping ECMAScript execution */
  JJS_FEATURE_VM_THROW, /**< capturing ECMAScript throws */
  JJS_FEATURE_JSON, /**< JSON support */
  JJS_FEATURE_PROMISE, /**< promise support */
  JJS_FEATURE_TYPEDARRAY, /**< Typedarray support */
  JJS_FEATURE_DATE, /**< Date support */
  JJS_FEATURE_REGEXP, /**< Regexp support */
  JJS_FEATURE_LINE_INFO, /**< line info available */
  JJS_FEATURE_LOGGING, /**< logging */
  JJS_FEATURE_SYMBOL, /**< symbol support */
  JJS_FEATURE_DATAVIEW, /**< DataView support */
  JJS_FEATURE_PROXY, /**< Proxy support */
  JJS_FEATURE_MAP, /**< Map support */
  JJS_FEATURE_SET, /**< Set support */
  JJS_FEATURE_WEAKMAP, /**< WeakMap support */
  JJS_FEATURE_WEAKSET, /**< WeakSet support */
  JJS_FEATURE_BIGINT, /**< BigInt support */
  JJS_FEATURE_REALM, /**< realm support */
  JJS_FEATURE_GLOBAL_THIS, /**< GlobalThisValue support */
  JJS_FEATURE_PROMISE_CALLBACK, /**< Promise callback support */
  JJS_FEATURE_MODULE, /**< Module API support */
  JJS_FEATURE_WEAKREF, /**< WeakRef support */
  JJS_FEATURE_FUNCTION_TO_STRING, /**< function toString support */
  JJS_FEATURE_QUEUE_MICROTASK, /**< queueMicrotask support */
  JJS_FEATURE_ESM, /**< ES Module support (import from file) */
  JJS_FEATURE_COMMONJS, /**< CommonJS module support (import from file) */
  JJS_FEATURE_PMAP, /**< Package Map support */
  JJS_FEATURE_VMOD, /**< Virtual Module support */
  JJS_FEATURE_VM_HEAP_STATIC, /**< VM stack heap size has been set at compile time.  */
  JJS_FEATURE_VM_STACK_STATIC, /**< VM stack limit size has been set at compile time. */
  JJS_FEATURE__COUNT /**< number of features. NOTE: must be at the end of the list */
} jjs_feature_t;

/**
 * GC operational modes.
 */
typedef enum
{
  JJS_GC_PRESSURE_LOW, /**< free unused objects, but keep memory
                          *   allocated for performance improvements
                          *   such as property hash tables for large objects */
  JJS_GC_PRESSURE_HIGH /**< free as much memory as possible */
} jjs_gc_mode_t;

/**
 * JJS regexp flags.
 */
typedef enum
{
  JJS_REGEXP_FLAG_GLOBAL = (1u << 1), /**< Globally scan string */
  JJS_REGEXP_FLAG_IGNORE_CASE = (1u << 2), /**< Ignore case */
  JJS_REGEXP_FLAG_MULTILINE = (1u << 3), /**< Multiline string scan */
  JJS_REGEXP_FLAG_STICKY = (1u << 4), /**< ECMAScript v11, 21.2.5.14 */
  JJS_REGEXP_FLAG_UNICODE = (1u << 5), /**< ECMAScript v11, 21.2.5.17 */
  JJS_REGEXP_FLAG_DOTALL = (1u << 6) /**< ECMAScript v11, 21.2.5.3 */
} jjs_regexp_flags_t;

/**
 * Character type of JJS.
 */
typedef uint8_t jjs_char_t;

/**
 * Size type of JJS.
 */
typedef uint32_t jjs_size_t;

/**
 * Length type of JJS.
 */
typedef uint32_t jjs_length_t;

/**
 * Description of a JJS value.
 */
typedef uint32_t jjs_value_t;

/**
 * Option bits for jjs_parse_options_t.
 */
typedef enum
{
  JJS_PARSE_NO_OPTS = 0, /**< no options passed */
  JJS_PARSE_STRICT_MODE = (1 << 0), /**< enable strict mode */
  JJS_PARSE_MODULE = (1 << 1), /**< parse source as an ECMAScript module */
  JJS_PARSE_HAS_ARGUMENT_LIST = (1 << 2), /**< argument_list field is valid,
                                             * this also means that function parsing will be done */
  JJS_PARSE_HAS_SOURCE_NAME = (1 << 3), /**< source_name field is valid */
  JJS_PARSE_HAS_START = (1 << 4), /**< start_line and start_column fields are valid */
  JJS_PARSE_HAS_USER_VALUE = (1 << 5), /**< user_value field is valid */
} jjs_parse_option_enable_feature_t;

/**
 * Various configuration options for parsing functions such as jjs_parse or jjs_parse_function.
 */
typedef struct
{
  uint32_t options; /**< combination of jjs_parse_option_enable_feature_t values */
  jjs_value_t argument_list; /**< function argument list if JJS_PARSE_HAS_ARGUMENT_LIST is set in options
                                *   Note: must be string value */
  jjs_value_t source_name; /**< source name string (usually a file name)
                              *   if JJS_PARSE_HAS_SOURCE_NAME is set in options
                              *   Note: must be string value */
  uint32_t start_line; /**< start line of the source code if JJS_PARSE_HAS_START is set in options */
  uint32_t start_column; /**< start column of the source code if JJS_PARSE_HAS_START is set in options */
  jjs_value_t user_value; /**< user value assigned to all functions created by this script including eval
                             *   calls executed by the script if JJS_PARSE_HAS_USER_VALUE is set in options */
} jjs_parse_options_t;

/**
 * Source code and configuration data of an in-memory ES module.
 *
 * This struct should not be accessed directly. See jjs_esm_source_init* and jjs_esm_source_set* API
 * functions for usage information.
 */
typedef struct
{
  jjs_value_t source_value; /**< module source code value. valid if !undefined. default is undefined. */
  const jjs_char_t* source_buffer_p; /**< module source code. valid if source_value is unset. default is NULL. */
  jjs_size_t source_buffer_size; /**< size, in bytes, of source_buffer_p. default is 0. */

  jjs_value_t filename; /**< simple filename of module. default is undefined, resolves to <anonymous>.mjs */
  jjs_value_t dirname; /**< fs dirname of the module. default is undefined, resolves to cwd. */
  jjs_value_t meta_extension; /**< value of import.meta.extension. default in undefined. */
  uint32_t start_line; /**< start line of the source code. default is 0. */
  uint32_t start_column; /**< start column of the source code. default is 0. */
  bool cache; /**< if true, the module will be put in the esm cache using resolved dirname + filename as the key. default is false.*/
} jjs_esm_source_t;

/**
 * Description of ECMA property descriptor.
 */
typedef enum
{
  JJS_PROP_NO_OPTS = (0), /**< empty property descriptor */
  JJS_PROP_IS_CONFIGURABLE = (1 << 0), /**< [[Configurable]] */
  JJS_PROP_IS_ENUMERABLE = (1 << 1), /**< [[Enumerable]] */
  JJS_PROP_IS_WRITABLE = (1 << 2), /**< [[Writable]] */

  JJS_PROP_IS_CONFIGURABLE_DEFINED = (1 << 3), /**< is [[Configurable]] defined? */
  JJS_PROP_IS_ENUMERABLE_DEFINED = (1 << 4), /**< is [[Enumerable]] defined? */
  JJS_PROP_IS_WRITABLE_DEFINED = (1 << 5), /**< is [[Writable]] defined? */

  JJS_PROP_IS_VALUE_DEFINED = (1 << 6), /**< is [[Value]] defined? */
  JJS_PROP_IS_GET_DEFINED = (1 << 7), /**< is [[Get]] defined? */
  JJS_PROP_IS_SET_DEFINED = (1 << 8), /**< is [[Set]] defined? */

  JJS_PROP_SHOULD_THROW = (1 << 9), /**< should throw on error, instead of returning with false */
} jjs_property_descriptor_flags_t;

/**
 * Description of ECMA property descriptor.
 */
typedef struct
{
  uint16_t flags; /**< any combination of jjs_property_descriptor_flags_t bits */
  jjs_value_t value; /**< [[Value]] */
  jjs_value_t getter; /**< [[Get]] */
  jjs_value_t setter; /**< [[Set]] */
} jjs_property_descriptor_t;

/**
 * JJS object property filter options.
 */
typedef enum
{
  JJS_PROPERTY_FILTER_ALL = 0, /**< List all property keys independently
                                  *   from key type or property value attributes
                                  *   (equivalent to Reflect.ownKeys call)  */
  JJS_PROPERTY_FILTER_TRAVERSE_PROTOTYPE_CHAIN = (1 << 0), /**< Include keys from the objects's
                                                              *   prototype chain as well */
  JJS_PROPERTY_FILTER_EXCLUDE_NON_CONFIGURABLE = (1 << 1), /**< Exclude property key if
                                                              *   the property is non-configurable */
  JJS_PROPERTY_FILTER_EXCLUDE_NON_ENUMERABLE = (1 << 2), /**< Exclude property key if
                                                            *   the property is non-enumerable */
  JJS_PROPERTY_FILTER_EXCLUDE_NON_WRITABLE = (1 << 3), /**< Exclude property key if
                                                          *   the property is non-writable */
  JJS_PROPERTY_FILTER_EXCLUDE_STRINGS = (1 << 4), /**< Exclude property key if it is a string */
  JJS_PROPERTY_FILTER_EXCLUDE_SYMBOLS = (1 << 5), /**< Exclude property key if it is a symbol */
  JJS_PROPERTY_FILTER_EXCLUDE_INTEGER_INDICES = (1 << 6), /**< Exclude property key if it is an integer index */
  JJS_PROPERTY_FILTER_INTEGER_INDICES_AS_NUMBER = (1 << 7), /**< By default integer index property keys are
                                                               *   converted to string. Enabling this flags keeps
                                                               *   integer index property keys as numbers. */
} jjs_property_filter_t;

/**
 * Description of JJS heap memory stats.
 * It is for memory profiling.
 */
typedef struct
{
  size_t version; /**< the version of the stats struct */
  size_t size; /**< heap total size */
  size_t allocated_bytes; /**< currently allocated bytes */
  size_t peak_allocated_bytes; /**< peak allocated bytes */
  size_t reserved[4]; /**< padding for future extensions */
} jjs_heap_stats_t;

/**
 * Call related information passed to jjs_external_handler_t.
 */
typedef struct jjs_call_info_t
{
  jjs_value_t function; /**< invoked function object */
  jjs_value_t this_value; /**< this value passed to the function  */
  jjs_value_t new_target; /**< current new target value, undefined for non-constructor calls */
} jjs_call_info_t;

/**
 * Type of an external function handler.
 */
typedef jjs_value_t (*jjs_external_handler_t) (const jjs_call_info_t *call_info_p,
                                                   const jjs_value_t args_p[],
                                                   const jjs_length_t args_count);

/**
 * Native free callback of generic value types.
 */
typedef void (*jjs_value_free_cb_t) (void *native_p);

/**
 * Forward definition of jjs_object_native_info_t.
 */
struct jjs_object_native_info_t;

/**
 * Native free callback of an object.
 */
typedef void (*jjs_object_native_free_cb_t) (void *native_p, const struct jjs_object_native_info_t *info_p);

/**
 * Free callback for external strings.
 */
typedef void (*jjs_external_string_free_cb_t) (jjs_char_t *string_p, jjs_size_t string_size, void *user_p);

/**
 * Decorator callback for Error objects. The decorator can create
 * or update any properties of the newly created Error object.
 */
typedef void (*jjs_error_object_created_cb_t) (const jjs_value_t error_object, void *user_p);

/**
 * Callback which tells whether the ECMAScript execution should be stopped.
 *
 * As long as the function returns with undefined the execution continues.
 * When a non-undefined value is returned the execution stops and the value
 * is thrown by the engine as an exception.
 *
 * Note: if the function returns with a non-undefined value it
 *       must return with the same value for future calls.
 */
typedef jjs_value_t (*jjs_halt_cb_t) (void *user_p);

/**
 * Callback function which is called when an exception is thrown in an ECMAScript code.
 * The callback should not change the exception_value. The callback is not called again
 * until the value is caught.
 *
 * Note: the engine considers exceptions thrown by external functions as never caught.
 */
typedef void (*jjs_throw_cb_t) (const jjs_value_t exception_value, void *user_p);

/**
 * Function type applied to each unit of encoding when iterating over a string.
 */
typedef void (*jjs_string_iterate_cb_t) (uint32_t value, void *user_p);

/**
 * Function type applied for each data property of an object.
 */
typedef bool (*jjs_object_property_foreach_cb_t) (const jjs_value_t property_name,
                                                    const jjs_value_t property_value,
                                                    void *user_data_p);

/**
 * Function type applied for each object in the engine.
 */
typedef bool (*jjs_foreach_live_object_cb_t) (const jjs_value_t object, void *user_data_p);

/**
 * Function type applied for each matching object in the engine.
 */
typedef bool (*jjs_foreach_live_object_with_info_cb_t) (const jjs_value_t object,
                                                          void *object_data_p,
                                                          void *user_data_p);

/**
 * User context item manager
 */
typedef struct
{
  /**
   * Callback responsible for initializing a context item, or NULL to zero out the memory. This is called lazily, the
   * first time jjs_context_data () is called with this manager.
   *
   * @param [in] data The buffer that JJS allocated for the manager. The buffer is zeroed out. The size is
   * determined by the bytes_needed field. The buffer is kept alive until jjs_cleanup () is called.
   */
  void (*init_cb) (void *data);

  /**
   * Callback responsible for deinitializing a context item, or NULL. This is called as part of jjs_cleanup (),
   * right *before* the VM has been cleaned up. This is a good place to release strong references to jjs_value_t's
   * that the manager may be holding.
   * Note: because the VM has not been fully cleaned up yet, jjs_object_native_info_t free_cb's can still get called
   * *after* all deinit_cb's have been run. See finalize_cb for a callback that is guaranteed to run *after* all
   * free_cb's have been run.
   *
   * @param [in] data The buffer that JJS allocated for the manager.
   */
  void (*deinit_cb) (void *data);

  /**
   * Callback responsible for finalizing a context item, or NULL. This is called as part of jjs_cleanup (),
   * right *after* the VM has been cleaned up and destroyed and jjs_... APIs cannot be called any more. At this point,
   * all values in the VM have been cleaned up. This is a good place to clean up native state that can only be cleaned
   * up at the very end when there are no more VM values around that may need to access that state.
   *
   * @param [in] data The buffer that JJS allocated for the manager. After returning from this callback,
   * the data pointer may no longer be used.
   */
  void (*finalize_cb) (void *data);

  /**
   * Number of bytes to allocate for this manager. This is the size of the buffer that JJS will allocate on
   * behalf of the manager. The pointer to this buffer is passed into init_cb, deinit_cb and finalize_cb. It is also
   * returned from the jjs_context_data () API.
   */
  size_t bytes_needed;
} jjs_context_data_manager_t;

/**
 * Function type for allocating buffer for JJS context.
 */
typedef void *(*jjs_context_alloc_cb_t) (size_t size, void *cb_data_p);

/**
 * Type information of a native pointer.
 */
typedef struct jjs_object_native_info_t
{
  jjs_object_native_free_cb_t free_cb; /**< the free callback of the native pointer */
  uint16_t number_of_references; /**< the number of value references which are marked by the garbage collector */
  uint16_t offset_of_references; /**< byte offset indicating the start offset of value
                                  *   references in the user allocated buffer */
} jjs_object_native_info_t;

/**
 * An opaque declaration of the JJS context structure.
 */
typedef struct jjs_context_t jjs_context_t;

/**
 * Enum that contains the supported binary operation types
 */
typedef enum
{
  JJS_BIN_OP_EQUAL = 0u, /**< equal comparison (==) */
  JJS_BIN_OP_STRICT_EQUAL, /**< strict equal comparison (===) */
  JJS_BIN_OP_LESS, /**< less relation (<) */
  JJS_BIN_OP_LESS_EQUAL, /**< less or equal relation (<=) */
  JJS_BIN_OP_GREATER, /**< greater relation (>) */
  JJS_BIN_OP_GREATER_EQUAL, /**< greater or equal relation (>=)*/
  JJS_BIN_OP_INSTANCEOF, /**< instanceof operation */
  JJS_BIN_OP_ADD, /**< addition operator (+) */
  JJS_BIN_OP_SUB, /**< subtraction operator (-) */
  JJS_BIN_OP_MUL, /**< multiplication operator (*) */
  JJS_BIN_OP_DIV, /**< division operator (/) */
  JJS_BIN_OP_REM, /**< remainder operator (%) */
} jjs_binary_op_t;

/**
 * Backtrace related types.
 */

/**
 * List of backtrace frame types returned by jjs_frame_type.
 */
typedef enum
{
  JJS_BACKTRACE_FRAME_JS, /**< indicates that the frame is created for a JavaScript function/method */
} jjs_frame_type_t;

/**
 * Location info retrieved by jjs_frame_location.
 */
typedef struct
{
  jjs_value_t source_name; /**< source name */
  jjs_size_t line; /**< line index */
  jjs_size_t column; /**< column index */
} jjs_frame_location_t;

/*
 * Internal data structure for jjs_frame_t definition.
 */
struct jjs_frame_internal_t;

/**
 * Backtrace frame data passed to the jjs_backtrace_cb_t handler.
 */
typedef struct jjs_frame_internal_t jjs_frame_t;

/**
 * Callback function which is called by jjs_backtrace for each stack frame.
 */
typedef bool (*jjs_backtrace_cb_t) (jjs_frame_t *frame_p, void *user_p);

/**
 * Detailed value type related types.
 */

/**
 * JJS API value type information.
 */
typedef enum
{
  JJS_TYPE_NONE = 0u, /**< no type information */
  JJS_TYPE_UNDEFINED, /**< undefined type */
  JJS_TYPE_NULL, /**< null type */
  JJS_TYPE_BOOLEAN, /**< boolean type */
  JJS_TYPE_NUMBER, /**< number type */
  JJS_TYPE_STRING, /**< string type */
  JJS_TYPE_OBJECT, /**< object type */
  JJS_TYPE_FUNCTION, /**< function type */
  JJS_TYPE_EXCEPTION, /**< exception/abort type */
  JJS_TYPE_SYMBOL, /**< symbol type */
  JJS_TYPE_BIGINT, /**< bigint type */
} jjs_type_t;

/**
 * JJS object type information.
 */
typedef enum
{
  JJS_OBJECT_TYPE_NONE = 0u, /**< Non object type */
  JJS_OBJECT_TYPE_GENERIC, /**< Generic JavaScript object without any internal property */
  JJS_OBJECT_TYPE_MODULE_NAMESPACE, /**< Namespace object */
  JJS_OBJECT_TYPE_ARRAY, /**< Array object */
  JJS_OBJECT_TYPE_PROXY, /**< Proxy object */
  JJS_OBJECT_TYPE_SCRIPT, /**< Script object (see jjs_parse) */
  JJS_OBJECT_TYPE_MODULE, /**< Module object (see jjs_parse) */
  JJS_OBJECT_TYPE_PROMISE, /**< Promise object */
  JJS_OBJECT_TYPE_DATAVIEW, /**< Dataview object */
  JJS_OBJECT_TYPE_FUNCTION, /**< Function object (see jjs_function_type) */
  JJS_OBJECT_TYPE_TYPEDARRAY, /**< %TypedArray% object (see jjs_typedarray_type) */
  JJS_OBJECT_TYPE_ITERATOR, /**< Iterator object (see jjs_iterator_type) */
  JJS_OBJECT_TYPE_CONTAINER, /**< Container object (see jjs_container_get_type) */
  JJS_OBJECT_TYPE_ERROR, /**< Error object */
  JJS_OBJECT_TYPE_ARRAYBUFFER, /**< Array buffer object */
  JJS_OBJECT_TYPE_SHARED_ARRAY_BUFFER, /**< Shared Array Buffer object */

  JJS_OBJECT_TYPE_ARGUMENTS, /**< Arguments object */
  JJS_OBJECT_TYPE_BOOLEAN, /**< Boolean object */
  JJS_OBJECT_TYPE_DATE, /**< Date object */
  JJS_OBJECT_TYPE_NUMBER, /**< Number object */
  JJS_OBJECT_TYPE_REGEXP, /**< RegExp object */
  JJS_OBJECT_TYPE_STRING, /**< String object */
  JJS_OBJECT_TYPE_SYMBOL, /**< Symbol object */
  JJS_OBJECT_TYPE_GENERATOR, /**< Generator object */
  JJS_OBJECT_TYPE_BIGINT, /**< BigInt object */
  JJS_OBJECT_TYPE_WEAKREF, /**< WeakRef object */
} jjs_object_type_t;

/**
 * JJS function object type information.
 */
typedef enum
{
  JJS_FUNCTION_TYPE_NONE = 0u, /**< Non function type */
  JJS_FUNCTION_TYPE_GENERIC, /**< Generic JavaScript function */
  JJS_FUNCTION_TYPE_ACCESSOR, /**< Accessor function */
  JJS_FUNCTION_TYPE_BOUND, /**< Bound function */
  JJS_FUNCTION_TYPE_ARROW, /**< Arrow function */
  JJS_FUNCTION_TYPE_GENERATOR, /**< Generator function */
} jjs_function_type_t;

/**
 * JJS iterator object type information.
 */
typedef enum
{
  JJS_ITERATOR_TYPE_NONE = 0u, /**< Non iterator type */
  JJS_ITERATOR_TYPE_ARRAY, /**< Array iterator */
  JJS_ITERATOR_TYPE_STRING, /**< String iterator */
  JJS_ITERATOR_TYPE_MAP, /**< Map iterator */
  JJS_ITERATOR_TYPE_SET, /**< Set iterator */
} jjs_iterator_type_t;

/**
 * Module related types.
 */

/**
 * An enum representing the current status of a module
 */
typedef enum
{
  JJS_MODULE_STATE_INVALID = 0, /**< return value for jjs_module_state when its argument is not a module */
  JJS_MODULE_STATE_UNLINKED = 1, /**< module is currently unlinked */
  JJS_MODULE_STATE_LINKING = 2, /**< module is currently being linked */
  JJS_MODULE_STATE_LINKED = 3, /**< module has been linked (its dependencies has been resolved) */
  JJS_MODULE_STATE_EVALUATING = 4, /**< module is currently being evaluated */
  JJS_MODULE_STATE_EVALUATED = 5, /**< module has been evaluated (its source code has been executed) */
  JJS_MODULE_STATE_ERROR = 6, /**< an error has been encountered before the evaluated state is reached */
} jjs_module_state_t;

/**
 * Callback which is called by jjs_module_link to get the referenced module.
 */
typedef jjs_value_t (*jjs_module_link_cb_t) (const jjs_value_t specifier, const jjs_value_t referrer, void *user_p);

/**
 * Callback which is called when an import is resolved dynamically to get the referenced module.
 */
typedef jjs_value_t (*jjs_module_import_cb_t) (const jjs_value_t specifier, const jjs_value_t user_value, void *user_p);

/**
 * Callback which is called after the module enters into linked, evaluated or error state.
 */
typedef void (*jjs_module_state_changed_cb_t) (jjs_module_state_t new_state,
                                                 const jjs_value_t module,
                                                 const jjs_value_t value,
                                                 void *user_p);

/**
 * Callback which is called when an import.meta expression of a module is evaluated the first time.
 */
typedef void (*jjs_module_import_meta_cb_t) (const jjs_value_t module,
                                               const jjs_value_t meta_object,
                                               void *user_p);

/**
 * Callback which is called by jjs_module_evaluate to evaluate the synthetic module.
 */
typedef jjs_value_t (*jjs_synthetic_module_evaluate_cb_t) (const jjs_value_t module);

typedef enum
{
  JJS_MODULE_TYPE_NONE,
  JJS_MODULE_TYPE_COMMONJS,
  JJS_MODULE_TYPE_MODULE,
} jjs_module_type_t;

/**
 *
 */
typedef struct
{
  jjs_module_type_t type;
  jjs_value_t referrer_path;
} jjs_esm_resolve_context_t;

/**
 *
 */
typedef struct
{
  jjs_module_type_t type;
  jjs_value_t format;
} jjs_esm_load_context_t;

/**
 *
 */
typedef jjs_value_t (*jjs_esm_resolve_cb_t) (jjs_value_t specifier, jjs_esm_resolve_context_t *context_p, void *user_p);

/**
 *
 */
typedef jjs_value_t (*jjs_esm_load_cb_t) (jjs_value_t path, jjs_esm_load_context_t *context_p, void *user_p);

/**
 * Proxy related types.
 */

/**
 * JJS special Proxy object options.
 */
typedef enum
{
  JJS_PROXY_SKIP_RESULT_VALIDATION = (1u << 0), /**< skip result validation for [[GetPrototypeOf]],
                                                   *   [[SetPrototypeOf]], [[IsExtensible]],
                                                   *   [[PreventExtensions]], [[GetOwnProperty]],
                                                   *   [[DefineOwnProperty]], [[HasProperty]], [[Get]],
                                                   *   [[Set]], [[Delete]] and [[OwnPropertyKeys]] */
} jjs_proxy_custom_behavior_t;

/**
 * Promise related types.
 */

/**
 * Enum values representing various Promise states.
 */
typedef enum
{
  JJS_PROMISE_STATE_NONE = 0u, /**< Invalid/Unknown state (possibly called on a non-promise object). */
  JJS_PROMISE_STATE_PENDING, /**< Promise is in "Pending" state. */
  JJS_PROMISE_STATE_FULFILLED, /**< Promise is in "Fulfilled" state. */
  JJS_PROMISE_STATE_REJECTED, /**< Promise is in "Rejected" state. */
} jjs_promise_state_t;

/**
 * Event types for jjs_promise_event_cb_t callback function.
 * The description of the 'object' and 'value' arguments are provided for each type.
 */
typedef enum
{
  JJS_PROMISE_EVENT_CREATE = 0u, /**< a new Promise object is created
                                    *   object: the new Promise object
                                    *   value: parent Promise for `then` chains, undefined otherwise */
  JJS_PROMISE_EVENT_RESOLVE, /**< called when a Promise is about to be resolved
                                *   object: the Promise object
                                *   value: value for resolving */
  JJS_PROMISE_EVENT_REJECT, /**< called when a Promise is about to be rejected
                               *   object: the Promise object
                               *   value: value for rejecting */
  JJS_PROMISE_EVENT_RESOLVE_FULFILLED, /**< called when a resolve is called on a fulfilled Promise
                                          *   object: the Promise object
                                          *   value: value for resolving */
  JJS_PROMISE_EVENT_REJECT_FULFILLED, /**< called when a reject is called on a fulfilled Promise
                                         *  object: the Promise object
                                         *  value: value for rejecting */
  JJS_PROMISE_EVENT_REJECT_WITHOUT_HANDLER, /**< called when a Promise is rejected without a handler
                                               *   object: the Promise object
                                               *   value: value for rejecting */
  JJS_PROMISE_EVENT_CATCH_HANDLER_ADDED, /**< called when a catch handler is added to a rejected
                                            *   Promise which did not have a catch handler before
                                            *   object: the Promise object
                                            *   value: undefined */
  JJS_PROMISE_EVENT_BEFORE_REACTION_JOB, /**< called before executing a Promise reaction job
                                            *   object: the Promise object
                                            *   value: undefined */
  JJS_PROMISE_EVENT_AFTER_REACTION_JOB, /**< called after a Promise reaction job is completed
                                           *   object: the Promise object
                                           *   value: undefined */
  JJS_PROMISE_EVENT_ASYNC_AWAIT, /**< called when an async function awaits the result of a Promise object
                                    *   object: internal object representing the execution status
                                    *   value: the Promise object */
  JJS_PROMISE_EVENT_ASYNC_BEFORE_RESOLVE, /**< called when an async function is continued with resolve
                                             *   object: internal object representing the execution status
                                             *   value: value for resolving */
  JJS_PROMISE_EVENT_ASYNC_BEFORE_REJECT, /**< called when an async function is continued with reject
                                            *   object: internal object representing the execution status
                                            *   value: value for rejecting */
  JJS_PROMISE_EVENT_ASYNC_AFTER_RESOLVE, /**< called when an async function resolve is completed
                                            *   object: internal object representing the execution status
                                            *   value: value for resolving */
  JJS_PROMISE_EVENT_ASYNC_AFTER_REJECT, /**< called when an async function reject is completed
                                           *   object: internal object representing the execution status
                                           *   value: value for rejecting */
} jjs_promise_event_type_t;

/**
 * Filter types for jjs_promise_on_event callback function.
 * The callback is only called for those events which are enabled by the filters.
 */
typedef enum
{
  JJS_PROMISE_EVENT_FILTER_DISABLE = 0, /**< disable reporting of all events */
  JJS_PROMISE_EVENT_FILTER_CREATE = (1 << 0), /**< enables the following event:
                                                 *   JJS_PROMISE_EVENT_CREATE */
  JJS_PROMISE_EVENT_FILTER_RESOLVE = (1 << 1), /**< enables the following event:
                                                  *   JJS_PROMISE_EVENT_RESOLVE */
  JJS_PROMISE_EVENT_FILTER_REJECT = (1 << 2), /**< enables the following event:
                                                 *   JJS_PROMISE_EVENT_REJECT */
  JJS_PROMISE_EVENT_FILTER_ERROR = (1 << 3), /**< enables the following events:
                                                *   JJS_PROMISE_EVENT_RESOLVE_FULFILLED
                                                *   JJS_PROMISE_EVENT_REJECT_FULFILLED
                                                *   JJS_PROMISE_EVENT_REJECT_WITHOUT_HANDLER
                                                *   JJS_PROMISE_EVENT_CATCH_HANDLER_ADDED */
  JJS_PROMISE_EVENT_FILTER_REACTION_JOB = (1 << 4), /**< enables the following events:
                                                       *   JJS_PROMISE_EVENT_BEFORE_REACTION_JOB
                                                       *   JJS_PROMISE_EVENT_AFTER_REACTION_JOB */
  JJS_PROMISE_EVENT_FILTER_ASYNC_MAIN = (1 << 5), /**< enables the following event:
                                                     *   JJS_PROMISE_EVENT_ASYNC_AWAIT */
  JJS_PROMISE_EVENT_FILTER_ASYNC_REACTION_JOB = (1 << 6), /**< enables the following events:
                                                             *   JJS_PROMISE_EVENT_ASYNC_BEFORE_RESOLVE
                                                             *   JJS_PROMISE_EVENT_ASYNC_BEFORE_REJECT
                                                             *   JJS_PROMISE_EVENT_ASYNC_AFTER_RESOLVE
                                                             *   JJS_PROMISE_EVENT_ASYNC_AFTER_REJECT */
} jjs_promise_event_filter_t;

/**
 * Notification callback for tracking Promise and async function operations.
 */
typedef void (*jjs_promise_event_cb_t) (jjs_promise_event_type_t event_type,
                                          const jjs_value_t object,
                                          const jjs_value_t value,
                                          void *user_p);

/**
 * Symbol related types.
 */

/**
 * List of well-known symbols.
 */
typedef enum
{
  JJS_SYMBOL_ASYNC_ITERATOR, /**< @@asyncIterator well-known symbol */
  JJS_SYMBOL_HAS_INSTANCE, /**< @@hasInstance well-known symbol */
  JJS_SYMBOL_IS_CONCAT_SPREADABLE, /**< @@isConcatSpreadable well-known symbol */
  JJS_SYMBOL_ITERATOR, /**< @@iterator well-known symbol */
  JJS_SYMBOL_MATCH, /**< @@match well-known symbol */
  JJS_SYMBOL_REPLACE, /**< @@replace well-known symbol */
  JJS_SYMBOL_SEARCH, /**< @@search well-known symbol */
  JJS_SYMBOL_SPECIES, /**< @@species well-known symbol */
  JJS_SYMBOL_SPLIT, /**< @@split well-known symbol */
  JJS_SYMBOL_TO_PRIMITIVE, /**< @@toPrimitive well-known symbol */
  JJS_SYMBOL_TO_STRING_TAG, /**< @@toStringTag well-known symbol */
  JJS_SYMBOL_UNSCOPABLES, /**< @@unscopables well-known symbol */
  JJS_SYMBOL_MATCH_ALL, /**< @@matchAll well-known symbol */
} jjs_well_known_symbol_t;

/**
 * TypedArray related types.
 */

/**
 * TypedArray types.
 */
typedef enum
{
  JJS_TYPEDARRAY_INVALID = 0,
  JJS_TYPEDARRAY_UINT8,
  JJS_TYPEDARRAY_UINT8CLAMPED,
  JJS_TYPEDARRAY_INT8,
  JJS_TYPEDARRAY_UINT16,
  JJS_TYPEDARRAY_INT16,
  JJS_TYPEDARRAY_UINT32,
  JJS_TYPEDARRAY_INT32,
  JJS_TYPEDARRAY_FLOAT32,
  JJS_TYPEDARRAY_FLOAT64,
  JJS_TYPEDARRAY_BIGINT64,
  JJS_TYPEDARRAY_BIGUINT64,
} jjs_typedarray_type_t;

/**
 * Container types.
 */
typedef enum
{
  JJS_CONTAINER_TYPE_INVALID = 0, /**< Invalid container */
  JJS_CONTAINER_TYPE_MAP, /**< Map type */
  JJS_CONTAINER_TYPE_SET, /**< Set type */
  JJS_CONTAINER_TYPE_WEAKMAP, /**< WeakMap type */
  JJS_CONTAINER_TYPE_WEAKSET, /**< WeakSet type */
} jjs_container_type_t;

/**
 * Container operations
 */
typedef enum
{
  JJS_CONTAINER_OP_ADD, /**< Set/WeakSet add operation */
  JJS_CONTAINER_OP_GET, /**< Map/WeakMap get operation */
  JJS_CONTAINER_OP_SET, /**< Map/WeakMap set operation */
  JJS_CONTAINER_OP_HAS, /**< Set/WeakSet/Map/WeakMap has operation */
  JJS_CONTAINER_OP_DELETE, /**< Set/WeakSet/Map/WeakMap delete operation */
  JJS_CONTAINER_OP_SIZE, /**< Set/WeakSet/Map/WeakMap size operation */
  JJS_CONTAINER_OP_CLEAR, /**< Set/Map clear operation */
} jjs_container_op_t;

/**
 * Miscellaneous types.
 */

/**
 * Enabled fields of jjs_source_info_t.
 */
typedef enum
{
  JJS_SOURCE_INFO_HAS_SOURCE_CODE = (1 << 0), /**< source_code field is valid */
  JJS_SOURCE_INFO_HAS_FUNCTION_ARGUMENTS = (1 << 1), /**< function_arguments field is valid */
  JJS_SOURCE_INFO_HAS_SOURCE_RANGE = (1 << 2), /**< both source_range_start and source_range_length
                                                  *   fields are valid */
} jjs_source_info_enabled_fields_t;

/**
 * Source related information of a script/module/function.
 */
typedef struct
{
  uint32_t enabled_fields; /**< combination of jjs_source_info_enabled_fields_t values */
  jjs_value_t source_code; /**< script source code or function body */
  jjs_value_t function_arguments; /**< function arguments */
  uint32_t source_range_start; /**< start position of the function in the source code */
  uint32_t source_range_length; /**< source length of the function in the source code */
} jjs_source_info_t;

/**
 * Array buffer types.
 */

/**
 * Type of an array buffer.
 */
typedef enum
{
  JJS_ARRAYBUFFER_TYPE_ARRAYBUFFER, /**< the object is an array buffer object */
  JJS_ARRAYBUFFER_TYPE_SHARED_ARRAYBUFFER, /**< the object is a shared array buffer object */
} jjs_arraybuffer_type_t;

/**
 * Callback for allocating the backing store of array buffer or shared array buffer objects.
 */
typedef uint8_t *(*jjs_arraybuffer_allocate_cb_t) (jjs_arraybuffer_type_t buffer_type,
                                                     uint32_t buffer_size,
                                                     void **arraybuffer_user_p,
                                                     void *user_p);

/**
 * Callback for freeing the backing store of array buffer or shared array buffer objects.
 */
typedef void (*jjs_arraybuffer_free_cb_t) (jjs_arraybuffer_type_t buffer_type,
                                             uint8_t *buffer_p,
                                             uint32_t buffer_size,
                                             void *arraybuffer_user_p,
                                             void *user_p);

/**
 * Callback to create exports for a Virtual Module.
 *
 * The return value must be an object containing a key 'exports'. When the module
 * is required, the exports value will be returned. When the module is imported,
 * the exports.default value will be returned. If default does not exist, then
 * exports will be returned as the ES module default.
 *
 * If an error occurs, the return value should be an exception.
 *
 * The caller is responsible for calling jjs_value_free() on the returned value.
 *
 * @param name the name of the virtual module being created
 * @param user_p user_p value passed to the jjs_vmod_native* function. can be NULL.
 */
typedef jjs_value_t (*jjs_vmod_create_cb_t) (jjs_value_t name, void* user_p);

/**
 * Converts a megabyte value representation to a kilobyte format for size values
 * in jjs_init settings.
 *
 * This is not a general purpose conversion. It should only be used for
 * declaratively setting jjs_init size values.
 *
 * Example: init_options.vm_heap_size = JJS_TO_MB (4); // set heap size to 4MB
 */
#define JJS_TO_MB(VALUE) ((VALUE) * 1024)

/**
 * Converts a kilobyte value representation to a kilobyte format for size values
 * in jjs_init settings.
 *
 * This is not a general purpose conversion. It should only be used for
 * declaratively setting jjs_init size values.
 *
 * Example: init_options.vm_heap_size = JJS_TO_KB (512); // set heap size to 512K
 */
#define JJS_TO_KB(VALUE) (VALUE)

/**
 * Options for jjs_platform_read_file function.
 */
typedef struct
{
  jjs_encoding_t encoding; /**< method to decode file contents */
} jjs_platform_read_file_options_t;

/**
 * @}
 */

JJS_C_API_END

#endif /* !JJS_TYPES_H */
