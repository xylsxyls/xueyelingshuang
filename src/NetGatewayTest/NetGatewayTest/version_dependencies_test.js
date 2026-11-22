'use strict';

// 执行真实version脚本，只替换构建命令以记录参数；不编译或发布业务产品。
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const { spawnSync } = require('node:child_process');

function run(repository, bash, output) {
    assert.ok(path.isAbsolute(repository) && path.isAbsolute(bash) && path.isAbsolute(output));
    assert.ok(!fs.existsSync(output), 'Use a new test directory');
    fs.mkdirSync(output, { recursive: true });
    const cloud = path.join(output, 'cloud.sh');
    fs.writeFileSync(cloud, '#!/bin/sh\nprintf "%s\\t%s\\t%s\\t%s\\t%s\\n" "$@" >> "$NET_TEST_COMMANDS"\n' +
        'if [ "$1" = "$NET_TEST_FAIL" ]; then exit 19; fi\n', { flag: 'wx' });
    fs.chmodSync(cloud, 0o700);
    const core = ['CStringManager', 'CSystem', 'AtomicMath', 'Semaphore', 'SharedMemory', 'ReadWriteMutex',
        'LockFreeQueue', 'LockFreeHashMap', 'CTaskThreadManager', 'Compress', 'CorrespondParam', 'CRandom',
        'ProcessWork', 'CDump', 'Variant', 'ProtoMessageManager', 'NetChannel', 'CEncodeDecode', 'NetInterface',
        'LibuvTcp', 'NetPeer', 'NetWork', 'NetSender'];
    const products = ['AccountServer', 'AccountServerTest', 'CookServer', 'SplitViewerServer',
        'SplitViewerServerTest', 'PdfReaderServer', 'PdfReaderServerTest'];
    const records = [];
    for (const product of products) for (const config of ['debug', 'release']) {
        const script = path.join(repository, 'src', product, 'version_' + config + '.sh');
        const syntax = spawnSync(bash, ['-n', script], { timeout: 15000, windowsHide: true, encoding: 'utf8' });
        assert.equal(syntax.status, 0, syntax.stderr);
        const calls = (bits) => {
            const libraries = core.slice();
            if (product.startsWith('Account')) libraries.push('Cini', 'MysqlCpp', 'Account');
            if (product === 'CookServer') libraries.push('Cini', 'IntDateTime', 'LogManager', 'RapidJsonCpp',
                'HttpService', 'MysqlCpp', 'FFmpegCpp', 'HiRedis');
            if (product === 'PdfReaderServerTest') libraries.push('Account', 'IntDateTime', 'LogManager', 'PdfReaderCore');
            return libraries.map(library => {
                const dll = library === 'MysqlCpp' || library === 'HiRedis';
                const only = dll || product === 'PdfReaderServerTest' &&
                    ['IntDateTime', 'LogManager', 'PdfReaderCore'].includes(library);
                return [library, String(bits), dll ? 'dll' : 'lib', config, only ? 'only' : 'same'];
            });
        };
        const execute = (label, args, expected, fail = '') => {
            const log = path.join(output, product + '-' + config + '-' + label + '.txt');
            const result = spawnSync(bash, ['-c',
                'uname() { printf "%s\\n" MINGW64_NT; }; export -f uname; script="$1"; shift; source "$script" "$@"',
                'version-test', script, ...args], { timeout: 15000, windowsHide: true, encoding: 'utf8',
                env: { ...process.env, CLOUD_REBUILD: cloud.replace(/\\/g, '/'),
                    NET_TEST_COMMANDS: log.replace(/\\/g, '/'), NET_TEST_FAIL: fail } });
            assert.equal(result.status, expected, result.stderr || String(result.error));
            const actual = fs.existsSync(log) ? fs.readFileSync(log, 'utf8').trim().split(/\r?\n/).map(line => line.split('\t')) : [];
            records.push({ product, config, label, status: result.status, commands: actual });
            return actual;
        };
        const fixed64 = product.startsWith('PdfReader');
        const defaultBits = product.startsWith('SplitViewer') && config === 'debug' ? 32 : 64;
        assert.deepEqual(execute('default', [], 0), calls(defaultBits));
        for (const [platform, bits] of [['Win32', 32], ['x64', 64]]) {
            assert.deepEqual(execute(platform, [platform], 0), calls(fixed64 ? 64 : bits));
        }
        if (!fixed64) assert.deepEqual(execute('invalid', ['wrong'], 2), []);
        const failed = execute('failure', [], product === 'PdfReaderServerTest' ? 1 : 19, 'LibuvTcp');
        assert.ok(failed.some(command => command[0] === 'LibuvTcp'));
        if (product !== 'PdfReaderServerTest') assert.equal(failed.at(-1)[0], 'LibuvTcp');
        else assert.deepEqual(failed, calls(64));
    }
    fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify({ passed: true, cases: records.length, records }, null, 2), { flag: 'wx' });
    console.log('VERSION_DEPENDENCIES_PASS cases=' + records.length);
}

if (require.main === module) {
    try {
        assert.equal(process.argv.length, 5, 'Arguments: repository bash output');
        run(...process.argv.slice(2));
    } catch (error) {
        console.error(error.stack || error);
        process.exitCode = 1;
    }
}

module.exports = run;
