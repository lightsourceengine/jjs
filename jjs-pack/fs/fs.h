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

#ifndef FS_H
#define FS_H

#include <stdint.h>

int fs_get_size (const char* path, uint32_t* size_p);

int fs_read(const char* path, uint8_t** buffer_p, uint32_t* buffer_size);
void fs_read_free(uint8_t* buffer_p);

int fs_write (const char* path_p, uint8_t* buffer_p, uint32_t buffer_size, uint32_t* written_p);
int fs_copy (const char* path_p, const char* source_p, uint32_t* written_p);
int fs_remove (const char* path);

const char* fs_errno_to_string(int errno_value);
const char* fs_errno_message(int errno_value);

#endif /* !defined (FS_H) */
