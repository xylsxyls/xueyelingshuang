'use strict';

// Real native nodes.cache -> production Wasm C ABI -> native-compatible cache.
// Pure C++ unit checks live in NetInterface.cpp's conditional main.
const fs = require('fs');
const path = require('path');
const assert = require('assert').strict;
const crypto = require('crypto');
const { spawnSync } = require('child_process');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { NetInterfaceDirectory } = require('./host/NetInterfaceDirectory');

async function main() {
    const options = {};
    for (const value of process.argv.slice(2)) {
        const split = value.indexOf('=');
        const name = value.slice(0, split);
        assert.ok(split > 0 && ['--module', '--cache', '--server', '--output'].includes(name) && !options[name]);
        options[name] = value.slice(split + 1);
    }
    for (const name of ['--module', '--output']) assert.ok(options[name], 'Missing ' + name);
    assert.ok(!!options['--cache'] !== !!options['--server'], 'Choose --cache or --server');
    assert.ok(!fs.existsSync(options['--output']), 'Use a fresh report');
    const modulePath = path.resolve(options['--module']);
    if (options['--server']) {
        const fixture = path.resolve(options['--output'] + '.fixture');
        const result = spawnSync(process.execPath, [path.join(__dirname, 'mesh_gateway_process_test.js'),
            '--server=' + path.resolve(options['--server']), '--module=' + modulePath, '--output=' + fixture],
            { timeout: 75000, windowsHide: true, encoding: 'utf8' });
        fs.writeFileSync(options['--output'] + '.fixture.log', (result.stdout || '') + (result.stderr || ''));
        assert.equal(result.status, 0, 'Native cache fixture failed: ' + (result.error || result.signal || result.status));
        options['--cache'] = path.join(fixture, 'nodes-live.cache');
    }
    const runtime = await NetInterfaceRuntime.load(require(modulePath), {
        randomSource: length => Promise.resolve(Uint8Array.from(crypto.randomBytes(length))),
        moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) }
    });
    const cache = Uint8Array.from(fs.readFileSync(options['--cache']));
    const directory = new NetInterfaceDirectory(runtime, cache, true);
    let exported;
    let original;
    try {
        original = directory.points();
        assert.ok(original.length >= 2);
        assert.ok(original.every(point => point.port > 0 && point.boot.length === 2 && point.boot.some(value => value !== 0)));
        exported = directory.cache();
    } finally { directory.dispose(); }
    const roundtrip = new NetInterfaceDirectory(runtime, exported, true);
    try { assert.deepEqual(roundtrip.points(), original); } finally { roundtrip.dispose(); }
    const malformed = exported.slice();
    malformed[malformed.length - 1] ^= 1;
    assert.throws(() => new NetInterfaceDirectory(runtime, malformed, true), error => error.code === -2); // NI_PROTOCOL_ERROR
    assert.throws(() => new NetInterfaceDirectory(runtime, exported.subarray(0, 47), true));
    // Extract an independent fixed cache envelope's raw protobuf to verify both C ABI parsers agree.
    const raw = cache.slice(16, cache.length - 32);
    const discovery = new NetInterfaceDirectory(runtime, raw);
    try { assert.deepEqual(discovery.points(), original); } finally { discovery.dispose(); }
    const result = { pass: true, nodes: original.length, nativeCacheParsedByWasm: true,
        fixedWidthBootPreserved: true, cacheRoundtrip: true, corruptedAndTruncatedRejected: true };
    fs.writeFileSync(options['--output'], JSON.stringify(result, null, 2));
    console.log('Native directory cache and production Wasm C ABI interoperability: PASS');
}

main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });