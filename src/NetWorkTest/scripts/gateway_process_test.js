'use strict';

// Production NetServerManager executable, production Wasm and a real loopback TCP
// connection. This does not claim that WXWebAssembly has run inside WeChat.
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');
const net = require('node:net');
const tls = require('node:tls');
const { spawn } = require('node:child_process');
const { performance } = require('node:perf_hooks');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { NetWxClient } = require('./host/NetWxClient');
const { RealTcpAdapter } = require('./tls_interop_test');

async function bounded(promise, ms, label) {
    let timer;
    try {
        return await Promise.race([promise, new Promise((resolve, reject) => {
            timer = setTimeout(() => reject(new Error(label + ' timeout')), ms);
        })]);
    } finally { clearTimeout(timer); }
}

async function main() {
    const args = {};
    for (const argument of process.argv.slice(2)) {
        const separator = argument.indexOf('=');
        const key = separator < 0 ? argument : argument.slice(0, separator);
        if (!['--module', '--executable', '--report', '--cases', '--instant'].includes(key) ||
            Object.hasOwn(args, key) || (key === '--instant' ? separator >= 0 : separator < 0 ||
            !argument.slice(separator + 1))) throw new Error('Invalid argument: ' + argument);
        args[key] = separator < 0 ? true : argument.slice(separator + 1);
    }
    if (Boolean(args['--instant']) === Boolean(args['--cases']) ||
        ['--module', '--executable', '--report'].some(key => !path.isAbsolute(args[key] || ''))) {
        throw new Error('Required absolute --module, --executable, --report and --instant or --cases=301,302');
    }
    if (args['--cases'] && !/^[1-9][0-9]*(,[1-9][0-9]*)*$/.test(args['--cases'])) throw new Error('Invalid case list');
    const ids = args['--cases'] ? args['--cases'].split(',').map(Number) : [301, 302, 303, 304, 305];
    if (new Set(ids).size !== ids.length || ids.some(id => id < 301 || id > 305)) throw new Error('Unknown or duplicate case');
    const directory = path.dirname(args['--report']);
    fs.mkdirSync(directory, { recursive: true });
    const stopFile = path.join(directory, `gateway-stop-${crypto.randomBytes(8).toString('hex')}.txt`);
    const root = path.resolve(__dirname, '../../..');
    // Mbed TLS 3.6.7 framework test certificates; independent of SDK build caches.
    const certificates = path.join(root, 'src/NetWorkTest/testdata/tls');
    const authorityPem = fs.readFileSync(path.join(certificates, 'test-ca-sha256.crt'), 'ascii');
    const runtime = await NetInterfaceRuntime.load(require(args['--module']), {
        moduleOptions: { wasmBinary: require('./host/read_wasm').readWasm(args['--module']) },
        randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size)))
    });
    const child = spawn(args['--executable'], [
        '--protocol=2', '--listen=127.0.0.1', '--port=0', '--run-for-ms=60000', `--stop-file=${stopFile}`,
        '--certificate=' + path.join(certificates, 'server2-sha256.crt'),
        '--private-key=' + path.join(certificates, 'server2.key')
    ], { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'], cwd: path.dirname(args['--executable']) });
    let output = '';
    let errors = '';
    let resolveReady;
    let rejectReady;
    const ready = new Promise((resolve, reject) => { resolveReady = resolve; rejectReady = reject; });
    child.stdout.on('data', bytes => {
        output += bytes.toString();
        const match = /GATEWAY_READY port=(\d+) pid=(\d+)/.exec(output);
        if (match) resolveReady({ port: Number(match[1]), pid: Number(match[2]) });
    });
    child.stderr.on('data', bytes => { errors += bytes.toString(); });
    const exited = new Promise((resolve, reject) => {
        child.on('error', error => { rejectReady(error); reject(error); });
        child.on('exit', (code, signal) => {
            rejectReady(new Error(`gateway exited ${code} ${signal}: ${errors}`));
            resolve({ code, signal });
        });
    });
    // Both promises have handlers before any possible child event.
    exited.catch(() => {});
    const clients = new Set();
    const sockets = new Set();
    const report = { executable: args['--executable'], module: args['--module'],
        engine: 'production Windows gateway + production Wasm + real TCP', cases: [] };
    let cancelled = false;
    const cancel = () => { cancelled = true; fs.writeFileSync(stopFile, 'cancel'); };
    process.on('SIGINT', cancel);
    process.on('SIGTERM', cancel);
    try {
        const listening = await bounded(ready, 10000, 'gateway startup');
        assert.equal(listening.pid, child.pid);
        report.pid = child.pid;
        report.port = listening.port;
        const options = { runtime, authorityPem, hostname: 'localhost', address: '127.0.0.1',
            port: listening.port, clock: () => performance.now(), deviceId: Uint8Array.from(crypto.randomBytes(16)),
            createSocket: () => new RealTcpAdapter() };
        const client = overrides => {
            const connection = new NetWxClient(Object.assign({}, options, overrides));
            clients.add(connection);
            return connection;
        };
        const cases = [
            [301, 'Wasm connects to actual gateway and receives explicit unavailable RPC status', async () => {
                const connection = client();
                await connection.connect();
                assert.equal(connection.negotiated.maxRequests, 32);
                assert.equal(connection.negotiated.capabilities, 7);
                const calls = Array.from({ length: 32 }, () => connection.request(4, 700, Uint8Array.of(0, 255), 3000));
                const results = await Promise.allSettled(calls);
                assert(results.every(result => result.status === 'rejected' && result.reason.status === 1));
                await assert.rejects(connection.request(4, 700, new Uint8Array(1048560), 3000), error => error.status === 1);
                assert.equal(connection.requests.count(), 0);
                assert.equal(connection.ready, true);
                connection.dispose();
            }],
            [302, 'Public client cannot forge internal Account parent route', async () => {
                let closed;
                const close = new Promise(resolve => { closed = resolve; });
                const connection = client({ onClose: closed });
                await connection.connect();
                const payload = Uint8Array.of(78, 82, 80, 50, 0, 0, 3, 232, 0, 0, 0, 0, 0, 0, 0, 1);
                connection.sendFrame([3, 4, 1, 0, 900], payload);
                await bounded(close, 2000, 'forged internal request rejection');
                assert.equal(connection.disposed, true);
            }],
            [303, 'Actual gateway enforces application handshake deadline', async () => {
                const socket = tls.connect({ host: '127.0.0.1', port: listening.port, servername: 'localhost',
                    ca: authorityPem, minVersion: 'TLSv1.2', maxVersion: 'TLSv1.2' });
                sockets.add(socket);
                socket.on('error', () => {});
                await bounded(new Promise((resolve, reject) => {
                    socket.once('secureConnect', resolve); socket.once('error', reject);
                }), 3000, 'independent TLS connect');
                const started = performance.now();
                await bounded(new Promise(resolve => socket.once('close', resolve)), 7000, 'application deadline');
                assert(performance.now() - started >= 4500);
            }],
            [304, 'TLS listener rejects legacy plaintext without downgrade', async () => {
                const socket = net.createConnection({ host: '127.0.0.1', port: listening.port });
                sockets.add(socket);
                let received = 0;
                socket.on('data', bytes => { received += bytes.length; });
                socket.on('error', () => {});
                socket.on('connect', () => socket.write('NetManager Login'));
                await bounded(new Promise(resolve => socket.once('close', resolve)), 3000, 'plaintext rejection');
                assert.equal(received, 0);
            }],
            [305, 'Owned stop file drains CTask and exits production gateway cleanly', async () => {
                fs.writeFileSync(stopFile, 'stop');
                const result = await bounded(exited, 3000, 'gateway graceful shutdown');
                assert.equal(result.code, 0);
                assert.match(output, /GATEWAY_STOPPED/);
            }]
        ];
        for (const [id, name, run] of cases.filter(entry => ids.includes(entry[0]))) {
            const started = performance.now();
            const result = { id, name, state: cancelled ? 'NOT_RUN' : 'PASS' };
            if (!cancelled) {
                try { await run(); } catch (error) { result.state = 'FAIL'; result.error = error.stack; }
            }
            result.durationMs = performance.now() - started;
            report.cases.push(result);
            fs.writeFileSync(args['--report'], JSON.stringify(report, null, 2));
            console.log(id, name, result.state, result.error || '');
        }
    } finally {
        clients.forEach(connection => connection.dispose());
        sockets.forEach(socket => socket.destroy());
        fs.writeFileSync(stopFile, 'stop');
        try { report.exit = await bounded(exited, 3000, 'cleanup'); }
        catch (error) {
            // Only the child created by this runner may be terminated.
            child.kill();
            report.cleanupError = error.message;
        }
        process.off('SIGINT', cancel);
        process.off('SIGTERM', cancel);
        report.stdout = output;
        report.stderr = errors;
        report.cancelled = cancelled;
        report.passed = report.cases.filter(entry => entry.state === 'PASS').length;
        report.failed = report.cases.filter(entry => entry.state === 'FAIL').length;
        report.notRun = report.cases.filter(entry => entry.state === 'NOT_RUN').length;
        fs.writeFileSync(args['--report'], JSON.stringify(report, null, 2));
    }
    process.exitCode = report.failed || report.notRun || report.cleanupError ? 1 : 0;
}

main().catch(error => { console.error(error.stack); process.exitCode = 2; });