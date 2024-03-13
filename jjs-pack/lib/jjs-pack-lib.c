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

#define PACK_INIT_BLOCK(FLAGS, NAME, INIT_FN)                                                   \
  do                                                                                            \
  {                                                                                             \
    if ((FLAGS) &JJS_PACK_INIT_##NAME)                                                          \
    {                                                                                           \
      jjs_value_t result = INIT_FN ();                                                          \
      if (jjs_value_is_exception (result))                                                      \
      {                                                                                         \
        return result;                                                                          \
      }                                                                                         \
      jjs_value_free (result);                                                                  \
    }                                                                                           \
  } while (0)

static jjs_value_t jjs_pack_lib_run_module (jjs_value_t fn, jjs_value_t bindings);

void
jjs_pack_init (uint32_t init_flags)
{
  jjs_value_free (jjs_pack_init_v (init_flags));
} /* jjs_pack_init */

jjs_value_t
jjs_pack_init_v (uint32_t init_flags)
{
  PACK_INIT_BLOCK (init_flags, CONSOLE, jjs_pack_console_init);
  PACK_INIT_BLOCK (init_flags, DOMEXCEPTION, jjs_pack_domexception_init);
  PACK_INIT_BLOCK (init_flags, FS, jjs_pack_fs_init);
  PACK_INIT_BLOCK (init_flags, PATH, jjs_pack_path_init);
  PACK_INIT_BLOCK (init_flags, PATH_URL, jjs_pack_path_url_init);
  PACK_INIT_BLOCK (init_flags, PERFORMANCE, jjs_pack_performance_init);
  PACK_INIT_BLOCK (init_flags, TEXT, jjs_pack_text_init);
  PACK_INIT_BLOCK (init_flags, URL, jjs_pack_url_init);

  return jjs_boolean (true);
} /* jjs_pack_init_v */

void
jjs_pack_cleanup (void)
{
  // nothing to do for current packs, but a cleanup will be needed for future packs
} /* jjs_pack_cleanup */

jjs_value_t
jjs_pack_lib_main (uint8_t* source, jjs_size_t source_size, jjs_value_t bindings, bool bindings_move)
{
  jjs_value_t exports =
    jjs_pack_lib_read_exports (source, source_size, bindings, bindings_move, JJS_PACK_LIB_EXPORTS_FORMAT_OBJECT);

  if (jjs_value_is_exception (exports))
  {
    return exports;
  }

  jjs_value_free (exports);

  return jjs_undefined ();
} /* jjs_pack_lib_main */

jjs_value_t
jjs_pack_lib_main_vmod (const char* package_name, jjs_external_handler_t vmod_callback)
{
  jjs_value_t function_value = jjs_function_external (vmod_callback);
  jjs_value_t result = jjs_vmod_sz (package_name, function_value);

  jjs_value_free (function_value);

  return result;
} /* jjs_pack_lib_main_vmod */

jjs_value_t
jjs_pack_lib_read_exports (uint8_t* source,
                           jjs_size_t source_size,
                           jjs_value_t bindings,
                           bool bindings_move,
                           jjs_pack_lib_exports_format_t exports_format)
{
  jjs_value_t fn =
    jjs_exec_snapshot ((const uint32_t*) source, source_size, 0, JJS_SNAPSHOT_EXEC_LOAD_AS_FUNCTION, NULL);

  if (jjs_value_is_exception (fn))
  {
    if (bindings_move)
    {
      jjs_value_free (bindings);
    }

    return fn;
  }

  jjs_value_t exports = jjs_pack_lib_run_module (fn, bindings);

  jjs_value_free (fn);

  if (bindings_move)
  {
    jjs_value_free (bindings);
  }

  if (jjs_value_is_exception (exports))
  {
    return exports;
  }

  if (exports_format == JJS_PACK_LIB_EXPORTS_FORMAT_VMOD)
  {
    jjs_value_t vmod_config = jjs_object ();
    jjs_value_t format_value = jjs_string_sz ("object");

    jjs_value_free (jjs_object_set_sz (vmod_config, "exports", exports));
    jjs_value_free (jjs_object_set_sz (vmod_config, "format", format_value));

    jjs_value_free (format_value);
    jjs_value_free (exports);

    return vmod_config;
  }

  return exports;
} /* jjs_pack_lib_read_exports */

void
jjs_bindings_platform (jjs_value_t bindings)
{
  jjs_value_t platform;

#if defined(_WIN32)
  platform = jjs_string_sz ("win32");
#elif defined(__linux__)
  platform = jjs_string_sz ("linux");
#elif defined(__APPLE__)
  platform = jjs_string_sz ("darwin");
#else /* !_WIN32 */
  platform = jjs_string_sz ("unknown");
#endif /* !_WIN32 */

  jjs_value_free (jjs_object_set_sz (bindings, "platform", platform));
  jjs_value_free (platform);
} /* jjs_bindings_platform */

void
jjs_bindings_function (jjs_value_t bindings, const char* name, jjs_external_handler_t function_p)
{
  jjs_value_t function = jjs_function_external (function_p);

  jjs_value_free (jjs_object_set_sz (bindings, name, function));
  jjs_value_free (function);
} /* jjs_bindings_function */

void
jjs_bindings_number (jjs_value_t bindings, const char* name, double number)
{
  jjs_value_t value = jjs_number (number);

  jjs_value_free (jjs_object_set_sz (bindings, name, value));
  jjs_value_free (value);
} /* jjs_bindings_number */

static jjs_value_t
jjs_pack_lib_run_module (jjs_value_t fn, jjs_value_t bindings)
{
  jjs_value_t module = jjs_object ();
  jjs_value_t exports = jjs_object ();
  jjs_value_t argv[] = { module, exports };

  jjs_value_free (jjs_object_set_sz (module, "exports", exports));
  jjs_value_free (jjs_object_set_sz (module, "bindings", bindings));

  jjs_value_t result = jjs_call (fn, jjs_undefined (), argv, sizeof (argv) / sizeof (jjs_value_t));

  if (!jjs_value_is_exception (result))
  {
    jjs_value_free (result);
    result = jjs_object_get_sz (module, "exports");

    if (jjs_value_is_exception (result))
    {
      jjs_value_free (result);
      result = jjs_throw_sz (JJS_ERROR_TYPE, "module exports property is not valid");
    }
  }

  jjs_value_free (module);
  jjs_value_free (exports);

  return result;
} /* jjs_pack_lib_run_module */
