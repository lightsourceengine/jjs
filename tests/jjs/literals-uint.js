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

const { test } = require ('jjs:test');

/*
 * If a string is digits and the parsed number is < ECMA_DIRECT_STRING_MAX_IMM, the
 * integer value is packed into the reference. For values between ECMA_DIRECT_STRING_MAX_IMM
 * and UINT_MAX, a special string/uint hybrid value is created in the vm (to optimize
 * array index lookup use cases) . For higher values, a normal string is created.
 *
 * JS cannot verify the type of value created. These tests run the boundary conditions,
 * but expect the C code to check memory leaks, etc.
 */

test('should create direct uint value for value under ECMA_DIRECT_STRING_MAX_IMM', () => {
  const i = '1000';

  // do busy work with i
  assert(typeof i === 'string');
});

test('should create new number for parsed number value over ECMA_DIRECT_STRING_MAX_IMM', () => {
  const i = '134217728';

  // do busy work with i
  assert(typeof i === 'string');
});

test('should create new number for parsed number value over ECMA_DIRECT_STRING_MAX_IMM', () => {
  const i = '134217728';

  // do busy work with i
  assert(typeof i === 'string');
});

test('should create new number for parsed number value over UINT_MAX', () => {
  const i = '4294967296'; /* 0xFFFFFFFF + 1 */

  // do busy work with i
  assert(typeof i === 'string');
});

test('should create string - ECMA_DIRECT_STRING_MAX_IMM - stress test', () => {
  const base = 134217728;

  for (let i = 0; i < 1000; i++) {
    eval(`const x = '${base + i}';`);
  }
});

test('should create strings - UINT_MAX  - stress test', () => {
  const base = 0xFFFFFFFF;

  for (let i = 0; i < 1000; i++) {
    eval(`const x = '${base + i}';`);
  }
});
