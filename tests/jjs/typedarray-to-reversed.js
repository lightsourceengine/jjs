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

for (const TypedArray of typedArrayTypes) {
  let rev;
  let source;
  let expected;

  // test: toReversed() should return a new array with elements in reversed order
  if (isBigIntArray(TypedArray)) {
    source = TypedArray.of(1n, 2n, 3n);
    expected = [3n, 2n, 1n];
  } else {
    source = TypedArray.of(1, 2, 3);
    expected = [3, 2, 1];
  }

  rev = source.toReversed();
  assert(source !== rev);
  arrayEquals(rev, expected);

  // test: toReversed() should return an empty array if the source array is empty
  source = TypedArray.of();
  rev = source.toReversed();
  assert(source !== rev);
  assert(rev instanceof TypedArray);
  assert(rev.length === 0);
}
