'use strict';
const common = require('../common');

const p1 = new Promise(function(res, rej) {
  consol.log('One'); // eslint-disable-line no-undef
});

new Promise(function(res, rej) {
  consol.log('Two'); // eslint-disable-line no-undef
});

const p3 = new Promise(function(res, rej) {
  consol.log('Three'); // eslint-disable-line no-undef
});

new Promise(function(res, rej) {
  setTimeout(common.mustCall(() => {
    p1.catch(() => {});
    p3.catch(() => {});
  }));
});
