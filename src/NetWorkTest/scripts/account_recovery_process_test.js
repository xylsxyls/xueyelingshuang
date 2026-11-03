'use strict';
// Real service/TCP/production dual-Wasm tests. Requires an explicitly owned loopback MySQL.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');
const { spawnSync } = require('node:child_process');
const { performance } = require('node:perf_hooks');
const { createMysqlTestConfig } = require('./account_service_process_test');
const { start } = require('./local_link_gateway_process_test');
const { TcpSocketHost } = require('./service_gateway_process_test');
const { NetPeerRuntime } = require('./host/NetInterfaceRuntime');
const { readWasm } = require('./host/read_wasm');
// The loopback-only fault proxy belongs to this integration scenario.
// Loopback-only MySQL transport fixture. Never records packets, SQL, credentials or result rows.
const net = require('node:net');
const { EventEmitter } = require('node:events');

class MysqlPackets {
    constructor(receive) { this.buffer = Buffer.alloc(0); this.receive = receive; }
    push(bytes) {
        this.buffer = Buffer.concat([this.buffer, bytes]);
        while (this.buffer.length >= 4) {
            const length = this.buffer.readUIntLE(0, 3) + 4;
            if (length > 16777219) throw new Error('MySQL fixture packet exceeds protocol bound');
            if (this.buffer.length < length) return;
            const packet = this.buffer.subarray(0, length);
            this.buffer = this.buffer.subarray(length);
            this.receive(packet, packet.subarray(4));
        }
    }
}

function sqlStage(text) {
    const sql = text.trim().toUpperCase();
    if (sql === 'COMMIT') return 'commit';
    if (sql === 'ROLLBACK') return 'rollback';
    if (sql.startsWith('UPDATE ACCOUNT_ORDERS SET STATE=1')) return 'fulfill-write';
    if (sql.startsWith('INSERT INTO ACCOUNT_USERS')) return 'register-write';
    if (sql === 'SELECT 1') return 'probe';
    return '';
}

class MysqlFaultProxy extends EventEmitter {
    constructor(port) {
        super();
        if (!Number.isInteger(port) || port < 1 || port > 65535) throw new Error('Explicit loopback backend port required');
        this.port = port;
        this.connections = new Set();
        this.fault = null;
        this.unavailable = false;
        this.stall = false;
        this.events = [];
        this.counts = { commit: 0, 'fulfill-write': 0, 'register-write': 0 };
    }
    async start() {
        this.server = net.createServer(client => this.accept(client));
        await new Promise((resolve, reject) => { this.server.once('error', reject); this.server.listen(0, '127.0.0.1', resolve); });
        return this.server.address().port;
    }
    arm(stage, mode) {
        if (this.fault) throw new Error('Previous transport fault not consumed');
        this.fault = { stage, mode };
    }
    record(stage, mode) {
        this.events.push({ stage, mode });
        this.emit('fault', { stage, mode });
    }
    accept(client) {
        const connection = { client, upstream: null, statements: new Map(), preparing: '', current: '', rollbackDrop: false };
        this.connections.add(connection);
        const close = () => { client.destroy(); if (connection.upstream) connection.upstream.destroy(); this.connections.delete(connection); };
        client.on('error', close);
        client.on('close', close);
        this.emit('accepted');
        if (this.unavailable) { close(); return; }
        if (this.stall) return;
        const upstream = connection.upstream = net.connect({ host: '127.0.0.1', port: this.port });
        upstream.on('error', close);
        upstream.on('close', close);
        const clientPackets = new MysqlPackets((packet, body) => {
            let stage = '';
            connection.preparing = '';
            if (body[0] === 3 || body[0] === 22) {
                stage = sqlStage(body.subarray(1).toString('utf8'));
                if (body[0] === 22) connection.preparing = stage;
            } else if (body[0] === 23 && body.length >= 5) {
                stage = connection.statements.get(body.readUInt32LE(1)) || '';
            }
            connection.current = stage;
            connection.executing = body[0] === 3 || body[0] === 23;
            if (body[0] === 3 || body[0] === 23) {
                if (Object.hasOwn(this.counts, stage)) ++this.counts[stage];
                if (connection.rollbackDrop && stage === 'rollback') {
                    this.record('rollback', 'before'); close(); return;
                }
                if (this.fault && this.fault.stage === stage && this.fault.mode !== 'after') {
                    const fault = this.fault; this.fault = null;
                    this.record(fault.stage, fault.mode);
                    if (fault.mode === 'error-rollback') {
                        connection.rollbackDrop = true;
                        const message = Buffer.from('test controlled lock failure');
                        const payload = Buffer.alloc(9 + message.length);
                        payload[0] = 255; payload.writeUInt16LE(1205, 1); payload.write('#HY000', 3); message.copy(payload, 9);
                        const header = Buffer.alloc(4); header.writeUIntLE(payload.length, 0, 3); header[3] = 1;
                        client.write(Buffer.concat([header, payload])); return;
                    }
                    close(); return;
                }
            }
            upstream.write(packet);
        });
        const serverPackets = new MysqlPackets((packet, body) => {
            if (packet[3] === 1 && connection.preparing && body[0] === 0 && body.length >= 5) {
                connection.statements.set(body.readUInt32LE(1), connection.preparing);
                connection.preparing = '';
            }
            if (connection.executing && packet[3] === 1 && body[0] === 0 && this.fault && this.fault.mode === 'after' && this.fault.stage === connection.current) {
                const fault = this.fault; this.fault = null;
                this.record(fault.stage, 'after-server-ok'); close(); return;
            }
            client.write(packet);
        });
        client.on('data', bytes => { try { clientPackets.push(bytes); } catch (_) { close(); } });
        upstream.on('data', bytes => { try { serverPackets.push(bytes); } catch (_) { close(); } });
    }
    dropAll() {
        for (const connection of this.connections) { connection.client.destroy(); if (connection.upstream) connection.upstream.destroy(); }
    }
    async close() {
        this.dropAll();
        if (this.server) await new Promise(resolve => this.server.close(resolve));
    }
}

const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
async function bounded(promise, ms, reason) {
    let timer;
    try { return await Promise.race([promise, new Promise((_, reject) => { timer = setTimeout(() => reject(new Error(reason)), ms); })]); }
    finally { clearTimeout(timer); }
}

async function main() {
    const options = {};
    for (const value of process.argv.slice(2)) {
        const split = value.indexOf('=');
        const key = value.slice(0, split);
        assert.ok(split > 0 && ['--gateway', '--account', '--service', '--luma-service', '--pdf-service', '--module', '--account-module', '--mysql-config', '--mysql-client', '--output'].includes(key) && !options[key]);
        options[key] = path.resolve(value.slice(split + 1));
    }
    for (const name of ['--gateway', '--account', '--service', '--luma-service', '--pdf-service', '--module', '--account-module', '--mysql-config', '--mysql-client', '--output']) assert.ok(options[name], name);
    assert.equal(path.basename(options['--gateway']), 'NetWorkTestNode.exe', 'Use the dedicated multi-instance gateway test host');
    assert.equal(path.basename(options['--service']), 'SplitViewerServer.exe');
    const output = options['--output'];
    assert.ok(!fs.existsSync(output)); fs.mkdirSync(output, { recursive: true });
    // Assemble current caller adapters with this exact SDK's generated constants.
    // Generated SDK files never belong in the application's source directory.
    const adapters = path.join(output, 'caller'); fs.mkdirSync(adapters);
    const adapterSha256 = {};
    for (const name of ['AccountRuntime.js', 'NetAccountClient.js', 'NetInterfaceMessage.js', 'NetWxClient.js', 'WasmMemory.js']) {
        const source = path.resolve(__dirname, '../../CookWx/network', name);
        fs.copyFileSync(source, path.join(adapters, name));
        adapterSha256[name] = crypto.createHash('sha256').update(fs.readFileSync(source)).digest('hex');
    }
    fs.copyFileSync(path.join(path.dirname(options['--account-module']), 'AccountProtocol.js'), path.join(adapters, 'AccountProtocol.js'));
    const { loadAccountRuntime } = require(path.join(adapters, 'AccountRuntime'));
    const { NetInterfaceMessage } = require(path.join(adapters, 'NetInterfaceMessage'));
    const { createServiceClient } = require(path.join(adapters, 'NetWxClient'));
    const { NetAccountClient } = require(path.join(adapters, 'NetAccountClient'));
    const A = require(path.join(adapters, 'AccountProtocol'));
    const source = fs.readFileSync(options['--mysql-config'], 'utf8');
    const config = {};
    for (const line of source.split(/\r?\n/)) { const match = /^(Host|Port|User|Password)=(.*)$/.exec(line); if (match) config[match[1]] = match[2]; }
    assert.ok(['127.0.0.1', 'localhost', '::1'].includes(config.Host));
    const proxy = new MysqlFaultProxy(Number(config.Port));
    const children = [];
    const cases = [];
    let account, client, service, lumaService, pdfService, accountService, gateway, runtime;
    const productClients = [];
    const productAccounts = [];
    let diagnostics = '';
    const run = (binary, args, name, ready) => { const child = start(binary, args, name, output, ready); children.push(child); return child; };
    const stop = async (entry, name) => { fs.writeFileSync(path.join(output, name + '.stop'), ''); assert.equal((await bounded(entry.completed, 12000, 'Owned service stop timeout')).code, 0); };
    try {
        const port = await proxy.start();
        const ini = createMysqlTestConfig(options['--mysql-config'], path.join(output, 'AccountServer.ini'));
        const original = fs.readFileSync(ini, 'utf8');
        const database = /^Database=(\w+)$/m.exec(original)[1];
        fs.writeFileSync(ini, original.replace(/^Port=\d+$/m, 'Port=' + port));
        const clientIni = path.join(output, 'mysql-client.ini');
        fs.writeFileSync(clientIni, `[client]\nhost=127.0.0.1\nport=${config.Port}\nuser=${config.User}\npassword=${config.Password}\nprotocol=TCP\n`, { mode: 0o600 });
        const sql = statement => {
            const result = spawnSync(options['--mysql-client'], ['--defaults-file=' + clientIni, '--batch', '--skip-column-names', '--database=' + database],
                { input: statement, encoding: 'utf8', windowsHide: true, timeout: 7000 });
            assert.equal(result.status, 0, 'Owned MySQL inspection failed');
            return result.stdout.replace(/\r\n/g, '\n').trim();
        };
        gateway = run(options['--gateway'], ['--protocol=3', '--listen=127.0.0.1', '--port=0', '--identity-directory=' + path.join(output, 'identity'),
            '--stop-file=' + path.join(output, 'gateway.stop'), '--run-for-ms=180000'], 'gateway', /GATEWAY_READY port=(\d+) pid=(\d+)/);
        const ready = await bounded(gateway.ready, 20000, 'Gateway startup timeout');
        accountService = run(options['--account'], ['--protocol=3', '--database=' + ini, '--workers=2', '--gateway-pid=' + ready[2], '--math-vip=1',
            '--stop-file=' + path.join(output, 'account.stop'), '--run-for-ms=180000'], 'account', /ACCOUNT_READY endpoint=/);
        accountService.child.stderr.on('data', bytes => { diagnostics += bytes.toString('utf8'); });
        await bounded(accountService.ready, 20000, 'Account startup timeout');
        service = run(options['--service'], ['--gateway-pid=' + ready[2], '--stop-file=' + path.join(output, 'service.stop'), '--run-for-ms=180000'],
            'service', /SPLITVIEWER_SERVICE_READY endpoint=/);
        await bounded(service.ready, 20000, 'SplitViewer proxy startup timeout');
        lumaService = run(options['--luma-service'], ['--gateway-pid=' + ready[2], '--stop-file=' + path.join(output, 'luma.stop'), '--run-for-ms=180000'], 'luma', /LUMA_SERVICE_READY endpoint=/);
        pdfService = run(options['--pdf-service'], ['--gateway-pid=' + ready[2], '--stop-file=' + path.join(output, 'pdf.stop'), '--run-for-ms=180000'], 'pdf', /PDF_SERVICE_READY endpoint=/);
        await bounded(Promise.all([lumaService.ready, pdfService.ready]), 20000, 'Product proxies startup timeout');
        runtime = await NetPeerRuntime.load(require(options['--module']), { createSocket: () => new TcpSocketHost(), clock: () => performance.now(), randomSource: async size => Uint8Array.from(crypto.randomBytes(size)),
            moduleOptions: { noInitialRun: true, wasmBinary: readWasm(options['--module']) } });
        assert.equal(typeof runtime.module._netInterfaceMessageCreate, 'function', 'Select the complete NetPeer SDK with message ABI exports');
        const business = await loadAccountRuntime(require(options['--account-module']), runtime, { noInitialRun: true, wasmBinary: readWasm(options['--account-module']) });
        account = new NetAccountClient(business, 102);
        client = createServiceClient({ onFrame: frame => { if (frame.kind === 5 && frame.type === A.ACCOUNT_EVENT_SESSION_REPLACED) account.applyEvent(frame.payload); return true; }, runtime, protocol: 3, autoReconnect: false, serviceId: 102, address: '127.0.0.1', port: Number(ready[1]),
            platform: 1, deviceId: Uint8Array.from(crypto.randomBytes(16)), createSocket: () => new TcpSocketHost(), clock: () => performance.now() });
        productClients.push(client);
        await client.connect(); account.setConnection(1);
        const read = bytes => {
            const message = new NetInterfaceMessage(runtime); message.parse(bytes);
            const value = key => { const item = message.getValue(0, key); if (!item) return undefined;
                const buffer = Buffer.from(item.bytes); return item.type === 4 ? buffer.toString('utf8') : item.type === 1 ? buffer.readInt32LE() : Number(buffer.readBigUInt64LE()); };
            const result = {};
            for (const [name, field] of Object.entries(A)) if (name.startsWith('ACCOUNT_FIELD_')) result[name.slice(14)] = value(field);
            message.dispose(); return result;
        };
        const request = async (operation, fields = {}, timeout = 10000) => {
            const message = new NetInterfaceMessage(runtime);
            for (const [field, value] of Object.entries(fields)) {
                const bytes = typeof value === 'string' ? Buffer.from(value) : Buffer.alloc(4);
                if (typeof value !== 'string') bytes.writeInt32LE(value);
                message.setValue(0, Number(field), typeof value === 'string' ? 4 : 1, Uint8Array.from(bytes));
            }
            const body = message.encode(); message.dispose();
            try { const result = await account.request(client, operation, body, timeout); return result.response.length ? read(result.response) : {}; }
            finally { body.fill(0); }
        };
        const learned = performance.now() + 10000;
        for (;;) { try { await request(A.ACCOUNT_QUERY_CHANNELS); break; } catch (error) { if (error.status !== 1 || performance.now() >= learned) throw error; await delay(50); } }
        const login = 'recovery_' + crypto.randomBytes(8).toString('hex');
        const password = 'isolated-password-123';
        const credentials = { [A.ACCOUNT_FIELD_LOGIN]: login, [A.ACCOUNT_FIELD_PASSWORD]: password };
        const user = await request(A.ACCOUNT_REGISTER_PASSWORD, credentials);
        assert.match(user.USER_ID, /^u_[a-f0-9]{32}$/); assert.equal(user.VIP_LEVEL, 0);
        const uid = user.USER_ID;
        // Real product proxies and separate authenticated roots, including device IDs that cannot be shared across apps.
        const openProduct = async (product, platform) => {
            const identity = new NetAccountClient(business, product);
            productAccounts.push(identity);
            const events = [];
            const network = createServiceClient({ runtime, protocol: 3, serviceId: product, address: '127.0.0.1', port: Number(ready[1]),
                platform, deviceId: Uint8Array.from(crypto.randomBytes(16)), clock: () => performance.now(),
                onFrame: frame => {
                    if (frame.kind === 5 && frame.type === A.ACCOUNT_EVENT_SESSION_REPLACED) {
                        events.push(read(frame.payload));
                        assert.equal(frame.product, product, 'Revocation crossed the authenticated product root');
                        identity.applyEvent(frame.payload);
                    }
                    return true;
                } });
            productClients.push(network);
            await bounded(network.connect(), 12000, 'Product client connect'); identity.setConnection(1);
            const call = async (operation, fields = {}) => {
                const message = new NetInterfaceMessage(runtime);
                for (const [field, value] of Object.entries(fields)) {
                    const bytes = typeof value === 'string' ? Buffer.from(value) : Buffer.alloc(4);
                    if (typeof value !== 'string') bytes.writeInt32LE(value);
                    message.setValue(0, Number(field), typeof value === 'string' ? 4 : 1, Uint8Array.from(bytes));
                }
                const body = message.encode(); message.dispose();
                try { const result = await identity.request(network, operation, body); return result.response.length ? read(result.response) : {}; }
                finally { body.fill(0); }
            };
            const deadline = performance.now() + 10000;
            for (;;) { try { await call(A.ACCOUNT_QUERY_CHANNELS); break; } catch (error) { if (error.status !== 1 || performance.now() >= deadline) throw error; await delay(50); } }
            return { identity, network, events, call };
        };
        const lumaPhone = await openProduct(101, 2);
        const pdfPhone = await openProduct(103, 2);
        const lumaPc = await openProduct(101, 1);
        const pdfPc = await openProduct(103, 1);
        for (const peer of [lumaPhone, pdfPhone, lumaPc, pdfPc]) assert.equal((await peer.call(A.ACCOUNT_LOGIN_PASSWORD, credentials)).USER_ID, uid);
        for (const peer of [lumaPhone, pdfPhone, lumaPc, pdfPc]) assert.equal((await peer.call(A.ACCOUNT_QUERY_STATE)).USER_ID, uid);
        cases.push({ name: '101/103 independent phone and desktop devices coexist through real new-gateway proxies', pass: true });
        const newLumaPhone = await openProduct(101, 3);
        assert.equal((await newLumaPhone.call(A.ACCOUNT_LOGIN_PASSWORD, credentials)).USER_ID, uid);
        const eventDeadline = performance.now() + 5000;
        while (!lumaPhone.events.length && performance.now() < eventDeadline) await delay(10);
        assert.equal(lumaPhone.events.length, 1, 'Old same-product mobile did not receive revocation');
        assert.equal(lumaPhone.events[0].PRODUCT, 101);
        assert.ok(lumaPhone.events[0].GENERATION);
        await assert.rejects(lumaPhone.call(A.ACCOUNT_QUERY_STATE), error => error.accountResult === A.ACCOUNT_UNAUTHORIZED);
        for (const peer of [pdfPhone, lumaPc, pdfPc]) {
            assert.equal((await peer.call(A.ACCOUNT_QUERY_STATE)).USER_ID, uid);
            assert.equal(peer.events.length, 0, 'Replacement event revoked another product/category');
        }
        cases.push({ name: '101 App to mini-program replacement revokes only 101 mobile, notification has product scope', pass: true });
        await newLumaPhone.call(A.ACCOUNT_REFRESH_SESSION);
        const oldLogout = newLumaPhone.identity.authorization();
        await newLumaPhone.call(A.ACCOUNT_LOGOUT);
        try { assert.equal(read(await newLumaPhone.network.requestWithBuilder(101, A.ACCOUNT_LOGOUT, () => oldLogout)).RESULT, A.ACCOUNT_OK); }
        finally { oldLogout.fill(0); }
        for (const peer of [pdfPhone, lumaPc, pdfPc]) assert.equal((await peer.call(A.ACCOUNT_QUERY_STATE)).USER_ID, uid);
        assert.equal(newLumaPhone.events.length, 0, 'Own logout/refresh event invalidated the pending successful RPC');
        assert.equal(pdfPhone.events.length + pdfPc.events.length, 0);
        // An authorization body from product 101 must fail when the trusted root is 103.
        const forged = lumaPc.identity.authorization();
        try {
            const response = read(await pdfPc.network.requestWithBuilder(103, A.ACCOUNT_QUERY_STATE, () => forged));
            assert.equal(response.RESULT, A.ACCOUNT_UNAUTHORIZED);
        } finally { forged.fill(0); }
        cases.push({ name: '101 refresh/logout/idempotent logout preserves 103, copied 101 authorization rejected at 103', pass: true });
        const state = () => request(A.ACCOUNT_QUERY_STATE);
        const create = () => request(A.ACCOUNT_CREATE_MATH_ORDER, { [A.ACCOUNT_FIELD_IDEMPOTENCY_KEY]: crypto.randomBytes(16).toString('hex') });
        const answer = order => {
            const numbers = /^(-?\d+) ([+*-]) (-?\d+) = \?$/.exec(order.QUESTION); assert.ok(numbers, 'Unexpected server question');
            const a = Number(numbers[1]), b = Number(numbers[3]);
            return request(A.ACCOUNT_ANSWER_QUESTION, { [A.ACCOUNT_FIELD_ORDER_ID]: order.ORDER_ID, [A.ACCOUNT_FIELD_QUESTION_ID]: order.QUESTION_ID,
                [A.ACCOUNT_FIELD_ANSWER]: numbers[2] === '+' ? a + b : numbers[2] === '-' ? a - b : a * b });
        };
        const high = Math.floor(Date.now() / 1000) + 900;
        sql(`INSERT INTO account_vip(user_id,product,until_utc,level) VALUES('${uid}',102,${high},3),('${uid}',101,${high + 900},4),('${uid}',103,${high + 1800},2);`);
        const order = await create(); const paid = await answer(order);
        assert.equal(paid.FULFILLED_UNTIL, high + 180);
        assert.equal((await state()).VIP_LEVEL, 3); assert.equal((await answer(order)).FULFILLED_UNTIL, high + 180);
        assert.equal(sql(`SELECT product,until_utc,level FROM account_vip WHERE user_id='${uid}' AND product<>102 ORDER BY product;`),
            `101\t${high + 900}\t4\n103\t${high + 1800}\t2`);
        cases.push({ name: 'SplitViewer102 active VIP3 +180, exact retry once, 101/103 unchanged', pass: true });
        sql(`UPDATE account_vip SET until_utc=${Math.floor(Date.now() / 1000) - 60},level=5 WHERE user_id='${uid}' AND product=102;`);
        const began = Math.floor(Date.now() / 1000); await answer(await create());
        const expired = await state(); assert.equal(expired.VIP_LEVEL, 1); assert.ok(expired.VIP_UNTIL >= began + 180 && expired.VIP_UNTIL <= expired.SERVER_UTC + 180);
        cases.push({ name: 'SplitViewer102 expired higher tier renews only VIP1', pass: true });
        let lastOrder;
        for (const [stage, mode, expected] of [['fulfill-write', 'before', A.ACCOUNT_STORAGE_FAILED], ['fulfill-write', 'error-rollback', A.ACCOUNT_BUSY], ['commit', 'after', A.ACCOUNT_STORAGE_FAILED]]) {
            const pending = await create(); const previous = (await state()).VIP_UNTIL;
            lastOrder = pending;
            const count = proxy.counts['fulfill-write']; proxy.arm(stage, mode);
            await assert.rejects(answer(pending), error => error.accountResult === expected);
            assert.equal(proxy.fault, null, 'Fault not reached');
            assert.equal(proxy.counts['fulfill-write'], count + 1, 'Original failed request replayed');
            const committed = mode === 'after';
            assert.equal(sql(`SELECT state FROM account_orders WHERE order_id='${pending.ORDER_ID}';`), committed ? '1' : '0');
            assert.equal(sql(`SELECT count(*) FROM account_confirmations WHERE order_id='${pending.ORDER_ID}';`), committed ? '1' : '0');
            assert.equal(Number(sql(`SELECT until_utc FROM account_vip WHERE user_id='${uid}' AND product=102;`)), previous + (committed ? 180 : 0));
            const retried = await answer(pending); assert.equal(retried.FULFILLED_UNTIL, previous + 180);
            assert.equal((await answer(pending)).FULFILLED_UNTIL, previous + 180);
            cases.push({ name: stage + '/' + mode + ': failure truthful, no replay, explicit same-order retry once', pass: true });
        }
        assert.ok(proxy.events.some(event => event.stage === 'rollback'), 'Rollback failure path not exercised');
        proxy.dropAll(); assert.equal((await state()).USER_ID, uid);
        cases.push({ name: 'Pre-transaction disconnected handle recovers with same process/UID', pass: true });
        proxy.unavailable = true; proxy.dropAll();
        const unavailableBegan = performance.now();
        for (let attempt = 0; attempt < 3; ++attempt) await assert.rejects(state(), error => error.accountResult === A.ACCOUNT_STORAGE_FAILED);
        assert.ok(performance.now() - unavailableBegan < 8000, 'Unavailable database requests not bounded');
        proxy.unavailable = false;
        let restored;
        const recoveryDeadline = performance.now() + 6000;
        do { try { restored = await state(); } catch (error) { if (error.accountResult !== A.ACCOUNT_STORAGE_FAILED) throw error; await delay(100); } } while (!restored && performance.now() < recoveryDeadline);
        assert.equal(restored && restored.USER_ID, uid);
        cases.push({ name: 'Persistent unavailable/backoff then recovery without process restart', pass: true });
        const readyMutator = async () => {
            const deadline = performance.now() + 6000;
            for (;;) {
                try { await answer(lastOrder); return; }
                catch (error) { if (error.accountResult !== A.ACCOUNT_STORAGE_FAILED || performance.now() >= deadline) throw error; await delay(100); }
            }
        };
        await readyMutator();
        sql('RENAME TABLE account_meta TO recovery_meta_held;');
        try {
            proxy.dropAll();
            await assert.rejects(state(), error => error.accountResult === A.ACCOUNT_STORAGE_FAILED);
            assert.equal(sql("SELECT count(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='account_meta';"), '0');
            assert.equal(sql('SELECT count(*) FROM account_users;'), '1');
        } finally { sql('RENAME TABLE recovery_meta_held TO account_meta;'); }
        await readyMutator();
        cases.push({ name: 'Recovery rejects missing metadata without runtime table/database initialization', pass: true });
        for (const cancel of [true, false]) {
            // Confirm the regular executor has left backoff using the already fulfilled order.
            await readyMutator();
            proxy.stall = true; proxy.dropAll();
            const accepted = new Promise(resolve => proxy.once('accepted', resolve));
            const forbiddenLogin = 'cancel_' + crypto.randomBytes(8).toString('hex');
            const parameters = { [A.ACCOUNT_FIELD_LOGIN]: forbiddenLogin, [A.ACCOUNT_FIELD_PASSWORD]: password };
            const message = new NetInterfaceMessage(runtime);
            for (const [key, value] of Object.entries(parameters)) message.setValue(0, Number(key), 4, Uint8Array.from(Buffer.from(value)));
            const body = message.encode(); message.dispose();
            const pending = account.request(client, A.ACCOUNT_REGISTER_PASSWORD, body, cancel ? 5000 : 250);
            const rejected = assert.rejects(pending); await bounded(accepted, 4000, 'Reconnect not attempted');
            if (cancel) pending.cancel();
            await rejected; proxy.stall = false; proxy.dropAll(); body.fill(0);
            const settledBy = performance.now() + 5000;
            let settled = false;
            do { try { await request(A.ACCOUNT_QUERY_CHANNELS); settled = true; } catch (_) { await delay(20); } } while (!settled && performance.now() < settledBy);
            assert.ok(settled); assert.equal(sql(`SELECT count(*) FROM account_users WHERE login='${forbiddenLogin}';`), '0');
            cases.push({ name: cancel ? 'Cancellation during reconnect never registers user' : 'Original deadline during reconnect never registers user', pass: true });
        }
        await readyMutator();
        await request(A.ACCOUNT_LOGOUT); const signed = await request(A.ACCOUNT_LOGIN_PASSWORD, credentials); assert.equal(signed.USER_ID, uid);
        cases.push({ name: 'Final authoritative login/state/logout/login', pass: true });
        // Hold a real connection handshake and stop the host while recovery is in flight.
        await readyMutator(); proxy.stall = true; proxy.dropAll();
        const accepted = new Promise(resolve => proxy.once('accepted', resolve));
        const stopLogin = 'stop_' + crypto.randomBytes(8).toString('hex');
        const stopMessage = new NetInterfaceMessage(runtime);
        stopMessage.setValue(0, A.ACCOUNT_FIELD_LOGIN, 4, Uint8Array.from(Buffer.from(stopLogin)));
        stopMessage.setValue(0, A.ACCOUNT_FIELD_PASSWORD, 4, Uint8Array.from(Buffer.from(password)));
        const stopBody = stopMessage.encode(); stopMessage.dispose();
        const stoppedRequest = account.request(client, A.ACCOUNT_REGISTER_PASSWORD, stopBody, 5000);
        const stoppedRejected = assert.rejects(stoppedRequest);
        await bounded(accepted, 4000, 'Stop test did not enter reconnect');
        await stop(accountService, 'account'); await stoppedRejected; stopBody.fill(0);
        assert.equal(sql(`SELECT count(*) FROM account_users WHERE login='${stopLogin}';`), '0');
        cases.push({ name: 'Stop during recovery reclaims owned workers without registering user', pass: true });
        account.dispose(); account = null; client.dispose(); client = null;
        await stop(lumaService, 'luma'); await stop(pdfService, 'pdf');
        await stop(service, 'service'); await stop(gateway, 'gateway');
        assert.match(diagnostics, /operation=40007 phase=commit code=\d+ outcome=failed/);
        assert.match(diagnostics, /phase=rollback code=\d+ outcome=failed/);
        assert.match(diagnostics, /phase=reconnect code=0 outcome=ready/);
        const result = { pass: true, product: 102, products: [101, 102, 103], cases, faults: proxy.events, counts: proxy.counts,
            gatewaySha256: crypto.createHash('sha256').update(fs.readFileSync(options['--gateway'])).digest('hex'),
            networkSdk: require(options['--module']).sdk, accountSdk: require(options['--account-module']).sdk, adapterSha256,
            accountSha256: crypto.createHash('sha256').update(fs.readFileSync(options['--account'])).digest('hex'),
            realServices: true, host: 'Production dual-Wasm and Node TCP; not GUI or real payment' };
        fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify(result, null, 2));
        console.log('Account recovery and SplitViewer102 real service cases PASS=' + cases.length);
    } catch (error) {
        fs.writeFileSync(path.join(output, 'failure.json'), JSON.stringify({ error: error.stack, cases, faults: proxy.events }, null, 2));
        throw error;
    } finally {
        for (const identity of productAccounts) identity.dispose();
        for (const network of productClients) network.dispose();
        if (account) account.dispose(); if (client) client.dispose();
        for (const [entry, name] of [[lumaService, 'luma'], [pdfService, 'pdf'], [service, 'service'], [accountService, 'account'], [gateway, 'gateway']]) {
            if (entry && entry.child.exitCode === null && entry.child.signalCode === null) {
                try { await stop(entry, name); } catch (_) { entry.child.kill(); }
            }
        }
        await Promise.all(children.map(child => child.completed.catch(() => {}))); await proxy.close();
        await bounded((async () => { while (productClients.some(network => !network.released)) await delay(10); })(), 5000, 'Network callers did not release');
        if (runtime) runtime.dispose();
    }
}
bounded(main(), 150000, 'Recovery integration deadline').catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });
