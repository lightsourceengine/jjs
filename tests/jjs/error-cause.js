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

const message = 'test';
const errors = [
  TypeError,
  SyntaxError,
  URIError,
  EvalError,
  RangeError,
  ReferenceError,
];
const emptyObject = {};
const emptyArray = [];
const sym = Symbol();
const causes = [
  null,
  undefined,
  emptyObject,
  emptyArray,
  0,
  NaN,
  Infinity,
  3n,
  'str',
  false,
  true,
  sym,
];

for (const ctor of errors) {
  // Expect no argument Error constructor should initialize cause to undefined.
  assert(ctor().cause === undefined);

  // Expect Error constructor to ignore non-object options value
  for (const cause of causes) {
    assert(ctor(message, cause).cause === undefined);
  }

  // Expect Error constructor to set cause to options.cause
  for (const cause of causes) {
    if (Number.isNaN(cause)) {
      assert(Number.isNaN(ctor(message, { cause }).cause));
    } else {
      assert(ctor(message, { cause }).cause === cause);
    }
  }
}

// Expect no argument AggregateError constructor should initialize cause to undefined.
assert(AggregateError([]).cause === undefined);

// Expect Error constructor to ignore non-object options value
for (const cause of causes) {
  assert(AggregateError([], message, cause).cause === undefined);
}

// Expect AggregateError constructor to set cause to options.cause
for (const cause of causes) {
  if (Number.isNaN(cause)) {
    assert(Number.isNaN(AggregateError([], message, { cause }).cause));
  } else {
    assert(AggregateError([], message, { cause }).cause === cause);
  }
}
