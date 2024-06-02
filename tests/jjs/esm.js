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

const { test } = require ('jjs:test');

test('jjs_esm_import() should import module from relative specifier', async () => {
  const { one } = await import('./fixtures/one-export.mjs');

  assert(one === 'test', `${one} !== test`);
});

test('jjs_esm_import() should import module from relative specifier', async () => {
  const { x } = await import('pkg');

  assert(x === 5, `${x} !== 5`);
});

test('jjs_esm_import() should import from module system with path', async () => {
  const ns = await import('@test/pkg2/a.js');

  assert(ns.default === "module.a", `${ns.default} !== module.a`);
})

test('jjs_esm_import() should throw Error when package not found', async () => {
  const specifiers = ['unknown', './unknown', '@unknown/pkg', '/unknown', ''];

  for (const specifier of specifiers) {
    try {
      await import(specifier);
    } catch (e) {
      continue;
    }
    assert(false, `expected import('${specifier}') to throw`);
  }
});

test('jjs_esm_import() should throw Error on non-string specifier', async () => {
  const inputs = [null, undefined, 0, 1, true, false, {}, [], () => {}];

  for (const specifier of inputs) {
    try {
      await import(specifier);
    } catch (e) {
      continue;
    }
    assert(false, `expected import('${specifier}') to throw`);
  }
});
