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

// test: toReversed() should return a new, reversed array
let source = [1, 2, 3];
let dest = source.toReversed();
assert(source !== dest);
arrayEquals(dest, [3, 2, 1]);

// test: toReversed() should reverse a sparse array
arrayEquals([, 2, , 4].toReversed(), [4, undefined, 2, undefined]);

// test: toReversed() should return a new empty array for an empty array
source = [];
dest = source.toReversed();
assert(source !== dest);
arrayEquals(dest, []);

// test: toReversed() should return a new array with one element for an array with one element
source = [1];
dest = source.toReversed();
assert(source !== dest);
arrayEquals(dest, [1]);

// test: toReversed() should run on an array-like object
source = { 0: 1, 1: 2, 2: 3, length: 3 };
dest = Array.prototype.toReversed.call(source);
assert(source !== dest);
assert(dest instanceof Array);
arrayEquals(dest, [3, 2, 1]);
