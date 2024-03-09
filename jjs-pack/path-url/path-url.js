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

const CHAR_LOWERCASE_A = 97
const CHAR_LOWERCASE_Z = 122
const CHAR_FORWARD_SLASH = 47
const CHAR_BACKWARD_SLASH = 92

const { toStringTag, replace } = Symbol
const SymbolReplace = RegExp.prototype[replace];
const RegExpPrototypeSymbolReplace = (self, ...args) => SymbolReplace.call(self, ...args)

const FORWARD_SLASH = /\//g;
const { isWindows } = module.bindings;
let resolve;
let sep = isWindows ? '\\' : '/';

function domainToASCII(domain) {
  // TODO: implement conversion in native code
  return domain;
}

function domainToUnicode(domain) {
  // TODO: implement conversion in native code
  return domain;
}

function getPathFromURLWin32(url) {
  let  { hostname, pathname } = url;
  const { length } = pathname;
  for (let n = 0; n < length; n++) {
    if (pathname[n] === '%') {
      const third = pathname.codePointAt(n + 2) | 0x20;
      if ((pathname[n + 1] === '2' && third === 102) || // 2f 2F /
        (pathname[n + 1] === '5' && third === 99)) {  // 5c 5C \
        throw new ERR_INVALID_FILE_URL_PATH(
          'must not include encoded \\ or / characters',
        );
      }
    }
  }
  pathname = RegExpPrototypeSymbolReplace(FORWARD_SLASH, pathname, '\\');
  pathname = decodeURIComponent(pathname);
  if (hostname !== '') {
    // If hostname is set, then we have a UNC path
    // Pass the hostname through domainToUnicode just in case
    // it is an IDN using punycode encoding. We do not need to worry
    // about percent encoding because the URL parser will have
    // already taken care of that for us. Note that this only
    // causes IDNs with an appropriate `xn--` prefix to be decoded.
    return `\\\\${domainToUnicode(hostname)}${pathname}`;
  }
  // Otherwise, it's a local path that requires a drive letter
  const letter = pathname.codePointAt(1) | 0x20;
  const sep = pathname.charAt(2);
  if (letter < CHAR_LOWERCASE_A || letter > CHAR_LOWERCASE_Z ||   // a..z A..Z
    (sep !== ':')) {
    throw new ERR_INVALID_FILE_URL_PATH('must be absolute');
  }
  return pathname.slice(1);
}

function getPathFromURLPosix(url) {
  if (url.hostname !== '') {
    throw new ERR_INVALID_FILE_URL_HOST();
  }
  const { pathname } = url;
  const { length } = pathname;
  for (let n = 0; n < length; n++) {
    if (pathname[n] === '%') {
      const third = pathname.codePointAt(n + 2) | 0x20;
      if (pathname[n + 1] === '2' && third === 102) {
        throw new ERR_INVALID_FILE_URL_PATH('must not include encoded / characters');
      }
    }
  }
  return decodeURIComponent(pathname);
}

// The following characters are percent-encoded when converting from file path
// to URL:
// - %: The percent character is the only character not encoded by the
//        `pathname` setter.
// - \: Backslash is encoded on non-windows platforms since it's a valid
//      character but the `pathname` setters replaces it by a forward slash.
// - LF: The newline character is stripped out by the `pathname` setter.
//       (See whatwg/url#419)
// - CR: The carriage return character is also stripped out by the `pathname`
//       setter.
// - TAB: The tab character is also stripped out by the `pathname` setter.
const percentRegEx = /%/g;
const backslashRegEx = /\\/g;
const newlineRegEx = /\n/g;
const carriageReturnRegEx = /\r/g;
const tabRegEx = /\t/g;

function encodePathChars(filepath) {
  if (filepath.includes('%'))
    filepath = RegExpPrototypeSymbolReplace(percentRegEx, filepath, '%25');
  // In posix, backslash is a valid character in paths:
  if (!isWindows && filepath.includes('\\'))
    filepath = RegExpPrototypeSymbolReplace(backslashRegEx, filepath, '%5C');
  if (filepath.includes('\n'))
    filepath = RegExpPrototypeSymbolReplace(newlineRegEx, filepath, '%0A');
  if (filepath.includes('\r'))
    filepath = RegExpPrototypeSymbolReplace(carriageReturnRegEx, filepath, '%0D');
  if (filepath.includes('\t'))
    filepath = RegExpPrototypeSymbolReplace(tabRegEx, filepath, '%09');
  return filepath;
}

function fileURLToPath(path) {
  assertURLPack();
  if (typeof path === 'string')
    path = new URL(path);
  else if (!(path instanceof URL))
    throw new ERR_INVALID_ARG_TYPE('path', ['string', 'URL'], path);
  if (path.protocol !== 'file:')
    throw new ERR_INVALID_URL_SCHEME();
  return isWindows ? getPathFromURLWin32(path) : getPathFromURLPosix(path);
}

function pathToFileURL(filepath) {
  assertURLPack();
  const outURL = new URL('file://');
  if (isWindows && filepath.startsWith('\\\\')) {
    // UNC path format: \\server\share\resource
    const hostnameEndIndex = filepath.indexOf('\\', 2);
    if (hostnameEndIndex === -1) {
      throw new ERR_INVALID_ARG_VALUE(
        'filepath',
        filepath,
        'Missing UNC resource path',
      );
    }
    if (hostnameEndIndex === 2) {
      throw new ERR_INVALID_ARG_VALUE(
        'filepath',
        filepath,
        'Empty UNC servername',
      );
    }
    const hostname = filepath.slice(2, hostnameEndIndex);
    outURL.hostname = domainToASCII(hostname);
    outURL.pathname = encodePathChars(
      RegExpPrototypeSymbolReplace(backslashRegEx, filepath.slice(hostnameEndIndex), '/'));
  } else {
    if (!resolve) {
      // lazy load because jjs:path may not exist (so other code paths work) and we cannot control load order of packs.
      resolve = lazyLoadResolve();
    }
    let resolved = resolve(filepath);
    // path.resolve strips trailing slashes so we must add them back
    const filePathLast = filepath.charCodeAt(filepath.length - 1);
    if ((filePathLast === CHAR_FORWARD_SLASH ||
        (isWindows && filePathLast === CHAR_BACKWARD_SLASH)) &&
      resolved[resolved.length - 1] !== sep)
      resolved += '/';
    outURL.pathname = encodePathChars(resolved);
  }
  return outURL;
}

function lazyLoadResolve() {
  try {
    return require('jjs:path').resolve;
  } catch (e) {
    throw Error('pathToFileURL(): jjs:path pack not installed');
  }
}

class ERR_INVALID_ARG_TYPE extends TypeError {
  constructor(name, expected, actual) {
    super(`The "${name}" argument must be of type ${expected}. Received ${typeof actual}`);
    this.code = 'ERR_INVALID_ARG_TYPE';
  }
}

class ERR_INVALID_ARG_VALUE extends TypeError {
  constructor(name, value, reason = 'is value') {
    super(`The ${ name.includes('.') ? 'property' : 'argument' } '${name}' ${reason}. Received ${typeof value}`);
    this.code = 'ERR_INVALID_ARG_VALUE';
  }
}

class ERR_INVALID_URL_SCHEME extends TypeError {
  constructor() {
    super('The URL must be of scheme \'file:\'.');
    this.code = 'ERR_INVALID_URL_SCHEME';
  }
}

class ERR_INVALID_FILE_URL_PATH extends TypeError {
  constructor(arg) {
    super(`File URL path ${arg}`);
    this.code = 'ERR_INVALID_FILE_URL_PATH';
  }
}

class ERR_INVALID_FILE_URL_HOST extends TypeError {
  constructor() {
    super(`File URL host must be "localhost" or empty on this platform`);
    this.code = 'ERR_INVALID_FILE_URL_HOST';
  }
}

function assertURLPack() {
  if (globalThis.URL?.prototype?.[toStringTag] !== 'URL') {
    throw TypeError('URL pack is not installed');
  }
}

module.exports = {
  fileURLToPath,
  pathToFileURL,
};
