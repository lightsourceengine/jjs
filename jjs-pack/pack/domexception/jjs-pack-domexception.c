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

#if JJS_PACK_DOMEXCEPTION
extern uint8_t jjs_pack_domexception_snapshot[];
extern const uint32_t jjs_pack_domexception_snapshot_len;
#endif /* JJS_PACK_DOMEXCEPTION */

jjs_value_t
jjs_pack_domexception_init (void)
{
#if JJS_PACK_DOMEXCEPTION
  return jjs_pack_lib_main (jjs_pack_domexception_snapshot,
                            jjs_pack_domexception_snapshot_len,
                            jjs_undefined (),
                            false);
#else /* !JJS_PACK_DOMEXCEPTION */
  return jjs_throw_sz (JJS_ERROR_COMMON, "domexception pack is not enabled");
#endif /* JJS_PACK_DOMEXCEPTION */
} /* jjs_pack_domexception_init */
