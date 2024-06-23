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

#include "jjs-test.h"

static jjs_value_t
create_object (const char *source_p) /**< source script */
{
  jjs_value_t result = jjs_eval_sz (ctx (), source_p, 0);
  TEST_ASSERT (jjs_value_is_object (ctx (), result));
  return result;
} /* create_object */

static void
compare_string (jjs_value_t value, /**< value to compare */
                const char *string_p) /**< expected value */
{
  jjs_char_t string_buffer[64];

  TEST_ASSERT (jjs_value_is_string (ctx (), value));

  size_t size = strlen (string_p);
  TEST_ASSERT (size <= sizeof (string_buffer));
  TEST_ASSERT (size == jjs_string_size (ctx (), value, JJS_ENCODING_CESU8));

  jjs_string_to_buffer (ctx (), value, JJS_ENCODING_CESU8, string_buffer, (jjs_size_t) size);
  TEST_ASSERT (memcmp (string_p, string_buffer, size) == 0);
} /* compare_string */

int
main (void)
{
  ctx_open (NULL);

  jjs_value_t pp_string = jjs_string_sz (ctx (), "pp");
  jjs_value_t qq_string = jjs_string_sz (ctx (), "qq");
  jjs_value_t rr_string = jjs_string_sz (ctx (), "rr");

  jjs_value_t object = create_object ("'use strict';\n"
                                        "({ pp:'A', get qq() { return 'B' } })");

  jjs_value_t result = jjs_object_find_own (ctx (), object, pp_string, object, NULL);
  compare_string (result, "A");
  jjs_value_free (ctx (), result);

  bool found = false;
  result = jjs_object_find_own (ctx (), object, pp_string, object, &found);
  compare_string (result, "A");
  TEST_ASSERT (found);
  jjs_value_free (ctx (), result);

  result = jjs_object_find_own (ctx (), object, qq_string, object, NULL);
  compare_string (result, "B");
  jjs_value_free (ctx (), result);

  found = false;
  result = jjs_object_find_own (ctx (), object, qq_string, object, &found);
  compare_string (result, "B");
  TEST_ASSERT (found);
  jjs_value_free (ctx (), result);

  result = jjs_object_find_own (ctx (), object, rr_string, object, NULL);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
  jjs_value_free (ctx (), result);

  found = true;
  result = jjs_object_find_own (ctx (), object, rr_string, object, &found);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
  TEST_ASSERT (!found);
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), object);

  object = create_object ("'use strict';\n"
                          "Object.create({ pp:'Found!' })\n");

  found = true;
  /* Does not check prototype. */
  result = jjs_object_find_own (ctx (), object, pp_string, object, &found);
  TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
  TEST_ASSERT (!found);
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), object);

  object = create_object ("'use strict';\n"
                          "var obj = Object.create({ get pp() { return this.qq } })\n"
                          "Object.defineProperty(obj, 'qq', { value: 'Prop' })\n"
                          "obj");
  jjs_value_t prototype = jjs_object_proto (ctx (), object);

  TEST_ASSERT (jjs_value_is_object (ctx (), prototype));
  found = false;
  result = jjs_object_find_own (ctx (), prototype, pp_string, object, &found);
  compare_string (result, "Prop");
  TEST_ASSERT (found);
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), prototype);
  jjs_value_free (ctx (), object);

  /* Error cases. */
  jjs_value_t invalid_arg = jjs_null (ctx ());
  object = jjs_object (ctx ());

  found = true;
  result = jjs_object_find_own (ctx (), invalid_arg, pp_string, object, &found);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  TEST_ASSERT (!found);
  jjs_value_free (ctx (), result);

  result = jjs_object_find_own (ctx (), object, pp_string, invalid_arg, NULL);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  jjs_value_free (ctx (), result);

  found = true;
  result = jjs_object_find_own (ctx (), object, invalid_arg, object, &found);
  TEST_ASSERT (jjs_value_is_exception (ctx (), result));
  TEST_ASSERT (!found);
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), object);
  jjs_value_free (ctx (), invalid_arg);

  if (jjs_feature_enabled (JJS_FEATURE_PROXY))
  {
    object = create_object ("'use strict';\n"
                            "var proxy = new Proxy({}, {\n"
                            "    get: function(target, prop, receiver) {\n"
                            "        if (prop === 'qq') return\n"
                            "        return receiver[prop]\n"
                            "    }\n"
                            "})\n"
                            "var obj = Object.create(proxy)\n"
                            "Object.defineProperty(obj, 'pp', { value: 'Prop' })\n"
                            "obj");

    prototype = jjs_object_proto (ctx (), object);
    found = false;
    result = jjs_object_find_own (ctx (), prototype, pp_string, object, &found);
    compare_string (result, "Prop");
    TEST_ASSERT (found);
    jjs_value_free (ctx (), result);

    found = false;
    result = jjs_object_find_own (ctx (), prototype, qq_string, object, &found);
    TEST_ASSERT (jjs_value_is_undefined (ctx (), result));
    TEST_ASSERT (found);
    jjs_value_free (ctx (), result);

    jjs_value_free (ctx (), prototype);
    jjs_value_free (ctx (), object);

    object = create_object ("'use strict';\n"
                            "(new Proxy({}, {\n"
                            "    get: function(target, prop, receiver) {\n"
                            "        throw 'Error'\n"
                            "    }\n"
                            "}))\n");

    found = false;
    result = jjs_object_find_own (ctx (), object, qq_string, object, &found);
    TEST_ASSERT (jjs_value_is_exception (ctx (), result));
    TEST_ASSERT (found);
    jjs_value_free (ctx (), result);

    jjs_value_free (ctx (), object);
  }

  object = create_object ("'use strict'\n"
                          "var sym = Symbol();\n"
                          "({ pp:sym, [sym]:'Prop' })");

  found = false;
  jjs_value_t symbol = jjs_object_find_own (ctx (), object, pp_string, object, &found);
  TEST_ASSERT (jjs_value_is_symbol (ctx (), symbol));
  TEST_ASSERT (found);

  found = false;
  result = jjs_object_find_own (ctx (), object, symbol, object, &found);
  compare_string (result, "Prop");
  TEST_ASSERT (found);
  jjs_value_free (ctx (), result);

  jjs_value_free (ctx (), symbol);
  jjs_value_free (ctx (), object);

  jjs_value_free (ctx (), pp_string);
  jjs_value_free (ctx (), qq_string);
  jjs_value_free (ctx (), rr_string);

  ctx_close ();
  return 0;
} /* main */
