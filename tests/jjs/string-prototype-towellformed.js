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

// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/toWellFormed#examples
const strings = [
  // Lone leading surrogate
  ["\uD800", "�"],
  ["ab\uD800", "ab�"],
  ["\uD800ab", "�ab"],
  ["ab\uD800c", "ab�c"],
  // Lone trailing surrogate
  ["\uDFFF", "�"],
  ["ab\uDFFF", "ab�"],
  ["\uDFFFab", "�ab"],
  ["c\uDFFFab", "c�ab"],
  // Well-formed
  ["", ""],
  ["abc", "abc"],
  ["\uD83D\uDE04", "😄"],
  ["ab\uD83D\uDE04", "ab😄"],
  ["\uD83D\uDE04ab", "😄ab"],
  ["ab\uD83D\uDE04c", "ab😄c"],
];

for (const [input, expected] of strings) {
  const actual = input.toWellFormed();
  assert(actual === expected, `'${input}'.toWellFormed() -> '${actual}', but expected '${expected}'`);
}
