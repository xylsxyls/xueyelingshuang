'use strict';

// Real CLOSE then replacement OPEN while the service is at its declared limit.
// The pause gate holds only the owned probe's poll; IPC and gateway still run.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');
const { performance } = require('node:perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { createServiceClient } = require('./host/NetWxClient');
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));

async function main() {
    const args = Object.fromEntries(process.argv.slice(2).map(value => {
        const i = value.indexOf('='); assert.ok(i > 0); return [value.slice(0, i), value.slice(i + 1)];
    }));
    for (const key of ['--server', '--module', '--output']) assert.ok(args[key], key);
    const output = path.resolve(args['--output']);
    assert.ok(!fs.existsSync(output)); fs.mkdirSync(output, { recursive: true });
    const children = [], clients = [];
    const run = (name, options, ready) => {
        const item = start(path.resolve(args['--server']), options.concat([
            '--stop-file=' + path.join(output, name + '.stop'), '--run-for-ms=20000']), name, output, ready);
        children.push(item); return item;
    };
    let timer;
    try {
        await Promise.race([(async () => {
            const gateway = run('gateway', ['--protocol=3', '--listen=127.0.0.1', '--port=0',
                '--identity-directory=' + path.join(output, 'identity')], /GATEWAY_READY port=(\d+) pid=(\d+)/);
            const ready = await gateway.ready;
            const service = run('service', ['--self-test-service=' + ready[2]], /SERVICE_PROBE_READY/);
            await service.ready;
            const modulePath = path.resolve(args['--module']);
            const runtime = await NetInterfaceRuntime.load(require(modulePath), {
                randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size))),
                moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) }
            });
            const create = () => {
                const client = createServiceClient({ runtime, protocol: 3, autoReconnect: false,
                    serviceId: 1, address: '127.0.0.1', port: Number(ready[1]), platform: 2,
                    deviceId: Uint8Array.from(crypto.randomBytes(16)), clock: () => performance.now(),
                    createSocket: () => new TcpSocketHost() });
                clients.push(client); return client;
            };
            for (let i = 0; i < 8; ++i) await create().connect();
            let previous = clients[0];
            for (let round = 0; round < 5; ++round) {
                const paused = new Promise(resolve => {
                    let text = '';
                    const onData = bytes => { text += bytes.toString(); if (text.includes('SERVICE_PROBE_PAUSED')) {
                        service.child.stdout.off('data', onData); resolve();
                    } };
                    service.child.stdout.on('data', onData);
                });
                const gate = path.join(output, 'service.stop.pause');
                fs.writeFileSync(gate, ''); await paused;
                previous.dispose();
                await delay(150);
                const replacement = create();
                const connected = replacement.connect();
                connected.catch(() => {});
                await delay(300);
                fs.unlinkSync(gate);
                await connected;
                const active = clients.filter(client => client.ready && !client.disposed);
                assert.equal(active.length, 8, 'The seven retained clients and one replacement must remain connected');
                for (const client of active) {
                    const body = Uint8Array.from(crypto.randomBytes(257));
                    assert.deepEqual(Buffer.from(await client.request(1, 0x7345, body, 2000)), Buffer.from(body));
                }
                previous = replacement;
            }
            clients.forEach(client => client.dispose());
            fs.writeFileSync(path.join(output, 'service.stop'), '');
            fs.writeFileSync(path.join(output, 'gateway.stop'), '');
            const completed = await Promise.all(children.map(item => item.completed));
            assert.ok(completed.every(item => item.code === 0), 'Healthy service must stay registered');
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, capacity: 8,
                retainedClients: 7, replacementClients: 5, host: 'Real IPC/TCP/Wasm; not WeChat GUI' }, null, 2));
        })(), new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Capacity test deadline')), 18000); })]);
        console.log('PASS service capacity CLOSE/OPEN handover preserves healthy clients');
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ error: String(error.stack || error) })); throw error;
    } finally {
        clearTimeout(timer); clients.forEach(client => client.dispose());
        for (const item of children) if (item.child.exitCode === null && item.child.signalCode === null) item.child.kill();
        await Promise.all(children.map(item => item.completed.catch(() => {})));
    }
}
main().catch(error => { console.error(error.stack || error); process.exitCode = 1; });