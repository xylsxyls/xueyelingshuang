/** 运行独立 C++ 测试模块；微信传包内 wasmPath，浏览器传工厂即可。
 * 通过时返回 0。加载失败、超时、非零退出码均拒绝 Promise。
 * 成对产物可用 node src/PlatformTest/PlatformTest/src/package_library_test.js TimerTest --wasm32 build32 --wasm64 build64 --output pair 打包。
 * 输出目录支持 node run-node.js auto、经本地 HTTP 打开 LibraryTestHost.html，以及直接导入微信开发者工具。
 */
async function runLibraryTest(createModule, settings = {}) {
    let timeout;
    let rejectModule;
    const failure = new Promise((resolve, reject) => { rejectModule = reject; });
    const deadline = setTimeout(() => rejectModule(new Error('Test module timed out')), settings.timeoutMs || 60000);
    const options = {
        print: settings.print || console.log.bind(console),
        printErr: settings.printErr || console.error.bind(console),
        onAbort: reason => rejectModule(new Error(String(reason)))
    };
    try {
        if (settings.wasmBinary) options.wasmBinary = settings.wasmBinary;
        if (settings.wasmPath) {
            options.wasmApi = WXWebAssembly;
            options.instantiateWasm = (imports, ready) => {
                WXWebAssembly.instantiate(settings.wasmPath, imports).then(
                    result => ready(result.instance, result.module), rejectModule);
                return {};
            };
        }
        const instance = await Promise.race([createModule(options), failure]);
        const args = settings.args === undefined ? (settings.benchmark ? ['--benchmark'] : []) : settings.args;
        if (!Array.isArray(args) || !args.every(value => typeof value === 'string')) {
            throw new TypeError('args must be an array of command-line strings');
        }
        let result = instance.callMain(args);
        if (result === 0 && !args.includes('--benchmark') && typeof instance._timerTestResult === 'function') {
            while ((result = instance._timerTestResult()) === -1) {
                await Promise.race([new Promise(resolve => { timeout = setTimeout(resolve, 20); }), failure]);
            }
        }
        if (result !== 0) throw new Error('C++ test exit code ' + result);
        return result;
    } finally {
        clearTimeout(timeout);
        clearTimeout(deadline);
    }
}
if (typeof module !== 'undefined' && module.exports) {
    module.exports = runLibraryTest;
    if (typeof process !== 'undefined' && process.versions && process.versions.node && require.main === module) {
        const path = require('path');
        const fs = require('fs');
        const entry = path.resolve(process.argv[2]);
        const wasmBinary = fs.readFileSync(entry.replace(/\.js$/, '.wasm'));
        runLibraryTest(require(entry), { wasmBinary, args: process.argv.slice(3) }).catch(error => {
            console.error(error);
            process.exitCode = 1;
        });
    }
}