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

function isFloatArray(TypedArray) {
  return TypedArray === Float32Array || TypedArray === Float64Array;
}

for (const TypedArray of typedArrayTypes) {
  let replace;
  let source;
  let expected;
  let copy;

  // test: with() should replace index 1 with 5
  if (isBigIntArray(TypedArray)) {
    source = TypedArray.of(1n, 2n, 3n);
    expected = [1n, 5n, 3n];
    replace = 5;
  } else {
    source = TypedArray.of(1, 2, 3);
    expected = [1, 5, 3];
    replace = 5;
  }

  copy = source.with(1, replace);
  assert(copy !== source);
  arrayEquals(copy, expected);

  // test: with() should replace index 0 with 0 when no arguments passed
  if (isBigIntArray(TypedArray)) {
    source = TypedArray.of(1n, 2n, 3n);
    // FIXME: snapshots appear to be converting 0n to 0, causing the arrayEquals to fail
    expected = [BigInt(0), 2n, 3n];
  } else {
    source = TypedArray.of(1, 2, 3);
    if (isFloatArray(TypedArray)) {
      expected = [NaN, 2, 3];
    } else {
      expected = [0, 2, 3];
    }
  }

  copy = source.with();
  assert(copy !== source);
  arrayEquals(copy, expected);

  // test: with() should throw RangeError when no arguments and the array is empty
  assertThrows(RangeError, () => TypedArray.of().with());
}
