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

#ifndef MAIN_OPTIONS_H
#define MAIN_OPTIONS_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Standalone JJS exit codes
 */
#define JJS_STANDALONE_EXIT_CODE_OK   (0)
#define JJS_STANDALONE_EXIT_CODE_FAIL (1)

/**
 * Argument option flags.
 */
typedef enum
{
  OPT_FLAG_EMPTY = 0,
  OPT_FLAG_PARSE_ONLY = (1 << 0),
  OPT_FLAG_DEBUG_SERVER = (1 << 1),
  OPT_FLAG_WAIT_SOURCE = (1 << 2),
  OPT_FLAG_NO_PROMPT = (1 << 3),
  OPT_FLAG_USE_STDIN = (1 << 4),
  OPT_FLAG_JJS_TEST = (1u << 5),
} main_option_flags_t;

/**
 * Source types
 */
typedef enum
{
  SOURCE_SNAPSHOT,
  SOURCE_MODULE,
  SOURCE_SCRIPT,
  SOURCE_UNKNOWN,
} main_source_type_t;

/**
 * Input source file.
 */
typedef struct
{
  uint32_t path_index;
  uint16_t snapshot_index;
  uint16_t type;
} main_source_t;

/**
 * Method for parsing source from stdin.
 */
typedef enum
{
  INPUT_TYPE_SLOPPY_MODE,
  INPUT_TYPE_STRICT_MODE,
  INPUT_TYPE_MODULE,
} main_input_type_t;

/**
 * Arguments struct to store parsed command line arguments.
 */
typedef struct
{
  main_source_t *sources_p;
  uint32_t source_count;

  const char *debug_channel;
  const char *debug_protocol;
  const char *debug_serial_config;
  uint16_t debug_port;

  uint16_t option_flags;
  uint32_t init_flags;
  uint8_t parse_result;
  main_input_type_t input_type;
  const char *stdin_filename;

  const char *pmap_filename;
} main_args_t;

bool main_parse_args (int argc, char **argv, main_args_t *arguments_p);

#endif /* !MAIN_OPTIONS_H */
