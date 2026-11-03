'use strict';
const { readPointer } = require('./WasmMemory');

// Host callbacks and Wasm memory only. C++ owns IDs, quotas, deadlines, cancellation,
// response correlation and the complete RPC wire envelope.
class NetInterfaceRequests {
    constructor(runtime, limit) {
        this.module = runtime.module;
        this.handle = 0;
        this.scratch = 0;
        this.closed = false;
        try {
            this.scratch = this.module._malloc(128);
            if (!this.scratch) throw new Error('请求表内存不足');
            this.check(this.module._netInterfaceRequestsCreate(limit, this.scratch));
            this.handle = readPointer(this.module, this.scratch);
            this.check(this.module._netInterfaceRequestsAddEndpoint(this.handle, 1, 0, 0, 0, limit, 0));
            this.check(this.module._netInterfaceRequestsAddEndpoint(this.handle, 2, 0, 1, 0, limit, 0));
        } catch (error) {
            this.dispose();
            throw error;
        }
    }

    assertOpen() {
        if (this.closed) throw new Error('请求表已关闭');
    }

    check(result) {
        if (result < 0) {
            const error = new Error(`网关请求失败：${result}`);
            error.code = result;
            throw error;
        }
        return result;
    }

    time(value) {
        if (!Number.isSafeInteger(value) || value < 0) throw new Error('请求时间非法');
        return [value >>> 0, Math.floor(value / 4294967296)];
    }

    record() {
        return this.module.HEAPU32.slice((this.scratch + 64) >>> 2, (this.scratch + 128) >>> 2);
    }

    begin(product, type, timeout, now) {
        this.assertOpen();
        for (const value of [product, type, timeout]) {
            if (!Number.isInteger(value) || value < 0 || value > 0xffffffff) throw new Error('请求参数非法');
        }
        const time = this.time(now);
        this.module.HEAPU32.set([1, 0, 0, 0, 2, 0, product, type, 0, 0, time[0], time[1], timeout],
            this.scratch >>> 2);
        this.check(this.module._netInterfaceRequestsBegin(this.handle, this.scratch, 13, 1, this.scratch + 64));
        return this.record();
    }

    encode(timeout, bytes) {
        this.assertOpen();
        if (!(bytes instanceof Uint8Array) || bytes.length > 1048560) throw new Error('请求消息体非法');
        let input = 0;
        let output = 0;
        try {
            if (bytes.length) {
                input = this.module._malloc(bytes.length);
                if (!input) throw new Error('请求消息内存不足');
                this.module.HEAPU8.set(bytes, input);
            }
            output = this.module._malloc(bytes.length + 16);
            if (!output) throw new Error('请求信封内存不足');
            this.check(this.module._netInterfaceRpcEncode(3, timeout, 0, 0, input, bytes.length,
                output, bytes.length + 16, this.scratch));
            const size = this.module.HEAPU32[this.scratch >>> 2];
            return this.module.HEAPU8.slice(output, output + size);
        } finally {
            if (input) this.module._free(input);
            if (output) this.module._free(output);
        }
    }

    complete(frame, now) {
        this.assertOpen();
        const time = this.time(now);
        let input = 0;
        let response;
        try {
            const bytes = frame.payload;
            if (!(bytes instanceof Uint8Array) || bytes.length > 1048576) throw new Error('响应消息体非法');
            if (bytes.length) {
                input = this.module._malloc(bytes.length);
                if (!input) throw new Error('响应消息内存不足');
                this.module.HEAPU8.set(bytes, input);
            }
            this.module.HEAPU32.set([frame.kind, frame.product, frame.requestLow, frame.requestHigh,
                frame.type, bytes.length], this.scratch >>> 2);
            this.check(this.module._netInterfaceRpcParse(this.scratch, 6, input, this.scratch + 64));
            response = this.module.HEAPU32.slice((this.scratch + 64) >>> 2, (this.scratch + 84) >>> 2);
        } finally {
            if (input) this.module._free(input);
        }
        this.module.HEAPU32.set([1, 0, frame.requestLow, frame.requestHigh, 2, 0,
            frame.product, frame.type, time[0], time[1]], this.scratch >>> 2);
        const result = this.module._netInterfaceRequestsComplete(this.handle, this.scratch, 10, this.scratch + 64);
        if (result === -8) return null; // C++ has already retired this original ID.
        if (result !== -9 && result !== -10) this.check(result);
        return { record: this.record(), result, status: response[0],
            payload: frame.payload.slice(response[3], response[3] + response[4]) };
    }

    cancel(record) {
        this.assertOpen();
        const result = this.module._netInterfaceRequestsCancel(this.handle, 1, 0, record[4], record[5],
            this.scratch + 64);
        if (result === -8) return null;
        this.check(result);
        return result === 2 ? this.record() : null;
    }

    takeExpired(now) {
        this.assertOpen();
        const time = this.time(now);
        const result = this.module._netInterfaceRequestsTake(this.handle, 0, time[0], time[1], this.scratch + 64);
        if (result === -8) return null;
        this.check(result);
        return this.record();
    }

    count() {
        this.assertOpen();
        return this.check(this.module._netInterfaceRequestsCount(this.handle));
    }

    control(frame) {
        this.assertOpen();
        this.module.HEAPU32.set([frame.kind, frame.product, frame.requestLow, frame.requestHigh,
            frame.type, frame.payload.length], this.scratch >>> 2);
        const result = this.check(this.module._netInterfaceRpcControl(this.scratch, 6, this.scratch + 64));
        return result === 2 ? Array.from(this.module.HEAPU32.subarray(
            (this.scratch + 64) >>> 2, (this.scratch + 84) >>> 2)) : null;
    }

    dispose() {
        if (this.closed) return;
        this.closed = true;
        if (this.handle) this.module._netInterfaceRequestsDestroy(this.handle);
        if (this.scratch) this.module._free(this.scratch);
        this.handle = this.scratch = 0;
    }
}

module.exports = { NetInterfaceRequests };