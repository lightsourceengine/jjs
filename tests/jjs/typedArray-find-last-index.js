// Copyright Light Source Software, LLC and other contributors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

let typedArrayTypes = [
  Int8Array,
  Uint8Array,
  Uint8ClampedArray,
  Int16Array,
  Uint16Array,
  Int32Array,
  Uint32Array,
  Float32Array,
  Float64Array,
];


typedArrayTypes.forEach (function (TypedArray) {

  let array1 = new TypedArray ([5, 12, 0, 8, 120, 44]);

  function equal_to_8 (element) {
    return element === 8;
  }

  assert (array1.findLastIndex (equal_to_8) === 3);

  function less_than_0 (element) {
    if (element === 0) {
      throw new Error ("zero");
    }
    return element < 0;
  }

  try {
    array1.findLastIndex (less_than_0);
    assert (false);
  } catch (e) {
    assert (e instanceof Error);
    assert (e.message === "zero");
  }

  assert (eval ("array1.findLastIndex (e => e > 100)") === 4);

  /* Test the callback function arguments */
  let src_array = new TypedArray ([4, 6, 8, 12]);
  let array_index = src_array.length - 1;

  function isPrime (element, index, array) {
    assert (array_index-- === index);
    assert (array === src_array)
    assert (element === array[index]);

    let start = 2;
    while (start <= Math.sqrt (element)) {
      if (element % start++ < 1) {
        return false;
      }
    }
    return element > 1;
  }

  assert (src_array.findLastIndex (isPrime) === -1);

  src_array = new TypedArray ([4, 5, 8, 12]);
  array_index = src_array.length - 1;
  assert (src_array.findLastIndex (isPrime) === 1);

  // Checking behavior when the given object is not %TypedArray%
  try {
    TypedArray.prototype.findLastIndex.call (5);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  // Checking behavior when the first argument is not a callback
  let array = new TypedArray ([1, 2, 3]);

  try {
    array.findLastIndex (5);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  // Checking behavior when the first argument does not exist
  try {
    array.findLastIndex ();
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  // Checking behavior when there are more than 2 arguments
  assert (array.findLastIndex (function (e) { return e < 2 }, {}, 8, 4, 5, 6, 6) === 0);
})

function isNegative(element, index, array) {
  return element < 0;
}

function isBigger(element, index, array) {
  return element > 40n;
}

let bigint_array = new BigInt64Array([10n, -20n, 30n, -40n, 50n]);
let biguint_array = new BigUint64Array([10n, 20n, 30n, 40n, 50n]);

assert(bigint_array.findLastIndex(isNegative) === 3);
assert(biguint_array.findLastIndex(isBigger) === 4);
