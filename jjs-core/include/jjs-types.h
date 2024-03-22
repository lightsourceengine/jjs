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
 * JJS init flags.
 */
typedef enum
{
  JJS_INIT_EMPTY = (0u), /**< empty flag set */
  JJS_INIT_SHOW_OPCODES = (1u << 0), /**< dump byte-code to log after parse */
  JJS_INIT_SHOW_REGEXP_OPCODES = (1u << 1), /**< dump regexp byte-code to log after compilation */
  JJS_INIT_MEM_STATS = (1u << 2), /**< dump memory statistics */
} jjs_init_flag_t;

/**
 * Option bits for jjs_init_options_t.
 */
typedef enum
{
  JJS_INIT_OPTION_NO_OPTS = 0, /**< no options passed */
  JJS_INIT_OPTION_HAS_HEAP_SIZE = (1 << 0), /**< heap_size field is valid */
  JJS_INIT_OPTION_HAS_GC_LIMIT = (1 << 1), /**< gc_limit field is valid */
  JJS_INIT_OPTION_HAS_GC_MARK_LIMIT = (1 << 2), /**< gc_mark_limit field is valid */
  JJS_INIT_OPTION_HAS_STACK_LIMIT = (1 << 3), /**< stack_limit field is valid */
  JJS_INIT_OPTION_HAS_GC_NEW_OBJECTS_FRACTION = (1 << 4), /**< gc_new_objects_fraction field is valid */
  JJS_INIT_OPTION_ALL = 0xFFFF,
} jjs_init_option_enable_feature_t;

/**
 * Various configuration options for jjs_init_ex().
 */
typedef struct
{
  uint32_t flags; /**< combination of jjs_parse_option_enable_feature_t values */
  uint32_t heap_size; /**< maximum global heap size in bytes. if 0, JJS_GLOBAL_HEAP_SIZE define is used */
  uint32_t gc_limit; /**< allowed heap usage limit until next garbage collection. if 0, 1/32 of heap_size is used */
  uint32_t gc_mark_limit; /**< gc mark depth. if 0, mark depth is unlimited */
  uint32_t stack_limit; /**< maximum stack usage size in bytes. if 0, stack usage is unlimited. */
  uint32_t gc_new_objects_fraction; /**< Amount of newly allocated objects since the last GC run,
                                      * represented as a fraction of all allocated objects. If 0,
                                      * 16 is used.
                                      */
} jjs_init_options_t;

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
 * Options loading modules from in-memory source.
 *
 * ES modules are tied to paths, either package, network or filesystem. When embedding JJS into another
 * program, the embedder may want to load a module from in-memory source. These options are configuration
 * for the in-memory module to play nice with the rest of the ES module ecosystem.
 *
 * To initialize a jjs_esm_options_t, use jjs_esm_options_init() and cleanup with
 * jjs_esm_options_free(&options). If a JS value is assigned to options, the options object owns the reference
 * and jjs_esm_options_free() will clean it up. Note, this is slightly different, and less verbose, API than
 * other options/desc objects in JJS.
 *
 * When source is parsed and loaded, the options will affect import.meta in the following way:
 *
 * import.meta.filename  = join(jjs_esm_options_t.dirname, basename(jjs_esm_options_t.filename))
 * import.meta.dirname   = realpath(jjs_esm_options_t.dirname) or cwd()
 * import.meta.extension = jjs_esm_options_t.meta_extension
 * import.meta.url       = path_to_file_url(import.meta.filename)
 *
 * When options is null or the default:
 *
 * import.meta.filename  = join(cwd(), basename(jjs_esm_options_t.filename))
 * import.meta.dirname   = cwd()
 * import.meta.extension = undefined
 * import.meta.url       = path_to_file_url(import.meta.filename)
 *
 * dirname must be a fs path that can be resolved with realpath. The requirement exists so that the cache key
 * for the source module is consistent with the rest of the system. Also, dirname is the referrer path for the
 * module. The referrer path is used when the module calls require or import or resolve. By default, dirname
 * will be the cwd.
 *
 * import.meta.extension is a JJS specific concept. It is intended as a way to pass bindings or other stuff from
 * native to a module, accessed in the JS runtime by import.meta.extension.
 *
 * With respect to caching, by default, in-memory source modules will NOT be cached. Calling the same source and
 * options multiple times will compile, link and evaluate the source each time. If options.cache is true, the
 * source module will be cached using options.filename (fully resolved) as the cache keys. Attempts to use this
 * key for in-memory source will result in an exception. Static or dynamic imports will be able to import using the
 * key.
 */
typedef struct
{
  /**
   * Simple filename.
   *
   * The value must be a string with no path separators.
   *
   * If undefined, a default value of "<anonymous>.mjs" is used.
   */
  jjs_value_t filename;

  /**
   * Directory path.
   *
   * The value must be a string to a directory path on disk. realpath will be run on the string to ensure
   * the path and that the directory exists.
   *
   * If undefined, the current working directory (cwd) is used.
   */
  jjs_value_t dirname;

  /**
   * Value for import.meta.extension.
   *
   * If undefined, the value is ignored.
   */
  jjs_value_t meta_extension;

  uint32_t start_line; /**< start line of the source code. default is 0. */
  uint32_t start_column; /**< start column of the source code. default is 0. */

  /**
   * If true, this module is put into the ESM cache using the resolve dirname + filename as the
   * key. By default, caching is turned off.
   */
  bool cache;
} jjs_esm_options_t;

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
 * String encoding.
 */
typedef enum
{
  JJS_ENCODING_CESU8, /**< cesu-8 encoding */
  JJS_ENCODING_UTF8, /**< utf-8 encoding */
} jjs_encoding_t;

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
 * @}
 */

JJS_C_API_END

#endif /* !JJS_TYPES_H */
