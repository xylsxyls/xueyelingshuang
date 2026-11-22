'use strict';

// 用Node实现wx的原始Socket/随机/时钟宿主，运行同一份小程序App；不冒充官方微信实机。
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const net = require('node:net');
const tls = require('node:tls');
const vm = require('node:vm');
const crypto = require('node:crypto');
const { performance } = require('node:perf_hooks');
const { readWasm } = require('./read_wasm');
const { attachApplicationEcho } = require('./application_echo');
const NetTestDeadline = require('./NetTestDeadline');

async function main() {
    const [entry, output] = process.argv.slice(2);
    assert.equal(process.argv.length, 4);
    assert.ok(path.isAbsolute(entry) && path.isAbsolute(output) && !fs.existsSync(output));
    const factory = require(entry), certs = __dirname;
    const sockets = new Set(), timers = new Set(), peers = new Set();
    const evidence = { scope: 'Node wx host simulation; official device NOT_RUN', bits: factory.sdk.bits,
        tlsConnections: 0, bytes: 0, requests: 0, moduleLoads: 0, clockCalls: 0 };
    const deadline = new NetTestDeadline(30000, 'same wx sample application');
    const server = tls.createServer({ cert: fs.readFileSync(path.join(certs, 'NetTestTlsServer.crt')),
        key: fs.readFileSync(path.join(certs, 'NetTestTlsServer.key')), minVersion: 'TLSv1.2', maxVersion: 'TLSv1.2' }, socket => {
        peers.add(socket); ++evidence.tlsConnections;
        socket.on('close', () => peers.delete(socket)); socket.on('error', () => {});
        attachApplicationEcho(socket, { mode: 'rpc', onHello: hello => { evidence.hello = hello; },
            onRpc: () => { ++evidence.requests; }, onApplicationBytes: count => { evidence.bytes += count; } });
    });
    server.on('tlsClientError', error => { evidence.peerError = error.message; });
    let app;
    try {
        await deadline.wait(new Promise((resolve, reject) => { server.once('error', reject); server.listen(0, '127.0.0.1', resolve); }));
        const config = { address: '127.0.0.1', port: server.address().port,
            authorityPem: fs.readFileSync(path.join(certs, 'NetTestTlsAuthority.crt'), 'ascii') };
        const wx = {
            getPerformance: () => ({ now() { ++evidence.clockCalls; return performance.now(); } }),
            getSystemInfoSync: () => ({ testHost: 'Node', officialDevice: false }),
            getRandomValues(options) { options.success({ randomValues: crypto.randomFillSync(new Uint8Array(options.length)).buffer }); },
            createTCPSocket() {
                deadline.check();
                const callbacks = {}, socket = new net.Socket(); sockets.add(socket);
                socket.on('connect', () => { if (callbacks.Connect) callbacks.Connect(); });
                socket.on('data', bytes => { if (callbacks.Message) callbacks.Message({ message: Uint8Array.from(bytes).buffer }); });
                socket.on('error', error => { if (callbacks.Error) callbacks.Error(error); });
                socket.on('close', () => { sockets.delete(socket); if (callbacks.Close) callbacks.Close(); });
                const adapter = {
                    connect(options) { socket.connect(options.port, options.address); },
                    write(bytes) { assert.ok(bytes instanceof ArrayBuffer); socket.write(Buffer.from(bytes)); },
                    close() { socket.destroy(); }
                };
                for (const name of ['Connect', 'Message', 'Error', 'Close']) {
                    adapter['on' + name] = callback => { callbacks[name] = callback; };
                    adapter['off' + name] = callback => { if (callbacks[name] === callback) delete callbacks[name]; };
                }
                return adapter;
            }
        };
        const sandbox = {
            wx, Uint8Array, ArrayBuffer, console,
            App(value) { assert.equal(app, undefined); app = value; },
            require(name) {
                if (name === './test-config') return config;
                if (name === './net/NetPeerAPI') return factory;
                throw new Error('Unexpected sample dependency: ' + name);
            },
            WXWebAssembly: Object.assign({}, WebAssembly, {
                async instantiate(filename, imports) {
                    assert.equal(filename, '/net/' + factory.sdk.wasm); ++evidence.moduleLoads;
                    return WebAssembly.instantiate(readWasm(entry), imports);
                }
            }),
            setTimeout(callback, milliseconds) {
                const timer = setTimeout(() => { timers.delete(timer); callback(); }, milliseconds);
                timers.add(timer); return timer;
            },
            clearTimeout(timer) { timers.delete(timer); clearTimeout(timer); }
        };
        // WebAssembly的内建属性不可枚举，显式保留加载器可能查询的类型；禁止给VM补全全局时钟。
        for (const name of ['Memory', 'Table', 'Module', 'Instance', 'RuntimeError', 'CompileError']) {
            sandbox.WXWebAssembly[name] = WebAssembly[name];
        }
        vm.runInNewContext(fs.readFileSync(path.join(__dirname, 'NetWxSampleApp.js'), 'utf8'), sandbox, { filename: 'NetWxSampleApp.js' });
        await deadline.wait(app.runNetGatewayTest());
        evidence.app = app.globalData.netInterfaceReport;
        assert.equal(evidence.app.state, 'PASS', JSON.stringify(evidence.app));
        assert.deepEqual(Array.from(evidence.app.cases, item => [item.id, item.state]), [[400, 'PASS'], [401, 'PASS']]);
        assert.equal(evidence.tlsConnections, 1); assert.equal(evidence.requests, 1); assert.equal(evidence.bytes, 65584);
        assert.equal(evidence.hello.platform, 3); assert.equal(evidence.moduleLoads, 1); assert.ok(evidence.clockCalls > 0);
        assert.equal(sandbox.performance, undefined); assert.equal(timers.size, 0);
        evidence.pass = true;
    } finally {
        deadline.close();
        for (const timer of timers) clearTimeout(timer);
        for (const socket of sockets) socket.destroy();
        for (const socket of peers) socket.destroy();
        if (server.listening) await new Promise(resolve => server.close(resolve));
    }
    fs.writeFileSync(output, JSON.stringify(evidence, null, 2), { flag: 'wx' });
    console.log(JSON.stringify(evidence));
}

main().catch(error => { console.error(error.stack || error); process.exitCode = 1; });