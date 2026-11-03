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
        const i = value.indexOf('='); assert.ok(i > 0); return [value.slice(0, i), value.slice(i + 1)];
    }));
    for (const key of ['--server', '--module', '--output']) assert.ok(args[key], key);
    const output = path.resolve(args['--output']);
    assert.ok(!fs.existsSync(output)); fs.mkdirSync(output, { recursive: true });
    const children = [];
    const run = (name, options, ready) => {
        const item = start(path.resolve(args['--server']), options.concat([
            '--stop-file=' + path.join(output, name + '.stop'), '--run-for-ms=15000']), name, output, ready);
        children.push(item); return item;
    };
    let timer, client, healthy;
    try {
        await Promise.race([(async () => {
            const gateway = run('gateway', ['--protocol=3', '--listen=127.0.0.1', '--port=0',
                '--identity-directory=' + path.join(output, 'identity')], /GATEWAY_READY port=(\d+) pid=(\d+)/);
            const ready = await gateway.ready;
            const service = run('service', ['--self-test-managed=' + ready[2]], /MANAGED_PROBE_READY/);
            await service.ready;
            const modulePath = path.resolve(args['--module']);
            const runtime = await NetInterfaceRuntime.load(require(modulePath), {
                randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size))),
                moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) }
            });
            client = createServiceClient({ runtime, protocol: 3, autoReconnect: false,
                serviceId: 1, address: '127.0.0.1', port: Number(ready[1]), platform: 3,
                deviceId: Uint8Array.from(crypto.randomBytes(16)), clock: () => performance.now(),
                createSocket: () => new TcpSocketHost() });
            await client.connect();
            healthy = createServiceClient({ runtime, protocol: 3, autoReconnect: false,
                serviceId: 1, address: '127.0.0.1', port: Number(ready[1]), platform: 3,
                deviceId: Uint8Array.from(crypto.randomBytes(16)), clock: () => performance.now(),
                createSocket: () => new TcpSocketHost() });
            await healthy.connect();
            const held = new Promise(resolve => {
                let text = '';
                const onData = bytes => { text += bytes.toString(); if (text.includes('MANAGED_PROBE_HELD')) {
                    service.child.stdout.off('data', onData); resolve();
                } };
                service.child.stdout.on('data', onData);
            });
            const gate = path.join(output, 'service.stop.hold');
            fs.writeFileSync(gate, '');
            const old = client.request(1, 0x7351, new Uint8Array([11]), 5000);
            const settled = Promise.allSettled([old]);
            await held;
            const other = await healthy.request(1, 0x7352, new Uint8Array([44]), 1500);
            assert.deepEqual(Buffer.from(other), Buffer.from([44]), 'Another root must bypass the blocked callback');
            old.cancel();
            assert.equal((await settled)[0].status, 'rejected');
            await new Promise(resolve => setTimeout(resolve, 250));
            fs.unlinkSync(gate);
            const result = await client.request(1, 0x7351, new Uint8Array([22]), 2000);
            assert.deepEqual(Buffer.from(result), Buffer.from([22]));
            // The optional internal dependency was absent through registration
            // and cancelled work. Bring it up later without restarting the caller
            // or its client, then verify the real two-service RPC path.
            await new Promise(resolve => setTimeout(resolve, 1000));
            assert.equal(client.ready, true);
            const internal = run('internal', ['--self-test-managed-internal=' + ready[2]], /MANAGED_PROBE_READY/);
            await internal.ready;
            const until = performance.now() + 6000;
            let nested = false;
            while (performance.now() < until) {
                try {
                    const reply = await client.request(1, 0x7345, new Uint8Array([33]), 2000);
                    assert.deepEqual(Buffer.from(reply), Buffer.from([33])); nested = true; break;
                } catch (error) { assert.equal(error.status, 1, 'Only the idempotent test echo retries UNAVAILABLE'); }
                await new Promise(resolve => setTimeout(resolve, 100));
            }
            assert.equal(nested, true, 'Late dependency must become available on the existing client');
            fs.writeFileSync(gate, '');
            const overloaded = client.request(1, 0x7351, new Uint8Array([55]), 9000);
            const busyResult = Promise.allSettled([overloaded]);
            let finished = false;
            busyResult.then(() => { finished = true; });
            let healthyReplies = 0;
            while (!finished) {
                const reply = await healthy.request(1, 0x7352, new Uint8Array([66]), 1500);
                assert.deepEqual(Buffer.from(reply), Buffer.from([66]));
                ++healthyReplies;
                await new Promise(resolve => setTimeout(resolve, 150));
            }
            const rejected = (await busyResult)[0];
            assert.equal(rejected.status, 'rejected');
            assert.equal(rejected.reason.status, 2, 'Unaccepted work must fail BUSY without a service disconnect');
            assert.ok(healthyReplies >= 10 && healthy.ready && client.ready);
            fs.unlinkSync(gate);
            await new Promise(resolve => setTimeout(resolve, 100));
            client.dispose();
            healthy.dispose();
            fs.writeFileSync(path.join(output, 'service.stop'), '');
            const completed = await service.completed;
            assert.equal(completed.code, 0);
            assert.equal((completed.stdout.match(/MANAGED_PROBE_EXECUTED/g) || []).length, 1,
                'A cancelled, unaccepted callback must never execute after backpressure is lifted');
            fs.writeFileSync(path.join(output, 'internal.stop'), '');
            assert.equal((await internal.completed).code, 0);
            fs.writeFileSync(path.join(output, 'gateway.stop'), '');
            assert.equal((await gateway.completed).code, 0);
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true,
                cancelledCallbackExecutions: 0, subsequentRequestExecutions: 1, realIpcTcpWasm: true,
                dependencyInitiallyAbsent: true, lateDependencyRecoveredWithoutClientRestart: true,
                healthyRootProgressDuringCallbackBackpressure: true, healthyRepliesDuringOverload: healthyReplies,
                overloadResult: 'BUSY; both clients stay connected' }, null, 2));
        })(), new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Cancel test deadline')), 12000); })]);
        console.log('PASS cancellation removes unaccepted queued work without breaking later requests');
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ error: String(error.stack || error) })); throw error;
    } finally {
        clearTimeout(timer); if (client) client.dispose(); if (healthy) healthy.dispose();
        for (const item of children) if (item.child.exitCode === null && item.child.signalCode === null) item.child.kill();
        await Promise.all(children.map(item => item.completed.catch(() => {})));
    }
}
main().catch(error => { console.error(error.stack || error); process.exitCode = 1; });