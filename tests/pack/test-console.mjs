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

test('console.log() should print a string', () => {
  console.log('console.log(): test');
});

test('console.warn() should print a string', () => {
  console.warn('console.warn(): test');
});

test('console.error() should print a string', () => {
  console.error('console.error(): test');
});

test('console.log() should print a formatted string', () => {
  console.log('%s: %d', 'console.log()', 1);
});

test('console.warn() should print a formatted string', () => {
  console.warn('%s: %d', 'console.warn()', 1);
});

test('console.error() should print a formatted string', () => {
  console.error('%s: %d', 'console.error()', 1);
});

test('console.time() should print a elapsed time with label', () => {
  console.time('console.time()');
  console.timeEnd('console.time()');
});

test('console.time() should throw error for invalid label', () => {
  assertThrows(TypeError, () => { console.time(Symbol()); });
});

test('console.count() should print a count with label', () => {
  console.count('console.count()');
  console.count('console.count()');
  console.count('console.count()');
  console.countReset('console.count()');
});

test('console.count() should throw error for invalid label', () => {
  assertThrows(TypeError, () => { console.count(Symbol()); });
});

runAllTests();
