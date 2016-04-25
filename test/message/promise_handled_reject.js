'use strict';
const common = require('../common');

// Flags: --expose-gc

var p = new Promise(function(res, rej) {
  consol.log('oops'); // eslint-disable-line no-undef
});

// Manually call GC due to possible memory contraints with attempting to
// trigger it "naturally".
setTimeout(common.mustCall(() => {
  p.catch(() => {});
  gc();
  gc();
  gc();
}, 1), 2);
