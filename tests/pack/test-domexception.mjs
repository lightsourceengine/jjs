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

const { test, runAllTests } = require('../lib/test.cjs');
const { assertThrows } = require('../lib/assert.js');

// TODO: use tests from wpt?

test('DOMException', () => {
  const e = new DOMException('test');
  assert(e.name === 'Error');
  assert(e.message === 'test');
  assert(e.code === 0);
});

test('DOMException', () => {
  const e = new DOMException('test', 'SyntaxError');
  assert(e.name === 'SyntaxError');
  assert(e.message === 'test');
  assert(e.code === DOMException.SYNTAX_ERR);
});

test('DOMException', () => {
  const e = new DOMException(null);
  assert(e.name === 'Error');
  assert(e.message === 'null');
  assert(e.code === 0);
});

test('DOMException', () => {
  const e = new DOMException();
  assert(e.name === 'Error');
  assert(e.message === '');
  assert(e.code === 0);
});

test('DOMException', () => {
  assertThrows(TypeError, () => new DOMException(Symbol()));
});

runAllTests();
