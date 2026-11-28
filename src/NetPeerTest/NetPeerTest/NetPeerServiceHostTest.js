'use strict';

/** 在内存文件系统中临时装入隔离叶素材，执行正式 C++ 服务参数用例。
 * @param {Function} createModule 匹配位宽的测试模块工厂。
 * @param {Uint8Array} certificate 隔离 service.7 叶证书。
 * @param {Uint8Array} privateKey 隔离叶私钥；调用结束后原缓冲被擦除。
 * @param {Object} options 模块加载参数，可含 wasmBinary；discovery 为 true 时执行发现专项。
 * @returns {Promise<number>} 测试全部通过为 0，失败拒绝 Promise。
 */
async function runServiceTest(createModule, certificate, privateKey, options = {}) {
    let instance;
    const discovery = options.discovery === true;
    const secretPath = discovery ? '/identity.dat' : '/service.key';
    try {
        instance = await createModule(options);
        if (!discovery) instance.FS.writeFile('/service.crt', certificate);
        instance.FS.writeFile(secretPath, privateKey);
        const args = discovery ? ['--discovery-timers', secretPath] : ['--service-options', '/service.crt', secretPath];
        const result = instance.callMain(args);
        if (result !== 0) throw new Error('Fixture test exit code ' + result);
        return result;
    } finally {
        privateKey.fill(0);
        if (instance) {
            // MEMFS 文件数据与调用方缓冲均不留存；构建产物从不包含叶私钥。
            try {
                const node = instance.FS.lookupPath(secretPath).node;
                if (node.contents) node.contents.fill(0);
                instance.FS.unlink(secretPath);
            } catch (error) {
                if (error.code !== 'ENOENT' && error.errno !== 44) throw error;
            } finally {
                try { if (!discovery) instance.FS.unlink('/service.crt'); }
                catch (error) {
                    if (error.code !== 'ENOENT' && error.errno !== 44) throw error;
                }
            }
        }
    }
}

if (typeof module !== 'undefined' && module.exports) {
    module.exports = runServiceTest;
    if (require.main === module) {
        const fs = require('fs');
        const path = require('path');
        if (process.argv.length !== 5) {
            console.error('NetPeerServiceHostTest.js <module.js> <test-certificate.pem> <test-private-key.pem>');
            console.error('NetPeerServiceHostTest.js <module.js> --discovery-timers <test-identity.dat>');
            process.exitCode = 2;
        } else {
            // 未完成模块加载或正式用例时，事件循环自然结束也必须按失败退出。
            process.exitCode = 1;
            const entry = path.resolve(process.argv[2]);
            const createModule = require(entry);
            const wasmBinary = fs.readFileSync(entry.replace(/\.js$/, '.wasm'));
            const discovery = process.argv[3] === '--discovery-timers';
            const certificate = discovery ? new Uint8Array(0) : fs.readFileSync(process.argv[3]);
            const privateKey = fs.readFileSync(process.argv[4]);
            runServiceTest(createModule, certificate, privateKey, {wasmBinary, discovery}).then(() => {
                    process.exitCode = 0;
                }).catch(error => {
                    console.error(error);
                    process.exitCode = 1;
                });
        }
    }
}