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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jjs-port.h"
#include "jjs.h"

#include "jjs-ext/handlers.h"
#include "jjs-ext/properties.h"
#include "shell.h"

/**
 * Standalone JJS exit codes
 */
#define JJS_STANDALONE_EXIT_CODE_OK   (0)
#define JJS_STANDALONE_EXIT_CODE_FAIL (1)

/**
 * JJS simple test
 */
int
test_jjs (int argc, char **argv)
{
  /* Suppress compiler errors */
  (void) argc;
  (void) argv;

  jjs_value_t ret_value = jjs_undefined ();

  const jjs_char_t script[] = "print ('Hello, World!');";
  printf ("This test run the following script code: [%s]\n\n", script);

  /* Initialize engine */
  jjs_init (JJS_INIT_EMPTY);

  /* Register the print function in the global object. */
  jjsx_register_global ("print", jjsx_handler_print);

  /* Setup Global scope code */
  ret_value = jjs_parse (script, sizeof (script) - 1, NULL);

  if (!jjs_value_is_exception (ret_value))
  {
    /* Execute the parsed source code in the Global scope */
    ret_value = jjs_run (ret_value);
  }

  int ret_code = JJS_STANDALONE_EXIT_CODE_OK;

  if (jjs_value_is_exception (ret_value))
  {
    printf ("Script Error!");

    ret_code = JJS_STANDALONE_EXIT_CODE_FAIL;
  }

  jjs_value_free (ret_value);

  /* Cleanup engine */
  jjs_cleanup ();

  return ret_code;

} /* test_jjs */

const shell_command_t shell_commands[] = { { "test", "JJS Hello World test", test_jjs },
                                           { NULL, NULL, NULL } };

int
main (void)
{
  union
  {
    double d;
    unsigned u;
  } now = { .d = jjs_port_current_time () };
  srand (now.u);
  printf ("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
  printf ("This board features a(n) %s MCU.\n", RIOT_MCU);

  /* start the shell */
  char line_buf[SHELL_DEFAULT_BUFSIZE];
  shell_run (shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

  return 0;
}
