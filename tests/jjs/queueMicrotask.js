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

const { assertThrows } = require('../lib/assert.js');

const nonFunctionValues = [
  undefined,
  null,
  true,
  false,
  1,
  1n,
  1.1,
  'string',
  Symbol(),
  {},
  [],
];

// test: queueMicrotask() should throw TypeError when passed no arguments
assertThrows(TypeError, queueMicrotask);

// test: queueMicrotask() should throw TypeError when passed a non-function arguments
for (const value of nonFunctionValues) {
  assertThrows(TypeError, () => queueMicrotask(value));
}

// test: queueMicrotask() should call callbacks in FIFO order
let called = false;

queueMicrotask(() => { called = true });
queueMicrotask(() => { assert(called, "expected microtask to be called") });
