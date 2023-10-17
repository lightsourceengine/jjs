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

const { require:globalRequire } = globalThis;

assert(Object.keys(globalRequire.cache).length === 0);

globalRequire('./lib/assert.js');

const cacheKeys = Object.keys(globalRequire.cache);

assert(cacheKeys.length === 1);
assert(cacheKeys[0].endsWith('assert.js'));
