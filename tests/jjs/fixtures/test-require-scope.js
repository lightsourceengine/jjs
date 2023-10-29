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

// test: require function exists
assert(typeof require === 'function');

// test: require has a resolve function
assert(typeof require.resolve === 'function');

// test cache exists
assert('cache' in require);

// test: require function is unique for this module
assert(require !== globalThis.require);

// test: module exists
assert(typeof module === 'object');
assert('id' in module);
assert('filename' in module);
assert('loaded' in module);
assert('exports' in module);
assert('path' in module);
assert(module.loaded === false);

// test: exports exists
assert(typeof exports === 'object');
assert(module.exports === exports);

// test: __filename exists
assert(typeof __filename === 'string');

// test: __dirname exists
assert(typeof __dirname === 'string');
assert(__filename.startsWith(__dirname));
