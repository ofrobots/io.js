'use strict';
const common = require('../common');

// Flags: --expose-gc

new Promise(function(res, rej) {
  consol.log('oops'); // eslint-disable-line no-undef
});

// Manually call GC due to possible memory contraints with attempting to
// trigger it "naturally".
setTimeout(common.mustCall(() => {
  gc();
  gc();
  gc();
}, 1), 2);
