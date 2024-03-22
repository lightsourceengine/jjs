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

#include "jjs-annex-module-util.h"

#include "jcontext.h"
#include "annex.h"

#if JJS_COMMONJS || JJS_ESM

/**
 * Call the module_on_resolve callback.
 *
 * @param request require request
 * @param referrer_path directory path of the referrer
 * @param module_type type of the module
 * @return resolve result. must be freed by jjs_annex_module_resolve_free.
 */
jjs_annex_module_resolve_t jjs_annex_module_resolve (ecma_value_t request,
                                                     ecma_value_t referrer_path,
                                                     jjs_module_type_t module_type)
{
  jjs_esm_resolve_context_t context = {
    .type = module_type,
    .referrer_path = referrer_path,
  };

  jjs_esm_resolve_cb_t module_on_resolve_cb = JJS_CONTEXT (module_on_resolve_cb);

  JJS_ASSERT (module_on_resolve_cb != NULL);

  ecma_value_t resolve_result;

  if (module_on_resolve_cb != NULL)
  {
    resolve_result = module_on_resolve_cb (request, &context, JJS_CONTEXT (module_on_resolve_user_p));
  }
  else
  {
    resolve_result = jjs_throw_sz (JJS_ERROR_COMMON, "module_on_resolve callback is not set");
  }

  if (jjs_value_is_exception (resolve_result))
  {
    return (jjs_annex_module_resolve_t) {
      .path = ECMA_VALUE_UNDEFINED,
      .format = ECMA_VALUE_UNDEFINED,
      .result = resolve_result,
    };
  }

  // TODO: validate object returned from resolve callback

  return (jjs_annex_module_resolve_t) {
    .path = ecma_find_own_m (resolve_result, LIT_MAGIC_STRING_PATH),
    .format = ecma_find_own_m (resolve_result, LIT_MAGIC_STRING_FORMAT),
    .result = resolve_result,
  };
} /* jjs_annex_module_resolve */

/**
 * Free the resolve result.
 *
 * @param resolve_result_p pointer to a resolve result
 */
void jjs_annex_module_resolve_free (jjs_annex_module_resolve_t *resolve_result_p)
{
  jjs_value_free(resolve_result_p->path);
  jjs_value_free(resolve_result_p->format);
  jjs_value_free(resolve_result_p->result);
} /* jjs_annex_module_resolve_free */

jjs_annex_module_load_t jjs_annex_module_load (ecma_value_t path,
                                               ecma_value_t format,
                                               jjs_module_type_t module_type)
{
  jjs_esm_load_context_t context = {
    .type = module_type,
    .format = format,
  };

  ecma_value_t load_result;

  if (JJS_CONTEXT (module_on_load_cb) != NULL)
  {
    load_result = JJS_CONTEXT (module_on_load_cb) (path, &context, JJS_CONTEXT (module_on_load_user_p));
  }
  else
  {
    load_result = jjs_throw_sz (JJS_ERROR_COMMON, "module_on_load callback is not set");
  }

  if (jjs_value_is_exception (load_result))
  {
    return (jjs_annex_module_load_t) {
      .source = ECMA_VALUE_UNDEFINED,
      .format = ECMA_VALUE_UNDEFINED,
      .result = load_result,
    };
  }

  ecma_value_t final_format = ecma_find_own_m (load_result, LIT_MAGIC_STRING_FORMAT);

  if (!jjs_value_is_string (final_format))
  {
    jjs_value_free (load_result);
    jjs_value_free (final_format);

    return (jjs_annex_module_load_t) {
      .source = ECMA_VALUE_UNDEFINED,
      .format = ECMA_VALUE_UNDEFINED,
      .result = jjs_throw_sz (JJS_ERROR_TYPE, "Invalid format"),
    };
  }

  // TODO: validate source ?

  return (jjs_annex_module_load_t) {
    .source = ecma_find_own_m (load_result, LIT_MAGIC_STRING_SOURCE),
    .format = final_format,
    .result = load_result,
  };
} /* jjs_annex_module_load */

/**
 * Free the module load result.
 *
 * @param load_result_p pointer to a load result
 */
void jjs_annex_module_load_free (jjs_annex_module_load_t *load_result_p)
{
  jjs_value_free (load_result_p->source);
  jjs_value_free (load_result_p->format);
  jjs_value_free (load_result_p->result);
} /* jjs_annex_module_load_free */

#endif /* JJS_COMMONJS || JJS_ESM */
