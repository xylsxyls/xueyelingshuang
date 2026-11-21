/** 浏览器或微信运行真实HttpWorkTest。origin是能访问的测试源站，微信另传包内wasm路径。
 * 返回Promise，解析值0为通过，模块加载或用例失败时拒绝。
 */
async function runHttpWorkTest(createModule, origin, hostSettings) {
    const settings = typeof hostSettings === 'string' ? { wasmPath: hostSettings } : (hostSettings || {});
    const options = {};
    if (typeof settings.print === 'function') options.print = settings.print;
    if (typeof settings.printErr === 'function') options.printErr = settings.printErr;
    let rejectLoading;
    const loadError = new Promise((resolve, reject) => { rejectLoading = reject; });
    let timer = setTimeout(() => rejectLoading(new Error('Wasm module load timeout')), 20000);
    options.onAbort = reason => rejectLoading(new Error(String(reason)));
    if (settings.wasmBinary) options.wasmBinary = settings.wasmBinary;
    if (settings.wasmPath) {
        options.wasmApi = WXWebAssembly;
        options.instantiateWasm = (imports, ready) => {
            WXWebAssembly.instantiate(settings.wasmPath, imports).then(
                result => ready(result.instance, result.module), rejectLoading);
            return {};
        };
    }
    let client;
    try {
        client = await Promise.race([createModule(options), loadError]);
    } finally {
        clearTimeout(timer);
    }
    const code = client.callMain(settings.args || [settings.benchmark ? '--benchmark' : '--client', origin]);
    if (code) throw new Error('HttpWorkTest rejected arguments or setup with exit code ' + code);
    const deadline = Date.now() + 65000;
    while (client._httpWorkTestResult() < 0 && Date.now() < deadline) {
        await new Promise(resolve => setTimeout(resolve, 20));
    }
    if (client._httpWorkTestResult() !== 0) throw new Error('HttpWorkTest failed or timed out');
    return 0;
}
if (typeof module !== 'undefined' && module.exports) {
    module.exports = runHttpWorkTest;
    if (typeof process !== 'undefined' && process.versions && process.versions.node && require.main === module) {
        const entry = require('path').resolve(process.argv[2]);
        const wasmBinary = require('fs').readFileSync(entry.replace(/\.js$/, '.wasm'));
        runHttpWorkTest(require(entry), process.argv[4], { wasmBinary, args: process.argv.slice(3) }).catch(error => {
            console.error(error);
            process.exitCode = 1;
        });
    }
}