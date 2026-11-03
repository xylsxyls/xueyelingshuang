'use strict';

// A deliberately blocked service consumer shares IPC and NODE links with a healthy
// client. Real production endpoints must close only the stalled channel.
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const assert = require('assert').strict;
const { performance } = require('perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetPeerRuntime } = require('./host/NetInterfaceRuntime');
const { createServiceClient } = require('../../CookWx/network/NetWxClient');
const delay = value => new Promise(resolve => setTimeout(resolve, value));

async function main() {
    const args = {};
    for (const value of process.argv.slice(2)) {
        const split = value.indexOf('=');
        const key = value.slice(0, split);
        assert.ok(split > 0 && ['--server', '--module', '--output'].includes(key) && !args[key]);
        args[key] = value.slice(split + 1);
    }
    for (const key of ['--server', '--module', '--output']) assert.ok(args[key], key);
    const output = path.resolve(args['--output']);
    assert.ok(!fs.existsSync(output));
    fs.mkdirSync(output, { recursive: true });
    const children = [], clients = [];
    const run = (name, options, ready) => {
        const item = start(path.resolve(args['--server']), options.concat([
            '--stop-file=' + path.join(output, name + '.stop'), '--run-for-ms=30000']), name, output, ready);
        children.push(item);
        return item;
    };
    let timer;
    try {
        await Promise.race([(async () => {
            const a = run('entry', ['--protocol=3', '--listen=127.0.0.1', '--port=0',
                '--identity-directory=' + path.join(output, 'entry-identity')], /GATEWAY_READY port=(\d+) pid=(\d+)/);
            const readyA = await a.ready;
            const b = run('target', ['--protocol=3', '--listen=127.0.0.1', '--port=0',
                '--identity-directory=' + path.join(output, 'target-identity'), '--seeds=127.0.0.1:' + readyA[1]],
                /GATEWAY_READY port=(\d+) pid=(\d+)/);
            const readyB = await b.ready;
            const service = run('service', ['--self-test-service=' + readyB[2]], /SERVICE_PROBE_READY/);
            await service.ready;
            await delay(2500);
            const modulePath = path.resolve(args['--module']);
            const runtime = await NetPeerRuntime.load(require(modulePath), {
                randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size))),
                createSocket: () => new TcpSocketHost(),
                moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) }
            });
            let closed = 0;
            for (let index = 0; index < 2; ++index) {
                const client = createServiceClient({ runtime, protocol: 3, autoReconnect: false,
                    serviceId: 1, address: '127.0.0.1', port: Number(readyA[1]), platform: 2,
                    deviceId: Uint8Array.from(crypto.randomBytes(16)), clock: () => performance.now(),
                    createSocket: () => new TcpSocketHost(), onClose: () => {
                        if (index === 0) { ++closed; clients[0].dispose(); }
                    }
                });
                clients.push(client);
                await client.connect();
            }
            const pending = [clients[0].request(1, 0x7350, new Uint8Array(1), 15000),
                clients[0].request(1, 0x7345, Uint8Array.from(crypto.randomBytes(1048560)), 15000)];
            const results = Promise.allSettled(pending);
            const began = performance.now();
            let healthy = 0;
            do {
                const body = Uint8Array.from(crypto.randomBytes(65537));
                const reply = await clients[1].request(1, 0x7345, body, 2500);
                assert.ok(Buffer.from(reply).equals(Buffer.from(body)));
                ++healthy;
                await delay(150);
            } while (performance.now() - began < 7500);
            const ended = await results;
            assert.ok(ended.every(item => item.status === 'rejected'), 'Stalled work must not report success');
            assert.equal(closed, 1, 'Exactly one terminal notification for the stalled channel');
            assert.ok(healthy >= 10 && clients[1].ready, 'Healthy client stopped progressing');
            clients.forEach(client => client.dispose());
            const releaseDeadline = performance.now() + 3000;
            while (clients.some(client => !client.released) && performance.now() < releaseDeadline) await delay(5);
            assert.ok(clients.every(client => client.released), 'All client handles must quiesce');
            assert.equal(closed, 1, 'Cleanup repeated the stalled generation terminal');
            for (const name of ['service', 'entry', 'target']) fs.writeFileSync(path.join(output, name + '.stop'), '');
            const completed = await Promise.all(children.map(item => item.completed));
            assert.ok(completed.every(item => item.code === 0));
            assert.match(completed[2].stdout, /SERVICE_PROBE_STALLED/);
            assert.match(completed[2].stdout, /SERVICE_PROBE_STALL_CLOSED/);
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, healthyReplies: healthy,
                sameServiceProcess: true, sharedNodeLink: true, stalledTerminalCallbacks: closed, allClientsReleased: true,
                host: 'Production Wasm/native TCP, not WeChat GUI' }, null, 2));
        })(), new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Slow channel deadline')), 25000); })]);
        console.log('Shared NODE/IPC slow-channel isolation: PASS');
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ error: String(error.stack || error) }));
        throw error;
    } finally {
        clearTimeout(timer);
        clients.forEach(client => client.dispose());
        for (const item of children) if (item.child.exitCode === null && item.child.signalCode === null) item.child.kill();
        await Promise.all(children.map(item => item.completed.catch(() => {})));
    }
}
main().catch(error => { console.error(error.stack || error); process.exitCode = 1; });