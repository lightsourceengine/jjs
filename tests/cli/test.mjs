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

// ensure this file has been parsed as a module
assert(typeof import.meta.url === 'string');

// ensure the "test" api globals have been provided by the jjs cli
assert(typeof assert == 'function');
assert(typeof queueAsyncAssert == 'function');
assert(typeof createRealm == 'function');
assert(typeof print == 'function');

import { test } from 'jjs:test'

test ('this is a test', () => {});
test ('this is another test', () => {});
