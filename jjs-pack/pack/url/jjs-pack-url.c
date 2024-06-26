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

#include "jjs-pack-lib.h"
#include "jjs-pack.h"

#if JJS_PACK_URL

extern uint8_t jjs_pack_url_snapshot[];
extern const uint32_t jjs_pack_url_snapshot_len;

#endif /* JJS_PACK_URL */

jjs_value_t
jjs_pack_url_init (jjs_context_t *context_p)
{
#if JJS_PACK_URL
  jjs_value_t exports = jjs_pack_lib_read_exports (context_p,
                                                   jjs_pack_url_snapshot,
                                                   jjs_pack_url_snapshot_len,
                                                   jjs_undefined (context_p),
                                                   JJS_MOVE,
                                                   JJS_PACK_LIB_EXPORTS_FORMAT_VMOD);
  return jjs_vmod_sz (context_p, "jjs:url", exports, JJS_MOVE);
#else /* !JJS_PACK_URL */
  return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "url pack is not enabled");
#endif /* JJS_PACK_URL */
} /* jjs_pack_url_init */
