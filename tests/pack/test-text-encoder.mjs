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

import { test } from 'jjs:test';

const { assertThrows, assertEquals } = require('../lib/assert.js');

test('check global', () => {
  assert(typeof globalThis.TextEncoder === 'function');
});

test('constructor should create a new object with encoding set to utf-8', () => {
  const encoder = new TextEncoder();
  assert(encoder.encoding === 'utf-8');
});

test('encode() should encode a string to a byte array', () => {
  const result = encode('hello');

  assert(result instanceof Uint8Array);
  assertEquals(result[0], 0x68);
  assertEquals(result[1], 0x65);
  assertEquals(result[2], 0x6c);
  assertEquals(result[3], 0x6c);
  assertEquals(result[4], 0x6f);
});

test('encode() should encode an empty string to an empty byte array', () => {
  const result = encode('');

  assert(result instanceof Uint8Array);
  assertEquals(result.length, 0);
});

test('encode() should encode undefined to an empty byte array', () => {
  const result = encode(undefined);

  assert(result instanceof Uint8Array);
  assertEquals(result.length, 0);
});

test('encode() should return an empty buffer when no argument passed', () => {
  const result = encode();

  assert(result instanceof Uint8Array);
  assertEquals(result.length, 0);
});

test('encode() should throw an error when Symbol is passed', () => {
  assertThrows(TypeError, () => encode(Symbol('hello')));
});

test('encodeInto() should encode a string to a byte array', () => {
  const buffer = new Uint8Array(5);
  const result = encodeInto('hello', buffer);

  assertEquals(result.read, 5);
  assertEquals(result.written, 5);
  assertEquals(buffer[0], 0x68);
  assertEquals(buffer[1], 0x65);
  assertEquals(buffer[2], 0x6c);
  assertEquals(buffer[3], 0x6c);
  assertEquals(buffer[4], 0x6f);
});

test('encodeInto() should encode an empty string to an empty byte array', () => {
  const result = encodeInto('', new Uint8Array(0));

  assertEquals(result.read, 0);
  assertEquals(result.written, 0);
});

test('encodeInto() should encode undefined as a string', () => {
  const result = encodeInto(undefined, new Uint8Array(10));

  assertEquals(result.read, 9);
  assertEquals(result.written, 9);
});

test('encodeInto() should tolerate a empty buffer', () => {
  const encoder = new TextEncoder();
  const result = encodeInto('hello', new Uint8Array(0));

  assertEquals(result.read, 0);
  assertEquals(result.written, 0);
});

test('encodeInto() should tolerate a buffer with a length less than the string', () => {
  const buffer = new Uint8Array(3);
  const result = encodeInto('hello', buffer);

  assertEquals(result.read, 3);
  assertEquals(result.written, 3);
  assertEquals(buffer[0], 0x68);
  assertEquals(buffer[1], 0x65);
  assertEquals(buffer[2], 0x6c);
});

test('encodeInto() should throw an error for unimplemented ArrayBuffer', () => {
  assertThrows(TypeError, () => encodeInto('hello', new ArrayBuffer(0)));
});

test('encodeInto() should throw an error for unimplemented DataView', () => {
  assertThrows(TypeError, () => encodeInto('hello', new DataView(new ArrayBuffer(0))));
});

test('encodeInto() should throw an error for invalid buffer', () => {
  const invalids = [9, {}, [], NaN, Infinity, null, true, false, 'hello', Symbol(1)];

  for (const invalid of invalids) {
    assertThrows(TypeError, () => encodeInto('hello', invalid));
  }
});

test('encodeInto() should throw an error when Symbol is passed', () => {
  assertThrows(TypeError, () => encodeInto(Symbol('hello'), new Uint8Array(10)));
});

function encode(...args) {
  return new TextEncoder().encode(...args);
}

function encodeInto(...args) {
  return new TextEncoder().encodeInto(...args);
}
