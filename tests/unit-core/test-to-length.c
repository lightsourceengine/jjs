/*
 * Copyright JS Foundation and other contributors, http://js.foundation
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

#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-helpers-number.h"
#include "ecma-init-finalize.h"

#include "jcontext.h"

/**
 * Unit test's main function.
 */
int
main (void)
{
  jjs_context_t *context_p = ctx_bootstrap (NULL);

  jmem_init (context_p);
  ecma_init (context_p);

  ecma_length_t num;

  ecma_value_t int_num = ecma_make_int32_value (context_p, 123);

  ecma_value_t result = ecma_op_to_length (context_p, int_num, &num);

  ecma_free_value (context_p, int_num);

  TEST_ASSERT (!ECMA_IS_VALUE_ERROR (result));
  TEST_ASSERT (num == 123);

  /* 1, 3 */
  ecma_value_t error_throw = ecma_raise_standard_error (context_p, JJS_ERROR_TYPE, ECMA_ERR_INVALID_ARRAY_LENGTH);

  result = ecma_op_to_length (context_p, error_throw, &num);

  jcontext_release_exception (context_p);

  TEST_ASSERT (ECMA_IS_VALUE_ERROR (result));

  /* zero */
  ecma_value_t zero = ecma_make_int32_value (context_p, 0);

  result = ecma_op_to_length (context_p, zero, &num);

  ecma_free_value (context_p, zero);

  TEST_ASSERT (!ECMA_IS_VALUE_ERROR (result));
  TEST_ASSERT (num == 0);

  /* negative */
  ecma_value_t negative = ecma_make_number_value (context_p, -26.5973f);

  result = ecma_op_to_length (context_p, negative, &num);

  ecma_free_value (context_p, negative);

  TEST_ASSERT (!ECMA_IS_VALUE_ERROR (result));
  TEST_ASSERT (num == 0);

  /* +infinity */
  ecma_value_t positive_infinity = ecma_make_number_value (context_p, ecma_number_make_infinity (false));

  result = ecma_op_to_length (context_p, positive_infinity, &num);

  ecma_free_value (context_p, positive_infinity);

  TEST_ASSERT (!ECMA_IS_VALUE_ERROR (result));
  TEST_ASSERT ((ecma_number_t) num == ECMA_NUMBER_MAX_SAFE_INTEGER);

  /* -infinity */
  ecma_value_t negative_infinity = ecma_make_number_value (context_p, ecma_number_make_infinity (true));

  result = ecma_op_to_length (context_p, negative_infinity, &num);

  ecma_free_value (context_p, negative_infinity);

  TEST_ASSERT (!ECMA_IS_VALUE_ERROR (result));
  TEST_ASSERT (num == 0);

  /* NaN */
  ecma_value_t nan = ecma_make_nan_value (context_p);

  result = ecma_op_to_length (context_p, nan, &num);

  ecma_free_value (context_p, nan);

  TEST_ASSERT (!ECMA_IS_VALUE_ERROR (result));
  TEST_ASSERT (num == 0);

  ecma_finalize (context_p);
  jmem_finalize (context_p);
  ctx_bootstrap_cleanup (context_p);

  return 0;
} /* main */
