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

#include "application.h"
#include "jjs.h"

SYSTEM_MODE (MANUAL);

/**
 * Set led value
 */
static jjs_value_t
set_led  (const jjs_value_t func_value, /**< function object */
          const jjs_value_t this_value, /**< this arg */
          const jjs_value_t *args_p, /**< function arguments */
          const jjs_length_t args_cnt) /**< number of function arguments */
{
  if (args_cnt != 2)
  {
    Serial.println ("Wrong arguments count in 'test.setLed' function.");
    return jjs_boolean (false);
  }

  int ledPin = jjs_value_as_number (args_p[0]);
  bool value = jjs_value_is_true (args_p[1]);

  pinMode (ledPin, OUTPUT);
  digitalWrite (ledPin, value);

  return jjs_boolean (true);
} /* set_led */

/**
 * Delay function
 */
static jjs_value_t
js_delay (const jjs_value_t func_value, /**< function object */
          const jjs_value_t this_value, /**< this arg */
          const jjs_value_t *args_p, /**< function arguments */
          const jjs_length_t args_cnt) /**< number of function arguments */
{
  if (args_cnt != 1)
  {
    Serial.println ("Wrong arguments count in 'test.delay' function.");
    return jjs_boolean (false);
  }

  int millisec = jjs_value_as_number (args_p[0]);

  delay (millisec);

  return jjs_boolean (true);
} /* js_delay */

/*
 * Init available js functions
 */
static void
init_jjs ()
{
  jjs_init (JJS_INIT_EMPTY);

  /* Create an empty JS object */
  jjs_value_t object = jjs_object ();

  jjs_value_t func_obj;
  jjs_value_t prop_name;

  func_obj = jjs_function_external (set_led);
  prop_name = jjs_string_sz ("setLed");
  jjs_value_free (jjs_object_set (object, prop_name, func_obj));
  jjs_value_free (prop_name);
  jjs_value_free (func_obj);

  func_obj = jjs_function_external (js_delay);
  prop_name = jjs_string_sz ("delay");
  jjs_value_free (jjs_object_set (object, prop_name, func_obj));
  jjs_value_free (prop_name);
  jjs_value_free (func_obj);

  /* Wrap the JS object (not empty anymore) into a JJS api value */
  jjs_value_t global_object = jjs_current_realm ();

  /* Add the JS object to the global context */
  prop_name = jjs_string_sz ("test");
  jjs_value_free (jjs_object_set (global_object, prop_name, object));
  jjs_value_free (prop_name);
  jjs_value_free (object);
  jjs_value_free (global_object);
} /* init_jjs */

/**
 * JJS simple test
 */
static void
test_jjs ()
{
  const jjs_char_t script[] = " \
    test.setLed(7, true); \
    test.delay(250); \
    test.setLed(7, false); \
    test.delay(250); \
  ";

  jjs_value_t eval_ret = jjs_eval (script, sizeof (script) - 1, JJS_PARSE_NO_OPTS);

  /* Free JavaScript value, returned by eval */
  jjs_value_free (eval_ret);
} /* test_jjs */

/**
 * Setup code for particle firmware
 */
void
setup ()
{
  Serial.begin (9600);
  delay (2000);
  Serial.println ("Beginning Listening mode test!");
} /* setup */

/**
 * Loop code for particle firmware
 */
void
loop ()
{
  init_jjs ();

  /* Turn on and off the D7 LED */
  test_jjs ();

  jjs_cleanup ();
} /* loop */
