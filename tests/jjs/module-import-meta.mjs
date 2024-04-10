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

const { assertThrows, assertEquals } = require('../lib/assert.js');
const { test, runAllTests } = require('../lib/test.cjs');
const { dirname, filename, url, resolve } = import.meta;

test('import.meta.dirname should be a string dirname of this file', () => {
  assert(typeof dirname === 'string', 'expected import.meta.dirname to be a string');
  assert(dirname.endsWith('jjs'), 'expected import.meta.dirname basename to be this directory');
});

test('import.meta.filename should be a string filename of this file', () => {
  assert(typeof filename === 'string', 'expected import.meta.filename to be a string');
  assert(filename.startsWith(dirname), 'expected import.meta.filename to start with dirname');
  assert(filename.endsWith('module-import-meta.mjs'), 'expected import.meta.filename file to be this filename');
});

test('import.meta.url should be a string file url for this file', () => {
  assert(typeof url === 'string', 'expected import.meta.url to be a string');
  assert(url.startsWith('file:'), 'expected import.meta.url to start with file: protocol');
  assert(url.endsWith('module-import-meta.mjs'), 'expected import.meta.url to end with this filename');
});

test('import.meta.resolve should be a function', () => {
  assert(typeof resolve === 'function', 'expected import.meta.resolve to be a function');
});

test('import.meta.resolve should return the full filename for the relative path to this file', () => {
  assertEquals(resolve('./module-import-meta.mjs'), filename);
});

test('import.meta.resolve should return package name for registered vmod package', () => {
  jjs.vmod('pkg', { exports: 5 });
  assertEquals(resolve('pkg'), 'pkg');
});

test('import.meta.resolve should throw Error when relative filename does not exist', () => {
  assertThrows(Error, () => { resolve('./index.js') });
});

test('import.meta.resolve should throw Error when package name does not exist', () => {
  assertThrows(Error, () => { resolve('not-a-package') });
});

test('import.meta.resolve should throw TypeError given no arguments', () => {
  assertThrows(TypeError, resolve);
});

runAllTests();
