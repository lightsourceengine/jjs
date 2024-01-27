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

#define PACK_INIT(INIT)                  \
  do                                     \
  {                                      \
    jjs_value_t result = INIT ();        \
    if (jjs_value_is_exception (result)) \
    {                                    \
      return result;                     \
    }                                    \
    jjs_value_free (result);             \
  } while (0)

static jjs_value_t jjs_pack_lib_run_module (jjs_value_t fn, jjs_pack_bindings_cb_t bindings);

jjs_value_t
jjs_pack_init (void)
{
  PACK_INIT (jjs_pack_console_init);
  PACK_INIT (jjs_pack_domexception_init);
  PACK_INIT (jjs_pack_fs_init);
  PACK_INIT (jjs_pack_path_init);
  PACK_INIT (jjs_pack_path_url_init);
  PACK_INIT (jjs_pack_performance_init);
  PACK_INIT (jjs_pack_text_init);
  PACK_INIT (jjs_pack_url_init);

  return jjs_undefined ();
} /* jjs_pack_init */

void
jjs_pack_init_unsafe (void)
{
  jjs_value_free (jjs_pack_init ());
} /* jjs_pack_init_unsafe */

jjs_value_t
jjs_pack_lib_load_from_snapshot (uint8_t* source, jjs_size_t source_size, jjs_pack_bindings_cb_t bindings, bool vmod_wrap)
{
  jjs_value_t fn =
    jjs_exec_snapshot ((const uint32_t*) source, source_size, 0, JJS_SNAPSHOT_EXEC_LOAD_AS_FUNCTION, NULL);

  if (jjs_value_is_exception (fn))
  {
    return fn;
  }

  jjs_value_t result = jjs_pack_lib_run_module (fn, bindings);

  jjs_value_free (fn);

  if (vmod_wrap && !jjs_value_is_exception (result))
  {
    jjs_value_t wrap = jjs_object();

    jjs_value_free(jjs_object_set_sz(wrap, "exports", result));
    jjs_value_free(result);
    result = wrap;
  }

  return result;
} /* jjs_pack_lib_load_from_snapshot */

jjs_value_t
jjs_pack_lib_global_set_from_snapshot (const char* id_p,
                                       uint8_t* source_p,
                                       jjs_size_t source_size,
                                       jjs_pack_bindings_cb_t bindings)
{
  if (jjs_pack_lib_global_has_sz (id_p))
  {
    return jjs_undefined ();
  }

  jjs_value_t value = jjs_pack_lib_load_from_snapshot (source_p, source_size, bindings, false);

  if (jjs_value_is_exception (value))
  {
    return value;
  }

  jjs_pack_lib_global_set_sz (id_p, value);
  jjs_value_free (value);

  return jjs_undefined ();
} /* jjs_pack_lib_global_set_from_snapshot */

jjs_value_t
jjs_pack_lib_vmod_sz (const char* name_p, jjs_vmod_create_cb_t create_cb)
{
  return jjs_vmod_exists_sz (name_p) ? jjs_undefined () : jjs_vmod_native_sz (name_p, create_cb, NULL);
} /* jjs_pack_lib_vmod_sz */

jjs_value_t
jjs_pack_lib_global_has_sz (const char* id_p)
{
  jjs_value_t realm = jjs_current_realm ();
  jjs_value_t result = jjs_object_has_sz (realm, id_p);

  jjs_value_free (realm);
  jjs_value_free (result);

  return jjs_value_is_true (result);
} /* jjs_pack_lib_global_has_sz */

static jjs_value_t
jjs_pack_lib_run_module (jjs_value_t fn, jjs_pack_bindings_cb_t bindings)
{
  jjs_value_t bindings_value;

  if (bindings)
  {
    bindings_value = bindings ();

    if (jjs_value_is_exception (bindings_value))
    {
      return bindings_value;
    }
  }
  else
  {
    bindings_value = jjs_undefined ();
  }

  jjs_value_t module = jjs_object ();
  jjs_value_t exports = jjs_object ();
  jjs_value_t argv[] = { module, exports };

  jjs_value_free (jjs_object_set_sz (module, "exports", exports));

  if (!jjs_value_is_undefined (bindings_value))
  {
    jjs_value_free (jjs_object_set_sz (module, "bindings", bindings_value));
  }

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

  jjs_value_free (bindings_value);
  jjs_value_free (module);
  jjs_value_free (exports);

  return result;
} /* jjs_pack_lib_run_module */

void
jjs_pack_lib_global_set_sz (const char* id_p, jjs_value_t value)
{
  jjs_value_t realm = jjs_current_realm ();

  jjs_value_free (jjs_object_set_sz (realm, id_p, value));
  jjs_value_free (realm);
} /* jjs_pack_lib_global_set_sz */

void
jjs_pack_lib_add_is_windows (jjs_value_t object)
{
#if defined(_WIN32)
  jjs_value_t is_windows = jjs_boolean (true);
#else /* !_WIN32 */
  jjs_value_t is_windows = jjs_boolean (false);
#endif /* !_WIN32 */

  jjs_value_free (jjs_object_set_sz (object, "isWindows", is_windows));
  jjs_value_free (is_windows);
} /* jjs_pack_lib_add_is_windows */

void jjs_pack_lib_set_function_sz (jjs_value_t bindings, const char* name, jjs_external_handler_t handler)
{
  jjs_value_t function = jjs_function_external (handler);
  jjs_value_free (jjs_object_set_sz (bindings, name, function));
  jjs_value_free (function);
}
