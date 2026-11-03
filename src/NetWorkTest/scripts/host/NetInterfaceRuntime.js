'use strict';
const { pointerBytes, heapPointer, readPointer, setModuleBits } = require('./WasmMemory');

// This file owns only Wasm memory and host I/O adaptation. All framing and TLS
// validation run in NetInterface C++; no socket or business protocol is encoded here.
class NetInterfaceRuntime {
    constructor(module, randomSource) {
        this.module = module;
        this.randomSource = randomSource;
        this.random = new Uint8Array(0);
        this.randomOffset = 0;
        this.refilling = null;
        this.randomError = null;
    }

    static async load(factory, options) {
        let runtime;
        const settings = Object.assign({}, options.moduleOptions, {
            netInterfaceEntropy(pointer, length) {
                return runtime ? runtime.copyRandom(pointer, length) : -1;
            }
        });
        const module = setModuleBits(await factory(settings), factory);
        runtime = new NetInterfaceRuntime(module, options.randomSource);
        await runtime.refillRandom();
        return runtime;
    }

    async refillRandom() {
        if (this.refilling) return this.refilling;
        if (typeof this.randomSource !== 'function') throw new Error('Strong random source is required');
        this.refilling = Promise.resolve().then(() => this.randomSource(8192)).then(value => {
            const bytes = value instanceof Uint8Array ? value : new Uint8Array(value);
            if (bytes.byteLength !== 8192) throw new Error('Strong random source returned a short buffer');
            const remaining = this.random.subarray(this.randomOffset);
            const joined = new Uint8Array(remaining.length + bytes.length);
            joined.set(remaining);
            joined.set(bytes, remaining.length);
            this.random.fill(0);
            bytes.fill(0);
            this.random = joined;
            this.randomOffset = 0;
            this.randomError = null;
        }).catch(error => {
            this.randomError = error;
            throw error;
        }).finally(() => { this.refilling = null; });
        return this.refilling;
    }

    copyRandom(pointer, length) {
        pointer = heapPointer(this.module, pointer);
        if (this.randomError || length > this.random.length - this.randomOffset) return -1;
        const end = this.randomOffset + length;
        this.module.HEAPU8.set(this.random.subarray(this.randomOffset, end), pointer);
        this.random.fill(0, this.randomOffset, end);
        this.randomOffset = end;
        return 0;
    }

    async prepare() {
        if (this.randomError || this.random.length - this.randomOffset < 1024) await this.refillRandom();
    }

    createSession(authorityPem, hostname, timeoutMs) {
        if (this.randomError || this.random.length - this.randomOffset < 1024) {
            throw new Error('Call prepare before creating a TLS session');
        }
        return new NetInterfaceSession(this, authorityPem, hostname, timeoutMs);
    }

    createServiceSession(product, timeoutMs) {
        if (!Number.isInteger(product) || product < 0 || product > 0xffffffff) throw new Error('目标服务编号非法');
        if (this.randomError || this.random.length - this.randomOffset < 1024) throw new Error('Call prepare before creating a TLS session');
        return new NetInterfaceSession(this, null, null, timeoutMs, product);
    }
}

// One serial connection. The public frame fields are six uint32 words; no C
// structure offsets, 64-bit JavaScript numbers, or cross-runtime frees are used.
class NetInterfaceSession {
    constructor(runtime, authorityPem, hostname, timeoutMs, serviceId) {
        this.runtime = runtime;
        this.module = runtime.module;
        this.connection = 0;
        this.scratch = 0;
        this.input = 0;
        this.closed = false;
        const temporary = [];
        try {
            this.scratch = this.allocate(64);
            this.input = this.allocate(32768);
            this.detailOffset = pointerBytes(this.module);
            let result;
            if (serviceId !== undefined) {
                result = this.module._netInterfaceWasmServiceConnectionCreate(serviceId, timeoutMs, this.scratch, this.scratch + this.detailOffset);
            } else {
                const root = this.asciiZ(authorityPem);
                temporary.push(root);
                const name = this.asciiZ(hostname);
                temporary.push(name);
                result = this.module._netInterfaceWasmConnectionCreate(root, authorityPem.length + 1,
                    name, timeoutMs, this.scratch, this.scratch + this.detailOffset);
            }
            this.connection = readPointer(this.module, this.scratch);
            this.check(result, 'connection configuration');
        } catch (error) {
            this.destroy();
            throw error;
        } finally {
            temporary.forEach(pointer => this.module._free(pointer));
        }
    }

    allocate(size) {
        const pointer = this.module._malloc(size);
        if (!pointer) throw new Error('NetInterface memory budget exceeded');
        return pointer;
    }

    asciiZ(text) {
        if (typeof text !== 'string' || text.length > 131071 || /[^\x01-\x7f]/.test(text)) {
            throw new Error('Expected bounded ASCII certificate or hostname');
        }
        const pointer = this.allocate(text.length + 1);
        for (let index = 0; index < text.length; ++index) this.module.HEAPU8[pointer + index] = text.charCodeAt(index);
        this.module.HEAPU8[pointer + text.length] = 0;
        return pointer;
    }

    word(index) { return this.module.HEAPU32[(this.scratch >>> 2) + index]; }

    check(result, operation) {
        if (result < 0) {
            if (this.connection) this.module._netInterfaceConnectionGetStatus(this.connection, this.scratch + this.detailOffset);
            const detail = this.module.HEAP32[(this.scratch + this.detailOffset) / 4];
            const error = new Error(`${operation}: ${result}, TLS detail ${detail}`);
            error.code = result;
            error.detail = detail;
            throw error;
        }
        return result;
    }

    assertOpen() {
        if (this.closed) throw new Error('NetInterface session is closed');
    }

    feed(bytes) {
        this.assertOpen();
        const amount = Math.min(bytes.length, 32768);
        this.module.HEAPU8.set(bytes.subarray(0, amount), this.input);
        this.check(this.module._netInterfaceConnectionFeed(this.connection, this.input, amount, this.scratch), 'connection input');
        return this.word(0);
    }

    progress(monotonicMs) {
        this.assertOpen();
        if (!Number.isSafeInteger(monotonicMs) || monotonicMs < 0) throw new Error('Invalid monotonic time');
        return this.check(this.module._netInterfaceConnectionProgress(this.connection,
            monotonicMs >>> 0, Math.floor(monotonicMs / 4294967296)), 'connection progress');
    }

    pullCiphertext(limit) {
        const bytes = this.peekCiphertext(limit);
        if (bytes) this.consumeCiphertext(bytes.length);
        return bytes;
    }

    peekCiphertext(limit) {
        this.assertOpen();
        this.check(this.module._netInterfaceConnectionPeekOutput(this.connection, this.scratch, this.scratch + this.detailOffset), 'connection output');
        const size = Math.min(this.word(this.detailOffset / 4), limit);
        if (!size) return null;
        const pointer = readPointer(this.module, this.scratch);
        const bytes = this.module.HEAPU8.slice(pointer, pointer + size);
        // Return an owned copy; native TCPSocket.write may retain its ArrayBuffer.
        return bytes;
    }

    consumeCiphertext(size) {
        this.assertOpen();
        this.check(this.module._netInterfaceConnectionConsumeOutput(this.connection, size), 'connection output consumption');
    }

    configureCompression(types, negotiated) {
        this.assertOpen();
        if (!Array.isArray(types) || types.length > 64 || types.some(type => !Number.isInteger(type) || type < 1 || type > 0xffffffff))
            throw new Error('Invalid public RPC compression whitelist');
        const pointer = types.length ? this.allocate(types.length * 4) : 0;
        try {
            if (pointer) this.module.HEAPU32.set(types, pointer >>> 2);
            this.check(this.module._netInterfaceConnectionConfigureCompression(this.connection, pointer, types.length, negotiated ? 1 : 0),
                'application compression negotiation');
        } finally { if (pointer) this.module._free(pointer); }
    }

    sendFrame(fields, payload) {
        this.assertOpen();
        if (!Array.isArray(fields) || fields.length !== 5 || fields.some(value =>
            !Number.isInteger(value) || value < 0 || value > 0xffffffff)) throw new Error('Invalid frame fields');
        if (!(payload instanceof Uint8Array) || payload.length > 1048576) throw new Error('Frame payload exceeds limit');
        // A caller may pass a Wasm heap view; snapshot before malloc can grow it.
        const bytes = payload.buffer === this.module.HEAPU8.buffer ? payload.slice() : payload;
        let body = 0;
        try {
            body = this.allocate(bytes.length || 1);
            this.module.HEAPU8.set(bytes, body);
            this.module.HEAPU32.set(fields.concat(bytes.length), this.scratch >>> 2);
            const result = this.module._netInterfaceConnectionSend(this.connection, this.scratch, body);
            if (result === -3) {
                const error = new Error('Gateway send queue is full');
                error.code = 'BACKPRESSURE';
                throw error;
            }
            this.check(result, 'frame send');
        } finally {
            if (body) {
                this.module.HEAPU8.fill(0, body, body + bytes.length);
                this.module._free(body);
            }
        }
    }

    takeFrame() {
        this.assertOpen();
        const status = this.check(this.module._netInterfaceConnectionPeek(this.connection,
            this.scratch, this.scratch + 24), 'frame view');
        if (status !== 2) return null;
        const fields = Array.from(this.module.HEAPU32.subarray(this.scratch >>> 2, (this.scratch >>> 2) + 6));
        const pointer = readPointer(this.module, this.scratch + 24);
        const payload = this.module.HEAPU8.slice(pointer, pointer + fields[5]);
        this.check(this.module._netInterfaceConnectionConsume(this.connection), 'frame consumption');
        return { kind: fields[0], product: fields[1], requestLow: fields[2],
            requestHigh: fields[3], type: fields[4], payload };
    }

    finishInput() {
        this.assertOpen();
        this.check(this.module._netInterfaceConnectionFinishInput(this.connection), 'TCP EOF');
    }

    destroy() {
        if (this.closed) return;
        this.closed = true;
        if (this.connection) this.module._netInterfaceConnectionDestroy(this.connection);
        if (this.scratch) this.module._free(this.scratch);
        if (this.input) this.module._free(this.input);
        this.connection = this.scratch = this.input = 0;
    }
}

// Test host adaptation only. The compiled NetPeer binding owns protocols and socket queues.
const NetPeerRuntime = {
    async load(factory, options) {
        const module = setModuleBits(await factory(options.moduleOptions || {}), factory);
        if (typeof module.NetPeerRuntime !== 'function') throw new Error('Select the complete NetPeer SDK');
        const runtime = await module.NetPeerRuntime.create({
            randomSource: options.randomSource,
            createSocket(events) {
                const socket = options.createSocket();
                socket.onConnect(() => events.onConnect());
                socket.onMessage(value => events.onData(value.message));
                socket.onError(value => events.onError(value));
                socket.onClose(() => events.onClose(0));
                return {
                    connect(address, port) { return socket.connect({ address, port }); },
                    write(bytes) { return socket.write(bytes); },
                    close() { socket.close(); },
                    dispose() {
                        for (const name of ['offConnect', 'offMessage', 'offError', 'offClose']) {
                            if (typeof socket[name] === 'function') socket[name]();
                        }
                        socket.close();
                    }
                };
            }
        });
        runtime.dispose = () => {
            if (runtime.clients !== 0 || runtime.hosts.size !== 0) throw new Error('Test owner must quiesce clients before dropping its module');
        };
        return runtime;
    }
};
module.exports = { NetInterfaceRuntime, NetPeerRuntime };