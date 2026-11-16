'use strict';

// 测试JS进程观察器本身；使用真实管道、退出码和独立固定期望，不打开网络端口。
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { start } = require('./local_link_gateway_process_test');

async function main() {
    const args = process.argv.slice(2);
    assert.equal(args.length, 1, 'Usage: process_host_test.js --output=<fresh absolute directory>');
    assert.ok(args[0].startsWith('--output='));
    const output = args[0].slice(9);
    assert.ok(path.isAbsolute(output) && !fs.existsSync(output));
    fs.mkdirSync(output, { recursive: true });
    let cancelled = false;
    const children = [];
    const results = [];
    const launch = (name, code, readiness = /PROBE_READY pid=(\d+)/) => {
        assert.ok(!cancelled, 'Suite already cancelled');
        const child = start(process.execPath, ['-e', code], name, output, readiness);
        children.push(child);
        return child;
    };
    const check = async (name, test) => {
        if (cancelled) throw new Error('Suite already cancelled');
        try { await test(); results.push({ name, state: 'PASS' }); }
        catch (error) { results.push({ name, state: 'FAIL', error: String(error.stack || error) }); }
    };
    let deadline;
    const body = async () => {
        await check('flat WeChat sample packs every asset without replacing existing output', async () => {
            const { prepareSampleProject } = require('./wx_smoke_test');
            const project = path.join(output, 'wx-project');
            fs.mkdirSync(project);
            prepareSampleProject(project);
            const app = JSON.parse(fs.readFileSync(path.join(project, 'app.json'), 'utf8'));
            assert.deepEqual(app.pages, ['pages/index/index']);
            const files = [['NetWxSampleApp.js', 'app.js'], ['NetWxSampleApp.json', 'app.json'],
                ...['js', 'json', 'wxml', 'wxss'].map(extension => ['NetWxSamplePage.' + extension, 'pages/index/index.' + extension])];
            for (const [source, destination] of files)
                assert.deepEqual(fs.readFileSync(path.join(project, destination)), fs.readFileSync(path.join(__dirname, source)));
            assert.throws(() => prepareSampleProject(project), /empty owned/);
            assert.throws(() => prepareSampleProject('relative'), /empty owned/);
        });
        await check('Account fixture imports without a repository SDK', async () => {
            const { NetAccountClient } = require('./NetAccountClient');
            assert.throws(() => new NetAccountClient(null, 101), /Matching Account protocol/);
            assert.equal(typeof require('./account_service_process_test').probePasswordRoute, 'function');
        });
        await check('Account constants follow the selected SDK and reject invalid metadata', async () => {
            const { readAccountProtocol } = require('./read_wasm');
            assert.throws(() => readAccountProtocol('relative/AccountAPI.js'), /absolute/);
            for (const [name, metadata, valid] of [
                ['sdk-a', { ACCOUNT_LOGOUT: 37, ACCOUNT_RESET_PASSWORD: 41 }, true],
                ['sdk-b', { ACCOUNT_LOGOUT: 53, ACCOUNT_RESET_PASSWORD: 59 }, true],
                ['sdk-invalid', { ACCOUNT_LOGOUT: 37, ACCOUNT_RESET_PASSWORD: '41' }, false]
            ]) {
                const sdk = path.join(output, name);
                fs.mkdirSync(sdk);
                fs.writeFileSync(path.join(sdk, 'AccountProtocol.js'), 'module.exports=' + JSON.stringify(metadata));
                const entry = path.join(sdk, 'AccountAPI.js');
                if (valid) assert.deepEqual(readAccountProtocol(entry), metadata);
                else assert.throws(() => readAccountProtocol(entry), /Invalid Account protocol/);
            }
            assert.throws(() => readAccountProtocol(path.join(output, 'missing', 'AccountAPI.js')), /Cannot find module/);
        });
        await check('fragmented complete readiness identity', async () => {
            const process = launch('fragment', "process.stdout.write('PROBE_READY pid=12'); setTimeout(() => process.stdout.write('345\\n'), 100)");
            assert.equal((await process.ready)[1], '12345');
            assert.equal((await process.completed).code, 0);
        });
        await check('UTF8 split across pipe writes', async () => {
            const process = launch('unicode', "process.stdout.write(Buffer.from([0xe4])); setTimeout(() => process.stdout.write(Buffer.from([0xb8,0xad,10])), 50)", /中/);
            assert.equal((await process.completed).stdout, '中\n');
            await process.ready;
        });
        await check('record observer installed after child exit', async () => {
            const process = launch('late', "console.log('STATS {\"value\":123456}')", /STATS /);
            assert.equal((await process.completed).code, 0);
            assert.deepEqual(await process.record('STATS'), { value: 123456 });
        });
        await check('record split across pipe writes', async () => {
            const process = launch('split', "process.stdout.write('STATS {\"value\":12'); setTimeout(() => process.stdout.write('3456}\\n'), 50)", /STATS /);
            assert.deepEqual(await process.record('STATS'), { value: 123456 });
            assert.equal((await process.completed).code, 0);
        });
        await check('missing record and readiness rejected', async () => {
            const process = launch('missing', 'process.exitCode=7');
            assert.equal((await process.completed).code, 7);
            await assert.rejects(process.ready, /before readiness/);
            await assert.rejects(process.record('STATS'), /missing/);
        });
        await check('unterminated record rejected', async () => {
            const process = launch('unterminated', "process.stdout.write('STATS {\"value\":1}')");
            await process.completed;
            await assert.rejects(process.record('STATS'), /missing/);
        });
        await check('malformed record rejected', async () => {
            const process = launch('malformed', "console.log('STATS {wrong}')");
            await assert.rejects(process.record('STATS'), SyntaxError);
            await process.completed;
        });
        await check('duplicate record rejected', async () => {
            const process = launch('duplicate', "console.log('STATS {}\\nSTATS {}')");
            await process.completed;
            await assert.rejects(process.record('STATS'), /Duplicate/);
        });
        await check('spawn failure rejects all observations', async () => {
            const process = start(path.join(output, 'absent.exe'), [], 'absent', output, /READY/);
            children.push(process);
            await assert.rejects(process.completed, /ENOENT/);
            await assert.rejects(process.ready, /ENOENT/);
            await assert.rejects(process.record('STATS'), /ENOENT/);
        });
        await check('existing evidence cannot be overwritten', async () => {
            const file = path.join(output, 'occupied.json');
            fs.writeFileSync(file, 'keep');
            assert.throws(() => launch('occupied', 'process.exitCode=0'), /EEXIST/);
            assert.equal(fs.readFileSync(file, 'utf8'), 'keep');
        });
        await check('report write failure is not success', async () => {
            const process = launch('write-failure', 'setTimeout(() => process.exitCode=0, 100)');
            const file = path.join(output, 'write-failure.json');
            fs.unlinkSync(file);
            fs.mkdirSync(file);
            await assert.rejects(process.completed);
        });
    };
    try {
        await Promise.race([body(), new Promise((resolve, reject) => {
            deadline = setTimeout(() => reject(new Error('Process observer self-test deadline')), 15000);
        })]);
    } finally {
        cancelled = true;
        clearTimeout(deadline);
        for (const child of children) {
            if (child.child.exitCode === null && child.child.signalCode === null) child.child.kill();
        }
        await Promise.all(children.map(child => child.completed.catch(() => {})));
        fs.writeFileSync(path.join(output, 'result.json'), JSON.stringify({
            pass: results.length === 14 && results.every(result => result.state === 'PASS'), results
        }, null, 2));
    }
    for (const result of results) console.log(result.state, result.name, result.error || '');
    assert.equal(results.length, 14);
    assert.ok(results.every(result => result.state === 'PASS'));
}

if (require.main === module) main().catch(error => { console.error(error.stack || error); process.exitCode = 1; });