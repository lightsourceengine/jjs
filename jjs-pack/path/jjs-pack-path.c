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
#include "jjs-port.h"

#if JJS_PACK_PATH

extern uint8_t jjs_pack_path_snapshot[];
extern const uint32_t jjs_pack_path_snapshot_len;

static JJS_HANDLER (jjs_pack_path_env)
{
  JJS_HANDLER_HEADER ();
  if (args_cnt == 0)
  {
    return jjs_undefined ();
  }

  uint8_t buffer[256];
  jjs_size_t written = jjs_string_to_buffer (args_p[0],
                                             JJS_ENCODING_UTF8,
                                             &buffer[0],
                                             (jjs_size_t) (sizeof (buffer) / sizeof (buffer[0])) - 1);
  buffer[written] = '\0';

  char* value = getenv ((const char*) buffer);

  if (value == NULL)
  {
    return jjs_string_sz ("");
  }

  return jjs_string ((const jjs_char_t*) value, (jjs_size_t) strlen (value), JJS_ENCODING_UTF8);
} /* jjs_pack_path_env */

static JJS_HANDLER (jjs_pack_path_cwd)
{
  JJS_HANDLER_HEADER ();
  jjs_char_t* cwd = jjs_port_path_normalize ((const jjs_char_t*) ".", 1);

  if (cwd == NULL)
  {
    return jjs_throw_sz (JJS_ERROR_COMMON, "Unable to get current working directory");
  }

  jjs_value_t cwd_value = jjs_string (cwd, (jjs_size_t) strlen ((const char*) cwd), JJS_ENCODING_UTF8);
  jjs_port_path_free (cwd);

  return cwd_value;
} /* jjs_pack_path_cwd */

static JJS_HANDLER (jjs_pack_lib_path_vmod_callback)
{
  JJS_HANDLER_HEADER ();
  jjs_value_t bindings = jjs_bindings ();

  jjs_bindings_platform (bindings);
  jjs_bindings_function (bindings, "env", &jjs_pack_path_env);
  jjs_bindings_function (bindings, "cwd", &jjs_pack_path_cwd);

  return jjs_pack_lib_read_exports (jjs_pack_path_snapshot,
                                    jjs_pack_path_snapshot_len,
                                    bindings,
                                    true,
                                    JJS_PACK_LIB_EXPORTS_FORMAT_VMOD);
} /* jjs_pack_lib_vmod_path */

#endif /* JJS_PACK_PATH */

jjs_value_t
jjs_pack_path_init (void)
{
#if JJS_PACK_PATH
  return jjs_pack_lib_main_vmod ("jjs:path", jjs_pack_lib_path_vmod_callback);
#else /* !JJS_PACK_PATH */
  return jjs_throw_sz (JJS_ERROR_COMMON, "path pack is not enabled");
#endif /* JJS_PACK_PATH */
} /* jjs_pack_path_init */
