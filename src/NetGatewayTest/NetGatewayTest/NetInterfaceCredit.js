'use strict';
const { readPointer } = require('./WasmMemory');

// Only binds the C++ per-channel window; JS does not duplicate its accounting rules.
class NetInterfaceCredit {
    constructor(runtime) {
        this.module = runtime.module;
        this.handle = 0;
        this.scratch = this.module._malloc(8);
        if (!this.scratch) throw new Error('通道窗口内存不足');
        try {
            this.check(this.module._netTestInterfaceCreditCreate(this.scratch));
            this.handle = readPointer(this.module, this.scratch);
        } catch (error) { this.dispose(); throw error; }
    }
    check(code) {
        if (code < 0) { const error = new Error('通道窗口协议失败：' + code); error.code = code; throw error; }
    }
    available() {
        this.check(this.module._netTestInterfaceCreditGet(this.handle, this.scratch));
        return this.module.HEAPU32[this.scratch >>> 2];
    }
    pending() {
        this.check(this.module._netTestInterfaceCreditGet(this.handle, this.scratch));
        return this.module.HEAPU32[(this.scratch + 4) >>> 2];
    }
    configureMessage(bytes) {
        const input = this.module._malloc(bytes.length);
        if (!input) throw new Error('窗口协商内存不足');
        try {
            this.module.HEAPU8.set(bytes, input);
            this.check(this.module._netTestInterfaceCreditConfigureMessage(this.handle, input, bytes.length));
        } finally { this.module._free(input); }
    }
    sent(size) { this.check(this.module._netTestInterfaceCreditSent(this.handle, size)); }
    received(size) { this.check(this.module._netTestInterfaceCreditReceived(this.handle, size)); }
    grant(size) { this.check(this.module._netTestInterfaceCreditGrant(this.handle, size)); }
    consume(size) { this.check(this.module._netTestInterfaceCreditConsume(this.handle, size)); }
    dispose() {
        if (this.handle) this.module._netTestInterfaceCreditDestroy(this.handle);
        if (this.scratch) this.module._free(this.scratch);
        this.handle = this.scratch = 0;
    }
}

module.exports = { NetInterfaceCredit };