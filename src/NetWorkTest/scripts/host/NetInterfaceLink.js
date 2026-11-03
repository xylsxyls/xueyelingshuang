'use strict';
const { pointerBytes, readPointer } = require('./WasmMemory');

// Owns Wasm handles/copies only. NTW3, management TLS, proofs and heartbeat stay in C++.
class NetInterfaceLink {
    constructor(runtime) {
        this.runtime = runtime;
        this.module = runtime.module;
        this.link = this.decoder = this.scratch = this.input = this.wire = 0;
        try {
            this.scratch = this.allocate(48);
            this.resultOffset = 24 + pointerBytes(this.module);
            this.input = this.allocate(32768);
            this.wire = this.allocate(16408);
            this.check(this.module._netInterfaceWasmLinkCreateClient(0, 0, 5000, this.scratch));
            this.link = readPointer(this.module, this.scratch);
            this.check(this.module._netInterfaceTunnelCreate(this.scratch));
            this.decoder = readPointer(this.module, this.scratch);
        } catch (error) { this.dispose(); throw error; }
    }

    allocate(size) {
        const pointer = this.module._malloc(size);
        if (!pointer) throw new Error('管理网络模块内存不足');
        return pointer;
    }

    word(index) { return this.module.HEAPU32[(this.scratch >>> 2) + index]; }

    check(code) {
        if (code < 0) { const error = new Error('管理网络协议失败：' + code); error.code = code; throw error; }
        return code;
    }

    acceptPending() {
        const status = this.check(this.module._netInterfaceTunnelPeek(this.decoder, this.scratch, this.scratch + 24));
        if (status !== 2) return true;
        const accepted = this.module._netInterfaceLinkAccept(this.link, this.scratch, readPointer(this.module, this.scratch + 24));
        if (accepted === -3) return false;
        this.check(accepted);
        this.check(this.module._netInterfaceTunnelConsume(this.decoder));
        return true;
    }

    feed(bytes) {
        if (!this.acceptPending()) return 0;
        const size = Math.min(bytes.length, 32768);
        this.module.HEAPU8.set(bytes.subarray(0, size), this.input);
        this.check(this.module._netInterfaceTunnelFeed(this.decoder, this.input, size, this.scratch + this.resultOffset));
        const consumed = this.word(this.resultOffset / 4);
        this.acceptPending();
        return consumed;
    }

    progress(now) {
        if (!Number.isSafeInteger(now) || now < 0) throw new Error('管理网络时钟非法');
        const result = this.check(this.module._netInterfaceLinkProgress(this.link, now >>> 0, Math.floor(now / 4294967296)));
        this.acceptPending();
        return result;
    }

    send(fields, bytes, control) {
        if (!(bytes instanceof Uint8Array) || bytes.length > 16384) throw new Error('管理网络消息超限');
        const pointer = this.allocate(bytes.length || 1);
        try {
            this.module.HEAPU8.set(bytes, pointer);
            this.module.HEAPU32.set(fields.concat(bytes.length), this.scratch >>> 2);
            const result = control ? this.module._netInterfaceLinkSendControl(this.link, this.scratch, pointer) :
                this.module._netInterfaceLinkSendRecord(this.link, this.scratch, pointer);
            if (result === -3) return false;
            this.check(result);
            return true;
        } finally {
            this.module.HEAPU8.fill(0, pointer, pointer + bytes.length);
            this.module._free(pointer);
        }
    }

    sendControl(fields, bytes) { return this.send(fields, bytes, true); }
    sendRecord(fields, bytes) { return this.send(fields, bytes, false); }

    take(control) {
        const result = control ? this.module._netInterfaceLinkPeekControl(this.link, this.scratch, this.scratch + 24) :
            this.module._netInterfaceLinkPeekRecord(this.link, this.scratch, this.scratch + 24);
        if (this.check(result) !== 2) return null;
        const count = control ? 6 : 5;
        const fields = Array.from(this.module.HEAPU32.subarray(this.scratch >>> 2, (this.scratch >>> 2) + count));
        const pointer = readPointer(this.module, this.scratch + 24);
        const bytes = this.module.HEAPU8.slice(pointer, pointer + fields[count - 1]);
        this.check(control ? this.module._netInterfaceLinkConsumeControl(this.link) : this.module._netInterfaceLinkConsumeRecord(this.link));
        return { fields, bytes };
    }

    takeControl() { return this.take(true); }
    takeRecord() { return this.take(false); }

    pullOutput() {
        if (this.check(this.module._netInterfaceLinkPeekOutput(this.link, this.scratch, this.scratch + 24)) !== 2) return null;
        this.check(this.module._netInterfaceTunnelEncode(this.scratch, readPointer(this.module, this.scratch + 24), this.wire, 16408, this.scratch + this.resultOffset));
        const result = this.module.HEAPU8.slice(this.wire, this.wire + this.word(this.resultOffset / 4));
        this.check(this.module._netInterfaceLinkConsumeOutput(this.link));
        return result;
    }

    dispose() {
        if (this.link) this.module._netInterfaceLinkDestroy(this.link);
        if (this.decoder) this.module._netInterfaceTunnelDestroy(this.decoder);
        for (const pointer of [this.scratch, this.input, this.wire]) if (pointer) this.module._free(pointer);
        this.link = this.decoder = this.scratch = this.input = this.wire = 0;
    }
}

module.exports = { NetInterfaceLink };