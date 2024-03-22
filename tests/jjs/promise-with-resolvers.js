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

const { assertEquals } = require('../lib/assert.js');
const { test, runAllTests } = require('../lib/test.cjs');

test('Promise.withResolvers() should create a new promise', () => {
  const { promise, resolve, reject } = Promise.withResolvers();

  assertEquals(promise.constructor, Promise);
  assertEquals(typeof resolve, 'function');
  assertEquals(typeof reject, 'function');
});

test('Promise.withResolvers() should return an Object', () => {
  const obj = Promise.withResolvers();

  assertEquals(typeof obj, 'object');
  assertEquals(obj instanceof Object, true);
});

test('Promise.withResolvers() should create a working resolve function', async () => {
  const { promise, resolve } = Promise.withResolvers();

  resolve(1);
  assertEquals(await promise, 1);
});

test('Promise.withResolvers() should create a working reject function', async () => {
  const { promise, reject } = Promise.withResolvers();
  const err = new Error();

  reject(err);

  try {
    await promise;
  } catch (e) {
    assertEquals(e, err);
  }
});

test('Promise.withResolvers() should be called with a Promise subclass', () => {
  class PromiseSubclass extends Promise {};

  const instance = Promise.withResolvers.call(PromiseSubclass);

  assertEquals(instance.promise.constructor, PromiseSubclass);
  assertEquals(instance.promise instanceof PromiseSubclass, true);
});

runAllTests();
