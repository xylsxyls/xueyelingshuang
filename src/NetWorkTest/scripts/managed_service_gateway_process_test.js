'use strict';

// Public server SDK: client A -> product service -> child B -> internal service -> B -> A.
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
        const key = argument.slice(0, split);
        check(split > 0 && ['--server', '--module', '--output'].includes(key) && !Object.hasOwn(options, key), 'Unknown/repeated option');
        options[key] = argument.slice(split + 1);
    }
    for (const key of ['--server', '--module', '--output']) check(options[key], 'Missing ' + key);
    const output = path.resolve(options['--output']);
    check(!fs.existsSync(output), 'Use a fresh owned output directory');
    fs.mkdirSync(output, { recursive: true });
    const children = [];
    const clients = [];
    const events = [[], []];
    let eventError;
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
        const deadline = new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Managed service test deadline')), 18000); });
        await Promise.race([(async () => {
            const gateway = await server.ready;
            const internal = start(serverPath, ['--self-test-managed-internal=' + gateway[2], '--stop-file=' + path.join(output, 'internal.stop'),
                '--run-for-ms=20000'], 'internal', output, /MANAGED_PROBE_READY internal=1/);
            children.push(internal);
            await internal.ready;
            const caller = start(serverPath, ['--self-test-managed=' + gateway[2], '--stop-file=' + path.join(output, 'caller.stop'),
                '--run-for-ms=20000'], 'caller', output, /MANAGED_PROBE_READY internal=0/);
            children.push(caller);
            await caller.ready;
            for (let index = 0; index < 2; ++index) {
                const storage = new Map();
                const host = { createTCPSocket: () => new TcpSocketHost(), getPerformance: () => performance,
                    getStorageSync: key => storage.get(key), setStorageSync: (key, value) => storage.set(key, value) };
                const client = createWxClient(runtime, { protocol: 3, serviceId: 1, address: '127.0.0.1', port: Number(gateway[1]), onFrame: frame => {
                    try {
                        check(frame.kind === 5 && frame.product === 1 && frame.type === 0x7348 && frame.payload.length === 24,
                            'Unexpected routed internal event header');
                        check(Buffer.from(frame.payload.subarray(0, 8)).equals(Buffer.from([78, 86, 69, 50, 0, 255, 7, 9])) &&
                            Buffer.from(frame.payload.subarray(8)).equals(Buffer.from(client.deviceId)), 'Internal event crossed root/device or changed its raw NVE2 body');
                        events[index].push(frame.payload.slice());
                    } catch (error) { eventError = error; }
                    return true;
                } }, host);
                clients.push(client);
                await client.connect();
            }
            const bodies = [new Uint8Array(65537), new Uint8Array(262144)];
            bodies.forEach((body, client) => { for (let offset = 0; offset < body.length; ++offset) body[offset] = (offset * 43 + client * 91) & 255; });
            const initial = clients.map((client, index) => client.request(1, 0x7345, bodies[index], 7000));
            check(initial[0].requestId.join(':') === initial[1].requestId.join(':'), 'Did not exercise equal client request IDs');
            let results = await Promise.allSettled(initial);
            let readinessAttempts = 0;
            // Only the fixed idempotent echo fixture retries an explicit UNAVAILABLE result while the dependency starts.
            // The production client/server libraries never replay an accepted business request.
            while (results.some(result => result.status === 'rejected') && readinessAttempts < 8) {
                for (const result of results) if (result.status === 'rejected') check(result.reason.status === 1, 'Unexpected RPC failure: ' + result.reason);
                ++readinessAttempts;
                results = await Promise.allSettled(clients.map((client, index) => client.request(1, 0x7345, bodies[index], 7000)));
            }
            results.forEach((result, index) => check(result.status === 'fulfilled' && Buffer.from(result.value).equals(Buffer.from(bodies[index])),
                'Nested response changed or crossed clients: ' + (result.reason || index)));
            const waitEvents = async (index, count) => {
                const until = performance.now() + 3000;
                while (events[index].length < count && !eventError && performance.now() < until) await new Promise(resolve => setTimeout(resolve, 10));
                if (eventError) throw eventError;
                check(events[index].length >= count, 'Missing authenticated internal directed event for client ' + index);
            };
            await Promise.all([waitEvents(0, 1), waitEvents(1, 1)]);
            let proxyStressRequests = 0;
            for (let round = 0; round < 4; ++round) {
                const pending = [];
                for (let index = 0; index < 16; ++index) {
                    const body = Uint8Array.from({ length: 65537 + index }, (_, offset) => (offset * 17 + index + round * 19) & 255);
                    pending.push(clients[index % 2].request(1, 0x7345, body, 7000).then(actual => {
                        check(Buffer.from(actual).equals(Buffer.from(body)), 'Public proxy concurrent response changed or crossed roots');
                        ++proxyStressRequests;
                    }));
                }
                await Promise.all(pending);
            }
            clients[0].dispose();
            const previousEvents = events[1].length;
            const afterClose = await clients[1].request(1, 0x7345, new Uint8Array([3, 1, 4, 1, 5]), 5000);
            check(Buffer.from(afterClose).equals(Buffer.from([3, 1, 4, 1, 5])), 'Closing one root broke another root on the shared internal channel');
            await waitEvents(1, previousEvents + 1);
            for (const client of clients) client.dispose();
            fs.writeFileSync(path.join(output, 'caller.stop'), '');
            const stoppedCaller = await caller.completed;
            check(stoppedCaller.code === 0 && (stoppedCaller.stdout.match(/MANAGED_PROBE_ROUNDTRIP/g) || []).length >= 3,
                'Public NetSenderService caller failed or skipped the actual child path');
            fs.writeFileSync(path.join(output, 'internal.stop'), '');
            const stoppedInternal = await internal.completed;
            check(stoppedInternal.code === 0 && (stoppedInternal.stdout.match(/MANAGED_PROBE_INTERNAL/g) || []).length >= 3,
                'Internal service failed or did not receive nonzero parent RPCs');
            fs.writeFileSync(path.join(output, 'server.stop'), '');
            check((await server.completed).code === 0, 'Gateway failed normal shutdown');
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, clients: 2, instances: 1,
                sizes: bodies.map(body => body.length), sameOriginalRequestId: initial[0].requestId, readinessAttempts,
                publicServerSdk: 'NetSenderService.connectManaged + NetSenderServiceProxy', proxyStressRequests, independentGatewayAndTwoServiceProcesses: true,
                realChildRequestWithNonzeroForeignParent: true, rootContextAndResponseIdsPreserved: true,
                directedEventsWithRawNve2Payload: events.map(values => values.length),
                healthySecondRootAfterFirstRootClose: true, host: 'Production Wasm and Node TCPSocket surface; not WeChat GUI' }, null, 2));
        })(), deadline]);
        console.log('Public NetSenderService nested RPC + shared internal TLS + equal client IDs: PASS');
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