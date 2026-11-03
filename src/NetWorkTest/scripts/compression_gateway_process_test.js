'use strict';

const fs = require('fs');
const path = require('path');
const assert = require('assert').strict;
const crypto = require('crypto');
const { performance } = require('perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { createServiceClient } = require('./host/NetWxClient');

async function main() {
    const options = {};
    for (const value of process.argv.slice(2)) {
        const split = value.indexOf('=');
        const key = value.slice(0, split);
        assert.ok(split > 0 && ['--server', '--module', '--output'].includes(key) && !options[key]);
        options[key] = value.slice(split + 1);
    }
    for (const key of ['--server', '--module', '--output']) assert.ok(options[key], key);
    const output = path.resolve(options['--output']);
    assert.ok(!fs.existsSync(output));
    fs.mkdirSync(output, { recursive: true });
    const children = [];
    const run = (args, name, pattern) => {
        const child = start(path.resolve(options['--server']), args, name, output, pattern);
        children.push(child);
        return child;
    };
    let timer;
    let client;
    try {
        const deadline = new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Compression test deadline')), 20000); });
        await Promise.race([(async () => {
            const server = run(['--protocol=3', '--listen=127.0.0.1', '--port=0', '--identity-directory=' + path.join(output, 'identity'),
                '--stop-file=' + path.join(output, 'server.stop'), '--run-for-ms=25000'], 'server', /GATEWAY_READY port=(\d+) pid=(\d+)/);
            const ready = await server.ready;
            const service = run(['--self-test-service=' + ready[2], '--stop-file=' + path.join(output, 'service.stop'), '--run-for-ms=25000'],
                'service', /SERVICE_PROBE_READY pid=/);
            await service.ready;
            const modulePath = path.resolve(options['--module']);
            const runtime = await NetInterfaceRuntime.load(require(modulePath), { randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size))),
                moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) } });
            let tcp;
            client = createServiceClient({ runtime, protocol: 3, autoReconnect: false, serviceId: 1,
                address: '127.0.0.1', port: Number(ready[1]), platform: 1, deviceId: Uint8Array.from(crypto.randomBytes(16)),
                compressionTypes: [0x7345], createSocket: () => { tcp = new TcpSocketHost(); return tcp; }, clock: () => performance.now() });
            await client.connect();
            assert.ok((client.negotiated.capabilities & 8) !== 0);
            const sent = tcp.socket.bytesWritten;
            const received = tcp.socket.bytesRead;
            const payload = new Uint8Array(65537).fill(0x41);
            const reply = await client.request(1, 0x7345, payload, 8000);
            assert.ok(Buffer.from(reply).equals(Buffer.from(payload)), 'Business did not receive original bytes');
            const outgoing = tcp.socket.bytesWritten - sent;
            const incoming = tcp.socket.bytesRead - received;
            // Count actual TCP ciphertext, including control/window overhead. Raw payload cannot pass this bound.
            assert.ok(outgoing < 12000 && incoming < 12000, 'Negotiated public RPC was not compressed in both directions');
            client.dispose(); client = null;
            fs.writeFileSync(path.join(output, 'service.stop'), '');
            fs.writeFileSync(path.join(output, 'server.stop'), '');
            const stopped = await Promise.all([service.completed, server.completed]);
            assert.ok(stopped.every(result => result.code === 0 && !result.signal));
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, rawBytesEachDirection: payload.length,
                tcpCiphertextSent: outgoing, tcpCiphertextReceived: incoming, negotiated: true, originalBusinessBytes: true }, null, 2));
        })(), deadline]);
        console.log('Production Wasm + opaque gateway + native service compression in both directions: PASS');
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ error: String(error.stack || error) }, null, 2));
        throw error;
    } finally {
        clearTimeout(timer);
        if (client) client.dispose();
        for (const child of children) if (child.child.exitCode === null && child.child.signalCode === null) child.child.kill();
        await Promise.all(children.map(child => child.completed.catch(() => {})));
    }
}
main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });