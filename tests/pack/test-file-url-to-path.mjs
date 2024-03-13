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

// tests adapted from https://github.com/nodejs/node/test/parallel/test-url-fileurltopath.js

const { fileURLToPath } = require('jjs:url');
const { test, runAllTests } = require('../lib/test.cjs');
const { assertThrows, assertEquals } = require('../lib/assert.js');
const isWindows = globalThis['@platform'] === 'win32';

// note: this test does not belong here, but jjs tests have no fileURLToPath() capabilities
test('fileURLToPath() should parse import.meta.url', () => {
  fileURLToPath(import.meta.url); // throws on invalid url
});

test('fileURLToPath() should throw for invalid args', () => {
  for (const arg of [null, undefined, 1, {}, true]) {
    assertThrows(TypeError, () => fileURLToPath(arg));
  }
});

test('fileURLToPath() should throw for non-file URL', () => {
  assertThrows(TypeError, () => fileURLToPath('https://a/b/c'));
});

test('fileURLToPath() should handle file host on windows', () => {
  if (isWindows) {
    assertEquals(fileURLToPath(new URL('file://host/a')), '\\\\host\\a');
  }
});

test('fileURLToPath() should throw TypeError when passed a file host on non-windows platforms', () => {
  if (!isWindows) {
    assertThrows(TypeError, () => fileURLToPath(new URL('file://host/a')));
  }
});

test('fileURLToPath() should throw TypeError for invalid file URL paths', () => {
  if (isWindows) {
    assertThrows(TypeError, () => fileURLToPath('file:///C:/a%2F/'));
    assertThrows(TypeError, () => fileURLToPath('file:///C:/a%5C/'));
    assertThrows(TypeError, () => fileURLToPath('file:///?:/'));
  } else {
    assertThrows(TypeError, () => fileURLToPath('file:///a%2F/'));
  }
});

test('fileURLToPath() should convert file path to file url', () => {
  let testCases;
  if (isWindows) {
    testCases = [
      // Lowercase ascii alpha
      { path: 'C:\\foo', fileURL: 'file:///C:/foo' },
      // Uppercase ascii alpha
      { path: 'C:\\FOO', fileURL: 'file:///C:/FOO' },
      // dir
      { path: 'C:\\dir\\foo', fileURL: 'file:///C:/dir/foo' },
      // trailing separator
      { path: 'C:\\dir\\', fileURL: 'file:///C:/dir/' },
      // dot
      { path: 'C:\\foo.mjs', fileURL: 'file:///C:/foo.mjs' },
      // space
      { path: 'C:\\foo bar', fileURL: 'file:///C:/foo%20bar' },
      // question mark
      { path: 'C:\\foo?bar', fileURL: 'file:///C:/foo%3Fbar' },
      // number sign
      { path: 'C:\\foo#bar', fileURL: 'file:///C:/foo%23bar' },
      // ampersand
      { path: 'C:\\foo&bar', fileURL: 'file:///C:/foo&bar' },
      // equals
      { path: 'C:\\foo=bar', fileURL: 'file:///C:/foo=bar' },
      // colon
      { path: 'C:\\foo:bar', fileURL: 'file:///C:/foo:bar' },
      // semicolon
      { path: 'C:\\foo;bar', fileURL: 'file:///C:/foo;bar' },
      // percent
      { path: 'C:\\foo%bar', fileURL: 'file:///C:/foo%25bar' },
      // backslash
      { path: 'C:\\foo\\bar', fileURL: 'file:///C:/foo/bar' },
      // backspace
      { path: 'C:\\foo\bbar', fileURL: 'file:///C:/foo%08bar' },
      // tab
      { path: 'C:\\foo\tbar', fileURL: 'file:///C:/foo%09bar' },
      // newline
      { path: 'C:\\foo\nbar', fileURL: 'file:///C:/foo%0Abar' },
      // carriage return
      { path: 'C:\\foo\rbar', fileURL: 'file:///C:/foo%0Dbar' },
      // latin1
      { path: 'C:\\fóóbàr', fileURL: 'file:///C:/f%C3%B3%C3%B3b%C3%A0r' },
      // Euro sign (BMP code point)
      { path: 'C:\\€', fileURL: 'file:///C:/%E2%82%AC' },
      // Rocket emoji (non-BMP code point)
      { path: 'C:\\🚀', fileURL: 'file:///C:/%F0%9F%9A%80' },
      // UNC path (see https://docs.microsoft.com/en-us/archive/blogs/ie/file-uris-in-windows)
      { path: '\\\\nas\\My Docs\\File.doc', fileURL: 'file://nas/My%20Docs/File.doc' },
    ];
  } else {
    testCases = [
      // Lowercase ascii alpha
      { path: '/foo', fileURL: 'file:///foo' },
      // Uppercase ascii alpha
      { path: '/FOO', fileURL: 'file:///FOO' },
      // dir
      { path: '/dir/foo', fileURL: 'file:///dir/foo' },
      // trailing separator
      { path: '/dir/', fileURL: 'file:///dir/' },
      // dot
      { path: '/foo.mjs', fileURL: 'file:///foo.mjs' },
      // space
      { path: '/foo bar', fileURL: 'file:///foo%20bar' },
      // question mark
      { path: '/foo?bar', fileURL: 'file:///foo%3Fbar' },
      // number sign
      { path: '/foo#bar', fileURL: 'file:///foo%23bar' },
      // ampersand
      { path: '/foo&bar', fileURL: 'file:///foo&bar' },
      // equals
      { path: '/foo=bar', fileURL: 'file:///foo=bar' },
      // colon
      { path: '/foo:bar', fileURL: 'file:///foo:bar' },
      // semicolon
      { path: '/foo;bar', fileURL: 'file:///foo;bar' },
      // percent
      { path: '/foo%bar', fileURL: 'file:///foo%25bar' },
      // backslash
      { path: '/foo\\bar', fileURL: 'file:///foo%5Cbar' },
      // backspace
      { path: '/foo\bbar', fileURL: 'file:///foo%08bar' },
      // tab
      { path: '/foo\tbar', fileURL: 'file:///foo%09bar' },
      // newline
      { path: '/foo\nbar', fileURL: 'file:///foo%0Abar' },
      // carriage return
      { path: '/foo\rbar', fileURL: 'file:///foo%0Dbar' },
      // latin1
      { path: '/fóóbàr', fileURL: 'file:///f%C3%B3%C3%B3b%C3%A0r' },
      // Euro sign (BMP code point)
      { path: '/€', fileURL: 'file:///%E2%82%AC' },
      // Rocket emoji (non-BMP code point)
      { path: '/🚀', fileURL: 'file:///%F0%9F%9A%80' },
    ];
  }

  for (const { path, fileURL } of testCases) {
    const fromString = fileURLToPath(fileURL);
    assertEquals(fromString, path);
    const fromURL = fileURLToPath(new URL(fileURL));
    assertEquals(fromURL, path);
  }
});

runAllTests();
