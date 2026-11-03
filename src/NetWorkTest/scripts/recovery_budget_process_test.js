'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const assert = require('assert').strict;
const { performance } = require('perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { createServiceClient } = require('./host/NetWxClient');

async function main() {
    const args = {};
    for (const value of process.argv.slice(2)) {
        const split = value.indexOf('='), key = value.slice(0, split);
        assert.ok(split > 0 && ['--server', '--test', '--module', '--output'].includes(key) && !args[key]);
        args[key] = value.slice(split + 1);
    }
    for (const key of ['--server', '--test', '--module', '--output']) assert.ok(args[key]);
    const output = path.resolve(args['--output']);
    assert.ok(!fs.existsSync(output));
    fs.mkdirSync(output, { recursive: true });
    const children = [];
    let client, timer;
    try {
        await Promise.race([(async () => {
            const server = start(path.resolve(args['--server']), ['--protocol=3', '--listen=127.0.0.1', '--port=0',
                '--identity-directory=' + path.join(output, 'identity'), '--stop-file=' + path.join(output, 'stop'),
                '--run-for-ms=45000'], 'gateway', output, /GATEWAY_READY port=(\d+) pid=(\d+)/);
            children.push(server);
            const ready = await server.ready;
            const native = start(path.resolve(args['--test']), ['--unavailable-service=127.0.0.1:' + ready[1]],
                'native', output, /UNAVAILABLE_SERVICE_PASS/);
            children.push(native);
            const modulePath = path.resolve(args['--module']);
            const runtime = await NetInterfaceRuntime.load(require(modulePath), {
                randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size))),
                moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) }
            });
            client = createServiceClient({ runtime, protocol: 3, serviceId: 77, address: '127.0.0.1', port: Number(ready[1]),
                platform: 2, deviceId: Uint8Array.from(crypto.randomBytes(16)), clock: () => performance.now(),
                createSocket: () => new TcpSocketHost() });
            const began = performance.now();
            await assert.rejects(client.connect(), /恢复连接超时/);
            const elapsed = Math.round(performance.now() - began);
            assert.ok(elapsed >= 29500 && elapsed < 34000 && client.disposed);
            const finished = await native.completed;
            assert.equal(finished.code, 0, finished.stdout);
            assert.match(finished.stdout, /UNAVAILABLE_SERVICE_PASS/);
            fs.writeFileSync(path.join(output, 'stop'), '');
            assert.equal((await server.completed).code, 0);
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, nativeBudget: '30 seconds',
                wasmMilliseconds: elapsed, noServiceRegistered: true, noBusinessSubmitted: true }, null, 2));
        })(), new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Budget test deadline')), 42000); })]);
        console.log('Native/Wasm missing-service total recovery budget: PASS');
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ error: String(error.stack || error) }));
        throw error;
    } finally {
        clearTimeout(timer);
        if (client) client.dispose();
        for (const entry of children) if (entry.child.exitCode === null && entry.child.signalCode === null) entry.child.kill();
        await Promise.all(children.map(entry => entry.completed.catch(() => {})));
    }
}
main().catch(error => { console.error(error.stack || error); process.exitCode = 1; });