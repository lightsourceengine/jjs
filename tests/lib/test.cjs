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

// Test API
//
// The Test API provides a node test runner-like interface for testing. The current
// JJS test runner has an assert system that aborts immediately and no test
// reporter at the test level. The API is limited as to what features it can
// support.
//
// To run tests, the test file must call runAllTests() at the end of the file.

const tests = []

function test(description, testFunctionOrOptions, testFunction) {
  let options;

  if (typeof testFunctionOrOptions === 'function') {
    testFunction = testFunctionOrOptions
    options = undefined
  } else {
    options = testFunctionOrOptions
  }

  tests.push({
    description,
    testFunction,
    options,
  })
}

function runAllTests() {
  for (const obj of tests) {
    if (obj.options?.skip) {
      continue;
    }

    try {
      obj.fn()
    } catch (e) {
      print(`Unhandled exception in test: "${obj.description}"`);
      print(`${e}`);
      throw e;
    }
  }
}

module.exports = { test, runAllTests }
