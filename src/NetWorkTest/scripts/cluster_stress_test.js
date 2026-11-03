'use strict';

// Explicit opt-in pressure batch. --plan validates configuration without starting processes or TCP.
const fs = require('fs');
const path = require('path');
const assert = require('assert').strict;
const crypto = require('crypto');
const { execFile } = require('child_process');
const { performance } = require('perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { createServiceClient } = require('./host/NetWxClient');
const delay = value => new Promise(resolve => setTimeout(resolve, value));

async function main() {
    const options = {};
    let execute = false;
    for (const value of process.argv.slice(2)) {
        if (value === '--run' || value === '--plan') { assert.ok(!options.mode); options.mode = value; execute = value === '--run'; continue; }
        const split = value.indexOf('=');
        const key = value.slice(0, split);
        assert.ok(split > 0 && ['--server', '--module', '--output', '--nodes', '--clients', '--rounds', '--seconds', '--churn', '--duration', '--seed', '--stop-file', '--runtime'].includes(key) && !options[key]);
        options[key] = value.slice(split + 1);
    }
    const number = (key, fallback, min, max) => {
        const text = options[key] === undefined ? String(fallback) : options[key];
        assert.match(text, /^\d+$/);
        const value = Number(text);
        assert.ok(Number.isSafeInteger(value) && value >= min && value <= max, key);
        return value;
    };
    const nodes = number('--nodes', 3, 2, 4);
    const clients = number('--clients', 8, nodes, Math.min(32, nodes * 8));
    const rounds = number('--rounds', 100, 2, 100000);
    const duration = number('--duration', 0, 0, 1800);
    const seconds = number('--seconds', duration ? duration + 90 : 120, 5, 3600);
    const processBudgetMs = Math.min(seconds * 1000 + 10000, 3600000);
    assert.ok(!duration || seconds >= duration + 30, 'Deadline must include setup and cleanup');
    assert.ok(!duration || options['--rounds'] === undefined, 'Choose duration or rounds, not both');
    const seed = options['--seed'] || crypto.randomBytes(16).toString('hex');
    assert.match(seed, /^[a-zA-Z0-9_-]{1,64}$/);
    const payloadKey = crypto.createHash('sha256').update(seed).digest();
    const churn = number('--churn', 1, 0, 1) === 1;
    const runtimeKind = options['--runtime'] || 'network';
    assert.ok(['interface', 'network'].includes(runtimeKind), '--runtime');
    const Runtime = runtimeKind === 'network' ? require('./host/NetInterfaceRuntime').NetPeerRuntime : NetInterfaceRuntime;
    const createEndpoint = runtimeKind === 'network' ? require('../../CookWx/network/NetWxClient').createServiceClient : createServiceClient;
    const sizes = [0, 1, 1023, 1024, 16384, 65537, 262144, 1048560];
    const plan = { execute, runtime: runtimeKind, nodes, clients, rounds: duration ? null : rounds, durationSeconds: duration, seconds, seed, churn, payloadSizes: sizes,
        checks: ['independent TLS contexts', 'equal original request numbers across clients', 'three-way service allocation',
            'full payload boundary and binary zeros', 'mixed compressible/random payloads', 'bounded reconnect on service loss',
            'no application replay of accepted failures', 'original byte equality', 'real service execution counts', 'normal owned-process stop'],
        companionBoundaries: ['NetWorkTest --cases=1,2,3,4,5,6,7,8,9,10,35,36,37,38,41,42,43,44,69,70,74,76',
            'core NetInterface.cpp conditional main: compression checksum/size/fragment/negotiation',
            'mesh_gateway_process_test.js: membership, cross-node routing, stale-instance/cached-node recovery',
            'client_recovery_process_test.js: native/desktop/Wasm generations and entry/service failure',
            'file gateway cases: token ownership, offset/window, hash, cancellation and commit'] };
    if (!execute) { console.log(JSON.stringify(plan, null, 2)); return; }
    for (const key of ['--server', '--module', '--output']) assert.ok(options[key], key);
    const output = path.resolve(options['--output']);
    assert.ok(!fs.existsSync(output), 'Use a new owned batch directory');
    fs.mkdirSync(output, { recursive: true });
    const children = [];
    const endpoints = [];
    const services = [];
    const statistics = { attempted: 0, received: 0, interrupted: 0, bytes: 0 };
    const latency = new Uint32Array(8002);
    let latencySum = 0;
    let latencyMax = 0;
    const faultPending = new Set();
    const resources = [];
    let sampling = null;
    let sampleTimer;
    const sample = phase => {
        if (sampling) return sampling;
        const own = children.filter(item => item.child.exitCode === null && item.child.signalCode === null).map(item => item.child.pid);
        const record = { phase, at: performance.now(), node: process.memoryUsage(), processes: [] };
        if (process.platform !== 'win32' || !own.length) { resources.push(record); return Promise.resolve(); }
        sampling = new Promise(resolve => {
            execFile('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command',
                'Get-Process -Id ' + own.join(',') + ' -ErrorAction SilentlyContinue | Select-Object Id,WorkingSet64,PrivateMemorySize64,HandleCount,@{n="Threads";e={$_.Threads.Count}},CPU | ConvertTo-Json -Compress'],
                { windowsHide: true, timeout: 5000 }, (error, stdout) => {
                    try { record.processes = JSON.parse(stdout); } catch (_) { record.error = String(error || 'Missing process samples'); }
                    resources.push(record);
                    fs.writeFileSync(path.join(output, 'resources.json'), JSON.stringify(resources, null, 2));
                    sampling = null;
                    resolve();
                });
        });
        return sampling;
    };
    const run = (args, name, pattern) => {
        const child = start(path.resolve(options['--server']), args, name, output, pattern);
        children.push(child);
        return child;
    };
    let timer;
    let runtime;
    let faultInjected = false;
    try {
        const deadline = new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Bounded pressure deadline; incomplete batch is FAIL')), seconds * 1000); });
        await Promise.race([(async () => {
            const ports = [];
            for (let index = 0; index < nodes; ++index) {
                const name = 'node-' + index;
                const node = run(['--protocol=3', '--listen=127.0.0.1', '--port=0', '--identity-directory=' + path.join(output, name + '-identity'),
                    '--stop-file=' + path.join(output, name + '.stop'), '--run-for-ms=' + processBudgetMs,
                    ...(index ? ['--seeds=127.0.0.1:' + ports[0]] : [])], name, /GATEWAY_READY port=(\d+) pid=(\d+)/);
                const ready = await node.ready;
                ports.push(Number(ready[1]));
                const service = run(['--self-test-service=' + ready[2], '--stop-file=' + path.join(output, 'service-' + index + '.stop'),
                    '--run-for-ms=' + processBudgetMs], 'service-' + index, /SERVICE_PROBE_READY pid=/);
                await service.ready;
                services.push(service);
            }
            await delay(2500); // Bounded roster/snapshot propagation; no stress work is counted during setup.
            const modulePath = path.resolve(options['--module']);
            runtime = await Runtime.load(require(modulePath), { randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size))),
                createSocket: () => new TcpSocketHost(), clock: () => performance.now(),
                moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) } });
            for (let index = 0; index < clients; ++index) {
                const endpoint = createEndpoint({ runtime, protocol: 3, serviceId: 1, address: '127.0.0.1', port: ports[0],
                    seeds: ports.map(port => ({ address: '127.0.0.1', port })), platform: 1, deviceId: Uint8Array.from(crypto.randomBytes(16)),
                    compressionTypes: [0x7345], createSocket: () => new TcpSocketHost(), clock: () => performance.now() });
                endpoints.push(endpoint);
            }
            await Promise.all(endpoints.map(endpoint => endpoint.connect()));
            const loadStarted = performance.now();
            await sample('connected');
            sampleTimer = setInterval(() => { sample('load'); }, 10000);
            for (let round = 0; duration ? performance.now() - loadStarted < duration * 1000 : round < rounds; ++round) {
                if (options['--stop-file'] && fs.existsSync(options['--stop-file'])) throw new Error('CANCELLED by stop file');
                if (churn && !faultInjected && (duration ? performance.now() - loadStarted >= duration * 500 : round >= Math.floor(rounds / 2))) {
                    faultInjected = true;
                    endpoints.forEach((_, index) => faultPending.add(index));
                    services[0].child.kill(); // Only this batch's deliberately failing service.
                    await services[0].completed;
                }
                await Promise.all(endpoints.map(async (endpoint, index) => {
                    await endpoint.connect();
                    const size = sizes[(round + index) % sizes.length];
                    const iv = Buffer.alloc(16); iv.writeUInt32LE(round, 0); iv.writeUInt32LE(index, 4);
                    const payload = (round & 1) ? crypto.createCipheriv('aes-256-ctr', payloadKey, iv).update(Buffer.alloc(size)) : new Uint8Array(size).fill(index + 1);
                    ++statistics.attempted;
                    const started = performance.now();
                    try {
                        const request = endpoint.request(1, 0x7345, payload, 8000);
                        if (round === 0) assert.deepEqual(request.requestId, [1, 0]);
                        const response = await request;
                        assert.ok(Buffer.from(response).equals(Buffer.from(payload)), 'Cross-client, truncated or altered business bytes');
                        ++statistics.received;
                        statistics.bytes += size;
                        faultPending.delete(index);
                        const elapsed = performance.now() - started;
                        ++latency[Math.min(8001, Math.floor(elapsed))];
                        latencySum += elapsed;
                        latencyMax = Math.max(latencyMax, elapsed);
                    } catch (error) {
                        // Do not replay this operation. The next loop is a new operation and payload.
                        const expectedLoss = [-6, -7, -9, -10].includes(error.code) || [1, 3, 4].includes(error.status) ||
                            /TCP|socket|连接.*(?:断开|关闭|就绪)|网络.*就绪/i.test(error.message || '');
                        if (!faultPending.delete(index) || !expectedLoss || error.code === 'ERR_ASSERTION') throw error;
                        ++statistics.interrupted;
                    }
                }));
                statistics.rounds = round + 1;
                statistics.loadSeconds = (performance.now() - loadStarted) / 1000;
                if ((round + 1) % 10 === 0) {
                    fs.writeFileSync(path.join(output, 'progress.json'), JSON.stringify({ plan, statistics }));
                    console.log('Pressure rounds ' + (round + 1) + (duration ? ', seconds=' + statistics.loadSeconds.toFixed(1) : '/' + rounds) + ', received=' + statistics.received);
                }
            }
            assert.ok(statistics.received > 0 && statistics.received + statistics.interrupted === statistics.attempted);
            for (const endpoint of endpoints) endpoint.dispose();
            if (runtimeKind === 'network') {
                const end = performance.now() + 3000;
                while (endpoints.some(endpoint => !endpoint.released) && performance.now() < end) await delay(5);
                assert.ok(endpoints.every(endpoint => endpoint.released), 'All NetPeer handles must quiesce');
                assert.equal(runtime.hosts.size, 0);
            }
            clearInterval(sampleTimer);
            if (duration) await delay(5000);
            await sample('disconnected');
            for (let index = 0; index < nodes; ++index) {
                fs.writeFileSync(path.join(output, 'service-' + index + '.stop'), '');
                fs.writeFileSync(path.join(output, 'node-' + index + '.stop'), '');
            }
            const finished = await Promise.all(children.map(child => child.completed));
            const intentional = faultInjected ? services[0] : null;
            for (let index = 0; index < children.length; ++index) if (children[index] !== intentional) assert.equal(finished[index].code, 0);
            const receipts = services.map((service, index) => {
                const stdout = finished[children.indexOf(service)].stdout;
                return (stdout.match(/SERVICE_PROBE_ECHO /g) || []).length;
            });
            assert.ok(receipts.every(count => count > 0), 'Load selection never reached one of the registered nodes');
            const executed = receipts.reduce((sum, count) => sum + count, 0);
            assert.ok(executed >= statistics.received && executed <= statistics.attempted, 'An accepted operation was replayed');
            const percentile = fraction => { let count = 0; for (let index = 0; index < latency.length; ++index) { count += latency[index]; if (count >= Math.ceil(statistics.received * fraction)) return index + 1; } return null; };
            statistics.latencyMs = { p50Upper: percentile(.5), p95Upper: percentile(.95), p99Upper: percentile(.99), max: latencyMax, mean: latencySum / statistics.received };
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, plan, statistics, serviceReceipts: receipts,
                resourceSamples: resources.length, resourceErrors: resources.filter(item => item.error).length }, null, 2));
        })(), deadline]);
        console.log('Bounded cluster pressure batch: PASS');
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ pass: false, plan, statistics, error: String(error.stack || error) }, null, 2));
        throw error;
    } finally {
        clearTimeout(timer);
        clearInterval(sampleTimer);
        if (sampling) await sampling;
        for (const endpoint of endpoints) endpoint.dispose();
        if (runtimeKind === 'network' && runtime) {
            const end = performance.now() + 2000;
            while (endpoints.some(endpoint => !endpoint.released) && performance.now() < end) await delay(5);
            runtime.dispose();
        }
        for (const child of children) if (child.child.exitCode === null && child.child.signalCode === null) child.child.kill();
        await Promise.all(children.map(child => child.completed.catch(() => {})));
    }
}
main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });