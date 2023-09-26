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
  Float64Array
];


typedArrayTypes.forEach (function (TypedArray) {

  let array1 = new TypedArray ([5, 12, 0, 8, 120, 44]);

  function equal_to_8 (element) {
    return element === 8;
  }

  assert (array1.findLast (equal_to_8) === 8);

  function less_than_0 (element) {
    if (element === 0) {
      throw new Error ("zero");
    }
    return element < 0;
  }

  try {
    array1.findLast (less_than_0);
    assert (false);
  } catch (e) {
    assert (e instanceof Error);
    assert (e.message === "zero");
  }

  assert (eval ("array1.findLast (e => e > 100)") === 120);

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

  assert (src_array.findLast (isPrime) === undefined);

  src_array = new TypedArray ([4, 5, 8, 12]);
  array_index = src_array.length - 1;
  assert (src_array.findLast (isPrime) === 5);

  // Checking behavior when the given object is not %TypedArray%
  try {
    TypedArray.prototype.findLast.call (5);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  // Checking behavior when the first argument is not a callback
  let array = new TypedArray ([1, 2, 3]);

  try {
    array.findLast (5);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  // Checking behavior when the first argument is not exists
  try {
    array.findLast ();
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  // Checking behavior when the there are more than 2 arguments
  assert (array.findLast (function (e) { return e < 2 }, {}, 8, 4, 5, 6, 6) === 1);
})
