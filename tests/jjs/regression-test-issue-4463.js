// Copyright JS Foundation and other contributors, http://js.foundation
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

function Regression4463Error(message) {
  this.message = message || "";
}

Regression4463Error.prototype.toString = function () {
  return "Regression4463Error: " + this.message;
};

var newTarget = function () {}.bind(null);
Object.defineProperty(newTarget, "prototype", {
  get() {
    throw new Regression4463Error();
  },
});

var typedArrayConstructors = [
  Float64Array,
  Float32Array,
  Int32Array,
  Int16Array,
  Int8Array,
  Uint32Array,
  Uint16Array,
  Uint8Array,
  Uint8ClampedArray,
];

for (var type of typedArrayConstructors) {
  try {
    Reflect.construct(Uint8ClampedArray, [], newTarget);
  } catch (error) {
    if (!(error instanceof Regression4463Error)) {
      throw "error must be instanceof Regression4463Error";
    }
  }
}
