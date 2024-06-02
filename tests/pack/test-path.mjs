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

// check that path is importable
import {
  resolve,
  normalize,
  isAbsolute,
  join,
  relative,
  toNamespacedPath,
  dirname,
  basename,
  extname,
  format,
  parse,
  sep,
  delimiter,
  win32,
  posix,
} from 'jjs:path';
import { test } from 'jjs:test';

const { arrayEquals } = require('../lib/assert.js');

// TODO: can we take path tests from node?

test('check require("jjs:path") exports', () => {
  arrayEquals(Object.keys(require('jjs:path')).toSorted(), cjsExports);
});

test('check import("jjs:path") exports', async () => {
  arrayEquals(Object.keys(await import('jjs:path')).toSorted(), esmExports);
});

test('path.basename', () => {
  assert(basename('usr/bin') === 'bin');
});

const cjsExports = [
  'resolve', 'normalize', 'isAbsolute', 'join', 'relative', 'toNamespacedPath', 'dirname', 'basename', 'extname', 'format', 'parse', 'sep', 'delimiter', 'win32', 'posix',
].toSorted();

const esmExports = [...cjsExports, 'default'].toSorted();
