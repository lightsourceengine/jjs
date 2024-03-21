#!/usr/bin/env jjs

// Keep first line to test #! is parsed.
// Tests are basic. test262 does more thorough tests of this feature.

const { assertThrows, assertEquals } = require('../lib/assert.js');
const { test, runAllTests } = require('../lib/test.cjs');

const functionConstructors = [
  Function,
  (async function (){}).constructor,
  (function *(){}).constructor,
  (async function *(){}).constructor,
];

test('Function constructor should throw SyntaxError for #! in function body', () => {
  for (const ctor of functionConstructors) {
    assertThrows(SyntaxError, () => ctor('#!\n_'), `${ctor.name} Call body`);
    assertThrows(SyntaxError, () => new ctor('#!\n_'), `${ctor.name} Construct body`);
  }
});

test('Function constructor should throw SyntaxError for #! in function argument', () => {
  for (const ctor of functionConstructors) {
    assertThrows(SyntaxError, () => ctor('#!\n_', ''), `${ctor.name} Call argument`);
    assertThrows(SyntaxError, () => new ctor('#!\n_', ''), `${ctor.name} Construct argument`);
  }
});

test('eval should accept #! as first line', () => {
  assertEquals(eval("#!\n5"), 5);
  assertEquals(eval("#!/jjs\n5"), 5);
});

test('eval should throw SyntaxError if #! is not first', () => {
  assertThrows(SyntaxError, () => eval("let x = 1; #!\n"));
});

runAllTests();
