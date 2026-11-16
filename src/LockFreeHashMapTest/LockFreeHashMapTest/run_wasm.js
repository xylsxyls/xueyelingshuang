'use strict';

// Wasm 仅由此脚本编译和启动；用例、数据校验、故障注入均来自相同的 C++ 源码
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const child = require('child_process');

/** 执行一个编译器或 Node 子进程，保留独占原始日志和有限超时 */
function run(program, args, directory, logName, env, timeout, report)
{
    const log = path.join(directory, logName);
    const descriptor = fs.openSync(log, 'wx');
    let result;
    const begin = Date.now();
    try
    {
        result = child.spawnSync(program, args, { cwd: directory, env, timeout, windowsHide: true,
            stdio: ['ignore', descriptor, descriptor], killSignal: 'SIGKILL' });
        fs.fsyncSync(descriptor);
    }
    finally
    {
        fs.closeSync(descriptor);
    }
    const record = { program, args, log, code: result.status, error: result.error ? result.error.message : '',
        signal: result.signal, seconds: (Date.now() - begin) / 1000 };
    report.runs.push(record);
    fs.writeFileSync(path.join(report.output, 'results.json'), JSON.stringify(report, null, 2));
    process.stdout.write(logName + ' code=' + record.code + '\n');
    return result.status === 0 && !result.error && !result.signal;
}

/** 检查有序且唯一的用例身份、资源行、终态及非零检查数，拒绝相互矛盾的结果 */
function verifyCase(log, id)
{
    let stage = 0;
    for (const line of log.split(/\r?\n/))
    {
        if (line.startsWith('BEGIN '))
        {
            if (stage !== 0 || line !== 'BEGIN id=' + id) return false;
            stage = 1;
        }
        else if (line.startsWith('RESOURCES '))
        {
            const deltas = line.split(/\s+/).filter(field => field.startsWith('allocation_delta='));
            if (stage !== 1 || deltas.length !== 1 || deltas[0] !== 'allocation_delta=0') return false;
            stage = 2;
        }
        else if (line.startsWith('END '))
        {
            if (stage !== 2 || line !== 'END id=' + id + ' result=PASS') return false;
            stage = 3;
        }
        else if (line.startsWith('RESULT '))
        {
            if (stage !== 3 || !/^RESULT checks=[1-9]\d* failed=0$/.test(line)) return false;
            stage = 4;
        }
    }
    return stage === 4;
}

/** 组合模块的每条汇总均须通过，指定资源计数只能出现一次并为零 */
function verifyModule(log, mode)
{
    const lines = log.split(/\r?\n/);
    const summaries = lines.filter(line => line.startsWith('MODULE_RESULT '));
    if (summaries.length === 0 || summaries.some(line => !/^MODULE_RESULT checks=[1-9]\d* failed=0$/.test(line))) return false;
    const counters = mode === 'epoch' ? [] : mode === 'pool' ? ['POOL_ALLOCATION_DELTA'] : ['POOL_ALLOCATION_DELTA', 'REWRITE_ALLOCATION_DELTA'];
    return counters.every(name =>
    {
        const values = lines.filter(line => line.startsWith(name + ' '));
        return values.length === 1 && values[0] === name + ' 0';
    });
}

/** 日志判定反例不启动被测库，证明失败、重复终态及缺少资源行不能误报通过 */
function selftest()
{
    const valid = 'BEGIN id=1\nRESOURCES allocation_delta=0 cpu_seconds=0\nEND id=1 result=PASS\nRESULT checks=1 failed=0\n';
    if (!verifyCase(valid, 1) || verifyCase(valid, 2)) throw new Error('用例身份自检失败');
    for (const invalid of [valid + 'END id=1 result=FAIL\n', valid + 'RESULT checks=1 failed=0\n',
        valid.replace('allocation_delta=0', 'allocation_delta=0 allocation_delta=1'), valid.replace('checks=1', 'checks=0'),
        valid.replace('RESOURCES allocation_delta=0 cpu_seconds=0\n', '')])
    {
        if (verifyCase(invalid, 1)) throw new Error('错误用例日志被接受');
    }
    const module = 'MODULE_RESULT checks=2 failed=0\nPOOL_ALLOCATION_DELTA 0\nREWRITE_ALLOCATION_DELTA 0\n';
    if (!verifyModule(module, 'rewrite') || verifyModule('MODULE_RESULT checks=1 failed=1\n' + module, 'rewrite') ||
        verifyModule(module + 'POOL_ALLOCATION_DELTA 1\n', 'rewrite') || verifyModule('MODULE_RESULT checks=0 failed=0\n', 'epoch'))
    {
        throw new Error('模块日志自检失败');
    }
    process.stdout.write('WASM_RUNNER_SELFTEST passed=1\n');
    return 0;
}

/** 校验限定输出根，隔离缓存，编译八种 Wasm 配置并检查完整用例结束证据 */
function main()
{
    if (process.argv.length !== 6 && process.argv.length !== 7)
    {
        throw new Error('用法：node run_wasm.js EMSDK NODE PYTHON OUTPUT [epoch|pool|rewrite]');
    }
    const mode = process.argv[6] || 'cases';
    if (!['cases', 'epoch', 'pool', 'rewrite'].includes(mode)) throw new Error('未知模块');
    const [sdk, node, python, output] = process.argv.slice(2, 6).map(value => path.resolve(value));
    const root = path.resolve(__dirname, '../../temp/LockFreeHashMapTest');
    const relative = path.relative(root, output);
    if (!relative || relative.startsWith('..') || path.isAbsolute(relative) || fs.existsSync(output))
    {
        throw new Error('输出必须是 src/temp/LockFreeHashMapTest 下尚不存在的子目录');
    }
    const emscripten = path.join(sdk, 'upstream/emscripten');
    const cacheSeed = fs.realpathSync(process.env.LOCKFREEHASHMAP_TEST_CACHE || path.join(emscripten, 'cache'));
    const cacheRelative = path.relative(cacheSeed, output);
    if (!cacheRelative || (!cacheRelative.startsWith('..') && !path.isAbsolute(cacheRelative)))
    {
        throw new Error('输出不能位于缓存种子内部');
    }
    // 磁盘受限时可复用本测试工程已有缓存，禁止借此写入共享 SDK 缓存。
    const reuseCache = process.env.LOCKFREEHASHMAP_TEST_REUSE_CACHE === '1';
    if (reuseCache)
    {
        const cacheWithinTests = path.relative(fs.realpathSync(root), cacheSeed);
        if (!cacheWithinTests || cacheWithinTests.startsWith('..') || path.isAbsolute(cacheWithinTests))
        {
            throw new Error('复用缓存必须位于本测试工程临时目录内部');
        }
    }
    fs.mkdirSync(output, { recursive: true });
    const cache = reuseCache ? cacheSeed : path.join(output, 'cache');
    if (!reuseCache) fs.cpSync(cacheSeed, cache, { recursive: true, errorOnExist: true });
    const config = path.join(output, 'emscripten.config');
    const quote = value => JSON.stringify(value.replace(/\\/g, '/'));
    fs.writeFileSync(config, 'LLVM_ROOT = ' + quote(path.join(sdk, 'upstream/bin')) + '\nBINARYEN_ROOT = ' +
        quote(path.join(sdk, 'upstream')) + '\nNODE_JS = ' + quote(node) + '\n');
    const env = { ...process.env, EM_CONFIG: config, EM_CACHE: cache, PYTHONDONTWRITEBYTECODE: '1' };
    const library = path.resolve(__dirname, '../../LockFreeHashMap/LockFreeHashMap/src');
    const names = mode === 'cases' ? ['LockFreeHashMapTests', 'LockFreeHashMapMigration', 'LockFreeHashMapBoundaries',
        'LockFreeHashMapOptimizations', 'LockFreeHashMapRetryTests', 'LockFreeHashMapTestThread', 'LockFreeHashMapTest'] :
        mode === 'rewrite' ? ['LockFreeHashMapRewriteTests', 'LockFreeHashMapPoolTests', 'LockFreeHashMapTestThread'] :
        [mode === 'epoch' ? 'LockFreeHashMapEpochTests' : 'LockFreeHashMapPoolTests', 'LockFreeHashMapTestThread'];
    const inputs = names.map(name => path.join(__dirname, name + '.cpp'));
    if (mode !== 'pool') inputs.push(path.join(__dirname, 'LockFreeHashMapTestState.cpp'));
    const hashes = {};
    for (const directory of [__dirname, library])
    {
        for (const name of fs.readdirSync(directory))
        {
            if (/\.(h|cpp|inl|js)$/.test(name))
            {
                const file = path.join(directory, name);
                hashes[file] = crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
            }
        }
    }
    const report = { output, mode, cacheSeed, cache, reuseCache, compiler: fs.readFileSync(path.join(emscripten, 'emscripten-version.txt'), 'utf8').trim(),
        node: process.version, hashes, runs: [], complete: false, passed: false };
    let passed = true;
    for (const bits of [32, 64])
    {
        for (const threads of [false, true])
        {
            for (const build of ['Debug', 'Release'])
            {
                const directory = path.join(output, 'wasm' + bits + '-' + (threads ? 'pthreads' : 'single') + '-' + build);
                fs.mkdirSync(directory);
                // Python 在此仅执行 Emscripten 自带编译器，不承载任何测试逻辑
                const args = ['-B', path.join(emscripten, 'em++.py'), '-std=c++11', build === 'Debug' ? '-O0' : '-O2',
                    '-Wall', '-Wextra', '-Werror', '-fexceptions', '-DSTATIC_LIB',
                    '-I' + library, '-sDISABLE_EXCEPTION_CATCHING=0', '-sENVIRONMENT=node', '-sEXIT_RUNTIME=1',
                    '-sSTACK_SIZE=1048576', '-sINITIAL_MEMORY=134217728', '-sABORTING_MALLOC=0'];
                if (build === 'Debug') args.push('-g');
                if (mode === 'rewrite') args.push('-DLOCKFREEHASHMAP_POOL_NO_MAIN');
                if (bits === 64) args.push('-sMEMORY64=1');
                if (threads) args.push('-pthread', '-sPTHREAD_POOL_SIZE=16', '-sPROXY_TO_PTHREAD=1');
                else args.push('-DLOCKFREEHASHMAP_TEST_SINGLE_THREAD', '-sALLOW_MEMORY_GROWTH=1', '-sMAXIMUM_MEMORY=536870912');
                args.push(...inputs, '-o', path.join(directory, 'tests.js'));
                if (!run(python, args, directory, 'build.log', env, 300000, report))
                {
                    passed = false;
                    continue;
                }
                for (const id of mode === 'cases' ? (threads ? [1,2,3,4,5,6,7,8,9,10,11] : [1,2,3,4,9,11]) : [1])
                {
                    const name = 'test-' + id + '.log';
                    const exited = run(node, [path.join(directory, 'tests.js'), String(id)], directory, name, env, 120000, report);
                    const log = fs.readFileSync(path.join(directory, name), 'utf8');
                    const verified = mode === 'cases' ? verifyCase(log, id) : verifyModule(log, mode);
                    report.runs[report.runs.length - 1].verified = verified;
                    passed = exited && verified && passed;
                    fs.writeFileSync(path.join(report.output, 'results.json'), JSON.stringify(report, null, 2));
                }
            }
        }
    }
    report.changed = Object.keys(hashes).filter(file => hashes[file] !== crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex'));
    report.complete = true;
    report.passed = report.changed.length === 0 && passed;
    fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify(report, null, 2));
    return report.passed ? 0 : 1;
}

try
{
    process.exitCode = process.argv.length === 3 && process.argv[2] === '--selftest' ? selftest() : main();
}
catch (error)
{
    process.stderr.write('WASM_RUNNER ' + error.message + '\n');
    process.exitCode = 2;
}