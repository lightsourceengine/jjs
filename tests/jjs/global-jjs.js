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
const TEST_FILE = './global-jjs.js';

test('jjs should contain version, os and arch strings', () => {
  assert (typeof globalThis.jjs === 'object');
  assert (typeof jjs.version === 'string');
  assert (typeof jjs.os === 'string');
  assert (typeof jjs.arch === 'string');
});

test('jjs.cwd() should return the current working directory', () => {
  assert(typeof jjs.cwd() === 'string');
});

test('jjs.realpath() should resolve a file path', () => {
  assert(typeof jjs.realpath('.') === 'string');
});

test('jjs.realpath() should throw Error if file not found', () => {
  const paths = ['', './does-not-exist', 'blahhhh.js'];

  for (const path of paths) {
    assertThrows(Error, () => jjs.realpath(path));
  }
});

test('jjs.realpath() should throw TypeError if not a string path', () => {
  const paths = [ null, undefined, 4, NaN, {}, [], Symbol(), true ];

  for (const path of paths) {
    assertThrows(TypeError, () => jjs.realpath(path));
  }
});

test('jjs.readFile() should read test file as a string', () => {
  const encodings = [
    'utf8',
    'UTF8',
    'utf-8',
    'UTF-8',
    'cesu8',
    'CESU8',
    'cesu-8',
    'CESU-8',
  ];

  for (const encoding of encodings) {
    assert(typeof jjs.readFile(TEST_FILE, encoding) === 'string');
    assert(typeof jjs.readFile(TEST_FILE, { encoding }) === 'string');
  }
});

test('jjs.readFile() should read test file as a buffer', () => {
  const encodings = [
    undefined,
    'none',
    'NONE',
  ];

  assert(jjs.readFile(TEST_FILE) instanceof ArrayBuffer);

  for (const encoding of encodings)
  {
    assert(jjs.readFile(TEST_FILE, encoding) instanceof ArrayBuffer);
    assert(jjs.readFile(TEST_FILE, { encoding }) instanceof ArrayBuffer);
  }
});

test('jjs.vmod() should be callback', () => {
  jjs.vmod('test-package', { exports: () => {} });
});

test('jjs.pmap.resolve() should be callback', () => {
  assertThrows(Error, () => jjs.pmap.resolve('pmap-package'));
});

test('jjs.gc() should be callable', () => {
  jjs.gc();
  jjs.gc(true);
  jjs.gc(false);
});

test('jjs.stdout.write() should be callable', () => {
  jjs.stdout.write();
  jjs.stdout.write('');
  jjs.stdout.write({});
});

test('jjs.stdout.flush() should be callable', () => {
  jjs.stdout.flush();
});

test('jjs.stderr.write() should be callable', () => {
  jjs.stderr.write();
  jjs.stderr.write('');
  jjs.stderr.write({});
});

test('jjs.stderr.flush() should be callable', () => {
  jjs.stderr.flush();
});

runAllTests();
