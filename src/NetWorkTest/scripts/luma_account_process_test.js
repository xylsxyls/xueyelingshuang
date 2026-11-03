'use strict';
const { createMysqlTestConfig } = require('./account_service_process_test');

// Real independent services, optional actual Qt title/dialog automation, and shared production Wasm.
const fs = require('fs');
const path = require('path');
const assert = require('assert').strict;
const crypto = require('crypto');
const { performance } = require('perf_hooks');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetInterfaceRuntime } = require('./host/NetInterfaceRuntime');
const { NetInterfaceMessage } = require('./host/NetInterfaceMessage');
const { createServiceClient } = require('./host/NetWxClient');
const { NetAccountClient } = require('./host/NetAccountClient');
const A = require('../../../include/Account/AccountProtocol');
const delay = value => new Promise(resolve => setTimeout(resolve, value));

async function main() {
    const options = {};
    for (const value of process.argv.slice(2)) {
        const split = value.indexOf('=');
        const name = value.slice(0, split);
        assert.ok(split > 0 && ['--bin', '--module', '--account-module', '--output', '--gui-test', '--qt', '--mysql-config'].includes(name) && !options[name]);
        options[name] = value.slice(split + 1);
    }
    for (const key of ['--bin', '--module', '--account-module', '--output']) assert.ok(options[key], key);
    const bin = path.resolve(options['--bin']);
    const output = path.resolve(options['--output']);
    assert.ok(!fs.existsSync(output));
    fs.mkdirSync(output, { recursive: true });
    const children = [];
    const run = (binary, args, name, pattern) => {
        const child = start(binary, args, name, output, pattern);
        children.push(child);
        return child;
    };
    let timer;
    let client;
    let account;
    try {
        const deadline = new Promise((resolve, reject) => { timer = setTimeout(() => reject(new Error('Luma account deadline')), 75000); });
        await Promise.race([(async () => {
            const gateway = run(path.join(bin, 'NetServerManager1.1.exe'), ['--protocol=3', '--listen=127.0.0.1', '--port=0',
                '--identity-directory=' + path.join(output, 'identity'), '--stop-file=' + path.join(output, 'gateway.stop'),
                '--run-for-ms=80000'], 'gateway', /GATEWAY_READY port=(\d+) pid=(\d+)/);
            const ready = await gateway.ready;
            const server = run(path.join(bin, 'AccountServer.exe'), ['--protocol=3', '--gateway-pid=' + ready[2],
                '--database=' + createMysqlTestConfig(options['--mysql-config'], path.join(output, 'AccountServer.ini')), '--math-vip=1', '--stop-file=' + path.join(output, 'account.stop'),
                '--run-for-ms=80000'], 'account', /ACCOUNT_READY endpoint=/);
            await server.ready;
            const luma = run(path.join(bin, 'LumaPlayerServer.exe'), ['--gateway-pid=' + ready[2],
                '--stop-file=' + path.join(output, 'luma.stop'), '--run-for-ms=80000'], 'luma', /LUMA_SERVICE_READY endpoint=/);
            await luma.ready;
            const modulePath = path.resolve(options['--module']);
            const runtime = await NetInterfaceRuntime.load(require(modulePath), {
                randomSource: size => Promise.resolve(Uint8Array.from(crypto.randomBytes(size))),
                moduleOptions: { noInitialRun: true, wasmBinary: require('./host/read_wasm').readWasm(modulePath) }
            });
            const accountRuntime = await require('../../NetWorkTest/scripts/host/read_wasm').loadAccountForTest(runtime, options['--account-module']);
            account = new NetAccountClient(accountRuntime, 101);
            client = createServiceClient({ runtime, protocol: 3, autoReconnect: false, serviceId: 101,
                address: '127.0.0.1', port: Number(ready[1]), platform: 1, deviceId: Uint8Array.from(crypto.randomBytes(16)),
                createSocket: () => new TcpSocketHost(), clock: () => performance.now() });
            await client.connect();
            account.setConnection(1);
            const read = bytes => {
                const message = new NetInterfaceMessage(runtime);
                try {
                    message.parse(bytes);
                    const text = key => { const item = message.getValue(0, key); return item ? Buffer.from(item.bytes).toString('utf8') : ''; };
                    const number = key => { const item = message.getValue(0, key); return item ? Buffer.from(item.bytes).readInt32LE(0) : 0; };
                    return { user: text(A.ACCOUNT_FIELD_USER_ID), login: text(A.ACCOUNT_FIELD_LOGIN),
                        level: number(A.ACCOUNT_FIELD_VIP_LEVEL), connected: number(A.ACCOUNT_FIELD_CONNECTED) };
                } finally { message.dispose(); }
            };
            assert.equal(read(account.copyState()).user, '');
            assert.equal(read(account.copyState()).level, 0);
            const learnedBy = Date.now() + 10000;
            for (;;) {
                try { await account.request(client, A.ACCOUNT_QUERY_CHANNELS); break; }
                catch (error) { if (error.status !== 1 || Date.now() >= learnedBy) throw error; await delay(100); }
            }
            const login = 'native_' + crypto.randomBytes(8).toString('hex');
            const parameters = new NetInterfaceMessage(runtime);
            let encoded;
            try {
                parameters.setValue(0, A.ACCOUNT_FIELD_LOGIN, 4, Uint8Array.from(Buffer.from(login)));
                parameters.setValue(0, A.ACCOUNT_FIELD_PASSWORD, 4, Uint8Array.from(Buffer.from('process-test-password-123')));
                encoded = parameters.encode();
            } finally { parameters.dispose(); }
            try {
                const registered = read((await account.request(client, A.ACCOUNT_REGISTER_PASSWORD, encoded)).state);
                assert.ok(registered.user && registered.connected === 1 && registered.level === 0 && registered.login === login);
                const state = read((await account.request(client, A.ACCOUNT_QUERY_STATE)).state);
                assert.equal(state.user, registered.user);
                assert.equal(state.level, 0);
                await account.request(client, A.ACCOUNT_LOGOUT);
                assert.equal(read(account.copyState()).user, '');
                const signed = read((await account.request(client, A.ACCOUNT_LOGIN_PASSWORD, encoded)).state);
                assert.equal(signed.user, registered.user);
                assert.equal(signed.level, 0);
            } finally { encoded.fill(0); }
            await require('./account_service_process_test').probePasswordRoute(runtime, client, 101);
            if (options['--gui-test']) {
                assert.ok(options['--qt'], '--qt is required for GUI runtime');
                const qt = path.resolve(options['--qt']);
                process.env.PATH = path.join(qt, 'bin') + path.delimiter + process.env.PATH;
                process.env.QT_PLUGIN_PATH = path.join(qt, 'plugins');
                const manager = run(path.join(bin, 'NetClientManager1.1.exe'), ['--protocol=3', '--address=127.0.0.1', '--port=' + ready[1],
                    '--cache-file=' + path.join(output, 'client.cache'), '--stop-file=' + path.join(output, 'client.stop'),
                    '--run-for-ms=80000'], 'client', /CLIENT_GATEWAY_READY pid=/);
                await manager.ready;
                const guiRoot = path.join(output, 'gui');
                fs.mkdirSync(guiRoot);
                const gui = run(path.resolve(options['--gui-test']), ['--mode', 'account', '--output', guiRoot], 'gui', /LUMA_ACCOUNT_GUI_PASS/);
                const finished = await gui.completed;
                assert.equal(finished.code, 0, 'Qt account test failed; inspect gui/account-gui.json and process logs');
                assert.equal(JSON.parse(fs.readFileSync(path.join(guiRoot, 'account-gui.json'), 'utf8')).pass, true);
                fs.writeFileSync(path.join(output, 'client.stop'), '');
                assert.equal((await manager.completed).code, 0);
            }
            client.dispose(); client = null;
            account.dispose(); account = null;
            for (const name of ['luma', 'account', 'gateway']) fs.writeFileSync(path.join(output, name + '.stop'), '');
            const stopped = await Promise.all([luma.completed, server.completed, gateway.completed]);
            assert.ok(stopped.every(entry => entry.code === 0 && !entry.signal));
        })(), deadline]);
        fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({ pass: true, product: 101, ordinaryVipLevel: 0,
            realServices: true, passwordOperationsRejectForgery: true, registration: true, logoutAndLogin: true, gui: !!options['--gui-test'] }, null, 2));
        console.log('Luma product -> internal AccountServer -> authoritative VIP0: PASS');
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ error: String(error.stack || error) }, null, 2));
        throw error;
    } finally {
        clearTimeout(timer);
        if (client) client.dispose();
        if (account) account.dispose();
        for (const child of children) if (child.child.exitCode === null && child.child.signalCode === null) child.child.kill();
        await Promise.all(children.map(child => child.completed.catch(() => {})));
    }
}
main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });