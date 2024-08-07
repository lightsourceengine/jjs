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

/*
 * Engine context for JJS
 */
#ifndef JCONTEXT_H
#define JCONTEXT_H

#include "jjs-debugger-transport.h"

#include "ecma-builtins.h"
#include "ecma-helpers.h"
#include "ecma-jobqueue.h"

#include "debugger.h"
#include "jmem.h"
#include "js-parser-internal.h"
#include "re-bytecode.h"
#include "vm-defines.h"

/** \addtogroup context Context
 * @{
 */

/**
 * Context Flags.
 */
typedef enum
{
  JJS_CONTEXT_FLAG_NONE = (0u), /**< empty flag set */
  JJS_CONTEXT_FLAG_SHOW_OPCODES = (1u << 0), /**< dump byte-code to log after parse */
  JJS_CONTEXT_FLAG_SHOW_REGEXP_OPCODES = (1u << 1), /**< dump regexp byte-code to log after compilation */
  JJS_CONTEXT_FLAG_MEM_STATS = (1u << 2), /**< dump memory statistics */
  JJS_CONTEXT_FLAG_STRICT_MEMORY_LAYOUT = (1u << 3) /**< enable strict context memory layout */
} jjs_context_flag_t;

/**
 * Heap structure
 *
 * Memory blocks returned by the allocator must not start from the
 * beginning of the heap area because offset 0 is reserved for
 * JMEM_CP_NULL. This special constant is used in several places,
 * e.g. it marks the end of the property chain list, so it cannot
 * be eliminated from the project. Although the allocator cannot
 * use the first 8 bytes of the heap, nothing prevents to use it
 * for other purposes. Currently the free region start is stored
 * there.
 */
typedef struct
{
  jmem_heap_free_t first; /**< first node in free region list */
  uint8_t area[]; /**< heap area */
} jmem_heap_t;

typedef struct
{
  char id[JJS_CONTEXT_DATA_ID_LIMIT];
  int32_t id_size;
  void* data_p;
} jjs_context_data_entry_t;

/**
 * JJS context
 *
 * The purpose of this header is storing all global variables for JJS.
 */
struct jjs_context_t
{
  jmem_heap_t *heap_p; /**< point to the heap aligned to JMEM_ALIGNMENT. */
  jmem_cellocator_t jmem_cellocator_32; /**< vm heap cell allocator. 32 byte cells. */

  uint32_t context_flags; /**< context flags */
  jjs_allocator_t context_allocator; /**< allocator that created this context, scratch and vm heap. stored for cleanup only. */
  jjs_allocator_t vm_allocator; /**< vm allocator associated with the context */
  jjs_size_t context_block_size_b; /**< size of context allocation. used to free context. */

  ecma_global_object_t *global_object_p; /**< current global object */

  jjs_log_level_t log_level; /**< log level. log requests at this level of less will be logged. */

  jjs_platform_io_target_t *io_target[2]; /**< JJS_STDOUT and JJS_STDERR native stream objects */
  jjs_encoding_t io_target_encoding[2]; /**< JJS_STDOUT and JJS_STDERR string encoding */

  jjs_promise_unhandled_rejection_cb_t
    unhandled_rejection_cb; /**< called when a promise rejection has no handler. cannot be NULL. */
  void *unhandled_rejection_user_p; /**< user defined token passed to unhandled_rejection_cb */

  uint32_t vm_heap_size; /**< size of vm heap */
  uint32_t vm_stack_limit; /**< vm stack limit. if 0, no stack limit checks are performed. */
  uint32_t vm_cell_count; /**< number of cells per vm cell allocator page */
  uint32_t gc_limit; /**< allocation limit before triggering a gc */
  uint32_t gc_mark_limit; /**< gc mark recursion depth */
  uint32_t gc_new_objects_fraction; /**< number of new objects before triggering gc */

  jmem_heap_free_t *jmem_heap_list_skip_p; /**< improves deallocation performance */
  uint8_t* jmem_area_end; /**< precomputed address of end of heap; only for pointer validation */

  jjs_context_data_entry_t data_entries[JJS_CONTEXT_DATA_LIMIT]; /**< context data entries */
  int32_t data_entries_size; /**< number of data_entries in use */

#if JJS_BUILTIN_REGEXP
  re_compiled_code_t *re_cache[RE_CACHE_SIZE]; /**< regex cache */
#endif /* JJS_BUILTIN_REGEXP */
  const lit_utf8_byte_t *const *lit_magic_string_ex_array; /**< array of external magic strings */
  const lit_utf8_size_t *lit_magic_string_ex_sizes; /**< external magic string lengths */
  jmem_cpointer_t ecma_gc_objects_cp; /**< List of currently alive objects. */
  jmem_cpointer_t symbol_list_first_cp; /**< first item of the global symbol list */
  jmem_cpointer_t number_list_first_cp; /**< first item of the literal number list */
#if JJS_BUILTIN_BIGINT
  jmem_cpointer_t bigint_list_first_cp; /**< first item of the literal bigint list */
#endif /* JJS_BUILTIN_BIGINT */
  jmem_cpointer_t global_symbols_cp[ECMA_BUILTIN_GLOBAL_SYMBOL_COUNT]; /**< global symbols */
  ecma_hashset_t string_literal_pool; /**< string literal cache used during parsing and snapshot loading */
#if JJS_MODULE_SYSTEM
  ecma_module_t *module_current_p; /**< current module context */
  ecma_module_on_init_scope_cb module_on_init_scope_p; /**< callback which is called on initialization
                                                          * of module scope */
  jjs_module_state_changed_cb_t module_state_changed_callback_p; /**< callback which is called after the
                                                                    *   state of a module is changed */
  void *module_state_changed_callback_user_p; /**< user pointer for module_state_changed_callback_p */
  jjs_module_import_meta_cb_t module_import_meta_callback_p; /**< callback which is called when an
                                                                *   import.meta expression of a module
                                                                *   is evaluated the first time */
  void *module_import_meta_callback_user_p; /**< user pointer for module_import_meta_callback_p */
  jjs_module_import_cb_t module_import_callback_p; /**< callback for dynamic module import */
  void *module_import_callback_user_p; /**< user pointer for module_import_callback_p */
#endif /* JJS_MODULE_SYSTEM */

#if JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM
  jjs_esm_load_cb_t module_on_load_cb; /**< callback for CommonJS module loading */
  void *module_on_load_user_p; /**< user pointer for commonjs_load_callback_p */
  jjs_esm_resolve_cb_t module_on_resolve_cb; /**< callback for CommonJS module resolving */
  void *module_on_resolve_user_p; /**< user pointer for commonjs_resolve_callback_p */
#endif /* JJS_ANNEX_COMMONJS || JJS_ANNEX_ESM */

#if JJS_ANNEX_COMMONJS
  ecma_value_t commonjs_args; /**< arguments of the CommonJS module */
#endif /* JJS_ANNEX_COMMONJS */

  vm_frame_ctx_t *vm_top_context_p; /**< top (current) interpreter context */
  jjs_external_string_free_cb_t external_string_free_callback_p; /**< free callback for external strings */
  void *error_object_created_callback_user_p; /**< user pointer for error_object_update_callback_p */
  jjs_error_object_created_cb_t error_object_created_callback_p; /**< decorator callback for Error objects */
  size_t ecma_gc_objects_number; /**< number of currently allocated objects */
  size_t ecma_gc_new_objects; /**< number of newly allocated objects since last GC session */
  size_t jmem_heap_allocated_size; /**< size of allocated regions */
  size_t jmem_heap_limit; /**< current limit of heap usage, that is upon being reached,
                           *   causes call of "try give memory back" callbacks */
  ecma_value_t error_value; /**< currently thrown error value */
  uint32_t lit_magic_string_ex_count; /**< external magic strings count */
  uint32_t status_flags; /**< run-time flags (the top 8 bits are used for passing class parsing options) */

  uint32_t ecma_gc_mark_recursion_limit; /**< GC mark recursion limit */

#if JJS_PROPERTY_HASHMAP
  uint8_t ecma_prop_hashmap_alloc_state; /**< property hashmap allocation state: 0-4,
                                          *   if !0 property hashmap allocation is disabled */
#endif /* JJS_PROPERTY_HASHMAP */

#if JJS_BUILTIN_REGEXP
  uint8_t re_cache_idx; /**< evicted item index when regex cache is full (round-robin) */
#endif /* JJS_BUILTIN_REGEXP */
  ecma_job_queue_item_t *job_queue_head_p; /**< points to the head item of the job queue */
  ecma_job_queue_item_t *job_queue_tail_p; /**< points to the tail item of the job queue */
#if JJS_PROMISE_CALLBACK
  uint32_t promise_callback_filters; /**< reported event types for promise callback */
  void *promise_callback_user_p; /**< user pointer for promise callback */
  jjs_promise_event_cb_t promise_callback; /**< user function for tracking Promise object operations */
#endif /* JJS_PROMISE_CALLBACK */

#if JJS_BUILTIN_TYPEDARRAY
  uint32_t arraybuffer_compact_allocation_limit; /**< maximum size of compact allocation */
  jjs_arraybuffer_allocate_cb_t arraybuffer_allocate_callback; /**< callback for allocating
                                                                *   arraybuffer memory */
  jjs_arraybuffer_free_cb_t arraybuffer_free_callback; /**< callback for freeing arraybuffer memory */
  void *arraybuffer_allocate_callback_user_p; /**< user pointer passed to arraybuffer_allocate_callback
                                               *   and arraybuffer_free_callback functions */
#endif /* JJS_BUILTIN_TYPEDARRAY */

#if JJS_VM_HALT
  uint32_t vm_exec_stop_frequency; /**< reset value for vm_exec_stop_counter */
  uint32_t vm_exec_stop_counter; /**< down counter for reducing the calls of vm_exec_stop_cb */
  void *vm_exec_stop_user_p; /**< user pointer for vm_exec_stop_cb */
  ecma_vm_exec_stop_callback_t vm_exec_stop_cb; /**< user function which returns whether the
                                                 *   ECMAScript execution should be stopped */
#endif /* JJS_VM_HALT */

#if JJS_VM_THROW
  void *vm_throw_callback_user_p; /**< user pointer for vm_throw_callback_p */
  jjs_throw_cb_t vm_throw_callback_p; /**< callback for capturing throws */
#endif /* JJS_VM_THROW */

  volatile uintptr_t stack_base; /**< stack base marker */

#if JJS_DEBUGGER
  uint8_t debugger_send_buffer[JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE]; /**< buffer for sending messages */
  uint8_t debugger_receive_buffer[JJS_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE]; /**< buffer for receiving messages */
  jjs_debugger_transport_header_t *debugger_transport_header_p; /**< head of transport protocol chain */
  uint8_t *debugger_send_buffer_payload_p; /**< start where the outgoing message can be written */
  vm_frame_ctx_t *debugger_stop_context; /**< stop only if the current context is equal to this context */
  const uint8_t *debugger_exception_byte_code_p; /**< Location of the currently executed byte code if an
                                                  *   error occours while the vm_loop is suspended */
  jmem_cpointer_t debugger_byte_code_free_head; /**< head of byte code free linked list */
  jmem_cpointer_t debugger_byte_code_free_tail; /**< tail of byte code free linked list */
  uint32_t debugger_flags; /**< debugger flags */
  uint16_t debugger_received_length; /**< length of currently received bytes */
  uint16_t debugger_eval_chain_index; /**< eval chain index */
  uint8_t debugger_message_delay; /**< call receive message when reaches zero */
  uint8_t debugger_max_send_size; /**< maximum amount of data that can be sent */
  uint8_t debugger_max_receive_size; /**< maximum amount of data that can be received */
#endif /* JJS_DEBUGGER */

#if JJS_MEM_STATS
  jmem_heap_stats_t jmem_heap_stats; /**< heap's memory usage statistics */
#endif /* JJS_MEM_STATS */

  /* This must be at the end of the context for performance reasons */
#if JJS_LCACHE
  /** hash table for caching the last access of properties */
  ecma_lcache_hash_entry_t lcache[ECMA_LCACHE_HASH_ROWS_COUNT][ECMA_LCACHE_HASH_ROW_LENGTH];
#endif /* JJS_LCACHE */

#if JJS_ANNEX_PMAP
  ecma_value_t pmap; /**< global package map */
  ecma_value_t pmap_root; /**< base directory for resolving relative pmap paths */
#endif /* JJS_ANNEX_PMAP */

  /**
   * Allowed values and it's meaning:
   * * NULL (0x0): the current "new.target" is undefined, that is the execution is inside a normal method.
   * * Any other valid function object pointer: the current "new.target" is valid and it is constructor call.
   */
  ecma_object_t *current_new_target_p;

  jmem_scratch_allocator_t scratch_allocator; /**< allocator for internal temporary allocations */
};

void jcontext_set_exception_flag (jjs_context_t *context_p, bool is_exception);

void jcontext_set_abort_flag (jjs_context_t *context_p, bool is_abort);

bool jcontext_has_pending_exception (jjs_context_t *context_p);

bool jcontext_has_pending_abort (jjs_context_t *context_p);

void jcontext_raise_exception (jjs_context_t *context_p, ecma_value_t error);

void jcontext_release_exception (jjs_context_t *context_p);

ecma_value_t jcontext_take_exception (jjs_context_t *context_p);

/**
 * @}
 */

#endif /* !JCONTEXT_H */
