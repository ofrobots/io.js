V8 Inspector Integration for Node.js
====================================

V8 Inspector integration allows attaching Chrome DevTools to Node.js
instances for debugging and profiling.

## Building

To enable V8 Inspector integration, run configure script with `--with-inspector`
flag. Afterwards, use `make` to build Node.js as usual.

## Running

V8 Inspector can be enabled by passing `--inspect` flag when starting Node.js
application. It is also possible to supply a custom port with that flag, e.g.
`--inspect=9222` will expect DevTools connection on the port 9222.

To break on the first line of the application code, provide the `--debug-brk`
flag in addition to `--inspect`.
