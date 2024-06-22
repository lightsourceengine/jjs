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

/* platform api implementation helpers */

bool jjsp_path_is_relative (const lit_utf8_byte_t* path_p, lit_utf8_size_t size);
bool jjsp_path_is_absolute (const lit_utf8_byte_t* path_p, lit_utf8_size_t size);

bool jjsp_find_root_end_index (const lit_utf8_byte_t* str_p, lit_utf8_size_t size, lit_utf8_size_t* index);
bool jjsp_path_is_separator (lit_utf8_byte_t ch);

jjs_status_t jjs_platform_buffer_new (jjs_platform_buffer_t* self_p, const jjs_allocator_t* allocator, jjs_size_t size);
jjs_platform_buffer_t
jjs_platform_buffer (void* data_p, jjs_size_t data_size, const jjs_allocator_t* allocator);
void jjs_platform_buffer_view_from_buffer (jjs_platform_buffer_view_t* self_p, jjs_platform_buffer_t* source_p, jjs_encoding_t encoding);
jjs_status_t jjs_platform_buffer_view_new (jjs_platform_buffer_view_t* self_p, const jjs_allocator_t* allocator, jjs_size_t size, jjs_encoding_t encoding);

/* platform api implementations */

void JJS_ATTR_NORETURN jjsp_fatal_impl (jjs_fatal_code_t code);

void jjsp_io_write_impl (void* target_p, const uint8_t* data_p, uint32_t data_size, jjs_encoding_t encoding);
void jjsp_io_flush_impl (void* target_p);

jjs_status_t jjsp_time_sleep_impl (uint32_t sleep_time_ms);
jjs_status_t jjsp_time_local_tza_impl (double unix_ms, int32_t* out_p);
jjs_status_t jjsp_time_now_ms_impl (double* out_p);

jjs_status_t jjsp_path_cwd_impl (const jjs_allocator_t* allocator, jjs_platform_buffer_view_t* buffer_view_p);
jjs_status_t jjsp_path_realpath_impl (const jjs_allocator_t* allocator, jjs_platform_path_t* path_p, jjs_platform_buffer_view_t* buffer_view_p);

jjs_status_t
jjsp_fs_read_file_impl (const jjs_allocator_t* allocator, jjs_platform_path_t* path_p, jjs_platform_buffer_t* out_p);

#endif /* JJS_PLATFORM_H */
