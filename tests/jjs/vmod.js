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

let called = false;

$jjs.jjs_vmod ('test-vmod', (name) => {
  assert(name === 'test-vmod');
  assert(!called);
  called = true;
  return {
    exports: {
      x: 5,
    }
  }
});

$jjs.jjs_vmod ('test-vmod-default', (name) => {
  assert(name === 'test-vmod-default');
  return {
    exports: {
      default: 'test',
    }
  }
});

test('require() should return vmod exports', () => {
  const ns = require('test-vmod');

  assert(typeof ns === 'object', `expected object, got ${typeof ns}`);
  assert(ns.x === 5, `expected 5, got ${ns.x}`);
});

test('require() should return cached vmod exports', () => {
  require('test-vmod');
  require('test-vmod');
  assert(called);
});

test('resolve() should return vmod name', () => {
  const resolved = require.resolve('test-vmod');
  assert(resolved === 'test-vmod', `expected 'test-vmod', got ${resolved}`);
});

test('import should return vmod exports', () => {
  const { default:ns } = $jjs.jjs_esm_import('test-vmod');

  assert(typeof ns === 'object', `expected object, got ${typeof ns}`);
  assert(ns.x === 5, `expected 5, got ${ns.x}`);
});

test('import should return vmod exports', () => {
  const { default:ns } = $jjs.jjs_esm_import('test-vmod-default');

  assert(ns === 'test', `expected 'test', got ${ns}`);
});

runAllTests();
