'use strict';
const { createMysqlTestConfig } = require('./account_service_process_test');
// Basic real-process route only. This does not test product integration or files.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { spawn } = require('node:child_process');
const { performance } = require('node:perf_hooks');

async function bounded(promise, ms, label) {
    let timer;
    try {
        return await Promise.race([promise, new Promise((resolve, reject) => {
            timer = setTimeout(() => reject(new Error(label + ' timeout')), ms);
        })]);
    } finally { clearTimeout(timer); }
}

async function main() {
    const options = {};
    for (const argument of process.argv.slice(2)) {
        const index = argument.indexOf('=');
        const key = index < 0 ? argument : argument.slice(0, index);
        if (!['--gateway', '--client-manager', '--probe', '--report', '--instant', '--files', '--account-server', '--mysql-config'].includes(key) ||
            Object.hasOwn(options, key) || (key === '--instant' || key === '--files' ? index >= 0 : index < 0 || index + 1 === argument.length)) {
            throw new Error('Invalid argument: ' + argument);
        }
        options[key] = index < 0 ? true : argument.slice(index + 1);
    }
    if (!options['--instant'] || ['--gateway', '--client-manager', '--probe', '--report']
        .some(key => !path.isAbsolute(options[key] || ''))) throw new Error('Absolute paths and --instant required');
    if (options['--account-server'] && !path.isAbsolute(options['--account-server'])) throw new Error('Absolute AccountServer path required');
    if (options['--account-server'] && options['--files']) throw new Error('Choose one basic route');
    const root = path.resolve(__dirname, '../../..');
    const certificates = path.join(root, 'src/NetWorkTest/testdata/tls');
    const directory = path.join(path.dirname(options['--report']), 'owned-' + crypto.randomBytes(8).toString('hex'));
    fs.mkdirSync(directory, { recursive: true });
    const services = path.join(directory, 'test-services.txt');
    const device = path.join(directory, 'test-device.txt');
    const credential = product => Buffer.from(Array.from({ length: 32 }, (_, index) =>
        (index * 7 + product * 31 + 1) & 255)).toString('hex');
    fs.writeFileSync(services, '0=' + credential(0) + '\n4=' + credential(4), { flag: 'wx', mode: 0o600 });
    fs.writeFileSync(device, '42'.repeat(16), { flag: 'wx' });
    const accountSecret = path.join(directory, 'test-account-secret.txt');
    if (options['--account-server']) fs.writeFileSync(accountSecret, credential(0), { flag: 'wx', mode: 0o600 });
    const report = { engine: 'actual desktop IPC + client manager + TCP + server manager + two services',
        artifacts: directory, cases: [], processes: [] };
    const children = [];
    let cancelled = false;
    const cancel = () => {
        cancelled = true;
        for (const record of children) if (record.stopFile && !record.exited) fs.writeFileSync(record.stopFile, 'cancel');
    };
    process.on('SIGINT', cancel);
    process.on('SIGTERM', cancel);

    function start(executable, args, name, expression, hasStop) {
        const stopFile = hasStop ? path.join(directory, name + '-stop.txt') : null;
        if (hasStop) args = args.concat(['--stop-file=' + stopFile, '--run-for-ms=30000']);
        const child = spawn(executable, args, { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'], cwd: path.dirname(executable) });
        const record = { child, name, stopFile, stdout: '', stderr: '', exited: false };
        let readyResolve;
        let readyReject;
        record.ready = new Promise((resolve, reject) => { readyResolve = resolve; readyReject = reject; });
        record.exit = new Promise((resolve, reject) => {
            child.on('error', error => { readyReject(error); reject(error); });
            child.on('exit', (code, signal) => {
                record.exited = true;
                readyReject(new Error(name + ' exited before ready: ' + code + ' ' + record.stderr));
                resolve({ code, signal });
            });
        });
        record.ready.catch(() => {});
        record.exit.catch(() => {});
        child.stdout.on('data', bytes => {
            record.stdout += bytes.toString();
            const match = expression && expression.exec(record.stdout);
            if (match) readyResolve(match);
        });
        child.stderr.on('data', bytes => { record.stderr += bytes.toString(); });
        children.push(record);
        return record;
    }

    async function normalExit(record, timeout = 5000) {
        if (record.stopFile && !record.exited) fs.writeFileSync(record.stopFile, 'stop');
        const result = await bounded(record.exit, timeout, record.name + ' exit');
        assert.equal(result.code, 0, record.name + ': ' + record.stdout + record.stderr);
        assert.equal(result.signal, null);
    }

    const began = performance.now();
    const entry = { id: options['--files'] ? 68 : options['--account-server'] ? 67 : 61,
        name: options['--files'] ? '实际网关上传、下载及文件提交' :
            options['--account-server'] ? '实际账号服务注册、查询、充值及VIP完整往返' : '实际桌面客户端完整网关和两级服务往返', state: 'PASS' };
    try {
        const gateway = start(options['--gateway'], ['--listen=127.0.0.1', '--port=0', '--services=' + services,
            '--protocol=2', '--certificate=' + path.join(certificates, 'server2-sha256.crt'),
            '--private-key=' + path.join(certificates, 'server2.key')], 'gateway', /GATEWAY_READY port=(\d+) pid=(\d+)/, true);
        const listening = await bounded(gateway.ready, 10000, 'gateway ready');
        assert.equal(Number(listening[2]), gateway.child.pid);
        const common = options['--files'] ? null : options['--account-server'] ? start(options['--account-server'], [
            '--database=' + createMysqlTestConfig(options['--mysql-config'], path.join(directory, 'AccountServer.ini')), '--gateway-pid=' + gateway.child.pid,
            '--authority=' + path.join(certificates, 'test-ca-sha256.crt'), '--secret-file=' + accountSecret],
            'actual-account-server', /ACCOUNT_READY endpoint=(\d+) pid=(\d+)/, true) :
            start(options['--probe'], ['--standalone-service=' + gateway.child.pid, '--service-product=0'],
                'common-test-service', /TEST_SERVICE_READY product=0/, false);
        const service = options['--files'] ? start(options['--probe'], ['--file-service=' + gateway.child.pid],
            'file-test-service', /FILE_SERVICE_READY/, false) : start(options['--probe'],
            ['--standalone-service=' + gateway.child.pid, '--service-product=4'], 'product-test-service', /TEST_SERVICE_READY product=4/, false);
        await bounded(Promise.all(common ? [common.ready, service.ready] : [service.ready]), 10000, 'services ready');
        const client = start(options['--client-manager'], ['--protocol=2', '--address=127.0.0.1', '--port=' + listening[1],
            '--hostname=localhost', '--authority=' + path.join(certificates, 'test-ca-sha256.crt'),
            '--certificate=' + path.join(certificates, 'server2-sha256.crt'),
            '--private-key=' + path.join(certificates, 'server2.key'), '--device-file=' + device],
            'client-manager', /CLIENT_GATEWAY_READY pid=(\d+)/, true);
        const clientReady = await bounded(client.ready, 10000, 'client manager ready');
        assert.equal(Number(clientReady[1]), client.child.pid);
        const desktop = start(options['--probe'], [(options['--files'] ? '--file-client=' : options['--account-server'] ? '--account-client=' : '--desktop-client=') + client.child.pid],
            'desktop-app-probe', null, false);
        await normalExit(desktop, options['--files'] ? 23000 : options['--account-server'] ? 18000 : 12000);
        assert(desktop.stdout.includes(options['--files'] ? 'completed=2 bad=0' : options['--account-server'] ?
            'results=5 closes=1 bad=0' : 'code=0 results=1 closes=1 bad=0'));
        assert.equal(cancelled, false);
        if (common) await normalExit(common, 12000);
        await normalExit(service, 12000);
        await normalExit(client);
        await normalExit(gateway);
    } catch (error) {
        entry.state = cancelled ? 'CANCELLED' : 'FAIL';
        entry.error = error.stack;
    } finally {
        for (const record of children) {
            if (!record.exited) {
                try {
                    if (record.stopFile) fs.writeFileSync(record.stopFile, 'finish');
                    await bounded(record.exit, 12000, 'owned cleanup');
                } catch (_) { if (!record.exited) record.child.kill(); }
            }
            report.processes.push({ name: record.name, pid: record.child.pid,
                stdout: record.stdout, stderr: record.stderr, exited: record.exited });
        }
        process.off('SIGINT', cancel);
        process.off('SIGTERM', cancel);
    }
    entry.elapsed_ms = Math.round(performance.now() - began);
    report.cases.push(entry);
    fs.writeFileSync(options['--report'], JSON.stringify(report, null, 2));
    console.log(entry.id + ' ' + entry.name + ' ' + entry.state);
    if (entry.error) console.error(entry.error);
    process.exitCode = entry.state === 'PASS' ? 0 : 1;
}

main().catch(error => { console.error(error.stack); process.exitCode = 2; });