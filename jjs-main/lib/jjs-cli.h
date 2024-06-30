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

typedef enum
{
  JJS_CLI_LOADER_UNKNOWN,
  JJS_CLI_LOADER_AUTO,
  JJS_CLI_LOADER_ESM,
  JJS_CLI_LOADER_CJS,
  JJS_CLI_LOADER_JS,
  JJS_CLI_LOADER_SNAPSHOT,
} jjs_cli_loader_t;

typedef enum
{
  JJS_CLI_PARSER_UNKNOWN,
  JJS_CLI_PARSER_SLOPPY,
  JJS_CLI_PARSER_STRICT,
  JJS_CLI_PARSER_MODULE,
  JJS_CLI_PARSER_SNAPSHOT,
} jjs_cli_parser_t;

jjs_char_t *jjs_cli_stdin_readline (jjs_size_t *out_size_p);

void jjs_cli_assert (bool condition, const char* message);

void jjs_cli_fmt_info (jjs_context_t *context, const char *format, jjs_size_t count , ...);
void jjs_cli_fmt_error (jjs_context_t *context, const char *format, jjs_size_t count, ...);

jjs_cli_loader_t jjs_cli_loader_from_string (const char *value);
jjs_cli_parser_t jjs_cli_parser_from_string (const char *value);

#endif /* !JJS_CLI_H */

#ifdef JJS_CLI_IMPLEMENTATION

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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
  const jjs_size_t args_size = (jjs_size_t) (sizeof (format_args) / sizeof (format_args[0]));

  assert ((jjs_size_t) count < args_size);

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
  const jjs_size_t args_size = (jjs_size_t) (sizeof (format_args) / sizeof (format_args[0]));

  assert ((jjs_size_t) count < args_size);

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
  if (strcmp ("esm", value) == 0)
  {
    return JJS_CLI_LOADER_ESM;
  }
  else if (strcmp ("auto", value) == 0)
  {
    return JJS_CLI_LOADER_AUTO;
  }
  else if (strcmp ("cjs", value) == 0)
  {
    return JJS_CLI_LOADER_CJS;
  }
  else if (strcmp ("js", value) == 0)
  {
    return JJS_CLI_LOADER_JS;
  }
  else if (strcmp ("snapshot", value) == 0)
  {
    return JJS_CLI_LOADER_SNAPSHOT;
  }

  return JJS_CLI_LOADER_UNKNOWN;
}

jjs_cli_parser_t
jjs_cli_parser_from_string (const char *value)
{
  if (strcmp ("sloppy", value) == 0)
  {
    return JJS_CLI_PARSER_SLOPPY;
  }
  else if (strcmp ("strict", value) == 0)
  {
    return JJS_CLI_PARSER_STRICT;
  }
  else if (strcmp ("module", value) == 0)
  {
    return JJS_CLI_PARSER_MODULE;
  }
  else if (strcmp ("snapshot", value) == 0)
  {
    return JJS_CLI_PARSER_SNAPSHOT;
  }

  return JJS_CLI_PARSER_UNKNOWN;
}

#endif /* JJS_CLI_IMPLEMENTATION */
