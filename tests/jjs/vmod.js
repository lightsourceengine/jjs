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
const { assertEquals, assertThrows } = require('../lib/assert.js');

test('vmod is in the global namespace', () => {
  assert(typeof jjs.vmod === 'function', 'expected vmod in the global namespace');
});

test('vmod.exists() should return false for invalid package names', () => {
  for (const name of invalidLookupPackageNames) {
    assert(!jjs.vmod.exists(name));
  }
});

test('vmod.remove() should remove a registered package', () => {
  const name = genPackageName();

  jjs.vmod(name, { exports: {} });
  assert(jjs.vmod.exists(name));
  jjs.vmod.remove(name);
  assert(!jjs.vmod.exists(name));
});

test('vmod.remove() should do nothing for invalid package names', () => {
  for (const name of invalidLookupPackageNames) {
    jjs.vmod.remove(name);
  }
});

test('vmod.resolve() should return the same vmod instance exports', () => {
  const name = genPackageName();

  jjs.vmod(name, { exports: 1 });

  assert(jjs.vmod.resolve(name) === jjs.vmod.resolve(name), 'vmod.resolve() expected to return the same object');
});

test('vmod.resolve() should throw Error when callback throws Error', () => {
  const name = genPackageName();

  jjs.vmod(name, () => { throw Error() });
  assertThrows(Error, () => jjs.vmod.resolve(name));
});

test('vmod.resolve() should throw Error for invalid vmod config', () => {
  const name = genPackageName();

  for (const config of invalidConfigs) {
    assertThrows(Error, () => jjs.vmod(name, config));
  }
});

test('vmod.resolve() should throw Error for invalid vmod config from callback', () => {
  for (const config of invalidConfigs) {
    const name = genPackageName();

    jjs.vmod(name, () => config);
    assertThrows(Error, () => jjs.vmod.resolve(name));
  }
});

test('vmod() should register a new package from exports and format = object', () => {
  const pkg = genPackageName();
  let called = false;

  jjs.vmod(pkg, () => {
    called = true;
    return {
      exports: {
        x: 5,
      },
      format: 'object',
    };
  });

  assert(!called);
  assert(jjs.vmod.exists(pkg));
  assert(!called);
  assertEquals(jjs.vmod.resolve(pkg).x, 5);
  assert(called);
});

test('vmod() should create a new package from exports and format = object', () => {
  const pkg = genPackageName();

  jjs.vmod(pkg, {
    exports: {
      x: 5,
    },
    format: 'object',
  });

  assert(jjs.vmod.exists(pkg));
  assertEquals(jjs.vmod.resolve(pkg).x, 5);
});

test('vmod() should register a new package from exports and format = not specified', () => {
  const pkg = genPackageName();
  let called = false;

  jjs.vmod(pkg, () => {
    called = true;
    return {
      exports: {
        x: 5,
      },
    };
  });

  assert(!called);
  assert(jjs.vmod.exists(pkg));
  assert(!called);
  assertEquals(jjs.vmod.resolve(pkg).x, 5);
  assert(called);
});

test('vmod() should create a new package from exports and format = not specified', () => {
  const pkg = genPackageName();

  jjs.vmod(pkg, {
    exports: {
      x: 5,
    },
  });

  assert(jjs.vmod.exists(pkg));
  assertEquals(jjs.vmod.resolve(pkg).x, 5);
});

test('vmod() should register supported package names', () => {
  const names = [
    'package',
    '@test/package',
    'pack-age',
    'pack_age',
    'pack.age',
    '123package',
  ];

  for (const name of names) {
    jjs.vmod(name, () => {});
  }
});

test('vmod() should throw TypeError for an invalid package name', () => {
  const names = [
    '',
    ''.padEnd(215, 'x'),
    '.test',
    '_test',
    't est',
    [],
    {},
    2,
    true,
    Symbol(),
    null,
    undefined
  ];

  for (const name of names) {
    assertThrows(TypeError, () => jjs.vmod(name, () => {}));
  }
});

const invalidConfigs = [
  null,
  undefined,
  true,
  Symbol(),
  '',
  123,
  NaN,
  4n,
  [],
  {},
  { format: 'dasdasd' },
  { format: 'object' },
  { format: 'module' },
  { format: 'commonjs' },
];

const invalidLookupPackageNames = [
  1,
  4n,
  true,
  [],
  {},
  Symbol(),
  '',
  'yyy123',
];

let nextPackageId = 1;

function genPackageName() {
  return `test:pkg${nextPackageId++}`;
}

runAllTests();
