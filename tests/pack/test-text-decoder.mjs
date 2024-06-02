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

const encodings = ['utf-8', 'utf8', 'unicode-1-1-utf-8'];

test('check global', () => {
  assert(typeof globalThis.TextDecoder === 'function');
});

test('constructor should accept no argument', () => {
  const decoder = new TextDecoder();
  assert(decoder.encoding === 'utf-8');
  assert(decoder.fatal === false);
  assert(decoder.ignoreBOM === false);
});

test('constructor should accept utf8 encoding strings', () => {
  for (const encoding of encodings) {
    const decoder = new TextDecoder(encoding);
    assert(decoder.encoding === 'utf-8');
    assert(decoder.fatal === false);
    assert(decoder.ignoreBOM === false);
  }
});

test('constructor should accept uppercase utf8 encoding strings', () => {
  for (const encoding of encodings) {
    const decoder = new TextDecoder(encoding.toUpperCase());
    assert(decoder.encoding === 'utf-8');
    assert(decoder.fatal === false);
    assert(decoder.ignoreBOM === false);
  }
});

test('constructor should set ignoreBOM', () => {
  const decoder = new TextDecoder('utf-8', { ignoreBOM: true });
  assert(decoder.encoding === 'utf-8');
  assert(decoder.fatal === false);
  assert(decoder.ignoreBOM === true);
});

test('constructor should set fatal', () => {
  const decoder = new TextDecoder('utf-8', { ignoreBOM: true, fatal: true });
  assert(decoder.encoding === 'utf-8');
  assert(decoder.fatal === true);
  assert(decoder.ignoreBOM === true);
});

test('constructor should throw an error for invalid encoding strings', () => {
  assertThrows(TypeError, () => new TextDecoder('invalid'));
});

test('decode() should decode a byte array to string', () => {
  const result = decode(new Uint8Array([0x68, 0x65, 0x6c, 0x6c, 0x6f]));

  assert(typeof result === 'string');
  assert(result === 'hello');
});

test('decode() should decode a byte array with a BOM', () => {
  const decoder = new TextDecoder('utf8', { ignoreBOM: true });
  const result = decoder.decode(new Uint8Array([0xef, 0xbb, 0xbf, 0x68, 0x65, 0x6c, 0x6c, 0x6f]));

  assert(typeof result === 'string');
  assert(result === 'hello');
});

test('decode() should decode empty byte array as empty string', () => {
  const result = decode(new Uint8Array(0));

  assert(typeof result === 'string');
  assert(result === '');
});

test('decode() should decode invalid UTF-8 bytes with replacement code point', () => {
  const result = decode(Uint8Array.of(0xff));

  assert(typeof result === 'string');
  assertEquals(result, '\uFFFD');
});

test('decode() should decode invalid UTF-8 bytes with replacement code point', () => {
  const result = decode(Uint8Array.of(0xff, 'a'.charCodeAt(0), 0xff, 'b'.charCodeAt(0), 0xff));

  assert(typeof result === 'string');
  assertEquals(result, '\uFFFDa\uFFFDb\uFFFD');
});

test('decode() should decode throw TypeError for invalid characters when fatal flag is set', () => {
  const decoder = new TextDecoder('utf8', { fatal: true });

  assertThrows(TypeError, () => decoder.decode(Uint8Array.of(0xff, 'a'.charCodeAt(0), 0xff, 'b'.charCodeAt(0), 0xff)));
});

test('decode() should throw unimplemented error for stream option', () => {
  assertThrows(TypeError, () => decode(new Uint8Array(0), { stream: true }));
});

test('decode() should handle zero filled buffer', () => {
  assertEquals(decode(new Uint8Array(3)), '\x00\x00\x00');
});

test('decode() should accept ArrayBuffer', () => {
  assertEquals(decode(new ArrayBuffer(0)), '');
});

test('decode() should accept DataView', () => {
  assertEquals(decode(new DataView(new ArrayBuffer(0))), '');
});

test('decode() should throw an error for unimplemented SharedArrayBuffer', () => {
  assertThrows(TypeError, () => decode(new SharedArrayBuffer(0)));
});

test('decode() should throw an error for invalid buffers', () => {
  const invalids = [9, {}, [], NaN, Infinity, null, true, false, 'hello', Symbol(1)];
  for (const invalid of invalids) {
    assertThrows(TypeError, () => decode(invalid));
  }
});

function decode(...args) {
  return new TextDecoder().decode(...args);
}
