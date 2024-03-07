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

const { dirname, filename, url } = import.meta;

assert(typeof dirname === 'string', 'expected import.meta.dirname to be a string');
assert(dirname.endsWith('jjs'), 'expected import.meta.dirname basename to be this directory');

assert(typeof filename === 'string', 'expected import.meta.filename to be a string');
assert(filename.startsWith(dirname), 'expected import.meta.filename to start with dirname');
assert(filename.endsWith('module-import-meta.mjs'), 'expected import.meta.filename file to be this filename');

assert(typeof url === 'string', 'expected import.meta.url to be a string');
assert(url.startsWith('file:'), 'expected import.meta.url to start with file: protocol');
assert(url.endsWith('module-import-meta.mjs'), 'expected import.meta.url to end with this filename');
