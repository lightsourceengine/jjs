// Copyright (C) 2017 Ecma International.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.
/*---
description: |
    Used in website/scripts/sth.js
defines: [setTimeout]
---*/

globalThis.setTimeout = function(callback, delay) {
  const p = Promise.resolve();
  const start = Date.now();
  const end = start + delay;
  function check(){
    const timeLeft = end - Date.now();
    if(timeLeft > 0)
      p.then(check);
    else
      callback();
  }
  p.then(check);
}
