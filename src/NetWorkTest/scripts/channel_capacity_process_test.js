'use strict';
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');
const { performance } = require('node:perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { createServiceClient } = require('./host/NetWxClient');

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
            const gateway = launch('gateway', ['--listen=127.0.0.1', '--port=0',
                '--identity-directory=' + path.join(output, 'identity')], /GATEWAY_READY port=(\d+) pid=(\d+)/);
            const ready = await gateway.ready;
            const service = launch('service', ['--self-test-capacity=' + ready[2]], /MANAGED_PROBE_READY/);
            await service.ready;
            const modulePath = path.resolve(args['--module']);
            const runtime = await NetInterfaceRuntime.load(require(modulePath), {
                randomSource: async size => Uint8Array.from(crypto.randomBytes(size)),
                moduleOptions: { wasmBinary: require('./host/read_wasm').readWasm(modulePath) }
            });
            for (let index = 0; index < 40; ++index) {
                const client = createServiceClient({ runtime, protocol: 3, autoReconnect: false, serviceId: 1,
                    address: '127.0.0.1', port: Number(ready[1]), platform: 3,
                    deviceId: Uint8Array.from(crypto.randomBytes(16)), createSocket: () => new TcpSocketHost(),
                    clock: () => performance.now(), maxRequests: 8 });
                clients.push(client); await client.connect();
            }
            assert.ok(clients.every(client => client.ready));
            const body = Uint8Array.from(crypto.randomBytes(4097));
            for (const client of clients) assert.deepEqual(Buffer.from(await client.request(1, 0x7352, body, 4000)), Buffer.from(body));
            const hold = path.join(output, 'service.stop.hold');
            fs.writeFileSync(hold, '');
            const pending = Array.from({ length: 8 }, () => clients[0].request(1, 0x7351, body, 4000));
            pending.forEach(item => item.catch(() => {}));
            await assert.rejects(async () => clients[0].request(1, 0x7351, body, 4000));
            for (const item of pending) item.cancel();
            await Promise.allSettled(pending);
            fs.unlinkSync(hold);
            assert.deepEqual(Buffer.from(await clients[1].request(1, 0x7352, body, 4000)), Buffer.from(body));
            clients.forEach(client => client.dispose());
            fs.writeFileSync(path.join(output, 'service.stop'), '');
            assert.equal((await service.completed).code, 0);
            fs.writeFileSync(path.join(output, 'gateway.stop'), '');
            assert.equal((await gateway.completed).code, 0);
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, online: 40, channelLimit: 64,
                negotiatedRequestLimit: 8, ninthRequestRejected: true, unauthenticatedChannelsOccupyCapacity: true }, null, 2));
        })(), new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Capacity test deadline')), 50000); })]);
        console.log('PASS 40 online channels, independent 8-request budget, cancellation and remaining client health');
    } finally {
        clearTimeout(timer); clients.forEach(client => client.dispose());
        for (const child of children) if (child.child.exitCode === null && child.child.signalCode === null) child.child.kill();
        await Promise.all(children.map(child => child.completed.catch(() => {})));
    }
}
main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });