'use strict';

// Actual gateway and AccountServer processes. Credentials and databases are
// isolated test resources; neither real user data nor existing processes change.
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');
const { spawn } = require('node:child_process');
const { performance } = require('node:perf_hooks');

// Shared fixtures for account process tests; importing this file does not start processes.
// Test databases are always random and separate from the production account schema.
function createMysqlTestConfig(source, destination) {
  assert.ok(path.isAbsolute(source || ''), 'An explicit absolute --mysql-config is required')
  const text = fs.readFileSync(source, 'utf8').replace(/^\uFEFF/, '')
  assert.ok(Buffer.byteLength(text) <= 16384, 'MySQL test INI too large')
  let section = ''
  const values = {}
  for (const line of text.split(/\r?\n/)) {
    const header = /^\s*\[([^\]]+)\]\s*$/.exec(line)
    if (header) { section = header[1].toLowerCase(); continue }
    const field = /^\s*([A-Za-z]+)\s*=(.*)$/.exec(line)
    if (section === 'mysql' && field) values[field[1].toLowerCase()] = field[2].trim()
  }
  assert.ok(['127.0.0.1', 'localhost', '::1'].includes(values.host), 'Tests require explicit local MySQL')
  assert.ok(/^\d{1,5}$/.test(values.port || '') && Number(values.port) > 0 && Number(values.port) < 65536, 'Invalid MySQL port')
  assert.ok(values.user, 'MySQL test user is required')
  const database = 'account_nettest_' + crypto.randomBytes(12).toString('hex')
  const output = `[MySQL]\nHost=${values.host}\nPort=${values.port}\nUser=${values.user}\nPassword=${values.password || ''}\nDatabase=${database}\n`
  fs.writeFileSync(destination, output, { flag: 'wx', mode: 0o600 })
  return destination
}

// Deliberately bypass the client proof guard: the actual service must reject fabricated
// credentials. ACCOUNT_UNAUTHORIZED (not missing-field INVALID) also proves both forwarding
// layers preserve the new password/proof parameters. No real account is changed.
async function probePasswordRoute(runtime, network, product) {
    const { NetInterfaceMessage } = require('./host/NetInterfaceMessage');
    const A = require('../../../include/Account/AccountProtocol');
    const cases = [
        [A.ACCOUNT_RESET_PASSWORD, [[A.ACCOUNT_FIELD_VERIFICATION, 'a'.repeat(64)], [A.ACCOUNT_FIELD_NEW_PASSWORD, 'fixture-new-password']]],
        [A.ACCOUNT_VERIFY_PASSWORD_SMS, [[A.ACCOUNT_FIELD_CHALLENGE, 'b'.repeat(32)], [A.ACCOUNT_FIELD_CODE, '123456']]]
    ];
    for (const [operation, fields] of cases) {
        const message = new NetInterfaceMessage(runtime);
        let body;
        try {
            const productBytes = Buffer.alloc(4);
            productBytes.writeInt32LE(product);
            message.setValue(0, A.ACCOUNT_FIELD_PRODUCT, 1, Uint8Array.from(productBytes));
            for (const [key, value] of fields) message.setValue(0, key, 4, Uint8Array.from(Buffer.from(value)));
            body = message.encode();
            const response = await network.requestWithBuilder(product, operation, () => body, 10000);
            message.parse(response);
            const result = message.getValue(0, A.ACCOUNT_FIELD_RESULT);
            assert.ok(result && result.type === 1 && result.bytes.length === 4, 'Missing authoritative account result');
            assert.equal(Buffer.from(result.bytes).readInt32LE(), A.ACCOUNT_UNAUTHORIZED, 'New operation/fields were lost or forged proof was accepted');
        } finally {
            if (body) body.fill(0);
            message.dispose();
        }
    }
}

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
        if (!['--gateway', '--account', '--report', '--instant', '--mysql-config'].includes(key) || Object.hasOwn(args, key) ||
            (key === '--instant' ? separator >= 0 : separator < 0 || !argument.slice(separator + 1))) {
            throw new Error('Invalid argument: ' + argument);
        }
        args[key] = separator < 0 ? true : argument.slice(separator + 1);
    }
    if (!args['--instant'] || ['--gateway', '--account', '--report'].some(key => !path.isAbsolute(args[key] || ''))) {
        throw new Error('Required absolute --gateway, --account, --report and --instant');
    }
    const root = path.resolve(__dirname, '../../..');
    const certificates = path.join(root, 'src/NetWorkTest/testdata/tls');
    const directory = path.join(path.dirname(args['--report']), 'owned-' + crypto.randomBytes(8).toString('hex'));
    fs.mkdirSync(directory, { recursive: true });
    const secret = crypto.randomBytes(32).toString('hex');
    const secretFile = path.join(directory, 'account-secret.txt');
    const servicesFile = path.join(directory, 'gateway-services.txt');
    const database = createMysqlTestConfig(args['--mysql-config'], path.join(directory, 'AccountServer.ini'));
    fs.writeFileSync(secretFile, secret, { flag: 'wx', mode: 0o600 });
    fs.writeFileSync(servicesFile, '0=' + secret, { flag: 'wx', mode: 0o600 });
    const children = new Set();
    const report = { engine: 'actual Windows gateway and AccountServer IPC/TLS', cases: [], artifacts: directory };
    let cancelled = false;
    const cancel = () => {
        cancelled = true;
        for (const child of children) fs.writeFileSync(child.stopFile, 'cancel');
    };
    process.on('SIGINT', cancel);
    process.on('SIGTERM', cancel);

    function start(executable, options, expression, name) {
        const stopFile = path.join(directory, name + '-stop.txt');
        const child = spawn(executable, options.concat(['--stop-file=' + stopFile, '--run-for-ms=30000']),
            { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'], cwd: path.dirname(executable) });
        const record = { child, stopFile, stdout: '', stderr: '', exited: false };
        let readyResolve;
        let readyReject;
        record.ready = new Promise((resolve, reject) => { readyResolve = resolve; readyReject = reject; });
        record.exit = new Promise((resolve, reject) => {
            child.on('error', error => { readyReject(error); reject(error); });
            child.on('exit', (code, signal) => {
                record.exited = true;
                readyReject(new Error(`${name} exited ${code} ${signal}: ${record.stderr}`));
                resolve({ code, signal });
            });
        });
        record.exit.catch(() => {});
        record.ready.catch(() => {});
        child.stdout.on('data', bytes => {
            record.stdout += bytes.toString();
            const match = expression.exec(record.stdout);
            if (match) readyResolve(match);
        });
        child.stderr.on('data', bytes => { record.stderr += bytes.toString(); });
        children.add(record);
        return record;
    }

    async function stop(record, name) {
        const began = performance.now();
        fs.writeFileSync(record.stopFile, 'stop');
        // The legacy IPC host takes about five seconds to drain; keep a bounded
        // deadline with cleanup margin, as in the account recovery process test.
        const result = await bounded(record.exit, 12000, name + ' graceful exit');
        assert.equal(result.code, 0, record.stderr);
        assert.equal(result.signal, null);
        children.delete(record);
        report[name] = { pid: record.child.pid, stopElapsedMs: Math.round(performance.now() - began),
            stdout: record.stdout, stderr: record.stderr };
    }

    const began = performance.now();
    const entry = { id: 1, name: '实际账号服务认证注册、重启恢复与有序退出', state: 'PASS' };
    try {
        const gateway = start(args['--gateway'], ['--protocol=2', '--listen=127.0.0.1', '--port=0', '--services=' + servicesFile,
            '--certificate=' + path.join(certificates, 'server2-sha256.crt'),
            '--private-key=' + path.join(certificates, 'server2.key')], /GATEWAY_READY port=(\d+) pid=(\d+)/, 'gateway');
        const listening = await bounded(gateway.ready, 10000, 'gateway registration startup');
        assert.equal(Number(listening[2]), gateway.child.pid);
        const options = ['--protocol=2', '--database=' + database, '--authority=' + path.join(certificates, 'test-ca-sha256.crt'),
            '--hostname=localhost', '--secret-file=' + secretFile, '--gateway-pid=' + gateway.child.pid];
        const first = start(args['--account'], options, /ACCOUNT_READY endpoint=(\d+) pid=(\d+)/, 'account-first');
        const authenticated = await bounded(first.ready, 10000, 'actual AccountServer registration');
        assert.equal(Number(authenticated[2]), first.child.pid);
        assert(fs.statSync(database).size > 0);
        const previous = Number(authenticated[1]);
        await stop(first, 'account-first');
        assert(first.stdout.includes('ACCOUNT_STOPPED'));
        const second = start(args['--account'], options, /ACCOUNT_READY endpoint=(\d+) pid=(\d+)/, 'account-second');
        const replaced = await bounded(second.ready, 10000, 'AccountServer reopen and register');
        assert(Number(replaced[1]) > previous, 'new service reused old gateway endpoint');
        assert.equal(Number(replaced[2]), second.child.pid);
        assert.equal(cancelled, false);
        await stop(second, 'account-second');
        await stop(gateway, 'gateway');
    } catch (error) {
        entry.state = cancelled ? 'CANCELLED' : 'FAIL';
        entry.error = error.stack;
    } finally {
        for (const record of children) {
            try {
                fs.writeFileSync(record.stopFile, 'test-finish');
                await bounded(record.exit, 12000, 'owned child cleanup');
            } catch (_) {
                if (!record.exited) record.child.kill();
            }
            report['unfinished-' + record.child.pid] = { stdout: record.stdout, stderr: record.stderr };
        }
        process.off('SIGINT', cancel);
        process.off('SIGTERM', cancel);
    }
    entry.elapsed_ms = Math.round(performance.now() - began);
    report.cases.push(entry);
    fs.writeFileSync(args['--report'], JSON.stringify(report, null, 2));
    console.log(`${entry.id} ${entry.name} ${entry.state}`);
    if (entry.error) console.error(entry.error);
    process.exitCode = entry.state === 'PASS' ? 0 : 1;
}

module.exports = { createMysqlTestConfig, probePasswordRoute };

if (require.main === module) {
    main().catch(error => { console.error(error.stack); process.exitCode = 2; });
}