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

jjs_status_t jjs_context_new (const jjs_context_options_t* options_p, jjs_context_t** context_p);
jjs_status_t jjs_context_new_with_allocator (const jjs_context_options_t* options_p, const jjs_allocator_t *allocator_p, jjs_context_t** context_p);
jjs_status_t jjs_context_new_with_buffer (const jjs_context_options_t* options_p, uint8_t *buffer_p, jjs_size_t buffer_size, jjs_context_t** context_p);

void jjs_context_free (jjs_context_t* context_p);

jjs_status_t jjs_context_data_init (jjs_context_t *context_p, const char *id_p, void *data_p, jjs_context_data_key_t* key_p);

jjs_context_data_key_t jjs_context_data_key (jjs_context_t *context_p, const char *id_p);
jjs_status_t jjs_context_data_set (jjs_context_t *context_p, jjs_context_data_key_t key, void *data_p);
jjs_status_t jjs_context_data_get (jjs_context_t *context_p, jjs_context_data_key_t key, void **data_p);

jjs_value_t jjs_current_realm (jjs_context_t* context_p);
jjs_value_t jjs_set_realm (jjs_context_t* context_p, jjs_value_t realm);
/**
 * jjs-api-general-context @}
 */

/**
 * @defgroup jjs-api-general-heap Heap management
 * @{
 */
void *jjs_heap_alloc (jjs_context_t* context_p, jjs_size_t size);
void jjs_heap_free (jjs_context_t* context_p, void *mem_p, jjs_size_t size);

bool jjs_heap_stats (jjs_context_t* context_p, jjs_heap_stats_t *out_stats_p);
void jjs_heap_gc (jjs_context_t* context_p, jjs_gc_mode_t mode);

bool jjs_foreach_live_object (jjs_context_t* context_p, jjs_foreach_live_object_cb_t callback, void *user_data);
bool jjs_foreach_live_object_with_info (jjs_context_t* context_p,
                                        const jjs_object_native_info_t *native_info_p,
                                        jjs_foreach_live_object_with_info_cb_t callback,
                                        void *user_data_p);
/**
 * jjs-api-general-heap @}
 */

/**
 * @defgroup jjs-api-general-fmt fmt helper functions
 * @{
 */

void jjs_fmt_v (jjs_context_t *context_p,
                const jjs_wstream_t *wstream_p,
                const char *format_p,
                const jjs_value_t *values_p,
                jjs_size_t values_length);
jjs_value_t jjs_fmt_to_string_v (jjs_context_t *context_p,
                                 const char *format_p,
                                 const jjs_value_t *values_p,
                                 jjs_size_t values_length);
jjs_size_t jjs_fmt_to_buffer_v (jjs_context_t *context_p,
                                jjs_char_t *buffer_p,
                                jjs_size_t buffer_size,
                                jjs_encoding_t encoding,
                                const char *format_p,
                                const jjs_value_t *values_p,
                                jjs_size_t values_length);
jjs_value_t jjs_fmt_join_v (jjs_context_t *context_p,
                            jjs_value_t delimiter,
                            jjs_own_t delimiter_o,
                            const jjs_value_t *values_p,
                            jjs_size_t values_length);

jjs_value_t jjs_fmt_throw (jjs_context_t *context_p,
                           jjs_error_t error_type,
                           const char *format_p,
                           const jjs_value_t *values_p,
                           jjs_size_t values_length,
                           jjs_own_t values_o);

/**
 * jjs-api-general-fmt @}
 */

/**
 * @defgroup jjs-api-general-misc Miscellaneous
 * @{
 */

void jjs_log_set_level (jjs_context_t* context_p, jjs_log_level_t level);
jjs_log_level_t jjs_log_get_level (jjs_context_t* context_p);

void JJS_ATTR_FORMAT (printf, 3, 4) jjs_log (jjs_context_t* context_p, jjs_log_level_t level, const char *format_p, ...);
void jjs_log_fmt_v (jjs_context_t* context_p, jjs_log_level_t level, const char *format_p, const jjs_value_t *values, jjs_size_t values_length);

bool jjs_validate_string (jjs_context_t* context_p, const jjs_char_t *buffer_p, jjs_size_t buffer_size, jjs_encoding_t encoding);
bool JJS_ATTR_CONST jjs_feature_enabled (const jjs_feature_t feature);
void
jjs_register_magic_strings (jjs_context_t* context_p, const jjs_char_t *const *ext_strings_p, uint32_t count, const jjs_length_t *str_lengths_p);

jjs_optional_u32_t jjs_optional_u32 (uint32_t value);
jjs_optional_encoding_t jjs_optional_encoding (jjs_encoding_t encoding);
jjs_optional_value_t jjs_optional_value (jjs_value_t value);

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
jjs_value_t jjs_parse (jjs_context_t *context_p,
                       const jjs_char_t *source_p,
                       size_t source_size,
                       const jjs_parse_options_t *options_p);
jjs_value_t jjs_parse_sz (jjs_context_t *context_p,
                          const char *source_p,
                          const jjs_parse_options_t *options_p);
jjs_value_t jjs_parse_value (jjs_context_t *context_p,
                             const jjs_value_t source,
                             jjs_own_t source_o,
                             const jjs_parse_options_t *options_p);

jjs_parse_options_t jjs_parse_options (void);
void jjs_parse_options_disown (jjs_context_t *context_p, const jjs_parse_options_t *options_p);

/**
 * jjs-api-code-parse @}
 */

/**
 * @defgroup jjs-api-code-exec Execution
 * @{
 */
jjs_value_t jjs_eval (jjs_context_t* context_p, const jjs_char_t *source_p, size_t source_size, uint32_t flags);
jjs_value_t jjs_eval_sz (jjs_context_t* context_p, const char *source_p, uint32_t flags);
jjs_value_t jjs_run (jjs_context_t* context_p, const jjs_value_t script, jjs_own_t script_o);
jjs_value_t jjs_run_jobs (jjs_context_t* context_p);
jjs_value_t jjs_queue_microtask (jjs_context_t* context_p, const jjs_value_t callback, jjs_own_t callback_o);
jjs_value_t jjs_queue_microtask_fn (jjs_context_t* context_p, jjs_external_handler_t callback);
bool jjs_has_pending_jobs (jjs_context_t* context_p);
/**
 * jjs-api-code-exec @}
 */

/**
 * @defgroup jjs-api-code-sourceinfo Source information
 * @{
 */
jjs_value_t jjs_source_name (jjs_context_t* context_p, const jjs_value_t value);
jjs_value_t jjs_source_user_value (jjs_context_t* context_p, const jjs_value_t value);
jjs_source_info_t *jjs_source_info (jjs_context_t* context_p, const jjs_value_t value);
void jjs_source_info_free (jjs_context_t* context_p, jjs_source_info_t *source_info_p);
/**
 * jjs-api-code-sourceinfo @}
 */

/**
 * @defgroup jjs-api-code-cb Callbacks
 * @{
 */
void jjs_halt_handler (jjs_context_t* context_p, uint32_t interval, jjs_halt_cb_t callback, void *user_p);
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
jjs_value_t jjs_backtrace (jjs_context_t* context_p, uint32_t max_depth);
void jjs_backtrace_capture (jjs_context_t* context_p, jjs_backtrace_cb_t callback, void *user_p);
/**
 * jjs-api-backtrace-capture @}
 */

/**
 * @defgroup jjs-api-backtrace-frame Frames
 * @{
 */
jjs_frame_type_t jjs_frame_type (jjs_context_t* context_p, const jjs_frame_t *frame_p);
const jjs_value_t *jjs_frame_callee (jjs_context_t* context_p, jjs_frame_t *frame_p);
const jjs_value_t *jjs_frame_this (jjs_context_t* context_p, jjs_frame_t *frame_p);
const jjs_frame_location_t *jjs_frame_location (jjs_context_t* context_p, jjs_frame_t *frame_p);
bool jjs_frame_is_strict (jjs_context_t* context_p, jjs_frame_t *frame_p);
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
jjs_value_t JJS_ATTR_WARN_UNUSED_RESULT jjs_value_copy (jjs_context_t* context_p, const jjs_value_t value);
void jjs_value_free (jjs_context_t* context_p, jjs_value_t value);
void jjs_value_free_array (jjs_context_t* context_p, const jjs_value_t *values_p, jjs_size_t count);
bool jjs_value_free_unless (jjs_context_t* context_p, jjs_value_t value, jjs_value_condition_fn_t condition_fn);

/**
 * @defgroup jjs-api-value-checks Type inspection
 * @{
 */
jjs_type_t jjs_value_type (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_exception (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_abort (jjs_context_t* context_p, const jjs_value_t value);

bool jjs_value_is_undefined (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_null (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_boolean (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_true (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_false (jjs_context_t* context_p, const jjs_value_t value);

bool jjs_value_is_number (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_bigint (jjs_context_t* context_p, const jjs_value_t value);

bool jjs_value_is_string (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_symbol (jjs_context_t* context_p, const jjs_value_t value);

bool jjs_value_is_object (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_array (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_promise (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_proxy (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_arraybuffer (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_shared_arraybuffer (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_dataview (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_typedarray (jjs_context_t* context_p, const jjs_value_t value);

bool jjs_value_is_constructor (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_function (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_value_is_async_function (jjs_context_t* context_p, const jjs_value_t value);

bool jjs_value_is_error (jjs_context_t* context_p, const jjs_value_t value);
/**
 * jjs-api-value-checks @}
 */

/**
 * @defgroup jjs-api-value-coerce Coercion
 * @{
 */
bool jjs_value_to_boolean (jjs_context_t* context_p, const jjs_value_t value);
jjs_value_t jjs_value_to_number (jjs_context_t* context_p, const jjs_value_t value);
jjs_value_t jjs_value_to_object (jjs_context_t* context_p, const jjs_value_t value);
jjs_value_t jjs_value_to_primitive (jjs_context_t* context_p, const jjs_value_t value);
jjs_value_t jjs_value_to_string (jjs_context_t* context_p, const jjs_value_t value);
jjs_value_t jjs_value_to_bigint (jjs_context_t* context_p, const jjs_value_t value);

double jjs_value_as_number (jjs_context_t* context_p, const jjs_value_t value);
double jjs_value_as_integer (jjs_context_t* context_p, const jjs_value_t value);
int32_t jjs_value_as_int32 (jjs_context_t* context_p, const jjs_value_t value);
uint32_t jjs_value_as_uint32 (jjs_context_t* context_p, const jjs_value_t value);
float jjs_value_as_float (jjs_context_t* context_p, const jjs_value_t value);
double jjs_value_as_double (jjs_context_t* context_p, const jjs_value_t value);

/**
 * jjs-api-value-coerce @}
 */

/**
 * @defgroup jjs-api-value-op Operations
 * @{
 */
jjs_value_t jjs_binary_op (jjs_context_t* context_p,
                           jjs_binary_op_t operation,
                           const jjs_value_t lhs,
                           jjs_own_t lhs_o,
                           const jjs_value_t rhs,
                           jjs_own_t rhs_o);

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
jjs_value_t jjs_throw (jjs_context_t* context_p, jjs_error_t type, const jjs_value_t message, jjs_own_t message_o);
jjs_value_t jjs_throw_sz (jjs_context_t* context_p, jjs_error_t type, const char *message_p);
jjs_value_t jjs_throw_value (jjs_context_t* context_p, jjs_value_t value, jjs_own_t value_o);
jjs_value_t jjs_throw_abort (jjs_context_t* context_p, jjs_value_t value, jjs_own_t value_o);
/**
 * jjs-api-exception-ctor @}
 */

/**
 * @defgroup jjs-api-exception-op Operations
 * @{
 */
void jjs_exception_allow_capture (jjs_context_t* context_p, jjs_value_t value, bool allow_capture);
/**
 * jjs-api-exception-op @}
 */

/**
 * @defgroup jjs-api-exception-get Getters
 * @{
 */
jjs_value_t jjs_exception_value (jjs_context_t* context_p, jjs_value_t value, jjs_own_t value_o);
bool jjs_exception_is_captured (jjs_context_t* context_p, const jjs_value_t value);
/**
 * jjs-api-exception-get @}
 */

/**
 * @defgroup jjs-api-exception-cb Callbacks
 * @{
 */
void jjs_on_throw (jjs_context_t* context_p, jjs_throw_cb_t callback, void *user_p);
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

jjs_value_t JJS_ATTR_CONST jjs_undefined (jjs_context_t* context_p);

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

jjs_value_t JJS_ATTR_CONST jjs_null (jjs_context_t* context_p);

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

jjs_value_t JJS_ATTR_CONST jjs_boolean (jjs_context_t* context_p, bool value);

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

jjs_value_t jjs_number (jjs_context_t* context_p, double value);
jjs_value_t jjs_infinity (jjs_context_t* context_p, bool sign);
jjs_value_t jjs_nan (jjs_context_t* context_p);

jjs_value_t jjs_number_from_float (jjs_context_t* context_p, float value);
jjs_value_t jjs_number_from_double (jjs_context_t* context_p, double value);
jjs_value_t jjs_number_from_int32 (jjs_context_t* context_p, int32_t value);
jjs_value_t jjs_number_from_uint32 (jjs_context_t* context_p, uint32_t value);

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
jjs_value_t jjs_bigint (jjs_context_t* context_p, const uint64_t *digits_p, uint32_t digit_count, bool sign);
/**
 * jjs-api-bigint-ctor @}
 */

/**
 * @defgroup jjs-api-bigint-get Getters
 * @{
 */
uint32_t jjs_bigint_digit_count (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_bigint_sign (jjs_context_t* context_p, const jjs_value_t value);

/**
 * jjs-api-bigint-get @}
 */

/**
 * @defgroup jjs-api-bigint-op Operations
 * @{
 */
void jjs_bigint_to_digits (jjs_context_t* context_p, const jjs_value_t value, uint64_t *digits_p, uint32_t digit_count);

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
jjs_value_t jjs_string (jjs_context_t* context_p, const jjs_char_t *buffer_p, jjs_size_t buffer_size, jjs_encoding_t encoding);
jjs_value_t jjs_string_sz (jjs_context_t* context_p, const char *str_p);
jjs_value_t jjs_string_external (jjs_context_t* context_p, const jjs_char_t *buffer_p, jjs_size_t buffer_size, void *user_p);
jjs_value_t jjs_string_external_sz (jjs_context_t* context_p, const char *str_p, void *user_p);
jjs_value_t jjs_string_utf8_sz (jjs_context_t* context_p, const char *str_p);
jjs_value_t jjs_string_cesu8_sz (jjs_context_t* context_p, const char *str_p);

/**
 * jjs-api-string-cotr @}
 */

/**
 * @defgroup jjs-api-string-get Getters
 * @{
 */
jjs_size_t jjs_string_size (jjs_context_t* context_p, const jjs_value_t value, jjs_encoding_t encoding);
jjs_length_t jjs_string_length (jjs_context_t* context_p, const jjs_value_t value);
void *jjs_string_user_ptr (jjs_context_t* context_p, const jjs_value_t value, bool *is_external);
/**
 * jjs-api-string-get @}
 */

/**
 * @defgroup jjs-api-string-op Operations
 * @{
 */
jjs_value_t jjs_string_substr (jjs_context_t* context_p, const jjs_value_t value, jjs_length_t start, jjs_length_t end);
jjs_size_t jjs_string_to_buffer (jjs_context_t* context_p,
                                 const jjs_value_t value,
                                 jjs_encoding_t encoding,
                                 jjs_char_t *buffer_p,
                                 jjs_size_t buffer_size);
void jjs_string_iterate (jjs_context_t* context_p,
                         const jjs_value_t value,
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
void jjs_string_external_on_free (jjs_context_t* context_p, jjs_external_string_free_cb_t callback);
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
jjs_value_t jjs_symbol (jjs_context_t* context_p);
jjs_value_t jjs_symbol_with_description (jjs_context_t* context_p, const jjs_value_t value, jjs_own_t value_o);
jjs_value_t jjs_symbol_with_description_sz (jjs_context_t* context_p, const char *value_p);

jjs_value_t jjs_symbol_get_well_known (jjs_context_t* context_p, jjs_well_known_symbol_t symbol);
/**
 * jjs-api-symbol-ctor @}
 */

/**
 * @defgroup jjs-api-symbol-get Getters
 * @{
 */
jjs_value_t jjs_symbol_description (jjs_context_t* context_p, const jjs_value_t symbol);
jjs_value_t jjs_symbol_descriptive_string (jjs_context_t* context_p, const jjs_value_t symbol);
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
jjs_value_t jjs_object (jjs_context_t* context_p);
/**
 * jjs-api-object-ctor @}
 */

/**
 * @defgroup jjs-api-object-get Getters
 * @{
 */

jjs_object_type_t jjs_object_type (jjs_context_t* context_p, const jjs_value_t object);
jjs_value_t jjs_object_proto (jjs_context_t* context_p, const jjs_value_t object);
jjs_value_t jjs_object_keys (jjs_context_t* context_p, const jjs_value_t object);
jjs_value_t jjs_object_property_names (jjs_context_t* context_p, const jjs_value_t object, jjs_property_filter_t filter);

/**
 * jjs-api-object-get @}
 */

/**
 * @defgroup jjs-api-object-op Operations
 * @{
 */

jjs_value_t jjs_object_set_proto (jjs_context_t* context_p, jjs_value_t object, const jjs_value_t proto, jjs_own_t proto_o);
bool jjs_object_foreach (jjs_context_t* context_p, const jjs_value_t object, jjs_object_property_foreach_cb_t foreach_p, void *user_data_p);

/**
 * @defgroup jjs-api-object-op-set Set
 * @{
 */
jjs_value_t jjs_object_set (jjs_context_t* context_p, jjs_value_t object, const jjs_value_t key, const jjs_value_t value, jjs_own_t value_o);
jjs_value_t jjs_object_set_sz (jjs_context_t* context_p, jjs_value_t object, const char *key_p, const jjs_value_t value, jjs_own_t value_o);
jjs_value_t jjs_object_set_index (jjs_context_t* context_p, jjs_value_t object, uint32_t index, const jjs_value_t value, jjs_own_t value_o);
jjs_value_t jjs_object_define_own_prop (jjs_context_t* context_p,
                                        jjs_value_t object,
                                        const jjs_value_t key,
                                        const jjs_property_descriptor_t *prop_desc_p);
bool jjs_object_set_internal (jjs_context_t* context_p,
                              jjs_value_t object,
                              const jjs_value_t key,
                              const jjs_value_t value,
                              jjs_own_t value_o);
bool jjs_object_set_internal_sz (jjs_context_t* context_p,
                                 jjs_value_t object,
                                 const char *key_p,
                                 const jjs_value_t value,
                                 jjs_own_t value_o);
void jjs_object_set_native_ptr (jjs_context_t* context_p,
                                jjs_value_t object,
                                const jjs_object_native_info_t *native_info_p,
                                void *native_pointer_p);
/**
 * jjs-api-object-op-set @}
 */

/**
 * @defgroup jjs-api-object-op-has Has
 * @{
 */
jjs_value_t jjs_object_has (jjs_context_t* context_p, const jjs_value_t object, const jjs_value_t key);
jjs_value_t jjs_object_has_sz (jjs_context_t* context_p, const jjs_value_t object, const char *key_p);
jjs_value_t jjs_object_has_own (jjs_context_t* context_p, const jjs_value_t object, const jjs_value_t key);
jjs_value_t jjs_object_has_own_sz (jjs_context_t* context_p, const jjs_value_t object, const char *key_p);

bool jjs_object_has_internal (jjs_context_t* context_p, const jjs_value_t object, const jjs_value_t key);
bool jjs_object_has_internal_sz (jjs_context_t* context_p, const jjs_value_t object, const char *key_p);

bool jjs_object_has_native_ptr (jjs_context_t* context_p, const jjs_value_t object, const jjs_object_native_info_t *native_info_p);
/**
 * jjs-api-object-op-has @}
 */

/**
 * @defgroup jjs-api-object-op-get Get
 * @{
 */
jjs_value_t jjs_object_get (jjs_context_t* context_p, const jjs_value_t object, const jjs_value_t key);
jjs_value_t jjs_object_get_sz (jjs_context_t* context_p, const jjs_value_t object, const char *key_p);
jjs_value_t jjs_object_get_index (jjs_context_t* context_p, const jjs_value_t object, uint32_t index);

jjs_value_t jjs_object_get_own_prop (jjs_context_t* context_p,
                                     const jjs_value_t object,
                                     const jjs_value_t key,
                                     jjs_property_descriptor_t *prop_desc_p);

jjs_value_t jjs_object_get_internal (jjs_context_t* context_p, const jjs_value_t object, const jjs_value_t key);
jjs_value_t jjs_object_get_internal_sz (jjs_context_t* context_p, const jjs_value_t object, const char *key_p);

void *jjs_object_get_native_ptr (jjs_context_t* context_p, const jjs_value_t object, const jjs_object_native_info_t *native_info_p);

jjs_value_t jjs_object_find_own (jjs_context_t* context_p,
                                 const jjs_value_t object,
                                 const jjs_value_t key,
                                 const jjs_value_t receiver,
                                 jjs_own_t receiver_o,
                                 bool *found_p);
jjs_value_t jjs_object_find_own_sz (jjs_context_t* context_p,
                                    const jjs_value_t object,
                                    const char *key_p,
                                    const jjs_value_t receiver,
                                    jjs_own_t receiver_o,
                                    bool *found_p);
/**
 * jjs-api-object-op-get @}
 */

/**
 * @defgroup jjs-api-object-op-del Delete
 * @{
 */
jjs_value_t jjs_object_delete (jjs_context_t* context_p, jjs_value_t object, const jjs_value_t key);
jjs_value_t jjs_object_delete_sz (jjs_context_t* context_p, const jjs_value_t object, const char *key_p);
jjs_value_t jjs_object_delete_index (jjs_context_t* context_p, jjs_value_t object, uint32_t index);

bool jjs_object_delete_internal (jjs_context_t* context_p, jjs_value_t object, const jjs_value_t key);
bool jjs_object_delete_internal_sz (jjs_context_t* context_p, jjs_value_t object, const char *key_p);

bool jjs_object_delete_native_ptr (jjs_context_t* context_p, jjs_value_t object, const jjs_object_native_info_t *native_info_p);
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
jjs_value_t jjs_property_descriptor_from_object (jjs_context_t* context_p,
                                                 const jjs_value_t obj_value,
                                                 jjs_property_descriptor_t *out_prop_desc_p);
/**
 * jjs-api-object-prop-desc-ctor @}
 */

/**
 * @defgroup jjs-api-object-prop-desc-op Operations
 * @{
 */
void jjs_property_descriptor_free (jjs_context_t* context_p, jjs_property_descriptor_t *prop_desc_p);
jjs_value_t jjs_property_descriptor_to_object (jjs_context_t* context_p, const jjs_property_descriptor_t *src_prop_desc_p);
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
void jjs_native_ptr_init (jjs_context_t* context_p, void *native_pointer_p, const jjs_object_native_info_t *native_info_p);
void jjs_native_ptr_free (jjs_context_t* context_p, void *native_pointer_p, const jjs_object_native_info_t *native_info_p);
void jjs_native_ptr_set (jjs_context_t* context_p, jjs_value_t *reference_p, const jjs_value_t value);
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
jjs_value_t jjs_array (jjs_context_t* context_p, jjs_length_t length);
/**
 * jjs-api-array-ctor @}
 */

/**
 * @defgroup jjs-api-array-get Getters
 * @{
 */
jjs_length_t jjs_array_length (jjs_context_t* context_p, const jjs_value_t value);
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
jjs_value_t jjs_arraybuffer (jjs_context_t* context_p, const jjs_length_t size);
jjs_value_t jjs_arraybuffer_external (jjs_context_t* context_p, uint8_t *buffer_p, jjs_length_t size, void *user_p);
/**
 * jjs-api-arraybuffer-ctor @}
 */

/**
 * @defgroup jjs-api-arraybuffer-get Getters
 * @{
 */
jjs_size_t jjs_arraybuffer_size (jjs_context_t* context_p, const jjs_value_t value);
uint8_t *jjs_arraybuffer_data (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_arraybuffer_is_detachable (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_arraybuffer_has_buffer (jjs_context_t* context_p, const jjs_value_t value);
/**
 * jjs-api-arraybuffer-get @}
 */

/**
 * @defgroup jjs-api-arraybuffer-op Operations
 * @{
 */
jjs_size_t
jjs_arraybuffer_read (jjs_context_t* context_p, const jjs_value_t value, jjs_size_t offset, uint8_t *buffer_p, jjs_size_t buffer_size);
jjs_size_t
jjs_arraybuffer_write (jjs_context_t* context_p, jjs_value_t value, jjs_size_t offset, const uint8_t *buffer_p, jjs_size_t buffer_size);
jjs_value_t jjs_arraybuffer_detach (jjs_context_t* context_p, jjs_value_t value);
void jjs_arraybuffer_heap_allocation_limit (jjs_context_t* context_p, jjs_size_t limit);
/**
 * jjs-api-arraybuffer-op @}
 */

/**
 * @defgroup jjs-api-arraybuffer-cb Callbacks
 * @{
 */
void jjs_arraybuffer_allocator (jjs_context_t* context_p,
                                jjs_arraybuffer_allocate_cb_t allocate_callback,
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
jjs_value_t jjs_shared_arraybuffer (jjs_context_t* context_p, jjs_size_t size);
jjs_value_t jjs_shared_arraybuffer_external (jjs_context_t* context_p, uint8_t *buffer_p, jjs_size_t buffer_size, void *user_p);
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
jjs_value_t jjs_dataview (jjs_context_t* context_p,
                          const jjs_value_t array_buffer,
                          jjs_own_t array_buffer_o,
                          jjs_size_t byte_offset,
                          jjs_size_t byte_length);
/**
 * jjs-api-dataview-ctr @}
 */

/**
 * @defgroup jjs-api-dataview-get Getters
 * @{
 */
jjs_value_t jjs_dataview_buffer (jjs_context_t* context_p, const jjs_value_t dataview, jjs_size_t *byte_offset, jjs_size_t *byte_length);
jjs_size_t jjs_dataview_byte_offset (jjs_context_t* context_p, const jjs_value_t dataview);
jjs_size_t jjs_dataview_byte_length (jjs_context_t* context_p, const jjs_value_t dataview);

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
jjs_value_t jjs_typedarray (jjs_context_t* context_p, jjs_typedarray_type_t type, jjs_length_t length);
jjs_value_t jjs_typedarray_with_buffer (jjs_context_t* context_p,
                                        jjs_typedarray_type_t type,
                                        const jjs_value_t arraybuffer,
                                        jjs_own_t arraybuffer_o);
jjs_value_t jjs_typedarray_with_buffer_span (jjs_context_t* context_p,
                                             jjs_typedarray_type_t type,
                                             const jjs_value_t arraybuffer,
                                             jjs_own_t arraybuffer_o,
                                             jjs_size_t byte_offset,
                                             jjs_size_t byte_length);
/**
 * jjs-api-typedarray-ctor @}
 */

/**
 * @defgroup jjs-api-typedarray-get Getters
 * @{
 */
jjs_typedarray_type_t jjs_typedarray_type (jjs_context_t* context_p, const jjs_value_t value);
jjs_length_t jjs_typedarray_length (jjs_context_t* context_p, const jjs_value_t value);
jjs_length_t jjs_typedarray_offset (jjs_context_t* context_p, const jjs_value_t value);
jjs_value_t jjs_typedarray_buffer (jjs_context_t* context_p, const jjs_value_t value, jjs_size_t *byte_offset, jjs_size_t *byte_length);
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
jjs_iterator_type_t jjs_iterator_type (jjs_context_t* context_p, const jjs_value_t value);
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
jjs_value_t jjs_function_external (jjs_context_t* context_p, jjs_external_handler_t handler);
/**
 * jjs-api-function-ctor @}
 */

/**
 * @defgroup jjs-api-function-get Getters
 * @{
 */
jjs_function_type_t jjs_function_type (jjs_context_t* context_p, const jjs_value_t value);
bool jjs_function_is_dynamic (jjs_context_t* context_p, const jjs_value_t value);
/**
 * jjs-api-function-get @}
 */

/**
 * @defgroup jjs-api-function-op Operations
 * @{
 */
jjs_value_t jjs_call (jjs_context_t* context_p,
                      const jjs_value_t function,
                      const jjs_value_t *args_p,
                      jjs_size_t args_count,
                      jjs_own_t args_o);
jjs_value_t jjs_call_noargs (jjs_context_t* context_p,
                             const jjs_value_t function);
jjs_value_t jjs_call_this (jjs_context_t* context_p,
                           const jjs_value_t function,
                           const jjs_value_t this_value,
                           jjs_own_t this_value_o,
                           const jjs_value_t *args_p,
                           jjs_size_t args_count,
                           jjs_own_t args_o);
jjs_value_t jjs_call_this_noargs (jjs_context_t* context_p,
                                  const jjs_value_t function,
                                  const jjs_value_t this_value,
                                  jjs_own_t this_value_o);

jjs_value_t jjs_construct (jjs_context_t* context_p,
                           const jjs_value_t function,
                           const jjs_value_t *args_p,
                           jjs_size_t args_count,
                           jjs_own_t args_o);
jjs_value_t jjs_construct_noargs (jjs_context_t* context_p, const jjs_value_t function);

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
jjs_value_t jjs_proxy (jjs_context_t* context_p,
                       const jjs_value_t target,
                       jjs_own_t target_o,
                       const jjs_value_t handler,
                       jjs_own_t handler_o);
jjs_value_t jjs_proxy_custom (jjs_context_t* context_p,
                              const jjs_value_t target,
                              jjs_own_t target_o,
                              const jjs_value_t handler,
                              jjs_own_t handler_o,
                              uint32_t flags);
/**
 * jjs-api-function-proxy-ctor @}
 */

/**
 * @defgroup jjs-api-proxy-get Getters
 * @{
 */
jjs_value_t jjs_proxy_target (jjs_context_t* context_p, const jjs_value_t value);
jjs_value_t jjs_proxy_handler (jjs_context_t* context_p, const jjs_value_t value);
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
jjs_value_t jjs_promise (jjs_context_t* context_p);
/**
 * jjs-api-promise-ctor @}
 */

/**
 * @defgroup jjs-api-promise-get Getters
 * @{
 */
jjs_value_t jjs_promise_result (jjs_context_t* context_p, const jjs_value_t promise);
jjs_promise_state_t jjs_promise_state (jjs_context_t* context_p, const jjs_value_t promise);
/**
 * jjs-api-promise-get @}
 */

/**
 * @defgroup jjs-api-promise-op Operations
 * @{
 */
jjs_value_t jjs_promise_resolve (jjs_context_t* context_p, jjs_value_t promise, const jjs_value_t argument, jjs_own_t argument_o);
jjs_value_t jjs_promise_reject (jjs_context_t* context_p, jjs_value_t promise, const jjs_value_t argument, jjs_own_t argument_o);
/**
 * jjs-api-promise-op @}
 */

/**
 * @defgroup jjs-api-promise-cb Callbacks
 * @{
 */
void jjs_promise_on_event (jjs_context_t* context_p, jjs_promise_event_filter_t filters, jjs_promise_event_cb_t callback, void *user_p);
void jjs_promise_on_unhandled_rejection (jjs_context_t *context_p, jjs_promise_unhandled_rejection_cb_t callback, void *user_p);

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
jjs_value_t jjs_container (jjs_context_t* context_p,
                           jjs_container_type_t container_type,
                           const jjs_value_t *arguments_p,
                           jjs_length_t argument_count,
                           jjs_own_t arguments_list_o);
jjs_value_t jjs_container_noargs (jjs_context_t* context_p, jjs_container_type_t container_type);

/**
 * jjs-api-promise-ctor @}
 */

/**
 * @defgroup jjs-api-container-get Getters
 * @{
 */
jjs_container_type_t jjs_container_type (jjs_context_t* context_p, const jjs_value_t value);
/**
 * jjs-api-container-get @}
 */

/**
 * @defgroup jjs-api-container-op Operations
 * @{
 */
jjs_value_t jjs_container_to_array (jjs_context_t* context_p, const jjs_value_t value, bool *is_key_value_p);
jjs_value_t jjs_container_op (jjs_context_t* context_p,
                              jjs_container_op_t operation,
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
jjs_value_t jjs_regexp (jjs_context_t* context_p, const jjs_value_t pattern, jjs_own_t pattern_o, uint16_t flags);
jjs_value_t jjs_regexp_sz (jjs_context_t* context_p, const char *pattern_p, uint16_t flags);
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
jjs_value_t jjs_error (jjs_context_t* context_p,
                       jjs_error_t type,
                       const jjs_value_t message,
                       jjs_own_t message_o,
                       const jjs_value_t options,
                       jjs_own_t options_o);
jjs_value_t jjs_error_sz (jjs_context_t* context_p,
                          jjs_error_t type,
                          const char *message_p,
                          const jjs_value_t options,
                          jjs_own_t options_o);
/**
 * jjs-api-error-ctor @}
 */

/**
 * @defgroup jjs-api-error-get Getters
 * @{
 */
jjs_error_t jjs_error_type (jjs_context_t* context_p, jjs_value_t value);
/**
 * jjs-api-error-get @}
 */

/**
 * @defgroup jjs-api-error-cb Callbacks
 * @{
 */
void jjs_error_on_created (jjs_context_t* context_p, jjs_error_object_created_cb_t callback, void *user_p);
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
jjs_value_t jjs_aggregate_error (jjs_context_t* context_p,
                                 const jjs_value_t errors,
                                 jjs_own_t errors_o,
                                 const jjs_value_t message,
                                 jjs_own_t message_o,
                                 const jjs_value_t options,
                                 jjs_own_t options_o);
jjs_value_t jjs_aggregate_error_sz (jjs_context_t* context_p,
                                    const jjs_value_t errors,
                                    jjs_own_t errors_o,
                                    const char *message_p,
                                    const jjs_value_t options,
                                    jjs_own_t options_o);
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

jjs_value_t jjs_json_parse (jjs_context_t* context_p, const jjs_char_t *string_p, jjs_size_t string_size);
jjs_value_t jjs_json_parse_sz (jjs_context_t* context_p, const char* string_p);
jjs_value_t jjs_json_parse_file (jjs_context_t* context_p, jjs_value_t filename, jjs_own_t filename_o);
jjs_value_t jjs_json_stringify (jjs_context_t* context_p, const jjs_value_t value, jjs_own_t value_o);

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
jjs_module_state_t jjs_module_state (jjs_context_t* context_p, const jjs_value_t module);
size_t jjs_module_request_count (jjs_context_t* context_p, const jjs_value_t module);
jjs_value_t jjs_module_request (jjs_context_t* context_p, const jjs_value_t module, size_t request_index);
jjs_value_t jjs_module_namespace (jjs_context_t* context_p, const jjs_value_t module);
/**
 * jjs-api-module-get @}
 */

/**
 * @defgroup jjs-api-module-op Operations
 * @{
 */

jjs_value_t jjs_module_link (jjs_context_t* context_p, const jjs_value_t module, jjs_module_link_cb_t callback, void *user_p);
jjs_value_t jjs_module_evaluate (jjs_context_t* context_p, const jjs_value_t module);

/**
 * jjs-api-module-op @}
 */

/**
 * @defgroup jjs-api-synthetic Synthetic modules
 * @{
 */

jjs_value_t jjs_synthetic_module (jjs_context_t* context_p,
                                  jjs_synthetic_module_evaluate_cb_t callback,
                                  const jjs_value_t *const exports_p,
                                  jjs_size_t export_count,
                                  jjs_own_t exports_o);
jjs_value_t
jjs_synthetic_module_set_export (jjs_context_t* context_p,
                                 jjs_value_t module,
                                 const jjs_value_t export_name,
                                 const jjs_value_t value,
                                 jjs_own_t value_o);
jjs_value_t
jjs_synthetic_module_set_export_sz (jjs_context_t* context_p,
                                    jjs_value_t module,
                                    const char *export_name,
                                    const jjs_value_t value,
                                    jjs_own_t value_o);

/**
 * jjs-api-synthetic-module @}
 */

/**
 * @defgroup jjs-api-module-cb Callbacks
 * @{
 */
void jjs_module_on_state_changed (jjs_context_t* context_p, jjs_module_state_changed_cb_t callback, void *user_p);
void jjs_module_on_import_meta (jjs_context_t* context_p, jjs_module_import_meta_cb_t callback, void *user_p);
void jjs_module_on_import (jjs_context_t* context_p, jjs_module_import_cb_t callback, void *user_p);

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

jjs_value_t jjs_pmap (jjs_context_t* context_p, jjs_value_t pmap, jjs_own_t pmap_o, jjs_value_t root, jjs_own_t root_o);

jjs_value_t jjs_pmap_resolve (jjs_context_t* context_p, jjs_value_t specifier, jjs_own_t specifier_o, jjs_module_type_t module_type);
jjs_value_t jjs_pmap_resolve_sz (jjs_context_t* context_p, const char* specifier_p, jjs_module_type_t module_type);

/**
 * jjs-pmap-ops @}
 */

/**
 * jjs-platform @}
 */

/**
 * @defgroup jjs-platform Platform API
 * @{
 */

/**
 * @defgroup jjs-platform-ops Operations
 * @{
 */

jjs_value_t jjs_platform_cwd (jjs_context_t* context_p);
bool jjs_platform_has_cwd (jjs_context_t* context_p);

jjs_value_t jjs_platform_realpath (jjs_context_t* context_p, jjs_value_t path, jjs_own_t path_o);
jjs_value_t jjs_platform_realpath_sz (jjs_context_t* context_p, const char* path_p);
bool jjs_platform_has_realpath (jjs_context_t* context_p);

jjs_value_t jjs_platform_read_file (jjs_context_t* context_p, jjs_value_t path, jjs_own_t path_o, const jjs_platform_read_file_options_t* opts);
jjs_value_t jjs_platform_read_file_sz (jjs_context_t* context_p, const char* path_p, const jjs_platform_read_file_options_t* opts);
bool jjs_platform_has_read_file (jjs_context_t* context_p);

void jjs_platform_stdout_write (jjs_context_t* context_p, jjs_value_t value, jjs_own_t value_o);
void jjs_platform_stdout_flush (jjs_context_t* context_p);
bool jjs_platform_has_stdout (jjs_context_t* context_p);

void jjs_platform_stderr_write (jjs_context_t* context_p, jjs_value_t value, jjs_own_t value_o);
void jjs_platform_stderr_flush (jjs_context_t* context_p);
bool jjs_platform_has_stderr (jjs_context_t* context_p);

jjs_value_t jjs_platform_os (jjs_context_t* context_p);
jjs_platform_os_t JJS_ATTR_CONST jjs_platform_os_type (void);
jjs_value_t jjs_platform_arch (jjs_context_t* context_p);
jjs_platform_arch_t JJS_ATTR_CONST jjs_platform_arch_type (void);

void jjs_platform_fatal (jjs_context_t* context_p, jjs_fatal_code_t code);

/**
 * jjs-platform-ops @}
 */

/**
 * jjs-platform @}
 */

/**
 * @defgroup jjs-commonjs CommonJS
 * @{
 */

/**
 * @defgroup jjs-commonjs-ops Operations
 * @{
 */

jjs_value_t jjs_commonjs_require (jjs_context_t* context_p, jjs_value_t specifier, jjs_own_t specifier_o);
jjs_value_t jjs_commonjs_require_sz (jjs_context_t* context_p, const char* specifier_p);

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

jjs_value_t jjs_esm_import (jjs_context_t* context_p, jjs_value_t specifier, jjs_own_t specifier_o);
jjs_value_t jjs_esm_import_sz (jjs_context_t* context_p, const char* specifier_p);

jjs_value_t jjs_esm_import_source (jjs_context_t* context_p, const jjs_char_t *buffer_p, jjs_size_t buffer_size, const jjs_esm_source_options_t *options_p);
jjs_value_t jjs_esm_import_source_sz (jjs_context_t* context_p, const char *source_p, const jjs_esm_source_options_t *options_p);
jjs_value_t jjs_esm_import_source_value (jjs_context_t* context_p, jjs_value_t source, jjs_own_t source_o, const jjs_esm_source_options_t *options_p);

jjs_value_t jjs_esm_evaluate (jjs_context_t* context_p, jjs_value_t specifier, jjs_own_t specifier_o);
jjs_value_t jjs_esm_evaluate_sz (jjs_context_t* context_p, const char* specifier_p);

jjs_value_t jjs_esm_evaluate_source (jjs_context_t* context_p, const jjs_char_t *buffer_p, jjs_size_t buffer_size, const jjs_esm_source_options_t *options_p);
jjs_value_t jjs_esm_evaluate_source_sz (jjs_context_t* context_p, const char *source_p, const jjs_esm_source_options_t *options_p);
jjs_value_t jjs_esm_evaluate_source_value (jjs_context_t* context_p, jjs_value_t source, jjs_own_t source_o, const jjs_esm_source_options_t *options_p);

jjs_esm_source_options_t jjs_esm_source_options (void);
void jjs_esm_source_options_disown (jjs_context_t *context_p, const jjs_esm_source_options_t* options_p);

void jjs_esm_on_load (jjs_context_t* context_p, jjs_esm_load_cb_t callback_p, void *user_p);
void jjs_esm_on_resolve (jjs_context_t* context_p, jjs_esm_resolve_cb_t callback_p, void *user_p);

jjs_value_t jjs_esm_default_on_load_cb (jjs_context_t* context_p, jjs_value_t path, jjs_esm_load_context_t * load_context_p, void *user_p);
jjs_value_t jjs_esm_default_on_resolve_cb (jjs_context_t* context_p, jjs_value_t specifier, jjs_esm_resolve_context_t * resolve_context_p, void *user_p);
jjs_value_t jjs_esm_default_on_import_cb (jjs_context_t* context_p, jjs_value_t specifier, jjs_value_t user_value, void *user_p);
void jjs_esm_default_on_import_meta_cb (jjs_context_t* context_p, jjs_value_t module, jjs_value_t meta_object, void *user_p);

/**
 * jjs-esm-ops @}
 */

/**
 * jjs-esm @}
 */

/**
 * @defgroup jjs-vmod Virtual Modules
 * @{
 */

/**
 * @defgroup jjs-vmod-ops Operations
 * @{
 */

jjs_value_t jjs_vmod (jjs_context_t* context_p, jjs_value_t name, jjs_own_t name_o, jjs_value_t value, jjs_own_t value_o);
jjs_value_t jjs_vmod_sz (jjs_context_t* context_p, const char* name, jjs_value_t value, jjs_own_t value_o);

jjs_value_t jjs_vmod_resolve (jjs_context_t* context_p, jjs_value_t name, jjs_own_t name_o);
jjs_value_t jjs_vmod_resolve_sz (jjs_context_t* context_p, const char* name);

bool jjs_vmod_exists (jjs_context_t* context_p, jjs_value_t name, jjs_own_t name_o);
bool jjs_vmod_exists_sz (jjs_context_t* context_p, const char* name);

void jjs_vmod_remove (jjs_context_t* context_p, jjs_value_t name, jjs_own_t name_o);
void jjs_vmod_remove_sz (jjs_context_t* context_p, const char* name);

/**
 * jjs-vmod-ops @}
 */

/**
 * jjs-vmod @}
 */

/**
 * @defgroup jjs-api-realm Realms
 * @{
 */

/**
 * @defgroup jjs-api-realm-ctor Constructors
 * @{
 */
jjs_value_t jjs_realm (jjs_context_t* context_p);
/**
 * jjs-api-realm-ctor @}
 */

/**
 * @defgroup jjs-api-realm-get Getters
 * @{
 */
jjs_value_t jjs_realm_this (jjs_context_t* context_p, jjs_value_t realm);
/**
 * jjs-api-realm-ctor @}
 */

/**
 * @defgroup jjs-api-realm-op Operation
 * @{
 */
jjs_value_t jjs_realm_set_this (jjs_context_t* context_p, jjs_value_t realm, jjs_value_t this_value, jjs_own_t this_value_o);
/**
 * jjs-api-realm-op @}
 */

/**
 * jjs-api-realm @}
 */

void* jjs_allocator_alloc (const jjs_allocator_t *allocator_p, jjs_size_t size);
void jjs_allocator_free (const jjs_allocator_t *allocator_p, void *chunk_p, jjs_size_t size);

/**
 * jjs-api @}
 */

/**
 * Helper macro for using jjs_log_fmt_v.
 */
#define jjs_log_fmt(CTX, LEVEL, FORMAT, ...)                                                           \
  do                                                                                                   \
  {                                                                                                    \
    jjs_value_t args__[] = { __VA_ARGS__ };                                                            \
    jjs_log_fmt_v ((CTX), LEVEL, FORMAT, args__, (jjs_size_t) (sizeof (args__) / sizeof (args__[0]))); \
  } while (false)

JJS_C_API_END

#endif /* !JJS_CORE_H */

/* vim: set fdm=marker fmr=@{,@}: */
