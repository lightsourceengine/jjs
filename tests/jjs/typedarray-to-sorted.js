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

const { arrayEquals, assertThrows } = require('./lib/assert.js');

const typedArrayTypes = [
  Int8Array,
  Uint8Array,
  Uint8ClampedArray,
  Int16Array,
  Uint16Array,
  Int32Array,
  Uint32Array,
  Float32Array,
  Float64Array,
  BigInt64Array,
  BigUint64Array,
];

function isBigIntArray(TypedArray) {
  return TypedArray === BigInt64Array || TypedArray === BigUint64Array;
}

function descending(a, b) {
  if (a < b) return 1;
  if (a > b) return -1;
  return 0;
}

for (const TypedArray of typedArrayTypes) {
  let sorted;
  let source;
  let expected;

  print(TypedArray.name)

  // test: toReversed() should return a sorted array with elements in ascending order
  if (isBigIntArray(TypedArray)) {
    source = TypedArray.of(4n, 8n, 6n, 7n, 1n);
    expected = [1n, 4n, 6n, 7n, 8n];
  } else {
    source = TypedArray.of(4, 8, 6, 7, 1);
    expected = [1, 4, 6, 7, 8];
  }

  sorted = source.toSorted();
  assert(source !== sorted);
  arrayEquals(sorted, expected);

  // test: toReversed() should return a sorted array with elements in descending order
  sorted = source.toSorted(descending);
  assert(source !== sorted);
  arrayEquals(sorted, expected.toReversed());

  // test: toSorted() should return an empty array if the source array is empty
  source = TypedArray.of();
  sorted = source.toSorted();
  assert(source !== sorted);
  assert(sorted instanceof TypedArray);
  assert(sorted.length === 0);
}
