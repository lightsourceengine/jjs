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

let source;
let dest;

// test: with(index, value) should replace this[index] with value
source = [1, 2, 3];
dest = source.with(1, 4);

assert(source !== dest);
arrayEquals(dest, [1, 4, 3]);

// test: with(-index, value) should replace this[this.length - 1] with value
source = [1, 2, 3];
dest = source.with(-1, 4);

assert(source !== dest);
arrayEquals(dest, [1, 2, 4]);

// test: with(index, value) should return undefined entries for sparse arrays
source = [,, 2];
dest = source.with(1, 4);

assert(source !== dest);
arrayEquals(dest, [undefined, 4, 2]);

// test: with(index, value) should cast index to integer
arrayEquals([1, 2, 3].with(0.4, 4), [4, 2, 3]);
arrayEquals([1, 2, 3].with(1.5, 4), [1, 4, 3]);
arrayEquals([1, 2, 3].with('1', 4), [1, 4, 3]);
arrayEquals([1, 2, 3].with('a', 4), [4, 2, 3]);
arrayEquals([1, 2, 3].with({}, 4), [4, 2, 3]);
arrayEquals([1, 2, 3].with([], 4), [4, 2, 3]);
arrayEquals([1, 2, 3].with(NaN, 4), [4, 2, 3]);

// test: with() should throw RangeError when index is out of range
assertThrows(RangeError, () => { [].with(); });
assertThrows(RangeError, () => { [1, 2, 3].with(10, 100); });
assertThrows(RangeError, () => { [1, 2, 3].with(-10, 100); });
assertThrows(RangeError, () => { [1, 2, 3].with(-Infinity, 100); });
assertThrows(RangeError, () => { [1, 2, 3].with(Infinity, 100); });

// test: with() should throw TypeError when index is a BigInt or Symbol
assertThrows(TypeError, () => { [1, 2, 3].with(1n, 100); });
assertThrows(TypeError, () => { [1, 2, 3].with(Symbol(), 100); });
assertThrows(TypeError, () => { [1, 2, 3].with(true, 100); });
assertThrows(TypeError, () => { [1, 2, 3].with(false, 100); });

// test: with() should be callable on non-array objects with length property
dest = Array.prototype.with.call({ length: 1, 0: 3 }, 0, 4);
arrayEquals([4], dest);
