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

let start;

if (globalThis.console?.time) {
  console.time('loop');
} else {
  start = Date.now();
}

for (let i = 0; i < 200000; i++)
{
  eval("1 + 1");
}

if (globalThis.console?.time) {
  console.timeEnd('loop');
} else {
  const { print = ((s) => jjs.stdout.write(s)) } = globalThis;

  print(`${Date.now() - start}ms\n`);
}
