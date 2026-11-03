'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const vm = require('node:vm');
const { NetWorkRuntime, NetSender } = require('../../NetWork/NetWork/NetSender');

// Raw socket fixture. It neither implements nor duplicates any gateway protocol.
class Socket {
    constructor() { this.listeners = {}; this.writes = []; this.closed = false; }
    onConnect(f) { this.listeners.connect = f; }
    onMessage(f) { this.listeners.message = f; }
    onError(f) { this.listeners.error = f; }
    onClose(f) { this.listeners.close = f; }
    offConnect(f) { if (this.listeners.connect === f) delete this.listeners.connect; }
    offMessage(f) { if (this.listeners.message === f) delete this.listeners.message; }
    offError(f) { if (this.listeners.error === f) delete this.listeners.error; }
    offClose(f) { if (this.listeners.close === f) delete this.listeners.close; }
    connect(address) { this.address = address; }
    write(buffer) { this.writes.push(new Uint8Array(buffer).slice()); }
    close() {
        if (this.closed) return;
        this.closed = true;
        if (this.listeners.close) this.listeners.close();
    }
}

async function checkModule(directory, bits) {
    const factory = require(path.resolve(directory, 'NetWork.js'));
    const moduleOptions = { wasmBinary: fs.readFileSync(path.join(directory, 'NetWork.wasm')) };
    const sockets = [];
    const runtime = await NetWorkRuntime.load(factory, { moduleOptions,
        createSocket() { const socket = new Socket(); sockets.push(socket); return socket; },
        randomSource: size => crypto.randomBytes(size) });
    const module = runtime.module;
    let checks = 0;
    function check(value) { ++checks; assert.ok(value); }
    check(module._netWorkWasmPointerBytes() === bits / 8);
    let platformTime = 0;
    let platformSockets = 0;
    const wx = { getPerformance: () => ({ now: () => platformTime }),
        createTCPSocket() { ++platformSockets; return new Socket(); }, getRandomValues() {} };
    const firstOptions = NetWorkRuntime.wxOptions(wx);
    const otherOptions = NetWorkRuntime.wxOptions(wx);
    for (let index = 0; index < 20; ++index) firstOptions.createSocket();
    check(platformSockets === 20);
    assert.throws(() => otherOptions.createSocket(), /budget exhausted/); ++checks;
    platformTime = 300000;
    check(otherOptions.createSocket() instanceof Socket && platformSockets === 21);
    platformTime = NaN;
    assert.throws(() => firstOptions.createSocket(), /monotonic platform clock/); ++checks;
    check(module.wasmBits === bits && module.wasmBigInt === true);
    check(module._netWorkWasmClientCreate(0, 0) === -1);
    const scratch = runtime.allocate(32);
    check(module._netWorkSenderClientEnsureGateway(scratch) === -11);
    check(new DataView(module.HEAPU8.buffer).getUint32(scratch, true) === 0);
    check(module._netWorkWasmClientCreate(0, scratch) === -1 && runtime.pointer(scratch) === 0);
    const closed = [];
    let client;
    let releaseResult;
    const callbacks = {
        onSessionReady() { throw new Error('Malformed raw fixture must never be ready'); },
        onSessionResult() { throw new Error('No request was admitted'); },
        onSessionFrame() { throw new Error('No application frame may be delivered'); },
        onSessionClosed(generation, reason, detail) {
            releaseResult = client.releaseGateway();
            closed.push({ generation, reason, detail });
        }
    };
    client = new NetSender(runtime, callbacks);
    check(client.prepareGateway() === 0 && client.handle !== 0);
    check(client.prepareGateway() === 0 && runtime.clients.size === 1);
    const options = { address: '127.0.0.1', port: 14230, platform: 3, device: new Uint8Array(16).fill(0x31), timeoutMs: 1000 };
    check(client.connectServiceGateway(0, options) === -1);
    check(client.connectServiceGateway(0x80000000, options) === -1);
    check(client.gatewayGeneration() === 0n);
    for (let platform = 1; platform <= 3; ++platform) {
        options.platform = platform;
        check(client.connectServiceGateway(101, options) === 0);
        check(client.gatewayGeneration() === BigInt(platform));
        check(client.closeGateway(-10) === 0);
        check(client.pollGateway() === 0);
        check(releaseResult === -7 && client.handle !== 0);
        check(closed.length === platform && closed[platform - 1].generation === BigInt(platform) && closed[platform - 1].reason === -10);
    }
    check(sockets.length === 0);
    check(client.configureCompression([17, 18]) === 0);
    check(client.configureGateways('127.0.0.1:14230', '') === 0);
    assert.throws(() => client.connectServiceGateway(101, Object.assign({}, options, { device: new Uint8Array(17) }))); ++checks;
    assert.throws(() => client.cancel(9007199254740993)); ++checks;
    check(client.cancel(9007199254740993n) === -8);
    check(client.beginRequest(101, 17, 1000).requestId === 0n);
    const originalAllocate = runtime.allocate;
    let held = 0;
    module.HEAPU8.fill(0x6d, scratch, scratch + 16);
    const borrowed = module.HEAPU8.subarray(scratch, scratch + 16);
    const borrowedBuffer = borrowed.buffer;
    runtime.allocate = function(size) {
        held = Number(module._malloc(20 * 1024 * 1024));
        assert.ok(held);
        return originalAllocate.call(runtime, size);
    };
    try {
        runtime.withBytes(borrowed, (pointer, size) => {
            check(module.HEAPU8.buffer !== borrowedBuffer);
            check(size === 16 && runtime.host.range(pointer, size).every(value => value === 0x6d));
        });
    } finally {
        runtime.allocate = originalAllocate;
        if (held) module._free(held);
    }
    const originalBuffer = module.HEAPU8.buffer;
    const growth = runtime.allocate(4194304);
    const large = Number(module._malloc(40 * 1024 * 1024));
    check(large !== 0 && module.HEAPU8.buffer !== originalBuffer);
    check(client.gatewayState().generation === 3n);
    check(runtime.host.call(10, 0, scratch, 32, 0n, 0) === 0);
    check(module.HEAPU8.subarray(scratch, scratch + 32).some(value => value !== 0));
    module._free(large);
    module._free(growth);
    options.platform = 3;
    check(client.connectServiceGateway(101, options) === 0);
    check(client.pollGateway() === 2);
    check(sockets.length >= 1);
    const socket = sockets[0];
    check(socket.address.address === '127.0.0.1' && socket.address.port === 14230);
    socket.listeners.connect();
    for (let index = 0; index < 8; ++index) client.pollGateway();
    assert.ok(socket.writes.length > 0 && Buffer.from(socket.writes[0].subarray(0, 4)).toString('ascii') === 'NTW3',
        JSON.stringify({ writes: socket.writes.map(bytes => Array.from(bytes.subarray(0, 16))), closed: socket.closed,
            hosts: runtime.host.instances.size, error: String(client.lastError) })); ++checks;
    check(socket.writes[0][4] === 3 && socket.writes[0][5] === 1);
    const oldMessage = socket.listeners.message;
    const oldConnect = socket.listeners.connect;
    const instanceId = Array.from(runtime.host.instances.keys())[0];
    // Unauthenticated wire garbage is handled by C++, never by a JS frame parser.
    socket.listeners.message({ message: new Uint8Array([0, 1, 2, 3, 4, 5, 6, 7]).buffer });
    for (let index = 0; index < 8; ++index) client.pollGateway();
    check(!client.gatewayReady());
    check(client.closeGateway(-10) === 0);
    for (let index = 0; index < 8 && client.pollGateway() !== 0; ++index) {}
    check(client.releaseGateway() === 0 && runtime.clients.size === 0 && runtime.host.instances.size === 0);
    oldMessage({ message: new Uint8Array(2097153).buffer });
    oldConnect();
    module._netWorkWasmHostConnected(instanceId, 0xffffffff00000001n);
    module._netWorkWasmHostData(instanceId, 0xffffffff00000001n, 0, 0);
    module._netWorkWasmHostWritable(instanceId, 0xffffffff00000001n);
    module._netWorkWasmHostClosed(instanceId, 0xffffffff00000001n, -99);
    check(runtime.host.instances.size === 0 && runtime.host.queuedInput === 0 && runtime.host.queuedOutput === 0);
    // Raw buffer tests use the real ABI bridge but never create a protocol session.
    const rawId = runtime.host.lastId + 100;
    check(runtime.host.call(0, rawId, 0, 0, 0n, 0) === 0);
    runtime.withText('127.0.0.1', address => check(runtime.host.call(1, rawId, address, 0, 1n, 14231) === 0));
    const raw = sockets[sockets.length - 1];
    raw.listeners.connect();
    raw.listeners.message({ message: new Uint8Array(2097153).buffer });
    check(runtime.host.instances.get(rawId).closed && runtime.host.queuedInput === 0);
    check(runtime.host.call(7, rawId, 0, 0, 0n, 0) === 0);
    check(runtime.host.call(0, rawId, 0, 0, 0n, 0) === -1);
    const queueId = rawId + 1;
    check(runtime.host.call(0, queueId, 0, 0, 0n, 0) === 0);
    runtime.withText('127.0.0.1', address => check(runtime.host.call(1, queueId, address, 0, 1n, 14231) === 0));
    const queued = sockets[sockets.length - 1];
    queued.listeners.connect();
    runtime.withBytes(new Uint8Array(32768), (pointer, size) => {
        for (let index = 0; index < 4; ++index) check(runtime.host.call(3, queueId, pointer, size, 1n, 0) === 0);
        check(runtime.host.call(3, queueId, pointer, size, 1n, 0) === 1);
    });
    check(runtime.host.queuedOutput === 131072);
    check(runtime.host.call(7, queueId, 0, 0, 0n, 0) === 0 && runtime.host.queuedOutput === 0);
    module._free(scratch);
    runtime.dispose();
    check(runtime.host.random.length === 0);
    await assert.rejects(() => NetWorkRuntime.load(factory, { moduleOptions, createSocket: () => new Socket(),
        randomSource: () => new Uint8Array(65535) }), /incorrect length/); ++checks;
    // Simulate a mini-program realm without a global performance object.
    const sandbox = { module: { exports: {} }, WebAssembly, console };
    sandbox.exports = sandbox.module.exports;
    vm.runInNewContext(fs.readFileSync(path.join(directory, 'NetWork.js'), 'utf8'), sandbox,
        { filename: 'NetWork-without-global-clock.js' });
    check(typeof sandbox.module.exports === 'function' && sandbox.performance === undefined);
    const isolated = await NetWorkRuntime.load(sandbox.module.exports, { moduleOptions,
        createSocket: () => new Socket(), randomSource: size => crypto.randomBytes(size), clock: () => performance.now() });
    let observedClose = 0;
    const isolatedClient = new NetSender(isolated, { onSessionReady() {}, onSessionResult() {},
        onSessionFrame() { return false; }, onSessionClosed() { ++observedClose; } });
    try {
        check(isolatedClient.prepareGateway() === 0);
        check(isolatedClient.connectServiceGateway(101, options) === 0);
        check(isolatedClient.closeGateway(0) === 0 && isolatedClient.pollGateway() === 0);
        check(observedClose === 1 && sandbox.performance === undefined);
    } finally {
        isolatedClient.releaseGateway();
        isolated.dispose();
    }
    console.log(JSON.stringify({ bits, checks, status: 'PASS' }));
}

async function main() {
    await checkModule(process.argv[2], 32);
    await checkModule(process.argv[3], 64);
}

main().catch(error => { console.error(error.stack); process.exitCode = 1; });
