'use strict';

// Two real authenticated gateways. Eight pinned A->B routes and a reverse B->A route share the same node connection.
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { performance } = require('perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { createWxClient } = require('./host/NetWxClient');
const { NetWxGatewaySocket } = require('./host/NetWxGatewaySocket');
const { NetInterfaceMessage } = require('./host/NetInterfaceMessage');
const { NetInterfaceDirectory } = require('./host/NetInterfaceDirectory');

function check(value, message) { if (!value) throw new Error(message); }
const delay = milliseconds => new Promise(resolve => setTimeout(resolve, milliseconds));

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
    const rosters = [];
    let timer;
    try {
        const modulePath = path.resolve(options['--module']);
        const runtime = await NetInterfaceRuntime.load(require(modulePath), {
            randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size))),
            moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) }
        });
        const serverPath = path.resolve(options['--server']);
        const gateway = (name, seed, identityName = name) => {
            const entry = start(serverPath, ['--protocol=3', '--listen=127.0.0.1', '--port=0',
                '--identity-directory=' + path.join(output, identityName + '-identity'), '--stop-file=' + path.join(output, name + '.stop'),
                '--run-for-ms=60000', ...(seed ? ['--seeds=127.0.0.1:' + seed] : [])], name, output, /GATEWAY_READY port=(\d+) pid=(\d+)/);
            children.push(entry);
            return entry;
        };
        const service = (name, pid) => {
            const entry = start(serverPath, ['--self-test-service=' + pid, '--stop-file=' + path.join(output, name + '.stop'), '--run-for-ms=60000'],
                name, output, /SERVICE_PROBE_READY pid=(\d+)/);
            children.push(entry);
            return entry;
        };
        const capture = bytes => {
            const message = new NetInterfaceMessage(runtime);
            try {
                message.parse(bytes);
                const count = message.getValue(0, 1);
                check(count && count.type === 1 && count.bytes.length === 4, 'Malformed directory count');
                const size = new DataView(count.bytes.buffer, count.bytes.byteOffset, 4).getInt32(0, true);
                const members = [];
                for (let group = 1; group <= size; ++group) {
                    const name = message.getValue(group, 2);
                    const port = message.getValue(group, 4);
                    const boot = message.getValue(group, 7);
                    check(name && port && boot && boot.bytes.some(value => value !== 0), 'Missing identity/port/boot in directory');
                    members.push({ name: Buffer.from(name.bytes).toString('utf8'),
                        port: new DataView(port.bytes.buffer, port.bytes.byteOffset, 4).getInt32(0, true) });
                }
                rosters.push(members);
            } finally { message.dispose(); }
        };
        const connect = async port => {
            const until = performance.now() + 10000;
            while (performance.now() < until) {
                const storage = new Map();
                const host = { createTCPSocket: () => new TcpSocketHost(), getPerformance: () => performance,
                    getStorageSync: key => storage.get(key), setStorageSync: (key, value) => storage.set(key, value) };
                const client = createWxClient(runtime, { protocol: 3, autoReconnect: false, serviceId: 1, address: '127.0.0.1', port, onNodes: capture }, host);
                try { await client.connect(); clients.push(client); return client; }
                catch (error) {
                    client.dispose();
                    if (error.code !== -8 && error.code !== -3) throw error;
                    await delay(40); // Explicit connection setup retry; no business request has been sent.
                }
            }
            throw new Error('No ready exact service instance discovered');
        };
        const echo = async (client, size, salt) => {
            const bytes = new Uint8Array(size);
            for (let index = 0; index < bytes.length; ++index) bytes[index] = (index * 19 + salt) & 255;
            const response = await client.request(1, 0x7345, bytes, 10000);
            check(Buffer.from(response).equals(Buffer.from(bytes)), 'Cross-node opaque RPC changed or crossed client payloads');
        };
        const refusePublicMesh = async port => {
            const socket = new NetWxGatewaySocket(runtime, { serviceId: 1, clock: () => performance.now(), createSocket: () => new TcpSocketHost() });
            let timeout;
            try {
                await new Promise((resolve, reject) => {
                    timeout = setTimeout(() => reject(new Error('Public MESH_OPEN was not explicitly rejected')), 5000);
                    const original = socket.management.bind(socket);
                    socket.management = message => {
                        if (message.fields[0] === 4 && message.fields[2] === 3 && message.fields[4] === 0x4e470008) {
                            const proto = new NetInterfaceMessage(runtime);
                            try { proto.parse(message.bytes); check(proto.getCode().code === -11, 'Public MESH_OPEN did not return NOT_SUPPORTED'); resolve(); }
                            catch (error) { reject(error); }
                            finally { proto.dispose(); }
                        } else original(message);
                    };
                    socket.onError(event => reject(new Error(event.errMsg)));
                    socket.onConnect(() => {
                        try {
                            check(socket.link.sendControl([3, 0, 3, 0, 0x4e470008], new Uint8Array(0)), 'Attack fixture could not send MESH_OPEN');
                            socket.schedule(0);
                        } catch (error) { reject(error); }
                    });
                    socket.connect({ address: '127.0.0.1', port });
                });
            } finally { clearTimeout(timeout); socket.close(); }
        };
        const deadline = new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Mesh process test deadline')), 55000); });
        await Promise.race([(async () => {
            const nodeA = gateway('node-a');
            const a = await nodeA.ready;
            const nodeB = gateway('node-b', a[1]);
            const b = await nodeB.ready;
            const serviceB = service('service-b', b[2]);
            await serviceB.ready;
            const held = [];
            for (let index = 0; index < 8; ++index) held.push(await connect(Number(a[1])));
            check(rosters.some(members => members.length === 2 && members.some(member => member.port === Number(a[1])) &&
                members.some(member => member.port === Number(b[1]))), 'A did not discover the authenticated B member');
            await Promise.all(held.map((client, index) => echo(client, index === 0 ? 262144 : 65537 + index, index * 23)));
            const serviceA = service('service-a', a[2]);
            await serviceA.ready;
            const reverse = await connect(Number(b[1])); // B's eight local slots are occupied; must use A.
            await echo(reverse, 78644, 71);
            await refusePublicMesh(Number(a[1]));
            await echo(held[0], 65537, 137);
            await echo(reverse, 65539, 149);
            serviceB.child.kill(); // Fault injection only into our owned service process.
            await serviceB.completed;
            const closedBy = performance.now() + 8000;
            while (held.some(client => !client.disposed) && performance.now() < closedBy) await delay(10);
            check(held.every(client => client.disposed), 'Remote backend failure did not terminate all pinned old client generations');
            await echo(reverse, 65541, 157);
            const fresh = await connect(Number(a[1]));
            await echo(fresh, 65543, 163);
            for (const client of clients) client.dispose();
            fs.writeFileSync(path.join(output, 'service-a.stop'), '');
            const endedA = await serviceA.completed;
            check(endedA.code === 0 && (endedA.stdout.match(/SERVICE_PROBE_ECHO/g) || []).length >= 4, 'A did not serve reverse/new healthy generations');
            const cacheA = path.join(output, 'node-a-identity', 'nodes.cache');
            const cachedBy = performance.now() + 5000;
            let liveCache;
            while (performance.now() < cachedBy) {
                if (fs.existsSync(cacheA)) {
                    const bytes = Uint8Array.from(fs.readFileSync(cacheA));
                    const directory = new NetInterfaceDirectory(runtime, bytes, true);
                    try {
                        const members = directory.points();
                        if (members.some(member => member.port === Number(a[1])) && members.some(member => member.port === Number(b[1]))) {
                            liveCache = bytes; break;
                        }
                    } finally { directory.dispose(); }
                }
                await delay(10);
            }
            check(liveCache, 'A did not persist both live authenticated nodes before restart');
            // Restart deliberately replaces the same live cache; retain the validated pre-restart fixture for interoperability.
            fs.writeFileSync(path.join(output, 'nodes-live.cache'), liveCache);
            fs.writeFileSync(path.join(output, 'node-a.stop'), '');
            check((await nodeA.completed).code === 0, 'A did not stop before cache-only restart');
            const cacheService = service('cache-service-b', b[2]);
            await cacheService.ready;
            const restartedA = gateway('node-a-restarted', null, 'node-a');
            const restarted = await restartedA.ready;
            const fromCache = await connect(Number(restarted[1]));
            await echo(fromCache, 65547, 181);
            fromCache.dispose();
            const corruptDirectory = path.join(output, 'node-corrupt-identity');
            fs.mkdirSync(corruptDirectory);
            fs.writeFileSync(path.join(corruptDirectory, 'nodes.cache'), Buffer.from('NGC3-corrupt-cache'));
            const corrupt = gateway('node-corrupt', b[1]);
            const corruptReady = await corrupt.ready;
            const fallback = await connect(Number(corruptReady[1]));
            await echo(fallback, 65549, 191);
            fallback.dispose();
            fs.writeFileSync(path.join(output, 'cache-service-b.stop'), '');
            check((await cacheService.completed).code === 0, 'Cache-backed service did not stop normally');
            fs.writeFileSync(path.join(output, 'node-corrupt.stop'), '');
            fs.writeFileSync(path.join(output, 'node-a-restarted.stop'), '');
            fs.writeFileSync(path.join(output, 'node-b.stop'), '');
            const ended = await Promise.all([restartedA.completed, nodeB.completed, corrupt.completed]);
            check(ended.every(result => result.code === 0), 'Healthy gateways did not stop normally');
            fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, nodes: 2, pinnedForwardClients: 8,
                reverseOnSameNodeLink: true, publicMeshOpenDenied: true, remoteServiceCrashClosedOldGenerations: true,
                healthyReverseAfterFault: true, explicitlyCreatedFreshTlsClientAfterFault: true, noBusinessReplay: true,
                gatewayRestartDiscoversServiceUsingOnlyPersistedCache: true, corruptCacheFallsBackToAuthenticatedSeed: true,
                roster: rosters.find(members => members.length === 2), host: 'Production Wasm and real TCP; not WeChat GUI' }, null, 2));
        })(), deadline]);
        console.log('Authenticated two-node mesh, both directions, exact instance, role boundary and backend failure: PASS');
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