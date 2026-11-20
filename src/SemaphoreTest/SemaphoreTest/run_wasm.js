'use strict';
// Wasm测试允许使用宿主语言；本脚本只编译并运行SemaphoreTest的C++能力用例
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const child = require('child_process');

/** 读取完整文件计算SHA256，失败向入口传播
@param [in] file 本次源文件或产物的路径
@return SHA256十六进制字符串
*/
function hash(file)
{
    return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
}

/** 保存独立日志并限制进程总时长，超时请求终止本次启动的进程树
@param [in] program 要启动的程序路径
@param [in] args 独立参数数组，不经shell拼接
@param [in] log 新日志路径，拒绝覆盖已有文件
@param [in] environment 子进程环境变量
@param [in] timeout 启动后超时预算，单位毫秒
@return Promise中的退出码；启动错误或超时标记为125，文件或同步启动异常向调用方传播
*/
function run(program, args, log, environment, timeout)
{
    const descriptor = fs.openSync(log, 'wx');
    return new Promise(resolve =>
    {
        let worker;
        let timer;
        let fallback;
        let failed = false;
        let finished = false;
        function report(error)
        {
            try { fs.writeSync(descriptor, String(error) + '\n'); }
            catch (writeError) { console.error(String(writeError)); }
        }
        function finish(code)
        {
            if (finished) return;
            finished = true;
            clearTimeout(timer);
            clearTimeout(fallback);
            try { fs.closeSync(descriptor); }
            catch (error) { failed = true; console.error(String(error)); }
            resolve(failed || code === null ? 125 : code);
        }
        try
        {
            worker = child.spawn(program, args, {env: environment, stdio: ['ignore', descriptor, descriptor], windowsHide: true});
        }
        catch (error)
        {
            failed = true;
            report(error);
            finish(125);
            return;
        }
        worker.on('error', error => { failed = true; if (!finished) report(error); });
        worker.on('close', finish);
        timer = setTimeout(() =>
        {
            if (worker.exitCode !== null || worker.signalCode || !worker.pid) return;
            failed = true;
            try
            {
                const result = child.spawnSync('taskkill.exe', ['/PID', String(worker.pid), '/T', '/F'],
                    {stdio: ['ignore', descriptor, descriptor], windowsHide: true, timeout: 15000});
                if (result.status !== 0) worker.kill();
            }
            catch (error) { report(error); }
            fallback = setTimeout(() =>
            {
                report('Process cleanup was not confirmed');
                worker.unref();
                finish(125);
            }, 5000);
        }, timeout);
    });
}

/** 核对每条完整结果行，拒绝缺失、重复、额外或失败的用例。
@param [in] text 子进程输出
@param [in] threaded 本轮是否启用 pthread
@return 是否得到该构建对应的全部预期结果
*/
function validResults(text, threaded)
{
    const expected = new Set(threaded ? ['PASS 48', 'PASS 49', 'PASS 67', 'PASS 87'] :
        ['PASS 48', 'NOT_RUN 49', 'NOT_RUN 67', 'PASS 87']);
    for (const line of text.split(/\r?\n/))
    {
        if (/^FAIL\b/.test(line)) return false;
        if (!/^(?:PASS|NOT_RUN)\b/.test(line)) continue;
        const match = /^(PASS|NOT_RUN) id=(\d+)(?: |$)/.exec(line);
        if (!match || !expected.delete(match[1] + ' ' + match[2])) return false;
    }
    return expected.size === 0;
}

/** 从同一SDK复制已有的C/C++运行库到本批私有缓存，缺少的归档仍由编译器生成
@param [in] compiler 本次使用的Emscripten安装目录
@param [in] cache 本批新建的缓存目录
*/
function seedRuntimeArchives(compiler, cache)
{
    for (const arch of ['wasm32-emscripten', 'wasm64-emscripten'])
    {
        const installed = path.join(compiler, 'cache', 'sysroot', 'lib', arch);
        if (!fs.existsSync(installed)) continue;
        const local = path.join(cache, 'sysroot', 'lib', arch);
        fs.mkdirSync(local, {recursive: true});
        for (const entry of fs.readdirSync(installed, {withFileTypes: true}))
        {
            if (entry.isFile() && /^lib(?:c(?:\+\+(?:abi)?)?|compiler_rt|dlmalloc)(?:-[\w-]+)?\.a$/.test(entry.name))
            {
                fs.copyFileSync(path.join(installed, entry.name), path.join(local, entry.name), fs.constants.COPYFILE_EXCL);
            }
        }
    }
}

/** 按独立SDK运行四个Wasm配置，并核对执行前后的全部源码和产物身份
@return Promise中的退出码，0表示全部配置通过，1表示构建、运行或身份检查失败
*/
async function main()
{
    const root = path.resolve(__dirname, '../../..');
    const output = path.resolve(process.argv[2] || '');
    const tools = process.argv[3] || 'D:/BuildTools/WebAssembly';
    const make = process.argv[4] || 'C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC/bin/amd64/nmake.exe';
    const configuration = process.env.SEMAPHORE_TEST_WASM_CONFIGURATION || 'Release';
    if (configuration !== 'Release' && configuration !== 'Debug')
    {
        throw new Error('SEMAPHORE_TEST_WASM_CONFIGURATION must be Release or Debug');
    }
    const cmake = path.join(tools, 'cmake/bin/cmake.exe');
    const allowed = path.join(root, 'src/temp/Semaphore');
    const relative = path.relative(allowed, output);
    if (!relative || relative.startsWith('..') || path.isAbsolute(relative))
    {
        throw new Error('Provide a new output directory under src/temp/Semaphore');
    }
    if (fs.existsSync(output))
    {
        throw new Error('Output directory must be new; existing evidence is not overwritten');
    }
    fs.mkdirSync(output, {recursive: true});
    const source = path.join(root, 'src/Semaphore/Semaphore/src');
    const compiler = path.join(tools, 'emsdk-3.1.74/upstream/emscripten');
    const project = path.join(__dirname, 'CMakeLists.txt');
    const fixture = path.join(__dirname, 'SemaphoreWasmTests.cpp');
    const inputs = [__filename, fixture, path.join(__dirname, 'SemaphoreWasmTests.h'), project];
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
    seedRuntimeArchives(compiler, path.join(output, 'cache'));
    const rows = [];
    for (const bits of [32, 64])
    {
        for (const threaded of [false, true])
        {
            const folder = path.join(output, 'wasm' + bits + (threaded ? '-pthread' : '-single'));
            fs.mkdirSync(folder, {recursive: true});
            const environment = Object.assign({}, process.env, {
                EMSDK: path.join(tools, 'emsdk-3.1.74'),
                EM_CONFIG: path.join(tools, 'emsdk-3.1.74/.emscripten'),
                EM_CACHE: path.join(output, 'cache'),
                // 首次构建标准库限制并发，避免按全部逻辑核启动编译器耗尽宿主内存。
                EMCC_CORES: process.env.EMCC_CORES || '8',
                PATH: path.join(tools, 'node') + path.delimiter + process.env.PATH
            });
            delete environment.CLOUD_WASM_SESSION;
            delete environment.CLOUD_WASM_PARENT;
            const args = ['-G', 'NMake Makefiles', '-S', __dirname, '-B', folder,
                '-DCMAKE_MAKE_PROGRAM=' + make, '-DCMAKE_BUILD_TYPE=' + configuration,
                '-DCMAKE_TOOLCHAIN_FILE=' + path.join(compiler, 'cmake/Modules/Platform/Emscripten.cmake'),
                '-DSEMAPHORE_TEST_PTHREADS=' + (threaded ? 'ON' : 'OFF')];
            if (bits === 64) args.push('-DCMAKE_CXX_FLAGS=-sMEMORY64=1', '-DCMAKE_C_FLAGS=-sMEMORY64=1');
            const configured = await run(cmake, args, path.join(folder, 'configure.log'), environment, 180000);
            const library = configured === 0 ? await run(cmake, ['--build', folder, '--target', 'Semaphore'],
                path.join(folder, 'library-build.log'), environment, 360000) : -1;
            const archive = path.join(folder, 'libSemaphore.a');
            const executable = path.join(folder, 'SemaphoreTest.js');
            const built = library === 0 ? await run(cmake, ['--build', folder, '--target', 'SemaphoreTest'],
                path.join(folder, 'test-build.log'), environment, 180000) : -1;
            const log = path.join(folder, 'run.log');
            // 在运行前记录实际执行产物，运行结束及全矩阵结束后都核对，避免只哈希运行后的文件。
            const artifacts = built === 0 ? [archive, executable, executable.replace(/\.js$/, '.wasm')] : [];
            const worker = executable.replace(/\.js$/, '.worker.js');
            if (built === 0 && fs.existsSync(worker)) artifacts.push(worker);
            for (const file of artifacts)
            {
                identity[file] = hash(file);
            }
            let tested = built === 0 ? await run(path.join(tools, 'node/node.exe'), [executable], log, environment, 30000) : -1;
            if (artifacts.some(file => identity[file] !== hash(file)))
            {
                tested = 125;
            }
            if (tested === 0)
            {
                const text = fs.readFileSync(log, 'utf8');
                if (!validResults(text, threaded))
                {
                    tested = 125;
                }
            }
            const row = {bits: bits, pthread: threaded, configuration: configuration, library: library, build: built, run: tested};
            rows.push(row);
            console.log(JSON.stringify(row));
            fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify({rows: rows, sha256: identity}, null, 2));
        }
    }
    const changed = Object.keys(identity).filter(file => identity[file] !== hash(file));
    fs.writeFileSync(path.join(output, 'results.json'), JSON.stringify({rows: rows, changed: changed, sha256: identity}, null, 2));
    return changed.length || rows.some(row => row.library !== 0 || row.build !== 0 || row.run !== 0) ? 1 : 0;
}

module.exports = {run, validResults};
if (require.main === module)
{
    main().then(code => { process.exitCode = code; }).catch(error =>
    {
        console.error(error.stack);
        process.exitCode = 1;
    });
}
