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

#include "jjs-pack.h"

#include "jjs-port.h"
#include "jjs-pack-lib.h"
#include <stdlib.h>
#include <string.h>

JJS_PACK_DEFINE_EXTERN_SOURCE (jjs_pack_path)

static JJS_HANDLER (jjs_pack_path_env)
{
  JJS_UNUSED (call_info_p);

  if (args_cnt == 0)
  {
    return jjs_undefined ();
  }

  uint8_t buffer[256];
  jjs_size_t written = jjs_string_to_buffer (args_p[0],
                                             JJS_ENCODING_UTF8,
                                             &buffer[0],
                                             (jjs_size_t)(sizeof (buffer) / sizeof (buffer[0])) - 1);
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
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_p);
  JJS_UNUSED (args_cnt);

  jjs_char_t* cwd = jjs_port_path_normalize ((const jjs_char_t*) ".", 1);

  if (cwd == NULL)
  {
    return jjs_throw_sz (JJS_ERROR_COMMON, "Unable to get current working directory");
  }

  jjs_value_t cwd_value = jjs_string (cwd, (jjs_size_t) strlen ((const char*) cwd), JJS_ENCODING_UTF8);
  jjs_port_path_free (cwd);

  return cwd_value;
} /* jjs_pack_path_cwd */

static jjs_value_t
jjs_pack_path_bindings (void)
{
  jjs_value_t bindings = jjs_object ();
#if defined(_WIN32)
  jjs_value_t is_windows = jjs_boolean (true);
#else /* !_WIN32 */
  jjs_value_t is_windows = jjs_boolean (false);
#endif /* !_WIN32 */

  jjs_value_free (jjs_object_set_sz (bindings, "isWindows", is_windows));
  jjs_value_free (is_windows);

  jjs_value_t env = jjs_function_external (&jjs_pack_path_env);
  jjs_value_free (jjs_object_set_sz (bindings, "env", env));
  jjs_value_free (env);

  jjs_value_t cwd = jjs_function_external (&jjs_pack_path_cwd);
  jjs_value_free (jjs_object_set_sz (bindings, "cwd", cwd));
  jjs_value_free (cwd);

  return bindings;
} /* jjs_pack_path_bindings */

JJS_PACK_LIB_VMOD_SETUP (jjs_pack_path, &jjs_pack_path_bindings)

jjs_value_t
jjs_pack_path_init (void)
{
  return jjs_pack_lib_vmod_sz ("jjs:path", &jjs_pack_path_vmod_setup);
} /* jjs_pack_path_init */
