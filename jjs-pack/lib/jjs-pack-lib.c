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

#define PACK_INIT_BLOCK(CTX, FLAGS, NAME, INIT_FN)                                              \
  do                                                                                            \
  {                                                                                             \
    if ((FLAGS) &JJS_PACK_INIT_##NAME)                                                          \
    {                                                                                           \
      jjs_value_t result = INIT_FN (CTX);                                                       \
      if (jjs_value_is_exception ((CTX), result))                                               \
      {                                                                                         \
        return result;                                                                          \
      }                                                                                         \
      jjs_value_free ((CTX), result);                                                           \
    }                                                                                           \
  } while (0)

static jjs_value_t jjs_pack_lib_run_module (jjs_context_t *context_p, jjs_value_t fn, jjs_value_t bindings);

void
jjs_pack_init (jjs_context_t *context_p, uint32_t init_flags)
{
  jjs_value_free (context_p, jjs_pack_init_v (context_p, init_flags));
} /* jjs_pack_init */

jjs_value_t
jjs_pack_init_v (jjs_context_t *context_p, uint32_t init_flags)
{
  PACK_INIT_BLOCK (context_p, init_flags, CONSOLE, jjs_pack_console_init);
  PACK_INIT_BLOCK (context_p, init_flags, DOMEXCEPTION, jjs_pack_domexception_init);
  PACK_INIT_BLOCK (context_p, init_flags, FS, jjs_pack_fs_init);
  PACK_INIT_BLOCK (context_p, init_flags, PATH, jjs_pack_path_init);
  PACK_INIT_BLOCK (context_p, init_flags, PERFORMANCE, jjs_pack_performance_init);
  PACK_INIT_BLOCK (context_p, init_flags, TEXT, jjs_pack_text_init);
  PACK_INIT_BLOCK (context_p, init_flags, URL, jjs_pack_url_init);

  return jjs_boolean (context_p, true);
} /* jjs_pack_init_v */

void
jjs_pack_cleanup (jjs_context_t *context_p)
{
  JJS_UNUSED (context_p);
  // nothing to do for current packs, but a cleanup will be needed for future packs
} /* jjs_pack_cleanup */

jjs_value_t
jjs_pack_lib_main (jjs_context_t *context_p, uint8_t* source, jjs_size_t source_size, jjs_value_t bindings, jjs_own_t bindings_o)
{
  jjs_value_t exports =
    jjs_pack_lib_read_exports (context_p, source, source_size, bindings, bindings_o, JJS_PACK_LIB_EXPORTS_FORMAT_OBJECT);

  if (jjs_value_is_exception (context_p, exports))
  {
    return exports;
  }

  jjs_value_free (context_p, exports);

  return jjs_undefined (context_p);
} /* jjs_pack_lib_main */

jjs_value_t
jjs_pack_lib_main_vmod (jjs_context_t *context_p, const char* package_name, jjs_external_handler_t vmod_callback)
{
  return jjs_vmod_sz (context_p, package_name, jjs_function_external (context_p, vmod_callback), JJS_MOVE);
} /* jjs_pack_lib_main_vmod */

jjs_value_t
jjs_pack_lib_read_exports (jjs_context_t *context_p,
                           uint8_t* source,
                           jjs_size_t source_size,
                           jjs_value_t bindings,
                           jjs_own_t bindings_o,
                           jjs_pack_lib_exports_format_t exports_format)
{
  jjs_value_t fn =
    jjs_exec_snapshot (context_p, (const uint32_t*) source, source_size, 0, JJS_SNAPSHOT_EXEC_LOAD_AS_FUNCTION, NULL);

  if (jjs_value_is_exception (context_p, fn))
  {
    if (bindings_o == JJS_MOVE)
    {
      jjs_value_free (context_p, bindings);
    }

    return fn;
  }

  jjs_value_t exports = jjs_pack_lib_run_module (context_p, fn, bindings);

  jjs_value_free (context_p, fn);

  if (bindings_o == JJS_MOVE)
  {
    jjs_value_free (context_p, bindings);
  }

  if (jjs_value_is_exception (context_p, exports))
  {
    return exports;
  }

  if (exports_format == JJS_PACK_LIB_EXPORTS_FORMAT_VMOD)
  {
    jjs_value_t vmod_config = jjs_object (context_p);
    jjs_value_t format_value = jjs_string_sz (context_p, "object");

    jjs_value_free (context_p, jjs_object_set_sz (context_p, vmod_config, "exports", exports));
    jjs_value_free (context_p, jjs_object_set_sz (context_p, vmod_config, "format", format_value));

    jjs_value_free (context_p, format_value);
    jjs_value_free (context_p, exports);

    return vmod_config;
  }

  return exports;
} /* jjs_pack_lib_read_exports */

void
jjs_bindings_function (jjs_context_t *context_p, jjs_value_t bindings, const char* name, jjs_external_handler_t function_p)
{
  jjs_bindings_value (context_p, bindings, name, jjs_function_external (context_p, function_p), JJS_MOVE);
} /* jjs_bindings_function */

void
jjs_bindings_value (jjs_context_t *context_p, jjs_value_t bindings, const char* name, jjs_value_t value, jjs_own_t value_o)
{
  jjs_value_free (context_p, jjs_object_set_sz (context_p, bindings, name, value));

  if (value_o == JJS_MOVE)
  {
    jjs_value_free (context_p, value);
  }
} /* jjs_bindings_value */

// note: the packs should never use the commonjs require or esm import(). if they need to depend on another
// pack package, vmod.resolve() should be used. Since vmod can only be imported through commonjs or esm, send
// vmod.resolve() to JS as require().
static JJS_HANDLER(jjs_pack_lib_require)
{
  JJS_HANDLER_HEADER ();
  jjs_context_t *context_p = call_info_p->context_p;

  return jjs_vmod_resolve (context_p, args_cnt > 0 ? args_p[0] : jjs_undefined (context_p), JJS_KEEP);
} /* jjs_pack_lib_require */

static jjs_value_t
jjs_pack_lib_run_module (jjs_context_t *context_p, jjs_value_t fn, jjs_value_t bindings)
{
  jjs_value_t module = jjs_object (context_p);
  jjs_value_t exports = jjs_object (context_p);
  jjs_value_t require = jjs_function_external (context_p, jjs_pack_lib_require);
  jjs_value_t argv[] = { module, exports, require };

  jjs_value_free (context_p, jjs_object_set_sz (context_p, module, "exports", exports));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, module, "bindings", bindings));
  jjs_value_free (context_p, jjs_object_set_sz (context_p, module, "require", require));

  jjs_value_t result = jjs_call (context_p, fn, jjs_undefined (context_p), argv, sizeof (argv) / sizeof (jjs_value_t));

  if (!jjs_value_is_exception (context_p, result))
  {
    jjs_value_free (context_p, result);
    result = jjs_object_get_sz (context_p, module, "exports");

    if (jjs_value_is_exception (context_p, result))
    {
      jjs_value_free (context_p, result);
      result = jjs_throw_sz (context_p, JJS_ERROR_TYPE, "module exports property is not valid");
    }
  }

  jjs_value_free (context_p, module);
  jjs_value_free (context_p, exports);
  jjs_value_free (context_p, require);

  return result;
} /* jjs_pack_lib_run_module */

#define NANOS_PER_SEC 1000000000

JJS_HANDLER (jjs_pack_hrtime_handler)
{
  JJS_HANDLER_HEADER ();
  jjs_context_t *context_p = call_info_p->context_p;
  uint64_t t = jjs_pack_platform_hrtime ();
  jjs_value_t result = jjs_array (context_p, 2);
  uint64_t seconds = t / NANOS_PER_SEC;
  jjs_value_t high_part = jjs_number (context_p, (double) seconds);
  jjs_value_t low_part = jjs_number (context_p, (double) (t % NANOS_PER_SEC));

  jjs_value_free (context_p, jjs_object_set_index (context_p, result, 0, high_part));
  jjs_value_free (context_p, jjs_object_set_index (context_p, result, 1, low_part));
  jjs_value_free (context_p, high_part);
  jjs_value_free (context_p, low_part);

  return result;
} /* jjs_pack_hrtime_handler */

JJS_HANDLER (jjs_pack_date_now_handler)
{
  JJS_HANDLER_HEADER ();
  jjs_context_t *context_p = call_info_p->context_p;

  return jjs_number (context_p, jjs_pack_platform_date_now ());
} /* jjs_pack_date_now_handler */
