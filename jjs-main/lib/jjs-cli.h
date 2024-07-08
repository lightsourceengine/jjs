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

#ifndef JJS_CLI_H
#define JJS_CLI_H

#include <jjs.h>
#include <stdlib.h>

#define JJS_CLI_EXIT_SUCCESS EXIT_SUCCESS
#define JJS_CLI_EXIT_FAILURE EXIT_FAILURE

typedef enum
{
  JJS_CLI_LOADER_UNDEFINED,
  JJS_CLI_LOADER_ESM,
  JJS_CLI_LOADER_CJS,
  JJS_CLI_LOADER_STRICT,
  JJS_CLI_LOADER_SLOPPY,
  JJS_CLI_LOADER_SNAPSHOT,
  JJS_CLI_LOADER_UNKNOWN,
} jjs_cli_loader_t;

typedef enum
{
  JJS_CLI_ALLOCATOR_STRATEGY_UNDEFINED,
  JJS_CLI_ALLOCATOR_STRATEGY_SYSTEM,
  JJS_CLI_ALLOCATOR_STRATEGY_VM,
  JJS_CLI_ALLOCATOR_STRATEGY_UNKNOWN,
} jjs_cli_allocator_strategy_t;

typedef struct
{
  jjs_context_options_t context_options;
  const char *cwd;
  const char *pmap_filename;
  const char *cwd_filename;
  int32_t log_level;
  bool has_log_level;
  jjs_cli_allocator_strategy_t buffer_allocator_strategy;
  char **argv;
  int argc;
} jjs_cli_config_t;

typedef struct
{
  const char *filename;
  bool from_stdin;
  jjs_cli_loader_t loader;
  jjs_size_t snapshot_index;
  bool is_main;
} jjs_cli_module_t;

typedef struct
{
  jjs_cli_module_t *items;
  size_t size;
  size_t capacity;
} jjs_cli_module_list_t;

jjs_char_t *jjs_cli_stdin_readline (jjs_size_t *out_size_p);
bool jjs_cli_stdin_drain (uint8_t **buffer, jjs_size_t *buffer_size);

void jjs_cli_assert (bool condition, const char* message);

void jjs_cli_fmt_info (jjs_context_t *context, const char *format, jjs_size_t count , ...);
void jjs_cli_fmt_error (jjs_context_t *context, const char *format, jjs_size_t count, ...);

jjs_cli_loader_t jjs_cli_loader_from_string (const char *value);
jjs_cli_allocator_strategy_t jjs_cli_allocator_strategy_from_string (const char *value);

void jjs_cli_engine_drop (jjs_context_t *context_p);
bool jjs_cli_engine_init (const jjs_cli_config_t *config, jjs_context_t **out);

void jjs_cli_module_list_drop (jjs_cli_module_list_t *includes);
void jjs_cli_module_list_append (jjs_cli_module_list_t *includes,
                                 const char *filename,
                                 jjs_cli_loader_t loader,
                                 jjs_size_t snapshot_index);

#endif /* !JJS_CLI_H */

#ifdef JJS_CLI_IMPLEMENTATION

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#if defined (JJS_PACK) && JJS_PACK
#include <jjs-pack.h>
#endif /* defined (JJS_PACK) && JJS_PACK */

#ifdef _WIN32
#include <direct.h>
#ifndef chdir
#define chdir _chdir
#endif
#else
#include <unistd.h>
#endif

bool
jjs_cli_stdin_drain (uint8_t **buffer, jjs_size_t *buffer_size)
{
  const size_t READ_SIZE = 8192;
  uint8_t *local_buffer = NULL;
  size_t bytes_read;
  jjs_size_t local_buffer_size = 0;

  while (true)
  {
    uint8_t *temp = realloc (local_buffer, local_buffer_size + READ_SIZE);

    if (temp == NULL)
    {
      free (local_buffer);
      return false;
    }

    local_buffer = temp;
    bytes_read = fread (local_buffer + local_buffer_size, 1, READ_SIZE, stdin);
    assert (bytes_read <= READ_SIZE);

    if (bytes_read < READ_SIZE)
    {
      local_buffer_size += (jjs_size_t) bytes_read;
      break;
    }
    else
    {
      local_buffer_size += READ_SIZE;
    }
  }

  if (!feof (stdin))
  {
    free (local_buffer);
    return false;
  }

  *buffer = local_buffer;
  *buffer_size = local_buffer_size;

  return true;
}

void
jjs_cli_module_list_drop (jjs_cli_module_list_t *includes)
{
  free (includes->items);
}

void
jjs_cli_module_list_append (jjs_cli_module_list_t *includes,
                             const char *filename,
                             jjs_cli_loader_t loader,
                             jjs_size_t snapshot_index)
{
  if (includes->size == includes->capacity)
  {
    includes->capacity += 8;

    size_t size = includes->capacity * sizeof (includes->items[0]);

    includes->items = includes->items == NULL ? malloc (size) : realloc (includes->items, size);
    jjs_cli_assert (includes->items, "jjs_cli_module_list_append: bad realloc");
  }

  includes->items[includes->size++] = (jjs_cli_module_t){
    .filename = filename,
    .loader = loader,
    .snapshot_index = snapshot_index,
  };
}

static void
stdout_wstream_write (jjs_context_t* context, const jjs_wstream_t *stream, const uint8_t *data, jjs_size_t size)
{
  (void) context;
  (void) stream;
  fwrite (data, 1, size, stdout);
}

static jjs_wstream_t stdout_wstream = {
  .write = stdout_wstream_write,
  .encoding = JJS_ENCODING_UTF8,
};

static void
stderr_wstream_write (jjs_context_t* context, const jjs_wstream_t *stream, const uint8_t *data, jjs_size_t size)
{
  (void) context;
  (void) stream;
  fwrite (data, 1, size, stderr);
}

static jjs_wstream_t stderr_wstream = {
  .write = stderr_wstream_write,
  .encoding = JJS_ENCODING_UTF8,
};

static bool
set_cwd (const char *cwd)
{
  return cwd ? chdir (cwd) == 0 : true;
}

static void
unhandled_rejection_cb (jjs_context_t* context, jjs_value_t promise, jjs_value_t reason, void *user_ptr)
{
  (void) promise;
  (void) user_ptr;
  jjs_cli_fmt_info (context, "Unhandled promise rejection: {}\n", 1, reason);
}

static uint8_t *
system_arraybuffer_allocate (jjs_context_t *context_p,
                             jjs_arraybuffer_type_t buffer_type,
                             uint32_t buffer_size,
                             void **arraybuffer_user_p,
                             void *user_p)
{
  (void) context_p, (void) buffer_type, (void) arraybuffer_user_p, (void) user_p;
  return malloc (buffer_size);
}

static void
system_arraybuffer_free (jjs_context_t *context_p,
                         jjs_arraybuffer_type_t buffer_type,
                         uint8_t *buffer_p,
                         uint32_t buffer_size,
                         void *arraybuffer_user_p,
                         void *user_p)
{
  (void) context_p, (void) buffer_p, (void) buffer_size, (void) buffer_type, (void) arraybuffer_user_p, (void) user_p;
  free (buffer_p);
}

void
jjs_cli_engine_drop (jjs_context_t *context_p)
{
#if defined (JJS_PACK) && JJS_PACK
  jjs_pack_cleanup (context_p);
#endif /* defined (JJS_PACK) && JJS_PACK */
  jjs_context_free (context_p);
}

bool
jjs_cli_engine_init (const jjs_cli_config_t *config, jjs_context_t **out)
{
  srand ((unsigned) time (NULL));
  set_cwd (config->cwd);

  jjs_context_t *context;

  if (jjs_context_new (&config->context_options, &context) != JJS_STATUS_OK)
  {
    return false;
  }

  jjs_promise_on_unhandled_rejection (context, &unhandled_rejection_cb, NULL);

  if (config->has_log_level)
  {
    jjs_log_set_level (context, (jjs_log_level_t) config->log_level);
  }

  if (config->buffer_allocator_strategy == JJS_CLI_ALLOCATOR_STRATEGY_SYSTEM
      || config->buffer_allocator_strategy == JJS_CLI_ALLOCATOR_STRATEGY_UNDEFINED)
  {
    jjs_arraybuffer_allocator (context, &system_arraybuffer_allocate, &system_arraybuffer_free, NULL);
  }

#if defined (JJS_PACK) && JJS_PACK
  jjs_pack_init (context, JJS_PACK_INIT_ALL);
#endif /* defined (JJS_PACK) && JJS_PACK */

  if (config->pmap_filename)
  {
    jjs_value_t result = jjs_pmap (context, jjs_string_sz (context, config->pmap_filename), JJS_MOVE, jjs_undefined (context), JJS_MOVE);

    if (jjs_value_is_exception (context, result))
    {
      jjs_cli_fmt_info (context, "Failed to load pmap: {}\n", 1, result);
      jjs_value_free (context, result);
      jjs_cli_engine_drop (context);
      return false;
    }

    jjs_value_free (context, result);
  }

  jjs_value_t global = jjs_current_realm (context);
  jjs_value_t jjs = jjs_object_get_sz (context, global, "jjs");
  jjs_value_t argv = jjs_array (context, (jjs_size_t) config->argc);
  const jjs_size_t len = jjs_array_length (context, argv);

  for (jjs_size_t i = 0; i < len; i++)
  {
    jjs_value_free (context, jjs_object_set_index (context, argv, i, jjs_string_utf8_sz (context, config->argv[i]), JJS_MOVE));
  }

  jjs_value_free (context, jjs_object_set_sz (context, jjs, "argv", argv, JJS_MOVE));
  jjs_value_free (context, global);
  jjs_value_free (context, jjs);

  *out = context;
  return true;
}

jjs_char_t *
jjs_cli_stdin_readline (jjs_size_t *out_size)
{
  static const jjs_size_t READ_SIZE = 1024;

  jjs_size_t capacity = READ_SIZE;
  char *line = malloc (capacity);
  size_t bytes_read = 0;

  if (!line)
  {
    return NULL;
  }

  while (true)
  {
    if (bytes_read == capacity)
    {
      capacity += READ_SIZE;
      line = realloc (line, capacity);
    }

    int character = fgetc (stdin);

    if ((character == '\n') || (character == EOF))
    {
      line[bytes_read] = 0;
      *out_size = (jjs_size_t) bytes_read;
      return (jjs_char_t *) line;
    }

    line[bytes_read++] = (char) character;
  }
}

void
jjs_cli_fmt_info (jjs_context_t *context, const char *format, jjs_size_t count, ...)
{
  jjs_value_t format_args [4];

  assert ((jjs_size_t) count < (jjs_size_t) (sizeof (format_args) / sizeof (format_args[0])));

  va_list a;
  va_start (a, count);

  for (jjs_size_t i = 0; i < count; i++)
  {
    format_args[i] = va_arg (a, jjs_value_t);
  }

  va_end (a);

  jjs_fmt_v (context, &stdout_wstream, format, format_args, (jjs_size_t) count);
}

void
jjs_cli_fmt_error (jjs_context_t *context, const char *format, jjs_size_t count, ...)
{
  jjs_value_t format_args [4];

  assert ((jjs_size_t) count < (jjs_size_t) (sizeof (format_args) / sizeof (format_args[0])));

  va_list a;
  va_start (a, count);

  for (jjs_size_t i = 0; i < count; i++)
  {
    format_args[i] = va_arg (a, jjs_value_t);
  }

  va_end (a);

  jjs_fmt_v (context, &stderr_wstream, format, format_args, (jjs_size_t) count);
}

void
jjs_cli_assert (bool condition, const char* message)
{
  if (!condition)
  {
    fprintf (stderr, "assertion failed: %s", message ? message : "");
    fputs ("\n", stderr);
    fflush (stderr);

    abort ();
  }
}

jjs_cli_loader_t
jjs_cli_loader_from_string (const char *value)
{
  if (strcmp ("esm", value) == 0 || strcmp ("module", value) == 0)
  {
    return JJS_CLI_LOADER_ESM;
  }
  else if (strcmp ("cjs", value) == 0 || strcmp ("commonjs", value) == 0)
  {
    return JJS_CLI_LOADER_CJS;
  }
  else if (strcmp ("strict", value) == 0)
  {
    return JJS_CLI_LOADER_STRICT;
  }
  else if (strcmp ("sloppy", value) == 0)
  {
    return JJS_CLI_LOADER_SLOPPY;
  }
  else if (strcmp ("snapshot", value) == 0)
  {
    return JJS_CLI_LOADER_SNAPSHOT;
  }

  return JJS_CLI_LOADER_UNKNOWN;
}

jjs_cli_allocator_strategy_t
jjs_cli_allocator_strategy_from_string (const char *value)
{
  if (strcmp ("auto", value) == 0)
  {
    return JJS_CLI_ALLOCATOR_STRATEGY_UNDEFINED;
  }
  else if (strcmp ("vm", value) == 0)
  {
    return JJS_CLI_ALLOCATOR_STRATEGY_VM;
  }
  else if (strcmp ("system", value) == 0)
  {
    return JJS_CLI_ALLOCATOR_STRATEGY_SYSTEM;
  }

  return JJS_CLI_ALLOCATOR_STRATEGY_UNKNOWN;
}

#endif /* JJS_CLI_IMPLEMENTATION */
