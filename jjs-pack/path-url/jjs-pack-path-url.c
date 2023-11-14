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

#if JJS_PACK_PATH_URL

JJS_PACK_DEFINE_EXTERN_SOURCE (jjs_pack_path_url)

static JJS_HANDLER (jjs_pack_path_url_path)
{
  JJS_UNUSED (call_info_p);
  JJS_UNUSED (args_p);
  JJS_UNUSED (args_cnt);

  jjs_value_t lib = jjs_commonjs_require_sz ("jjs:path");

  if (jjs_value_is_exception (lib))
  {
    jjs_value_free (lib);
    return jjs_throw_sz (JJS_ERROR_COMMON, "jjs:path-url requires jjs:path to be installed");
  }

  return lib;
} /* jjs_pack_path_url_path */

static jjs_value_t
jjs_pack_path_url_bindings (void)
{
  jjs_value_t bindings = jjs_object ();

  jjs_pack_lib_add_is_windows (bindings);

  jjs_value_t path = jjs_function_external (&jjs_pack_path_url_path);
  jjs_value_free (jjs_object_set_sz(bindings, "path", path));
  jjs_value_free (path);

  return bindings;
} /* jjs_pack_path_url_bindings */

JJS_PACK_LIB_VMOD_SETUP (jjs_pack_path_url, &jjs_pack_path_url_bindings)

#endif /* JJS_PACK_PATH_URL */

jjs_value_t
jjs_pack_path_url_init (void)
{
#if JJS_PACK_PATH_URL
  return jjs_pack_lib_vmod_sz ("jjs:path-url", &jjs_pack_path_url_vmod_setup);
#else /* !JJS_PACK_PATH_URL */
  return jjs_throw_sz (JJS_ERROR_COMMON, "path url pack is not enabled");
#endif /* JJS_PACK_PATH_URL */
} /* jjs_pack_path_url_init */
