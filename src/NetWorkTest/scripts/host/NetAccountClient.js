'use strict';
const { readPointer, u64Arguments } = require('./WasmMemory');
const { ACCOUNT_LOGOUT, ACCOUNT_RESET_PASSWORD } = require('../../../../include/Account/AccountProtocol');

// Wasm memory and network correlation only. Account.cpp / AccountClientState.cpp
// remain the sole implementation of identity, token handling and online state.
class NetAccountClient {
    constructor(runtime, product) {
        this.module = runtime.module;
        this.product = product;
        this.handle = 0;
        this.memory = 0;
        this.generation = [0, 0];
        try {
            this.memory = this.module._malloc(16416);
            if (!this.memory) throw new Error('Account memory budget exceeded');
            this.result(this.module._accountClientCreate(product, this.memory));
            this.handle = readPointer(this.module, this.memory);
        } catch (error) { this.dispose(); throw error; }
    }

    result(value) {
        if (value !== 0) {
            const error = new Error('Account operation failed: ' + value);
            error.accountResult = value;
            throw error;
        }
    }

    wire(value) {
        if (value < 0) {
            const error = new Error('Account network payload failed: ' + value);
            error.code = value;
            throw error;
        }
    }

    assertOpen() {
        if (!this.handle) throw new Error('Account client has been disposed');
    }

    setConnection(low, high = 0) {
        this.assertOpen();
        for (const value of [low, high]) if (!Number.isInteger(value) || value < 0 || value > 0xffffffff) throw new Error('Invalid generation word');
        this.result(this.module._accountClientSetConnection(this.handle, ...u64Arguments(this.module, low, high)));
        this.generation = [low, high];
    }

    encode(operation, requestId, parameters) {
        this.assertOpen();
        if (!(parameters instanceof Uint8Array) || parameters.length > 8192) throw new Error('Account parameters exceed budget');
        this.module.HEAPU8.set(parameters, this.memory + 32);
        try {
            this.wire(this.module._accountClientBuildRequest(this.handle, operation, ...u64Arguments(this.module, requestId[0], requestId[1]),
                this.memory + 32, parameters.length, this.memory + 8224, 8192, this.memory));
            const size = this.module.HEAPU32[this.memory >>> 2];
            return this.module.HEAPU8.slice(this.memory + 8224, this.memory + 8224 + size);
        } finally { this.module.HEAPU8.fill(0, this.memory + 32, this.memory + 16416); }
    }

    request(client, operation, parameters = new Uint8Array(0), timeout = 10000) {
        this.assertOpen();
        const generation = this.generation.slice();
        const handle = this.handle;
        let body;
        const pending = client.requestWithBuilder(this.product, operation, id => {
            body = this.encode(operation, id, parameters);
            return body;
        }, timeout);
        if (body) body.fill(0);
        const completed = (async () => { try {
            const response = await pending;
            if (this.handle !== handle || generation[0] !== this.generation[0] || generation[1] !== this.generation[1])
                throw new Error('Account response belongs to an old connection');
            if (!(response instanceof Uint8Array) || response.length > 8192) throw new Error('Account response exceeds budget');
            this.module.HEAPU8.set(response, this.memory + 32);
            try {
                this.result(this.module._accountClientApplyResponse(this.handle, ...u64Arguments(this.module, generation[0], generation[1]),
                    ...u64Arguments(this.module, pending.requestId[0], pending.requestId[1]), operation, 1, this.memory + 32, response.length));
            } finally { this.module.HEAPU8.fill(0, this.memory + 32, this.memory + 8224); }
            // Borrowed UI may decode this through NetInterfaceMessage functions;
            // never persist or log a raw login response containing its token.
            // 成功退出会由C++擦除令牌及最近业务响应；桥接层不要把这个正常空值当作网络失败。
            return { state: this.copyState(), response: (operation === ACCOUNT_LOGOUT || operation === ACCOUNT_RESET_PASSWORD) ? new Uint8Array(0) : this.copyResponse() };
        } catch (error) {
            if (this.handle === handle && generation[0] === this.generation[0] && generation[1] === this.generation[1])
                this.module._accountClientCancel(this.handle, ...u64Arguments(this.module, pending.requestId[0], pending.requestId[1]));
            throw error;
        } })();
        completed.cancel = pending.cancel;
        return completed;
    }

    copyState() {
        this.assertOpen();
        this.wire(this.module._accountClientCopyState(this.handle, this.memory + 8224, 8192, this.memory));
        const size = this.module.HEAPU32[this.memory >>> 2];
        const bytes = this.module.HEAPU8.slice(this.memory + 8224, this.memory + 8224 + size);
        this.module.HEAPU8.fill(0, this.memory + 8224, this.memory + 16416);
        return bytes;
    }

    copyResponse() {
        this.assertOpen();
        this.wire(this.module._accountClientCopyResponse(this.handle, this.memory + 8224, 8192, this.memory));
        const size = this.module.HEAPU32[this.memory >>> 2];
        const bytes = this.module.HEAPU8.slice(this.memory + 8224, this.memory + 8224 + size);
        this.module.HEAPU8.fill(0, this.memory + 8224, this.memory + 16416);
        return bytes;
    }

    authorization(parameters = new Uint8Array(0)) {
        this.assertOpen();
        if (!(parameters instanceof Uint8Array) || parameters.length > 8192) throw new Error('Account parameters exceed budget');
        this.module.HEAPU8.set(parameters, this.memory + 32);
        try {
            this.wire(this.module._accountClientBuildAuthorization(this.handle, this.memory + 32, parameters.length,
                this.memory + 8224, 8192, this.memory));
            const size = this.module.HEAPU32[this.memory >>> 2];
            return this.module.HEAPU8.slice(this.memory + 8224, this.memory + 8224 + size);
        } finally { this.module.HEAPU8.fill(0, this.memory + 32, this.memory + 16416); }
    }

    applyEvent(bytes) {
        this.assertOpen();
        if (!(bytes instanceof Uint8Array) || bytes.length > 8192) throw new Error('Account event exceeds budget');
        this.module.HEAPU8.set(bytes, this.memory + 32);
        try {
            return this.module._accountClientApplyEvent(this.handle, ...u64Arguments(this.module, this.generation[0], this.generation[1]), this.memory + 32, bytes.length);
        } finally { this.module.HEAPU8.fill(0, this.memory + 32, this.memory + 8224); }
    }

    dispose() {
        if (this.handle) this.module._accountClientDestroy(this.handle);
        if (this.memory) {
            this.module.HEAPU8.fill(0, this.memory, this.memory + 16416);
            this.module._free(this.memory);
        }
        this.handle = this.memory = 0;
        this.generation = [0, 0];
    }
}

module.exports = { NetAccountClient };