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

#ifndef JJS_UTIL_H
#define JJS_UTIL_H

#include "jjs.h"
#include "ecma-globals.h"

typedef struct
{
  const char* name_sz;
  uint32_t value;
} jjs_util_option_pair_t;

typedef struct
{
  jjs_context_t *context_p;
  jjs_value_t buffer;
  jjs_allocator_t allocator;
} jjs_arraybuffer_allocator_t;

typedef struct
{
  uint8_t *buffer_p;
  jjs_size_t buffer_size;
  bool used;
  jjs_allocator_t allocator;
} jjs_oneshot_allocator_t;

jjs_value_t jjs_return (jjs_context_t *context_p, jjs_value_t value);

bool jjs_util_map_option (jjs_context_t* context_p,
                          jjs_value_t option,
                          jjs_own_t option_o,
                          jjs_value_t key,
                          jjs_own_t key_o,
                          const jjs_util_option_pair_t* option_mappings_p,
                          jjs_size_t len,
                          uint32_t default_mapped_value,
                          uint32_t* out_p);

jjs_status_t jjs_util_oneshot_allocator_init (uint8_t *buffer_p,
                                              jjs_size_t buffer_size,
                                              jjs_oneshot_allocator_t* dest_p);

jjs_status_t jjs_util_arraybuffer_allocator_init (jjs_context_t *context_p,
                                                  jjs_arraybuffer_allocator_t* dest_p);
void jjs_util_arraybuffer_allocator_deinit (jjs_arraybuffer_allocator_t* allocator_p);
jjs_value_t jjs_util_arraybuffer_allocator_move (jjs_arraybuffer_allocator_t* allocator_p);

jjs_allocator_t jjs_util_system_allocator (void);
const jjs_allocator_t* jjs_util_system_allocator_ptr (void);
jjs_allocator_t jjs_util_vm_allocator (jjs_context_t* context_p);

jjs_allocator_t* jjs_util_context_acquire_scratch_allocator (jjs_context_t* context_p);
void jjs_util_context_release_scratch_allocator (jjs_context_t* context_p);

jjs_status_t
jjs_util_convert (jjs_allocator_t* allocator,
                  const uint8_t* source_p,
                  jjs_size_t source_size,
                  jjs_encoding_t source_encoding,
                  void** dest_p,
                  jjs_size_t *dest_size,
                  jjs_encoding_t dest_encoding,
                  bool add_null_terminator,
                  bool add_windows_long_filename_prefix);

void
jjs_util_promise_unhandled_rejection_default (jjs_context_t* context_p,
                                              jjs_value_t promise,
                                              jjs_value_t reason,
                                              void *user_p);

jjs_value_t jjs_optional_value_or_undefined (const jjs_optional_value_t* optional_p);
uint32_t jjs_optional_u32_or (const jjs_optional_u32_t* optional_p, uint32_t value);

/**
 * Assert that it is correct to call API in current state.
 *
 * Note:
 *         By convention, there are some states when API could not be invoked.
 *
 *         The API can be and only be invoked when the ECMA_STATUS_API_ENABLED
 *         flag is set.
 *
 *         This procedure checks whether the API is available, and terminates
 *         the engine if it is unavailable. Otherwise it is a no-op.
 *
 * Note:
 *         The API could not be invoked in the following cases:
 *           - before jjs_init and after jjs_cleanup
 *           - between enter to and return from a native free callback
 */
// TODO: NDEBUG version
#define jjs_assert_api_enabled(CTX) JJS_ASSERT ((CTX) != NULL && ((CTX)->status_flags & ECMA_STATUS_API_ENABLED))

#define jjs_disown_value(CTX, VALUE, OWN) if ((OWN) == JJS_MOVE) jjs_value_free ((CTX), VALUE)
#define jjs_disown_source(CTX, SOURCE, OWN) if ((OWN) == JJS_MOVE) jjs_esm_source_free_values ((CTX), SOURCE)

#endif /* JJS_UTIL_H */
