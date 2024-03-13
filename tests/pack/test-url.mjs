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

// TODO: use tests from wpt?

test('URL()', () => {
  assert(new URL('file:///usr').href === 'file:///usr');
});

test('URL() with base', () => {
  assert(new URL('file', 'file:///usr').href === 'file:///file');
});

test('URL() with base and relative', () => {
  assert(new URL('file', 'file:///usr/').href === 'file:///usr/file');
});

test ('require jjs:url should export url api', () => {
  checkPackageExports(require('jjs:url'));
});

test ('import jjs:url should export url api', async () => {
  const ns = await import('jjs:url')

  // TODO: export symbols! checkPackageExports(ns);
  checkPackageExports(ns.default);
});

function checkPackageExports(exports) {
  assert(Object.keys(exports).length === 4);
  assert(typeof exports.fileURLToPath === 'function');
  assert(typeof exports.pathToFileURL === 'function');
  assert(typeof exports.URL === 'function');
  assert(typeof exports.URLSearchParams === 'function');
}

runAllTests();
