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

const { arrayEquals, assertThrows } = include('./lib/assert.js');

let source;
let expected;

// test: toSorted() should return a sorted copy of a string array when no arguments are passed
source = ["Peach", "Apple", "Orange", "Grape", "Cherry", "Apricot", "Grapefruit"];
expected = ["Apple", "Apricot", "Cherry", "Grape", "Grapefruit", "Orange", "Peach"];
actual = source.toSorted();

assert(actual !== source);
arrayEquals(actual, expected);
actual = source.toSorted(undefined);
assert(actual !== source);
arrayEquals(actual, expected);

// test: toSorted() should return a sorted copy of a number array when a comparison function is passed
source = [6, 4, 5, 1, 2, 9, 7, 3, 0, 8];
expected = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
actual = source.toSorted();

assert(actual !== source);
arrayEquals(actual, expected);
actual = source.toSorted(undefined);
assert(actual !== source);
arrayEquals(actual, expected);

// test: toSorted() should return a sorted copy of a string array when a comparison function is passed
function descending(a, b) {
  if (a < b) return 1;
  if (a > b) return -1;
  return 0;
}

source = [6, 4, 5, 1, 2, 9, 7, 3, 0, 8];
expected = [9, 8, 7, 6, 5, 4, 3, 2, 1, 0];
actual = source.toSorted(descending);
assert(actual !== source);
arrayEquals(actual, expected);

// test: toSorted() should return a sorted copy of a sparse array when no arguments are passed
source = [1, , 2, , 3, , 4, undefined, 5];
expected = [1, 2, 3, 4, 5, undefined, undefined, undefined, undefined];
actual = source.toSorted();
assert(actual !== source);
arrayEquals(actual, expected);

// test: toSorted() should throw if argument is not callable
assertThrows(TypeError, () => [1].toSorted({}));

// test: toSorted() should throw if source length throws an error
let obj = {};
Object.defineProperty(obj, 'length', { 'get' : function () { throw new ReferenceError ("foo"); } });
assertThrows(ReferenceError, () => Array.prototype.toSorted.call(obj));

// test: toSorted() should throw if accessing property at index throws an error
obj = { sort : Array.prototype.sort, length : 1};
Object.defineProperty(obj, '0', { 'get' : function () { throw new ReferenceError ("foo"); } });
assertThrows(ReferenceError, () => Array.prototype.toSorted.call(obj));
