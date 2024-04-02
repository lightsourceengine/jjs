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
