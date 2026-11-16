'use strict';

const { NetInterfaceDirectory } = require('./NetInterfaceDirectory');

// Host lifecycle only. Each attempt owns a fresh Wasm TLS/request context; accepted operations are never replayed.
class NetWxServiceClient {
    constructor(options, createAttempt) {
        this.options = options;
        this.runtime = options.runtime;
        this.createAttempt = createAttempt;
        this.current = null;
        this.disposed = false;
        this.recovering = true;
        this.timer = this.deadline = null;
        this.scheduling = false;
        this.promise = this.resolve = this.reject = null;
        this.delay = 250;
        this.cursor = 0;
        this.generation = 0;
        this.nodes = [];
        this.seeds = [{ address: options.address || options.hostname, port: options.port }].concat(options.seeds || []);
        if (this.seeds.length > 64 || this.seeds.some(point => !point || typeof point.address !== 'string' ||
            !point.address.length || point.address.length > 39 || !Number.isInteger(point.port) || point.port < 1024 || point.port > 65535)) {
            throw new Error('网关入口配置非法');
        }
        try {
            const bytes = typeof options.loadNodes === 'function' ? options.loadNodes() : this.runtime.gatewayNodeCache;
            if (bytes instanceof Uint8Array) {
                const directory = new NetInterfaceDirectory(this.runtime, bytes, true);
                try { this.nodes = directory.points(); } finally { directory.dispose(); }
            }
        } catch (_) { /* Invalid/expired public hints never replace seed trust. */ }
    }

    get ready() { return !!(this.current && this.current.ready && !this.current.disposed); }
    get negotiated() { return this.current && this.current.negotiated; }
    get session() { return this.current && this.current.session; }
    get deviceId() { return this.current && this.current.deviceId; }

    candidates() {
        const points = [];
        const unique = new Set();
        for (const point of this.seeds.concat(this.nodes)) {
            const key = point.address + ':' + point.port;
            if (!unique.has(key)) { unique.add(key); points.push({ address: point.address, port: point.port }); }
        }
        return points;
    }

    observe(bytes, points, cache) {
        this.nodes = points.map(point => Object.assign({}, point, { boot: point.boot.slice() }));
        this.runtime.gatewayNodeCache = cache.slice();
        try { if (typeof this.options.saveNodes === 'function') this.options.saveNodes(cache.slice()); }
        catch (error) { this.cacheError = error; }
        try { if (typeof this.options.onNodes === 'function') this.options.onNodes(bytes, points, cache); }
        catch (error) { this.callbackError = error; }
    }

    connect() {
        if (this.disposed) return Promise.reject(new Error('连接已关闭'));
        if (this.ready) return Promise.resolve(this);
        if (this.promise) return this.promise;
        this.promise = new Promise((resolve, reject) => { this.resolve = resolve; this.reject = reject; });
        const pending = this.promise;
        if (this.deadline === null) {
            this.deadline = setTimeout(() => this.dispose(new Error('网关恢复连接超时')), 30000);
        }
        this.start();
        return pending;
    }

    start() {
        if (this.disposed || this.current || this.timer !== null || this.scheduling) return;
        const points = this.candidates();
        const point = points[this.cursor++ % points.length];
        if (this.generation >= 0xffffffff) { this.dispose(new Error('网络连接代次已耗尽')); return; }
        ++this.generation;
        let client;
        try {
            client = this.createAttempt(Object.assign({}, this.options, point, {
                onNodes: (bytes, nodes, cache) => this.observe(bytes, nodes, cache),
                onFrame: frame => this.options.onFrame ? this.options.onFrame(frame, client) : true,
                onClose: error => this.closed(client, error)
            }));
            this.current = client;
            client.connect().then(() => {
                if (this.disposed || this.current !== client) return;
                this.delay = 250;
                try { if (typeof this.options.onReady === 'function') this.options.onReady(this); }
                catch (error) { this.dispose(error); return; }
                // A host callback may synchronously close/dispose this attempt.
                // Only the still-ready generation can settle connect or cancel
                // its deadline; closed() has already scheduled its replacement.
                if (this.disposed || this.current !== client || !client.ready || client.disposed) return;
                if (this.resolve) this.resolve(this);
                this.resolve = this.reject = this.promise = null;
                clearTimeout(this.deadline);
                this.deadline = null;
            }).catch(error => {
                // NetWxClient emits onClose before rejecting its own connect promise.
                if (this.current === client && !this.disposed) this.closed(client, error);
            });
        } catch (error) {
            if (this.current === client) this.current = null;
            this.closed(null, error);
        }
    }

    closed(client, error) {
        if (this.disposed || (client !== null && this.current !== client)) return;
        this.current = null;
        try { if (typeof this.options.onClose === 'function') this.options.onClose(error || new Error('网关连接已断开')); }
        catch (failure) { this.callbackError = failure; }
        if (this.disposed) return;
        if (error && (error.code === -1 || error.code === -11)) { this.dispose(error, false); return; }
        if (this.deadline === null) {
            this.deadline = setTimeout(() => this.dispose(new Error('网关恢复连接超时')), 30000);
        }
        this.schedule();
    }

    schedule() {
        if (this.disposed || this.timer !== null || this.scheduling) return;
        this.scheduling = true;
        Promise.resolve().then(() => this.runtime.randomSource(4)).then(value => {
            const bytes = value instanceof Uint8Array ? value : new Uint8Array(value);
            if (bytes.length !== 4) throw new Error('重连随机源不可用');
            const random = new DataView(bytes.buffer, bytes.byteOffset, 4).getUint32(0, true);
            this.scheduling = false;
            if (this.disposed) return;
            const pause = Math.floor(this.delay / 2) + random % this.delay;
            this.delay = Math.min(this.delay * 2, 5000);
            this.timer = setTimeout(() => { this.timer = null; this.start(); }, pause);
        }).catch(error => { this.scheduling = false; this.dispose(error); });
    }

    requireReady() {
        if (!this.ready || this.disposed) throw new Error('网关尚未恢复连接');
        return this.current;
    }
    request(product, type, bytes, timeout) { return this.requireReady().request(product, type, bytes, timeout); }
    requestWithBuilder(product, type, builder, timeout) { return this.requireReady().requestWithBuilder(product, type, builder, timeout); }
    sendFrame(fields, bytes) { return this.requireReady().sendFrame(fields, bytes); }
    connection() { return this.requireReady(); } // Pin files to this TLS generation.
    schedulePump() { if (this.current && !this.disposed) this.current.schedulePump(); }
    fail(error) { if (this.current && !this.disposed) this.current.fail(error); }

    dispose(reason, notify = true) {
        if (this.disposed) return;
        this.disposed = true;
        this.recovering = false;
        clearTimeout(this.timer);
        clearTimeout(this.deadline);
        this.timer = this.deadline = null;
        const client = this.current;
        this.current = null;
        if (client) client.dispose(reason);
        if (this.reject) this.reject(reason || new Error('连接已取消'));
        this.resolve = this.reject = this.promise = null;
        if (notify && typeof this.options.onClose === 'function') {
            try { this.options.onClose(reason || null); } catch (error) { this.callbackError = error; }
        }
    }
}

module.exports = { NetWxServiceClient };