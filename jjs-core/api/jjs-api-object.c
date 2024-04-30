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

#include "jjs-api-object.h"

#include "jjs-annex.h"
#include "jjs-stream.h"
#include "jjs-util.h"
#if JJS_ANNEX_VMOD
#include "jjs-annex-vmod.h"
#endif /* JJS_ANNEX_VMOD */

#include "annex.h"
#include "jcontext.h"

static jjs_util_option_pair_t READ_FILE_ENCODING_OPTION_MAP[] = {
  { "none", JJS_ENCODING_NONE },  { "utf8", JJS_ENCODING_UTF8 },   { "utf-8", JJS_ENCODING_UTF8 },
  { "cesu8", JJS_ENCODING_UTF8 }, { "cesu-8", JJS_ENCODING_UTF8 },
};
static jjs_size_t READ_FILE_ENCODING_OPTION_MAP_LEN =
  sizeof (READ_FILE_ENCODING_OPTION_MAP) / sizeof (READ_FILE_ENCODING_OPTION_MAP[0]);

/** jjs.cwd handler */
static JJS_HANDLER (jjs_api_cwd_handler)
{
  JJS_UNUSED_ALL (args_count, args_p);
  jjs_context_t *context_p = call_info_p->context_p;

  return jjs_platform_cwd (context_p);
} /* jjs_api_cwd_handler */

/** jjs.realpath handler */
static JJS_HANDLER (jjs_api_realpath_handler)
{
  jjs_context_t *context_p = call_info_p->context_p;
  return jjs_platform_realpath (context_p, args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED, JJS_KEEP);
} /* jjs_api_realpath_handler */

/** jjs.gc handler */
static JJS_HANDLER (jjs_api_gc_handler)
{
  jjs_context_t *context_p = call_info_p->context_p;
  jjs_gc_mode_t mode =
    ((args_count > 0 && jjs_value_to_boolean (context_p, args_p[0])) ? JJS_GC_PRESSURE_HIGH : JJS_GC_PRESSURE_LOW);

  jjs_heap_gc (context_p, mode);

  return ECMA_VALUE_UNDEFINED;
} /* jjs_api_gc_handler */

/** jjs.readFile handler */
static JJS_HANDLER (jjs_api_read_file_handler)
{
  jjs_context_t *context_p = call_info_p->context_p;

  uint32_t raw_encoding;

  bool result = jjs_util_map_option (context_p,
                                     args_count > 1 ? args_p[1] : jjs_undefined (context_p),
                                     JJS_KEEP,
                                     jjs_string_sz (context_p, "encoding"),
                                     JJS_MOVE,
                                     READ_FILE_ENCODING_OPTION_MAP,
                                     READ_FILE_ENCODING_OPTION_MAP_LEN,
                                     JJS_ENCODING_NONE,
                                     &raw_encoding);

  if (result)
  {
    jjs_platform_read_file_options_t options = {
      .encoding = (jjs_encoding_t) raw_encoding,
    };

    return jjs_platform_read_file (context_p, args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED, JJS_KEEP, &options);
  }
  else
  {
    return jjs_throw_sz (context_p, JJS_ERROR_TYPE, "Invalid encoding in argument 2");
  }
} /* jjs_api_read_file_handler */

void
jjs_api_object_init (jjs_context_t* context_p, ecma_value_t realm)
{
  jjs_value_t jjs = jjs_object (context_p);
  ecma_object_t* jjs_p = ecma_get_object_from_value (context_p, jjs);
  uint32_t exclusions = context_p->jjs_namespace_exclusions;

  annex_util_define_ro_value (context_p, jjs_p, LIT_MAGIC_STRING_VERSION, jjs_string_sz (context_p, JJS_API_VERSION_STRING), JJS_MOVE);
  annex_util_define_ro_value (context_p, jjs_p, LIT_MAGIC_STRING_OS, jjs_platform_os (context_p), JJS_MOVE);
  annex_util_define_ro_value (context_p, jjs_p, LIT_MAGIC_STRING_ARCH, jjs_platform_arch (context_p), JJS_MOVE);

  if ((exclusions & JJS_NAMESPACE_EXCLUSION_CWD) == 0 && jjs_platform_has_cwd (context_p))
  {
    annex_util_define_function (context_p, jjs_p, LIT_MAGIC_STRING_CWD, jjs_api_cwd_handler);
  }

  if ((exclusions & JJS_NAMESPACE_EXCLUSION_REALPATH) == 0 && jjs_platform_has_realpath (context_p))
  {
    annex_util_define_function (context_p, jjs_p, LIT_MAGIC_STRING_REALPATH, jjs_api_realpath_handler);
  }

  if ((exclusions & JJS_NAMESPACE_EXCLUSION_READ_FILE) == 0 && jjs_platform_has_read_file (context_p))
  {
    annex_util_define_function (context_p, jjs_p, LIT_MAGIC_STRING_READ_FILE, jjs_api_read_file_handler);
  }

#if JJS_ANNEX_PMAP
  if ((exclusions & JJS_NAMESPACE_EXCLUSION_PMAP) == 0)
  {
    annex_util_define_ro_value (context_p, jjs_p, LIT_MAGIC_STRING_PMAP, jjs_annex_pmap_create_api (context_p), JJS_MOVE);
  }
#endif /* JJS_ANNEX_PMAP */

#if JJS_ANNEX_VMOD
  if ((exclusions & JJS_NAMESPACE_EXCLUSION_VMOD) == 0)
  {
    annex_util_define_ro_value (context_p, jjs_p, LIT_MAGIC_STRING_VMOD, jjs_annex_vmod_create_api (context_p), JJS_MOVE);
  }
#endif /* JJS_ANNEX_VMOD */

  if ((exclusions & JJS_NAMESPACE_EXCLUSION_GC) == 0)
  {
    annex_util_define_function (context_p, jjs_p, LIT_MAGIC_STRING_GC, jjs_api_gc_handler);
  }

  jjs_value_t stream;

  /* stdout */
  if ((exclusions & JJS_NAMESPACE_EXCLUSION_STDOUT) == 0 && jjs_wstream_new (context_p, JJS_STDOUT, &stream))
  {
    annex_util_define_value (context_p, jjs_p, LIT_MAGIC_STRING_STDOUT, stream, JJS_MOVE);
  }

  /* stderr */
  if ((exclusions & JJS_NAMESPACE_EXCLUSION_STDERR) == 0 && jjs_wstream_new (context_p, JJS_STDERR, &stream))
  {
    annex_util_define_value (context_p, jjs_p, LIT_MAGIC_STRING_STDERR, stream, JJS_MOVE);
  }

  annex_util_define_ro_value (context_p, ecma_get_object_from_value (context_p, realm), LIT_MAGIC_STRING_JJS, jjs, JJS_MOVE);
} /* jjs_api_object_init */
