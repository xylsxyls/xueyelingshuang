'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { NetWorkRuntime, NetSender } = require('../../NetWork/NetWork/NetSender');
const { start } = require('../scripts/local_link_gateway_process_test');
const { TcpSocketHost } = require('../scripts/service_gateway_process_test');
const { createServiceClient } = require('../../CookWx/network/NetWxClient');
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));

async function applicationBridge(runtime, port) {
    const callbacks = { ready: 0, closed: 0 };
    const client = createServiceClient({ protocol: 3, runtime, platform: 3, serviceId: 1,
        address: '127.0.0.1', port, deviceId: new Uint8Array(16).fill(0x73),
        onReady() { ++callbacks.ready; }, onClose() { ++callbacks.closed; } });
    try {
        assert.equal(await client.connect(), client);
        assert.equal(callbacks.ready, 1);
        const pin = client.connection();
        assert.ok(pin.ready);
        const payload = Uint8Array.from({ length: 65537 }, (_, index) => (index * 17 + 3) & 255);
        let requested;
        const response = client.requestWithBuilder(1, 0x7345, id => { requested = id; return payload; }, 5000);
        assert.deepEqual(response.requestId, requested);
        assert.deepEqual(await response, payload);
        response.cancel();
        const sentinel = new Error('Application builder failed before any payload was sent');
        const failed = client.requestWithBuilder(1, 0x7345, () => { throw sentinel; }, 5000);
        await assert.rejects(failed, error => error === sentinel);
        assert.ok(client.ready);
        assert.equal(client.callbacks.size, 0);
        const interrupted = client.request(1, 0x7345, payload, 5000);
        const settled = assert.rejects(interrupted, error => error.code < 0);
        for (const state of runtime.host.instances.values()) if (state.socket) state.socket.close();
        await settled;
        const recoveryDeadline = Date.now() + 10000;
        while ((!client.ready || callbacks.ready !== 2) && Date.now() < recoveryDeadline) await delay(5);
        assert.equal(callbacks.ready, 2);
        assert.equal(callbacks.closed, 1);
        assert.equal(pin.ready, false);
        assert.notEqual(client.connection().generation, pin.generation);
        assert.throws(() => pin.sendFrame([9, 1, 1, 0, 1], new Uint8Array()), /generation has ended/);
        pin.fail(new Error('Late file callback'));
        assert.ok(client.ready);
        assert.deepEqual(await client.request(1, 0x7345, payload, 5000), payload);
        client.dispose();
        assert.equal(pin.ready, false);
        assert.throws(() => pin.sendFrame([9, 1, 1, 0, 1], new Uint8Array()), /generation has ended/);
        pin.fail(new Error('Late file completion must not affect a future generation'));
        const deadline = Date.now() + 2000;
        while (!client.released && Date.now() < deadline) await delay(5);
        assert.equal(client.released, true);
        assert.equal(callbacks.closed, 2);
        return { authenticatedRoundTrips: 2, builderCancellation: true, nativeRecovery: true, staleFileGenerationRejected: true };
    } finally {
        client.dispose();
        const deadline = Date.now() + 2000;
        while (!client.released && Date.now() < deadline) await delay(5);
        assert.equal(client.released, true);
    }
}

async function exchange(directory, bits, port) {
    const runtime = await NetWorkRuntime.load(require(path.resolve(directory, 'NetWork.js')), {
        moduleOptions: { wasmBinary: fs.readFileSync(path.join(directory, 'NetWork.wasm')) },
        createSocket: () => new TcpSocketHost(), randomSource: size => crypto.randomBytes(size) });
    const clients = [];
    const observations = [];
    try {
        for (let platform = 1; platform <= 3; ++platform) {
            const state = { platform, ready: false, request: 0n, sent: false, received: false, closed: 0, error: null,
                payload: Uint8Array.from({ length: 65537 }, (_, index) => (index * 31 + platform) & 255) };
            const client = new NetSender(runtime, {
                onSessionReady(generation, requests, chunk) {
                    assert.equal(typeof generation, 'bigint');
                    assert.ok(requests > 0 && chunk >= 1024);
                    const pending = client.beginRequest(1, 0x7345, 5000);
                    assert.equal(pending.result, 0);
                    assert.ok(pending.requestId > 0n);
                    state.request = pending.requestId;
                    state.ready = true;
                },
                onSessionResult(generation, requestId, product, type, status, bytes) {
                    assert.equal(requestId, state.request);
                    assert.equal(product, 1);
                    assert.equal(type, 0x7345);
                    assert.equal(status, 0);
                    assert.deepEqual(bytes, state.payload);
                    assert.equal(state.received, false);
                    state.received = true;
                },
                onSessionFrame() { throw new Error('Unexpected application push'); },
                onSessionClosed() { ++state.closed; }
            });
            clients.push(client);
            observations.push(state);
            assert.equal(client.prepareGateway(), 0);
            assert.equal(client.configureGateways('127.0.0.1:' + port, ''), 0);
            assert.equal(client.connectServiceGateway(1, { address: '127.0.0.1', port, platform,
                device: new Uint8Array(16).fill(0x40 + platform), maxChunk: 65536, timeoutMs: 5000 }), 0);
        }
        const deadline = Date.now() + 12000;
        while (observations.some(state => !state.received) && Date.now() < deadline) {
            for (let index = 0; index < clients.length; ++index) {
                const client = clients[index];
                const state = observations[index];
                assert.ok(client.pollGateway() >= 0);
                if (client.lastError) throw client.lastError;
                if (state.ready && !state.sent) {
                    const result = client.postRequest(state.request, state.payload);
                    assert.ok(result === 0 || result === -3);
                    state.sent = result === 0;
                }
            }
            await delay(1);
        }
        assert.ok(observations.every(state => state.ready && state.sent && state.received),
            JSON.stringify(observations.map(state => ({ platform: state.platform, ready: state.ready, sent: state.sent,
                received: state.received, closed: state.closed }))));
        for (const client of clients) assert.equal(client.closeGateway(0), 0);
        for (let step = 0; step < 20; ++step) {
            for (const client of clients) client.pollGateway();
            await delay(1);
        }
        for (const client of clients) assert.equal(client.releaseGateway(), 0);
        assert.equal(runtime.host.instances.size, 0);
        assert.ok(observations.every(state => state.closed === 1));
        const application = await applicationBridge(runtime, port);
        assert.equal(runtime.host.instances.size, 0);
        return { bits, platforms: [1, 2, 3], payloadBytesEachDirection: 65537, clients: clients.length,
            authenticatedRoundTrips: observations.filter(state => state.received).length, application, status: 'PASS' };
    } finally {
        for (const client of clients) {
            client.closeGateway(0);
            for (let step = 0; step < 10 && client.releaseGateway() !== 0; ++step) { client.pollGateway(); await delay(1); }
        }
        runtime.dispose();
    }
}

async function main() {
    const [serverPath, wasm32, wasm64, destination] = process.argv.slice(2);
    assert.ok(serverPath && wasm32 && wasm64 && destination);
    const output = path.resolve(destination);
    assert.equal(fs.existsSync(output), false, 'Use a new owned directory');
    fs.mkdirSync(output, { recursive: true });
    const children = [];
    let timer;
    try {
        await Promise.race([(async () => {
            const gateway = start(path.resolve(serverPath), ['--protocol=3', '--listen=127.0.0.1', '--port=0',
                '--identity-directory=' + path.join(output, 'identity'), '--stop-file=' + path.join(output, 'gateway.stop'),
                '--run-for-ms=40000'], 'gateway', output, /GATEWAY_READY port=(\d+) pid=(\d+)/);
            children.push(gateway);
            const ready = await gateway.ready;
            const service = start(path.resolve(serverPath), ['--self-test-service=' + ready[2],
                '--stop-file=' + path.join(output, 'service.stop'), '--run-for-ms=40000'], 'service', output, /SERVICE_PROBE_READY pid=(\d+)/);
            children.push(service);
            await service.ready;
            const results = [];
            results.push(await exchange(wasm32, 32, Number(ready[1])));
            results.push(await exchange(wasm64, 64, Number(ready[1])));
            for (const name of ['service', 'gateway']) fs.writeFileSync(path.join(output, name + '.stop'), '');
            const stopped = await Promise.all(children.map(child => child.completed));
            assert.ok(stopped.every(result => result.code === 0 && !result.signal));
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify(results, null, 2));
            for (const result of results) console.log(JSON.stringify(result));
        })(), new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Whole NetWork process deadline')), 35000); })]);
    } finally {
        clearTimeout(timer);
        for (const child of children) if (child.child.exitCode === null && child.child.signalCode === null) child.child.kill();
        await Promise.all(children.map(child => child.completed.catch(() => {})));
    }
}

main().catch(error => { console.error(error.stack); process.exitCode = 1; });