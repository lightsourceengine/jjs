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

#ifndef IMCL_H
#define IMCL_H

/*
 * Simple immediate mode commandline argument processor.
 *
 * The design approach is to wrap arguments in a state object and provide an
 * api to shift or pop arguments. The shift api are varied to communicate the
 * intent of processing an arg, such as checking a subcommand or an option.
 * The intent is used to infer how to process the next arg and how to report
 * errors. The processor stores almost no state about processed args or
 * options. The calling code should do appropriate bookkeeping on shifts.
 *
 * When an error occurs, the processor is put into an error state and
 * subsequent calls to shift are no-ops. The calling code should not have
 * to keep checking if the processor is in an error state.
 *
 * This implementation is specific to JJS commandline tools. It is not meant
 * for general purpose use.
 */

/* state of the commandline processing */
typedef struct
{
  /* number of arguments in argv */
  int argc;
  /* array of all arguments */
  char **argv;
  /* currently focused arg in the argv array */
  int index;
  /* is the cursor in the error state? */
  bool has_error;
  /* last seen command */
  const char *command;
  /* last seen option in short (-x) format; NULL if no option recently seen */
  const char *option_short;
  /* last seen option int (--long) format; NULL if no option recently seen */
  const char *option_long;
  /* error message, valid if in error state */
  char *error;
  /* error message must be stdlib free'd */
  bool free_error;
} imcl_args_t;

imcl_args_t imcl_args (int argc, char **argv);
void imcl_args_drop (imcl_args_t *args);

bool imcl_args_has_more (imcl_args_t *args);

const char *imcl_args_shift (imcl_args_t *args);
int32_t imcl_args_shift_int (imcl_args_t *args);
uint32_t imcl_args_shift_uint (imcl_args_t *args);
int32_t imcl_args_shift_ranged_int (imcl_args_t *args, int32_t min, int32_t max);

const char *imcl_args_current (imcl_args_t *args);

bool imcl_args_shift_if_command (imcl_args_t *args, const char *command_name);
bool imcl_args_shift_if_option (imcl_args_t *args, const char *option_short, const char *option_long);
bool imcl_args_shift_if_help_option (imcl_args_t *args);
bool imcl_args_shift_if_version_option (imcl_args_t *args);

#endif /* !IMCL_H */

#ifdef IMCL_IMPLEMENTATION

imcl_args_t
imcl_args (int argc, char **argv)
{
  return (imcl_args_t) {
    .argc = argc,
    .argv = argv,
  };
}

void
imcl_args_drop (imcl_args_t *processor)
{
  if (processor && processor->free_error)
  {
    free (processor->error);
  }
}

bool
imcl_args_has_more (imcl_args_t *args)
{
  return (args->index < args-> argc) && !args->has_error;
}

const char *
imcl_args_shift (imcl_args_t *args)
{
  if (!args->has_error)
  {
    if (args->index < args->argc)
    {
      return args->argv[args->index++];
    }

    args->has_error = true;
  }

  return "";
}

const char *
imcl_args_current (imcl_args_t *args)
{
  return (!args->has_error && args->index < args-> argc) ? args->argv[args->index] : "";
}

int32_t
imcl_args_shift_int (imcl_args_t *args)
{
  const char * value = imcl_args_shift (args);

  if (args->has_error)
  {
    return 0;
  }

  char *endptr;
  long int_value = strtol (value, &endptr, 10);

  if (*endptr != '\0' || int_value > INT32_MAX)
  {
    args->has_error = true;
    return 0;
  }

  return (int32_t) int_value;
}

uint32_t imcl_args_shift_uint (imcl_args_t *args)
{
  const char * value = imcl_args_shift (args);

  if (args->has_error)
  {
    return 0;
  }

  char *endptr;
  long int_value = strtol (value, &endptr, 10);

  if (*endptr != '\0' || int_value > UINT32_MAX)
  {
    args->has_error = true;
    return 0;
  }

  return (uint32_t) int_value;
}

int32_t
imcl_args_shift_ranged_int (imcl_args_t *args, int32_t min, int32_t max)
{
  int32_t value = imcl_args_shift_int (args);

  if (args->has_error)
  {
    return 0;
  }

  if (value < min || value > max)
  {
    args->has_error = true;
    return 0;
  }

  return value;
}

// TODO: shift non-option

bool
imcl_args_shift_if_command (imcl_args_t *args, const char *command_name)
{
  // TODO: check -
  if (imcl_args_has_more (args) && (strcmp (args->argv[args->index], command_name) == 0))
  {
    args->command = command_name;
    args->option_short = NULL;
    args->option_long = NULL;
    imcl_args_shift (args);
    return true;
  }

  return false;
}

bool
imcl_args_shift_if_option (imcl_args_t *args, const char *option_short, const char *option_long)
{
  // TODO: check -
  if (imcl_args_has_more (args)
      && (strcmp (args->argv[args->index], option_short ? option_short : "") == 0
      || strcmp (args->argv[args->index], option_long ? option_long : "") == 0))
  {
    args->option_short = option_short;
    args->option_long = option_long;
    imcl_args_shift (args);
    return true;
  }

  return false;
}

bool
imcl_args_shift_if_help_option (imcl_args_t *args)
{
  return imcl_args_shift_if_option (args, "-h", "--help");
}

bool
imcl_args_shift_if_version_option (imcl_args_t *args)
{
  return imcl_args_shift_if_option (args, "-v", "--version");
}

#endif /* IMCL_IMPLEMENTATION */
