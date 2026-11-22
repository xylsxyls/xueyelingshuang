/** 运行独立 C++ 测试模块；微信传包内 wasmPath，浏览器传工厂即可。
 * 通过时返回 0。加载失败、超时、非零退出码均拒绝 Promise。
 * 成对产物可用 node src/PlatformTest/PlatformTest/src/package_library_test.js TimerTest --wasm32 build32 --wasm64 build64 --output pair 打包。
 * 输出目录支持 node run-node.js auto、经本地 HTTP 打开 LibraryTestHost.html，以及直接导入微信开发者工具。
 */
async function runLibraryTest(createModule, settings = {}) {
    let timeout;
    let originServer;
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
        let args = settings.args === undefined ? (settings.benchmark ? ['--benchmark'] : []) : settings.args;
        if (!Array.isArray(args) || !args.every(value => typeof value === 'string')) {
            throw new TypeError('args must be an array of command-line strings');
        }
        const httpResult = instance._httpRequestTestResult;
        if (typeof httpResult === 'function' && !args.includes('--benchmark') && !args.includes('--origin')) {
            let origin = settings.origin;
            if (!origin && typeof process !== 'undefined' && process.versions && process.versions.node) {
                // Node 上为本模块分配回环端口。设备和浏览器使用调用者提供的可达源站。
                originServer = require('http').createServer((req, res) => {
                    if (req.method === 'POST' && req.url === '/echo') {
                        req.pipe(res);
                    } else if (req.method === 'GET' && req.url === '/video/sample') {
                        const body = Buffer.from([65, 0, 66, 67, 68]);
                        res.writeHead(200, {'Content-Length': body.length});
                        res.end(body);
                    } else {
                        res.writeHead(404);
                        res.end('missing');
                    }
                });
                await Promise.race([new Promise((resolve, reject) => {
                    originServer.once('error', reject);
                    originServer.listen(0, '127.0.0.1', resolve);
                }), failure]);
                origin = 'http://127.0.0.1:' + originServer.address().port;
            }
            if (origin) args = ['--origin', origin, ...args];
        }
        let result = instance.callMain(args);
        const poll = httpResult || instance._timerTestResult;
        if (result === 0 && !args.includes('--benchmark') && typeof poll === 'function') {
            while ((result = poll()) === -1) {
                await Promise.race([new Promise(resolve => { timeout = setTimeout(resolve, 20); }), failure]);
            }
        }
        if (result !== 0) throw new Error('C++ test exit code ' + result);
        return result;
    } finally {
        clearTimeout(timeout);
        clearTimeout(deadline);
        if (originServer && originServer.listening) {
            originServer.close();
            originServer.closeAllConnections();
        }
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
