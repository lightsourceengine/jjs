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

// tests adapted from https://github.com/nodejs/node/test/parallel/test-url-pathtofileurl.js

const { resolve } = require('jjs:path');
const { pathToFileURL } = require('jjs:path-url');
const { test, runAllTests } = require('../lib/test.cjs');
const { assertThrows, assertEquals } = require('../lib/assert.js');
// TODO: jjs needs an api to detect platform
const isWindows = !resolve(".").startsWith('/')

test('pathToFileURL() should create a file url from relative path', () => {
  const fileURL = pathToFileURL('test/').href;
  assert(fileURL.startsWith('file:///'));
  assert(fileURL.endsWith('/'));
});

test('pathToFileURL() should handle invalid windows unc paths', () => {
  if (isWindows) {
    // UNC path: \\server\share\resource

    // Missing server:
    assertThrows(TypeError, () => pathToFileURL('\\\\\\no-server'));

    // Missing share or resource:
    assertThrows(TypeError, () => pathToFileURL('\\\\host'));

    // Regression test for direct String.prototype.startsWith call
    assertThrows(TypeError, () => pathToFileURL([
      '\\\\',
      { [Symbol.toPrimitive]: () => 'blep\\blop' }
    ]));
    assertThrows(TypeError, () => pathToFileURL(['\\\\', 'blep\\blop']));
    assertThrows(TypeError, () => pathToFileURL({ [Symbol.toPrimitive]: () => '\\\\blep\\blop', }));

  } else {
    // UNC paths on posix are considered a single path that has backslashes:
    const fileURL = pathToFileURL('\\\\nas\\share\\path.txt').href;
    assert(fileURL.startsWith('file:///'));
    assert(fileURL.endsWith('%5C%5Cnas%5Cshare%5Cpath.txt'))
  }
});

test('pathToFileURL', () => {
  let testCases;
  if (isWindows) {
    testCases = [
      // Lowercase ascii alpha
      { path: 'C:\\foo', expected: 'file:///C:/foo' },
      // Uppercase ascii alpha
      { path: 'C:\\FOO', expected: 'file:///C:/FOO' },
      // dir
      { path: 'C:\\dir\\foo', expected: 'file:///C:/dir/foo' },
      // trailing separator
      { path: 'C:\\dir\\', expected: 'file:///C:/dir/' },
      // dot
      { path: 'C:\\foo.mjs', expected: 'file:///C:/foo.mjs' },
      // space
      { path: 'C:\\foo bar', expected: 'file:///C:/foo%20bar' },
      // question mark
      { path: 'C:\\foo?bar', expected: 'file:///C:/foo%3Fbar' },
      // number sign
      { path: 'C:\\foo#bar', expected: 'file:///C:/foo%23bar' },
      // ampersand
      { path: 'C:\\foo&bar', expected: 'file:///C:/foo&bar' },
      // equals
      { path: 'C:\\foo=bar', expected: 'file:///C:/foo=bar' },
      // colon
      { path: 'C:\\foo:bar', expected: 'file:///C:/foo:bar' },
      // semicolon
      { path: 'C:\\foo;bar', expected: 'file:///C:/foo;bar' },
      // percent
      { path: 'C:\\foo%bar', expected: 'file:///C:/foo%25bar' },
      // backslash
      { path: 'C:\\foo\\bar', expected: 'file:///C:/foo/bar' },
      // backspace
      { path: 'C:\\foo\bbar', expected: 'file:///C:/foo%08bar' },
      // tab
      { path: 'C:\\foo\tbar', expected: 'file:///C:/foo%09bar' },
      // newline
      { path: 'C:\\foo\nbar', expected: 'file:///C:/foo%0Abar' },
      // carriage return
      { path: 'C:\\foo\rbar', expected: 'file:///C:/foo%0Dbar' },
      // latin1
      { path: 'C:\\fóóbàr', expected: 'file:///C:/f%C3%B3%C3%B3b%C3%A0r' },
      // Euro sign (BMP code point)
      { path: 'C:\\€', expected: 'file:///C:/%E2%82%AC' },
      // Rocket emoji (non-BMP code point)
      { path: 'C:\\🚀', expected: 'file:///C:/%F0%9F%9A%80' },
      // UNC path (see https://docs.microsoft.com/en-us/archive/blogs/ie/file-uris-in-windows)
      { path: '\\\\nas\\My Docs\\File.doc', expected: 'file://nas/My%20Docs/File.doc' },
    ];
  } else {
    testCases = [
      // Lowercase ascii alpha
      { path: '/foo', expected: 'file:///foo' },
      // Uppercase ascii alpha
      { path: '/FOO', expected: 'file:///FOO' },
      // dir
      { path: '/dir/foo', expected: 'file:///dir/foo' },
      // trailing separator
      { path: '/dir/', expected: 'file:///dir/' },
      // dot
      { path: '/foo.mjs', expected: 'file:///foo.mjs' },
      // space
      { path: '/foo bar', expected: 'file:///foo%20bar' },
      // question mark
      { path: '/foo?bar', expected: 'file:///foo%3Fbar' },
      // number sign
      { path: '/foo#bar', expected: 'file:///foo%23bar' },
      // ampersand
      { path: '/foo&bar', expected: 'file:///foo&bar' },
      // equals
      { path: '/foo=bar', expected: 'file:///foo=bar' },
      // colon
      { path: '/foo:bar', expected: 'file:///foo:bar' },
      // semicolon
      { path: '/foo;bar', expected: 'file:///foo;bar' },
      // percent
      { path: '/foo%bar', expected: 'file:///foo%25bar' },
      // backslash
      { path: '/foo\\bar', expected: 'file:///foo%5Cbar' },
      // backspace
      { path: '/foo\bbar', expected: 'file:///foo%08bar' },
      // tab
      { path: '/foo\tbar', expected: 'file:///foo%09bar' },
      // newline
      { path: '/foo\nbar', expected: 'file:///foo%0Abar' },
      // carriage return
      { path: '/foo\rbar', expected: 'file:///foo%0Dbar' },
      // latin1
      { path: '/fóóbàr', expected: 'file:///f%C3%B3%C3%B3b%C3%A0r' },
      // Euro sign (BMP code point)
      { path: '/€', expected: 'file:///%E2%82%AC' },
      // Rocket emoji (non-BMP code point)
      { path: '/🚀', expected: 'file:///%F0%9F%9A%80' },
    ];
  }

  for (const { path, expected } of testCases) {
    assertEquals(pathToFileURL(path).href, expected);
  }
});

test('pathToFileURL() should throw for non-string parameter', () => {
  for (const badPath of [
    undefined, null, true, 42, 42n, Symbol('42'), NaN, {}, [], () => {},
    Promise.resolve('foo'),
    new Date(),
    new String('notPrimitive'),
    { toString() { return 'amObject'; } },
    { [Symbol.toPrimitive]: (hint) => 'amObject' },
  ]) {
    assertThrows(TypeError, () => pathToFileURL(badPath));
  }
});

runAllTests();
