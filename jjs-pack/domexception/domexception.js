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

const errorTable = {
  IndexSizeError: ['INDEX_SIZE_ERR', 1 ],
  DOMStringSizeError: ['DOMSTRING_SIZE_ERR', 2 ],
  HierarchyRequestError: ['HIERARCHY_REQUEST_ERR', 3 ],
  WrongDocumentError: ['WRONG_DOCUMENT_ERR', 4 ],
  InvalidCharacterError: ['INVALID_CHARACTER_ERR', 5 ],
  NoDataAllowedError: ['NO_DATA_ALLOWED_ERR', 6 ],
  NoModificationAllowedError: ['NO_MODIFICATION_ALLOWED_ERR', 7 ],
  NotFoundError: ['NOT_FOUND_ERR', 8 ],
  NotSupportedError: ['NOT_SUPPORTED_ERR', 9 ],
  InUseAttributeError: ['INUSE_ATTRIBUTE_ERR', 10 ],
  InvalidStateError: ['INVALID_STATE_ERR', 11 ],
  SyntaxError: ['SYNTAX_ERR', 12 ],
  InvalidModificationError: ['INVALID_MODIFICATION_ERR', 13 ],
  NamespaceError: ['NAMESPACE_ERR', 14 ],
  InvalidAccessError: ['INVALID_ACCESS_ERR', 15 ],
  ValidationError: ['VALIDATION_ERR', 16 ],
  TypeMismatchError: ['TYPE_MISMATCH_ERR', 17 ],
  SecurityError: ['SECURITY_ERR', 18 ],
  NetworkError: ['NETWORK_ERR', 19 ],
  AbortError: ['ABORT_ERR', 20 ],
  URLMismatchError: ['URL_MISMATCH_ERR', 21 ],
  QuotaExceededError: ['QUOTA_EXCEEDED_ERR', 22 ],
  TimeoutError: ['TIMEOUT_ERR', 23 ],
  InvalidNodeTypeError: ['INVALID_NODE_TYPE_ERR', 24 ],
  DataCloneError: ['DATA_CLONE_ERR', 25 ],
};

class DOMException extends Error {
  constructor(message = '', name = 'Error') {
    super(message);
    this.name = typeof name === 'string' ? name : `${name}`;
    this.code = errorTable[this.name]?.[1] ?? 0;
  }
}

for (const [id, code] of Object.values(errorTable)) {
  DOMException[id] = code;
}

module.exports = DOMException;
