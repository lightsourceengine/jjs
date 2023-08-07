/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "lit-magic-strings.h"
#include "lit-strings.h"

#include "jjs-core.h"
#include "jjs-port.h"

#include "ecma-errors.h"

#if JJS_MODULE_SYSTEM

/**
 * A module descriptor.
 */
typedef struct jjs_module_t
{
  struct jjs_module_t *next_p; /**< next_module */
  jjs_char_t *path_p; /**< path to the module */
  jjs_size_t basename_offset; /**< offset of the basename in the module path*/
  jjs_value_t realm; /**< the realm of the module */
  jjs_value_t module; /**< the module itself */
} jjs_module_t;

/**
 * Native info descriptor for modules.
 */
static const jjs_object_native_info_t jjs_module_native_info JJS_ATTR_CONST_DATA = {
  .free_cb = NULL,
};

/**
 * Default module manager.
 */
typedef struct
{
  jjs_module_t *module_head_p; /**< first module */
} jjs_module_manager_t;

/**
 * Release known modules.
 */
static void
jjs_module_free (jjs_module_manager_t *manager_p, /**< module manager */
                   const jjs_value_t realm) /**< if this argument is object, release only those modules,
                                               *   which realm value is equal to this argument. */
{
  jjs_module_t *module_p = manager_p->module_head_p;

  bool release_all = !jjs_value_is_object (realm);

  jjs_module_t *prev_p = NULL;

  while (module_p != NULL)
  {
    jjs_module_t *next_p = module_p->next_p;

    if (release_all || module_p->realm == realm)
    {
      jjs_port_path_free (module_p->path_p);
      jjs_value_free (module_p->realm);
      jjs_value_free (module_p->module);

      jjs_heap_free (module_p, sizeof (jjs_module_t));

      if (prev_p == NULL)
      {
        manager_p->module_head_p = next_p;
      }
      else
      {
        prev_p->next_p = next_p;
      }
    }
    else
    {
      prev_p = module_p;
    }

    module_p = next_p;
  }
} /* jjs_module_free */

/**
 * Initialize the default module manager.
 */
static void
jjs_module_manager_init (void *user_data_p)
{
  ((jjs_module_manager_t *) user_data_p)->module_head_p = NULL;
} /* jjs_module_manager_init */

/**
 * Deinitialize the default module manager.
 */
static void
jjs_module_manager_deinit (void *user_data_p) /**< context pointer to deinitialize */
{
  jjs_value_t undef = jjs_undefined ();
  jjs_module_free ((jjs_module_manager_t *) user_data_p, undef);
  jjs_value_free (undef);
} /* jjs_module_manager_deinit */

/**
 * Declare the context data manager for modules.
 */
static const jjs_context_data_manager_t jjs_module_manager JJS_ATTR_CONST_DATA = {
  .init_cb = jjs_module_manager_init,
  .deinit_cb = jjs_module_manager_deinit,
  .bytes_needed = sizeof (jjs_module_manager_t)
};

#endif /* JJS_MODULE_SYSTEM */

/**
 * Default module resolver.
 *
 * @return a module object if resolving is successful, an error otherwise
 */
jjs_value_t
jjs_module_resolve (const jjs_value_t specifier, /**< module specifier string */
                      const jjs_value_t referrer, /**< parent module */
                      void *user_p) /**< user data */
{
#if JJS_MODULE_SYSTEM
  JJS_UNUSED (user_p);

  const jjs_char_t *directory_p = lit_get_magic_string_utf8 (LIT_MAGIC_STRING__EMPTY);
  jjs_size_t directory_size = lit_get_magic_string_size (LIT_MAGIC_STRING__EMPTY);

  jjs_module_t *module_p = jjs_object_get_native_ptr (referrer, &jjs_module_native_info);

  if (module_p != NULL)
  {
    directory_p = module_p->path_p;
    directory_size = module_p->basename_offset;
  }

  jjs_size_t specifier_size = jjs_string_size (specifier, JJS_ENCODING_UTF8);
  jjs_size_t reference_size = directory_size + specifier_size;
  jjs_char_t *reference_path_p = jjs_heap_alloc (reference_size + 1);

  memcpy (reference_path_p, directory_p, directory_size);
  jjs_string_to_buffer (specifier, JJS_ENCODING_UTF8, reference_path_p + directory_size, specifier_size);
  reference_path_p[reference_size] = '\0';

  jjs_char_t *path_p = jjs_port_path_normalize (reference_path_p, reference_size);
  jjs_heap_free (reference_path_p, reference_size + 1);

  if (path_p == NULL)
  {
    return jjs_throw_sz (JJS_ERROR_SYNTAX, "Failed to resolve module");
  }

  jjs_value_t realm = jjs_current_realm ();

  jjs_module_manager_t *manager_p;
  manager_p = (jjs_module_manager_t *) jjs_context_data (&jjs_module_manager);

  module_p = manager_p->module_head_p;

  while (module_p != NULL)
  {
    if (module_p->realm == realm && strcmp ((const char *) module_p->path_p, (const char *) path_p) == 0)
    {
      jjs_value_free (realm);
      jjs_port_path_free (path_p);
      return jjs_value_copy (module_p->module);
    }

    module_p = module_p->next_p;
  }

  jjs_size_t source_size;
  jjs_char_t *source_p = jjs_port_source_read ((const char *) path_p, &source_size);

  if (source_p == NULL)
  {
    jjs_value_free (realm);
    jjs_port_path_free (path_p);

    return jjs_throw_sz (JJS_ERROR_SYNTAX, "Module file not found");
  }

  jjs_parse_options_t parse_options;
  parse_options.options = JJS_PARSE_MODULE | JJS_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jjs_value_copy (specifier);

  jjs_value_t ret_value = jjs_parse (source_p, source_size, &parse_options);
  jjs_value_free (parse_options.source_name);

  jjs_port_source_free (source_p);

  if (jjs_value_is_exception (ret_value))
  {
    jjs_port_path_free (path_p);
    jjs_value_free (realm);
    return ret_value;
  }

  module_p = (jjs_module_t *) jjs_heap_alloc (sizeof (jjs_module_t));

  module_p->next_p = manager_p->module_head_p;
  module_p->path_p = path_p;
  module_p->basename_offset = jjs_port_path_base (module_p->path_p);
  module_p->realm = realm;
  module_p->module = jjs_value_copy (ret_value);

  jjs_object_set_native_ptr (ret_value, &jjs_module_native_info, module_p);
  manager_p->module_head_p = module_p;

  return ret_value;
#else /* JJS_MODULE_SYSTEM */
  JJS_UNUSED (specifier);
  JJS_UNUSED (referrer);
  JJS_UNUSED (user_p);

  return jjs_throw_sz (JJS_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_resolve */

/**
 * Release known modules.
 */
void
jjs_module_cleanup (const jjs_value_t realm) /**< if this argument is object, release only those modules,
                                                  *   which realm value is equal to this argument. */
{
#if JJS_MODULE_SYSTEM
  jjs_module_free ((jjs_module_manager_t *) jjs_context_data (&jjs_module_manager), realm);
#else /* JJS_MODULE_SYSTEM */
  JJS_UNUSED (realm);
#endif /* JJS_MODULE_SYSTEM */
} /* jjs_module_cleanup */
