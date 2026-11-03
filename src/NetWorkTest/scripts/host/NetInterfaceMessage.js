'use strict';
const { readPointer } = require('./WasmMemory');

// Only manages handles and copies host values. Protobuf parsing, encoding, type
// validation and limits are implemented in the shared C++ library.
class NetInterfaceMessage {
    constructor(runtime) {
        this.module = runtime.module;
        this.handle = 0;
        this.scratch = this.module._malloc(16);
        if (!this.scratch) throw new Error('消息模块内存不足');
        try {
            this.check(this.module._netInterfaceMessageCreate(this.scratch));
            this.handle = readPointer(this.module, this.scratch);
        } catch (error) {
            this.dispose();
            throw error;
        }
    }

    check(result) {
        if (result < 0) {
            const error = new Error('消息处理失败：' + result);
            error.code = result;
            throw error;
        }
        return result;
    }

    assertOpen() {
        if (!this.handle) throw new Error('消息已关闭');
    }

    integer(value) {
        if (!Number.isInteger(value) || value < -2147483648 || value > 2147483647) {
            throw new Error('消息字段必须是32位整数');
        }
        return value;
    }

    withBytes(bytes, operation) {
        this.assertOpen();
        if (!(bytes instanceof Uint8Array) || bytes.length > 1048576) throw new Error('消息字节超限或类型错误');
        if (bytes.buffer === this.module.HEAPU8.buffer) bytes = bytes.slice();
        const pointer = this.module._malloc(Math.max(1, bytes.length));
        if (!pointer) throw new Error('消息模块内存不足');
        try {
            this.module.HEAPU8.set(bytes, pointer);
            return operation(pointer, bytes.length);
        } finally {
            // Bytes may contain a password or session token.
            this.module.HEAPU8.fill(0, pointer, pointer + bytes.length);
            this.module._free(pointer);
        }
    }

    output(operation) {
        this.assertOpen();
        let result = operation(0, 0);
        if (result !== -4) this.check(result);
        const length = this.module.HEAPU32[this.scratch >>> 2];
        if (length > 1048576) throw new Error('消息输出超限');
        const pointer = this.module._malloc(Math.max(1, length));
        if (!pointer) throw new Error('消息模块内存不足');
        try {
            this.check(operation(pointer, length));
            return this.module.HEAPU8.slice(pointer, pointer + length);
        } finally {
            this.module.HEAPU8.fill(0, pointer, pointer + length);
            this.module._free(pointer);
        }
    }

    clear() {
        this.assertOpen();
        this.check(this.module._netInterfaceMessageClear(this.handle));
    }

    parse(bytes) {
        this.clear();
        this.withBytes(bytes, (pointer, length) =>
            this.check(this.module._netInterfaceMessageParse(this.handle, pointer, length)));
        return this;
    }

    encode() {
        return this.output((pointer, capacity) =>
            this.module._netInterfaceMessageEncode(this.handle, pointer, capacity, this.scratch));
    }

    setValue(group, key, type, bytes) {
        this.integer(group);
        this.integer(key);
        this.integer(type);
        this.withBytes(bytes, (pointer, length) => this.check(
            this.module._netInterfaceMessageSetValue(this.handle, group, key, type, pointer, length)));
        return this;
    }

    getValue(group, key) {
        this.integer(group);
        this.integer(key);
        try {
            const bytes = this.output((pointer, capacity) => this.module._netInterfaceMessageGetValue(
                this.handle, group, key, this.scratch + 4, pointer, capacity, this.scratch));
            return { type: this.module.HEAP32[(this.scratch + 4) >>> 2], bytes };
        } catch (error) {
            if (error.code === -8) return null;
            throw error;
        }
    }

    setCode(code, errorBytes = new Uint8Array(0)) {
        this.integer(code);
        this.withBytes(errorBytes, (pointer, length) => this.check(
            this.module._netInterfaceMessageSetCode(this.handle, code, pointer, length)));
        return this;
    }

    getCode() {
        const bytes = this.output((pointer, capacity) => this.module._netInterfaceMessageGetCode(
            this.handle, this.scratch + 4, pointer, capacity, this.scratch));
        return { code: this.module.HEAP32[(this.scratch + 4) >>> 2], bytes };
    }

    dispose() {
        if (this.handle) this.module._netInterfaceMessageDestroy(this.handle);
        if (this.scratch) this.module._free(this.scratch);
        this.handle = this.scratch = 0;
    }
}

module.exports = { NetInterfaceMessage };