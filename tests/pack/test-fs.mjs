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

// check that fs is importable
import path from 'jjs:fs';

// check that fs is requirable
const { file, write } = require('jjs:fs');

const { assertEquals, assertThrows } = require('../lib/assert.js');
const { test, runAllTests } = require('../lib/test.cjs');

test('file() should return a JJSFile object', () => {
  const f = file('fixtures/test.bin');

  assertEquals(typeof f, 'object');
  assertEquals(f[Symbol.toStringTag], 'JJSFile');
});

test('file() should throw if path is not a string', () => {
  for (const f of [null, undefined, {}, [], true, Symbol('test'), 123, NaN]) {
    assertThrows(TypeError, () => file(f));
  }
});

test('file() should throw if path is an empty string', () => {
  assertThrows(TypeError, () => file(''));
});

test('size should return size of test.bin', () => {
  assertEquals(file('fixtures/test.bin').size, 3000);
});

test('size should return 0 when file does not exist', () => {
  assertEquals(file('file-not-found').size, 0);
});

test('json() should read JSON file into JS object', () => {
  assertEquals(file('fixtures/test.json').json().key, 'test');
});

test('json() should throw if file is not valid JSON', () => {
  assertThrows(SyntaxError, () => file('fixtures/test.bin').json());
});

test('json() should throw if file does not exist', () => {
  assertThrows(Error, () => file('does-not-exist').json());
});

test('text() should read UTF-8 encoded text file into a string', () => {
  const f = file('test-fs.mjs');
  const size = f.size;
  const contents = f.text();

  assertEquals(typeof contents, 'string');
  assertEquals(contents.length, size);
});

test('text() should throw if file is not valid UTF8', () => {
  assertThrows(Error, () => file('fixtures/test.bin').text());
});

test('text() should throw if file does not exist', () => {
  assertThrows(Error, () => file('does-not-exist').text());
});

test('arrayBuffer() should read file contents into an ArrayBuffer', () => {
  const f = file('fixtures/test.bin');
  const size = f.size;
  const contents = f.arrayBuffer();

  assert(contents instanceof ArrayBuffer);
  assertEquals(contents.byteLength, size);
});

test('arrayBuffer() should throw if file does not exist', () => {
  assertThrows(Error, () => file('does-not-exist').arrayBuffer());
});

test('write() should write a string to file (UTF-8 encoding)', () => {
  let path = 'temp.txt';
  let text = 'hello world';
  let contents;

  try {
    write(path, text);
    contents = file(path).text();
  } finally {
    deleteFile(path);
  }

  assertEquals(contents, text);
});

test('write() should write a buffer to file', () => {
  let path = 'temp.bin';
  let data = Uint8Array.of(0x68, 0x65, 0x6c, 0x6c, 0x6f);
  let contents;

  try {
    write(path, data);
    contents = file(path).arrayBuffer();
  } finally {
    deleteFile(path);
  }

  contents = new Uint8Array(contents);

  assertEquals(contents.length, data.length);
  data.forEach((value, index) => assertEquals(contents[index], value));
});

function deleteFile(path) {
  try {
    file(path).remove();
  } catch (e) {
    // ignore
  }
}

runAllTests();
