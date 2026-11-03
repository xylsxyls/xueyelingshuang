'use strict';

// This is an actual loopback TCP + independent OpenSSL interoperability test.
// The socket adapter emulates the WeChat callback shape; it is not a claim that
// WXWebAssembly or wx.TCPSocket has run in WeChat.
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const fs = require('node:fs');
const net = require('node:net');
const path = require('node:path');
const tls = require('node:tls');
const { performance } = require('node:perf_hooks');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { NetWxClient, createWxClient } = require('./host/NetWxClient');
const { NetInterfaceMessage } = require('./host/NetInterfaceMessage');
const { attachApplicationEcho } = require('./application_echo');

class RealTcpAdapter {
    constructor() { this.listeners = {}; this.socket = null; }
    onConnect(callback) { this.listeners.connect = callback; }
    onMessage(callback) { this.listeners.message = callback; }
    onError(callback) { this.listeners.error = callback; }
    onClose(callback) { this.listeners.close = callback; }
    offConnect() { delete this.listeners.connect; }
    offMessage() { delete this.listeners.message; }
    offError() { delete this.listeners.error; }
    offClose() { delete this.listeners.close; }
    connect(options) {
        this.socket = net.createConnection({ host: options.address, port: options.port });
        this.socket.on('connect', () => { if (this.listeners.connect) this.listeners.connect(); });
        this.socket.on('data', bytes => {
            if (this.listeners.message) this.listeners.message({ message: Uint8Array.from(bytes).buffer });
        });
        this.socket.on('error', error => {
            if (this.listeners.error) this.listeners.error({ errMsg: error.message });
        });
        this.socket.on('close', () => { if (this.listeners.close) this.listeners.close(); });
    }
    write(bytes) { this.socket.write(Buffer.from(bytes)); }
    close() { if (this.socket) this.socket.destroy(); }
}

async function main() {
    const args = {};
    for (const argument of process.argv.slice(2)) {
        const separator = argument.indexOf('=');
        const key = separator < 0 ? argument : argument.slice(0, separator);
        if (!['--module', '--report', '--cases', '--instant'].includes(key) || Object.hasOwn(args, key) ||
            (key === '--instant' ? separator >= 0 : separator < 0 || !argument.slice(separator + 1))) {
            throw new Error('Unknown, repeated or malformed argument: ' + argument);
        }
        args[key] = separator < 0 ? true : argument.slice(separator + 1);
    }
    if (Boolean(args['--instant']) === Boolean(args['--cases']) ||
        (args['--cases'] && !/^[1-9][0-9]*(,[1-9][0-9]*)*$/.test(args['--cases']))) {
        throw new Error('Select exactly one of --instant or --cases=201,202');
    }
    const selectedIds = args['--cases'] ? args['--cases'].split(',').map(Number) : null;
    if (selectedIds && (new Set(selectedIds).size !== selectedIds.length ||
        selectedIds.some(id => id < 201 || id > 217))) throw new Error('Unknown or duplicate case ID');
    if (!path.isAbsolute(args['--module'] || '') || !path.isAbsolute(args['--report'] || '')) {
        throw new Error('Required: --module=<absolute generated JS> --report=<absolute JSON>');
    }
    const root = path.resolve(__dirname, '../../..');
    // Mbed TLS 3.6.7 framework test certificates; independent of SDK build caches.
    const certificates = path.join(root, 'src/NetWorkTest/testdata/tls');
    const factory = require(args['--module']);
    const runtime = await NetInterfaceRuntime.load(factory, {
        moduleOptions: { wasmBinary: require('./host/read_wasm').readWasm(args['--module']) },
        randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size)))
    });
    const authorityPem = fs.readFileSync(path.join(certificates, 'test-ca-sha256.crt'), 'ascii');
    const peers = new Set();
    const ciphers = new Set();
    let nextServerMode = 'normal';
    const applicationHellos = [];
    let pongCount = 0;
    const server = tls.createServer({
        cert: fs.readFileSync(path.join(certificates, 'server2-sha256.crt')),
        key: fs.readFileSync(path.join(certificates, 'server2.key')),
        minVersion: 'TLSv1.2', maxVersion: 'TLSv1.2'
    }, socket => {
        peers.add(socket);
        ciphers.add(socket.getCipher().standardName);
        socket.on('close', () => peers.delete(socket));
        socket.on('error', () => {});
        attachApplicationEcho(socket, { mode: nextServerMode, onHello: hello => applicationHellos.push(hello),
            onPong: () => { ++pongCount; } });
        nextServerMode = 'normal';
    });
    server.on('tlsClientError', () => {});
    await new Promise((resolve, reject) => {
        server.once('error', reject);
        server.listen(0, '127.0.0.1', resolve);
    });
    const report = { node: process.version, openssl: process.versions.openssl,
        engine: 'WebAssembly in Node, real TCP, independent OpenSSL server', cases: [] };
    const clients = new Set();
    const options = {
        runtime, authorityPem, address: '127.0.0.1', hostname: 'localhost',
        deviceId: Uint8Array.from(crypto.randomBytes(16)),
        port: server.address().port, clock: () => performance.now(),
        createSocket: () => new RealTcpAdapter()
    };
    function client(overrides) {
        const connection = new NetWxClient(Object.assign({}, options, overrides));
        clients.add(connection);
        return connection;
    }
    const cases = [
        [201, 'OpenSSL TCP binary frames and uint64 identity', async () => {
            let receive;
            const connection = client({ onFrame: frame => { if (receive) receive(frame); } });
            await connection.connect();
            assert.deepEqual(Object.assign({}, connection.negotiated, { peerNonce: null }),
                { platform: 3, capabilities: 3, maxRequests: 16, maxChunk: 4096, peerNonce: null });
            assert(applicationHellos.some(hello => hello.device === Buffer.from(options.deviceId).toString('hex')));
            for (const length of [0, 1, 16352, 16353, 65536, 1048576]) {
                const payload = Uint8Array.from(crypto.randomBytes(length));
                const response = new Promise((resolve, reject) => {
                    const timeout = setTimeout(() => reject(new Error('echo timeout')), 5000);
                    receive = frame => { clearTimeout(timeout); resolve(frame); };
                });
                connection.sendFrame([3, 5, 0x76543210, 0xfedcba98, 42], payload);
                const frame = await response;
                assert.equal(frame.kind, 3);
                assert.equal(frame.product, 5);
                assert.equal(frame.requestLow, 0x76543210);
                assert.equal(frame.requestHigh, 0xfedcba98);
                assert.equal(frame.type, 42);
                assert.deepEqual(frame.payload, payload);
            }
            assert(ciphers.size > 0);
            for (const cipher of ciphers) assert.match(cipher, /^TLS_ECDHE_(RSA|ECDSA)_WITH_(CHACHA20_POLY1305|AES_\d+_GCM)/);
            connection.dispose();
        }],
        [202, 'OpenSSL wrong hostname rejected', async () => {
            const connection = client({ hostname: 'wrong.invalid' });
            await assert.rejects(connection.connect(), error => error.code === -103 && error.detail !== 0);
            assert.equal(connection.ready, false);
        }],
        [203, 'OpenSSL wrong authority rejected', async () => {
            const connection = client({ authorityPem: fs.readFileSync(path.join(certificates, 'test-ca2.crt'), 'ascii') });
            await assert.rejects(connection.connect(), error => error.code === -103);
            assert.equal(connection.ready, false);
        }],
        [204, 'Disposed connection ignores late TCP callbacks', async () => {
            let socket;
            let frames = 0;
            const connection = client({ createSocket: () => { socket = new RealTcpAdapter(); return socket; },
                onFrame: () => { ++frames; } });
            await connection.connect();
            const late = Object.assign({}, socket.listeners);
            connection.dispose();
            late.connect();
            late.message({ message: new ArrayBuffer(100) });
            late.error({ errMsg: 'late error' });
            late.close();
            assert.equal(frames, 0);
            assert.equal(connection.incomingBytes, 0);
            assert.equal(connection.session, null);
        }],
        [205, 'Host entropy failure rejects module creation', async () => {
            await assert.rejects(NetInterfaceRuntime.load(factory, {
                moduleOptions: { wasmBinary: require('./host/read_wasm').readWasm(args['--module']) },
                randomSource: () => Promise.reject(new Error('entropy unavailable'))
            }), /entropy unavailable/);
        }],
        [206, 'Shared protobuf binary wire in production Wasm', async () => {
            const message = new NetInterfaceMessage(runtime);
            try {
                message.setCode(7, Uint8Array.of(65, 0, 90));
                message.setValue(0, 1, 1, Uint8Array.of(0xef, 0xbe, 0xad, 0xde));
                const expected = Uint8Array.of(10, 7, 8, 7, 18, 3, 65, 0, 90,
                    18, 16, 18, 14, 10, 12, 8, 1, 18, 8, 10, 4, 239, 190, 173, 222, 16, 1);
                assert.deepEqual(message.encode(), expected);
                message.clear();
                message.parse(expected);
                assert.deepEqual(message.getCode(), { code: 7, bytes: Uint8Array.of(65, 0, 90) });
                assert.deepEqual(message.getValue(0, 1), { type: 1, bytes: Uint8Array.of(239, 190, 173, 222) });
                assert.equal(message.getValue(0, 2), null);
                assert.throws(() => message.parse(Uint8Array.of(10, 7, 8)), error => error.code === -2);
                assert.equal(message.getValue(0, 1), null);
            } finally {
                message.dispose();
            }
            assert.throws(() => message.encode(), /已关闭/);
            message.dispose();
        }],
        [207, 'Application reply bound to this connection nonce', async () => {
            nextServerMode = 'wrong-nonce';
            const connection = client();
            await assert.rejects(connection.connect(), error => error.code === -2);
            assert.equal(connection.ready, false);
        }],
        [208, 'Business frame before application handshake rejected', async () => {
            nextServerMode = 'early-business';
            let received = 0;
            const connection = client({ onFrame: () => { ++received; } });
            await assert.rejects(connection.connect(), error => error.code === -2);
            assert.equal(received, 0);
        }],
        [209, 'TLS peer silent during application handshake times out', async () => {
            nextServerMode = 'silent';
            const connection = client();
            const start = performance.now();
            await assert.rejects(connection.connect(), error => error.code === -9);
            assert(performance.now() - start < 7000);
        }],
        [210, 'Concurrent adapter connections share one stored device identity', async () => {
            const storage = new Map();
            let writes = 0;
            const wxStorageAndSocketShape = {
                getPerformance: () => performance,
                getStorageSync: key => storage.get(key),
                setStorageSync: (key, value) => { ++writes; storage.set(key, value); },
                createTCPSocket: () => new RealTcpAdapter()
            };
            const first = createWxClient(runtime, options, wxStorageAndSocketShape);
            const second = createWxClient(runtime, options, wxStorageAndSocketShape);
            clients.add(first);
            clients.add(second);
            await Promise.all([first.connect(), second.connect()]);
            assert.equal(writes, 1);
            assert.deepEqual(first.deviceId, second.deviceId);
            assert.notDeepEqual(first.negotiated.peerNonce, second.negotiated.peerNonce);
            first.dispose();
            second.dispose();
        }],
        [211, 'Disposal inside application callback releases both Wasm handles', async () => {
            let delivered;
            const received = new Promise(resolve => { delivered = resolve; });
            const connection = client({ onFrame: () => { connection.dispose(); delivered(); } });
            await connection.connect();
            connection.sendFrame([3, 5, 123, 0, 42], Uint8Array.of(9));
            let timer;
            try {
                await Promise.race([received, new Promise((resolve, reject) => {
                    timer = setTimeout(() => reject(new Error('callback disposal timeout')), 5000);
                })]);
            } finally { clearTimeout(timer); }
            assert.equal(connection.session, null);
            assert.equal(connection.application, null);
            assert.equal(connection.callbackError, undefined);
        }],
        [212, 'C++ RPC envelope and multiplexed binary responses over TCP', async () => {
            nextServerMode = 'rpc';
            const connection = client();
            await connection.connect();
            const payloads = Array.from({ length: 16 }, (_, index) => Uint8Array.from(crypto.randomBytes(index * 1024)));
            const calls = payloads.map(payload => connection.request(4, 900, payload, 5000));
            const replies = await Promise.all(calls);
            assert.deepEqual(replies, payloads);
            assert.equal(connection.requests.count(), 0);
            assert.equal(connection.callbacks.size, 0);
            const large = Uint8Array.from(crypto.randomBytes(1048560));
            assert.deepEqual(await connection.request(4, 905, large, 5000), large);
            await new Promise(resolve => setTimeout(resolve, 100));
            assert.equal(connection.ready, true);
            assert.equal(connection.callbacks.size, 0);
            connection.dispose();
        }],
        [213, 'C++ request budget, early cancellation and late response cleanup', async () => {
            nextServerMode = 'rpc';
            const connection = client();
            await connection.connect();
            const calls = Array.from({ length: 16 }, () => connection.request(4, 901, Uint8Array.of(9), 1000));
            const all = Promise.allSettled(calls);
            assert.throws(() => connection.request(4, 901, Uint8Array.of(9), 1000), error => error.code === -3);
            calls.forEach(call => { call.cancel(); call.cancel(); });
            const results = await all;
            assert(results.every(result => result.status === 'rejected' && result.reason.code === -10));
            assert.equal(connection.requests.count(), 0);
            assert.equal(connection.callbacks.size, 0);
            await new Promise(resolve => setTimeout(resolve, 250));
            assert.equal(connection.ready, true);
            assert.deepEqual(await connection.request(4, 900, Uint8Array.of(7), 1000), Uint8Array.of(7));
            connection.dispose();
        }],
        [214, 'Monotonic C++ RPC deadline and late response rejection', async () => {
            nextServerMode = 'rpc';
            const connection = client();
            await connection.connect();
            await assert.rejects(connection.request(4, 901, Uint8Array.of(1), 25), error => error.code === -9);
            await new Promise(resolve => setTimeout(resolve, 200));
            assert.equal(connection.requests.count(), 0);
            assert.equal(connection.callbacks.size, 0);
            assert.equal(connection.ready, true);
            connection.dispose();
        }],
        [215, 'Transport failure status and mismatched operation response', async () => {
            nextServerMode = 'rpc';
            const connection = client();
            await connection.connect();
            await assert.rejects(connection.request(4, 902, Uint8Array.of(1), 1000), error => error.status === 1);
            assert.equal(connection.ready, true);
            await assert.rejects(connection.request(4, 903, Uint8Array.of(1), 1000), error => error.code === -2);
            assert.equal(connection.disposed, true);
            assert.equal(connection.requests, null);
            assert.equal(connection.callbacks.size, 0);
        }],
        [216, 'Disposal settles pending RPC and frees request handles', async () => {
            nextServerMode = 'rpc';
            const connection = client();
            await connection.connect();
            const call = connection.request(4, 901, new Uint8Array(0), 1000);
            const rejected = assert.rejects(call, /test shutdown/);
            connection.dispose(new Error('test shutdown'));
            await rejected;
            call.cancel();
            connection.dispose();
            assert.equal(connection.requests, null);
            assert.equal(connection.callbacks.size, 0);
        }],
        [217, 'Shared C++ heartbeat replies preserve full uint64 without business callbacks', async () => {
            nextServerMode = 'heartbeat';
            const before = pongCount;
            let business = 0;
            const connection = client({ onFrame: () => { ++business; } });
            await connection.connect();
            const deadline = performance.now() + 2000;
            while (pongCount === before && performance.now() < deadline) {
                await new Promise(resolve => setTimeout(resolve, 10));
            }
            assert.equal(pongCount, before + 1);
            assert.equal(business, 0);
            assert.equal(connection.ready, true);
            connection.dispose();
        }]
    ];
    let selected = cases;
    if (selectedIds) {
        selected = cases.filter(entry => selectedIds.includes(entry[0]));
    }
    fs.mkdirSync(path.dirname(args['--report']), { recursive: true });
    try {
        for (const [id, name, run] of selected) {
            const started = performance.now();
            const result = { id, name, state: 'PASS' };
            try { await run(); } catch (error) { result.state = 'FAIL'; result.error = error.stack; }
            result.durationMs = performance.now() - started;
            report.cases.push(result);
            fs.writeFileSync(args['--report'], JSON.stringify(report, null, 2));
            console.log(id, name, result.state, result.error || '');
        }
    } finally {
        clients.forEach(connection => connection.dispose());
        peers.forEach(socket => socket.destroy());
        await new Promise(resolve => server.close(resolve));
    }
    report.ciphers = Array.from(ciphers);
    report.passed = report.cases.filter(entry => entry.state === 'PASS').length;
    report.failed = report.cases.length - report.passed;
    fs.writeFileSync(args['--report'], JSON.stringify(report, null, 2));
    process.exitCode = report.failed ? 1 : 0;
}

module.exports = { RealTcpAdapter };
if (require.main === module) {
    main().catch(error => { console.error(error.stack); process.exitCode = 2; });
}