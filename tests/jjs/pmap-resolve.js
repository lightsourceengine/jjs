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

const { cwd, realpath, pmap } = jjs;

test ('pmap.resolve() should resolve "pkg" path with module', () => {
  assertEquals(pmap.resolve('pkg', 'module'), resolvePmapPath('./pkg.mjs'));
});

test ('pmap.resolve() should resolve "pkg" path with commonjs', () => {
  assertEquals(pmap.resolve('pkg', 'commonjs'), resolvePmapPath('./pkg.cjs'));
});

test ('pmap.resolve() should throw Error for "pkg" path with none', () => {
  assertThrows(Error, () => pmap.resolve('pkg', 'none'));
});

test ('pmap.resolve() should throw Error for "pkg" path with undefined', () => {
  assertThrows(Error, () => pmap.resolve('pkg'));
});

test ('pmap.resolve() should resolve "@test/pkg1" path with module', () => {
  assertEquals(pmap.resolve('@test/pkg1', 'module'), resolvePmapPath('./@test/pkg1/b.cjs'));
});

test ('pmap.resolve() should resolve "@test/pkg1" path with commonjs', () => {
  assertEquals(pmap.resolve('@test/pkg1', 'commonjs'), resolvePmapPath('./@test/pkg1/b.cjs'));
});

test ('pmap.resolve() should resolve "@test/pkg1" path with none', () => {
  assertEquals(pmap.resolve('@test/pkg1', 'none'), resolvePmapPath('./@test/pkg1/b.cjs'));
});

test ('pmap.resolve() should resolve "@test/pkg1" path with undefined', () => {
  assertEquals(pmap.resolve('@test/pkg1'), resolvePmapPath('./@test/pkg1/b.cjs'));
});

test ('pmap.resolve() should resolve "@test/pkg1/b.cjs" path with module', () => {
  assertEquals(pmap.resolve('@test/pkg1/b.cjs', 'module'), resolvePmapPath('./@test/pkg1/b.cjs'));
});

test ('pmap.resolve() should resolve "@test/pkg1/b.cjs" path with commonjs', () => {
  assertEquals(pmap.resolve('@test/pkg1/b.cjs', 'commonjs'), resolvePmapPath('./@test/pkg1/b.cjs'));
});

test ('pmap.resolve() should resolve "@test/pkg1/b.cjs" path with none', () => {
  assertEquals(pmap.resolve('@test/pkg1/b.cjs', 'none'), resolvePmapPath('./@test/pkg1/b.cjs'));
});

test ('pmap.resolve() should resolve "@test/pkg1/b.cjs" path with undefined', () => {
  assertEquals(pmap.resolve('@test/pkg1/b.cjs'), resolvePmapPath('./@test/pkg1/b.cjs'));
});

test ('pmap.resolve() should resolve "@test/pkg2" path with module', () => {
  assertEquals(pmap.resolve('@test/pkg2', 'module'), resolvePmapPath('./@test/pkg2/module/a.js'));
});

test ('pmap.resolve() should resolve "@test/pkg2" path with commonjs', () => {
  assertEquals(pmap.resolve('@test/pkg2', 'commonjs'), resolvePmapPath('./@test/pkg2/commonjs/a.js'));
});

test ('pmap.resolve() should throw Error for "@test/pkg2" path with none', () => {
  assertThrows(Error, () => pmap.resolve('@test/pkg2', 'none'));
});

test ('pmap.resolve() should throw Error for "@test/pkg2" path with undefined', () => {
  assertThrows(Error, () => pmap.resolve('@test/pkg2'));
});

function resolvePmapPath (path) {
  return realpath(cwd() + "/fixtures/pmap/" + path);
}

runAllTests();
