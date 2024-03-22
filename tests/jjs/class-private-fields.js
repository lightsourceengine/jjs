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

// Tests the parsing of private and bracketed class fields without semicolons at end of line.

let counter = 1

function genKey() {
  return `key${counter++}`
}

// private fields
class P1 {
  #m1 = 1
  #m2 = 2
  #m3 = ''
  #m4 = {}
  #m5 = []
  #m6 = () => {}
  #m7 = async () => {}
  #m8 = null
  #m9 = undefined
  #mA = true
  f () {}
}

// private fields with no initialization
class P2 {
  #m1
  #m2
  #m3
  #f () {}
}

// private static fields
class SP1 {
  static #m1 = 1
  static #m2 = 2
  static #m3 = ''
  static #m4 = {}
  static #m5 = []
  static #m6 = () => {}
  static #m7 = async () => {}
  static #m8 = null
  static #m9 = undefined
  static #mA = true

  f () {}
}

// private static fields with no initialization
class SP2 {
  static #m1
  static #m2
  static #m3
  #f () {}
}

// bracketed fields
class B1 {
  [genKey()] = 1
  [genKey()] = 2
  [genKey()] = ''
  [genKey()] = {}
  [genKey()] = []
  [genKey()] = () => {}
  [genKey()] = async () => {}
  [genKey()] = null
  [genKey()] = undefined
  [genKey()] = true
  f () {}
}

// bracketed fields with no initialization
class B2 {
  [genKey()]
  [genKey()]
  [genKey()]
  #f () {}
}

// static bracketed fields
class SB1 {
  static [genKey()] = 1
  static [genKey()] = 2
  static [genKey()] = ''
  static [genKey()] = {}
  static [genKey()] = []
  static [genKey()] = () => {}
  static [genKey()] = async () => {}
  static [genKey()] = null
  static [genKey()] = undefined
  static [genKey()] = true
  f () {}
}

// static bracketed fields with no initialization
class SB2 {
  static [genKey()]
  static [genKey()]
  static [genKey()]
  #f () {}
}

// mixed private fields
class M1 {
  #x1
  [genKey()] = () => {}
  #x2 = []
  static #x3
  [genKey()]
  get x() { return 1 }
  #f() {}
}
