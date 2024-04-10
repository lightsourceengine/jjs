/* Copyright Light Source Software, LLC and other contributors.
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

#ifndef JJS_STREAM_H
#define JJS_STREAM_H

#include "jjs.h"

struct ecma_stringbuilder_t;

/**
 * Writable Stream in-memory buffer state.
 */
typedef struct
{
  uint8_t *buffer; /**< buffer to write to */
  jjs_size_t buffer_size; /**< size of the buffer */
  jjs_size_t buffer_index; /**< write position in buffer. this is also the # of bytes written */
} jjs_wstream_buffer_state_t;

/**
 * Platform stream IDs.
 */
typedef enum
{
  JJS_STDOUT, /**< platform io_stdout from context */
  JJS_STDERR, /**< platform io_stderr from context */
} jjs_std_stream_id_t;

bool jjs_wstream_new (jjs_std_stream_id_t id, jjs_value_t* out);

bool jjs_wstream_from_buffer (jjs_wstream_buffer_state_t* buffer_p, jjs_encoding_t encoding, jjs_wstream_t* out);
bool jjs_wstream_from_id (jjs_std_stream_id_t id, jjs_wstream_t* out);
bool jjs_wstream_from_stringbuilder (struct ecma_stringbuilder_t* builder, jjs_wstream_t* out);
bool jjs_wstream_log (jjs_wstream_t* out);

void jjs_wstream_write_string (const jjs_wstream_t* stream_p, jjs_value_t value, jjs_value_t value_o);

#endif /* JJS_STREAM_H */
