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

#ifndef JJS_SNAPSHOT_H
#define JJS_SNAPSHOT_H

#include "jjs-types.h"

JJS_C_API_BEGIN

/** \addtogroup jjs-snapshot JJS engine interface - Snapshot feature
 * @{
 */

/**
 * JJS snapshot format version.
 */
#define JJS_SNAPSHOT_VERSION (71u)

/**
 * Flags for jjs_generate_snapshot and jjs_generate_function_snapshot.
 */
typedef enum
{
  JJS_SNAPSHOT_SAVE_STATIC = (1u << 0), /**< static snapshot */
} jjs_generate_snapshot_opts_t;

/**
 * Flags for jjs_exec_snapshot.
 */
typedef enum
{
  JJS_SNAPSHOT_EXEC_COPY_DATA = (1u << 0), /**< copy snashot data */
  JJS_SNAPSHOT_EXEC_ALLOW_STATIC = (1u << 1), /**< static snapshots allowed */
  JJS_SNAPSHOT_EXEC_LOAD_AS_FUNCTION = (1u << 2), /**< load snapshot as function instead of executing it */
  JJS_SNAPSHOT_EXEC_HAS_SOURCE_NAME = (1u << 3), /**< source_name field is valid
                                                    *   in jjs_exec_snapshot_option_values_t */
  JJS_SNAPSHOT_EXEC_HAS_USER_VALUE = (1u << 4), /**< user_value field is valid
                                                   *   in jjs_exec_snapshot_option_values_t */
} jjs_exec_snapshot_opts_t;

/**
 * Various configuration options for jjs_exec_snapshot.
 */
typedef struct
{
  jjs_value_t source_name; /**< source name string (usually a file name)
                              *   if JJS_SNAPSHOT_EXEC_HAS_SOURCE_NAME is set in exec_snapshot_opts
                              *   Note: non-string values are ignored */
  jjs_value_t user_value; /**< user value assigned to all functions created by this script including
                             *   eval calls executed by the script if JJS_SNAPSHOT_EXEC_HAS_USER_VALUE
                             *   is set in exec_snapshot_opts */
} jjs_exec_snapshot_option_values_t;

/**
 * Snapshot functions.
 */
jjs_value_t jjs_generate_snapshot (jjs_context_t* context_p,
                                   jjs_value_t compiled_code,
                                   uint32_t generate_snapshot_opts,
                                   uint32_t *buffer_p,
                                   size_t buffer_size);

jjs_value_t jjs_exec_snapshot (jjs_context_t* context_p,
                               const uint32_t *snapshot_p,
                               size_t snapshot_size,
                               size_t func_index,
                               uint32_t exec_snapshot_opts,
                               const jjs_exec_snapshot_option_values_t *options_values_p);

size_t jjs_merge_snapshots (jjs_context_t* context_p,
                            const uint32_t **inp_buffers_p,
                            size_t *inp_buffer_sizes_p,
                            size_t number_of_snapshots,
                            uint32_t *out_buffer_p,
                            size_t out_buffer_size,
                            const char **error_p);
size_t jjs_get_literals_from_snapshot (jjs_context_t* context_p,
                                       const uint32_t *snapshot_p,
                                       size_t snapshot_size,
                                       jjs_char_t *lit_buf_p,
                                       size_t lit_buf_size,
                                       bool is_c_format);
/**
 * @}
 */

JJS_C_API_END

#endif /* !JJS_SNAPSHOT_H */
