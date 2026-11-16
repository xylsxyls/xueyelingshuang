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
// Constants must come from the selected SDK, never from a shared repository include directory.
function readAccountProtocol(entry) {
    if (!path.isAbsolute(entry || '')) throw new Error('An absolute Account module entry is required');
    const protocol = require(path.join(path.dirname(entry), 'AccountProtocol.js'));
    for (const name of ['ACCOUNT_LOGOUT', 'ACCOUNT_RESET_PASSWORD']) {
        if (!protocol || !Number.isInteger(protocol[name]) || protocol[name] < 0 || protocol[name] > 0xffffffff)
            throw new Error('Invalid Account protocol constant: ' + name);
    }
    return protocol;
}

async function loadAccountForTest(networkRuntime, entry) {
    const filename = path.resolve(entry || path.join(__dirname, '../../../include/Account/AccountAPI.js'));
    const { loadAccountRuntime } = require('./AccountRuntime');
    return loadAccountRuntime(require(filename), networkRuntime, {
        noInitialRun: true, wasmBinary: readWasm(filename)
    });
}
module.exports = { readWasm, readAccountProtocol, loadAccountForTest };