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

// TODO: use tests from wpt?

test('check global', () => {
  assert(typeof globalThis.performance === 'object');
});

test('now() should return a value greater than zero', () => {
  assert(performance.now() > 0);
});

test('now() should return a value when destructured', () => {
  const { now } = performance;
  assert(now() > 0);
});

test('now() should should increase over consecutive calls', () => {
  const start = performance.now();

  // do some busy work
  for (let i = 0; i < 1000; i++) {
    new TypeError('test message'); // eslint-disable-line
  }

  assert(start < performance.now());
});

test('timeOrigin should be greater than zero', () => {
  assert(performance.timeOrigin > 0);
});

// TODO: this test intermittently fails on Windows
test('timeOrigin should be less than Date.now()', { skip: true },  () => {
  assert(performance.timeOrigin < Date.now());
});

// TODO: this test intermittently fails on Windows
test('timeOrigin + now() should equal Date.now()', { skip: true }, () => {
  const actual = new Date(performance.timeOrigin + performance.now());
  const expected = new Date();

  // allow for some wiggle room
  assert(
    expected - actual < 1000,
    `expected ${expected.getTime()} to be within 1000ms of ${actual.getTime()}`,
  );
});
