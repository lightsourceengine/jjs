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

#include "jjs-port.h"
#include "jjs.h"

#include "jjs-ext/handlers.h"
#include "jjs-ext/properties.h"
#include "mbed.h"

/**
 * Standalone JJS exit codes
 */
#define JJS_STANDALONE_EXIT_CODE_OK   (0)
#define JJS_STANDALONE_EXIT_CODE_FAIL (1)

#if MBED_MAJOR_VERSION == 5
static Serial serial(USBTX, USBRX, 115200);
#elif MBED_MAJOR_VERSION == 6
static BufferedSerial serial(USBTX, USBRX, 115200);
#else
#error Unsupported Mbed OS version.
#endif

int main()
{
  /* Initialize engine */
  jjs_init (JJS_INIT_EMPTY);

  const jjs_char_t script[] = "print ('Hello, World!');";
  jjs_log (JJS_LOG_LEVEL_DEBUG, "This test run the following script code: [%s]\n\n", script);

  /* Register the print function in the global object. */
  jjsx_register_global ("print", jjsx_handler_print);

  /* Setup Global scope code */
  jjs_value_t ret_value = jjs_parse (script, sizeof (script) - 1, NULL);

  if (!jjs_value_is_exception (ret_value))
  {
    /* Execute the parsed source code in the Global scope */
    ret_value = jjs_run (ret_value);
  }

  int ret_code = JJS_STANDALONE_EXIT_CODE_OK;

  if (jjs_value_is_exception (ret_value))
  {
    jjs_log (JJS_LOG_LEVEL_ERROR, "[Error] Script Error!");

    ret_code = JJS_STANDALONE_EXIT_CODE_FAIL;
  }

  jjs_value_free (ret_value);

  /* Cleanup engine */
  jjs_cleanup ();

  return ret_code;
}
