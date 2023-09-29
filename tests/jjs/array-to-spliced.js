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
let dest;

// test: toSpliced() should return a copy of the array when no arguments are passed
source = [1, 2, 3];
dest = source.toSpliced();
assert(source !== dest);
arrayEquals(dest, [1, 2, 3]);

// test: toSpliced() should return a copy of the array when start and skipCount are 0
source = [1, 2, 3];
dest = source.toSpliced(0, 0);
assert(source !== dest);
arrayEquals(dest, [1, 2, 3]);

// test: toSpliced() should return an empty array when start is undefined and skipCount is not present
source = [1, 2, 3];
dest = source.toSpliced(undefined);
assert(source !== dest);
arrayEquals(dest, []);

// test: toSpliced() should splice at the beginning of the array
source = [3, 4, 5];
dest = source.toSpliced(0, 0, 1, 2);
assert(source !== dest);
arrayEquals(dest, [1, 2, 3, 4, 5]);

// test: toSpliced() should splice in the middle of the array
source = [3, 'x', 5];
dest = source.toSpliced(1, 1, 4);
assert(source !== dest);
arrayEquals(dest, [3, 4, 5]);

// test: toSpliced() should copy an empty array
source = [];
dest = source.toSpliced();
assert(source !== dest);
arrayEquals(dest, []);

// test: toSpliced() should copy a sparse array
source = [];
source[5] = 1;
dest = source.toSpliced();
assert(source !== dest);
assert(dest instanceof Array);
arrayEquals(dest, [undefined, undefined, undefined, undefined, undefined, 1]);

// test: toSpliced() should copy an array-like object
source = { 0: 1, 1: 2, 2: 3, length: 3 };
dest = Array.prototype.toSpliced.call(source);
assert(source !== dest);
assert(dest instanceof Array);
arrayEquals(dest, [1, 2, 3]);

// test: toSpliced() should throw RangeError when array length is too big
assertThrows(RangeError, () => { Array.prototype.toSpliced.call({length: Number.MAX_SAFE_INTEGER}); });

// test: toSpliced() should throw TypeError when start is not a number
assertThrows(TypeError, () => { [1, 2, 3].toSpliced(1n, 0); });
assertThrows(TypeError, () => { [1, 2, 3].toSpliced(Symbol(), 0); });
assertThrows(TypeError, () => { [1, 2, 3].toSpliced(true, 0); });
assertThrows(TypeError, () => { [1, 2, 3].toSpliced(false, 0); });

// test: toSpliced() should throw TypeError when skipCount is not a number
assertThrows(TypeError, () => { [1, 2, 3].toSpliced(0, 1n); });
assertThrows(TypeError, () => { [1, 2, 3].toSpliced(0, Symbol()); });
assertThrows(TypeError, () => { [1, 2, 3].toSpliced(0, true); });
assertThrows(TypeError, () => { [1, 2, 3].toSpliced(0, false); });
