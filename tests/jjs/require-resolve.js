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
const { assertThrows } = require('./lib/assert.js');

// test: resolve() should resolve relative path request
const path = globalRequire.resolve('./lib/assert.js');

assert(path.endsWith('assert.js'));

// test: resolve() should throw TypeError when request is not specified
assertThrows(TypeError, () => { globalRequire.resolve(); });

// test: resolve() should throw TypeError when request is not a string
assertThrows(TypeError, () => { globalRequire.resolve(23); });

// test: resolve() should throw error when request does not match a pmap entry
assertThrows(Error, () => { globalRequire.resolve('invalid-package'); });

// test: resolve() should throw Error when relative file path does not exist
assertThrows(Error, () => { globalRequire.resolve('./invalid-package.js'); });
