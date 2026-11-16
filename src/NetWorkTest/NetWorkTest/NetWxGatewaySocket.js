'use strict';

// 测试客户端显式声明线协议期望：字段17为协商窗口；实际额度计算仍由C++实现。
// 不依赖仓库共享include或旧版生成的JS旁文件，独立SDK也能执行同一用例。
const NI_GATEWAY_WINDOW = 17;
const { NetInterfaceLink } = require('./NetInterfaceLink');
const { NetInterfaceMessage } = require('./NetInterfaceMessage');
const { NetInterfaceCredit } = require('./NetInterfaceCredit');
const { NetInterfaceDirectory } = require('./NetInterfaceDirectory');

// Socket-shaped adapter carrying one service TLS stream through an authenticated gateway.
// Native wx.TCPSocket only transports the Wasm-produced bytes; no cryptography/protobuf parser in JS.
class NetWxGatewaySocket {
    constructor(runtime, options) {
        if (!Number.isInteger(options.serviceId) || options.serviceId < 0 || options.serviceId > 0xffffffff ||
            typeof options.createSocket !== 'function' || typeof options.clock !== 'function') throw new Error('目标服务配置非法');
        this.runtime = runtime;
        this.options = options;
        this.listeners = {};
        this.socket = this.link = this.timer = null;
        this.timerAt = 0;
        this.pumpQueued = false;
        this.credit = null;
        this.incoming = [];
        this.incomingBytes = 0;
        this.outgoing = [];
        this.outgoingBytes = 0;
        this.step = 0;
        this.directoryAt = 0;
        this.directorySentAt = 0;
        this.directoryRequest = 0;
        this.nextManagementRequest = 3;
        this.connected = this.ready = this.closed = this.pumping = false;
    }

    onConnect(callback) { this.listeners.connect = callback; }
    onMessage(callback) { this.listeners.message = callback; }
    onError(callback) { this.listeners.error = callback; }
    onClose(callback) { this.listeners.close = callback; }
    offConnect() { delete this.listeners.connect; }
    offMessage() { delete this.listeners.message; }
    offError() { delete this.listeners.error; }
    offClose() { delete this.listeners.close; }

    emit(name, value) { if (this.listeners[name]) this.listeners[name](value); }

    connect(address) {
        if (this.closed || this.socket) throw new Error('管理连接不能重复启动');
        this.link = new NetInterfaceLink(this.runtime);
        this.credit = new NetInterfaceCredit(this.runtime);
        this.socket = this.options.createSocket();
        this.socket.onConnect(() => { if (!this.closed) { this.connected = true; this.pump(); } });
        this.socket.onMessage(event => {
            if (this.closed) return;
            try {
                const value = event.message;
                const view = ArrayBuffer.isView(value);
                const buffer = view ? value.buffer : value;
                const length = Object.getOwnPropertyDescriptor(ArrayBuffer.prototype, 'byteLength').get.call(buffer);
                const bytes = new Uint8Array(buffer, view ? value.byteOffset : 0, view ? value.byteLength : length);
                if (bytes.length > 2097152 || this.incomingBytes + bytes.length > 2097152 || this.incoming.length >= 128) {
                    throw new Error('管理连接接收队列超限');
                }
                if (bytes.length) { this.incoming.push({ bytes: bytes.slice(), offset: 0 }); this.incomingBytes += bytes.length; }
                this.pump();
            } catch (error) { this.fail(error); }
        });
        this.socket.onError(event => this.fail(new Error(event.errMsg || '管理连接失败')));
        this.socket.onClose(() => { if (!this.closed) this.fail(new Error('管理连接已关闭')); });
        this.socket.connect(address);
    }

    canWrite(size) {
        return !this.closed && this.ready && size <= 16384 && this.outgoing.length < 32 && this.outgoingBytes + size <= 65536;
    }

    write(value) {
        const bytes = value instanceof Uint8Array ? value : new Uint8Array(value);
        if (!this.canWrite(bytes.length)) { const error = new Error('业务密文发送窗口已满'); error.code = 'BACKPRESSURE'; throw error; }
        this.outgoing.push(bytes.slice());
        this.outgoingBytes += bytes.length;
        this.schedule(0);
    }

    schedule(delay) {
        if (this.closed) return;
        if (delay === 0) {
            if (this.pumpQueued) return;
            if (this.timer !== null) clearTimeout(this.timer);
            this.timer = null;
            this.pumpQueued = true;
            Promise.resolve().then(() => {
                this.pumpQueued = false;
                if (!this.closed) this.pump();
            });
            return;
        }
        const due = this.options.clock() + delay;
        if (this.timer !== null && this.timerAt <= due) return;
        if (this.timer !== null) clearTimeout(this.timer);
        this.timerAt = due;
        this.timer = setTimeout(() => { this.timer = null; this.pump(); }, delay);
    }

    management(message) {
        const frame = message.fields;
        if (frame[0] === 4 && frame[1] === 0 && frame[3] === 0 && frame[4] === 0x4e470001 &&
            this.directoryRequest !== 0 && frame[2] === this.directoryRequest) {
            this.acceptNodes(message.bytes);
            this.directoryRequest = 0;
            return;
        }
        if (frame[0] === 5 && frame[1] === 0 && frame[2] === 0 && frame[3] === 0 && frame[4] === 0x4e470005) {
            const proto = new NetInterfaceMessage(this.runtime);
            try {
                proto.parse(message.bytes);
                const channel = proto.getValue(0, 9);
                const amount = proto.getValue(0, 14);
                if (!this.ready || !channel || channel.type !== 2 || channel.bytes.length !== 8 ||
                    new DataView(channel.bytes.buffer, channel.bytes.byteOffset, 8).getUint32(0, true) !== 1 ||
                    new DataView(channel.bytes.buffer, channel.bytes.byteOffset, 8).getUint32(4, true) !== 0 ||
                    !amount || amount.type !== 1 || amount.bytes.length !== 4) throw new Error('业务密文额度通道不匹配');
                this.credit.grant(new DataView(amount.bytes.buffer, amount.bytes.byteOffset, 4).getUint32(0, true));
            } finally { proto.dispose(); }
            return;
        }
        if (frame[0] === 5 && frame[4] === 0x4e470006) {
            const proto = new NetInterfaceMessage(this.runtime);
            try {
                proto.parse(message.bytes);
                const channel = proto.getValue(0, 9);
                if (frame[1] !== 0 || frame[2] !== 0 || frame[3] !== 0 || !channel || channel.type !== 2 ||
                    channel.bytes.length !== 8 || new DataView(channel.bytes.buffer, channel.bytes.byteOffset, 8).getUint32(0, true) !== 1 ||
                    new DataView(channel.bytes.buffer, channel.bytes.byteOffset, 8).getUint32(4, true) !== 0) {
                    throw new Error('关闭通知不属于当前通道');
                }
                const reason = proto.getValue(0, 13);
                const detail = reason && reason.bytes.length === 4 ?
                    new DataView(reason.bytes.buffer, reason.bytes.byteOffset, 4).getInt32(0, true) : 'missing';
                const error = new Error('目标服务通道已关闭：reason=' + detail);
                error.code = -6;
                throw error;
            } finally { proto.dispose(); }
        }
        if (frame[0] !== 4 || frame[1] !== 0 || frame[3] !== 0 ||
            frame[2] !== (this.step === 1 ? 1 : 2) || frame[4] !== (this.step === 1 ? 0x4e470001 : 0x4e470002)) {
            throw new Error('管理响应不属于当前请求');
        }
        const proto = new NetInterfaceMessage(this.runtime);
        try {
            proto.parse(message.bytes);
            const status = proto.getCode().code;
            if (status !== 0) { const error = new Error('目标服务不可用：' + status); error.code = status; throw error; }
            if (this.step === 1) {
                this.acceptNodes(message.bytes);
                this.step = 2;
            } else {
                const channel = proto.getValue(0, 9);
                if (!channel || channel.type !== 2 || channel.bytes.length !== 8 ||
                    new DataView(channel.bytes.buffer, channel.bytes.byteOffset, 8).getUint32(0, true) !== 1 ||
                    new DataView(channel.bytes.buffer, channel.bytes.byteOffset, 8).getUint32(4, true) !== 0) {
                    throw new Error('目标服务分配了错误的通道');
                }
                this.credit.configureMessage(message.bytes);
                this.step = 4;
                this.ready = true;
                this.emit('connect');
            }
        } finally { proto.dispose(); }
    }

    acceptNodes(bytes) {
        const directory = new NetInterfaceDirectory(this.runtime, bytes);
        try {
            if (typeof this.options.onNodes === 'function') this.options.onNodes(bytes.slice(), directory.points(), directory.cache());
            this.directoryAt = this.options.clock();
        } finally { directory.dispose(); }
    }

    pump() {
        if (this.closed || !this.connected || this.pumping) return;
        this.pumping = true;
        let moved = false;
        let writes = [];
        let writeBytes = 0;
        const flush = () => {
            if (!writeBytes || this.closed) return;
            const bytes = new Uint8Array(writeBytes);
            let offset = 0;
            for (const part of writes) { bytes.set(part, offset); offset += part.length; }
            this.socket.write(bytes.buffer);
            writes = [];
            writeBytes = 0;
        };
        try {
            for (let count = 0; count < 64 && !this.closed; ++count) {
                let current = false;
                if (this.incoming.length) {
                    const first = this.incoming[0];
                    const consumed = this.link.feed(first.bytes.subarray(first.offset));
                    first.offset += consumed;
                    this.incomingBytes -= consumed;
                    if (first.offset === first.bytes.length) this.incoming.shift();
                    current = consumed !== 0;
                }
                const status = this.link.progress(Math.floor(this.options.clock()));
                if (status === 2 && this.step === 0) {
                    if (this.link.sendControl([3, 0, 1, 0, 0x4e470001], new Uint8Array(0))) { this.step = 1; current = true; }
                }
                if (this.step === 2) {
                    const open = new NetInterfaceMessage(this.runtime);
                    try {
                        const window = this.options.windowBytes === undefined ? 1048576 : this.options.windowBytes;
                        if (!Number.isInteger(window) || window < 65536 || window > 1048576 || window % 16384) throw new Error('窗口配置非法');
                        const windowBytes = new Uint8Array(4);
                        new DataView(windowBytes.buffer).setInt32(0, window, true);
                        open.setValue(0, NI_GATEWAY_WINDOW, 1, windowBytes);
                        const target = new Uint8Array(8);
                        new DataView(target.buffer).setUint32(0, this.options.serviceId, true);
                        open.setValue(0, 8, 2, target);
                        const channel = new Uint8Array(8);
                        channel[0] = 1;
                        open.setValue(0, 9, 2, channel);
                        if (this.link.sendControl([3, 0, 2, 0, 0x4e470002], open.encode())) { this.step = 3; current = true; }
                    } finally { open.dispose(); }
                }
                for (let i = 0; i < 16 && !this.closed; ++i) {
                    const message = this.link.takeControl();
                    if (!message) break;
                    this.management(message);
                    current = true;
                }
                for (let i = 0; i < 16 && this.ready && !this.closed; ++i) {
                    const message = this.link.takeRecord();
                    if (!message) break;
                    const frame = message.fields;
                    if (frame[1] !== this.options.serviceId || frame[2] !== 1 || frame[3] !== 0 || frame[0] !== 3) {
                        const reason = frame[0] === 4 && message.bytes.length === 4 ?
                            new DataView(message.bytes.buffer, message.bytes.byteOffset, 4).getInt32(0, false) : 0;
                        const error = new Error('业务密文通道失效或不匹配：kind=' + frame[0] + ', service=' + frame[1] +
                            ', channel=' + frame[3] + ':' + frame[2] + ', reason=' + reason);
                        error.code = -6;
                        throw error;
                    }
                    this.credit.received(message.bytes.length);
                    this.emit('message', { message: message.bytes.buffer });
                    current = true;
                }
                if (this.closed) break;
                if (this.ready) {
                    const now = this.options.clock();
                    if (this.directoryRequest !== 0 && now - this.directorySentAt >= 5000) throw new Error('节点列表刷新超时');
                    if (this.directoryRequest === 0 && now - this.directoryAt >= 5000) {
                        if (this.nextManagementRequest >= 0xffffffff) throw new Error('管理请求编号已耗尽');
                        const request = this.nextManagementRequest;
                        if (this.link.sendControl([3, 0, request, 0, 0x4e470001], new Uint8Array(0))) {
                            this.directoryRequest = request;
                            ++this.nextManagementRequest;
                            this.directorySentAt = now;
                            current = true;
                        }
                    }
                }
                if (this.credit.pending() !== 0) {
                    const pending = this.credit.pending();
                    const grant = new NetInterfaceMessage(this.runtime);
                    try {
                        const channel = new Uint8Array(8);
                        channel[0] = 1;
                        const amount = new Uint8Array(4);
                        new DataView(amount.buffer).setUint32(0, pending, true);
                        grant.setValue(0, 9, 2, channel);
                        grant.setValue(0, 14, 1, amount);
                        if (this.link.sendControl([5, 0, 0, 0, 0x4e470005], grant.encode())) { this.credit.consume(pending); current = true; }
                    } finally { grant.dispose(); }
                }
                if (this.outgoing.length && this.outgoing[0].length <= this.credit.available() &&
                    this.link.sendRecord([3, this.options.serviceId, 1, 0], this.outgoing[0])) {
                    this.credit.sent(this.outgoing[0].length);
                    this.outgoingBytes -= this.outgoing[0].length;
                    this.outgoing.shift();
                    current = true;
                }
                const bytes = this.link.pullOutput();
                if (bytes) {
                    // Preserve record order while avoiding a native socket call for every
                    // small credit/control record. Never wait on a timer to flush a batch.
                    if (writeBytes + bytes.length > 65536) flush();
                    writes.push(bytes);
                    writeBytes += bytes.length;
                    if (writeBytes >= 65536) flush();
                    current = true;
                }
                moved = moved || current;
                if (!current) break;
            }
            flush();
            if (!this.closed && this.runtime.random.length - this.runtime.randomOffset < 1024) {
                this.runtime.prepare().catch(error => this.fail(error));
            }
        } catch (error) { this.fail(error); }
        finally { this.pumping = false; }
        if (!this.closed) this.schedule(moved || this.outgoing.length || this.incoming.length ? 1 : 25);
    }

    fail(error) {
        if (this.closed) return;
        this.close();
        this.emit('error', { errMsg: error.message || String(error), code: error.code });
        this.emit('close');
    }

    close() {
        if (this.closed) return;
        this.closed = true;
        this.ready = false;
        clearTimeout(this.timer);
        this.timer = null;
        if (this.socket) {
            for (const name of ['Connect', 'Message', 'Error', 'Close']) {
                if (typeof this.socket['off' + name] === 'function') this.socket['off' + name]();
            }
            try { this.socket.close(); } catch (_) { }
        }
        if (this.link) this.link.dispose();
        if (this.credit) this.credit.dispose();
        this.credit = null;
        this.socket = this.link = null;
        this.incoming = this.outgoing = [];
        this.incomingBytes = this.outgoingBytes = 0;
    }
}

module.exports = { NetWxGatewaySocket };