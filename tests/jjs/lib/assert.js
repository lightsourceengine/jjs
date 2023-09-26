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

function arrayEquals(actual, expected) {
  assert(actual.length === expected.length, `expected length: ${expected.length}, actual length: ${actual.length}`);
  for (let i = 0; i < actual.length; i++) {
    assert(actual[i] === expected[i], `expected ${expected[i]} at index ${i}, actual ${actual[i]}`);
  }
}

function assertThrows(type, fn) {
  try {
    fn();
    assert(false, `expected ${type} to be thrown`);
  } catch (e) {
    assert(e instanceof type, `expected ${type} to be thrown, got ${e}`);
  }
}

module.exports = {
  arrayEquals,
  assertThrows,
}
