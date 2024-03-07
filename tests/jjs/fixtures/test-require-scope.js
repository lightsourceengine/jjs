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
assert(typeof require === 'function', 'expected require in scope');

// test: require has a resolve function
assert(typeof require.resolve === 'function', 'expected require.resolve() to be a function');

// test cache exists
assert('cache' in require, 'expected cache to be a property of require');

// test: require function is unique for this module
assert(require !== globalThis.require, 'expected a unique require() function for this module');

// test: module exists
assert(typeof module === 'object', 'expected module to be in scope');
assert('id' in module, 'expected id property on module');
assert('filename' in module, 'expected filename property on module');
assert('loaded' in module, 'expected loaded property on module');
assert('exports' in module, 'expected exports property on module');
assert('path' in module, 'expected path property on module');
assert(module.loaded === false);

// test: exports exists
assert(typeof exports === 'object', 'expected exports object to be in scope');
assert(module.exports === exports, 'expected module.exports to equal exports');

// test: __filename exists
assert(typeof __filename === 'string', 'expected __filename to be in scope');

// test: __dirname exists
assert(typeof __dirname === 'string', 'expected __dirname to be in scope');
assert(__filename.startsWith(__dirname), 'expected __filename to start with __dirname');
