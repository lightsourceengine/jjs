#include "main-module.h"

#include "jjs-port.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
static char*
platform_dirname(char* path_p);
static bool
platform_is_absolute_path(const char* path_p, size_t path_len);

#define platform_realpath(path_p) _fullpath(NULL, path_p, 0)
#else
#include <libgen.h>

#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#define platform_realpath(path_p) realpath(path_p, NULL)
static char*
platform_dirname(char* path_p);
static bool
platform_is_absolute_path(const char* path_p, size_t path_len);
#endif

static void
cache_data_init (void *data)
{
  jjs_value_t* value_p = data;
  *value_p = jjs_object();
} /* cache_data_init */

static void
cache_data_deinit (void *data)
{
  jjs_value_t* value_p = data;
  jjs_value_free (*value_p);
} /* cache_data_deinit */

static jjs_context_data_manager_t
main_module_cache_data_id = {
  .bytes_needed = sizeof(jjs_value_t),
  .finalize_cb = NULL,
  .init_cb = cache_data_init,
  .deinit_cb = cache_data_deinit,
};

typedef struct raw_source_s
{
  jjs_char_t* buffer_p;
  jjs_size_t buffer_size;
  jjs_value_t status;
} raw_source_t;

typedef struct on_resolve_options_s
{
  bool allow_regular_file_name_specifier;
} on_resolve_options_t;

static on_resolve_options_t default_on_resolve_options = {
  .allow_regular_file_name_specifier = false,
};

static on_resolve_options_t main_on_resolve_options = {
  .allow_regular_file_name_specifier = true,
};

static jjs_value_t
on_import (const jjs_value_t specifier, const jjs_value_t user_value, void *user_p);
static jjs_value_t
on_resolve (const jjs_value_t specifier, const jjs_value_t referrer, void *user_p);
static char*
resolve_path(jjs_value_t referrer, jjs_value_t specifier, on_resolve_options_t* options_p);
static jjs_value_t
jjs_string_utf8_sz(const char* str);
static jjs_value_t
get_module_cache(void);
static raw_source_t
read_source (const char* path_p);
static void
free_source (raw_source_t* source);

void
main_module_init (void)
{
  jjs_module_on_import (on_import, NULL);
} /* main_module_init */

void
main_module_cleanup (void)
{
  jjs_module_cleanup (jjs_undefined());
} /* main_module_cleanup */

/**
 * Run a module from a file.
 *
 * @param path_p filename of module to run
 * @return evaluation result or exception (read file, parse, JS runtime error)
 */
jjs_value_t
main_module_run_esm (const char* path_p) {
  jjs_value_t user_value = jjs_string_utf8_sz(path_p);
  jjs_value_t module = on_resolve (user_value, jjs_undefined(), &main_on_resolve_options);

  jjs_value_free (user_value);

  if (jjs_value_is_exception (module))
  {
    return module;
  }

  jjs_value_t link_result = jjs_module_link (module, on_resolve, &default_on_resolve_options);

  if (jjs_value_is_exception (link_result))
  {
    jjs_value_free (module);

    return link_result;
  }

  jjs_value_t evaluate_result = jjs_module_evaluate (module);

  jjs_value_free (link_result);
  jjs_value_free (module);

  return evaluate_result;
} /* main_module_run_esm */

/**
 * Run a non-ESM file.
 */
jjs_value_t
main_module_run (const char* path_p)
{
  raw_source_t source = read_source (path_p);

  if (jjs_value_is_exception (source.status))
  {
    return source.status;
  }

  jjs_value_t path_value = jjs_string_utf8_sz(path_p);
  char* user_value_p = resolve_path (jjs_undefined(), path_value, &main_on_resolve_options);

  jjs_parse_options_t parse_options = {
    .options = JJS_PARSE_HAS_SOURCE_NAME | JJS_PARSE_HAS_USER_VALUE,
    .source_name = path_value,
    .user_value = jjs_string_utf8_sz (user_value_p),
  };

  jjs_value_t result;
  jjs_value_t parse_result = jjs_parse (source.buffer_p, source.buffer_size, &parse_options);

  if (jjs_value_is_exception (parse_result))
  {
    result = parse_result;
  }
  else
  {
    result = jjs_run (parse_result);
    jjs_value_free (parse_result);
  }

  jjs_value_free (parse_options.source_name);
  jjs_value_free (parse_options.user_value);
  free (user_value_p);
  free_source(&source);

  return result;
} /* main_module_run */

/**
 * Convert a JJS string to a UTF-8 encoded C string.
 *
 * memory: caller must free() the returned string
 * error: NULL
 */
static char* read_string(jjs_value_t value)
{
  jjs_size_t size = jjs_string_size (value, JJS_ENCODING_UTF8);

  if (size > 0)
  {
    char* buffer = malloc(size + 1);
    jjs_size_t written = jjs_string_to_buffer(value, JJS_ENCODING_UTF8, (uint8_t*)buffer, size);
    buffer[written] = '\0';
    return buffer;
  }

  return NULL;
} /* read_string */

/**
 * Gets the dirname of a module, filename or undefined (in which case the
 * current working directory is returned).
 *
 * memory: caller must free() the returned string
 * error: NULL
 */
static char*
get_referrer_dirname(jjs_value_t referrer)
{
  char* referrer_path;

  if (jjs_value_is_undefined (referrer))
  {
    referrer_path = NULL;
  }
  else if (jjs_value_is_string(referrer))
  {
    referrer_path = read_string (referrer);

    if (referrer_path == NULL)
    {
      return NULL;
    }
  }
  else if (jjs_value_is_object(referrer))
  {
    jjs_value_t user_value = jjs_source_user_value(referrer);

    referrer_path = read_string (user_value);

    jjs_value_free (user_value);

    if (referrer_path == NULL)
    {
      return NULL;
    }
  }
  else
  {
    return NULL;
  }

  char* result = platform_dirname(referrer_path);

  free(referrer_path);

  return result;
} /* get_referrer_dirname */

/**
 * Resolves a filename specifier with respect to a referring module.
 *
 * For ESM importers, referrer will be a module. dirname of the module
 * filename (module.user_value) will be used as the base for resolving
 * the specifier.
 *
 * For non-ESM importers, referred will be a string to the module/scripts
 * filename. dirname of the module filename (module.user_value) will be used
 * as the base for resolving the specifier.
 *
 * For other cases, referrer will be undefined. In that case, assume that the
 * import is at the top level and use the current working directory as
 * the base.
 *
 * The specifier must be relative (./ or ../) or absolute (starts with /). In
 * the case of an absolute path, the referrer is ignored.
 *
 * memory: caller must free() the returned string
 * error: NULL
 */
static char*
resolve_path(jjs_value_t referrer, jjs_value_t specifier, on_resolve_options_t* options_p)
{
  char* specifier_path_p = read_string (specifier);

  if (specifier_path_p == NULL)
  {
    return NULL;
  }

  size_t specifier_path_len = strlen(specifier_path_p);

  if (platform_is_absolute_path (specifier_path_p, specifier_path_len))
  {
    char* result = platform_realpath(specifier_path_p);

    free(specifier_path_p);

    return result;
  }

  if (!options_p->allow_regular_file_name_specifier)
  {
    if (strstr (specifier_path_p, "./") == NULL && strstr (specifier_path_p, "../") == NULL)
    {
      free (specifier_path_p);

      return NULL;
    }
  }

  char* referrer_dirname_p = get_referrer_dirname(referrer);
  size_t referer_dirname_len = strlen(referrer_dirname_p);
  size_t path_separator_len = strlen(PATH_SEPARATOR_STR);
  size_t len = referer_dirname_len + specifier_path_len + path_separator_len + 1;
  char* buffer_p = malloc(len);
  char* p = buffer_p;

  memcpy (p, referrer_dirname_p, referer_dirname_len);
  p += referer_dirname_len;
  memcpy (p, PATH_SEPARATOR_STR, path_separator_len);
  p += path_separator_len;
  memcpy (p, specifier_path_p, specifier_path_len);
  p [specifier_path_len] = '\0';

  char* result = platform_realpath(buffer_p);

  free(buffer_p);
  free(specifier_path_p);
  free(referrer_dirname_p);

  return result;
} /* resolve_path */

/**
 * Callback that is invoked when a script calls import().
 *
 * @param specifier specifier string passed to import()
 * @param user_value user_value from the calling script
 * @param user_p not used
 *
 * success: JJS module object
 * error: JJS exception
 */
static jjs_value_t
on_import (const jjs_value_t specifier, const jjs_value_t user_value, void *user_p)
{
  (void)user_p;

  jjs_value_t module = on_resolve (specifier, user_value, &default_on_resolve_options);

  if (jjs_value_is_exception (module))
  {
    return module;
  }

  switch (jjs_module_state (module))
  {
    case JJS_MODULE_STATE_UNLINKED:
    {
      jjs_value_t link = jjs_module_link (module, on_resolve, &default_on_resolve_options);

      if (jjs_value_is_exception (link))
      {
        jjs_value_free (module);
        return link;
      }
      break;
    }
    case JJS_MODULE_STATE_LINKED:
    {
      break;
    }
    case JJS_MODULE_STATE_EVALUATED:
    {
      return module;
    }
    default:
    {
      jjs_value_free (module);
      return jjs_throw_sz (JJS_ERROR_COMMON, "import: Module is not linked or evaluated.");
    }

  }

  jjs_value_t evaluate = jjs_module_evaluate (module);

  if (jjs_value_is_exception (evaluate))
  {
    jjs_value_free (module);
    return evaluate;
  }

  jjs_value_free (evaluate);

  return module;
} /* on_import */

/**
 * JJS Resolve Callback used during link() step.
 *
 * @param specifier specifier string from import request
 * @param referrer string or undefined
 * @param user_p not used
 *
 * success: JJS module object
 * error: JJS exception
 */
static jjs_value_t
on_resolve (const jjs_value_t specifier, const jjs_value_t referrer, void *user_p)
{
  (void)user_p;
  jjs_value_t resolved_value = jjs_undefined();
  jjs_value_t has_result = jjs_undefined();
  jjs_value_t module;
  jjs_value_t module_cache = get_module_cache();
  jjs_parse_options_t parse_options = {
    .options = JJS_PARSE_MODULE | JJS_PARSE_HAS_USER_VALUE | JJS_PARSE_HAS_SOURCE_NAME,
  };
  raw_source_t source = { .buffer_p = NULL, .buffer_size = 0, .status = jjs_undefined() };
  char* resolved = resolve_path (referrer, specifier, (on_resolve_options_t*)user_p);

  if (resolved == NULL)
  {
    module = jjs_throw_sz (JJS_ERROR_COMMON, "link: Failed to resolve path.");
    goto cleanup;
  }

  resolved_value = jjs_string_utf8_sz (resolved);

  has_result = jjs_object_has (module_cache, resolved_value);

  if (jjs_value_is_true (has_result))
  {
    module = jjs_object_get (module_cache, resolved_value);
    goto cleanup;
  }

  parse_options.user_value = resolved_value;
  parse_options.source_name = resolved_value;

  source = read_source (resolved);

  if (jjs_value_is_exception (source.status))
  {
    module = source.status;
    goto cleanup;
  }

  module = jjs_parse (source.buffer_p, source.buffer_size, &parse_options);

  if (jjs_value_is_exception (module))
  {
    goto cleanup;
  }
  else
  {
    jjs_value_t set_result = jjs_object_set (module_cache, resolved_value, module);

    if (jjs_value_is_exception (set_result))
    {
      jjs_value_free (module);
      module = set_result;
      goto cleanup;
    }

    jjs_value_free (set_result);
  }

cleanup:
  jjs_value_free (resolved_value);
  jjs_value_free (has_result);
  jjs_value_free (module_cache);
  free_source(&source);
  free(resolved);

  return module;
} /* on_resolve */

/**
 * Get the module cache from the JJS context.
 *
 * success: module cache object
 * error: panic
 */
static jjs_value_t
get_module_cache(void)
{
  jjs_value_t* cache = jjs_context_data (&main_module_cache_data_id);

  if (!cache)
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Failed to get module cache from context.");
    jjs_port_fatal (JJS_FATAL_FAILED_ASSERTION);
  }

  return jjs_value_copy (*cache);
} /* get_module_cache */

/**
 * Creates JJS string from a UTF-8 encoded C string.
 */
static jjs_value_t
jjs_string_utf8_sz(const char* str)
{
  return jjs_string ((const jjs_char_t *) str, (jjs_size_t) strlen (str), JJS_ENCODING_UTF8);
} /* jjs_string_utf8_sz */

/**
 * Read source from a UTF-8 encoded file into a buffer.
 *
 * @param path_p path to file
 *
 * error: JJS exception
 * success: raw_source_t containing buffer and buffer size
 * memory: caller must call free_source() on result
 */
static raw_source_t
read_source (const char* path_p)
{
  jjs_size_t source_size;
  jjs_char_t *source_p = jjs_port_source_read (path_p, &source_size);

  if (source_p == NULL)
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "Failed to open file: %s\n", path_p);
    return (raw_source_t) {
      .buffer_p = NULL,
      .buffer_size = 0,
      .status = jjs_throw_sz (JJS_ERROR_SYNTAX, "Source file not found"),
    };
  }
  else if (!jjs_validate_string (source_p, source_size, JJS_ENCODING_UTF8))
  {
    jjs_port_source_free (source_p);
    return (raw_source_t) {
      .buffer_p = NULL,
      .buffer_size = 0,
      .status = jjs_throw_sz (JJS_ERROR_SYNTAX, "Source is not a valid UTF-8 encoded string."),
    };
  }

  return  (raw_source_t){ .buffer_p = source_p, .buffer_size = source_size, .status = jjs_undefined() };
} /* read_source */

/**
 * Cleanup the result of read_source().
 */
static void
free_source (raw_source_t* source)
{
  jjs_port_source_free (source->buffer_p);
  jjs_value_free (source->status);
} /* free_source */

#ifdef _WIN32

static char*
platform_dirname(char* path_p)
{
  if (path_p == NULL || *path_p == '\0') {
    char* p = ".";
    return strcpy (malloc(strlen(p) + 1), p);
  }

  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];

  errno_t e = _splitpath_s(path_p, drive, sizeof(drive), dir, sizeof(dir), NULL, 0, NULL, 0);

  if (e != 0) {
    jjs_log (JJS_LOG_LEVEL_ERROR, "_splitpath_s failed: %d\n", e);
    jjs_port_fatal (JJS_FATAL_FAILED_ASSERTION);
  } else {
    size_t dirname_len = strlen(drive) + strlen(dir) + 1;
    char* dirname_p = malloc(dirname_len);

    strcpy_s(dirname_p, dirname_len, drive);
    strcat_s(dirname_p, dirname_len, dir);

    size_t last = strlen(dirname_p) - 1;

    if (dirname_p[last] == '/' || dirname_p[last] == '\\') {
      dirname_p[last] = '\0';
    }

    return dirname_p;
  }
} /* platform_dirname */

static bool
platform_is_absolute_path(const char* path_p, size_t path_len)
{
  if (path_p[0] == '/' || path_p[0] == '\\')
  {
    return true;
  }
  else
  {
    return path_len > 2 && isalpha(path_p[0]) && path_p[1] == ':' && (path_p[2] == '/' || path_p[2] == '\\');
  }
} /* platform_is_absolute_path */

#else

static char*
platform_dirname(char* path_p) {
  char* p;

  if (path_p == NULL || *path_p == '\0') {
    p = ".";
  } else {
    p = dirname (path_p);
  }

  return strcpy (malloc(strlen(p) + 1), p);
} /* platform_dirname */

static bool
platform_is_absolute_path(const char* path_p, size_t path_len)
{
  (void) path_len;
  return path_p[0] == '/';
} /* platform_is_absolute_path */

#endif
