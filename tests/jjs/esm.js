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

const { test, runAllTests } = require('./lib/test.cjs');
const { assertThrows } = require('./lib/assert.js');

$jjs.jjs_pmap_from_file('./fixtures/pmap/pmap.json');

test('jjs_esm_import() should import module from relative specifier', () => {
  const { one } = $jjs.jjs_esm_import('./fixtures/one-export.mjs');

  assert(one === 'test', `${one} !== test`);
});

test('jjs_esm_import() should import module from relative specifier', () => {
  const { x } = $jjs.jjs_esm_import('pkg');

  assert(x === 5, `${x} !== 5`);
});

test('jjs_esm_import() should import from module system with path', () => {
  const ns = $jjs.jjs_esm_import('@test/pkg2/a.js');

  assert(ns.default === "module.a", `${ns.default} !== module.a`);
})

test('jjs_esm_import() should throw Error when package not found', () => {
  const specifiers = ['unknown', './unknown', '@unknown/pkg', '/unknown', ''];

  specifiers.forEach((specifier) => assertThrows(Error, () => { $jjs.jjs_esm_import(specifier); }));
});

test('jjs_esm_import() should throw Error on non-string specifier', () => {
  const inputs = [null, undefined, 0, 1, true, false, {}, [], () => {}];

  inputs.forEach((input) => assertThrows(Error, () => { $jjs.jjs_esm_import(input); }));
});

runAllTests();
