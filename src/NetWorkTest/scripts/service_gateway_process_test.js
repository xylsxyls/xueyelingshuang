'use strict';

// Production JS/Wasm + real TCP + independent gateway + two real IPC service processes.
// The host implements the TCPSocket callback surface; it is not a WeChat GUI run.
const fs = require('fs');
const path = require('path');
const net = require('net');
const crypto = require('crypto');
const { performance } = require('perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { createWxClient } = require('./host/NetWxClient');
const { NetWxGatewaySocket } = require('./host/NetWxGatewaySocket');
const { NetInterfaceMessage } = require('./host/NetInterfaceMessage');

function check(value, message) { if (!value) throw new Error(message); }

async function rejectForgedGrant(runtime, port) {
    await runtime.prepare();
    const socket = new NetWxGatewaySocket(runtime, { serviceId: 1, clock: () => performance.now(), createSocket: () => new TcpSocketHost() });
    let attacked = false;
    let timer;
    try {
        return await new Promise((resolve, reject) => {
            timer = setTimeout(() => reject(new Error('Gateway did not reject an unsolicited credit grant')), 5000);
            socket.onError(() => resolve(attacked));
            socket.onClose(() => resolve(attacked));
            socket.onMessage(() => reject(new Error('Service sent business ciphertext before any TLS ClientHello')));
            socket.onConnect(() => {
                const message = new NetInterfaceMessage(runtime);
                try {
                    // Neither endpoint has sent any business TLS byte. There is no outstanding byte to grant.
                    const channel = new Uint8Array(8);
                    channel[0] = 1;
                    const amount = new Uint8Array(4);
                    amount[0] = 1;
                    message.setValue(0, 9, 2, channel);
                    message.setValue(0, 14, 1, amount);
                    attacked = socket.link.sendControl([5, 0, 0, 0, 0x4e470005], message.encode());
                    check(attacked, 'Forged credit fixture was not sent');
                    socket.schedule(0);
                } catch (error) { reject(error); }
                finally { message.dispose(); }
            });
            socket.connect({ address: '127.0.0.1', port });
        });
    } finally { clearTimeout(timer); socket.close(); }
}

class TcpSocketHost {
    constructor() { this.listeners = {}; this.socket = null; }
    onConnect(callback) { this.listeners.connect = callback; }
    onMessage(callback) { this.listeners.message = callback; }
    onError(callback) { this.listeners.error = callback; }
    onClose(callback) { this.listeners.close = callback; }
    offConnect() { delete this.listeners.connect; }
    offMessage() { delete this.listeners.message; }
    offError() { delete this.listeners.error; }
    offClose() { delete this.listeners.close; }
    connect(options) {
        this.socket = net.createConnection({ host: options.address, port: options.port });
        this.socket.on('connect', () => { if (this.listeners.connect) this.listeners.connect(); });
        this.socket.on('data', bytes => { if (this.listeners.message) this.listeners.message({ message: Uint8Array.from(bytes).buffer }); });
        this.socket.on('error', error => { if (this.listeners.error) this.listeners.error({ errMsg: error.message }); });
        this.socket.on('close', () => { if (this.listeners.close) this.listeners.close(); });
    }
    write(bytes) { this.socket.write(Buffer.from(bytes)); }
    close() { if (this.socket) this.socket.destroy(); }
}

async function main() {
    const options = {};
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        check(split > 0, 'Expected --name=value');
        options[argument.slice(0, split)] = argument.slice(split + 1);
    }
    for (const key of ['--server', '--module', '--output']) check(options[key], 'Missing ' + key);
    check(Boolean(options['--client']) === Boolean(options['--test']), 'Supply both --client and --test for the desktop path');
    const output = path.resolve(options['--output']);
    check(!fs.existsSync(output), 'Use a fresh owned output directory');
    fs.mkdirSync(output, { recursive: true });
    const children = [];
    const clients = [];
    let timer;
    try {
        const modulePath = path.resolve(options['--module']);
        const runtime = await NetInterfaceRuntime.load(require(modulePath), {
            randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size))),
            moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) }
        });
        const serverPath = path.resolve(options['--server']);
        const serverStop = path.join(output, 'server.stop');
        const server = start(serverPath, ['--protocol=3', '--listen=127.0.0.1', '--port=0',
            '--identity-directory=' + path.join(output, 'identity'), '--stop-file=' + serverStop, '--run-for-ms=20000'],
            'server', output, /GATEWAY_READY port=(\d+) pid=(\d+)/);
        children.push(server);
        const deadline = new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Service process test deadline')), 18000); });
        let evidence;
        await Promise.race([(async () => {
            const serverReady = await server.ready;
            const port = Number(serverReady[1]);
            const services = [];
            for (let index = 0; index < 2; ++index) {
                const stop = path.join(output, 'service-' + index + '.stop');
                const service = start(serverPath, ['--self-test-service=' + serverReady[2], '--stop-file=' + stop, '--run-for-ms=20000'],
                    'service-' + index, output, /SERVICE_PROBE_READY pid=(\d+)/);
                service.stop = stop;
                services.push(service);
                children.push(service);
                await service.ready;
            }
            const storage = new Map();
            const wxHost = { createTCPSocket: () => new TcpSocketHost(), getPerformance: () => performance,
                getStorageSync: key => storage.get(key), setStorageSync: (key, value) => storage.set(key, value) };
            for (let index = 0; index < 2; ++index) {
                const client = createWxClient(runtime, { protocol: 3, serviceId: 1, address: '127.0.0.1', port }, wxHost);
                clients.push(client);
                await client.connect();
            }
            let desktop;
            let clientGateway;
            let clientStop;
            if (options['--client']) {
                clientStop = path.join(output, 'client.stop');
                clientGateway = start(path.resolve(options['--client']), ['--protocol=3', '--address=127.0.0.1', '--port=' + port,
                    '--cache-file=' + path.join(output, 'client-nodes.cache'),
                    '--stop-file=' + clientStop, '--run-for-ms=18000'], 'client', output, /CLIENT_GATEWAY_READY pid=(\d+)/);
                children.push(clientGateway);
                const connected = await clientGateway.ready;
                desktop = start(path.resolve(options['--test']), ['--service-client-test=' + connected[1]], 'desktop-endpoint', output,
                    /DESKTOP_SERVICE_CLIENT_PASS/);
                children.push(desktop);
            }
            const payloads = [new Uint8Array(262144), new Uint8Array(65537)];
            for (let index = 0; index < payloads.length; ++index) {
                for (let offset = 0; offset < payloads[index].length; ++offset) payloads[index][offset] = (offset * 13 + index * 79) & 255;
            }
            const requests = clients.map((client, index) => client.request(1, 0x7345, payloads[index], 10000));
            check(requests[0].requestId.join(':') === requests[1].requestId.join(':'), 'Test did not exercise equal per-client request IDs');
            const replies = await Promise.all(requests);
            for (let index = 0; index < replies.length; ++index) {
                check(Buffer.from(replies[index]).equals(Buffer.from(payloads[index])), 'End-to-end RPC payload crossed clients or changed');
            }
            check(await rejectForgedGrant(runtime, port), 'The bad connection ended before the forged grant was sent');
            const healthy = await clients[0].request(1, 0x7345, new Uint8Array([0, 255, 7]), 5000);
            check(Buffer.from(healthy).equals(Buffer.from([0, 255, 7])), 'Bad-client grant tore down a healthy service/client');
            if (desktop) {
                const tested = await desktop.completed;
                check(tested.code === 0 && /DESKTOP_SERVICE_CLIENT_PASS/.test(tested.stdout), 'Native public NetSender service client failed');
                fs.writeFileSync(clientStop, '');
                check((await clientGateway.completed).code === 0, 'Client gateway did not stop normally');
            }
            for (const client of clients) client.dispose();
            for (const service of services) fs.writeFileSync(service.stop, '');
            const stopped = await Promise.all(services.map(service => service.completed));
            check(stopped.every(value => value.code === 0 && !value.signal && /SERVICE_PROBE_ECHO/.test(value.stdout)),
                'Both registered instances did not serve a real request or failed shutdown');
            fs.writeFileSync(serverStop, '');
            const ended = await server.completed;
            check(ended.code === 0 && !ended.signal, 'Gateway did not stop normally');
            evidence = { pass: true, instances: 2, simultaneousClients: 2, sameRequestIds: requests[0].requestId,
                sizes: payloads.map(bytes => bytes.length), protocol: 'production Wasm management TLS + endpoint TLS + TCP + ProcessWork',
                host: 'Node TCPSocket callback adapter; not WeChat GUI', independentInstancesBothServed: true,
                nativePublicNetSenderThroughClientGateway: Boolean(desktop), nativePayloadSize: desktop ? 65537 : 0 };
            evidence.forgedGrantDisconnected = true;
            evidence.healthyClientAfterBadPeer = true;
        })(), deadline]);
        fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify(evidence, null, 2));
        console.log('Wasm endpoint TLS -> actual gateway -> two ProcessWork service instances: PASS');
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

module.exports = { TcpSocketHost };
if (require.main === module) main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });