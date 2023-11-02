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

JJS_PACK_DEFINE_EXTERN_SOURCE (jjs_pack_url)
JJS_PACK_DEFINE_EXTERN_SOURCE (jjs_pack_url_search_params)

static const char* URL_ID = "URL";
static const char* URL_SEARCH_PARAMS_ID = "URLSearchParams";

jjs_value_t
jjs_pack_url_init (void)
{
  jjs_value_t url;
  bool url_set;

  if (jjs_pack_lib_global_has (URL_ID))
  {
    url = jjs_undefined ();
    url_set = false;
  }
  else
  {
    url = JJS_PACK_LIB_GLOBAL_SET (URL_ID, jjs_pack_url, NULL);
    url_set = true;
  }

  if (jjs_value_is_exception (url))
  {
    return url;
  }

  jjs_value_t usp;

  if (jjs_pack_lib_global_has (URL_SEARCH_PARAMS_ID))
  {
    usp = jjs_undefined ();
  }
  else
  {
    usp = JJS_PACK_LIB_GLOBAL_SET (URL_SEARCH_PARAMS_ID, jjs_pack_url_search_params, NULL);
  }

  if (jjs_value_is_exception (usp))
  {
    if (url_set)
    {
      jjs_value_t realm = jjs_current_realm ();
      jjs_value_free (jjs_object_delete_sz (realm, URL_ID));
      jjs_value_free (realm);
      jjs_value_free (url);
    }

    return usp;
  }

  jjs_value_free (url);
  jjs_value_free (usp);

  return jjs_undefined ();
} /* jjs_pack_url_init */
