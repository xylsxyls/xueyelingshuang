'use strict';
const { readPointer } = require('./WasmMemory');

// Owns only flat C ABI memory. Versions, limits, nonce binding and state transitions
// are validated by NetInterfaceHandshake.cpp in the generated Wasm module.
class NetInterfaceHandshake {
    constructor(runtime, platform, deviceId, monotonicMs, capabilities = 7) {
        if (!(deviceId instanceof Uint8Array) || deviceId.length !== 16) {
            throw new Error('设备标识必须为16字节');
        }
        const device = deviceId.slice();
        this.module = runtime.module;
        this.handle = 0;
        this.scratch = 0;
        this.closed = false;
        this.ready = false;
        try {
            this.scratch = this.module._malloc(128);
            if (!this.scratch) throw new Error('应用握手内存不足');
            const time = this.time(monotonicMs);
            this.module.HEAPU32.set([0, platform, capabilities, 32, 65536, 5000], this.scratch >>> 2);
            if (runtime.copyRandom(this.scratch + 32, 16) !== 0) throw new Error('应用握手强随机源不可用');
            this.module.HEAPU8.set(device, this.scratch + 48);
            const result = this.module._netInterfaceHandshakeCreate(this.scratch, 6, this.scratch + 32,
                32, time[0], time[1], this.scratch + 96);
            this.handle = readPointer(this.module, this.scratch + 96);
            this.check(result);
        } catch (error) {
            this.dispose();
            throw error;
        }
    }

    time(value) {
        if (!Number.isSafeInteger(value) || value < 0) throw new Error('应用握手时间非法');
        return [value >>> 0, Math.floor(value / 4294967296)];
    }

    assertOpen() {
        if (this.closed) throw new Error('应用握手已关闭');
    }

    check(result) {
        if (result < 0) {
            const error = new Error(`网关应用握手失败：${result}`);
            error.code = result;
            throw error;
        }
        return result;
    }

    progress(monotonicMs) {
        this.assertOpen();
        const time = this.time(monotonicMs);
        this.ready = this.check(this.module._netInterfaceHandshakeProgress(this.handle, time[0], time[1])) === 2;
        return this.ready;
    }

    peekOutput() {
        this.assertOpen();
        const result = this.check(this.module._netInterfaceHandshakePeekOutput(this.handle,
            this.scratch, this.scratch + 24));
        if (result !== 2) return null;
        const fields = Array.from(this.module.HEAPU32.subarray(this.scratch >>> 2, (this.scratch >>> 2) + 6));
        const pointer = readPointer(this.module, this.scratch + 24);
        return { fields: fields.slice(0, 5), payload: this.module.HEAPU8.slice(pointer, pointer + fields[5]) };
    }

    consumeOutput() {
        this.assertOpen();
        this.ready = this.check(this.module._netInterfaceHandshakeConsumeOutput(this.handle)) === 2;
    }

    accept(frame, monotonicMs) {
        this.assertOpen();
        const time = this.time(monotonicMs);
        // After negotiation C++ only checks the frame kind. The real payload has
        // already passed the connection decoder and stays owned by the caller.
        const bytes = this.ready ? new Uint8Array(0) : frame.payload;
        let body = 0;
        try {
            if (bytes.length) {
                body = this.module._malloc(bytes.length);
                if (!body) throw new Error('应用握手内存不足');
                this.module.HEAPU8.set(bytes, body);
            }
            this.module.HEAPU32.set([frame.kind, frame.product, frame.requestLow, frame.requestHigh,
                frame.type, bytes.length], this.scratch >>> 2);
            const result = this.check(this.module._netInterfaceHandshakeAccept(this.handle,
                this.scratch, body, time[0], time[1]));
            if (result === 2) this.ready = true;
            return result === 0;
        } finally {
            if (body) this.module._free(body);
        }
    }

    peerInfo() {
        this.assertOpen();
        const result = this.check(this.module._netInterfaceHandshakePeerInfo(this.handle,
            this.scratch, this.scratch + 32));
        if (result !== 2) return null;
        const values = this.module.HEAPU32.slice(this.scratch >>> 2, (this.scratch >>> 2) + 4);
        return { platform: values[0], capabilities: values[1], maxRequests: values[2], maxChunk: values[3],
            peerNonce: this.module.HEAPU8.slice(this.scratch + 32, this.scratch + 48) };
    }

    dispose() {
        if (this.closed) return;
        this.closed = true;
        this.ready = false;
        if (this.handle) this.module._netInterfaceHandshakeDestroy(this.handle);
        if (this.scratch) this.module._free(this.scratch);
        this.handle = this.scratch = 0;
    }
}

module.exports = { NetInterfaceHandshake };