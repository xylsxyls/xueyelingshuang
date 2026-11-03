'use strict';
// Real TCP/native gateways/Wasm with a deterministic 50 ms delivery delay in each direction.
// This is an injected RTT test, not a public-Internet bandwidth benchmark.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');
const { performance } = require('node:perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { createServiceClient } = require('./host/NetWxClient');

class DelayedSocket extends TcpSocketHost {
    constructor() { super(); this.timers = new Set(); this.queued = 0; }
    later(bytes, callback) {
        this.queued += bytes;
        if (this.queued > 4 * 1048576) throw new Error('RTT host queue exceeded');
        const timer = setTimeout(() => { this.timers.delete(timer); this.queued -= bytes; callback(); }, 50);
        this.timers.add(timer);
    }
    onMessage(callback) { super.onMessage(event => this.later(event.message.byteLength, () => callback(event))); }
    write(bytes) { const copy = bytes.slice(0); this.later(copy.byteLength, () => super.write(copy)); }
    connect(options) { super.connect(options); this.socket.setNoDelay(true); }
    close() { for (const timer of this.timers) clearTimeout(timer); this.timers.clear(); this.queued = 0; super.close(); }
}

async function main() {
    const args = Object.fromEntries(process.argv.slice(2).map(value => {
        const at = value.indexOf('='); assert.ok(at > 0); return [value.slice(0, at), value.slice(at + 1)];
    }));
    for (const key of ['--server', '--module', '--output']) assert.ok(args[key], key);
    const output = path.resolve(args['--output']);
    assert.ok(!fs.existsSync(output)); fs.mkdirSync(output, { recursive: true });
    const children = [], clients = [];
    let timer;
    const launch = (name, parameters, ready) => {
        const child = start(path.resolve(args['--server']), parameters.concat([
            '--stop-file=' + path.join(output, name + '.stop'), '--run-for-ms=55000']), name, output, ready);
        children.push(child); return child;
    };
    try {
        await Promise.race([(async () => {
            const gateway = launch('gateway', ['--listen=127.0.0.1', '--port=0', '--identity-directory=' + path.join(output, 'identity')],
                /GATEWAY_READY port=(\d+) pid=(\d+)/);
            const ready = await gateway.ready;
            const service = launch('service', ['--self-test-service=' + ready[2]], /SERVICE_PROBE_READY/);
            await service.ready;
            const entry = path.resolve(args['--module']);
            const runtime = await NetInterfaceRuntime.load(require(entry), {
                randomSource: async size => Uint8Array.from(crypto.randomBytes(size)),
                moduleOptions: { wasmBinary: require('./host/read_wasm').readWasm(entry) }
            });
            const payload = Uint8Array.from(crypto.randomBytes(786432));
            const measurements = [];
            for (const windowBytes of [65536, 1048576]) {
                const client = createServiceClient({ runtime, protocol: 3, autoReconnect: false, serviceId: 1, windowBytes,
                    address: '127.0.0.1', port: Number(ready[1]), platform: 2, deviceId: Uint8Array.from(crypto.randomBytes(16)),
                    createSocket: () => new DelayedSocket(), clock: () => performance.now() });
                clients.push(client); await client.connect();
                const began = performance.now();
                for (let round = 0; round < 4; ++round)
                    assert.deepEqual(Buffer.from(await client.request(1, 0x7345, payload, 10000)), Buffer.from(payload));
                const elapsedMs = Math.round(performance.now() - began);
                measurements.push({ windowBytes, elapsedMs, uploadBytes: payload.length * 4, downloadBytes: payload.length * 4 });
                client.dispose();
            }
            assert.ok(measurements[1].elapsedMs < measurements[0].elapsedMs * 0.7, 'Negotiated window did not remove the injected RTT bottleneck');
            fs.writeFileSync(path.join(output, 'service.stop'), ''); assert.equal((await service.completed).code, 0);
            fs.writeFileSync(path.join(output, 'gateway.stop'), ''); assert.equal((await gateway.completed).code, 0);
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, injectedRttMs: 100, measurements }, null, 2));
            console.log(JSON.stringify(measurements));
        })(), new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('RTT test deadline')), 50000); })]);
    } finally {
        clearTimeout(timer); clients.forEach(client => client.dispose());
        for (const child of children) if (child.child.exitCode === null && child.child.signalCode === null) child.child.kill();
        await Promise.all(children.map(child => child.completed.catch(() => {})));
    }
}
main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });