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

const { encode, encodeInto, decodeUTF8 } = module.bindings;

const { toStringTag } = Symbol;
const encodingsUTF8 = ['utf-8', 'utf8', 'unicode-1-1-utf-8'];

class TextEncoder {
  get encoding() {
    return 'utf-8';
  }

  encode(str) {
    return str === undefined ? new Uint8Array(0) : encode(stringify(str));
  }

  encodeInto(str, buffer) {
    return encodeInto(stringify(str), buffer);
  }

  [toStringTag] = 'TextEncoder';
}

class TextDecoder {
  #ignoreBOM;
  #fatal;
  #label;

  constructor(label = undefined, options = undefined) {
    if (
      label === undefined ||
      encodingsUTF8.includes(label) ||
      encodingsUTF8.includes(label?.toLowerCase())
    ) {
      this.#label = 'utf-8';
    } else {
      throw new ERR_DECODER_ENCODING(label);
    }

    this.#fatal = !!options?.fatal;
    this.#ignoreBOM = !!options?.ignoreBOM;
  }

  get encoding() {
    return this.#label;
  }

  get fatal() {
    return this.#fatal;
  }

  get ignoreBOM() {
    return this.#ignoreBOM;
  }

  decode(buffer, options = undefined) {
    if (options?.stream) {
      throw TypeError('TextDecoder decode() stream option is not implemented');
    }

    return buffer === undefined ? '' : decodeUTF8(buffer, this.#ignoreBOM, this.#fatal);
  }

  [toStringTag] = 'TextDecoder';
}

class ERR_DECODER_ENCODING extends TypeError {
  constructor(label) {
    super(`Failed to construct 'TextDecoder': The encoding label provided ('${label}') is invalid`);
    this.code = 'ERR_DECODER_ENCODING';
  }
}

function stringify(str) {
  return typeof str === 'string' ? str : `${str}`;
}

module.exports = {
  TextEncoder,
  TextDecoder,
}
