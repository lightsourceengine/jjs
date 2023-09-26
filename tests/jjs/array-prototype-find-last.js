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

const array1 = [5, 12, 0, 8, 130, 44];

function bigger_than_10 (element) {
  return element > 10;
}

assert (array1.findLast (bigger_than_10) === 44);

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

const inventory = [
    {name: 'apples', quantity: 2},
    {name: 'bananas', quantity: 0},
    {name: 'cherries', quantity: 5}
];

function isCherries (fruit) {
    return fruit.name === 'cherries';
}

assert (JSON.stringify (inventory.findLast (isCherries)) === '{"name":"cherries","quantity":5}');

assert (eval ("inventory.findLast (fruit => fruit.name === 'bananas')") === inventory[1]);

/* Test the callback function arguments */
let src_array = [4, 6, 8, 12];
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

src_array = [4, 5, 8, 12];
array_index = src_array.length - 1;
assert (src_array.findLast (isPrime) === 5);


// Checking behavior when unable to get length
let obj = {};
Object.defineProperty (obj, 'length', { 'get' : function () { throw new ReferenceError ("foo"); } });
obj.findLast = Array.prototype.findLast;

try {
  obj.findLast ();
  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}

const data = { 0: 1, 2: -3, 3: "string" }
assert (Array.prototype.findLast.call (data, function (e) { return e < 5; }) === undefined);

// Checking behavior when unable to get element
obj = {}
obj.length = 1;
Object.defineProperty (obj, '0', { 'get' : function () { throw new ReferenceError ("foo"); } });
obj.findLast = Array.prototype.findLast;

try {
  obj.findLast (function () { return undefined; });
  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}

// Checking behavior when the first argument is not a callback
let array = [1, 2, 3];

try {
  array.findLast (5);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

// Checking behavior when the first argument does not exist
try {
  array.findLast ();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

// Checking behavior when the there are more than 2 arguments
assert (array.findLast (function (e) { return e < 2 }, {}, 8, 4, 5, 6, 6) === 1);

function func (element) {
  return element > 8;
}

/* ES v6.0 22.1.3.8.8.c
   Checking behavior when the first element deletes the second */
function f() { delete arr[1]; }
let arr = [0, 1, 2, 3];
Object.defineProperty(arr, '0', { 'get' : f });
Array.prototype.findLast.call(arr, func);

/* ES v6.0 22.1.3.8.8
   Checking whether predicate is called also for empty elements */
let count = 0;

[,,,].findLast(function() { count++; return false; });
assert (count === 3);
