'use strict';
// Wasm测试允许使用宿主语言；本脚本只编译并运行SemaphoreTest的C++能力用例
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const child = require('child_process');

// 读取完整文件计算SHA256，失败向入口传播
function hash(file)
{
    return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
}

// 保存独立日志并限制进程总时长，超时先回收本次启动的进程树
function run(program, args, log, environment, timeout)
{
    const descriptor = fs.openSync(log, 'wx');
    return new Promise((resolve, reject) =>
    {
        let process;
        try
        {
            process = child.spawn(program, args, {
                env: environment, stdio: ['ignore', descriptor, descriptor], windowsHide: true
            });
        }
        catch (error)
        {
            fs.closeSync(descriptor);
            reject(error);
            return;
        }
        let failed = false;
        const timer = setTimeout(() =>
        {
            if (process.exitCode === null && process.pid)
            {
                failed = true;
                child.spawnSync('taskkill.exe', ['/PID', String(process.pid), '/T', '/F'], {
                    stdio: ['ignore', descriptor, descriptor], windowsHide: true, timeout: 15000
                });
            }
        }, timeout);
        process.on('error', error =>
        {
            failed = true;
            fs.writeSync(descriptor, '\n' + error.message + '\n');
        });
        process.on('close', code =>
        {
            clearTimeout(timer);
            fs.closeSync(descriptor);
            resolve(failed || code === null ? 125 : code);
        });
    });
}

// 按独立SDK运行四个Wasm配置，并核对执行前后的全部源码和产物身份
async function main()
{
    const root = path.resolve(__dirname, '../../..');
    const output = path.resolve(process.argv[2] || '');
    const tools = process.argv[3] || 'D:/BuildTools/WebAssembly';
    const python = process.argv[4] || process.env.CLOUD_REBUILD_PYTHON || 'python';
    const allowed = path.join(root, 'src/temp/Semaphore');
    const relative = path.relative(allowed, output);
    if (!relative || relative.startsWith('..') || path.isAbsolute(relative))
    {
        throw new Error('Provide a new output directory under src/temp/Semaphore');
    }
    fs.mkdirSync(output, {recursive: true});
    const source = path.join(root, 'src/Semaphore/Semaphore/src');
    const compiler = path.join(tools, 'emsdk-3.1.74/upstream/emscripten');
    const rebuild = path.join(root, 'src/Semaphore/scripts/rebuild_Semaphore.py');
    const fixture = path.join(__dirname, 'SemaphoreWasmTests.cpp');
    const inputs = [__filename, fixture, path.join(__dirname, 'SemaphoreWasmTests.h'), rebuild];
    for (const name of fs.readdirSync(source))
    {
        if (/\.(h|inl|cpp)$/.test(name))
        {
            inputs.push(path.join(source, name));
        }
    }
    const identity = {};
    for (const file of inputs)
    {
        identity[file] = hash(file);
    }
    const rows = [];
    for (const bits of [32, 64])
    {
        for (const threaded of [false, true])
        {
            const folder = path.join(output, 'wasm' + bits + (threaded ? '-pthread' : '-single'));
            fs.mkdirSync(folder, {recursive: true});
            const sdk = path.join(folder, 'sdk');
            const environment = Object.assign({}, process.env, {
                EMSDK: path.join(tools, 'emsdk-3.1.74'),
                EM_CONFIG: path.join(tools, 'emsdk-3.1.74/.emscripten'),
                EM_CACHE: path.join(output, 'cache'),
                // 首次构建标准库限制并发，避免按全部逻辑核启动编译器耗尽宿主内存。
                EMCC_CORES: process.env.EMCC_CORES || '8',
                WASM_BUILD_ROOT: path.join(folder, 'build'),
                CLOUD_REBUILD_PYTHON: python,
                CLOUD_REBUILD_SDK_ROOT: sdk,
                PATH: path.join(tools, 'node') + path.delimiter + process.env.PATH
            });
            delete environment.CLOUD_WASM_SESSION;
            delete environment.CLOUD_WASM_PARENT;
            const buildArgs = [rebuild, 'wasm' + bits, 'lib', 'debug', 'only', '--output-root', sdk];
            if (threaded)
            {
                buildArgs.push('--pthread');
            }
            const library = await run(python, buildArgs, path.join(folder, 'library-build.log'), environment, 300000);
            const archive = path.join(sdk, 'lib/Semaphored.a');
            const executable = path.join(folder, 'SemaphoreTest.js');
            const args = [path.join(compiler, 'em++.py'), '-std=c++11', '-DSTATIC_LIB', '-iquote', source,
                '-fexceptions', '-sDISABLE_EXCEPTION_CATCHING=0', '-sEXIT_RUNTIME=1', '-g', fixture,
                archive, '-o', executable];
            if (bits === 64)
            {
                args.push('-sMEMORY64=1');
            }
            if (threaded)
            {
                args.push('-pthread', '-sPTHREAD_POOL_SIZE=2');
            }
            const built = library === 0 ? await run(python, args, path.join(folder, 'test-build.log'), environment, 180000) : -1;
            const log = path.join(folder, 'run.log');
            let tested = built === 0 ? await run(path.join(tools, 'node/node.exe'), [executable], log, environment, 30000) : -1;
            if (tested === 0)
            {
                const text = fs.readFileSync(log, 'utf8');
                if (!text.includes('PASS id=48') || !text.includes('PASS id=87') || text.includes('FAIL') ||
                    (threaded && (!text.includes('PASS id=49') || !text.includes('PASS id=67'))))
                {
                    tested = 125;
                }
                identity[archive] = hash(archive);
                identity[executable] = hash(executable);
                identity[executable.replace(/\.js$/, '.wasm')] = hash(executable.replace(/\.js$/, '.wasm'));
            }
            const row = {bits: bits, pthread: threaded, library: library, build: built, run: tested};
            rows.push(row);
            console.log(JSON.stringify(row));
            fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify({rows: rows, sha256: identity}, null, 2));
        }
    }
    const changed = Object.keys(identity).filter(file => identity[file] !== hash(file));
    fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify({rows: rows, changed: changed, sha256: identity}, null, 2));
    return changed.length || rows.some(row => row.library !== 0 || row.build !== 0 || row.run !== 0) ? 1 : 0;
}

main().then(code =>
{
    process.exitCode = code;
}).catch(error =>
{
    console.error(error.stack);
    process.exitCode = 1;
});