'use strict';
const { readPointer } = require('./WasmMemory');

// Copies validated public nodes through the C ABI; protobuf/cache rules stay in C++.
class NetInterfaceDirectory {
    constructor(runtime, bytes, cached = false, utc = Date.now()) {
        this.module = runtime.module;
        this.handle = this.scratch = 0;
        if (!(bytes instanceof Uint8Array) || bytes.length > 16432 || !Number.isSafeInteger(utc) || utc <= 0) {
            throw new Error('节点候选输入非法');
        }
        if (bytes.buffer === this.module.HEAPU8.buffer) bytes = bytes.slice();
        this.scratch = this.module._malloc(96);
        const input = this.module._malloc(Math.max(1, bytes.length));
        if (!this.scratch || !input) {
            if (input) this.module._free(input);
            this.dispose();
            throw new Error('节点候选内存不足');
        }
        try {
            this.module.HEAPU8.set(bytes, input);
            this.check(cached ? this.module._netInterfaceDirectoryCreateCache(input, bytes.length, utc >>> 0,
                Math.floor(utc / 4294967296), this.scratch) :
                this.module._netInterfaceDirectoryCreate(input, bytes.length, this.scratch));
            this.handle = readPointer(this.module, this.scratch);
        } catch (error) { this.dispose(); throw error; }
        finally { this.module._free(input); }
    }

    check(code) {
        if (code < 0) { const error = new Error('节点候选校验失败：' + code); error.code = code; throw error; }
    }

    points() {
        if (!this.handle) throw new Error('节点候选已关闭');
        this.check(this.module._netInterfaceDirectoryCount(this.handle, this.scratch));
        const count = this.module.HEAPU32[this.scratch >>> 2];
        const points = [];
        const text = (pointer, maximum) => {
            let value = '';
            for (let index = 0; index < maximum && this.module.HEAPU8[pointer + index]; ++index) {
                value += String.fromCharCode(this.module.HEAPU8[pointer + index]);
            }
            return value;
        };
        for (let index = 0; index < count; ++index) {
            this.check(this.module._netInterfaceDirectoryNode(this.handle, index,
                this.scratch + 16, 38, this.scratch + 54, 40, this.scratch));
            const values = this.module.HEAPU32.slice(this.scratch >>> 2, (this.scratch >>> 2) + 4);
            points.push({ name: text(this.scratch + 16, 38), address: text(this.scratch + 54, 40),
                port: values[0], capacity: values[1], boot: [values[2], values[3]] });
        }
        return points;
    }

    cache(utc = Date.now()) {
        if (!this.handle || !Number.isSafeInteger(utc) || utc <= 0) throw new Error('节点缓存状态非法');
        const low = utc >>> 0;
        const high = Math.floor(utc / 4294967296);
        const result = this.module._netInterfaceDirectoryCache(this.handle, low, high, 0, 0, this.scratch);
        if (result !== -4) this.check(result);
        const length = this.module.HEAPU32[this.scratch >>> 2];
        const output = this.module._malloc(length);
        if (!output) throw new Error('节点缓存内存不足');
        try {
            this.check(this.module._netInterfaceDirectoryCache(this.handle, low, high, output, length, this.scratch));
            return this.module.HEAPU8.slice(output, output + length);
        } finally { this.module._free(output); }
    }

    dispose() {
        if (this.handle) this.module._netInterfaceDirectoryDestroy(this.handle);
        if (this.scratch) this.module._free(this.scratch);
        this.handle = this.scratch = 0;
    }
}

module.exports = { NetInterfaceDirectory };