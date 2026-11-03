'use strict';

const { NetInterfaceRuntime } = require('./NetInterfaceRuntime');
const { NetInterfaceHandshake } = require('./NetInterfaceHandshake');
const { NetInterfaceRequests } = require('./NetInterfaceRequests');
const { NetWxGatewaySocket } = require('./NetWxGatewaySocket');
const { NetWxServiceClient } = require('./NetWxServiceClient');

// wx-native adapter. The caller supplies the generated module factory, code-package
// wasm path and trusted CA. No insecure fallback or HTTP bypass is provided.
async function loadWxRuntime(factory, wasmPath, wxApi) {
    if (typeof WXWebAssembly === 'undefined' || !wxApi || typeof wxApi.getRandomValues !== 'function') {
        throw new Error('当前微信版本不支持安全网络模块');
    }
    let rejectLoad;
    let timer;
    const failed = new Promise((resolve, reject) => {
        rejectLoad = reject;
        timer = setTimeout(() => reject(new Error('网络模块加载超时')), 10000);
    });
    const loaded = NetInterfaceRuntime.load(factory, {
        randomSource: length => new Promise((resolve, reject) => wxApi.getRandomValues({
            length, success: result => resolve(result.randomValues),
            fail: error => reject(new Error(error.errMsg || '强随机源不可用'))
        })),
        moduleOptions: {
            wasmApi: WXWebAssembly,
            instantiateWasm(imports, receiveInstance) {
                WXWebAssembly.instantiate(wasmPath, imports).then(result => {
                    receiveInstance(result.instance, result.module);
                }).catch(rejectLoad);
                return {};
            },
            onAbort: reason => rejectLoad(new Error(String(reason)))
        }
    });
    try {
        return await Promise.race([loaded, failed]);
    } finally {
        clearTimeout(timer);
    }
}

// The transport only moves bytes. Authentication, RPC, replay protection, framing
// and encryption belong to the shared C++ modules. One instance is one connection.
class NetWxClient {
    constructor(options) {
        this.options = options;
        this.runtime = options.runtime;
        this.socket = null;
        this.session = null;
        this.application = null;
        this.negotiated = null;
        this.requests = null;
        this.callbacks = new Map();
        this.incoming = [];
        this.incomingBytes = 0;
        this.connected = false;
        this.ready = false;
        this.disposed = false;
        this.pumping = false;
        this.pumpTimer = null;
        this.pumpTimerAt = 0;
        this.pumpQueued = false;
        this.deadlineTimer = null;
        this.connectPromise = null;
        this.resolveConnect = null;
        this.rejectConnect = null;
        this.lastPing = 0;
        this.lastPong = 0;
        this.pingLow = 0;
        this.pingHigh = 0;
        this.pingOutstanding = false;
    }

    connect() {
        if (this.disposed) return Promise.reject(new Error('连接已关闭'));
        if (this.connectPromise) return this.connectPromise;
        this.connectPromise = new Promise((resolve, reject) => {
            this.resolveConnect = resolve;
            this.rejectConnect = reject;
            this.deadlineTimer = setTimeout(() => this.fail(new Error('网关连接超时')), 10000);
            this.open().catch(error => this.fail(error));
        });
        return this.connectPromise;
    }

    async open() {
        const options = this.options;
        if (!Number.isInteger(options.port) || options.port < 1024 || options.port > 65535 ||
            typeof options.clock !== 'function' || typeof options.createSocket !== 'function') {
            throw new Error('网关连接配置不完整');
        }
        await this.runtime.prepare();
        if (this.disposed) return;
        this.deviceId = typeof options.loadDevice === 'function' ? await options.loadDevice() : options.deviceId;
        if (this.disposed) return;
        if (!(this.deviceId instanceof Uint8Array) || this.deviceId.length !== 16) {
            throw new Error('网关设备标识未配置');
        }
        this.deviceId = this.deviceId.slice();
        this.session = options.protocol === 3 ? this.runtime.createServiceSession(options.serviceId, 10000) :
            this.runtime.createSession(options.authorityPem, options.hostname, 10000);
        this.socket = options.createSocket();
        this.listeners = {
            connect: () => {
                if (this.disposed) return;
                this.connected = true;
                this.schedulePump();
            },
            message: event => {
                if (this.disposed) return;
                const value = event.message;
                // 微信开发工具的socket回调可来自另一JS执行域，instanceof会误拒真实ArrayBuffer。
                // 用原生byteLength getter核实内部类型；不接纳仅伪造byteLength的普通对象。
                let buffer;
                let offset = 0;
                let length;
                try {
                    const view = ArrayBuffer.isView(value);
                    buffer = view ? value.buffer : value;
                    const bytes = Object.getOwnPropertyDescriptor(ArrayBuffer.prototype, 'byteLength').get.call(buffer);
                    offset = view ? value.byteOffset : 0;
                    length = view ? value.byteLength : bytes;
                    if (!Number.isSafeInteger(offset) || !Number.isSafeInteger(length) || offset < 0 || length < 0 ||
                        offset + length > bytes) throw new Error('Invalid TCP binary view');
                } catch (_) { return this.fail(new Error('TCP返回了非法二进制数据')); }
                if (length > 1048576 || this.incomingBytes + length > 2097152 ||
                    this.incoming.length >= 128) return this.fail(new Error('网关接收队列超限'));
                if (length) {
                    this.incoming.push({ bytes: new Uint8Array(buffer, offset, length).slice(), offset: 0 });
                    this.incomingBytes += length;
                }
                // Consume a bounded slice during the host delivery callback. A
                // timer alone can starve behind consecutive TCP events and fill
                // the queue even while the decoder itself has capacity.
                if (this.connected) this.pump();
                else this.schedulePump();
            },
            error: event => {
                const error = new Error(event.errMsg || '网关网络错误');
                if (Number.isInteger(event.code)) error.code = event.code;
                this.fail(error);
            },
            close: () => {
                if (this.disposed) return;
                if (!this.connected) return this.fail(new Error('TCP连接建立前已关闭'));
                // The buffered TLS close record, if any, is processed before TCP EOF.
                this.tcpEnded = true;
                this.schedulePump();
            }
        };
        this.socket.onConnect(this.listeners.connect);
        this.socket.onMessage(this.listeners.message);
        this.socket.onError(this.listeners.error);
        this.socket.onClose(this.listeners.close);
        this.socket.connect({ address: options.address || options.hostname, port: options.port });
    }

    sendFrame(fields, payload) {
        if (!this.ready || this.disposed) throw new Error('网关尚未连接');
        if (!(payload instanceof Uint8Array)) throw new Error('消息体必须是二进制数据');
        this.session.sendFrame(fields, payload);
        this.schedulePump();
    }

    // Returns a normal Promise with an explicit cancel() method. Host callback
    // entries are keyed by the exact low/high words returned by C++, never Number.
    request(product, type, payload, timeout = 10000) {
        return this.requestWithBuilder(product, type, () => payload, timeout);
    }

    // The shared request manager allocates the ID before a dependent SDK builds
    // its payload. It remains a single admission/terminal-result contract.
    requestWithBuilder(product, type, builder, timeout = 10000) {
        if (!this.ready || this.disposed) throw new Error('网关尚未连接');
        if (typeof builder !== 'function') throw new Error('请求构造器非法');
        const record = this.requests.begin(product, type, timeout, Math.floor(this.options.clock()));
        const key = `${record[4]}:${record[5]}`;
        let resolveRequest;
        let rejectRequest;
        const promise = new Promise((resolve, reject) => { resolveRequest = resolve; rejectRequest = reject; });
        this.callbacks.set(key, { resolve: resolveRequest, reject: rejectRequest });
        promise.cancel = () => {
            if (this.disposed || !this.callbacks.has(key)) return;
            const cancelled = this.requests.cancel(record);
            if (cancelled) {
                this.finishRequest(cancelled, this.rpcError(-10));
                this.sendCancellation(cancelled);
            }
        };
        try {
            const body = this.requests.encode(timeout, builder([record[4], record[5]]));
            this.sendFrame([3, product, record[4], record[5], type], body);
        } catch (error) {
            this.requests.cancel(record);
            this.finishRequest(record, error);
        }
        promise.requestId = [record[4], record[5]];
        return promise;
    }

    rpcError(code, status) {
        const error = new Error(status === undefined ? `网关请求失败：${code}` : `网关返回失败：${status}`);
        error.code = code;
        if (status !== undefined) error.status = status;
        return error;
    }

    finishRequest(record, error, payload) {
        const key = `${record[4]}:${record[5]}`;
        const callback = this.callbacks.get(key);
        if (!callback) return;
        this.callbacks.delete(key);
        if (error) callback.reject(error);
        else callback.resolve(payload);
    }

    sendCancellation(record) {
        if (this.disposed) return;
        try {
            this.sendFrame([6, record[14], record[4], record[5], record[15]], new Uint8Array(0));
        } catch (error) {
            // If even the reserved control queue is full, close this connection
            // so the gateway can retire all remaining work for its generation.
            this.fail(error);
        }
    }

    acceptBusiness(frame, now) {
        if (frame.kind === 7 || frame.kind === 8) {
            const reply = this.requests.control(frame);
            if (reply) this.sendFrame(reply, new Uint8Array(0));
            if (frame.kind === 8 && this.pingOutstanding && frame.requestLow === this.pingLow && frame.requestHigh === this.pingHigh) {
                this.lastPong = now;
                this.pingOutstanding = false;
            }
        } else if (frame.kind === 4) {
            const response = this.requests.complete(frame, now);
            // A late/already-settled response is consumed, not host backpressure.
            if (!response) return true;
            const error = response.result < 0 ? this.rpcError(response.result) :
                response.status ? this.rpcError(-7, response.status) : null;
            this.finishRequest(response.record, error, response.payload);
        } else if (this.options.onFrame) {
            return this.options.onFrame(frame) !== false;
        }
        return true;
    }

    schedulePump(delay = 0) {
        if (this.disposed) return;
        if (delay === 0) {
            if (this.pumpQueued) return;
            if (this.pumpTimer !== null) clearTimeout(this.pumpTimer);
            this.pumpTimer = null;
            this.pumpQueued = true;
            // Host background timer clamping must not add a second to each
            // request/ACK. One bounded pump per microtask; retries still yield.
            Promise.resolve().then(() => {
                this.pumpQueued = false;
                if (!this.disposed) this.pump();
            });
            return;
        }
        const due = this.options.clock() + delay;
        // Business data and file ACKs preempt an idle heartbeat deadline. Keep
        // an already earlier wakeup so consecutive events cannot postpone it.
        if (this.pumpTimer !== null && this.pumpTimerAt <= due) return;
        if (this.pumpTimer !== null) clearTimeout(this.pumpTimer);
        this.pumpTimerAt = due;
        this.pumpTimer = setTimeout(() => {
            this.pumpTimer = null;
            this.pump();
        }, delay);
    }

    pump() {
        if (this.disposed || !this.connected || this.pumping) return;
        this.pumping = true;
        let outputBudget = 65536;
        let reschedule = false;
        try {
            for (let step = 0; step < 64 && !this.disposed; ++step) {
                let moved = false;
                if (this.incoming.length) {
                    const first = this.incoming[0];
                    const consumed = this.session.feed(first.bytes.subarray(first.offset));
                    first.offset += consumed;
                    this.incomingBytes -= consumed;
                    moved = moved || consumed > 0;
                    if (first.offset === first.bytes.length) this.incoming.shift();
                } else if (this.tcpEnded) {
                    this.session.finishInput();
                }
                const now = Math.floor(this.options.clock());
                const state = this.session.progress(now);
                if (state === 2) {
                    if (!this.application) {
                        this.application = new NetInterfaceHandshake(this.runtime,
                            this.options.platform === undefined ? 3 : this.options.platform, this.deviceId, now, this.options.protocol === 3 ? 15 : 7);
                    }
                    this.application.progress(now);
                    const hello = this.application.peekOutput();
                    if (hello) {
                        this.session.sendFrame(hello.fields, hello.payload);
                        this.application.consumeOutput();
                        moved = true;
                    }
                    const frame = this.pendingFrame || this.session.takeFrame();
                    if (frame) {
                        moved = true;
                        if (this.pendingFrame || this.application.accept(frame, now)) {
                            if (!this.acceptBusiness(frame, now)) {
                                this.pendingFrame = frame;
                                moved = false;
                            } else this.pendingFrame = null;
                        }
                    }
                    if (this.disposed) break;
                    if (this.application.ready && !this.ready) {
                        this.negotiated = this.application.peerInfo();
                        if ((this.negotiated.capabilities & 8) !== 0)
                            this.session.configureCompression(this.options.compressionTypes || [], true);
                        this.ready = true;
                        this.requests = new NetInterfaceRequests(this.runtime, this.negotiated.maxRequests);
                        this.lastPing = this.lastPong = now;
                        clearTimeout(this.deadlineTimer);
                        this.deadlineTimer = null;
                        this.resolveConnect(this);
                        this.resolveConnect = this.rejectConnect = null;
                    }
                }
                if (state === 3) {
                    this.dispose(new Error('网关已关闭连接'));
                    break;
                }
                if (this.disposed) break;
                const bytes = this.session.peekCiphertext(Math.min(outputBudget, 16384));
                if (bytes) {
                    if (this.socket.canWrite && !this.socket.canWrite(bytes.length)) { reschedule = true; break; }
                    // A copied ArrayBuffer has its own lifetime. write returning means
                    // handed to WeChat, never a business acknowledgement or delivery receipt.
                    this.socket.write(bytes.buffer);
                    this.session.consumeCiphertext(bytes.length);
                    outputBudget -= bytes.length;
                    moved = true;
                }
                if (outputBudget === 0) {
                    reschedule = true;
                    break;
                }
                if (!moved) break;
                if (step === 63) reschedule = true;
            }
            if (!this.disposed && this.runtime.random.length - this.runtime.randomOffset < 1024) {
                this.runtime.prepare().catch(error => this.fail(error));
            }
            if (!this.disposed && this.requests) {
                const now = Math.floor(this.options.clock());
                if (this.pingOutstanding && now - this.lastPing >= 60000) throw new Error('网关心跳超时');
                if (!this.pingOutstanding && now - this.lastPing >= 30000) {
                    this.pingLow = (this.pingLow + 1) >>> 0;
                    if (!this.pingLow) this.pingHigh = (this.pingHigh + 1) >>> 0;
                    if (!this.pingLow && !this.pingHigh) throw new Error('网关心跳编号耗尽');
                    this.sendFrame([7, 0, this.pingLow, this.pingHigh, 0], new Uint8Array(0));
                    this.lastPing = now;
                    this.pingOutstanding = true;
                }
                for (let count = 0; count < 128 && !this.disposed; ++count) {
                    const record = this.requests.takeExpired(now);
                    if (!record) break;
                    this.finishRequest(record, this.rpcError(-9));
                    this.sendCancellation(record);
                }
            }
        } catch (error) {
            this.fail(error);
        } finally {
            this.pumping = false;
        }
        if (reschedule) this.schedulePump(1);
        else if (!this.disposed) {
            const delay = !this.ready || this.pendingFrame || this.requests.count() !== 0 ? 25 : 1000;
            this.schedulePump(delay);
        }
    }

    fail(error) {
        this.dispose(error instanceof Error ? error : new Error(String(error)));
    }

    dispose(reason) {
        if (this.disposed) return;
        this.disposed = true;
        this.ready = false;
        clearTimeout(this.deadlineTimer);
        clearTimeout(this.pumpTimer);
        this.deadlineTimer = this.pumpTimer = null;
        if (this.rejectConnect) this.rejectConnect(reason || new Error('连接已取消'));
        this.resolveConnect = this.rejectConnect = null;
        if (this.socket) {
            const socket = this.socket;
            this.socket = null;
            if (this.listeners) {
                if (socket.offConnect) socket.offConnect(this.listeners.connect);
                if (socket.offMessage) socket.offMessage(this.listeners.message);
                if (socket.offError) socket.offError(this.listeners.error);
                if (socket.offClose) socket.offClose(this.listeners.close);
            }
            try { socket.close(); } catch (_) { /* Dispose remains idempotent after TCP failure. */ }
        }
        if (this.session) this.session.destroy();
        if (this.application) this.application.dispose();
        if (this.requests) this.requests.dispose();
        const closedError = reason || new Error('连接已取消');
        for (const callback of this.callbacks.values()) callback.reject(closedError);
        this.callbacks.clear();
        this.session = null;
        this.application = null;
        this.requests = null;
        this.negotiated = null;
        this.incoming = [];
        this.incomingBytes = 0;
        this.pendingFrame = null;
        if (this.options.onClose) {
            try { this.options.onClose(reason || null); } catch (error) { this.callbackError = error; }
        }
    }
}

async function loadWxDevice(runtime, wxApi) {
    if (!runtime.wxDevicePending) {
        runtime.wxDevicePending = Promise.resolve().then(async () => {
            const key = 'net.gateway.device.v2';
            if (typeof wxApi.getStorageSync !== 'function' || typeof wxApi.setStorageSync !== 'function') {
                throw new Error('微信设备存储不可用');
            }
            const existing = wxApi.getStorageSync(key);
            if (typeof existing === 'string' && /^[0-9a-f]{32}$/.test(existing) && /[1-9a-f]/.test(existing)) {
                return Uint8Array.from(existing.match(/../g), byte => parseInt(byte, 16));
            }
            const value = await runtime.randomSource(16);
            const bytes = value instanceof Uint8Array ? value : new Uint8Array(value);
            if (bytes.length !== 16 || !bytes.some(byte => byte !== 0)) throw new Error('设备强随机源不可用');
            wxApi.setStorageSync(key, Array.from(bytes, byte => byte.toString(16).padStart(2, '0')).join(''));
            return bytes;
        }).catch(error => { runtime.wxDevicePending = null; throw error; });
    }
    return runtime.wxDevicePending;
}

function createServiceClient(options) {
    if (options.protocol !== 3) return new NetWxClient(options);
    const createAttempt = current => {
        const createSocket = current.createSocket;
        return new NetWxClient(Object.assign({}, current, {
            createSocket: () => new NetWxGatewaySocket(current.runtime, {
                serviceId: current.serviceId, createSocket, clock: current.clock, onNodes: current.onNodes, windowBytes: current.windowBytes
            })
        }));
    };
    return options.autoReconnect === false ? createAttempt(options) : new NetWxServiceClient(options, createAttempt);
}

function createWxClient(runtime, options, wxApi) {
    if (!wxApi || typeof wxApi.createTCPSocket !== 'function' || typeof wxApi.getPerformance !== 'function') {
        throw new Error('当前微信版本不支持TCP网络模块');
    }
    const performance = wxApi.getPerformance();
    return createServiceClient(Object.assign({}, options, {
        runtime,
        platform: 3,
        loadDevice: () => loadWxDevice(runtime, wxApi),
        createSocket: () => {
            const now = performance.now();
            runtime.wxTcpAttempts = (runtime.wxTcpAttempts || []).filter(time => now - time < 300000);
            if (runtime.wxTcpAttempts.length >= 20) throw new Error('TCP重连过于频繁，请稍后重试');
            runtime.wxTcpAttempts.push(now);
            return wxApi.createTCPSocket();
        },
        loadNodes: () => {
            const value = wxApi.getStorageSync('net.gateway.nodes.v3');
            if (typeof value !== 'string' || value.length > 32864 || value.length % 2 || /[^0-9a-f]/.test(value)) return null;
            return Uint8Array.from(value.match(/../g) || [], byte => parseInt(byte, 16));
        },
        saveNodes: bytes => wxApi.setStorageSync('net.gateway.nodes.v3', Array.from(bytes,
            byte => byte.toString(16).padStart(2, '0')).join('')),
        clock: () => performance.now()
    }));
}

module.exports = { NetWxClient, loadWxRuntime, createWxClient, createServiceClient };