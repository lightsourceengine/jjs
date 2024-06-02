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

const { arrayEquals } = require('../lib/assert.js')

// TODO: use tests from wpt?

test('check global', () => {
  assert(typeof globalThis.URL === 'function');
  assert(typeof globalThis.URLSearchParams === 'function');
});

test('check require("jjs:url") exports', () => {
  arrayEquals(Object.keys(require('jjs:url')).toSorted(), cjsExports);
});

test('check import("jjs:url") exports', async () => {
  arrayEquals(Object.keys(await import('jjs:url')).toSorted(), esmExports);
});

test('URL()', () => {
  assert(new URL('file:///usr').href === 'file:///usr');
});

test('URL() with base', () => {
  assert(new URL('file', 'file:///usr').href === 'file:///file');
});

test('URL() with base and relative', () => {
  assert(new URL('file', 'file:///usr/').href === 'file:///usr/file');
});

const cjsExports = [
  'URL',
  'URLSearchParams',
  'fileURLToPath',
  'pathToFileURL',
].toSorted();

const esmExports = [ ...cjsExports, 'default'].toSorted();
