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

const { assertThrows } = require('../lib/assert.js');
const { test, runAllTests } = require('../lib/test.cjs');

const fromJSON = $jjs.jjs_pmap_from_json;
const resolve = $jjs.jjs_pmap_resolve;

test('should resolve "pkg" with package main shorthand', () => {
  pmap({
    packages: {
      'pkg': './pkg.cjs'
    }
  });

  expectEndsWith(resolve('pkg', 'none'), 'pkg.cjs');
  expectEndsWith(resolve('pkg', 'commonjs'), 'pkg.cjs');
  expectEndsWith(resolve('pkg', 'module'), 'pkg.cjs');
});

test('should resolve "pkg" with package main', () => {
  pmap({
    packages: {
      pkg: {
        main: './pkg.cjs'
      }
    }
  });

  expectEndsWith(resolve('pkg', 'none'), 'pkg.cjs');
  expectEndsWith(resolve('pkg', 'commonjs'), 'pkg.cjs');
  expectEndsWith(resolve('pkg', 'module'), 'pkg.cjs');
});

test('should resolve "pkg" with package main and module type', () => {
  pmap({
    packages: {
      pkg: {
        commonjs: {
          main: './pkg.cjs'
        },
        module: {
          main: './pkg.mjs'
        }
      }
    }
  });

  assertThrows(TypeError, () => resolve('pkg', 'none'));
  expectEndsWith(resolve('pkg', 'commonjs'), 'pkg.cjs');
  expectEndsWith(resolve('pkg', 'module'), 'pkg.mjs');
});

test('should resolve "pkg" with package main shorthand and module type', () => {
  pmap({
    packages: {
      pkg: {
        commonjs: './pkg.cjs',
        module: './pkg.mjs',
      }
    }
  });

  assertThrows(TypeError, () => resolve('pkg', 'none'));
  expectEndsWith(resolve('pkg', 'commonjs'), 'pkg.cjs');
  expectEndsWith(resolve('pkg', 'module'), 'pkg.mjs');
});

test('should resolve "@test/pkg1" with package path', () => {
  pmap({
    packages: {
      '@test/pkg1': {
        path: './@test/pkg1'
      }
    }
  });

  expectEndsWith(resolve('@test/pkg1/b.cjs', 'none'), 'b.cjs');
  expectEndsWith(resolve('@test/pkg1/b.cjs', 'commonjs'), 'b.cjs');
  expectEndsWith(resolve('@test/pkg1/b.cjs', 'module'), 'b.cjs');
});

test('should resolve "@test/pkg2" with module system specific package path', () => {
  pmap({
    packages: {
      '@test/pkg2': {
        commonjs: {
          path: './@test/pkg2/commonjs'
        },
        module: {
          path: './@test/pkg2/module'
        }
      }
    }
  });

  assertThrows(TypeError, () => resolve('@test/pkg2/a.js', 'none'));
  expectEndsWithRegEx(resolve('@test/pkg2/a.js', 'commonjs'), /commonjs[/\\]a\.js$/);
  expectEndsWithRegEx(resolve('@test/pkg2/a.js', 'module'), /module[/\\]a\.js$/);
});

function pmap(obj) {
  try {
    fromJSON(JSON.stringify(obj), './fixtures/pmap');
  } catch (e) {
    print(e);
    throw e;
  }
}

function expectEndsWith(actual, expected)  {
  assert(typeof actual === 'string', `expected typeof 'string', actual '${typeof actual}' for '${actual}'`);
  assert(actual.endsWith(expected), `expected '${actual}' to end with '${expected}'`);
}

function expectEndsWithRegEx(actual, expectedRegEx)  {
  assert(typeof actual === 'string', `expected typeof 'string', actual '${typeof actual}' for '${actual}'`);
  assert(expectedRegEx.test(actual), `expected '${actual}' to end with '${expectedRegEx}'`);
}

runAllTests();
