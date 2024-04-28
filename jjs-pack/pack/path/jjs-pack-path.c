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

#include <stdlib.h>
#include <string.h>

#include "jjs-pack-lib.h"
#include "jjs-pack.h"

#if JJS_PACK_PATH

extern uint8_t jjs_pack_path_snapshot[];
extern const uint32_t jjs_pack_path_snapshot_len;

static JJS_HANDLER (jjs_pack_path_env)
{
  JJS_HANDLER_HEADER ();
  jjs_context_t *context_p = call_info_p->context_p;

  if (args_cnt == 0)
  {
    return jjs_undefined (context_p);
  }

  uint8_t buffer[256];
  jjs_size_t written = jjs_string_to_buffer (context_p,
                                             args_p[0],
                                             JJS_ENCODING_UTF8,
                                             &buffer[0],
                                             (jjs_size_t) (sizeof (buffer) / sizeof (buffer[0])) - 1);
  buffer[written] = '\0';

  char* value = getenv ((const char*) buffer);

  return jjs_string_utf8_sz (context_p, value ? value : "");
} /* jjs_pack_path_env */

static JJS_HANDLER (jjs_pack_lib_path_vmod_callback)
{
  JJS_HANDLER_HEADER ();
  jjs_context_t *context_p = call_info_p->context_p;
  jjs_value_t bindings = jjs_bindings (context_p);

  jjs_bindings_function (context_p, bindings, "env", &jjs_pack_path_env);

  return jjs_pack_lib_read_exports (context_p,
                                    jjs_pack_path_snapshot,
                                    jjs_pack_path_snapshot_len,
                                    bindings,
                                    JJS_MOVE,
                                    JJS_PACK_LIB_EXPORTS_FORMAT_VMOD);
} /* jjs_pack_lib_vmod_path */

#endif /* JJS_PACK_PATH */

jjs_value_t
jjs_pack_path_init (jjs_context_t *context_p)
{
#if JJS_PACK_PATH
  return jjs_pack_lib_main_vmod (context_p, "jjs:path", jjs_pack_lib_path_vmod_callback);
#else /* !JJS_PACK_PATH */
  return jjs_throw_sz (context_p, JJS_ERROR_COMMON, "path pack is not enabled");
#endif /* JJS_PACK_PATH */
} /* jjs_pack_path_init */
