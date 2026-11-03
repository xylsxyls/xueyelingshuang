'use strict';

// Authenticated service -> opaque gateway -> internal service, two fresh endpoint TLS generations.
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { performance } = require('perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { createWxClient } = require('./host/NetWxClient');

function check(value, message) { if (!value) throw new Error(message); }

async function main() {
    const options = {};
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        check(split > 0, 'Expected --name=value');
        const name = argument.slice(0, split);
        check(['--server', '--module', '--output'].includes(name) && !Object.hasOwn(options, name), 'Unknown/repeated option');
        options[name] = argument.slice(split + 1);
    }
    for (const key of ['--server', '--module', '--output']) check(options[key], 'Missing ' + key);
    const output = path.resolve(options['--output']);
    check(!fs.existsSync(output), 'Use a fresh owned output directory');
    fs.mkdirSync(output, { recursive: true });
    const children = [];
    const clients = [];
    let timer;
    try {
        const modulePath = path.resolve(options['--module']);
        const runtime = await NetInterfaceRuntime.load(require(modulePath), {
            randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size))),
            moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) }
        });
        const serverPath = path.resolve(options['--server']);
        const server = start(serverPath, ['--protocol=3', '--listen=127.0.0.1', '--port=0',
            '--identity-directory=' + path.join(output, 'identity'), '--stop-file=' + path.join(output, 'server.stop'), '--run-for-ms=20000'],
            'server', output, /GATEWAY_READY port=(\d+) pid=(\d+)/);
        children.push(server);
        const deadline = new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Internal service test deadline')), 18000); });
        await Promise.race([(async () => {
            const ready = await server.ready;
            const internal = start(serverPath, ['--self-test-internal=' + ready[2], '--stop-file=' + path.join(output, 'internal.stop'),
                '--run-for-ms=20000'], 'internal', output, /SERVICE_PROBE_READY pid=(\d+)/);
            children.push(internal);
            await internal.ready;
            const caller = start(serverPath, ['--self-test-query=' + ready[2], '--stop-file=' + path.join(output, 'caller.stop'),
                '--run-for-ms=20000'], 'caller', output, /SERVICE_PROBE_INTERNAL_PASS rounds=2/);
            children.push(caller);
            await caller.ready;
            const storage = new Map();
            const wxHost = { createTCPSocket: () => new TcpSocketHost(), getPerformance: () => performance,
                getStorageSync: key => storage.get(key), setStorageSync: (key, value) => storage.set(key, value) };
            const client = createWxClient(runtime, { protocol: 3, serviceId: 1, address: '127.0.0.1', port: Number(ready[1]) }, wxHost);
            clients.push(client);
            await client.connect();
            const body = new Uint8Array(65537);
            for (let index = 0; index < body.length; ++index) body[index] = (index * 23 + 17) & 255;
            const reply = await client.request(1, 0x7345, body, 5000);
            check(Buffer.from(reply).equals(Buffer.from(body)), 'Closing outgoing channels broke the inbound service');
            const denied = createWxClient(runtime, { protocol: 3, serviceId: 0, address: '127.0.0.1', port: Number(ready[1]) }, wxHost);
            clients.push(denied);
            let refusal;
            try { await denied.connect(); } catch (error) { refusal = error; }
            const notSupported = -11; // NetInterfaceResult::NI_NOT_SUPPORTED
            check(refusal && refusal.code === notSupported, 'Public internal-service access was not explicitly denied: ' + (refusal && refusal.message));
            for (const item of clients) item.dispose();
            fs.writeFileSync(path.join(output, 'caller.stop'), '');
            const callerStopped = await caller.completed;
            check(callerStopped.code === 0 && /SERVICE_PROBE_ECHO/.test(callerStopped.stdout), 'Caller failed or did not serve the public client');
            fs.writeFileSync(path.join(output, 'internal.stop'), '');
            const internalStopped = await internal.completed;
            check(internalStopped.code === 0 && (internalStopped.stdout.match(/SERVICE_PROBE_ECHO/g) || []).length === 2,
                'Internal service did not process both new TLS generations');
            fs.writeFileSync(path.join(output, 'server.stop'), '');
            check((await server.completed).code === 0, 'Gateway failed normal shutdown');
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, outgoingTlsGenerations: 2,
                outgoingPayloadSizes: [65537, 78644], inboundAfterOutgoingClose: true, publicInternalServiceDenied: true,
                host: 'Production Wasm and Node TCPSocket callback surface; not WeChat GUI' }, null, 2));
        })(), deadline]);
        console.log('Service registration + two internal endpoint TLS generations + public role boundary: PASS');
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ error: error.stack || String(error) }, null, 2));
        throw error;
    } finally {
        clearTimeout(timer);
        for (const client of clients) client.dispose();
        for (const entry of children) if (entry.child.exitCode === null && entry.child.signalCode === null) entry.child.kill();
        await Promise.all(children.map(entry => entry.completed.catch(() => {})));
    }
}

main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });