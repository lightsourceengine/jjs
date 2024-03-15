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

const { read, readUTF8, readJSON, size, copy, writeBuffer, writeString, remove } = module.bindings;

function file(path) {
  if (typeof path !== 'string' || path.length === 0) {
    throw new ERR_INVALID_ARG_TYPE('path', path, 'string');
  }

  return new JJSFile(path);
}

function write(destination, input) {
  // TODO: options: mode, encoding, append

  destination = validatePath(destination, 'destination');

  if (typeof input === 'string') {
    return writeString(destination, input);
  }

  if (isJJSFile(input)) {
    return copy(destination, validatePath(input, 'input'));
  }

  return writeBuffer(destination, input);
}

class JJSFile {
  #path;
  #size;

  constructor(path) {
    this.#path = path;
  }

  arrayBuffer() {
    return read(this.#path);
  }

  text() {
    return readUTF8(this.#path);
  }

  json() {
    return readJSON(this.#path);
  }

  remove() {
    remove(this.#path);
  }

  get size() {
    return this.#size ?? (this.#size = size(this.#path));
  }

  get __path() {
    return this.#path;
  }

  [Symbol.toStringTag] = 'JJSFile';
}

class ERR_INVALID_ARG_TYPE extends TypeError {
  constructor(name, actual, expected = undefined) {
    super(`"${name}" must be of type ${expected ?? 'string or JJSFile'}. Received ${actual?.toString() ?? actual}`);
    this.code = 'ERR_INVALID_ARG_TYPE';
  }
}

function isJJSFile(value) {
  return value?.constructor?.name === JJSFile.name;
}

function validatePath(path, name) {
  if (isJJSFile(path)) {
    path = path.__path;
  }

  if (typeof path !== 'string' || path.length === 0) {
    throw new ERR_INVALID_ARG_TYPE(name, path);
  }

  return path;
}

module.exports = {
  file,
  write,
};
