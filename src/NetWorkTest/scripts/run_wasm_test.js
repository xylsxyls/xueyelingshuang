'use strict';

// Run the same C/C++ tests in real WebAssembly and copy the virtual report out.
// This host does not implement the frame protocol or replace native assertions.
const fs = require('fs');
const path = require('path');

// File-only host regression: native Node executes bytes only inside this test
// loader. The generated SDK can request packaged paths and cannot compile buffers.
async function loadFileOnlyPeer(modulePath, moduleOptions = {}) {
    const factory = require(modulePath);
    const native = WebAssembly;
    const probe = { fileLoads: 0, dynamicAttempts: 0, table: null };
    const packagedPath = '/network/' + factory.sdk.wasm;
    const api = Object.create(native);
    const rejectDynamic = () => {
        ++probe.dynamicAttempts;
        throw new TypeError('WXWebAssembly.Module: only support file type .wasm or .wasm.br');
    };
    api.Function = undefined;
    api.Module = function(file) {
        if (file !== packagedPath) return rejectDynamic();
        return new native.Module(require('./host/read_wasm').readWasm(modulePath));
    };
    api.compile = api.compileStreaming = api.instantiateStreaming = rejectDynamic;
    api.instantiate = async (file, imports) => {
        if (file !== packagedPath) return rejectDynamic();
        ++probe.fileLoads;
        const result = await native.instantiate(require('./host/read_wasm').readWasm(modulePath), imports);
        probe.table = Object.values(result.instance.exports).find(value => value instanceof native.Table);
        return result;
    };
    let rejectLoad;
    let timeout;
    const failed = new Promise((resolve, reject) => {
        rejectLoad = reject;
        timeout = setTimeout(() => reject(new Error('File-only module initialization timed out')), 10000);
    });
    try {
        const loading = factory({ ...moduleOptions, wasmApi: api,
            instantiateWasm(imports, receive) {
                api.instantiate(packagedPath, imports).then(result => receive(result.instance, result.module)).catch(rejectLoad);
                return {};
            }, onAbort: reason => rejectLoad(new Error(String(reason))) });
        return { module: await Promise.race([loading, failed]), probe };
    } finally { clearTimeout(timeout); }
}

// Direct trampoline invocations below check ABI and context routing only. They
// do not count as TLS/session/business acceptance; senderDelivery checks that.
async function peerFileHost(modulePath, reportPath) {
    const assert = require('node:assert/strict');
    const crypto = require('node:crypto');
    let checks = 0;
    const check = (value, label) => { ++checks; assert.ok(value, label); };
    const loaded = await loadFileOnlyPeer(modulePath);
    const { module, probe } = loaded;
    const record = { status: 'FAIL', stage: 'host callbacks', bits: module.netPeerABI.pointerBytes * 8,
        scope: 'File-only Node host and callback ABI regression; not an official WeChat result' };
    try {
        check(probe.fileLoads === 1 && probe.table, 'actual main module loaded from packaged path');
        const tableLength = Number(probe.table.length);
        const options = { randomSource: count => crypto.randomFillSync(new Uint8Array(count)),
            createSocket() { throw new Error('ABI-only test must not open a socket'); } };
        const entropyError = new Error('injected entropy failure');
        await assert.rejects(module.NetPeerRuntime.create({ ...options, randomSource() { throw entropyError; } }), error => error === entropyError);
        ++checks;
        const configure = module._netPeerConfigureHost;
        module._netPeerConfigureHost = () => -7;
        try {
            await assert.rejects(module.NetPeerRuntime.create(options), /NetPeer host configuration failed: -7/);
            ++checks;
        } finally { module._netPeerConfigureHost = configure; }
        // Both creators reach their asynchronous random host before installation.
        const creators = await Promise.allSettled([module.NetPeerRuntime.create(options), module.NetPeerRuntime.create(options)]);
        check(creators.filter(item => item.status === 'fulfilled').length === 1, 'one runtime wins concurrent initialization');
        check(creators.filter(item => item.status === 'rejected').length === 1, 'other initialization fails promptly');
        const runtime = creators.find(item => item.status === 'fulfilled').value;
        record.stage = 'sender callbacks';
        const offsets = module.netPeerABI.offsets;
        const ptr = value => record.bits === 64 ? BigInt(value) : value;
        const view = () => new DataView(module.HEAPU8.buffer);
        const readPointer = address => record.bits === 64 ? Number(view().getBigUint64(address, true)) : view().getUint32(address, true);
        const registrations = [];
        // Resolve Emscripten's lazy export before intercepting it; the first
        // native call otherwise replaces our interception with the real export.
        check(module._netPeerSenderClientCreate(0, 0) === -1, 'invalid create has no resources');
        const create = module._netPeerSenderClientCreate;
        let createFailure = null;
        module._netPeerSenderClientCreate = (callbacks, output) => {
            callbacks = Number(callbacks);
            const entry = { context: readPointer(callbacks + offsets.callbacks_context) };
            for (const name of ['ready', 'result', 'frame', 'closed']) entry[name] = readPointer(callbacks + offsets['callbacks_' + name]);
            registrations.push(entry);
            if (createFailure) throw createFailure;
            return create(callbacks, output);
        };
        const firstEvents = [], secondEvents = [];
        const listener = events => ({
            onSessionReady(...args) { events.push(['ready', ...args]); },
            onSessionResult(...args) { events.push(['result', ...args]); },
            onSessionFrame(...args) { events.push(['frame', ...args]); return false; },
            onSessionClosed(...args) { events.push(['closed', ...args]); }
        });
        const failedSender = runtime.createSender(listener(firstEvents));
        createFailure = new Error('injected client registration failure');
        check(failedSender.prepareGateway() === -5 && runtime.lastBindingError === createFailure, 'registration cause retained');
        createFailure = null;
        check(runtime.clients === 0, 'failed registration owns no client');
        const first = runtime.createSender(listener(firstEvents));
        const second = runtime.createSender(listener(secondEvents));
        check(first.prepareGateway() === 0 && second.prepareGateway() === 0, 'two independent Senders prepared');
        const failedEntry = registrations[0], a = registrations[1], b = registrations[2];
        check(a.context !== b.context && a.context !== failedEntry.context, 'context IDs are unique and not reused');
        const invoke = (entry, name, ...args) => probe.table.get(ptr(entry[name]))(ptr(entry.context), ...args);
        const generation = 0x20000000000001n;
        const request = 0x20000000000003n;
        invoke(failedEntry, 'ready', generation, 4, 4096);
        check(firstEvents.length === 0, 'failed registration ignores late callback');
        invoke(a, 'ready', generation, 4, 4096);
        invoke(b, 'ready', generation + 1n, 8, 8192);
        check(firstEvents[0][1] === generation && secondEvents[0][1] === generation + 1n, 'BigInt generations and listeners stay separate');
        const memory = Number(module._malloc(offsets.frameSize + 8));
        try {
            module.HEAPU8.fill(0, memory, memory + offsets.frameSize + 8);
            const payload = memory + offsets.frameSize;
            module.HEAPU8.set([65, 0, 66], payload);
            invoke(a, 'result', generation, request, 101, 7, -123, ptr(payload), 3);
            invoke(a, 'result', generation, request + 1n, 101, 7, 0, ptr(payload), 3);
            check(firstEvents[1][2] === request && firstEvents[2][2] === request + 1n, 'adjacent uint64 request IDs remain exact');
            module.HEAPU8[payload] = 90;
            check(firstEvents[1][6][0] === 65 && firstEvents[1][5] === -123, 'callback owns a copy and signed status is preserved');
            view().setUint32(memory + offsets.frame_m_productId, 101, true);
            view().setBigUint64(memory + offsets.frame_m_requestId, request, true);
            view().setUint32(memory + offsets.frame_m_messageType, 7, true);
            view().setUint16(memory + offsets.frame_m_kind, 5, true);
            view().setUint32(memory + offsets.frame_m_payloadSize, 3, true);
            if (record.bits === 64) view().setBigUint64(memory + offsets.frame_m_payload, BigInt(payload), true);
            else view().setUint32(memory + offsets.frame_m_payload, payload, true);
            check(invoke(a, 'frame', generation, ptr(memory)) === 0, 'frame backpressure return reaches Wasm');
            check(firstEvents[3][2].requestId === request && firstEvents[3][2].bytes[0] === 90, 'frame pointers and uint64 fields use target ABI');
            invoke(b, 'closed', generation + 1n, -7, -2147483648);
            check(secondEvents[1][2] === -7 && secondEvents[1][3] === -2147483648, 'closed signed detail preserved');
        } finally { module._free(memory); }
        first.releaseGateway(); second.releaseGateway();
        const eventCount = firstEvents.length + secondEvents.length;
        const next = runtime.createSender(listener(secondEvents));
        check(next.prepareGateway() === 0, 'new Sender after release');
        invoke(a, 'ready', generation, 1, 1024);
        invoke(b, 'closed', generation, 0, 0);
        check(firstEvents.length + secondEvents.length === eventCount, 'released contexts never target a new Sender');
        next.releaseGateway();
        for (let i = 0; i < 1000; ++i) {
            const sender = runtime.createSender(listener(firstEvents));
            check(sender.prepareGateway() === 0, 'repeated prepare ' + i);
            sender.releaseGateway();
        }
        check(runtime.clients === 0 && runtime.hosts.size === 0, 'all native client/host resources released');
        check(Number(probe.table.length) === tableLength, 'repeated creation never grows the function table');
        check(probe.dynamicAttempts === 0, 'no runtime Wasm compilation attempted');
        Object.assign(record, { status: 'PASS', stage: 'complete', checks, tableLength, fileLoads: probe.fileLoads,
            dynamicAttempts: probe.dynamicAttempts, senderRegistrations: registrations.length, sdk: require(modulePath).sdk });
    } catch (error) {
        Object.assign(record, { checks, dynamicAttempts: probe.dynamicAttempts, error: error.stack || String(error) });
        throw error;
    } finally {
        fs.mkdirSync(path.dirname(reportPath), { recursive: true });
        fs.writeFileSync(reportPath, JSON.stringify(record, null, 2));
    }
    console.log('NetPeer file-only callback regression: ' + checks + ' checks PASS (' + record.bits + ' bit)');
}


// Generated public binding, C++ lifecycle and copied raw-host events; no fake TLS success.
async function peerContract(modulePath, reportPath) {
    const assert = require('node:assert/strict');
    const crypto = require('node:crypto');
    let checks = 0;
    const check = condition => { ++checks; assert(condition, 'check ' + checks); };
    const factory = require(modulePath);
    const module = await factory({ wasmBinary: require('./host/read_wasm').readWasm(modulePath), wasmApi: WebAssembly });
    const bits = factory.sdk.bits;
    check(module._netPeerPointerBytes() === bits / 8);
    check(!Object.keys(module).some(name => name.startsWith('_netInterface')));
    check(!Object.keys(module).some(name => name.startsWith('_netPeerClient')));
    check(typeof module.NetPeerRuntime === 'function' && typeof module.NetSender === 'function');
    check(module.netPeerABI.offsets.callbacksSize === (bits === 64 ? 48 : 28));
    let sockets = 0;
    let disposed = 0;
    let writes = 0;
    const late = [];
    const runtime = await module.NetPeerRuntime.create({
        randomSource: count => crypto.randomFillSync(new Uint8Array(count)),
        createSocket(events) {
            ++sockets;
            late.push(events);
            return {
                connect() { events.onConnect(); },
                write(bytes) { check(bytes instanceof Uint8Array && bytes.length > 0); ++writes; },
                close() { events.onClose(0); },
                dispose() { ++disposed; }
            };
        }
    });
    const publicMethods = ['prepareGateway', 'connectGateway', 'connectServiceGateway', 'configureCompression', 'pollGateway',
        'beginRequest', 'sendRequest', 'postRequest', 'cancelRequest', 'sendData', 'postData', 'closeGateway', 'uninitGateway', 'createClient', 'destroyClient',
        'gatewayReady', 'gatewayGeneration', 'lastGatewayError'];
    check(publicMethods.every(name => typeof module.NetSender.prototype[name] === 'function'));
    let closed = 0;
    let inside = false;
    const listener = {
        onSessionReady() { throw new Error('mock raw TCP must not fabricate TLS ready'); },
        onSessionResult() { throw new Error('no accepted business request'); },
        onSessionFrame() { throw new Error('no authenticated application frame'); },
        onSessionClosed(generation) {
            check(generation > 0n);
            inside = true;
            sender.releaseGateway();
            check(sender.gatewayGeneration() === generation && sender.lastGatewayError() === -7);
            inside = false;
            ++closed;
        }
    };
    const entry = module.NetSender.instance();
    check(entry === module.NetSender.instance() && Object.keys(entry).length === 0);
    assert.throws(() => new module.NetSender(), /NetSender.instance/);
    const sender = entry.createClient(runtime, listener);
    check(entry.prepareGateway(sender) === 0);
    check(entry.pollGateway(sender) === 0 && entry.gatewayGeneration(sender) === 0n);
    check(entry.destroyClient(null) === 0);
    check(sender.configureCompression([]) === 0);
    check(sender.gatewayGeneration() === 0n && !sender.gatewayReady());
    check(sender.beginRequest(101, 1, 1000).result === -7);
    const options = { address: '127.0.0.1', port: 1, platform: 3, capabilities: 7, maxRequests: 4,
        maxChunk: 4096, timeoutMs: 1000, device: Uint8Array.from({ length: 16 }, () => 0x31) };
    check(sender.connectServiceGateway(0, options) === -1);
    check(sender.connectServiceGateway(101, Object.assign({ localGatewayPid: 1 }, options)) === -1);
    check(runtime.configureGateways(sender, 'invalid') === -1);
    check(runtime.configureGateways(sender, 'x'.repeat(4097)) === -1);
    check(runtime.configureGateways(sender, '127.0.0.1:1') === 0);
    check(sender.connectServiceGateway(101, options) === 0);
    check(runtime.configureGateways(sender, '127.0.0.1:2') === -7);
    check(sender.gatewayGeneration() === 1n);
    sender.releaseGateway();
    check(sender.lastGatewayError() === -7 && sender.gatewayGeneration() === 1n);
    for (let i = 0; i < 8; ++i) sender.pollGateway();
    check(sockets > 0 && writes > 0 && !sender.gatewayReady());
    check(sender.connectServiceGateway(101, options, Object.assign({}, listener, { onSessionClosed() { throw new Error('rejected listener changed live connection'); } })) === -7);
    sender.closeGateway(0);
    for (let i = 0; i < 8 && sender.pollGateway(); ++i) {}
    check(closed === 1 && !inside);
    sender.releaseGateway();
    check(sender.gatewayGeneration() === 0n && runtime.clients === 0 && runtime.hosts.size === 0 && sockets === disposed);
    for (const events of late) { events.onConnect(); events.onData(new Uint8Array([1, 2])); events.onClose(0); }
    check(runtime.hosts.size === 0 && closed === 1);
    // File-flow ABI is implemented by the outer module, not exported by NetInterface.
    check(typeof module._netPeerFileFlowCreateWire === 'function');
    check(typeof module._netPeerFileFlowDestroy === 'function');
    const record = { status: 'PASS', bits, checks, sockets, disposed, writes, sdk: factory.sdk,
        scope: 'Node binding/ABI/lifecycle only; not a WeChat device result' };
    fs.mkdirSync(path.dirname(reportPath), { recursive: true });
    fs.writeFileSync(reportPath, JSON.stringify(record, null, 2));
    console.log('NetPeer public JS binding: ' + checks + ' checks PASS (' + bits + ' bit)');
}


// The test application owns both independent modules and copies payloads between heaps.
// No product adapter, shared C++ handle, or runtime Account-to-Peer import is used.
async function accountBridge(peerEntry, accountEntry, reportPath, fileOnly = false) {
    const assert = require('node:assert/strict');
    const { readWasm } = require('./host/read_wasm');
    const { setModuleBits, readPointer, u64Arguments } = require('./host/WasmMemory');
    const accountFactory = require(accountEntry), peerFactory = require(peerEntry);
    const A = require(path.join(path.dirname(accountEntry), 'AccountProtocol.js'));
    let crossCalls = 0, checks = 0;
    const check = (condition, label) => { ++checks; assert(condition, label); };
    const options = { accountNetworkCall() { ++crossCalls; throw new Error('Account must not call another module'); } };
    // Account must initialize successfully before a network module even exists.
    const restrictedAccount = fileOnly ? await loadFileOnlyPeer(accountEntry, options) : null;
    const core = setModuleBits(restrictedAccount ? restrictedAccount.module :
        await accountFactory({ ...options, wasmBinary: readWasm(accountEntry) }), accountFactory);
    const restrictedPeer = fileOnly ? await loadFileOnlyPeer(peerEntry) : null;
    const peer = restrictedPeer ? restrictedPeer.module : await peerFactory({ wasmBinary: readWasm(peerEntry) });
    const bits = accountFactory.sdk.bits;
    check(bits === peerFactory.sdk.bits, 'matched pointer ABI');
    check(core.HEAPU8.buffer !== peer.HEAPU8.buffer, 'independent heaps');
    check(!Object.keys(core).some(name => name.startsWith('_netInterface') || name.startsWith('_netPeer')), 'Account exports its own business only');
    check(!Object.keys(peer).some(name => name.startsWith('_netInterface') || name.startsWith('_account')), 'Peer exports no inner protocol/account API');
    check(typeof core._accountClientBindRuntimeSender === 'undefined', 'removed hidden runtime sender binding');
    const cells = Number(core._malloc(16416));
    const peerBytes = Number(peer._malloc(8192));
    check(cells > 0 && peerBytes > 0, 'owned scratch allocation');
    const clients = [];
    const generation = [0x76543210, 0x12345678];
    const id = [0x89abcdef, 0x12345678];
    try {
        for (const product of [101, 102, 103]) {
            check(core._accountClientCreate(product, cells) === 0, 'create business instance');
            const handle = readPointer(core, cells); clients.push(handle);
            check(core._accountClientSetConnection(handle, ...u64Arguments(core, ...generation)) === 0, 'application supplies generation');
            check(core._accountClientRequest(handle, A.ACCOUNT_QUERY_CHANNELS, 0, 0, 5000, cells + 8) === -7, 'no implicit global sender');
            const build = (output, capacity) => core._accountClientBuildRequest(handle, A.ACCOUNT_QUERY_CHANNELS,
                ...u64Arguments(core, ...id), 0, 0, output, capacity, cells + 16);
            check(build(0, 0) === -4, 'size probe does not register a request');
            const size = new DataView(core.HEAPU8.buffer).getUint32(cells + 16, true);
            check(size > 0 && size <= 8192, 'bounded business payload');
            core.HEAPU8.fill(0xa5, cells + 32, cells + 32 + size);
            check(build(cells + 32, size - 1) === -4 && core.HEAPU8[cells + 32] === 0xa5, 'short buffer preserves output and request');
            check(build(cells + 32, size) === 0, 'own codec prepares the actual request');
            const bytes = core.HEAPU8.slice(cells + 32, cells + 32 + size);
            peer.HEAPU8.set(bytes, peerBytes);
            core.HEAPU8.fill(0, cells + 32, cells + 32 + size);
            check(Buffer.from(peer.HEAPU8.subarray(peerBytes, peerBytes + size)).equals(Buffer.from(bytes)), 'upper application copies bytes, never passes cross-module addresses');
            peer.HEAPU8.fill(0, peerBytes, peerBytes + size); bytes.fill(0);
            check(core._accountClientCancel(handle, ...u64Arguments(core, ...id)) === 0, 'application settles a rejected or cancelled transport');
            check(core._accountClientCancel(handle, ...u64Arguments(core, ...id)) === A.ACCOUNT_NOT_FOUND, 'unique terminal');
            check(core._accountClientApplyResponse(handle, ...u64Arguments(core, ...generation), ...u64Arguments(core, ...id),
                A.ACCOUNT_QUERY_CHANNELS, 0, 0, 0) !== 0, 'late response cannot revive cancelled request');
            const next = [id[0] + 1, id[1]];
            check(core._accountClientBuildRequest(handle, A.ACCOUNT_QUERY_CHANNELS, ...u64Arguments(core, ...next),
                0, 0, cells + 32, 8192, cells + 16) === 0, 'new request remains usable');
            check(core._accountClientSetConnection(handle, ...u64Arguments(core, 0, 0)) === 0, 'disconnect clears pending work');
            check(core._accountClientApplyResponse(handle, ...u64Arguments(core, ...generation), ...u64Arguments(core, ...next),
                A.ACCOUNT_QUERY_CHANNELS, 0, 0, 0) !== 0, 'old-generation response is rejected');
        }
        check(crossCalls === 0, 'no module-to-module runtime callback');
    } finally {
        for (const handle of clients) core._accountClientDestroy(handle);
        if (cells) { core.HEAPU8.fill(0, cells, cells + 16416); core._free(cells); }
        if (peerBytes) { peer.HEAPU8.fill(0, peerBytes, peerBytes + 8192); peer._free(peerBytes); }
    }
    const record = { status: 'PASS', bits, checks, account: accountFactory.sdk, peer: peerFactory.sdk,
        scope: 'independent compiled modules, application-owned payload copying and Account request lifecycle; no AccountServer authorization or real TCP claim' };
    if (fileOnly) {
        check(restrictedAccount.probe.fileLoads === 1 && restrictedPeer.probe.fileLoads === 1, 'one packaged load per module');
        check(restrictedAccount.probe.dynamicAttempts === 0 && restrictedPeer.probe.dynamicAttempts === 0, 'no dynamic mini-Wasm bridge');
        record.fileOnlyHost = { accountLoads: 1, peerLoads: 1, dynamicAttempts: 0 }; record.checks = checks;
    }
    fs.mkdirSync(path.dirname(reportPath), { recursive: true });
    fs.writeFileSync(reportPath, JSON.stringify(record, null, 2));
    console.log('Account/NetPeer independent modules: ' + checks + ' checks PASS (' + bits + ' bit)');
}


// Exercise the generated NetSender itself against owned TCP/TLS gateway/service
// processes. The gate rejects raw writes without side effects until released.
async function senderDelivery(modulePath, serverPath, reportPath, fileOnly = false) {
    const assert = require('node:assert/strict');
    const net = require('node:net');
    const crypto = require('node:crypto');
    const { start } = require('./local_link_gateway_process_test');
    const directory = path.dirname(reportPath);
    assert.ok(!fs.existsSync(reportPath), 'Use a fresh report');
    fs.mkdirSync(directory, { recursive: true });
    const children = [];
    const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
    const gate = { hold: false, writes: 0, bytes: 0 };
    const trace = [];
    const note = (event, detail) => {
        if (trace.length === 48) trace.shift();
        trace.push({ at: Date.now(), event, detail });
    };
    let socketSequence = 0;
    const factory = require(modulePath);
    const restricted = fileOnly ? await loadFileOnlyPeer(modulePath) : null;
    const module = restricted ? restricted.module : await factory({ wasmBinary: require('./host/read_wasm').readWasm(modulePath) });
    const runtime = await module.NetPeerRuntime.create({
        randomSource: count => crypto.randomFillSync(new Uint8Array(count)),
        createSocket(events) {
            let socket;
            const socketId = ++socketSequence;
            return {
                connect(address, port) {
                    note('tcp-connect', [socketId, address, port]);
                    socket = net.createConnection({ host: address, port });
                    socket.on('connect', () => { note('tcp-ready', socketId); events.onConnect(); });
                    socket.on('data', bytes => { note('tcp-data', [socketId, bytes.length]); events.onData(bytes); });
                    socket.on('error', error => { note('tcp-error', [socketId, error.code]); events.onError(error); });
                    socket.on('close', () => { note('tcp-close', socketId); events.onClose(0); });
                },
                write(bytes) {
                    if (gate.hold) return 1;
                    ++gate.writes; gate.bytes += bytes.length;
                    note('tcp-write', [socketId, bytes.length]);
                    // Node false means accepted with backpressure; it is not
                    // the raw-host's "not accepted" result.
                    socket.write(Buffer.from(bytes));
                    return 0;
                },
                pauseRead(paused) { if (socket) { if (paused) socket.pause(); else socket.resume(); } },
                close() { if (socket) socket.destroy(); },
                dispose() { if (socket) { socket.removeAllListeners(); socket.destroy(); } }
            };
        }
    });
    const owners = [];
    let finalRecord;
    let scenarioError = null;
    let checks = 0;
    const check = (condition, detail) => { ++checks; assert.ok(condition, detail); };
    const drive = async (sender, predicate, limit = 5000) => {
        const end = Date.now() + limit;
        while (!predicate() && Date.now() < end) { sender.pollGateway(); await delay(1); }
        check(predicate(), 'bounded NetPeer progress; trace=' + JSON.stringify(trace));
    };
    try {
        const node = start(serverPath, ['--protocol=3', '--listen=127.0.0.1', '--port=0',
            '--identity-directory=' + path.join(directory, 'identity'), '--stop-file=' + path.join(directory, 'node.stop'), '--run-for-ms=30000'],
            'sender-node', directory, /GATEWAY_READY port=(\d+) pid=(\d+)/);
        children.push(node);
        node.completed.then(result => note('node-exit', result.code));
        const ready = await node.ready;
        const service = start(serverPath, ['--self-test-service=' + ready[2], '--stop-file=' + path.join(directory, 'service.stop'), '--run-for-ms=30000'],
            'sender-service', directory, /SERVICE_PROBE_READY pid=(\d+)/);
        children.push(service);
        service.completed.then(result => note('service-exit', result.code));
        await service.ready;
        const connect = async () => {
            const observed = { ready: false, closed: 0, results: new Map(), reentry: null };
            const sender = runtime.createSender({
                onSessionReady() { note('session-ready', owners.length); observed.ready = true; observed.reentry = sender.sendRequest(0n, new Uint8Array(0)); },
                onSessionResult(generation, id, product, type, status, bytes) {
                    check(!observed.results.has(id) && product === 1 && type === 0x7345, 'unique authenticated request identity');
                    observed.results.set(id, { status, bytes });
                },
                onSessionFrame() { throw new Error('Unexpected push'); },
                onSessionClosed(generation, reason, detail) { note('session-closed', [owners.length, String(generation), reason, detail]); ++observed.closed; }
            });
            owners.push(sender);
            check(sender.prepareGateway() === 0, 'sender prepare');
            check(sender.connectServiceGateway(1, { address: '127.0.0.1', port: Number(ready[1]), platform: 3,
                device: crypto.randomFillSync(new Uint8Array(16)), maxRequests: 8, maxChunk: 65536, timeoutMs: 5000 }) === 0, 'sender connect');
            await drive(sender, () => observed.ready);
            check(await observed.reentry === -7, 'send rejected in protocol callback');
            const reserve = timeout => {
                const value = sender.beginRequest(1, 0x7345, timeout || 5000);
                check(value.result === 0 && value.requestId > 0n, 'request reserve');
                return value.requestId;
            };
            return { sender, observed, reserve };
        };
        const first = await connect();
        const body = Uint8Array.from({ length: 65537 }, (_, i) => (i * 31 + 17) & 255);
        const expected = body.slice();
        const postId = first.reserve();
        const writes = gate.writes;
        check(first.sender.postRequest(postId, body) === 0 && gate.writes === writes, 'post only copies to the local queue');
        body.fill(0xee);
        await delay(20);
        check(gate.writes === writes && !first.observed.results.has(postId), 'single-thread post needs a subsequent poll');
        await drive(first.sender, () => first.observed.results.has(postId));
        assert.deepEqual(first.observed.results.get(postId).bytes, expected);
        const held = first.reserve();
        gate.hold = true;
        let resolved = false;
        const sending = first.sender.sendRequest(held, expected).then(result => { resolved = true; return result; });
        check(sending instanceof Promise, 'JS send yields its event loop');
        await delay(20);
        check(!resolved, 'send must not report success while raw writes are rejected');
        const second = first.reserve();
        const otherSending = first.sender.sendRequest(second, expected);
        const postedAlongside = first.reserve();
        check(first.sender.postRequest(postedAlongside, expected) === 0, 'post remains usable beside concurrent sends');
        const beforeSend = gate.writes;
        gate.hold = false;
        check(await sending === 0 && await otherSending === 0 && gate.writes > beforeSend, 'concurrent sends complete only after actual network write calls');
        await drive(first.sender, () => [held, second, postedAlongside].every(id => first.observed.results.has(id)));
        for (const id of [held, second, postedAlongside]) assert.deepEqual(first.observed.results.get(id).bytes, expected);
        // A full local budget rejects without accepting a third body. Explicit
        // cancellation then produces one terminal for every reserved request.
        const large = new Uint8Array(1048560);
        const ids = [first.reserve(), first.reserve(), first.reserve()];
        check(first.sender.postRequest(ids[0], large) === 0 && first.sender.postRequest(ids[1], large) === 0, 'bounded post budget');
        check(first.sender.postRequest(ids[2], new Uint8Array(33)) === -3, 'post rejects bytes beyond the budget');
        ids.forEach(id => check(first.sender.cancelRequest(id) === 0, 'cancel accepted reserve'));
        await drive(first.sender, () => ids.every(id => first.observed.results.has(id)));
        ids.forEach(id => check(first.observed.results.get(id).status === 4, 'one cancellation terminal'));
        first.sender.closeGateway(0);
        await drive(first.sender, () => !first.sender.pollGateway());
        first.sender.releaseGateway();
        check(first.observed.closed === 1, 'first client closed once');
        const ordered = await connect();
        const orderIds = [ordered.reserve(), ordered.reserve(), ordered.reserve()];
        const actualOrder = [2, 0, 1];
        for (const index of actualOrder) {
            check(ordered.sender.postRequest(orderIds[index], new Uint8Array([index + 11])) === 0,
                'reverse reservation submission accepted');
        }
        await drive(ordered.sender, () => ordered.observed.results.size === 3);
        const arrived = [...ordered.observed.results.keys()];
        check(actualOrder.every((index, position) => arrived[position] === orderIds[index]),
            'actual RPC submission order is independent of reservation numbers');
        for (let index = 0; index < 3; ++index) {
            const reply = ordered.observed.results.get(orderIds[index]);
            check(reply.status === 0 && reply.bytes.length === 1 && reply.bytes[0] === index + 11,
                'wire numbering preserves the original public request association');
        }
        const mixedIds = [ordered.reserve(), ordered.reserve(), ordered.reserve()];
        check(ordered.sender.postRequest(mixedIds[0], expected) === 0, 'ordered post before send');
        const orderedSend = ordered.sender.sendRequest(mixedIds[1], expected);
        check(ordered.sender.postRequest(mixedIds[2], expected) === 0, 'ordered post after send invocation');
        check(await orderedSend === 0, 'ordered send reaches raw handoff');
        await drive(ordered.sender, () => ordered.observed.results.size === 6);
        const mixedArrived = [...ordered.observed.results.keys()].slice(3);
        check(mixedIds.every((id, position) => mixedArrived[position] === id), 'post/send/post retains same-executor order');
        ordered.sender.closeGateway(0);
        await drive(ordered.sender, () => !ordered.sender.pollGateway());
        ordered.sender.releaseGateway();
        const expired = await connect();
        gate.hold = true;
        const timeoutId = expired.reserve(30);
        check(await expired.sender.sendRequest(timeoutId, new Uint8Array([7])) === -9, 'sync delivery respects the original deadline');
        await drive(expired.sender, () => expired.observed.closed === 1);
        check(expired.observed.results.size === 1, 'deadline settles exactly once');
        expired.sender.releaseGateway();
        gate.hold = false;
        const queuedExpiry = await connect();
        gate.hold = true;
        const gateRequest = queuedExpiry.reserve(15000);
        let gateRequestDone = false;
        const gateSend = queuedExpiry.sender.sendRequest(gateRequest, expected).then(result => { gateRequestDone = true; return result; });
        await delay(20);
        check(!gateRequestDone, 'earlier synchronous request retains its raw handoff barrier');
        const expiryWrites = gate.writes;
        // This ACK is deliberately not a valid file-session response. It must
        // expire before staging and never reach the authenticated service.
        const lateFrame = queuedExpiry.sender.sendData({ kind: 10, product: 1, type: 60005,
            requestId: 0x7fffn, bytes: new Uint8Array([7]) });
        check(await lateFrame === -9, 'unstaged sendData reaches its own five-second deadline');
        check(!gateRequestDone && queuedExpiry.sender.gatewayReady() && queuedExpiry.observed.closed === 0,
            'expired unstaged data does not close an otherwise healthy connection');
        check(gate.writes === expiryWrites, 'expired data was not accepted by the raw host');
        gate.hold = false;
        check(await gateSend === 0, 'earlier live request still hands off normally');
        await drive(queuedExpiry.sender, () => queuedExpiry.observed.results.has(gateRequest));
        assert.deepEqual(queuedExpiry.observed.results.get(gateRequest).bytes, expected);
        const followup = queuedExpiry.reserve();
        check(await queuedExpiry.sender.sendRequest(followup, expected) === 0, 'new request remains usable after stream expiry');
        await drive(queuedExpiry.sender, () => queuedExpiry.observed.results.has(followup));
        check(queuedExpiry.observed.results.get(followup).status === 0 && queuedExpiry.observed.closed === 0,
            'no delayed invalid stream frame leaked into the live service');
        queuedExpiry.sender.closeGateway(0);
        await drive(queuedExpiry.sender, () => !queuedExpiry.sender.pollGateway());
        queuedExpiry.sender.releaseGateway();
        const closing = await connect();
        gate.hold = true;
        const closeId = closing.reserve();
        const closeSend = closing.sender.sendRequest(closeId, new Uint8Array([8]));
        closing.sender.closeGateway(0);
        check(await closeSend === -10, 'closing wakes the pending send with cancellation');
        await drive(closing.sender, () => closing.observed.closed === 1);
        closing.sender.releaseGateway();
        gate.hold = false;
        check(runtime.clients === 0 && runtime.hosts.size === 0, 'all sender and raw-host resources released');
        const record = { status: 'PASS', bits: factory.sdk.bits, checks, rawWriteCalls: gate.writes, rawBytes: gate.bytes,
            requestBytes: 65537, coverage: ['post-copy-deferred', 'send-raw-handoff', 'send-backpressure', 'concurrent-post',
                'bounded-queue', 'cancel-before-progress', 'callback-rejection', 'original-deadline', 'close-wakes-send', 'unique-terminals', 'unstaged-stream-expiry-no-late-send', 'submission-order-vs-reservation', 'post-send-post-order'],
            scope: 'Generated NetPeer SDK over Node TCP and real owned gateways; not a WeChat device result' };
        finalRecord = record;
    } catch (error) {
        scenarioError = error;
        fs.writeFileSync(reportPath, JSON.stringify({ status: 'FAIL', bits: factory.sdk.bits, checks,
            error: String(error && error.stack || error), scope: 'Actual generated NetSender and owned TCP gateways' }, null, 2));
        throw error;
    } finally {
        gate.hold = false;
        for (const sender of owners) {
            sender.closeGateway(0);
            const until = Date.now() + 2000;
            while (sender.pollGateway() && Date.now() < until) await delay(1);
            sender.releaseGateway();
        }
        for (const name of ['service', 'node']) fs.writeFileSync(path.join(directory, name + '.stop'), 'stop');
        await Promise.race([Promise.all(children.map(item => item.completed)), delay(3000)]);
        for (const item of children) if (item.child.exitCode === null) item.child.kill();
        const cleanup = await Promise.race([Promise.all(children.map(item => item.completed)), delay(3000).then(() => null)]);
        // Preserve the primary assertion when a rejected frame also causes a service fixture to exit.
        if (!scenarioError) check(cleanup !== null && cleanup.every(item => item.code === 0), 'owned diagnostic children stopped normally');
    }
    if (restricted) {
        check(restricted.probe.dynamicAttempts === 0, 'real delivery never dynamically compiles Wasm');
        finalRecord.fileOnlyHost = { fileLoads: restricted.probe.fileLoads, dynamicAttempts: restricted.probe.dynamicAttempts };
    }
    finalRecord.checks = checks;
    fs.writeFileSync(reportPath, JSON.stringify(finalRecord, null, 2));
    console.log('NetPeer send/post real TLS: ' + checks + ' checks PASS (' + factory.sdk.bits + ' bit)');
}

async function main() {
    let modulePath = '';
    let reportPath = '';
    let accountPath = '';
    let serverPath = '';
    const testArgs = [];
    for (const argument of process.argv.slice(2)) {
        if (argument.startsWith('--module=')) {
            modulePath = argument.slice(9);
        } else if (argument.startsWith('--account-module=')) {
            accountPath = argument.slice(17);
        } else if (argument.startsWith('--server=')) {
            serverPath = argument.slice(9);
        } else if (argument.startsWith('--report=')) {
            reportPath = argument.slice(9);
        } else {
            testArgs.push(argument);
        }
    }
    if (testArgs.length === 1 && ['--sender-delivery', '--sender-delivery-file-host'].includes(testArgs[0])) {
        if (![modulePath, serverPath, reportPath].every(path.isAbsolute) || accountPath) throw new Error('Sender delivery requires absolute module/server/report');
        return senderDelivery(modulePath, serverPath, reportPath, testArgs[0] === '--sender-delivery-file-host');
    }
    if (serverPath) throw new Error('--server is only used by --sender-delivery');
    if (testArgs.length === 1 && testArgs[0] === '--peer-file-host') {
        if (!path.isAbsolute(modulePath) || !path.isAbsolute(reportPath) || accountPath) throw new Error('File-only host requires absolute module/report');
        return peerFileHost(modulePath, reportPath);
    }
    if (testArgs.length === 1 && testArgs[0] === '--peer-contract') {
        if (!path.isAbsolute(modulePath) || !path.isAbsolute(reportPath) || accountPath) throw new Error('Peer contract requires absolute module/report');
        return peerContract(modulePath, reportPath);
    }
    if (testArgs.length === 1 && ['--account-bridge', '--account-bridge-file-host'].includes(testArgs[0])) {
        if (!path.isAbsolute(modulePath) || !path.isAbsolute(accountPath) || !path.isAbsolute(reportPath)) throw new Error('Account bridge requires absolute module/account-module/report');
        return accountBridge(modulePath, accountPath, reportPath, testArgs[0] === '--account-bridge-file-host');
    }
    if (accountPath) {
        if (!path.isAbsolute(modulePath) || !path.isAbsolute(accountPath) || testArgs.length || reportPath)
            throw new Error('Account self-test requires --module=<absolute NetInterface JS> --account-module=<absolute AccountProbe JS>');
        const { readWasm, loadAccountForTest } = require('./host/read_wasm');
        const { setModuleBits } = require('./host/WasmMemory');
        const factory = require(modulePath);
        const network = setModuleBits(await factory({ wasmBinary: readWasm(modulePath), noInitialRun: true }), factory);
        if (typeof network._accountClientCreate !== 'undefined') throw new Error('Account was linked into NetInterface');
        const { module } = await loadAccountForTest({ module: network }, accountPath);
        if (typeof module._accountSdkSelfTest !== 'function') throw new Error('Enable ACCOUNT_BUILD_SELF_TEST in the Account CMake entry');
        if (typeof module._netInterfaceLinkCreate !== 'undefined') throw new Error('Network connection implementation linked into Account');
        const result = module._accountSdkSelfTest();
        console.log('Account C++ Wasm self test: ' + result);
        if (result !== 0) throw new Error('Account.cpp check failed: ' + result);
        return;
    }
    if (!path.isAbsolute(modulePath) || !path.isAbsolute(reportPath) || testArgs.length === 0) {
        throw new Error('Required: --module=<absolute js> --report=<absolute csv> and a test selector');
    }
    const createModule = require(modulePath);
    const instance = await createModule({
        noInitialRun: true,
        print: line => process.stdout.write(`${line}\n`),
        printErr: line => process.stderr.write(`${line}\n`)
    });
    let exitCode = 1;
    try {
        exitCode = instance.callMain(testArgs.concat('--report=/NetWorkTest.csv'));
    } catch (error) {
        if (typeof error.status !== 'number') {
            throw error;
        }
        exitCode = error.status;
    }
    const report = instance.FS.readFile('/NetWorkTest.csv');
    fs.mkdirSync(path.dirname(reportPath), { recursive: true });
    fs.writeFileSync(reportPath, report);
    if (!Number.isInteger(exitCode)) {
        throw new Error('WebAssembly runner did not return an exit status');
    }
    process.exitCode = exitCode;
}

main().catch(error => {
    console.error(error && error.stack ? error.stack : String(error));
    process.exitCode = 3;
});