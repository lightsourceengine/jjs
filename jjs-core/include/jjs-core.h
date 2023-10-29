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

#ifndef JJS_CORE_H
#define JJS_CORE_H

#include "jjs-types.h"

JJS_C_API_BEGIN

/**
 * @defgroup jjs-api JJS public API
 * @{
 */

/**
 * @defgroup jjs-api-general General functions
 * @{
 */

/**
 * @defgroup jjs-api-general-context Context management
 * @{
 */
void jjs_init (jjs_init_flag_t flags);
void jjs_init_ex (jjs_init_flag_t flags, jjs_init_options_t* options);
void jjs_cleanup (void);

void *jjs_context_data (const jjs_context_data_manager_t *manager_p);

jjs_value_t jjs_current_realm (void);
jjs_value_t jjs_set_realm (jjs_value_t realm);
/**
 * jjs-api-general-context @}
 */

/**
 * @defgroup jjs-api-general-heap Heap management
 * @{
 */
void *jjs_heap_alloc (jjs_size_t size);
void jjs_heap_free (void *mem_p, jjs_size_t size);

bool jjs_heap_stats (jjs_heap_stats_t *out_stats_p);
void jjs_heap_gc (jjs_gc_mode_t mode);

bool jjs_foreach_live_object (jjs_foreach_live_object_cb_t callback, void *user_data);
bool jjs_foreach_live_object_with_info (const jjs_object_native_info_t *native_info_p,
                                          jjs_foreach_live_object_with_info_cb_t callback,
                                          void *user_data_p);
/**
 * jjs-api-general-heap @}
 */

/**
 * @defgroup jjs-api-general-misc Miscellaneous
 * @{
 */

void JJS_ATTR_FORMAT (printf, 2, 3) jjs_log (jjs_log_level_t level, const char *format_p, ...);
void jjs_log_set_level (jjs_log_level_t level);
bool jjs_validate_string (const jjs_char_t *buffer_p, jjs_size_t buffer_size, jjs_encoding_t encoding);
bool JJS_ATTR_CONST jjs_feature_enabled (const jjs_feature_t feature);
void jjs_register_magic_strings (const jjs_char_t *const *ext_strings_p,
                                   uint32_t count,
                                   const jjs_length_t *str_lengths_p);
/**
 * jjs-api-general-misc @}
 */

/**
 * jjs-api-general @}
 */

/**
 * @defgroup jjs-api-code Scripts and Executables
 * @{
 */

/**
 * @defgroup jjs-api-code-parse Parsing
 * @{
 */
jjs_value_t jjs_parse (const jjs_char_t *source_p, size_t source_size, const jjs_parse_options_t *options_p);
jjs_value_t jjs_parse_value (const jjs_value_t source, const jjs_parse_options_t *options_p);
/**
 * jjs-api-code-parse @}
 */

/**
 * @defgroup jjs-api-code-exec Execution
 * @{
 */
jjs_value_t jjs_eval (const jjs_char_t *source_p, size_t source_size, uint32_t flags);
jjs_value_t jjs_run (const jjs_value_t script);
jjs_value_t jjs_run_jobs (void);
jjs_value_t jjs_queue_microtask(const jjs_value_t callback);
bool jjs_has_pending_jobs (void);
/**
 * jjs-api-code-exec @}
 */

/**
 * @defgroup jjs-api-code-sourceinfo Source information
 * @{
 */
jjs_value_t jjs_source_name (const jjs_value_t value);
jjs_value_t jjs_source_user_value (const jjs_value_t value);
jjs_source_info_t *jjs_source_info (const jjs_value_t value);
void jjs_source_info_free (jjs_source_info_t *source_info_p);
/**
 * jjs-api-code-sourceinfo @}
 */

/**
 * @defgroup jjs-api-code-cb Callbacks
 * @{
 */
void jjs_halt_handler (uint32_t interval, jjs_halt_cb_t callback, void *user_p);
/**
 * jjs-api-code-cb @}
 */

/**
 * jjs-api-code @}
 */

/**
 * @defgroup jjs-api-backtrace Backtraces
 * @{
 */

/**
 * @defgroup jjs-api-backtrace-capture Capturing
 * @{
 */
jjs_value_t jjs_backtrace (uint32_t max_depth);
void jjs_backtrace_capture (jjs_backtrace_cb_t callback, void *user_p);
/**
 * jjs-api-backtrace-capture @}
 */

/**
 * @defgroup jjs-api-backtrace-frame Frames
 * @{
 */
jjs_frame_type_t jjs_frame_type (const jjs_frame_t *frame_p);
const jjs_value_t *jjs_frame_callee (jjs_frame_t *frame_p);
const jjs_value_t *jjs_frame_this (jjs_frame_t *frame_p);
const jjs_frame_location_t *jjs_frame_location (jjs_frame_t *frame_p);
bool jjs_frame_is_strict (jjs_frame_t *frame_p);
/**
 * jjs-api-backtrace-frame @}
 */

/**
 * jjs-api-backtrace @}
 */

/**
 * @defgroup jjs-api-value Values
 * @{
 */

/* Reference management */
jjs_value_t JJS_ATTR_WARN_UNUSED_RESULT jjs_value_copy (const jjs_value_t value);
void jjs_value_free (jjs_value_t value);

/**
 * @defgroup jjs-api-value-checks Type inspection
 * @{
 */
jjs_type_t jjs_value_type (const jjs_value_t value);
bool jjs_value_is_exception (const jjs_value_t value);
bool jjs_value_is_abort (const jjs_value_t value);

bool jjs_value_is_undefined (const jjs_value_t value);
bool jjs_value_is_null (const jjs_value_t value);
bool jjs_value_is_boolean (const jjs_value_t value);
bool jjs_value_is_true (const jjs_value_t value);
bool jjs_value_is_false (const jjs_value_t value);

bool jjs_value_is_number (const jjs_value_t value);
bool jjs_value_is_bigint (const jjs_value_t value);

bool jjs_value_is_string (const jjs_value_t value);
bool jjs_value_is_symbol (const jjs_value_t value);

bool jjs_value_is_object (const jjs_value_t value);
bool jjs_value_is_array (const jjs_value_t value);
bool jjs_value_is_promise (const jjs_value_t value);
bool jjs_value_is_proxy (const jjs_value_t value);
bool jjs_value_is_arraybuffer (const jjs_value_t value);
bool jjs_value_is_shared_arraybuffer (const jjs_value_t value);
bool jjs_value_is_dataview (const jjs_value_t value);
bool jjs_value_is_typedarray (const jjs_value_t value);

bool jjs_value_is_constructor (const jjs_value_t value);
bool jjs_value_is_function (const jjs_value_t value);
bool jjs_value_is_async_function (const jjs_value_t value);

bool jjs_value_is_error (const jjs_value_t value);
/**
 * jjs-api-value-checks @}
 */

/**
 * @defgroup jjs-api-value-coerce Coercion
 * @{
 */
bool jjs_value_to_boolean (const jjs_value_t value);
jjs_value_t jjs_value_to_number (const jjs_value_t value);
jjs_value_t jjs_value_to_object (const jjs_value_t value);
jjs_value_t jjs_value_to_primitive (const jjs_value_t value);
jjs_value_t jjs_value_to_string (const jjs_value_t value);
jjs_value_t jjs_value_to_bigint (const jjs_value_t value);

double jjs_value_as_number (const jjs_value_t value);
double jjs_value_as_integer (const jjs_value_t value);
int32_t jjs_value_as_int32 (const jjs_value_t value);
uint32_t jjs_value_as_uint32 (const jjs_value_t value);
/**
 * jjs-api-value-coerce @}
 */

/**
 * @defgroup jjs-api-value-op Operations
 * @{
 */
jjs_value_t jjs_binary_op (jjs_binary_op_t operation, const jjs_value_t lhs, const jjs_value_t rhs);

/**
 * jjs-api-value-op @}
 */

/**
 * jjs-api-value @}
 */

/**
 * @defgroup jjs-api-exception Exceptions
 * @{
 */

/**
 * @defgroup jjs-api-exception-ctor Constructors
 * @{
 */
jjs_value_t jjs_throw (jjs_error_t type, const jjs_value_t message);
jjs_value_t jjs_throw_sz (jjs_error_t type, const char *message_p);
jjs_value_t jjs_throw_value (jjs_value_t value, bool take_ownership);
jjs_value_t jjs_throw_abort (jjs_value_t value, bool take_ownership);
/**
 * jjs-api-exception-ctor @}
 */

/**
 * @defgroup jjs-api-exception-op Operations
 * @{
 */
void jjs_exception_allow_capture (jjs_value_t value, bool allow_capture);
/**
 * jjs-api-exception-op @}
 */

/**
 * @defgroup jjs-api-exception-get Getters
 * @{
 */
jjs_value_t jjs_exception_value (jjs_value_t value, bool free_exception);
bool jjs_exception_is_captured (const jjs_value_t value);
/**
 * jjs-api-exception-get @}
 */

/**
 * @defgroup jjs-api-exception-cb Callbacks
 * @{
 */
void jjs_on_throw (jjs_throw_cb_t callback, void *user_p);
/**
 * jjs-api-exception-cb @}
 */

/**
 * jjs-api-error @}
 */

/**
 * @defgroup jjs-api-primitives Primitive types
 * @{
 */

/**
 * @defgroup jjs-api-undefined Undefined
 * @{
 */

/**
 * @defgroup jjs-api-undefined-ctor Constructors
 * @{
 */

jjs_value_t JJS_ATTR_CONST jjs_undefined (void);

/**
 * jjs-api-undefined-ctor @}
 */

/**
 * jjs-api-undefined @}
 */

/**
 * @defgroup jjs-api-null Null
 * @{
 */

/**
 * @defgroup jjs-api-null-ctor Constructors
 * @{
 */

jjs_value_t JJS_ATTR_CONST jjs_null (void);

/**
 * jjs-api-null-ctor @}
 */

/**
 * jjs-api-null @}
 */

/**
 * @defgroup jjs-api-boolean Boolean
 * @{
 */

/**
 * @defgroup jjs-api-boolean-ctor Constructors
 * @{
 */

jjs_value_t JJS_ATTR_CONST jjs_boolean (bool value);

/**
 * jjs-api-boolean-ctor @}
 */

/**
 * jjs-api-boolean @}
 */

/**
 * @defgroup jjs-api-number Number
 * @{
 */

/**
 * @defgroup jjs-api-number-ctor Number
 * @{
 */

jjs_value_t jjs_number (double value);
jjs_value_t jjs_infinity (bool sign);
jjs_value_t jjs_nan (void);

/**
 * jjs-api-number-ctor @}
 */

/**
 * jjs-api-number @}
 */

/**
 * @defgroup jjs-api-bigint BigInt
 * @{
 */

/**
 * @defgroup jjs-api-bigint-ctor Constructors
 * @{
 */
jjs_value_t jjs_bigint (const uint64_t *digits_p, uint32_t digit_count, bool sign);
/**
 * jjs-api-bigint-ctor @}
 */

/**
 * @defgroup jjs-api-bigint-get Getters
 * @{
 */
uint32_t jjs_bigint_digit_count (const jjs_value_t value);
/**
 * jjs-api-bigint-get @}
 */

/**
 * @defgroup jjs-api-bigint-op Operations
 * @{
 */
void jjs_bigint_to_digits (const jjs_value_t value, uint64_t *digits_p, uint32_t digit_count, bool *sign_p);
/**
 * jjs-api-bigint-get @}
 */

/**
 * jjs-api-bigint @}
 */

/**
 * @defgroup jjs-api-string String
 * @{
 */

/**
 * @defgroup jjs-api-string-ctor Constructors
 * @{
 */
jjs_value_t jjs_string (const jjs_char_t *buffer_p, jjs_size_t buffer_size, jjs_encoding_t encoding);
jjs_value_t jjs_string_sz (const char *str_p);
jjs_value_t jjs_string_external (const jjs_char_t *buffer_p, jjs_size_t buffer_size, void *user_p);
jjs_value_t jjs_string_external_sz (const char *str_p, void *user_p);
/**
 * jjs-api-string-cotr @}
 */

/**
 * @defgroup jjs-api-string-get Getters
 * @{
 */
jjs_size_t jjs_string_size (const jjs_value_t value, jjs_encoding_t encoding);
jjs_length_t jjs_string_length (const jjs_value_t value);
void *jjs_string_user_ptr (const jjs_value_t value, bool *is_external);
/**
 * jjs-api-string-get @}
 */

/**
 * @defgroup jjs-api-string-op Operations
 * @{
 */
jjs_size_t jjs_string_substr (const jjs_value_t value, jjs_length_t start, jjs_length_t end);
jjs_size_t jjs_string_to_buffer (const jjs_value_t value,
                                     jjs_encoding_t encoding,
                                     jjs_char_t *buffer_p,
                                     jjs_size_t buffer_size);
void jjs_string_iterate (const jjs_value_t value,
                           jjs_encoding_t encoding,
                           jjs_string_iterate_cb_t callback,
                           void *user_p);
/**
 * jjs-api-string-op @}
 */

/**
 * @defgroup jjs-api-string-cb Callbacks
 * @{
 */
void jjs_string_external_on_free (jjs_external_string_free_cb_t callback);
/**
 * jjs-api-string-cb @}
 */

/**
 * jjs-api-string @}
 */

/**
 * @defgroup jjs-api-symbol Symbol
 * @{
 */

/**
 * @defgroup jjs-api-symbol-ctor Constructors
 * @{
 */
jjs_value_t jjs_symbol (jjs_well_known_symbol_t symbol);
jjs_value_t jjs_symbol_with_description (const jjs_value_t value);
/**
 * jjs-api-symbol-ctor @}
 */

/**
 * @defgroup jjs-api-symbol-get Getters
 * @{
 */
jjs_value_t jjs_symbol_description (const jjs_value_t symbol);
jjs_value_t jjs_symbol_descriptive_string (const jjs_value_t symbol);
/**
 * jjs-api-symbol-get @}
 */

/**
 * jjs-api-symbol @}
 */

/**
 * jjs-api-primitives @}
 */

/**
 * @defgroup jjs-api-objects Objects
 * @{
 */

/**
 * @defgroup jjs-api-object-ctor Constructors
 * @{
 */
jjs_value_t jjs_object (void);
/**
 * jjs-api-object-ctor @}
 */

/**
 * @defgroup jjs-api-object-get Getters
 * @{
 */

jjs_object_type_t jjs_object_type (const jjs_value_t object);
jjs_value_t jjs_object_proto (const jjs_value_t object);
jjs_value_t jjs_object_keys (const jjs_value_t object);
jjs_value_t jjs_object_property_names (const jjs_value_t object, jjs_property_filter_t filter);

/**
 * jjs-api-object-get @}
 */

/**
 * @defgroup jjs-api-object-op Operations
 * @{
 */

jjs_value_t jjs_object_set_proto (jjs_value_t object, const jjs_value_t proto);
bool jjs_object_foreach (const jjs_value_t object, jjs_object_property_foreach_cb_t foreach_p, void *user_data_p);

/**
 * @defgroup jjs-api-object-op-set Set
 * @{
 */
jjs_value_t jjs_object_set (jjs_value_t object, const jjs_value_t key, const jjs_value_t value);
jjs_value_t jjs_object_set_sz (jjs_value_t object, const char *key_p, const jjs_value_t value);
jjs_value_t jjs_object_set_index (jjs_value_t object, uint32_t index, const jjs_value_t value);
jjs_value_t jjs_object_define_own_prop (jjs_value_t object,
                                            const jjs_value_t key,
                                            const jjs_property_descriptor_t *prop_desc_p);
bool jjs_object_set_internal (jjs_value_t object, const jjs_value_t key, const jjs_value_t value);
void jjs_object_set_native_ptr (jjs_value_t object,
                                  const jjs_object_native_info_t *native_info_p,
                                  void *native_pointer_p);
/**
 * jjs-api-object-op-set @}
 */

/**
 * @defgroup jjs-api-object-op-has Has
 * @{
 */
jjs_value_t jjs_object_has (const jjs_value_t object, const jjs_value_t key);
jjs_value_t jjs_object_has_sz (const jjs_value_t object, const char *key_p);
jjs_value_t jjs_object_has_own (const jjs_value_t object, const jjs_value_t key);
bool jjs_object_has_internal (const jjs_value_t object, const jjs_value_t key);
bool jjs_object_has_native_ptr (const jjs_value_t object, const jjs_object_native_info_t *native_info_p);
/**
 * jjs-api-object-op-has @}
 */

/**
 * @defgroup jjs-api-object-op-get Get
 * @{
 */
jjs_value_t jjs_object_get (const jjs_value_t object, const jjs_value_t key);
jjs_value_t jjs_object_get_sz (const jjs_value_t object, const char *key_p);
jjs_value_t jjs_object_get_index (const jjs_value_t object, uint32_t index);
jjs_value_t jjs_object_get_own_prop (const jjs_value_t object,
                                         const jjs_value_t key,
                                         jjs_property_descriptor_t *prop_desc_p);
jjs_value_t jjs_object_get_internal (const jjs_value_t object, const jjs_value_t key);
void *jjs_object_get_native_ptr (const jjs_value_t object, const jjs_object_native_info_t *native_info_p);

jjs_value_t jjs_object_find_own (const jjs_value_t object,
                                     const jjs_value_t key,
                                     const jjs_value_t receiver,
                                     bool *found_p);
/**
 * jjs-api-object-op-get @}
 */

/**
 * @defgroup jjs-api-object-op-del Delete
 * @{
 */
jjs_value_t jjs_object_delete (jjs_value_t object, const jjs_value_t key);
jjs_value_t jjs_object_delete_sz (const jjs_value_t object, const char *key_p);
jjs_value_t jjs_object_delete_index (jjs_value_t object, uint32_t index);
bool jjs_object_delete_internal (jjs_value_t object, const jjs_value_t key);
bool jjs_object_delete_native_ptr (jjs_value_t object, const jjs_object_native_info_t *native_info_p);
/**
 * jjs-api-object-op-del @}
 */

/**
 * jjs-api-object-op @}
 */

/**
 * @defgroup jjs-api-object-prop-desc Property descriptors
 * @{
 */

/**
 * @defgroup jjs-api-object-prop-desc-ctor Constructors
 * @{
 */
jjs_property_descriptor_t jjs_property_descriptor (void);
jjs_value_t jjs_property_descriptor_from_object (const jjs_value_t obj_value,
                                                     jjs_property_descriptor_t *out_prop_desc_p);
/**
 * jjs-api-object-prop-desc-ctor @}
 */

/**
 * @defgroup jjs-api-object-prop-desc-op Operations
 * @{
 */
void jjs_property_descriptor_free (jjs_property_descriptor_t *prop_desc_p);
jjs_value_t jjs_property_descriptor_to_object (const jjs_property_descriptor_t *src_prop_desc_p);
/**
 * jjs-api-object-prop-desc-op @}
 */

/**
 * jjs-api-object-prop-desc @}
 */

/**
 * @defgroup jjs-api-object-native-ptr Native pointers
 * @{
 */

/**
 * @defgroup jjs-api-object-native-ptr-op Operations
 * @{
 */
void jjs_native_ptr_init (void *native_pointer_p, const jjs_object_native_info_t *native_info_p);
void jjs_native_ptr_free (void *native_pointer_p, const jjs_object_native_info_t *native_info_p);
void jjs_native_ptr_set (jjs_value_t *reference_p, const jjs_value_t value);
/**
 * jjs-api-object-native-ptr-op @}
 */

/**
 * jjs-api-object-native-ptr @}
 */

/**
 * @defgroup jjs-api-array Array
 * @{
 */

/**
 * @defgroup jjs-api-array-ctor Constructors
 * @{
 */
jjs_value_t jjs_array (jjs_length_t length);
/**
 * jjs-api-array-ctor @}
 */

/**
 * @defgroup jjs-api-array-get Getters
 * @{
 */
jjs_length_t jjs_array_length (const jjs_value_t value);
/**
 * jjs-api-array-get @}
 */

/**
 * jjs-api-array @}
 */

/**
 * @defgroup jjs-api-arraybuffer ArrayBuffer
 * @{
 */

/**
 * @defgroup jjs-api-arraybuffer-ctor Constructors
 * @{
 */
jjs_value_t jjs_arraybuffer (const jjs_length_t size);
jjs_value_t jjs_arraybuffer_external (uint8_t *buffer_p, jjs_length_t size, void *user_p);
/**
 * jjs-api-arraybuffer-ctor @}
 */

/**
 * @defgroup jjs-api-arraybuffer-get Getters
 * @{
 */
jjs_size_t jjs_arraybuffer_size (const jjs_value_t value);
uint8_t *jjs_arraybuffer_data (const jjs_value_t value);
bool jjs_arraybuffer_is_detachable (const jjs_value_t value);
bool jjs_arraybuffer_has_buffer (const jjs_value_t value);
/**
 * jjs-api-arraybuffer-get @}
 */

/**
 * @defgroup jjs-api-arraybuffer-op Operations
 * @{
 */
jjs_size_t
jjs_arraybuffer_read (const jjs_value_t value, jjs_size_t offset, uint8_t *buffer_p, jjs_size_t buffer_size);
jjs_size_t
jjs_arraybuffer_write (jjs_value_t value, jjs_size_t offset, const uint8_t *buffer_p, jjs_size_t buffer_size);
jjs_value_t jjs_arraybuffer_detach (jjs_value_t value);
void jjs_arraybuffer_heap_allocation_limit (jjs_size_t limit);
/**
 * jjs-api-arraybuffer-op @}
 */

/**
 * @defgroup jjs-api-arraybuffer-cb Callbacks
 * @{
 */
void jjs_arraybuffer_allocator (jjs_arraybuffer_allocate_cb_t allocate_callback,
                                  jjs_arraybuffer_free_cb_t free_callback,
                                  void *user_p);
/**
 * jjs-api-arraybuffer-cb @}
 */

/**
 * jjs-api-arraybuffer @}
 */

/**
 * @defgroup jjs-api-sharedarraybuffer SharedArrayBuffer
 * @{
 */

/**
 * @defgroup jjs-api-sharedarraybuffer-ctor Constructors
 * @{
 */
jjs_value_t jjs_shared_arraybuffer (jjs_size_t size);
jjs_value_t jjs_shared_arraybuffer_external (uint8_t *buffer_p, jjs_size_t buffer_size, void *user_p);
/**
 * jjs-api-sharedarraybuffer-ctor @}
 */

/**
 * jjs-api-sharedarraybuffer @}
 */

/**
 * @defgroup jjs-api-dataview DataView
 * @{
 */

/**
 * @defgroup jjs-api-dataview-ctor Constructors
 * @{
 */
jjs_value_t jjs_dataview (const jjs_value_t value, jjs_size_t byte_offset, jjs_size_t byte_length);
/**
 * jjs-api-dataview-ctr @}
 */

/**
 * @defgroup jjs-api-dataview-get Getters
 * @{
 */
jjs_value_t
jjs_dataview_buffer (const jjs_value_t dataview, jjs_size_t *byte_offset, jjs_size_t *byte_length);
/**
 * jjs-api-dataview-get @}
 */

/**
 * jjs-api-dataview @}
 */

/**
 * @defgroup jjs-api-typedarray TypedArray
 * @{
 */

/**
 * @defgroup jjs-api-typedarray-ctor Constructors
 * @{
 */
jjs_value_t jjs_typedarray (jjs_typedarray_type_t type, jjs_length_t length);
jjs_value_t jjs_typedarray_with_buffer (jjs_typedarray_type_t type, const jjs_value_t arraybuffer);
jjs_value_t jjs_typedarray_with_buffer_span (jjs_typedarray_type_t type,
                                                 const jjs_value_t arraybuffer,
                                                 jjs_size_t byte_offset,
                                                 jjs_size_t byte_length);
/**
 * jjs-api-typedarray-ctor @}
 */

/**
 * @defgroup jjs-api-typedarray-get Getters
 * @{
 */
jjs_typedarray_type_t jjs_typedarray_type (const jjs_value_t value);
jjs_length_t jjs_typedarray_length (const jjs_value_t value);
jjs_value_t jjs_typedarray_buffer (const jjs_value_t value, jjs_size_t *byte_offset, jjs_size_t *byte_length);
/**
 * jjs-api-typedarray-get @}
 */

/**
 * jjs-api-typedarray @}
 */

/**
 * @defgroup jjs-api-iterator Iterator
 * @{
 */

/**
 * @defgroup jjs-api-iterator-get Getters
 * @{
 */
jjs_iterator_type_t jjs_iterator_type (const jjs_value_t value);
/**
 * jjs-api-iterator-get @}
 */

/**
 * jjs-api-iterator @}
 */

/**
 * @defgroup jjs-api-function Function
 * @{
 */

/**
 * @defgroup jjs-api-function-ctor Constructors
 * @{
 */
jjs_value_t jjs_function_external (jjs_external_handler_t handler);
/**
 * jjs-api-function-ctor @}
 */

/**
 * @defgroup jjs-api-function-get Getters
 * @{
 */
jjs_function_type_t jjs_function_type (const jjs_value_t value);
bool jjs_function_is_dynamic (const jjs_value_t value);
/**
 * jjs-api-function-get @}
 */

/**
 * @defgroup jjs-api-function-op Operations
 * @{
 */
jjs_value_t jjs_call (const jjs_value_t function,
                          const jjs_value_t this_value,
                          const jjs_value_t *args_p,
                          jjs_size_t args_count);
jjs_value_t jjs_construct (const jjs_value_t function, const jjs_value_t *args_p, jjs_size_t args_count);
/**
 * jjs-api-function-op @}
 */

/**
 * jjs-api-function @}
 */

/**
 * @defgroup jjs-api-proxy Proxy
 * @{
 */

/**
 * @defgroup jjs-api-proxy-ctor Constructors
 * @{
 */
jjs_value_t jjs_proxy (const jjs_value_t target, const jjs_value_t handler);
jjs_value_t jjs_proxy_custom (const jjs_value_t target, const jjs_value_t handler, uint32_t flags);
/**
 * jjs-api-function-proxy-ctor @}
 */

/**
 * @defgroup jjs-api-proxy-get Getters
 * @{
 */
jjs_value_t jjs_proxy_target (const jjs_value_t value);
jjs_value_t jjs_proxy_handler (const jjs_value_t value);
/**
 * jjs-api-function-proxy-get @}
 */

/**
 * jjs-api-proxy @}
 */

/**
 * @defgroup jjs-api-promise Promise
 * @{
 */

/**
 * @defgroup jjs-api-promise-ctor Constructors
 * @{
 */
jjs_value_t jjs_promise (void);
/**
 * jjs-api-promise-ctor @}
 */

/**
 * @defgroup jjs-api-promise-get Getters
 * @{
 */
jjs_value_t jjs_promise_result (const jjs_value_t promise);
jjs_promise_state_t jjs_promise_state (const jjs_value_t promise);
/**
 * jjs-api-promise-get @}
 */

/**
 * @defgroup jjs-api-promise-op Operations
 * @{
 */
jjs_value_t jjs_promise_resolve (jjs_value_t promise, const jjs_value_t argument);
jjs_value_t jjs_promise_reject (jjs_value_t promise, const jjs_value_t argument);
/**
 * jjs-api-promise-op @}
 */

/**
 * @defgroup jjs-api-promise-cb Callbacks
 * @{
 */
void jjs_promise_on_event (jjs_promise_event_filter_t filters, jjs_promise_event_cb_t callback, void *user_p);
/**
 * jjs-api-promise-cb @}
 */

/**
 * jjs-api-promise @}
 */

/**
 * @defgroup jjs-api-container Map, Set, WeakMap, WeakSet
 * @{
 */

/**
 * @defgroup jjs-api-container-ctor Constructors
 * @{
 */
jjs_value_t jjs_container (jjs_container_type_t container_type,
                               const jjs_value_t *arguments_p,
                               jjs_length_t argument_count);
/**
 * jjs-api-promise-ctor @}
 */

/**
 * @defgroup jjs-api-container-get Getters
 * @{
 */
jjs_container_type_t jjs_container_type (const jjs_value_t value);
/**
 * jjs-api-container-get @}
 */

/**
 * @defgroup jjs-api-container-op Operations
 * @{
 */
jjs_value_t jjs_container_to_array (const jjs_value_t value, bool *is_key_value_p);
jjs_value_t jjs_container_op (jjs_container_op_t operation,
                                  jjs_value_t container,
                                  const jjs_value_t *arguments,
                                  uint32_t argument_count);
/**
 * jjs-api-container-op @}
 */

/**
 * jjs-api-container @}
 */

/**
 * @defgroup jjs-api-regexp RegExp
 * @{
 */

/**
 * @defgroup jjs-api-regexp-ctor Constructors
 * @{
 */
jjs_value_t jjs_regexp (const jjs_value_t pattern, uint16_t flags);
jjs_value_t jjs_regexp_sz (const char *pattern_p, uint16_t flags);
/**
 * jjs-api-regexp-ctor @}
 */

/**
 * jjs-api-regexp @}
 */

/**
 * @defgroup jjs-api-error Error
 * @{
 */

/**
 * @defgroup jjs-api-error-ctor Constructors
 * @{
 */
jjs_value_t jjs_error (jjs_error_t type, const jjs_value_t message, const jjs_value_t options);
jjs_value_t jjs_error_sz (jjs_error_t type, const char *message_p, const jjs_value_t options);
/**
 * jjs-api-error-ctor @}
 */

/**
 * @defgroup jjs-api-error-get Getters
 * @{
 */
jjs_error_t jjs_error_type (jjs_value_t value);
/**
 * jjs-api-error-get @}
 */

/**
 * @defgroup jjs-api-error-cb Callbacks
 * @{
 */
void jjs_error_on_created (jjs_error_object_created_cb_t callback, void *user_p);
/**
 * jjs-api-error-cb @}
 */

/**
 * jjs-api-error @}
 */

/**
 * @defgroup jjs-api-aggregate-error AggregateError
 * @{
 */

/**
 * @defgroup jjs-api-aggregate-error-ctor Constructors
 * @{
 */
jjs_value_t jjs_aggregate_error (const jjs_value_t errors, const jjs_value_t message, const jjs_value_t options);
jjs_value_t jjs_aggregate_error_sz (const jjs_value_t errors, const char *message_p, const jjs_value_t options);
/**
 * jjs-api-aggregate-error--ctor @}
 */

/**
 * jjs-api-aggregate-error @}
 */

/**
 * jjs-api-objects @}
 */

/**
 * @defgroup jjs-api-json JSON
 * @{
 */

/**
 * @defgroup jjs-api-json-op Operations
 * @{
 */
jjs_value_t jjs_json_parse (const jjs_char_t *string_p, jjs_size_t string_size);
jjs_value_t jjs_json_stringify (const jjs_value_t object);
/**
 * jjs-api-json-op @}
 */

/**
 * jjs-api-json @}
 */

/**
 * @defgroup jjs-api-module Modules
 * @{
 */

/**
 * @defgroup jjs-api-module-get Getters
 * @{
 */
jjs_module_state_t jjs_module_state (const jjs_value_t module);
size_t jjs_module_request_count (const jjs_value_t module);
jjs_value_t jjs_module_request (const jjs_value_t module, size_t request_index);
jjs_value_t jjs_module_namespace (const jjs_value_t module);
/**
 * jjs-api-module-get @}
 */

/**
 * @defgroup jjs-api-module-op Operations
 * @{
 */

/**
 * Resolve and parse a module file
 *
 * @param specifier: module request specifier string.
 * @param referrer: parent module.
 * @param user_p: user specified pointer.
 *
 * @return module object if resolving is successful, error otherwise.
 */
jjs_value_t jjs_module_resolve (const jjs_value_t specifier, const jjs_value_t referrer, void *user_p);

jjs_value_t jjs_module_link (const jjs_value_t module, jjs_module_link_cb_t callback, void *user_p);
jjs_value_t jjs_module_evaluate (const jjs_value_t module);

/**
 * Release known modules in the current context. If realm parameter is supplied, cleans up modules native to that realm
 * only. This function should be called by the user application when the module database in the current context is no
 * longer needed.
 *
 * @param realm: release only those modules which realm value is equal to this argument.
 */
void jjs_module_cleanup (const jjs_value_t realm);

/**
 * jjs-api-module-op @}
 */

/**
 * @defgroup jjs-api-module-native Native modules
 * @{
 */
jjs_value_t jjs_native_module (jjs_native_module_evaluate_cb_t callback,
                                   const jjs_value_t *const exports_p,
                                   size_t export_count);
jjs_value_t jjs_native_module_get (const jjs_value_t native_module, const jjs_value_t export_name);
jjs_value_t
jjs_native_module_set (jjs_value_t native_module, const jjs_value_t export_name, const jjs_value_t value);
/**
 * jjs-api-module-native @}
 */

/**
 * @defgroup jjs-api-module-cb Callbacks
 * @{
 */
void jjs_module_on_state_changed (jjs_module_state_changed_cb_t callback, void *user_p);
void jjs_module_on_import_meta (jjs_module_import_meta_cb_t callback, void *user_p);
void jjs_module_on_import (jjs_module_import_cb_t callback, void *user_p);
void jjs_module_on_load (jjs_module_load_cb_t callback_p, void *user_p);
void jjs_module_on_resolve (jjs_module_resolve_cb_t callback_p, void *user_p);

jjs_value_t jjs_module_default_load (jjs_value_t path, jjs_module_load_context_t* context_p, void *user_p);
jjs_value_t jjs_module_default_resolve (jjs_value_t specifier, jjs_module_resolve_context_t* context_p, void *user_p);
jjs_value_t jjs_module_default_import (jjs_value_t specifier, jjs_value_t user_value, void *user_p);
void jjs_module_default_import_meta (jjs_value_t module, jjs_value_t meta_object, void *user_p);
/**
 * jjs-api-module-cb @}
 */

/**
 * jjs-api-module @}
 */

/**
 * @defgroup jjs-pmap Property Map
 * @{
 */

/**
 * @defgroup jjs-pmap-ops Operations
 * @{
 */

jjs_value_t jjs_pmap_from_file (jjs_value_t filename);
jjs_value_t jjs_pmap_from_file_sz (const char* filename_sz);
jjs_value_t jjs_pmap_from_json (jjs_value_t json_string, jjs_value_t root);

jjs_value_t jjs_pmap_resolve (jjs_value_t specifier, jjs_module_type_t module_type);
jjs_value_t jjs_pmap_resolve_sz (const char* specifier_sz, jjs_module_type_t module_type);

/**
 * jjs-pmap-ops @}
 */

/**
 * jjs-pmap @}
 */

/**
 * @defgroup jjs-commonjs CommonJS
 * @{
 */

/**
 * @defgroup jjs-commonjs-ops Operations
 * @{
 */

jjs_value_t jjs_commonjs_require (jjs_value_t specifier);
jjs_value_t jjs_commonjs_require_sz (const char* specifier_p);

/**
 * jjs-commonjs-ops @}
 */

/**
 * jjs-commonjs @}
 */

/**
 * @defgroup jjs-esm ES Modules
 * @{
 */

/**
 * @defgroup jjs-esm-ops Operations
 * @{
 */

jjs_value_t jjs_esm_import (jjs_value_t specifier);
jjs_value_t jjs_esm_import_sz (const char* specifier_p);
jjs_value_t jjs_esm_evaluate (jjs_value_t specifier);
jjs_value_t jjs_esm_evaluate_sz (const char* specifier_p);

/**
 * jjs-esm-ops @}
 */

/**
 * jjs-esm @}
 */

/**
 * @defgroup jjs-api-realm Realms
 * @{
 */

/**
 * @defgroup jjs-api-realm-ctor Constructors
 * @{
 */
jjs_value_t jjs_realm (void);
/**
 * jjs-api-realm-ctor @}
 */

/**
 * @defgroup jjs-api-realm-get Getters
 * @{
 */
jjs_value_t jjs_realm_this (jjs_value_t realm);
/**
 * jjs-api-realm-ctor @}
 */

/**
 * @defgroup jjs-api-realm-op Operation
 * @{
 */
jjs_value_t jjs_realm_set_this (jjs_value_t realm, jjs_value_t this_value);
/**
 * jjs-api-realm-op @}
 */

/**
 * jjs-api-realm @}
 */

/**
 * jjs-api @}
 */

JJS_C_API_END

#endif /* !JJS_CORE_H */

/* vim: set fdm=marker fmr=@{,@}: */
