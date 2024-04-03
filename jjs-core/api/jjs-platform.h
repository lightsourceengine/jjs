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

#ifndef JJS_PLATFORM_H
#define JJS_PLATFORM_H

#include "jjs-core.h"
#include "ecma-globals.h"

jjs_platform_t jjsp_defaults (void);

jjs_value_t jjsp_read_file (jjs_value_t path, jjs_encoding_t encoding);
jjs_value_t jjsp_read_file_buffer (jjs_value_t path, jjs_platform_buffer_t* buffer_p);

void jjsp_buffer_free (jjs_platform_buffer_t* buffer_p);
char* jjsp_strndup (const char* str_p, uint32_t length, bool is_null_terminated);
char* jjsp_cesu8_to_utf8_sz (const uint8_t* cesu8_p, uint32_t cesu8_size);
ecma_char_t* jjsp_cesu8_to_utf16_sz (const uint8_t* cesu8_p, uint32_t cesu8_size);
ecma_value_t jjsp_buffer_to_string_value (jjs_platform_buffer_t* buffer_p, bool move);

bool jjsp_find_root_end_index (const lit_utf8_byte_t* str_p, lit_utf8_size_t size, lit_utf8_size_t* index);
bool jjsp_path_is_separator (lit_utf8_byte_t ch);

void JJS_ATTR_NORETURN jjsp_fatal (jjs_fatal_code_t code);
jjs_platform_status_t jjsp_cwd (jjs_platform_buffer_t* buffer_p);

void jjsp_io_log (const char* message_p);

void jjsp_time_sleep (uint32_t sleep_time_ms);
int32_t jjsp_time_local_tza (double unix_ms);
double jjsp_time_now_ms (void);
uint64_t jjsp_time_hrtime (void);

jjs_platform_status_t jjsp_path_realpath (const uint8_t* utf8_p, uint32_t size, jjs_platform_buffer_t* buffer_p);

jjs_platform_status_t jjsp_fs_read_file (const uint8_t* cesu8_p, uint32_t size, jjs_platform_buffer_t* buffer_p);

#endif /* JJS_PLATFORM_H */
