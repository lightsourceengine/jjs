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

// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/isWellFormed#examples
const strings = [
  // TODO: re-enable tests when isWellFormed() is ready
  // Lone leading surrogate
  // ["ab\uD800", false],
  // ["ab\uD800c", false],
  // Lone trailing surrogate
  // ["\uDFFFab", false],
  // ["c\uDFFFab", false],
  // Well-formed
  // ["", true],
  // ["abc", true],
  // ["ab\uD83D\uDE04c", true],
];

for (const [ str, expected ] of strings) {
  assert(str.isWellFormed() === expected, `'${str}'.isWellFormed() expected to return ${expected}`);
}
