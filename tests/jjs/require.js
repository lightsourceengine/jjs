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

assert(require === globalThis.require, 'require !== globalThis.require');

const { test, runAllTests } = require('../lib/test.cjs');
const { assertThrows } = require('../lib/assert.js');

$jjs.jjs_pmap_from_file('./fixtures/pmap/pmap.json');

test('require() should load require, module, exports, __filename and __dirname into a module scope', () => {
  // note: this file is not a module. scope checks will happen in the required module
  require('./fixtures/test-require-scope.js');
});

test('require() should load a module from a relative path', () => {
  const exports = require('./fixtures/pkg.cjs');

  assert(typeof exports === 'object');
  assert(exports.x === 5);
});

test('require() should load a module from a package name', () => {
  const exports = require('pkg');

  assert(typeof exports === 'object');
  assert(exports.x === 5);
});

test('require() should resolve file from module system path defined in pmap', () => {
  const exports = require('@test/pkg2/a.js');

  assert(exports === 'commonjs.a', `${exports} !== commonjs.a`);
});

test('require() should throw Error when a package name does not exist in the pmap', () => {
  assertThrows(Error, () => { require('unknown'); });
});

test('require() should throw Error when a relative filename does not exist', () => {
  assertThrows(Error, () => { require('./unknown'); });
});

test('require() should throw Error when module has circular dependencies', () => {
  assertThrows(Error, () => { require('./fixtures/circular.cjs'); });
});

test('require() should throw Error for non-string specifier', () => {
  const inputs = [23, {}, [], null, undefined, Symbol(), NaN];

  inputs.forEach(input => assertThrows(TypeError, () => { require(input); }));
});

runAllTests();
