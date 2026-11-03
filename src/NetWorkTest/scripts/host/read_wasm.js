'use strict';
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');

// Test caller loader: the SDK has no filesystem or CookWx dependency.
function readWasm(entry) {
    const filename = path.resolve(entry);
    const factory = require(filename);
    if (!factory.sdk || !factory.sdk.wasm) return fs.readFileSync(filename.replace(/\.js$/, '.wasm'));
    const local = path.join(path.dirname(filename), factory.sdk.wasm);
    const file = fs.existsSync(local) ? local : path.resolve(path.dirname(filename), '../../lib', factory.sdk.wasm);
    const bytes = fs.readFileSync(file);
    if (crypto.createHash('sha256').update(bytes).digest('hex') !== factory.sdk.wasmSha256)
        throw new Error('SDK JS/Wasm identity mismatch: ' + file);
    return bytes;
}
// Account test hosts use this same SDK reader and independent Account loading; the caller owns request dispatch.
async function loadAccountForTest(networkRuntime, entry) {
    const filename = path.resolve(entry || path.join(__dirname, '../../../../include/Account/AccountAPI.js'));
    const { loadAccountRuntime } = require('./AccountRuntime');
    return loadAccountRuntime(require(filename), networkRuntime, {
        noInitialRun: true, wasmBinary: readWasm(filename)
    });
}
module.exports = { readWasm, loadAccountForTest };