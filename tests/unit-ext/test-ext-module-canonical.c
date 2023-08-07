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

#define ACTUAL_NAME "alice"
#define ALIAS_NAME  "bob"

static jjs_value_t
get_canonical_name (const jjs_value_t name)
{
  jjs_size_t name_size = jjs_string_size (name, JJS_ENCODING_CESU8);
  JJS_VLA (jjs_char_t, name_string, name_size + 1);
  jjs_string_to_buffer (name, JJS_ENCODING_CESU8, name_string, name_size);
  name_string[name_size] = 0;

  if (!strcmp ((char *) name_string, ACTUAL_NAME))
  {
    return jjs_value_copy (name);
  }
  else if (!strcmp ((char *) name_string, ALIAS_NAME))
  {
    return jjs_string_sz (ACTUAL_NAME);
  }
  else
  {
    return jjs_undefined ();
  }
} /* get_canonical_name */

static bool
resolve (const jjs_value_t canonical_name, jjs_value_t *result)
{
  jjs_size_t name_size = jjs_string_size (canonical_name, JJS_ENCODING_CESU8);
  JJS_VLA (jjs_char_t, name_string, name_size + 1);
  jjs_string_to_buffer (canonical_name, JJS_ENCODING_CESU8, name_string, name_size);
  name_string[name_size] = 0;

  if (!strcmp ((char *) name_string, ACTUAL_NAME))
  {
    *result = jjs_object ();
    return true;
  }
  return false;
} /* resolve */

static const jjsx_module_resolver_t canonical_test = { .get_canonical_name_p = get_canonical_name,
                                                         .resolve_p = resolve };

#define TEST_VALUE 95.0

int
main (int argc, char **argv)
{
  (void) argc;
  (void) argv;

  const jjsx_module_resolver_t *resolver = &canonical_test;

  jjs_init (JJS_INIT_EMPTY);

  jjs_value_t actual_name = jjs_string_sz (ACTUAL_NAME);
  jjs_value_t alias_name = jjs_string_sz (ALIAS_NAME);

  /* It's important that we resolve by the non-canonical name first. */
  jjs_value_t result2 = jjsx_module_resolve (alias_name, &resolver, 1);
  jjs_value_t result1 = jjsx_module_resolve (actual_name, &resolver, 1);
  jjs_value_free (actual_name);
  jjs_value_free (alias_name);

  /* An elaborate way of doing strict equal - set a property on one object and it "magically" appears on the other. */
  jjs_value_t prop_name = jjs_string_sz ("something");
  jjs_value_t prop_value = jjs_number (TEST_VALUE);
  jjs_value_free (jjs_object_set (result1, prop_name, prop_value));
  jjs_value_free (prop_value);

  prop_value = jjs_object_get (result2, prop_name);
  TEST_ASSERT (jjs_value_as_number (prop_value) == TEST_VALUE);
  jjs_value_free (prop_value);

  jjs_value_free (prop_name);
  jjs_value_free (result1);
  jjs_value_free (result2);

  jjs_cleanup ();

  return 0;
} /* main */
