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
#include "jjs.h"

#include "annex.h"
#include "jcontext.h"

/** jjs.cwd handler */
static JJS_HANDLER (jjs_api_cwd_handler)
{
  JJS_UNUSED_ALL (call_info_p, args_count, args_p);
  return jjs_platform_cwd ();
} /* jjs_api_cwd_handler */

/** jjs.realpath handler */
static JJS_HANDLER (jjs_api_realpath_handler)
{
  JJS_UNUSED_ALL (call_info_p);
  return jjs_platform_realpath (args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED, JJS_KEEP);
} /* jjs_api_realpath_handler */

/** jjs.gc handler */
static JJS_HANDLER (jjs_api_gc_handler)
{
  JJS_UNUSED_ALL (call_info_p);

  jjs_gc_mode_t mode =
    ((args_count > 0 && jjs_value_to_boolean (args_p[0])) ? JJS_GC_PRESSURE_HIGH : JJS_GC_PRESSURE_LOW);

  jjs_heap_gc (mode);

  return ECMA_VALUE_UNDEFINED;
} /* jjs_api_gc_handler */

/** jjs.readFile handler */
static JJS_HANDLER (jjs_api_read_file_handler)
{
  JJS_UNUSED_ALL (call_info_p);

  jjs_value_t encoding_value;

  if (args_count > 1)
  {
    if (jjs_value_is_string (args_p[1]))
    {
      encoding_value = jjs_value_copy (args_p[1]);
    }
    else if (jjs_value_is_object (args_p[1]))
    {
      encoding_value = jjs_object_get_sz (args_p[1], "encoding");
    }
    else if (jjs_value_is_undefined (args_p[1]))
    {
      encoding_value = ECMA_VALUE_UNDEFINED;
    }
    else
    {
      encoding_value = ECMA_VALUE_EMPTY;
    }
  }
  else
  {
    encoding_value = ECMA_VALUE_UNDEFINED;
  }

  jjs_platform_read_file_options_t options = { 0 };

  if (!jjs_util_parse_encoding (encoding_value, &options.encoding))
  {
    jjs_throw_sz (JJS_ERROR_TYPE, "readFile got invalid encoding in argument 2");
  }

  return jjs_platform_read_file (args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED, JJS_KEEP, &options);
} /* jjs_api_read_file_handler */

void
jjs_api_object_init (ecma_value_t realm)
{
  jjs_value_t jjs = jjs_object ();
  ecma_object_t* jjs_p = ecma_get_object_from_value (jjs);

  annex_util_define_ro_value (jjs_p, LIT_MAGIC_STRING_VERSION, jjs_string_sz (JJS_API_VERSION_STRING), JJS_MOVE);
  annex_util_define_ro_value (jjs_p, LIT_MAGIC_STRING_OS, jjs_platform_os (), JJS_MOVE);
  annex_util_define_ro_value (jjs_p, LIT_MAGIC_STRING_ARCH, jjs_platform_arch (), JJS_MOVE);

  if (jjs_platform_has_cwd ())
  {
    annex_util_define_function (jjs_p, LIT_MAGIC_STRING_CWD, jjs_api_cwd_handler);
  }

  if (jjs_platform_has_realpath ())
  {
    annex_util_define_function (jjs_p, LIT_MAGIC_STRING_REALPATH, jjs_api_realpath_handler);
  }

  if (jjs_platform_has_read_file ())
  {
    annex_util_define_function (jjs_p, LIT_MAGIC_STRING_READ_FILE, jjs_api_read_file_handler);
  }

  if ((JJS_CONTEXT (context_flags) & JJS_CONTEXT_FLAG_EXPOSE_GC) != 0)
  {
    annex_util_define_function (jjs_p, LIT_MAGIC_STRING_GC, jjs_api_gc_handler);
  }

  jjs_value_t stream;

  /* stdout */
  if (jjs_wstream_new (JJS_STDOUT, &stream))
  {
    annex_util_define_value (jjs_p, LIT_MAGIC_STRING_STDOUT, stream, JJS_MOVE);
  }

  /* stderr */
  if (jjs_wstream_new (JJS_STDERR, &stream))
  {
    annex_util_define_value (jjs_p, LIT_MAGIC_STRING_STDERR, stream, JJS_MOVE);
  }

  annex_util_define_value (ecma_get_object_from_value (realm), LIT_MAGIC_STRING_JJS, jjs, JJS_MOVE);
} /* jjs_api_object_init */
