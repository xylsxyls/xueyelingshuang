'use strict';

// 实际Wasm扩容和公开Sender入口；仅截获最末C边界核对传参，不伪造TLS或业务发送成功。
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');
const { readWasm } = require('./read_wasm');

async function checkCase(modulePath, method, borrowed) {
    const factory = require(modulePath);
    const module = await factory({ wasmBinary: readWasm(modulePath), wasmApi: WebAssembly });
    const runtime = await module.NetPeerRuntime.create({
        randomSource: size => crypto.randomFillSync(new Uint8Array(size)),
        createSocket() { throw new Error('Argument marshalling must not open a socket'); }
    });
    const sender = runtime.createSender({});
    assert.equal(sender.prepareGateway(), 0);
    const connect = method === 'connectGateway';
    const file = method === 'fileMetadata';
    const pattern = [49, 50, 0, 52, 53, 54, 255];
    const expected = Uint8Array.from({ length: file ? 160 : connect ? 16 : 7 }, (_, at) => pattern[at % pattern.length]);
    const allocation = module._malloc(expected.length);
    assert.ok(allocation);
    module.HEAPU8.set(expected, allocation);
    const originalHeap = module.HEAPU8.buffer;
    const bytes = borrowed ? module.HEAPU8.subarray(allocation, allocation + expected.length) : expected.slice();
    const requestId = 0x20000000000001n;
    const data = method.endsWith('Data');
    const symbol = { postRequest: '_netPeerSenderClientPost', sendRequest: '_netPeerSenderClientSendBegin',
        postData: '_netPeerSenderClientPostData', sendData: '_netPeerSenderClientSendDataBegin',
        connectGateway: '_netPeerSenderClientConnectService', fileMetadata: '_netPeerFileInfoFields' }[method];
    const originalFunction = module[symbol];
    const originalMalloc = module._malloc;
    let held = 0;
    let grew = false;
    const calls = [];
    // 内存增长必须由被测入口的首次分配触发，不能事先使输入失效。
    module._malloc = size => {
        if (!grew) {
            held = originalMalloc(module.HEAPU8.length + 65536);
            assert.ok(held);
            assert.notEqual(module.HEAPU8.buffer, originalHeap, 'Real memory growth must occur');
            grew = true;
        }
        return originalMalloc(size);
    };
    module[symbol] = (handle, value, pointer, size) => {
        assert.ok(handle);
        let actualId = value;
        if (file) {
            pointer = handle;
            size = value;
        } else if (connect) {
            assert.equal(value, 101);
            const options = Number(size);
            const offsets = module.netPeerABI.offsets;
            const view = new DataView(module.HEAPU8.buffer);
            for (const name of ['authority', 'certificate', 'privateKey']) {
                const field = options + offsets.options_tls + offsets['tls_' + name];
                const dataPointer = factory.sdk.bits === 64 ? Number(view.getBigUint64(field, true)) : view.getUint32(field, true);
                const length = view.getUint32(options + offsets.options_tls + offsets['tls_' + name + 'Size'], true);
                assert.equal(length, expected.length, name + ' length');
                assert.deepEqual(module.HEAPU8.slice(dataPointer, dataPointer + length), expected, name + ' bytes');
            }
            pointer = options + offsets.options_device;
            size = 16;
        }
        if (data) {
            const frame = Number(value);
            const view = new DataView(module.HEAPU8.buffer);
            const offsets = module.netPeerABI.offsets;
            actualId = view.getBigUint64(frame + offsets.frame_m_requestId, true);
            assert.equal(view.getUint32(frame + offsets.frame_m_productId, true), 101);
            assert.equal(view.getUint32(frame + offsets.frame_m_messageType, true), 7);
            assert.equal(view.getUint16(frame + offsets.frame_m_kind, true), 5);
            size = view.getUint32(frame + offsets.frame_m_payloadSize, true);
            pointer = factory.sdk.bits === 64 ? Number(view.getBigUint64(frame + offsets.frame_m_payload, true)) :
                view.getUint32(frame + offsets.frame_m_payload, true);
        }
        calls.push({ requestId: actualId, size, bytes: module.HEAPU8.slice(Number(pointer), Number(pointer) + size) });
        return -7; // 独立预期为拒绝，不能用替身制造一次成功网络发送。
    };
    try {
        let result;
        if (file) {
            assert.throws(() => new module.NetFileTransfer({ runtime }, bytes, 101, true, { read() {}, close() {} }),
                error => error.code === -7, 'Native metadata rejection must remain observable');
            result = -7;
        } else if (connect) {
            result = sender.connectServiceGateway(101, { address: '127.0.0.1', port: 5203, platform: 3, device: bytes,
                tls: { authority: bytes, certificate: bytes, privateKey: bytes } },
                { onSessionReady() {}, onSessionClosed() {}, onSessionResult() {}, onSessionFrame() {} });
        } else {
            result = await (data ? sender[method]({ product: 101, requestId, type: 7, kind: 5, bytes }) : sender[method](requestId, bytes));
        }
        assert.equal(grew, true, 'Public entry must exercise allocation');
        assert.equal(result, -7, 'Native rejection must be returned unchanged');
        assert.equal(calls.length, 1, 'One public invocation must reach the native boundary once');
        if (!file && !connect) assert.equal(calls[0].requestId, requestId);
        assert.equal(calls[0].size, expected.length, 'Heap growth must not truncate input length');
        assert.deepEqual(calls[0].bytes, expected, 'Heap growth must preserve every payload byte');
    } finally {
        module._malloc = originalMalloc;
        module[symbol] = originalFunction;
        if (held) module._free(held);
        module._free(allocation);
        assert.equal(sender.releaseGateway(), 0);
        assert.equal(runtime.clients, 0);
        assert.equal(runtime.hosts.size, 0);
    }
}

async function main() {
    const options = {};
    for (const argument of process.argv.slice(2)) {
        const split = argument.indexOf('=');
        const key = argument.slice(0, split);
        assert.ok(split > 0 && split + 1 < argument.length && ['--module', '--report'].includes(key) && !Object.hasOwn(options, key));
        options[key] = path.resolve(argument.slice(split + 1));
    }
    assert.ok(options['--module'] && options['--report']);
    const report = fs.openSync(options['--report'], 'wx');
    const results = [];
    try {
        for (const method of ['postRequest', 'sendRequest', 'postData', 'sendData', 'connectGateway', 'fileMetadata']) {
            for (const borrowed of [false, true]) {
                try {
                    await checkCase(options['--module'], method, borrowed);
                    results.push({ method, borrowed, pass: true });
                } catch (error) { results.push({ method, borrowed, pass: false, error: String(error.stack || error) }); }
            }
        }
        const passed = results.length === 12 && results.every(result => result.pass);
        fs.writeFileSync(report, JSON.stringify({ pass: passed, bits: require(options['--module']).sdk.bits,
            scope: 'Real memory-growth marshalling; native boundary returns an injected rejection, no TLS success claimed', results }, null, 2));
        console.log('NetPeer memory growth: ' + results.filter(result => result.pass).length + '/12 PASS');
        if (!passed) process.exitCode = 1;
    } finally { fs.closeSync(report); }
}

main().catch(error => { console.error(error.stack || String(error)); process.exitCode = 1; });