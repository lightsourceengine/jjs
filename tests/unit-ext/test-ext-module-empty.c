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

#include <string.h>

#include "jjs.h"

#include "jjs-ext/module.h"
#include "test-common.h"

int
main (int argc, char **argv)
{
  (void) argc;
  (void) argv;
  jjs_char_t buffer[256];
  jjs_size_t bytes_copied;
  const jjsx_module_resolver_t *resolver = &jjsx_module_native_resolver;
  jjs_value_t module_name;

  jjs_init (JJS_INIT_EMPTY);

  /* Attempt to load a non-existing module. */
  module_name = jjs_string_sz ("some-unknown-module-name");
  jjs_value_t module = jjsx_module_resolve (module_name, &resolver, 1);
  jjs_value_free (module_name);

  TEST_ASSERT (jjs_value_is_exception (module));

  /* Retrieve the error message. */
  module = jjs_exception_value (module, true);
  jjs_value_t prop_name = jjs_string_sz ("message");
  jjs_value_t prop = jjs_object_get (module, prop_name);

  /* Assert that the error message is a string with specific contents. */
  TEST_ASSERT (jjs_value_is_string (prop));

  bytes_copied = jjs_string_to_buffer (prop, JJS_ENCODING_UTF8, buffer, sizeof (buffer));
  buffer[bytes_copied] = 0;
  TEST_ASSERT (!strcmp ((const char *) buffer, "Module not found"));

  /* Release the error message property name and value. */
  jjs_value_free (prop);
  jjs_value_free (prop_name);

  /* Retrieve the moduleName property. */
  prop_name = jjs_string_sz ("moduleName");
  prop = jjs_object_get (module, prop_name);

  /* Assert that the moduleName property is a string containing the requested module name. */
  TEST_ASSERT (jjs_value_is_string (prop));

  bytes_copied = jjs_string_to_buffer (prop, JJS_ENCODING_UTF8, buffer, sizeof (buffer));
  buffer[bytes_copied] = 0;
  TEST_ASSERT (!strcmp ((const char *) buffer, "some-unknown-module-name"));

  /* Release everything. */
  jjs_value_free (prop);
  jjs_value_free (prop_name);
  jjs_value_free (module);

  return 0;
} /* main */
