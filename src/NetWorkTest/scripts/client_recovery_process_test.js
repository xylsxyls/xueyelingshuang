'use strict';

// Actual public C ABI and desktop NetSender survive entry-node loss without host-side reconnection.
const fs = require('fs');
const path = require('path');
const assert = require('assert').strict;
const crypto = require('crypto');
const { performance } = require('perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { NetInterfaceDirectory } = require('./host/NetInterfaceDirectory');
const { createWxClient } = require('./host/NetWxClient');
const { TcpSocketHost } = require('./service_gateway_process_test');
const delay = milliseconds => new Promise(resolve => setTimeout(resolve, milliseconds));

async function main() {
    const options = {};
    for (const value of process.argv.slice(2)) {
        const split = value.indexOf('=');
        const name = value.slice(0, split);
        assert.ok(split > 0 && ['--server', '--client', '--test', '--module', '--output', '--fault', '--wx'].includes(name) && !options[name]);
        options[name] = value.slice(split + 1);
    }
    for (const key of ['--server', '--client', '--test', '--module', '--output']) assert.ok(options[key]);
    const fault = options['--fault'] || 'entry';
    assert.ok(['entry', 'service'].includes(fault));
    assert.ok(!options['--wx'] || options['--wx'] === '1');
    const includeWx = options['--wx'] === '1';
    const output = path.resolve(options['--output']);
    assert.ok(!fs.existsSync(output), 'Use a fresh owned report directory');
    fs.mkdirSync(output, { recursive: true });
    const children = [];
    const run = (binary, args, name, ready) => {
        const entry = start(path.resolve(binary), args, name, output, ready);
        children.push(entry);
        return entry;
    };
    let timer;
    let wxClient;
    try {
        const modulePath = path.resolve(options['--module']);
        const runtime = await NetInterfaceRuntime.load(require(modulePath), {
            randomSource: length => Promise.resolve(Uint8Array.from(crypto.randomBytes(length))),
            moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) }
        });
        const gateway = (name, seed) => run(options['--server'], ['--protocol=3', '--listen=127.0.0.1', '--port=0',
            '--identity-directory=' + path.join(output, name + '-identity'), '--stop-file=' + path.join(output, name + '.stop'),
            '--run-for-ms=60000', ...(seed ? ['--seeds=127.0.0.1:' + seed] : [])], name, /GATEWAY_READY port=(\d+) pid=(\d+)/);
        const service = (name, pid) => run(options['--server'], ['--self-test-service=' + pid,
            '--stop-file=' + path.join(output, name + '.stop'), '--run-for-ms=60000'], name, /SERVICE_PROBE_READY pid=(\d+)/);
        const deadline = new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Client recovery deadline')), 50000); });
        await Promise.race([(async () => {
            const a = gateway('node-a');
            const readyA = await a.ready;
            const serviceA = service('service-a', readyA[2]);
            await serviceA.ready;
            const clientCache = path.join(output, 'client-nodes.cache');
            const client = run(options['--client'], ['--protocol=3', '--address=127.0.0.1', '--port=' + readyA[1],
                '--cache-file=' + clientCache, '--stop-file=' + path.join(output, 'client.stop'), '--run-for-ms=60000'],
                'client-gateway', /CLIENT_GATEWAY_READY pid=(\d+)/);
            const clientReady = await client.ready;
            const native = run(options['--test'], ['--recovery-native=127.0.0.1:' + readyA[1]], 'native-endpoint', /RECOVERY_ROUND count=1 /);
            const desktop = run(options['--test'], ['--recovery-desktop=' + clientReady[1]], 'desktop-endpoint', /RECOVERY_ROUND count=1 /);
            await Promise.all([native.ready, desktop.ready]);
            let wxNodes = [];
            let wxClosed = 0;
            let wxGeneration = 0;
            if (includeWx) {
                const storage = new Map();
                wxClient = createWxClient(runtime, { protocol: 3, serviceId: 1, address: '127.0.0.1', port: Number(readyA[1]),
                    onNodes: (wire, points) => { wxNodes = points; }, onClose: error => { if (error) ++wxClosed; } },
                    { createTCPSocket: () => new TcpSocketHost(), getPerformance: () => performance,
                        getStorageSync: key => storage.get(key), setStorageSync: (key, value) => storage.set(key, value) });
                await wxClient.connect();
                wxGeneration = wxClient.generation;
                const payload = new Uint8Array(65537).fill(0x41);
                const request = wxClient.request(1, 0x7345, payload, 10000);
                assert.deepEqual(request.requestId, [1, 0]);
                assert.ok(Buffer.from(await request).equals(Buffer.from(payload)));
            }
            const b = gateway('node-b', readyA[1]);
            const readyB = await b.ready;
            const serviceB = service('service-b', readyB[2]);
            await serviceB.ready;
            let cacheReady = false;
            const learnedBy = Date.now() + 12000;
            while ((!cacheReady || (includeWx && !wxNodes.some(point => point.port === Number(readyB[1])))) && Date.now() < learnedBy) {
                if (fs.existsSync(clientCache)) {
                    try {
                        const directory = new NetInterfaceDirectory(runtime, Uint8Array.from(fs.readFileSync(clientCache)), true);
                        try { cacheReady = directory.points().some(point => point.port === Number(readyB[1])); }
                        finally { directory.dispose(); }
                    } catch (_) { /* An atomic cache replacement may race a host read; poll the next snapshot. */ }
                }
                if (!cacheReady || (includeWx && !wxNodes.some(point => point.port === Number(readyB[1])))) await delay(30);
            }
            assert.ok(cacheReady, 'Real NetClientManager did not authenticate and persist dynamic B');
            if (includeWx) assert.ok(wxNodes.some(point => point.port === Number(readyB[1])), 'Wasm client did not periodically discover B');
            await delay(1000); // Both public clients have the same bounded five-second roster refresh.
            const failed = fault === 'entry' ? a : serviceA;
            const faultAt = performance.now();
            failed.child.kill(); // Owned process only. Neither endpoint is recreated or manually reconnected.
            await failed.completed;
            const endpoints = await Promise.all([native.completed, desktop.completed]);
            const recoveryMilliseconds = Math.round(performance.now() - faultAt);
            assert.ok(recoveryMilliseconds < 10000, 'Transport close must not wait for the business idle timeout');
            for (const entry of endpoints) {
                assert.equal(entry.code, 0, entry.stdout);
                assert.match(entry.stdout, /RECOVERY_CLIENT_PASS/);
                const rounds = [...entry.stdout.matchAll(/RECOVERY_ROUND count=(\d+) generation=(\d+) request=(\d+)/g)];
                assert.equal(rounds.length, 2);
                assert.ok(BigInt(rounds[1][2]) > BigInt(rounds[0][2]));
                assert.equal(rounds[0][3], '1');
                assert.ok(BigInt(rounds[1][3]) > BigInt(rounds[0][3])); // The same facade retains its monotonic request source across TLS generations.
            }
            if (includeWx) {
                const restoredBy = Date.now() + 10000;
                while ((!wxClient.ready || wxClient.generation <= wxGeneration) && Date.now() < restoredBy) await delay(20);
                assert.ok(wxClient.ready && wxClient.generation > wxGeneration && wxClosed > 0, 'Wx controller did not automatically establish a new TLS generation');
                const payload = new Uint8Array(65539).fill(0x42);
                const request = wxClient.request(1, 0x7345, payload, 10000);
                assert.deepEqual(request.requestId, [1, 0]);
                assert.ok(Buffer.from(await request).equals(Buffer.from(payload)));
                wxClient.dispose();
            }
            await serviceA.completed;
            fs.writeFileSync(path.join(output, 'service-b.stop'), '');
            const servedB = await serviceB.completed;
            assert.equal(servedB.code, 0);
            assert.equal((servedB.stdout.match(/SERVICE_PROBE_ECHO/g) || []).length, includeWx ? 3 : 2, 'B observed a replay or a missing new operation');
            fs.writeFileSync(path.join(output, 'client.stop'), '');
            fs.writeFileSync(path.join(output, 'node-b.stop'), '');
            assert.ok((await Promise.all([client.completed, b.completed])).every(result => result.code === 0));
            if (fault === 'service') {
                fs.writeFileSync(path.join(output, 'node-a.stop'), '');
                assert.equal((await a.completed).code, 0);
            }
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, actualOwnedProcessTerminated: fault,
                nativePublicCAbiAutomaticallyRecovered: true, desktopPublicNetSenderAutomaticallyRecovered: true,
                dynamicNodePersistedByClientGateway: true, distinctTlsGenerations: true, noBusinessReplay: true,
                productionWasmClientAutomaticallyRecovered: includeWx,
                recoveryMilliseconds,
                noHostSideReconnect: true, sameMachineIndependentProcesses: true }, null, 2));
        })(), deadline]);
        console.log('Native C ABI and desktop NetSender entry failover, discovery/cache and new TLS: PASS');
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ error: error.stack || String(error) }, null, 2));
        throw error;
    } finally {
        clearTimeout(timer);
        if (wxClient) wxClient.dispose();
        for (const entry of children) if (entry.child.exitCode === null && entry.child.signalCode === null) entry.child.kill();
        await Promise.all(children.map(entry => entry.completed.catch(() => {})));
    }
}

main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });