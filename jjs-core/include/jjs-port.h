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

#ifndef JJS_PORT_H
#define JJS_PORT_H

#include "jjs-types.h"

JJS_C_API_BEGIN

/**
 * @defgroup jjs-port JJS Port API
 * @{
 */

/**
 * @defgroup jjs-port-process Process management API
 *
 * It is questionable whether a library should be able to terminate an
 * application. However, as of now, we only have the concept of completion
 * code around jjs_parse and jjs_run. Most of the other API functions
 * have no way of signaling an error. So, we keep the termination approach
 * with this port function.
 *
 * @{
 */

/**
 * Error codes that can be passed by the engine when calling jjs_port_fatal
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
 * Signal the port that the process experienced a fatal failure from which it cannot
 * recover.
 *
 * A libc-based port may implement this with exit() or abort(), or both.
 *
 * Note: This function is expected to not return.
 *
 * @param code: the cause of the error.
 */
void JJS_ATTR_NORETURN jjs_port_fatal (jjs_fatal_code_t code);

/**
 * jjs-port-process @}
 */

/**
 * @defgroup jjs-port-io I/O API
 * @{
 */

/**
 * Print a single character to standard output.
 *
 * This port function is never called from jjs-core directly, it is only used by jjs-ext components to print
 * information.
 *
 * @param byte: the byte to print.
 */
void jjs_port_print_byte (jjs_char_t byte);

/**
 * Print a buffer to standard output
 *
 * This port function is never called from jjs-core directly, it is only used by jjs-ext components to print
 * information.
 *
 * @param buffer_p: input buffer
 * @param buffer_size: data size
 */
void jjs_port_print_buffer (const jjs_char_t *buffer_p, jjs_size_t buffer_size);

/**
 * jjs-port-io @}
 */

/**
 * @defgroup jjs-port-fd Filesystem API
 * @{
 */

/**
 * Canonicalize a file path.
 *
 * If possible, the implementation should resolve symbolic links and other directory references found in the input path,
 * and create a fully canonicalized file path as the result.
 *
 * The function may return with NULL in case an error is encountered, in which case the calling operation will not
 * proceed.
 *
 * The implementation should allocate storage for the result path as necessary. Non-NULL return values will be passed
 * to `jjs_port_path_free` when the result is no longer needed by the caller, which can be used to finalize
 * dynamically allocated buffers.
 *
 * NOTE: The implementation must not return directly with the input, as the input buffer is released after the call.
 *
 * @param path_p: zero-terminated string containing the input path
 * @param path_size: size of the input path string in bytes, excluding terminating zero
 *
 * @return buffer with the normalized path if the operation is successful,
 *         NULL otherwise
 */
jjs_char_t *jjs_port_path_normalize (const jjs_char_t *path_p, jjs_size_t path_size);

/**
 * Gets the dirname of the given path.
 *
 * @param path_p the path. if null of empty string, "." is returned.
 * @param dirname_size_p out value for the size of the returned string. if NULL, the
 * size is ignored.
 * @return dirname of the given path. on error, "." is returned. return value must be
 * freed with jjs_port_path_free.
 */
jjs_char_t *jjs_port_path_dirname (const char* path_p, jjs_size_t* dirname_size_p);

/**
 * Free a path buffer returned by jjs_port_path_normalize.
 *
 * @param path_p: the path buffer to free
 */
void jjs_port_path_free (jjs_char_t *path_p);

/**
 * Open a source file and read the content into a buffer.
 *
 * When the source file is no longer needed by the caller, the returned pointer will be passed to
 * `jjs_port_source_free`, which can be used to finalize the buffer.
 *
 * @param file_name_p: Path that points to the source file in the filesystem.
 * @param out_size_p: The opened file's size in bytes.
 *
 * @return pointer to the buffer which contains the content of the file.
 */
jjs_char_t *jjs_port_source_read (const char *file_name_p, jjs_size_t *out_size_p);

/**
 * Free a source file buffer.
 *
 * @param buffer_p: buffer returned by jjs_port_source_read
 */
void jjs_port_source_free (jjs_char_t *buffer_p);

/**
 * jjs-port-fs @}
 */

/**
 * jjs-port @}
 */

JJS_C_API_END

#endif /* !JJS_PORT_H */

/* vim: set fdm=marker fmr=@{,@}: */
