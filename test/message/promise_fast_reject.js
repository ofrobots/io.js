'use strict';
require('../common');

new Promise(function(res, rej) {
  consol.log('One'); // eslint-disable-line no-undef
});

new Promise(function(res, rej) {
  consol.log('Two'); // eslint-disable-line no-undef
});
