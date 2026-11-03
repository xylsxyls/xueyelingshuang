'use strict';

// Real production C++ NetSender in compiled NetPeer.wasm. The JavaScript only
// schedules the host, injects faults into owned PIDs and checks business results.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { performance } = require('node:perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetPeerRuntime } = require('./host/NetInterfaceRuntime');
const { createServiceClient } = require('../../CookWx/network/NetWxClient');
const { readWasm } = require('./host/read_wasm');
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));

// A test-only silent link drops bytes and terminal notifications for the failed
// entry. The C++ management heartbeat must detect it without a TCP close event.
class SilentSocketHost extends TcpSocketHost {
    constructor(gate) { super(); this.gate = gate; this.port = 0; }
    blocked() { return this.gate.port !== 0 && this.gate.port === this.port; }
    connect(options) { this.port = options.port; super.connect(options); }
    onConnect(callback) { super.onConnect(event => { if (!this.blocked()) callback(event); }); }
    onMessage(callback) { super.onMessage(event => { if (!this.blocked()) callback(event); }); }
    onError(callback) { super.onError(event => { if (!this.blocked()) callback(event); }); }
    onClose(callback) { super.onClose(event => { if (!this.blocked()) callback(event); }); }
    write(bytes) { if (!this.blocked()) super.write(bytes); }
}

async function until(predicate, milliseconds, message) {
    const end = performance.now() + milliseconds;
    while (!predicate() && performance.now() < end) await delay(10);
    assert.ok(predicate(), message);
}

async function scenario(server, module, output, fault) {
    fs.mkdirSync(output);
    const children = [], clients = [], expectedFaults = new Set();
    const wholeNode = fault !== 'service-only';
    const gate = { port: 0 };
    let runtime;
    const run = (name, args, ready) => {
        const item = start(server, args.concat(['--stop-file=' + path.join(output, name + '.stop'), '--run-for-ms=95000']),
            name, output, ready);
        item.name = name;
        item.text = '';
        item.child.stdout.on('data', bytes => { item.text += bytes.toString('utf8'); });
        children.push(item);
        return item;
    };
    const gateway = (name, port) => run(name, ['--protocol=3', '--listen=127.0.0.1', '--port=0',
        '--identity-directory=' + path.join(output, name + '-identity'), ...(port ? ['--seeds=127.0.0.1:' + port] : [])],
        /GATEWAY_READY port=(\d+) pid=(\d+)/);
    const service = (name, pid) => run(name, ['--self-test-service=' + pid], /SERVICE_PROBE_READY pid=(\d+)/);
    const echo = async (client, size, seed) => {
        const bytes = Uint8Array.from({ length: size }, (_, index) => (index * 29 + seed) & 255);
        assert.deepEqual(await client.request(1, 0x7345, bytes, 5000), bytes);
    };
    try {
        runtime = await NetPeerRuntime.load(require(module), {
            moduleOptions: { wasmBinary: readWasm(module) },
            createSocket: () => new SilentSocketHost(gate), randomSource: size => crypto.randomBytes(size),
            clock: () => performance.now()
        });
        const create = async port => {
            const observed = { generations: [], closed: 0, events: [] };
            const client = createServiceClient({ protocol: 3, runtime, platform: 1, serviceId: 1,
                address: '127.0.0.1', port: Number(port), deviceId: Uint8Array.from(crypto.randomBytes(16)),
                onReady(current) {
                    observed.generations.push(current.sender.gatewayGeneration());
                    observed.events.push({ kind: 'ready', at: performance.now(), generation: String(current.sender.gatewayGeneration()) });
                },
                onClose(error) {
                    ++observed.closed;
                    observed.events.push({ kind: 'close', at: performance.now(), code: error && error.code,
                        detail: error && error.detail, readyCount: observed.generations.length });
                } });
            client.observed = observed;
            clients.push(client);
            await client.connect();
            return client;
        };
        const a = gateway('node-a');
        const ar = await a.ready;
        const victim = service('service-a', ar[2]);
        await victim.ready;
        const migrants = await Promise.all(Array.from({ length: 8 }, () => create(ar[1])));
        await Promise.all(migrants.map((client, index) => echo(client, 2000 + index, index)));
        await until(() => (victim.text.match(/SERVICE_PROBE_ECHO /g) || []).length >= 8, 1000, 'Victim execution log');
        assert.equal((victim.text.match(/SERVICE_PROBE_ECHO /g) || []).length, 8, 'All original clients must be on the victim');
        const original = migrants.map(client => client.connection());
        const b = gateway('node-b', ar[1]);
        const br = await b.ready;
        // First connection retries are legitimate before any B service registers.
        // Force that boundary, then measure only changes to established healthy sessions.
        const warmB = create(br[1]);
        await until(() => clients[8].observed.closed >= 1, 8000, 'Initial unavailable service must report a failed attempt');
        const spareB = service('service-b', br[2]);
        await spareB.ready;
        const recoveredWarm = await warmB;
        assert.equal(recoveredWarm.observed.generations.length, 1);
        const startupRetryCount = recoveredWarm.observed.closed;
        assert.ok(startupRetryCount >= 1);
        // This retry may select A as ingress and reach B through relay. It is not
        // a surviving-ingress fixture when A is killed. Retire it before pinning B.
        recoveredWarm.dispose();
        await until(() => recoveredWarm.released, 3000, 'Startup retry fixture must release');
        await delay(500);
        const retained = [];
        for (let index = 0; index < 3; ++index) retained.push(await create(br[1]));
        await Promise.all(retained.map((client, index) => echo(client, 1000 + index, index)));
        await until(() => (spareB.text.match(/SERVICE_PROBE_ECHO /g) || []).length >= 3, 1000, 'Warm B execution log');
        const c = gateway('node-c', ar[1]);
        const cr = await c.ready;
        const spareC = service('service-c', cr[2]);
        await spareC.ready;
        await delay(500);
        retained.push(await create(cr[1]));
        await echo(retained[3], 1003, 3);
        await until(() => (spareC.text.match(/SERVICE_PROBE_ECHO /g) || []).length >= 1, 1000, 'Warm C execution log');
        assert.equal((spareB.text.match(/SERVICE_PROBE_ECHO /g) || []).length, 3);
        assert.equal((spareC.text.match(/SERVICE_PROBE_ECHO /g) || []).length, 1);
        const healthyPins = retained.map(client => client.connection());
        retained.forEach(client => { client.observed.beforeFault = { closed: client.observed.closed,
            generations: client.observed.generations.map(String) }; });
        // Discovery is the C++ background management link. Original clients have
        // only A configured as a seed; B/C must be learned before A is removed.
        await delay(6000);
        const terminals = Array(8).fill(0);
        const pending = migrants.map((client, index) => client.request(1, 0x7350, new Uint8Array([index]), 55000).then(
            () => { ++terminals[index]; throw new Error('A held operation unexpectedly completed'); },
            error => { ++terminals[index]; assert.ok(error.code < 0 && error.code !== -9, 'Fault must settle before request timeout'); }));
        await until(() => (victim.text.match(/SERVICE_PROBE_STALLED /g) || []).length === 8, 5000, 'All held requests reached the original service');
        const faultAt = performance.now();
        if (fault === 'silent-node-loss') gate.port = Number(ar[1]);
        expectedFaults.add(victim);
        victim.child.kill();
        if (wholeNode) { expectedFaults.add(a); a.child.kill(); }
        await Promise.all([...expectedFaults].map(item => item.completed));
        await Promise.all(pending);
        await until(() => migrants.every((client, index) => client.ready &&
            client.sender.gatewayGeneration() > original[index].generation), 30000, 'Original NetSender instances did not recover automatically');
        const recoveredMs = Math.ceil(performance.now() - faultAt);
        for (let index = 0; index < migrants.length; ++index) {
            assert.equal(original[index].ready, false);
            assert.equal(terminals[index], 1, 'Each interrupted request has exactly one terminal');
            assert.ok(migrants[index].observed.closed >= 1);
        }
        await Promise.all(retained.map((client, index) => echo(client, 60000 + index, index + 17)));
        retained.forEach((client, index) => {
            assert.equal(healthyPins[index].ready, true, 'Healthy existing TLS must not migrate');
            assert.deepEqual(client.observed.generations.map(String), client.observed.beforeFault.generations,
                'Fault must not create a new healthy generation');
            assert.equal(client.observed.closed, client.observed.beforeFault.closed,
                'Fault must not close an already healthy connection');
        });
        for (let round = 0; round < 2; ++round)
            await Promise.all(migrants.map((client, index) => echo(client, 40000 + round * 10000 + index, index + 31)));
        await until(() => [spareB, spareC].reduce((count, item) => count +
            (item.text.match(/SERVICE_PROBE_ECHO /g) || []).length, 0) >= 24, 1000, 'Survivor execution logs');
        const distribution = [spareB, spareC].map(item => {
            const sizes = [...item.text.matchAll(/SERVICE_PROBE_ECHO pid=\d+ size=(\d+)/g)].map(match => Number(match[1]));
            return sizes.filter(size => size >= 40000 && size <= 40007).length;
        });
        assert.equal(distribution[0] + distribution[1], 8, 'Each new operation executes once');
        assert.ok(distribution.every(count => count > 0), 'Migration must use multiple survivors');
        assert.ok(distribution[0] + 3 <= 8 && distribution[1] + 1 <= 8, 'Existing and migrating clients respect capacity');
        assert.ok(Math.abs((distribution[0] + 3) - (distribution[1] + 1)) <= 2, 'Final occupancy should favor actual spare capacity');
        for (const item of [spareB, spareC]) {
            assert.doesNotMatch(item.text, /SERVICE_PROBE_STALLED /, 'Old held operations must not be replayed after recovery');
            assert.equal((item.text.match(/SERVICE_PROBE_ECHO /g) || []).length,
                (item === spareB ? 6 : 2) + 2 * distribution[item === spareB ? 0 : 1],
                'Probe execution count must match only explicitly issued operations');
        }
        for (const client of clients) client.dispose();
        await until(() => clients.every(client => client.released), 3000, 'All caller handles must quiesce');
        assert.equal(runtime.hosts.size, 0);
        for (const item of children) if (!expectedFaults.has(item)) fs.writeFileSync(path.join(output, item.name + '.stop'), '');
        const stopped = await Promise.all(children.map(item => item.completed));
        stopped.forEach((result, index) => { if (!expectedFaults.has(children[index])) assert.equal(result.code, 0, children[index].name); });
        return { fault, migrants: 8, retainedHealthy: 4,
            capacityEach: 8, initialSurvivorOccupancy: [3, 1], migratedToSurvivors: distribution,
            automaticRecoveryMs: recoveredMs, sameCallerObjects: true, learnedNodesFromSingleSeed: true,
            heartbeatFallbackWithoutTcpNotification: fault === 'silent-node-loss',
            newTlsGenerations: true, interruptedTerminals: terminals, noBusinessReplay: true,
            initialRetryBeforeHealthyBaseline: startupRetryCount, healthyGenerationsPreserved: true, status: 'PASS' };
    } finally {
        fs.writeFileSync(path.join(output, 'client-observations.json'), JSON.stringify(clients.map(client => ({
            generations: client.observed.generations.map(String), closed: client.observed.closed,
            beforeFault: client.observed.beforeFault, events: client.observed.events })), null, 2));
        clients.forEach(client => client.dispose());
        const end = performance.now() + 2000;
        while (clients.some(client => !client.released) && performance.now() < end) await delay(10);
        for (const item of children) if (item.child.exitCode === null && item.child.signalCode === null) item.child.kill();
        await Promise.all(children.map(item => item.completed.catch(() => {})));
        if (runtime) runtime.dispose();
    }
}

async function main() {
    const args = {};
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        const key = argument.slice(0, split);
        assert.ok(split > 0 && ['--server', '--module', '--output'].includes(key) && !args[key]);
        args[key] = argument.slice(split + 1);
    }
    for (const key of ['--server', '--module', '--output']) assert.ok(args[key]);
    const output = path.resolve(args['--output']);
    assert.ok(!fs.existsSync(output), 'Use a fresh owned batch directory');
    fs.mkdirSync(output, { recursive: true });
    let timer;
    const results = [];
    try {
        await Promise.race([(async () => {
            for (const fault of ['service-only', 'gateway-and-service', 'silent-node-loss']) {
                const record = await scenario(path.resolve(args['--server']), path.resolve(args['--module']),
                    path.join(output, fault), fault);
                results.push(record);
                console.log(JSON.stringify(record));
            }
        })(), new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Balanced failover batch deadline')), 190000); })]);
        fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ status: 'PASS', results,
            host: 'Node compiled NetPeer.wasm, real TCP/IPC; no real power outage or WeChat device' }, null, 2));
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ error: String(error.stack || error), results }, null, 2));
        throw error;
    } finally { clearTimeout(timer); }
}

main().catch(error => { console.error(error.stack || error); process.exitCode = 1; });
