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

JJS_PACK_DEFINE_EXTERN_SOURCE (jjs_pack_url_api)

static const char* URL_ID = "URL";
static const char* URL_SEARCH_PARAMS_ID = "URLSearchParams";

jjs_value_t
jjs_pack_url_init (void)
{
  if (jjs_pack_lib_global_has_sz (URL_ID))
  {
    return jjs_undefined ();
  }

  jjs_value_t api = jjs_pack_lib_load_from_snapshot (jjs_pack_url_api_snapshot, jjs_pack_url_api_snapshot_len, NULL, false);

  if (jjs_value_is_exception (api))
  {
    return api;
  }

  jjs_value_t url = jjs_object_get_sz (api, URL_ID);
  jjs_value_t usp = jjs_object_get_sz (api, URL_SEARCH_PARAMS_ID);
  jjs_value_t result;

  if (jjs_value_is_exception (url) || jjs_value_is_exception (usp))
  {
    result = jjs_throw_sz (JJS_ERROR_COMMON, "Invalid url-api.js");
  }
  else
  {
    jjs_pack_lib_global_set_sz (URL_ID, url);
    jjs_pack_lib_global_set_sz (URL_SEARCH_PARAMS_ID, usp);
    result = jjs_undefined ();
  }

  jjs_value_free (url);
  jjs_value_free (usp);
  jjs_value_free (api);

  return result;
} /* jjs_pack_url_init */
