/* Copyright JS Foundation and other contributors, http://js.foundation
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

import {ns1 as obj, ns2} from "./module-export-10.mjs"
import {ns3} from "./module-export-10.mjs"

assert(typeof obj === "object")
assert(obj === ns2)
assert(obj === ns3)

assert(obj.x === 41)
try {
  obj.x = 42
  assert(false)
} catch (e) {
  assert(e instanceof TypeError)
}
assert(obj.x === 41)
