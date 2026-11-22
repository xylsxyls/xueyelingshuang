'use strict';

// 实际NetPeer模块、宿主ABI及缓冲池；Socket替身不伪造TLS成功，不代表微信实机验证。
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const vm = require('node:vm');
const { performance } = require('node:perf_hooks');
const { readWasm } = require('./read_wasm');

async function checkModule(entry) {
    const factory = require(entry);
    const bits = factory.sdk.bits;
    let table;
    const module = await factory({
        wasmApi: WebAssembly,
        instantiateWasm(imports, receive) {
            const compiled = new WebAssembly.Module(readWasm(entry));
            const instance = new WebAssembly.Instance(compiled, imports);
            table = Object.values(instance.exports).find(value => value instanceof WebAssembly.Table);
            receive(instance, compiled);
            return instance.exports;
        }
    });
    let checks = 0;
    const check = (condition, label) => { ++checks; assert.ok(condition, label); };
    const equal = (actual, expected, label) => { ++checks; assert.deepEqual(actual, expected, label); };
    const slots = [], pending = [];
    let holdWrites = false;
    const runtime = await module.NetPeerRuntime.create({
        randomSource: size => crypto.randomFillSync(new Uint8Array(size)),
        hostBufferPools: { inputBytes: 2 * 1048576, outputBytes: 131072 },
        createSocket(events) {
            const slot = { events, writes: [], address: null, disposed: false, id: runtime.nextHost };
            slots.push(slot);
            return {
                connect(address, port) { slot.address = { address, port }; },
                write(bytes) {
                    slot.writes.push(bytes.slice());
                    if (holdWrites) return new Promise(resolve => pending.push({ resolve, bytes }));
                },
                close() { events.onClose(0); },
                dispose() { slot.disposed = true; }
            };
        }
    });
    check(table && (bits === 32 || bits === 64), 'actual callback table and pointer ABI');
    equal(module._netPeerPointerBytes(), bits / 8);
    const pointer = value => bits === 64 ? BigInt(value) : Number(value);
    const readPointer = address => {
        const view = new DataView(module.HEAPU8.buffer);
        return bits === 64 ? Number(view.getBigUint64(address, true)) : view.getUint32(address, true);
    };
    const callback = name => table.get(pointer(readPointer(runtime.memory + module.netPeerABI.offsets['host_' + name])));
    const liveBytes = pool => pool.reserved - pool.free.reduce((count, blocks, index) => count + blocks.length * pool.classes[index], 0);
    const closed = [];
    let callbackFailure;
    let sender;
    const listener = {
        onSessionReady() { callbackFailure = new Error('Raw fixture fabricated TLS readiness'); },
        onSessionResult() { callbackFailure = new Error('No business request was admitted'); },
        onSessionFrame() { callbackFailure = new Error('Unauthenticated application frame'); },
        onSessionClosed(generation, reason) {
            try { equal(sender.releaseGateway(), -7, 'reentrant destroy must be rejected'); }
            catch (error) { callbackFailure = error; }
            closed.push({ generation, reason });
        }
    };
    sender = runtime.createSender(listener);
    const options = { address: '127.0.0.1', port: 14230, platform: 3, device: new Uint8Array(16).fill(0x31), timeoutMs: 1000 };
    const scratch = Number(module._malloc(32768));
    check(scratch > 0, 'owned scratch');
    try {
        equal(sender.prepareGateway(), 0);
        equal(sender.prepareGateway(), 0);
        equal(runtime.clients, 1, 'idempotent prepare');
        equal(sender.connectServiceGateway(0, options), -1);
        equal(sender.connectServiceGateway(0x80000000, options), -1);
        equal(sender.gatewayGeneration(), 0n);
        for (let platform = 1; platform <= 3; ++platform) {
            equal(sender.connectServiceGateway(101, { ...options, platform }), 0);
            equal(sender.gatewayGeneration(), BigInt(platform));
            sender.closeGateway(-10);
            equal(sender.lastGatewayError(), 0);
            equal(sender.pollGateway(), false);
            equal(closed[platform - 1], { generation: BigInt(platform), reason: -10 });
        }
        equal(slots.length, 0, 'close before poll must not create a socket');
        equal(sender.configureCompression([17, 18]), 0);
        equal(runtime.configureGateways(sender, '127.0.0.1:14230'), 0);
        equal(sender.connectServiceGateway(101, { ...options, device: new Uint8Array(17) }), -1);
        equal(sender.cancelRequest(9007199254740993), -1, 'unsafe Number cannot become uint64');
        equal(sender.cancelRequest(9007199254740993n), -8, 'exact BigInt identifies an unregistered request');
        equal(sender.beginRequest(101, 17, 1000).requestId, 0n);
        const oldMemory = module.HEAPU8.buffer;
        const growth = Number(module._malloc(40 * 1048576));
        check(growth > 0 && module.HEAPU8.buffer !== oldMemory, 'actual memory growth');
        equal(sender.gatewayGeneration(), 3n, 'growth preserves native lifecycle');
        module._free(growth);
        equal(runtime.copyRandom(scratch, 32), 0);
        check(module.HEAPU8.subarray(scratch, scratch + 32).some(value => value !== 0), 'strong entropy bytes');
        equal(sender.connectServiceGateway(101, options), 0);
        sender.pollGateway();
        check(slots.length > 0, 'lazy poll creates actual transport hosts');
        const slot = slots.find(value => value.address !== null);
        check(slot, 'one transport actually starts connecting');
        equal(slot.address, { address: '127.0.0.1', port: 14230 });
        slot.events.onConnect();
        for (let count = 0; count < 8; ++count) sender.pollGateway();
        check(slot.writes.length > 0 && Buffer.from(slot.writes[0].subarray(0, 4)).toString('ascii') === 'NTW3', 'native framing reaches raw host');
        equal(Array.from(slot.writes[0].subarray(4, 6)), [3, 1]);
        const host = runtime.hosts.get(slot.id);
        check(host, 'raw ABI probe targets this exact socket instance');
        holdWrites = true;
        module.HEAPU8.fill(0x6d, scratch, scratch + 32768);
        for (let count = 0; count < 4; ++count) equal(callback('write')(pointer(host.id), 1n, pointer(scratch), 32768), 0);
        equal(callback('write')(pointer(host.id), 1n, pointer(scratch), 32768), 1, 'explicit pool exhausted');
        equal(liveBytes(runtime.outputPool), 131072);
        module.HEAPU8.fill(0, scratch, scratch + 32768);
        check(pending.every(item => item.bytes.every(value => value === 0x6d)), 'copied bytes remain valid until network completion');
        pending[0].resolve();
        await Promise.resolve();
        equal(liveBytes(runtime.outputPool), 98304, 'one completion returns one pool block');
        equal(callback('write')(pointer(host.id), 1n, pointer(scratch), 32768), 0, 'space is reusable');
        // 超过显式输入池只关闭该宿主；不再把旧固定消息个数当成容量规则。
        slot.events.onData(new Uint8Array(2 * 1048576 + 1));
        check(liveBytes(runtime.inputPool) <= 2 * 1048576 && host.fault === -3, 'input pool exhaustion');
        for (let count = 0; count < 8; ++count) sender.pollGateway();
        equal(liveBytes(runtime.inputPool), 0, 'failure returns partial allocations');
        check(!sender.gatewayReady(), 'unauthenticated host cannot become ready');
        sender.closeGateway(-10);
        for (let count = 0; count < 8; ++count) sender.pollGateway();
        equal(sender.releaseGateway(), 0);
        equal(runtime.clients, 0);
        equal(runtime.hosts.size, 0);
        check(slot.disposed, 'raw host disposal');
        equal(liveBytes(runtime.outputPool), 131072, 'close must retain buffers borrowed by unfinished writes');
        for (const item of pending) item.resolve();
        await Promise.resolve();
        equal(liveBytes(runtime.outputPool), 0);
        check(pending.every(item => item.bytes.every(value => value === 0)), 'returned buffers are wiped');
        slot.events.onConnect(); slot.events.onData(new Uint8Array(2097153)); slot.events.onClose(-99);
        equal(runtime.hosts.size, 0, 'stale callbacks cannot resurrect a host');
        equal(liveBytes(runtime.inputPool), 0);
        equal(callback('write')(pointer(host.id), 1n, pointer(scratch), 32), -1, 'stale raw host cannot send');
        equal(callback('destroy')(pointer(host.id)), 0, 'repeated host cleanup is idempotent');
        check(!callbackFailure, callbackFailure && callbackFailure.stack);
    } finally {
        for (const item of pending) item.resolve();
        await Promise.resolve();
        sender.closeGateway(0);
        for (let count = 0; count < 8; ++count) sender.pollGateway();
        sender.releaseGateway();
        module.HEAPU8.fill(0, scratch, scratch + 32768); module._free(scratch);
    }
    // 完整坏帧必须进入真实解析路径并终止连接，不能用未收齐的八字节冒充拒绝测试。
    const oldSlots = slots.length;
    try {
        equal(sender.prepareGateway(), 0);
        equal(sender.connectServiceGateway(101, options), 0);
        sender.pollGateway();
        const malformed = slots.slice(oldSlots).find(value => value.address !== null);
        check(malformed, 'malformed input owns a fresh connection');
        malformed.events.onConnect();
        for (let count = 0; count < 8; ++count) sender.pollGateway();
        malformed.events.onData(new Uint8Array([0x4e, 0x54, 0x57]));
        for (let count = 0; count < 8; ++count) sender.pollGateway();
        check(!malformed.disposed && runtime.hosts.has(malformed.id), 'partial valid prefix waits for more bytes');
        malformed.events.onData(new Uint8Array(21).fill(0x58));
        for (let count = 0; count < 16; ++count) sender.pollGateway();
        // 发现连接故障允许后续换入口；这里判定损坏的实际Socket被释放，而不是要求整个客户端退出。
        check(!sender.gatewayReady() && malformed.disposed && !runtime.hosts.has(malformed.id),
            'complete invalid wire header closes the exact unauthenticated socket');
        check(!callbackFailure, callbackFailure && callbackFailure.stack);
    } finally {
        sender.closeGateway(0);
        for (let count = 0; count < 8; ++count) sender.pollGateway();
        equal(sender.releaseGateway(), 0);
        check(runtime.clients === 0 && runtime.hosts.size === 0 && liveBytes(runtime.inputPool) === 0);
    }
    // 平台转接不自设20连接上限；实际平台拒绝仍由原调用结果表达。
    const wxSockets = [];
    const wx = {
        getRandomValues(options) { options.success({ randomValues: new Uint8Array(options.length).fill(0x73).buffer }); },
        createTCPSocket() {
            const value = { callbacks: {}, onConnect(f) { this.callbacks.connect = f; }, onMessage(f) { this.callbacks.data = f; },
                onError(f) { this.callbacks.error = f; }, onClose(f) { this.callbacks.close = f; },
                offConnect() {}, offMessage() {}, offError() {}, offClose() {}, connect() {}, write() {}, close() {} };
            wxSockets.push(value); return value;
        }
    };
    const first = module.NetPeerRuntime.wxOptions(wx), second = module.NetPeerRuntime.wxOptions(wx);
    let events = 0;
    const callbacks = { onConnect() { ++events; }, onData() { ++events; }, onError() { ++events; }, onClose() { ++events; } };
    for (let count = 0; count < 21; ++count) {
        const socket = (count % 2 ? first : second).createSocket(callbacks);
        const raw = wxSockets[count];
        raw.callbacks.connect(); socket.dispose();
        raw.callbacks.connect(); raw.callbacks.data({ message: new Uint8Array(1).buffer }); raw.callbacks.close(); raw.callbacks.error({});
    }
    equal(wxSockets.length, 21);
    equal(events, 21, 'disposed platform callbacks are inert');
    equal(await first.randomSource(32), new Uint8Array(32).fill(0x73));
    const rejectedModule = await factory({ wasmBinary: readWasm(entry) });
    await assert.rejects(rejectedModule.NetPeerRuntime.create({ randomSource: () => new Uint8Array(8191), createSocket() {} }), /short buffer/);
    ++checks;
    // 平台时钟通过模块参数借用；不向宿主全局写入performance，也不使用可回拨的Date.now。
    const sandbox = { module: { exports: {} }, WebAssembly, console };
    sandbox.exports = sandbox.module.exports;
    vm.runInNewContext(fs.readFileSync(entry, 'utf8'), sandbox, { filename: 'NetPeer-without-global-clock.js' });
    check(sandbox.performance === undefined && typeof sandbox.module.exports === 'function', 'clock-free host loader');
    let clockCalls = 0;
    const platformClock = Object.freeze({ now() { ++clockCalls; return performance.now(); } });
    const isolated = await sandbox.module.exports({ wasmBinary: readWasm(entry), wasmApi: WebAssembly, performance: platformClock });
    check(isolated.netPeerABI.pointerBytes === bits / 8 && sandbox.performance === undefined, 'clock-free module instance');
    const isolatedRuntime = await isolated.NetPeerRuntime.create({
        randomSource: size => crypto.randomFillSync(vm.runInContext('new Uint8Array(' + size + ')', sandbox)),
        createSocket(events) {
            return { connect() { events.onConnect(); }, write() {}, close() { events.onClose(0); }, dispose() {} };
        }
    });
    let isolatedClosed = 0;
    let isolatedUnexpected = false;
    const isolatedSender = isolatedRuntime.createSender({
        onSessionReady() { isolatedUnexpected = true; },
        onSessionResult() { isolatedUnexpected = true; },
        onSessionFrame() { isolatedUnexpected = true; },
        onSessionClosed() { ++isolatedClosed; }
    });
    try {
        equal(isolatedSender.prepareGateway(), 0);
        equal(isolatedSender.connectServiceGateway(101, { ...options,
            device: vm.runInContext('new Uint8Array(16).fill(0x31)', sandbox) }), 0);
        isolatedSender.pollGateway();
        isolatedSender.closeGateway(0);
        for (let count = 0; count < 8; ++count) isolatedSender.pollGateway();
        equal(isolatedClosed, 1, 'native lifecycle runs without a global clock object');
    } finally {
        isolatedSender.closeGateway(0);
        for (let count = 0; count < 8; ++count) isolatedSender.pollGateway();
        equal(isolatedSender.releaseGateway(), 0);
    }
    check(isolatedRuntime.clients === 0 && isolatedRuntime.hosts.size === 0 && sandbox.performance === undefined);
    check(clockCalls > 0, 'native lifecycle actually uses the injected monotonic clock');
    check(!isolatedUnexpected, 'raw clock fixture cannot synthesize authenticated callbacks');
    return { pass: true, bits, checks, scope: 'raw-host pool, lifecycle and ABI; no TLS-ready or official-device claim' };
}

async function main() {
    const [entry, report] = process.argv.slice(2);
    assert.equal(process.argv.length, 4);
    assert.ok(path.isAbsolute(entry) && path.isAbsolute(report) && !fs.existsSync(report));
    const result = await checkModule(entry);
    fs.writeFileSync(report, JSON.stringify(result, null, 2), { flag: 'wx' });
    console.log(JSON.stringify(result));
}

main().catch(error => { console.error(error.stack || error); process.exitCode = 1; });