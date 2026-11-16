'use strict';

// 仅用于 Wasm 编译和启动；负载、时钟、校验仍来自与原生相同的 C++
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const child = require('child_process');

/** 执行独占日志子进程并保存准确命令，失败立即停止该轮验收 */
function run(program, args, directory, log, env, report)
{
    const descriptor = fs.openSync(log, 'wx');
    let result;
    try
    {
        result = child.spawnSync(program, args, { cwd: directory, env, timeout: 300000,
            windowsHide: true, stdio: ['ignore', descriptor, descriptor] });
        fs.fsyncSync(descriptor);
    }
    finally
    {
        fs.closeSync(descriptor);
    }
    report.runs.push({ program, args, log, code: result.status, signal: result.signal,
        error: result.error ? result.error.message : '' });
    fs.writeFileSync(path.join(report.output, 'results.json'), JSON.stringify(report, null, 2));
    if (result.status !== 0 || result.error || result.signal)
    {
        throw new Error('子进程失败：' + log);
    }
}

/** 严格读取唯一结果行，校验身份、实际调用数、时长和数据错误数 */
function parse(text, spec, repeats)
{
    const lines = text.split(/\r?\n/).filter(line => line.startsWith('PERF '));
    if (lines.length !== 1) throw new Error('结果行数量错误');
    const result = Object.create(null);
    for (const field of lines[0].slice(5).split(' '))
    {
        const parts = field.split('=');
        if (parts.length !== 2 || !parts[0] || !parts[1] || Object.prototype.hasOwnProperty.call(result, parts[0])) throw new Error('重复或错误字段');
        result[parts[0]] = parts[0] === 'name' ? parts[1] : Number(parts[1]);
        if (parts[0] !== 'name' && (!Number.isFinite(result[parts[0]]) || result[parts[0]] < 0)) throw new Error('非法数值');
    }
    const calls = spec.iterations * repeats * spec.calls_per_iteration;
    const iterations = spec.iterations / spec.threads;
    // 集合导出和clear逐次采样，其余场景在独立延迟批次中按127间隔采样。
    // operation是C++目录协议：10/11/12为keys/values/pairs，13为clear。
    const whole = spec.operation >= 10 && spec.operation <= 13;
    const samples = whole ? spec.iterations : spec.threads * (iterations < 2048 ? iterations : Math.floor((iterations - 1) / 127) + 1);
    const required = ['id', 'repeats', 'calls', 'seconds', 'ops', 'p50_us', 'p95_us', 'p99_us', 'samples', 'errors', 'extra_calls', 'attempted_calls', 'batch_min_s', 'batch_max_s', 'output_string_capacity', 'state_tail_padding', 'collection_validation', 'conditional_interfaces'];
    if (required.some(name => !Object.prototype.hasOwnProperty.call(result, name)) || result.id !== spec.id ||
        result.name !== spec.name || result.repeats !== repeats || result.calls !== calls || result.errors !== 0 || result.attempted_calls !== calls + result.extra_calls || result.output_string_capacity !== 512 || result.state_tail_padding !== 64 || result.collection_validation !== 2 ||
        ![0, 1].includes(result.conditional_interfaces) || (spec.operation >= 15 && result.conditional_interfaces !== 1) ||
        !Number.isSafeInteger(calls) || calls <= 0 || !Number.isSafeInteger(iterations) || iterations <= 0 ||
        !Number.isSafeInteger(result.extra_calls) || !Number.isSafeInteger(result.attempted_calls) ||
        result.seconds <= 0 || result.ops <= 0 || result.samples !== samples || result.p50_us > result.p95_us ||
        result.p95_us > result.p99_us || result.batch_min_s <= 0 || result.batch_min_s > result.batch_max_s ||
        result.seconds < result.batch_min_s * repeats * 0.999999 || result.seconds > result.batch_max_s * repeats * 1.000001 ||
        Math.abs(result.ops * result.seconds / calls - 1) > 0.000001)
    {
        throw new Error('结果合同校验失败');
    }
    return result;
}

/** 使用独立文本反例验证采样数、调用数和结果唯一性，不启动被测库 */
function selftest()
{
    const spec = { id: 1, name: 'fixture', iterations: 4096, threads: 2, calls_per_iteration: 1, operation: 0 };
    const valid = 'PERF id=1 name=fixture repeats=1 calls=4096 seconds=1 ops=4096 p50_us=1 p95_us=2 p99_us=3 samples=34 errors=0 extra_calls=0 attempted_calls=4096 batch_min_s=1 batch_max_s=1 output_string_capacity=512 state_tail_padding=64 collection_validation=2 conditional_interfaces=1';
    parse(valid, spec, 1);
    parse(valid.replace('conditional_interfaces=1', 'conditional_interfaces=0'), spec, 1);
    const conditional = { ...spec, operation: 15 };
    parse(valid, conditional, 1);
    let rejectedCapability = false;
    try { parse(valid.replace('conditional_interfaces=1', 'conditional_interfaces=0'), conditional, 1); }
    catch (error) { rejectedCapability = true; }
    if (!rejectedCapability) throw new Error('缺少条件接口的程序未被拒绝');
    for (const invalid of [valid.replace(' output_string_capacity=512', ''), valid.replace('output_string_capacity=512', 'output_string_capacity=0'),
        valid.replace(' state_tail_padding=64', ''), valid.replace('state_tail_padding=64', 'state_tail_padding=0'),
        valid.replace(' collection_validation=2', ''), valid.replace('collection_validation=2', 'collection_validation=1'),
        valid.replace(' conditional_interfaces=1', ''), valid.replace('conditional_interfaces=1', 'conditional_interfaces=2'),
        valid.replace('samples=34', 'samples=1'), valid.replace('extra_calls=0', 'extra_calls=0.5').replace('attempted_calls=4096', 'attempted_calls=4096.5'),
        valid + '\n' + valid, valid.replace(' errors=0', ''), valid.replace('seconds=1', 'seconds=0'), valid.replace('errors=0', 'errors=1'),
        valid.replace('batch_min_s=1 batch_max_s=1', 'batch_min_s=2 batch_max_s=2')])
    {
        let rejected = false;
        try { parse(invalid, spec, 1); } catch (error) { rejected = true; }
        if (!rejected) throw new Error('结果解析反例未被拒绝');
    }
    process.stdout.write('WASM_PERFORMANCE_SELFTEST passed=1\n');
    return 0;
}

/** 使用所有轮次的中位数，不剔除异常低值 */
function median(values)
{
    const sorted = [...values].sort((left, right) => left - right);
    return (sorted[Math.floor((sorted.length - 1) / 2)] + sorted[Math.floor(sorted.length / 2)]) / 2;
}

/** 为两版构建相同的 Wasm32/64 pthreads 程序，并进行五轮交替测量 */
function main()
{
    if (process.argv.length !== 9) throw new Error('参数：EMSDK NODE PYTHON ORIGINAL CURRENT CATALOGUE OUTPUT');
    const [sdk, node, python, original, current, catalogue, output] = process.argv.slice(2).map(value => path.resolve(value));
    const allowed = path.resolve(__dirname, '../../temp/LockFreeHashMapTest');
    const relative = path.relative(allowed, output);
    if (!relative || relative.startsWith('..') || path.isAbsolute(relative) || fs.existsSync(output)) throw new Error('输出须为独占测试临时目录');
    const csv = fs.readFileSync(catalogue, 'utf8').trim().split(/\r?\n/);
    const headers = csv.shift().split(',');
    const allSpecs = csv.map(line => Object.fromEntries(line.split(',').map((value, index) => [headers[index], headers[index] === 'name' ? value : Number(value)])));
    const names = ['int_r100_t1', 'int_r100_t4', 'int_r95_t4', 'int_r50_t4', 'shared_r100_t1', 'shared_r95_t4',
        'shared_r50_t4', 'string_key_r100_t1', 'string_key_r95_t4', 'int_miss_t4', 'shared_read_k4096',
        'string_key_r95_k4096', 'int_hash0_r95', 'string_key_hash0_read', 'wide64_r95', 'value512_r50',
        'int_churn', 'shared_churn', 'string_key_churn', 'int_clear', 'shared_insert_new', 'shared_erase',
        'shared_new_find', 'shared_new_find_compare'];
    const specs = names.map(name => allSpecs.find(spec => spec.name === name));
    if (specs.some(spec => !spec || spec.threads > 4)) throw new Error('负载目录不完整');
    const emscripten = path.join(sdk, 'upstream/emscripten');
    const cache = fs.realpathSync(path.join(emscripten, 'cache'));
    fs.mkdirSync(output, { recursive: true });
    fs.cpSync(cache, path.join(output, 'cache'), { recursive: true, errorOnExist: true });
    const config = path.join(output, 'emscripten.config');
    const quote = value => JSON.stringify(value.replace(/\\/g, '/'));
    fs.writeFileSync(config, 'LLVM_ROOT = ' + quote(path.join(sdk, 'upstream/bin')) + '\nBINARYEN_ROOT = ' +
        quote(path.join(sdk, 'upstream')) + '\nNODE_JS = ' + quote(node) + '\n');
    const env = { ...process.env, EM_CONFIG: config, EM_CACHE: path.join(output, 'cache'), PYTHONDONTWRITEBYTECODE: '1' };
    const hashes = {};
    for (const directory of [__dirname, original, current])
    {
        for (const file of fs.readdirSync(directory))
        {
            if (/\.(cpp|h|inl|js)$/.test(file))
            {
                const absolute = path.join(directory, file);
                hashes[absolute] = crypto.createHash('sha256').update(fs.readFileSync(absolute)).digest('hex');
            }
        }
    }
    const report = { output, node: process.version, compiler: fs.readFileSync(path.join(emscripten, 'emscripten-version.txt'), 'utf8').trim(),
        hashes, specs, rounds: 5, runs: [], passed: false };
    const raw = ['bits,id,name,phase,version,round,repeats,ops_s,seconds,p50_us,p95_us,p99_us,samples,extra_calls,attempted_calls,log'];
    const summary = ['bits,id,name,original_ops_s,current_ops_s,current_over_original,original_p50_us,current_p50_us,original_p95_us,current_p95_us,original_p99_us,current_p99_us,rounds,repeats,current_only,original_extra_calls,current_extra_calls'];
    for (const bits of [32, 64])
    {
        const programs = [];
        for (const [version, library] of [['original', original], ['current', current], ['extended', current]])
        {
            const directory = path.join(output, 'wasm' + bits + '-' + version);
            fs.mkdirSync(directory);
            const program = path.join(directory, 'performance.js');
            // 大批满表在计时外触发共享内存增长；保留SDK的增长性能提示，其他警告仍按错误处理。
            const args = ['-B', path.join(emscripten, 'em++.py'), '-std=c++11', '-O2', '-DNDEBUG', '-DSTATIC_LIB',
                '-Wall', '-Wextra', '-Werror', '-Wno-error=pthreads-mem-growth', '-fexceptions', '-I' + library, '-pthread', '-sPTHREAD_POOL_SIZE=8',
                '-sPROXY_TO_PTHREAD=1', '-sDISABLE_EXCEPTION_CATCHING=0', '-sENVIRONMENT=node', '-sEXIT_RUNTIME=1',
                '-sSTACK_SIZE=1048576', '-sINITIAL_MEMORY=134217728', '-sALLOW_MEMORY_GROWTH=1',
                '-sMAXIMUM_MEMORY=2147483648', '-sABORTING_MALLOC=0'];
            if (bits === 64) args.push('-sMEMORY64=1');
            if (version === 'extended') args.push('-DLOCKFREEHASHMAP_PERF_CURRENT');
            args.push(...['LockFreeHashMapPerformance.cpp', 'LockFreeHashMapPerfModels.cpp', 'LockFreeHashMapTestThread.cpp'].map(file => path.join(__dirname, file)), '-o', program);
            run(python, args, directory, path.join(directory, 'build.log'), env, report);
            programs.push(program);
        }
        for (const spec of specs)
        {
            const pilots = [];
            const versions = spec.current_only ? [1] : [0, 1];
            const measurements = [[], []];
            const execute = (version, round, repeats) =>
            {
                const label = version === 0 ? 'original' : 'current';
                const log = path.join(output, 'wasm' + bits + '-id' + spec.id + '-' + label + '-r' + round + '.log');
                run(node, [programs[spec.current_only ? 2 : version], String(repeats * 1000 + spec.id)], output, log, env, report);
                const result = parse(fs.readFileSync(log, 'utf8'), spec, repeats);
                raw.push([bits, spec.id, spec.name, round === 0 ? 'pilot' : 'measured', label, round, repeats,
                    result.ops, result.seconds, result.p50_us, result.p95_us, result.p99_us, result.samples, result.extra_calls, result.attempted_calls, log].join(','));
                fs.writeFileSync(path.join(output, 'runs.csv'), raw.join('\n') + '\n');
                return result;
            };
            for (const version of versions) pilots[version] = execute(version, 0, 1);
            const times = versions.map(version => pilots[version].seconds);
            const repeats = Math.max(1, Math.min(16, Math.ceil(0.08 / Math.min(...times)), Math.floor(0.4 / Math.max(...times))));
            for (let round = 1; round <= 5; ++round)
            {
                const order = spec.current_only ? [1] : ((round + spec.id) % 2 === 0 ? [0, 1] : [1, 0]);
                for (const version of order) measurements[version].push(execute(version, round, repeats));
            }
            const statistic = (version, name) => measurements[version].length ? median(measurements[version].map(value => value[name])) : '';
            const oldRate = statistic(0, 'ops');
            const newRate = statistic(1, 'ops');
            summary.push([bits, spec.id, spec.name, oldRate, newRate, oldRate ? newRate / oldRate : '',
                statistic(0, 'p50_us'), statistic(1, 'p50_us'), statistic(0, 'p95_us'), statistic(1, 'p95_us'),
                statistic(0, 'p99_us'), statistic(1, 'p99_us'), 5, repeats, spec.current_only,
                measurements[0].reduce((sum, value) => sum + value.extra_calls, 0),
                measurements[1].reduce((sum, value) => sum + value.extra_calls, 0)].join(','));
            fs.writeFileSync(path.join(output, 'comparison.csv'), summary.join('\n') + '\n');
            process.stdout.write('WASM' + bits + ' ' + spec.name + ' ratio=' + (oldRate ? newRate / oldRate : 'NA') + '\n');
        }
    }
    report.changed = Object.keys(hashes).filter(file => hashes[file] !== crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex'));
    report.passed = report.changed.length === 0;
    fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify(report, null, 2));
    return report.passed ? 0 : 1;
}

try
{
    process.exitCode = process.argv.length === 3 && process.argv[2] === '--selftest' ? selftest() : main();
}
catch (error)
{
    process.stderr.write('WASM_PERFORMANCE ' + error.message + '\n');
    process.exitCode = 2;
}