'use strict';

// 1300 字节固定内容的文件元数据，摘要独立于待测协议实现。
function netPeerFileTestMetadata() {
    const metadata = new Uint8Array(160);
    const fields = new DataView(metadata.buffer);
    metadata.set([78, 70, 73, 50]);
    for (const [at, value] of [[4, 2], [8, 1], [12, 2], [16, 1024], [20, 2]]) fields.setUint32(at, value);
    fields.setBigUint64(24, 1300n);
    fields.setBigUint64(40, 61n);
    fields.setBigUint64(48, 3600n);
    metadata[56] = 1;
    metadata[72] = 2;
    const sha256 = 'b60f0e05937c77e7c3818dde3d41f69073ae3bf508374d1295df7a55bde99ee9';
    metadata.set(sha256.match(/../g).map(pair => parseInt(pair, 16)), 104);
    return metadata;
}

// 用真实 Wasm 文件流交换协议帧，内存文件后端逐次核对偏移、持久化边界和最终字节。
async function runNetPeerFileWriteTest(wasm, failure, rejection, expectedCode = -7) {
    const source = Uint8Array.from({length: 1300}, (_, index) => index % 251);
    const destination = new Uint8Array(source.length);
    const metadata = netPeerFileTestMetadata();
    const frames = [];
    const writes = [];
    let written = 0, durable = 0, acks = 0, commits = 0, inputCloses = 0, outputCloses = 0;
    let lateCancellations = 0, cancelFrames = 0;
    const client = toReceiver => ({
        runtime: {module: wasm}, options: {clock: () => Math.floor(performance.now())},
        ready: !toReceiver && failure === 'lateCancel',
        schedulePump() {},
        sendFrame(fields, bytes) {
            if (fields[0] === 6) ++cancelFrames;
            if (!toReceiver && fields[0] === 10) {
                const offset = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getBigUint64(16);
                if (offset > BigInt(durable)) throw new Error('ACK ahead of durable data');
                ++acks;
                if (failure === 'lateCancel' && commits === 1) {
                    // 此微任务在 sendFrame 返回且原生流消费最终 ACK 后、下一次定时 poll 前运行。
                    Promise.resolve().then(() => { ++lateCancellations; receiver.cancel(-83); });
                }
            }
            frames.push({toReceiver, frame: {kind: fields[0], product: fields[1], requestLow: fields[2],
                requestHigh: fields[3], type: fields[4], payload: bytes.slice()}});
        }
    });
    let rejectInputClose, readCalls = 0, codeReads = 0;
    const hostileError = Object.create(null, {code: {get() {
        ++codeReads;
        throw new Error('Host error code getter threw');
    }}});
    const input = {
        read(offset, size) {
            ++readCalls;
            if (failure === 'readCodeGetter') {
                // 错误处理失效时阻断重复读的微任务循环，让专项的期限检查仍能执行。
                if (readCalls > 1) return new Promise(() => {});
                return Promise.reject(hostileError);
            }
            if (failure === 'read') return Promise.reject(rejection);
            return source.slice(offset, offset + size);
        },
        close() {
            ++inputCloses;
            if (failure === 'inputCloseCodeGetter') return Promise.reject(hostileError);
            if (failure === 'pendingClose') return new Promise((resolve, reject) => { rejectInputClose = reject; });
            if (failure === 'inputClose') return Promise.reject(rejection);
        }
    };
    let maximumReads = 0;
    const output = {
        get maxWriteBytes() {
            if (++maximumReads !== 1) throw new Error('Write limit read twice');
            return 300;
        },
        read: () => new Uint8Array(0),
        async write(offset, bytes) {
            if (offset !== written || !bytes.length || bytes.length > 300) throw new Error('Invalid host write');
            await Promise.resolve();
            destination.set(bytes, offset);
            written += bytes.length;
            writes.push(bytes.length);
        },
        flush() { durable = written; },
        close() {
            ++outputCloses;
            if (failure === 'close') throw Object.assign(new Error('Close failed'), {code: -9});
        },
        commit() {
            if (outputCloses !== 1 || durable !== source.length) throw new Error('Commit before durable close');
            ++commits;
        }
    };
    const connection = client(true);
    if (failure === 'schedule') connection.schedulePump = () => {
        throw Object.assign(new Error('Host scheduling failed'), {code: -83});
    };
    const sender = new wasm.NetFileTransfer(connection, metadata, 7, true, input);
    let receiver;
    try {
        receiver = new wasm.NetFileTransfer(client(false), metadata, 7, false, output);
        if (maximumReads !== 1) throw new Error('File write limit was not captured');
        if (failure === 'cancelNotify' || failure === 'cancelNoHandler' || failure === 'cancelInvalid') {
            let sends = 0, failures = 0;
            connection.ready = failure !== 'cancelInvalid';
            connection.sendFrame = () => { ++sends; throw new Error('Host refused cancellation'); };
            if (failure === 'cancelNotify') connection.fail = () => {
                ++failures;
                if (failures === 1) sender.cancel(-9);
                throw new Error('Host failure callback threw');
            };
            sender.cancel(failure === 'cancelInvalid' ? -0.5 : -83);
            receiver.cancel();
            if (await sender.wait() !== (failure === 'cancelInvalid' ? -10 : -83) || await receiver.wait() !== -10 ||
                sends !== (failure === 'cancelInvalid' ? 0 : 1) || failures !== (failure === 'cancelNotify' ? 1 : 0) ||
                !sender.done() || inputCloses !== 1 || outputCloses !== 1) throw new Error('Cancellation cleanup mismatch');
            return;
        }
        const end = performance.now() + 5000;
        while (!sender.done() || !receiver.done()) {
            if (performance.now() >= end) throw new Error('File transfer deadline');
            while (frames.length) {
                const next = frames[0];
                if (!(next.toReceiver ? receiver : sender).accept(next.frame)) break;
                frames.shift();
            }
            sender.poll();
            receiver.poll();
            if (rejectInputClose) {
                let settled = false;
                sender.wait().then(() => { settled = true; });
                sender.destroy();
                sender.destroy();
                await Promise.resolve();
                const early = sender.done() || settled;
                rejectInputClose(Object.assign(new Error('Deferred close failed'), {code: -9}));
                rejectInputClose = null;
                if (early) throw new Error('File released before host close settled');
            }
            if ((failure === 'read' || failure === 'readCodeGetter' || failure === 'schedule') && sender.done()) receiver.cancel();
            if (failure === 'close' && receiver.done()) sender.cancel();
            await new Promise(resolve => setTimeout(resolve, 0));
        }
        const sent = await sender.wait(), received = await receiver.wait();
        if (inputCloses !== 1 || outputCloses !== 1) throw new Error('Unexpected file close count');
        if (failure === 'readCodeGetter') {
            if (sent !== -7 || received !== -10 || readCalls !== 1 || codeReads !== 1 || written || commits)
                throw new Error('Hostile read error did not close exactly once');
            return;
        }
        if (failure === 'inputCloseCodeGetter') {
            if (sent !== -7 || received !== 0 || readCalls !== 2 || codeReads !== 1 || commits !== 1 || durable !== source.length ||
                !destination.every((byte, index) => byte === source[index])) throw new Error('Hostile close error result mismatch');
            return;
        }
        if (failure === 'inputClose' || failure === 'pendingClose') {
            const expected = failure === 'pendingClose' ? -9 : expectedCode;
            if (sent !== expected || received !== 0 || readCalls !== 2 || commits !== 1 || durable !== source.length ||
                !destination.every((byte, index) => byte === source[index])) throw new Error('Input close result mismatch');
            return;
        }
        if (failure === 'schedule') {
            if (sent !== -83 || received !== -10 || written !== 0 || commits) throw new Error('Scheduling failure mismatch');
            return;
        }
        if (failure === 'lateCancel' && (lateCancellations !== 1 || cancelFrames !== 0))
            throw new Error('Late cancellation emitted a frame after the final ACK');
        if (failure && failure !== 'lateCancel') {
            if (commits || (failure === 'read' ? sent !== expectedCode || readCalls !== 1 || received !== -10 || written !== 0 :
                received !== -9 || sent !== -10 || durable !== source.length)) throw new Error('File failure result mismatch');
            return;
        }
        if (sent !== 0 || received !== 0 || writes.join(',') !== '300,300,300,124,276' ||
            frames.length || !acks || commits !== 1 || durable !== source.length ||
            !destination.every((byte, index) => byte === source[index])) throw new Error('File transfer result mismatch');
    } finally {
        sender.cancel();
        if (receiver) receiver.cancel();
        await Promise.all([sender.wait(), receiver && receiver.wait()]);
    }
}

// 用同一已登记监听器依次派发原始 END 和另一个流的帧，核对会话和文件流各自的处理结果。
function checkNetPeerLateEnd(wasm, runtime, flow, end) {
    const {offsets, pointerBytes} = wasm.netPeerABI;
    const create = wasm._netPeerSenderClientCreate, close = wasm._netPeerSenderClientClose;
    let context, memory = 0, closes = 0, late = 0, independent = 0;
    const client = runtime.createSender({
        onSessionFrame(generation, frame) {
            if (generation !== 9n) throw new Error('Wrong generation for late END');
            const accepted = flow.accept({kind: frame.kind, product: frame.product, type: frame.type,
                requestLow: Number(frame.requestId), requestHigh: 0, payload: frame.bytes});
            if (frame.requestId === 61n && accepted) ++late;
            else if (frame.requestId === 62n && !accepted && frame.bytes.length === 1 && frame.bytes[0] === 83) ++independent;
            else throw new Error('Unexpected late END routing');
            return true;
        }
    });
    try {
        wasm._netPeerSenderClientCreate = (table, out) => {
            const view = new DataView(wasm.HEAPU8.buffer), address = table + offsets.callbacks_context;
            context = pointerBytes === 8 ? view.getBigUint64(address, true) : view.getUint32(address, true);
            return create(table, out);
        };
        if (client.prepareGateway() !== 0 || !context) throw new Error('Cannot prepare late END listener');
        wasm._netPeerSenderClientClose = (handle, reason) => { ++closes; return close(handle, reason); };
        memory = wasm._malloc(offsets.frameSize + end.payload.length);
        if (!memory) throw new Error('Cannot allocate late END frame');
        const dispatch = (requestId, bytes) => {
            const view = new DataView(wasm.HEAPU8.buffer), payload = memory + offsets.frameSize;
            view.setUint32(memory + offsets.frame_m_productId, end.product, true);
            view.setBigUint64(memory + offsets.frame_m_requestId, requestId, true);
            view.setUint32(memory + offsets.frame_m_messageType, end.type, true);
            view.setUint16(memory + offsets.frame_m_kind, end.kind, true);
            view.setUint32(memory + offsets.frame_m_payloadSize, bytes.length, true);
            if (pointerBytes === 8) view.setBigUint64(memory + offsets.frame_m_payload, BigInt(payload), true);
            else view.setUint32(memory + offsets.frame_m_payload, payload, true);
            wasm.HEAPU8.set(bytes, payload);
            return wasm.__netPeerClientCallback(2, context, 9n, 0n, 0, 0, 0, memory, 0);
        };
        if (dispatch(61n, end.payload) !== 1 || closes || late !== 1 ||
            dispatch(62n, Uint8Array.of(83)) !== 1 || closes || independent !== 1)
            throw new Error('Late END interrupted another stream on the same session');
    } finally {
        wasm._netPeerSenderClientCreate = create;
        wasm._netPeerSenderClientClose = close;
        if (memory) wasm._free(memory);
        if (client.releaseGateway() !== 0) throw new Error('Cannot release late END listener');
    }
}

// 双端真实文件协议。仅在指定的最终输出边界模拟宿主背压、分配错误或断开。
async function runNetPeerFinalOutputTest(wasm, fault, runtime) {
    const failures = {sendError: -83, oomTimeout: -9, blockedTimeout: -9, oomCancel: -72};
    const data = Uint8Array.from({length: 1300}, (_, index) => index % 251);
    const saved = new Uint8Array(data.length);
    const pending = [];
    let now = 100, commits = 0, readsClosed = 0, writesClosed = 0, written = 0, finalAcks = 0;
    let lastData, duplicated = false, refused = 0, oldAcks = 0, peekFailures = 0;
    let endFrame, lateChecked = false;
    const connection = toReceiver => ({
        runtime: {module: wasm}, options: {clock: () => now}, schedulePump() {},
        sendFrame(fields, bytes) {
            const frame = {kind: fields[0], product: fields[1], requestLow: fields[2],
                requestHigh: fields[3], type: fields[4], payload: bytes.slice()};
            const flags = new DataView(bytes.buffer, bytes.byteOffset, bytes.length).getUint32(4);
            if (toReceiver && flags === 0) lastData = frame;
            if (fault === 'lateEnd' && toReceiver && flags === 1) endFrame = frame;
            if (fault === 'heldAck' && toReceiver && flags === 1) {
                pending.push({toReceiver, frame: lastData});
                duplicated = true;
            }
            if (!toReceiver && duplicated && flags === 0) {
                if (!commits) { ++refused; throw Object.assign(new Error('ACK blocked'), {code: -3}); }
                ++oldAcks;
            }
            if (!toReceiver && flags === 1) {
                if (fault === 'lateEnd' && !lateChecked) { ++refused; throw Object.assign(new Error('ACK blocked'), {code: -3}); }
                if (fault === 'sendError') throw Object.assign(new Error('Socket closed'), {code: -83});
                if (fault === 'blockedTimeout') { ++refused; throw Object.assign(new Error('Socket blocked'), {code: -3}); }
                ++finalAcks;
            }
            pending.push({toReceiver, frame});
        }
    });
    const metadata = netPeerFileTestMetadata();
    const sender = new wasm.NetFileTransfer(connection(true), metadata, 7, true, {
        read: (at, size) => data.slice(at, at + size), close() { ++readsClosed; }
    });
    const receiver = new wasm.NetFileTransfer(connection(false), metadata, 7, false, {
        progressTimeoutMs: 1000, read: () => new Uint8Array(0),
        write(at, bytes) {
            if (at !== written) throw new Error('Duplicate disk write');
            saved.set(bytes, at); written += bytes.length;
        },
        flush() {}, close() { ++writesClosed; }, commit() { ++commits; }
    });
    // Emscripten 首次调用会把惰性导出替换成真实入口，再保存和包装该入口。
    wasm._netPeerFileFlowPeek(0, 0, 0);
    const peek = wasm._netPeerFileFlowPeek;
    try {
        wasm._netPeerFileFlowPeek = (handle, fields, bytes) => {
            if (handle === receiver.handle && commits && (fault === 'oomTimeout' || fault === 'oomCancel' || (fault === 'oomOnce' && !peekFailures)) &&
                wasm._netPeerFileFlowProgress(handle, now, 0) === 2) { ++peekFailures; return -5; }
            return peek(handle, fields, bytes);
        };
        const end = performance.now() + 5000;
        while (!receiver.done() || !sender.done()) {
            if (performance.now() >= end) throw new Error('Final output deadline: ' + fault);
            while (pending.length) {
                const next = pending[0];
                if (!(next.toReceiver ? receiver : sender).accept(next.frame)) break;
                pending.shift();
            }
            sender.poll();
            receiver.poll();
            if (fault === 'lateEnd' && refused && !lateChecked) {
                if (!endFrame || commits !== 1 || receiver.done() || finalAcks) throw new Error('Missing committed ACK backlog');
                checkNetPeerLateEnd(wasm, runtime, receiver, endFrame);
                lateChecked = true;
            }
            if ((fault === 'oomTimeout' && peekFailures >= 2) || (fault === 'blockedTimeout' && refused >= 2)) now = 1100;
            if (fault === 'oomCancel' && peekFailures >= 2) receiver.cancel(-72);
            if (receiver.done() && fault in failures) sender.cancel();
            await new Promise(resolve => setTimeout(resolve, 0));
        }
        const peerResult = await sender.wait(), local = await receiver.wait();
        if (commits !== 1 || readsClosed !== 1 || writesClosed !== 1 || written !== data.length ||
            !saved.every((value, index) => value === data[index])) throw new Error('File cleanup or contents: ' + fault);
        if (fault in failures) {
            if (peerResult !== -10 || local !== failures[fault] || finalAcks ||
                ((fault === 'oomTimeout' || fault === 'oomCancel') && peekFailures < 2) ||
                (fault === 'blockedTimeout' && refused < 2)) throw new Error('Final ACK terminal failure: ' + fault);
        } else if (local !== 0 || peerResult !== 0 || finalAcks !== 1 ||
            (fault === 'oomOnce' && peekFailures !== 1) || (fault === 'heldAck' && (!refused || oldAcks !== 1)) ||
            (fault === 'lateEnd' && !lateChecked)) {
            throw new Error('Final ACK was not delivered: ' + [fault, local, peerResult, finalAcks, refused, oldAcks, peekFailures].join(','));
        }
    } finally {
        wasm._netPeerFileFlowPeek = peek;
        sender.cancel(); receiver.cancel();
        await Promise.all([sender.wait(), receiver.wait()]);
    }
}

// 用真实就绪及帧回调验证仅终止收到失败 ACK 的文件流，并保留真正的协议错误。
async function runNetPeerFileAckTest(wasm, runtime, reason, wrongToken) {
    let callbacks, flow, fileCloses = 0, sessionCloses = 0, independent = 0, ready = 0;
    const sender = runtime.createSender({
        onSessionReady() {
            ++ready;
            flow = new wasm.NetFileTransfer({runtime, options: {clock: () => 1}, schedulePump() {},
                sendFrame() { throw new Error('Unexpected outgoing frame'); }},
                netPeerFileTestMetadata(), 7, true, {read() { throw new Error('Unexpected read'); }, close() { ++fileCloses; }});
        },
        onSessionFrame(generation, frame) {
            if (generation !== 9n || frame.product !== 7) throw new Error('Wrong session');
            if (frame.requestId === 62n) {
                if (frame.kind !== 9 || frame.bytes[0] !== 78) throw new Error('Wrong independent frame');
                ++independent;
                return true;
            }
            return flow.accept({kind: frame.kind, type: frame.type, product: frame.product, requestLow: Number(frame.requestId),
                requestHigh: 0, payload: frame.bytes});
        }
    });
    const create = wasm._netPeerSenderClientCreate, close = wasm._netPeerSenderClientClose;
    try {
        wasm._netPeerSenderClientCreate = (table, handle) => { callbacks = table; return create(table, handle); };
        if (sender.prepareGateway() !== 0 || !callbacks) throw new Error('Cannot install file ACK callback');
        wasm._netPeerSenderClientClose = (handle, code) => { ++sessionCloses; return close(handle, code); };
        const table = wasm.netPeerABI.pointerBytes === 8 ? BigInt(callbacks) : Number(callbacks);
        if (wasm._netPeerTestFileAck(table, reason, wrongToken ? 1 : 0) !== 0 || ready !== 1)
            throw new Error('File ACK callback was not dispatched');
        if (wrongToken) {
            if (sessionCloses !== 1 || runtime.lastBindingError.code !== -2) throw new Error('Invalid ACK lost its protocol error');
            flow.poll();
        } else if (sessionCloses !== 0 || independent !== 1 || !flow.closed) {
            throw new Error('Peer file failure interrupted the shared session');
        }
        if (await flow.wait() !== (wrongToken ? -2 : reason) || !flow.done() || fileCloses !== 1)
            throw new Error('File ACK terminal result mismatch');
    } finally {
        wasm._netPeerSenderClientCreate = create;
        wasm._netPeerSenderClientClose = close;
        if (flow) { flow.cancel(); await flow.wait(); }
        if (sender.releaseGateway() !== 0) throw new Error('Cannot release file ACK listener');
    }
}

// 连接可能在取消通知前失效。用真实文件流分别从主动取消和异步读取失败进入关闭。
async function checkNetPeerCancelState(wasm, property, readFailure) {
    let reads = 0, closes = 0, stateReads = 0, sends = 0, failures = 0;
    const connection = {
        runtime: {module: wasm}, options: {clock: () => 1}, schedulePump() {},
        ready: true, disposed: false,
        sendFrame() { ++sends; }, fail() { ++failures; }
    };
    Object.defineProperty(connection, property, {get() {
        ++stateReads;
        throw new Error('Connection state unavailable: ' + property);
    }});
    const flow = new wasm.NetFileTransfer(connection, netPeerFileTestMetadata(), 7, true, {
        read() { ++reads; return Promise.reject(-83); }, close() { ++closes; }
    });
    try {
        if (readFailure) flow.poll();
        else flow.cancel(-72);
        const result = await flow.wait();
        flow.cancel(-9);
        if (result !== (readFailure ? -83 : -72) || await flow.wait() !== result || !flow.done() ||
            reads !== (readFailure ? 1 : 0) || stateReads !== 1 || closes !== 1 || sends || failures)
            throw new Error('Connection state changed the file terminal result: ' + property);
    } finally {
        flow.cancel();
        await flow.wait();
    }
}

// 宿主异常必须收敛为一次失败，且不得产生未处理的 Promise 拒绝。
async function runNetPeerHostileErrorTest(wasm, failure, run = () => runNetPeerFileWriteTest(wasm, failure)) {
    let unhandled = 0, timeout;
    const onNodeRejection = () => { ++unhandled; };
    const onBrowserRejection = event => { ++unhandled; event.preventDefault(); };
    const node = typeof process !== 'undefined' && typeof process.on === 'function';
    if (node) process.on('unhandledRejection', onNodeRejection);
    else if (typeof addEventListener === 'function') addEventListener('unhandledrejection', onBrowserRejection);
    try {
        await Promise.race([
            run(),
            new Promise((resolve, reject) => {
                timeout = setTimeout(() => reject(new Error('Hostile error cleanup deadline: ' + failure)), 6500);
            })
        ]);
        await new Promise(resolve => setTimeout(resolve, 0));
        if (unhandled) throw new Error('Unhandled rejection from host error: ' + failure);
    } finally {
        clearTimeout(timeout);
        if (node) process.removeListener('unhandledRejection', onNodeRejection);
        else if (typeof removeEventListener === 'function') removeEventListener('unhandledrejection', onBrowserRejection);
    }
}

// 经真正的宿主回调读取不同批次。检查重试不丢失余量，源失败时不交付残存数据。
async function runNetPeerEntropyTest(wasm, runtime, source) {
    const read = (size, prefix, first, second, expected) => {
        if (wasm._netPeerTestEntropy(size, prefix, first, second, expected) !== 0)
            throw new Error('Unexpected entropy contents or status at source call ' + source.calls);
    };
    read(1, 1, 9, 0, 0);
    await Promise.resolve();
    if (source.calls !== 1) throw new Error('Early entropy refill');
    read(8192, 0, 0, 0, -7);
    read(8192, 0, 0, 0, -7);
    if (!runtime.refilling) throw new Error('Unsatisfied request did not schedule entropy refill');
    await runtime.refilling;
    if (source.calls !== 2) throw new Error('Concurrent requests did not share a refill');
    await runtime.refillRandom();
    if (source.calls !== 2) throw new Error('Unneeded refill called random source');
    read(8192, 8191, 9, 10, 0);

    source.fail = true;
    read(8192, 0, 0, 0, -7);
    if (!runtime.refilling) throw new Error('Missing pending entropy refill');
    const rejected = await runtime.refilling.then(() => false, () => true);
    if (!rejected || source.calls !== 3) throw new Error('Expected random source failure');
    read(0, 0, 0, 0, 0);
    read(1, 0, 0, 0, -7);
    if (!runtime.refilling) throw new Error('Read after failure did not retry');
    await runtime.refilling;
    read(8192, 8191, 10, 12, 0);
    if (source.calls !== 4) throw new Error('Unexpected entropy retry count');
}

// 使用真实Sender配置网关，核对UTF-8边界以及过长参数进入模块堆之前即被拒绝。
function runNetPeerGatewayConfigTest(wasm, runtime) {
    const sender = runtime.createSender(null);
    if (sender.prepareGateway() !== 0) throw new Error('Cannot prepare gateway configuration');
    try {
        const seed = '127.0.0.1:9000';
        const path = '网'.repeat(1365) + 'a';
        if (runtime.configureGateways(sender, seed, path) !== 0) throw new Error('Valid 4096-byte path rejected');
        const allocate = wasm._malloc;
        let largest = 0;
        wasm._malloc = size => { largest = Math.max(largest, Number(size)); return allocate(size); };
        try {
            if (runtime.configureGateways(sender, seed, path + 'b') !== -1 ||
                runtime.configureGateways(sender, 'a'.repeat(4097), '') !== -1 || largest > 4097)
                throw new Error('Oversized gateway text reached module heap');
        } finally { wasm._malloc = allocate; }
        if (runtime.configureGateways(sender, '', '') !== 0) throw new Error('Cannot clear gateways');
    } finally {
        if (sender.releaseGateway() !== 0) throw new Error('Cannot release gateway configuration');
    }
}

// 调用真实C++回调入口，覆盖32/64位整数最高位；同时核对负的关闭原因和字节副本。
function runNetPeerCallbackIntegersTest(wasm, runtime) {
    const values32 = [1, 0x7fffffff, 0x80000000, 0xffffffff];
    const values64 = [1n, 0x7fffffffffffffffn, 0x8000000000000000n, 0xffffffffffffffffn];
    const observed = [];
    const sender = runtime.createSender({
        onSessionReady(...args) { observed.push(['ready', ...args]); },
        onSessionResult(...args) { observed.push(['result', ...args]); },
        onSessionFrame(...args) { observed.push(['frame', ...args]); return true; },
        onSessionClosed(...args) { observed.push(['closed', ...args]); }
    });
    const create = wasm._netPeerSenderClientCreate;
    let callbacks = 0;
    try {
        wasm._netPeerSenderClientCreate = (address, output) => {
            callbacks = address;
            return create(address, output);
        };
        if (sender.prepareGateway() !== 0 || !callbacks) throw new Error('Cannot prepare callback integers');
    } finally { wasm._netPeerSenderClientCreate = create; }
    try {
        for (let step = 0; step < 4; ++step) {
            observed.length = 0;
            const pointer = wasm.netPeerABI.pointerBytes === 8 ? BigInt(callbacks) : Number(callbacks);
            if (wasm._netPeerTestCallbackIntegers(pointer, step) !== 0 || observed.length !== 4)
                throw new Error('Missing native callbacks at ' + step);
            const [ready, result, frame, closed] = observed;
            const generation = values64[step], request = values64[3 - step];
            const product = values32[step], type = values32[3 - step];
            const bytesMatch = bytes => bytes instanceof Uint8Array && bytes.buffer !== wasm.HEAPU8.buffer && bytes.length === 3 &&
                bytes[0] === 0 && bytes[1] === 83 && bytes[2] === 255;
            if (ready[0] !== 'ready' || ready[1] !== generation || ready[2] !== product || ready[3] !== type ||
                result[0] !== 'result' || result[1] !== generation || result[2] !== request ||
                result[3] !== product || result[4] !== type || result[5] !== 0 || !bytesMatch(result[6]) ||
                frame[0] !== 'frame' || frame[1] !== generation || frame[2].product !== product ||
                frame[2].type !== type || frame[2].requestId !== request || frame[2].kind !== 9 || !bytesMatch(frame[2].bytes) ||
                closed[0] !== 'closed' || closed[1] !== generation || closed[2] !== -7 || closed[3] !== -83)
                throw new Error('Native callback integer or byte mismatch at ' + step);
        }
    } finally {
        if (sender.releaseGateway() !== 0) throw new Error('Cannot release callback integer client');
    }
}

// 配置和提交更新诊断；正常推进、状态查询及关闭保留最近一次诊断。
function runNetPeerSenderDiagnosticsTest(wasm, runtime) {
    const api = wasm.NetSender.instance();
    const client = api.createClient(runtime, null);
    const failures = [];
    const check = (condition, message) => { if (!condition) failures.push(message); };
    // 初始化前也须记录此次配置失败，不能返回上一项无效产品参数的诊断。
    check(client.connectServiceGateway(0, {}, null) === -1 && api.lastGatewayError(client) === -1,
        'Missing initial argument diagnosis');
    check(runtime.configureGateways(client, '', '') === -7 && api.lastGatewayError(client) === -7,
        'Unprepared gateway configuration did not record diagnosis');
    if (api.prepareGateway(client) !== 0) throw new Error('Cannot prepare diagnostic client');
    try {
        check(api.postRequest(client, 0n, null) === -1 && api.lastGatewayError(client) === -1, 'Invalid submission diagnosis');
        check(!api.gatewayReady(client) && api.gatewayGeneration(client) === 0n && api.pollGateway(client) === 0 &&
            !client.pollGateway() && api.lastGatewayError(client) === -1, 'Queries or normal poll changed diagnosis');
        check(runtime.configureGateways(client, '127.0.0.1:9000', '') === 0 && api.lastGatewayError(client) === 0,
            'Successful gateway configuration did not clear diagnosis');
        check(runtime.configureGateways(client, '127.0.0.1:0', '') === -1 && api.lastGatewayError(client) === -1,
            'Rejected gateway configuration did not record diagnosis');
        check(runtime.configureGateways(client, '', '') === 0 && api.lastGatewayError(client) === 0,
            'Corrected gateway configuration did not clear diagnosis');
        check(api.postRequest(client, 0n, null) === -1 && api.lastGatewayError(client) === -1, 'Missing pre-close diagnosis');
        api.closeGateway(client);
        check(api.lastGatewayError(client) === -1, 'Close erased submission diagnosis');
        api.uninitGateway(client);
        check(api.lastGatewayError(client) === -1, 'Uninit erased submission diagnosis');
        if (failures.length) throw new Error(failures.join('; '));
    } finally {
        if (api.destroyClient(client) !== 0) throw new Error('Cannot release diagnostic client');
    }
}

// 向同一客户端提交指定 JS 上下文创建的正文，并核对两个原生入口实际借用的字节。
async function checkNetPeerRequestBodies(wasm, api, client, realm) {
    const slice = new realm.Uint8Array([17, 83, 29]).subarray(1, 2);
    Object.defineProperty(slice, 'length', {get() { throw new Error('Use the typed-array view bounds'); }});
    const memory = new realm.WebAssembly.Memory({initial: 1, maximum: 2});
    const detached = memory.buffer, detachedView = new realm.Uint8Array(detached);
    memory.grow(1);
    const disguised = new realm.Int8Array(1);
    Object.defineProperty(disguised, Symbol.toStringTag, {value: 'Uint8Array'});
    const valid = [[slice, '83'], [new realm.Uint8Array([0, 255]).buffer, '0,255'],
        [new realm.ArrayBuffer(0), ''], [new realm.Uint8Array(0), '']];
    const invalid = [detached, detachedView, disguised, new realm.Uint8ClampedArray(1), new realm.Uint16Array(1),
        new realm.DataView(new realm.ArrayBuffer(1)), {[Symbol.toStringTag]: 'ArrayBuffer'}];
    if (typeof realm.SharedArrayBuffer === 'function') invalid.push(new realm.SharedArrayBuffer(1));

    // Emscripten 首次调用时把延迟绑定的 JS 导出替换为原生函数。先完成绑定，再观察后续每一项输入。
    const empty = new Uint8Array(0);
    if (api.postRequest(client, 1n, empty) !== -7 || await api.sendRequest(client, 1n, empty) !== -7)
        throw new Error('Unexpected native result on an idle client');
    const post = wasm._netPeerSenderClientPost, send = wasm._netPeerSenderClientSendBegin;
    const seen = [];
    const observe = entry => (handle, id, address, size, ...tail) => {
        seen.push(wasm.HEAPU8.slice(Number(address), Number(address) + size).join(','));
        return entry(handle, id, address, size, ...tail);
    };
    try {
        wasm._netPeerSenderClientPost = observe(post);
        wasm._netPeerSenderClientSendBegin = observe(send);
        for (const [body, expected] of valid) {
            seen.length = 0;
            const posted = api.postRequest(client, 1n, body);
            const sent = await api.sendRequest(client, 1n, body);
            if (posted !== -7 || sent !== -7 || seen.length !== 2 || seen[0] !== expected || seen[1] !== expected)
                throw new Error('Request body [' + expected + '] returned ' + posted + '/' + sent +
                    ', native observations ' + JSON.stringify(seen));
        }
        seen.length = 0;
        for (const body of invalid) {
            if (api.postRequest(client, 1n, body) !== -1 || api.lastGatewayError(client) !== -1 ||
                await api.sendRequest(client, 1n, body) !== -1 || api.lastGatewayError(client) !== -1 || seen.length)
                throw new Error('Invalid or detached body reached a native request entry');
        }
    } finally {
        wasm._netPeerSenderClientPost = post;
        wasm._netPeerSenderClientSendBegin = send;
    }
}

// 使用未连接的真实客户端核对 JS 门面；仅在输出回写故障处控制 C 返回的请求号。
async function runNetPeerSenderFacadeTest(wasm, runtime) {
    const api = wasm.NetSender.instance();
    const client = api.createClient(runtime, null);
    const bytes = new Uint8Array([0, 83]);
    const frame = {kind: 3, product: 7, type: 2, requestId: 1n, bytes};
    try {
        if (api.pollGateway(null) !== -1 || api.pollGateway(client) !== 0 || client.pollGateway() || api.prepareGateway(client) !== 0 ||
            api.prepareGateway(client) !== 0 || api.gatewayReady(client) || api.gatewayGeneration(client) !== 0n ||
            api.pollGateway(client) !== 0 || client.pollGateway()) throw new Error('Idle sender lifecycle');
        if (api.configureCompression(client, [0]) !== -1 || api.configureCompression(client, [9]) !== 0 ||
            api.connectGateway(client, null, null) !== -1 || api.connectServiceGateway(client, 0, {}, null) !== -1)
            throw new Error('Sender options');
        let reads = 0;
        const types = [1];
        Object.defineProperty(types, 0, {get() { ++reads; return 17; }});
        const unavailable = [1];
        Object.defineProperty(unavailable, 0, {get() { throw new Error('Option unavailable'); }});
        if (api.configureCompression(client, unavailable) !== -1 || api.lastGatewayError(client) !== -1 ||
            api.configureCompression(client, types) !== 0 || reads !== 1) throw new Error('Compression option snapshot');
        const brokenListener = {get onSessionReady() { throw new Error('Listener unavailable'); }};
        const brokenOptions = {get network() { throw new Error('Options unavailable'); }};
        const quiet = {onSessionReady() {}, onSessionResult() {}, onSessionFrame() {}, onSessionClosed() {}};
        if (api.connectGateway(client, {}, brokenListener) !== -1 ||
            api.connectServiceGateway(client, 7, brokenOptions, quiet) !== -1) throw new Error('Throwing connection inputs');
        const read = api.beginRequest(client, 7, 2, 1000);
        const output = {requestId: 19n};
        if (read.result !== -7 || read.requestId !== 0n || api.beginRequest(client, 0, 2, 1000, output) !== -1 ||
            output.requestId !== 0n || api.beginRequest(client, 7, 2, 1000, false) !== -1 ||
            api.beginRequest(client, 7, 2, 1000, Object.freeze({})) !== -1) throw new Error('Request result output');
        if (api.postRequest(client, -1n, bytes) !== -1 || api.cancelRequest(client, 1) !== -1 ||
            api.postData(client, null) !== -1 || api.postData(client, frame) >= 0 ||
            await api.sendRequest(client, -1n, bytes) !== -1 || await api.sendData(client, frame) >= 0 ||
            api.lastGatewayError(client) >= 0) throw new Error('Inactive sender submissions');
        await checkNetPeerRequestBodies(wasm, api, client, globalThis);
        if (typeof process !== 'undefined' && process.versions && process.versions.node) {
            await checkNetPeerRequestBodies(wasm, api, client, require('node:vm').runInNewContext('globalThis'));
        } else if (typeof document !== 'undefined' && document.body) {
            const iframe = document.createElement('iframe');
            document.body.appendChild(iframe);
            try { await checkNetPeerRequestBodies(wasm, api, client, iframe.contentWindow); }
            finally { iframe.remove(); }
        }
        const throwingFrame = {get kind() { throw new Error('Frame field unavailable'); }};
        if (api.postData(client, throwingFrame) !== -1 || await api.sendData(client, throwingFrame) !== -1 ||
            api.lastGatewayError(client) !== -1) throw new Error('Throwing frame fields');
        let kindReads = 0;
        const guardedFrame = Object.assign({}, frame);
        Object.defineProperty(guardedFrame, 'kind', {get() {
            ++kindReads;
            if (api.destroyClient(client) !== -7) throw new Error('Released client during frame access');
            return frame.kind;
        }});
        if (api.postData(client, guardedFrame) !== api.postData(client, frame) ||
            await api.sendData(client, guardedFrame) !== await api.sendData(client, frame) || kindReads !== 2 ||
            api.configureCompression(client, [9]) !== 0) throw new Error('Snapshot or lifetime of frame fields');
        const begin = wasm._netPeerSenderClientBegin, cancel = wasm._netPeerSenderClientCancel;
        let reservations = 0, cancellations = 0;
        try {
            wasm._netPeerSenderClientBegin = (handle, product, type, timeout, address) => {
                ++reservations;
                new DataView(wasm.HEAPU8.buffer).setBigUint64(Number(address), 0x100000053n, true);
                return 0;
            };
            wasm._netPeerSenderClientCancel = (handle, request) => {
                if (request !== 0x100000053n) throw new Error('Wrong reservation cancelled');
                ++cancellations;
                return 0;
            };
            let released = 0;
            const refused = {set requestId(id) { released = api.destroyClient(client); throw new Error('Output refused'); }};
            if (api.beginRequest(client, 7, 2, 1000, refused) !== -1 || released !== -7 || reservations !== 1 || cancellations !== 1 ||
                api.beginRequest(client, 7, 2, 1000, output) !== 0 || output.requestId !== 0x100000053n ||
                reservations !== 2 || cancellations !== 1) throw new Error('Request output rollback');
        } finally { wasm._netPeerSenderClientBegin = begin; wasm._netPeerSenderClientCancel = cancel; }
        api.closeGateway(client, -10);
        api.uninitGateway(client);
    } finally {
        if (api.destroyClient(client) !== 0 || api.destroyClient(client) !== 0 || api.destroyClient(null) !== 0)
            throw new Error('Sender release');
    }
}

// 在真实客户端上借用 C 回调表，仅拦截拨号入口以检查 ABI 参数并避免外部网络依赖。
async function runNetPeerConnectionInputTest(wasm, runtime) {
    const sender = runtime.createSender(null);
    const create = wasm._netPeerSenderClientCreate, connect = wasm._netPeerSenderClientConnect;
    const malloc = wasm._malloc, free = wasm._free;
    const connectService = wasm._netPeerSenderClientConnectService;
    let callbacks, attempts = 0, events = 0;
    try {
        wasm._netPeerSenderClientCreate = (table, output) => { callbacks = table; return create(table, output); };
        if (sender.prepareGateway() !== 0) throw new Error('Cannot create connection input client');
        const options = {address: '127.0.0.1', port: 8123, platform: 1, capabilities: 7, maxRequests: 17,
            maxChunk: 2048, timeoutMs: 1000, device: new Uint8Array(16).fill(4)};
        const tls = {hostname: 'localhost', timeoutMs: 1500, authority: Uint8Array.of(1),
            certificate: Uint8Array.of(2), privateKey: Uint8Array.of(3)};
        const once = (values, duringRead = () => {}) => {
            const copy = {};
            for (const [name, value] of Object.entries(values)) {
                let reads = 0;
                Object.defineProperty(copy, name, {get() {
                    if (++reads !== 1) throw new Error('Field read more than once: ' + name);
                    duringRead();
                    return value;
                }});
            }
            return copy;
        };
        const keep = () => { if (sender.releaseGateway() !== -7) throw new Error('Released during input access'); };
        const event = function() { if (this !== listener) throw new Error('Wrong listener receiver'); ++events; };
        const listener = once({onSessionReady: event, onSessionResult: event, onSessionFrame: event, onSessionClosed: event}, keep);
        wasm._netPeerSenderClientConnect = (handle, address) => {
            ++attempts;
            const at = Number(address), offsets = wasm.netPeerABI.offsets;
            const view = new DataView(wasm.HEAPU8.buffer);
            const number = field => view.getUint32(at + offsets['options_' + field], true);
            if (number('port') !== 8123 || number('platform') !== 1 || number('maxRequests') !== 17 ||
                number('maxChunk') !== 2048 || number('handshakeTimeoutMs') !== 1000 ||
                view.getUint32(at + offsets.options_tls + offsets.tls_handshakeTimeoutMs, true) !== 1500)
                throw new Error('Incorrect encoded connection options');
            return 0;
        };
        const supplied = once({...options, tls: once(tls, keep)}, keep);
        if (sender.connectGateway(once({network: supplied}, keep), listener) !== 0 || attempts !== 1)
            throw new Error('Connection option snapshot or lifetime');
        const table = wasm.netPeerABI.pointerBytes === 8 ? BigInt(callbacks) : Number(callbacks);
        if (wasm._netPeerTestCallbackIntegers(table, 0) !== 0 || events !== 4)
            throw new Error('Listener snapshot did not survive connection setup');
        const quiet = {onSessionReady() {}, onSessionResult() {}, onSessionFrame() {}, onSessionClosed() {}};
        const config = {...options, tls};
        for (let failAt = 1; failAt <= 6; ++failAt) {
            let calls = 0;
            const active = new Map();
            wasm._malloc = size => {
                if (++calls === failAt) return 0;
                const address = malloc(size);
                active.set(address, Number(size));
                return address;
            };
            wasm._free = address => {
                const size = active.get(address);
                if (!size || wasm.HEAPU8.subarray(Number(address), Number(address) + size).some(byte => byte !== 0))
                    throw new Error('Duplicate release or unsanitized connection allocation');
                active.delete(address);
                free(address);
            };
            if (sender.connectGateway(config, quiet) !== -5 || active.size || attempts !== 1 || calls !== failAt)
                throw new Error('Partial allocation cleanup at ' + failAt);
        }
        wasm._free = free;
        wasm._malloc = () => { throw new Error('Allocator failed'); };
        const bytes = Uint8Array.of(13);
        const frame = {kind: 3, product: 7, type: 2, requestId: 1n, bytes};
        if (sender.postRequest(1n, bytes) !== -5 || await sender.sendRequest(1n, bytes) !== -5 ||
            sender.postData(frame) !== -5 || await sender.sendData(frame) !== -5 ||
            sender.configureCompression([7]) !== -5 || runtime.configureGateways(sender, '', '') !== -5 ||
            sender.postRequest(-1n, bytes) !== -1 || runtime.configureGateways(sender, '', null) !== -1 ||
            sender.connectGateway({...options, port: 0}, quiet) !== -1)
            throw new Error('Allocation failure versus invalid inputs');

        const maxName = [63, 63, 63, 61].map(size => 'a'.repeat(size)).join('.');
        const maxAddress = 'é'.repeat(2048);
        const ipv6 = Array(8).fill('abcd').join(':');
        const maxAuthority = new Uint8Array(131072);
        const detached = new WebAssembly.Memory({initial: 1, maximum: 2});
        const authorityView = new Uint8Array(detached.buffer, 0, 16);
        const lateDetach = {...tls, authority: authorityView, get certificate() {
            detached.grow(1);
            return tls.certificate;
        }};
        let allocations = 0;
        wasm._malloc = () => { ++allocations; return 0; };
        for (const invalid of [
            {...config, address: ''}, {...config, address: maxAddress + 'x'}, {...config, tls: lateDetach},
            ...['', maxName + '.', 'é'.repeat(127), 'é', 'a'.repeat(64), '-a', 'a..'].map(hostname =>
                ({...config, tls: {...tls, hostname}})),
            ...[undefined, new Uint8Array(0), new Uint8Array(131073)].map(authority =>
                ({...config, tls: {...tls, authority}}))
        ]) {
            if (sender.connectGateway(invalid, quiet) !== -1 || sender.lastGatewayError() !== -1)
                throw new Error('Invalid direct input reached the allocator or native connection');
        }
        let serviceCalls = 0;
        wasm._netPeerSenderClientConnectService = () => { ++serviceCalls; return 0; };
        if (sender.connectServiceGateway(7, {...options, address: ipv6 + '0'}, quiet) !== -1 ||
            attempts !== 1 || serviceCalls || allocations)
            throw new Error('Connection limit must be checked before any Wasm allocation');
        wasm._malloc = malloc;
        if (sender.connectGateway({...config, address: maxAddress, tls: {...tls, hostname: maxName, authority: maxAuthority}}, quiet) !== 0 ||
            sender.connectGateway({...config, tls: {...tls, hostname: maxName.slice(0, -1) + '.'}}, quiet) !== 0 ||
            attempts !== 3 || sender.connectServiceGateway(7, {...options, address: ipv6}, quiet) !== 0 ||
            sender.connectServiceGateway(7, {...options, tls: {...tls, hostname: maxName + '.', authority: new Uint8Array(131073)}}, quiet) !== 0 ||
            serviceCalls !== 2)
            throw new Error('Direct or service connection rejected an admissible boundary');
    } finally {
        wasm._netPeerSenderClientConnectService = connectService;
        wasm._netPeerSenderClientCreate = create;
        wasm._netPeerSenderClientConnect = connect;
        wasm._malloc = malloc;
        wasm._free = free;
        if (sender.releaseGateway() !== 0) throw new Error('Cannot release connection input client');
    }
}

// 经真实原生入口和可控字节宿主核对 JS 别名、显式非法值与成功拨号。
function runNetPeerConnectionAliasTest(wasm, runtime) {
    // Emscripten 的首次导出调用可能替换自身。先以空句柄核对原生返回码，再观察已绑定的入口。
    if (wasm._netPeerSenderClientConnect(0, 0) !== -1 || wasm._netPeerSenderClientConnectService(0, 7, 0, 0) !== -1)
        throw new Error('Expected native dial to reject null client');
    const direct = wasm._netPeerSenderClientConnect, service = wasm._netPeerSenderClientConnectService;
    const createSocket = runtime.options.createSocket;
    const network = {address: '127.0.0.1', port: 8123, platform: 1, capabilities: 3, maxRequests: 8,
        maxChunk: 16384, timeoutMs: 5000, device: new Uint8Array(16).fill(4),
        tls: {hostname: 'localhost', authority: Uint8Array.of(1)}};
    const listener = {onSessionReady() {}, onSessionResult() {}, onSessionFrame() {}, onSessionClosed() {}};
    let directCalls = 0, serviceCalls = 0, dials = 0;
    try {
        wasm._netPeerSenderClientConnect = (...args) => { ++directCalls; return direct(...args); };
        wasm._netPeerSenderClientConnectService = (...args) => { ++serviceCalls; return service(...args); };
        runtime.options.createSocket = events => ({
            connect(address, port) {
                if (address !== network.address || port !== network.port) throw new Error('Wrong service endpoint');
                ++dials;
                return 0;
            },
            write() { return 0; },
            close() { events.onClose(0); }
        });
        // 每次成功接入都建立独立客户端，先验证拒绝输入不会在原生侧创建连接。
        for (const legacy of [false, true]) {
            const sender = runtime.createSender(listener);
            try {
                if (sender.prepareGateway() !== 0) throw new Error('Cannot prepare alias test client');
                const before = dials, beforeService = serviceCalls;
                const rejected = options => {
                    if (sender.connectGateway(options) !== -1 || sender.lastGatewayError() !== -1 ||
                        sender.connectServiceGateway(7, options) !== -1 || sender.lastGatewayError() !== -1 ||
                        directCalls !== 0 || serviceCalls !== beforeService || dials !== before || sender.gatewayGeneration() !== 0n)
                        throw new Error('Invalid connection option reached native dial');
                };
                const pidKey = legacy ? 'm_localGatewayPid' : 'localGatewayPid';
                const networkKey = legacy ? 'm_network' : 'network';
                for (const value of [NaN, false, '', null, 1]) rejected({...network, [pidKey]: value});
                for (const value of [false, '', null, 0]) rejected({...network, [networkKey]: value});
                let pidReads = 0, legacyPidReads = 0, networkReads = 0, legacyNetworkReads = 0;
                const options = {
                    get localGatewayPid() { ++pidReads; return legacy ? undefined : 0; },
                    get m_localGatewayPid() { ++legacyPidReads; return 0; },
                    get network() { ++networkReads; return legacy ? undefined : network; },
                    get m_network() { ++legacyNetworkReads; return network; }
                };
                const result = sender.connectServiceGateway(7, options), generation = sender.gatewayGeneration();
                if (result !== 0 || generation !== 1n ||
                    serviceCalls !== beforeService + 1 || pidReads !== 1 || networkReads !== 1 ||
                    legacyPidReads !== Number(legacy) || legacyNetworkReads !== Number(legacy))
                    throw new Error('Service connection with legacy=' + legacy + ' returned ' + result + ', generation=' + generation +
                        ', native calls=' + (serviceCalls - beforeService) + ', field reads=' +
                        [pidReads, legacyPidReads, networkReads, legacyNetworkReads].join(','));
                for (let i = 0; i < 8 && dials === before; ++i) sender.pollGateway();
                if (dials === before) throw new Error('Accepted service options did not reach byte host');
            } finally {
                sender.closeGateway();
                for (let i = 0; i < 32 && sender.pollGateway(); ++i) {}
                if (sender.releaseGateway() !== 0) throw new Error('Cannot release alias test client');
            }
        }
    } finally {
        runtime.options.createSocket = createSocket;
        wasm._netPeerSenderClientConnect = direct;
        wasm._netPeerSenderClientConnectService = service;
    }
}

// 核对微信适配器转交的地址、端口和字节切片，并在注销后忽略先前保存的回调。
async function runNetPeerWxOptionsTest(wasm) {
    const bound = new Map(), observed = [];
    let randomRequest;
    const socket = {
        connect(options) { observed.push(options.address, options.port); },
        write(buffer) { observed.push(...new Uint8Array(buffer)); },
        close() { bound.get('Close')(); }
    };
    for (const name of ['Connect', 'Message', 'Error', 'Close']) {
        socket['on' + name] = cb => bound.set(name, cb);
        socket['off' + name] = cb => { if (cb === bound.get(name)) bound.delete(name); };
    }
    const options = wasm.NetPeerRuntime.wxOptions({
        createTCPSocket: () => socket,
        getRandomValues: request => { randomRequest = request; }
    });
    const readRandom = async result => {
        const pending = options.randomSource(4).then(bytes => ({bytes}), error => ({error}));
        await Promise.resolve();
        if (!randomRequest || randomRequest.length !== 4) throw new Error('Incorrect wx random request');
        const request = randomRequest;
        randomRequest = null;
        request.success(result);
        let timer;
        try {
            return await Promise.race([pending, new Promise((resolve, reject) => {
                timer = setTimeout(() => reject(new Error('Unsettled wx random callback')), 1000);
            })]);
        } finally { clearTimeout(timer); }
    };
    const events = [];
    const channel = options.createSocket({onConnect: () => events.push('open'), onData: data => events.push(...data),
        onError: error => events.push(error), onClose: code => events.push(code)});
    const old = [...bound.values()];
    channel.connect('127.0.0.1', 42);
    const part = Uint8Array.of(255, 2, 3, 4, 5, 254).subarray(1, 5);
    Object.defineProperties(part, {length: {value: 9}, byteOffset: {value: 0}, buffer: {value: new ArrayBuffer(9)}});
    channel.write(part);
    bound.get('Connect')(); bound.get('Message')({message: new Uint8Array([4, 0]).buffer});
    const invalidBytes = [4, [1, 2, 3, 4], {length: 4}, new Uint16Array(4), new DataView(new ArrayBuffer(4))];
    if (typeof structuredClone === 'function') {
        const detached = new ArrayBuffer(4), view = new Uint8Array(detached);
        structuredClone(detached, {transfer: [detached]});
        invalidBytes.push(detached, view);
    }
    for (const value of invalidBytes) {
        if (!('error' in await readRandom({randomValues: value}))) throw new Error('wx accepted non-binary random data');
        bound.get('Message')({message: value});
    }
    const unavailable = new Error('wx byte field unavailable');
    if ((await readRandom({get randomValues() { throw unavailable; }})).error !== unavailable ||
        !('error' in await readRandom({randomValues: Uint8Array.of(7)}))) throw new Error('wx random failure did not settle');
    bound.get('Message')({get message() { throw unavailable; }});
    if (events.join(',') !== ['open', 4, 0, ...Array(invalidBytes.length + 1).fill(-7)].join(','))
        throw new Error('Invalid wx TCP payload reached the data callback or failed to report an error');
    events.length = 0;
    bound.get('Message')({message: part});
    bound.get('Error')(-9); channel.close(); channel.dispose();
    for (const cb of old) cb();
    const array = await readRandom({randomValues: Uint8Array.of(4, 3, 2, 1).buffer});
    const slice = await readRandom({randomValues: part});
    if (observed.join(',') !== '127.0.0.1,42,2,3,4,5' || events.join(',') !== '2,3,4,5,-9,0' || bound.size ||
        !array.bytes || array.bytes.join(',') !== '4,3,2,1' || !slice.bytes || slice.bytes.join(',') !== '2,3,4,5')
        throw new Error('wx byte adapter recovery or view boundaries');
    const failedRegistration = new Error('Failed during registration');
    let closes = 0, detached = [];
    socket.close = () => { ++closes; };
    socket.onError = cb => { bound.set('Error', cb); throw failedRegistration; };
    for (const name of ['Connect', 'Message', 'Error', 'Close']) {
        socket['off' + name] = () => { detached.push(name); bound.delete(name); if (name === 'Message') throw new Error('Detach failed'); };
    }
    let failure;
    try { options.createSocket({}); } catch (error) { failure = error; }
    if (failure !== failedRegistration || closes !== 1 || bound.size || detached.join(',') !== 'Error,Message,Connect')
        throw new Error('Partial wx registration leaked a socket or callback');
    socket.onError = cb => bound.set('Error', cb);
    detached = [];
    const partial = options.createSocket({});
    try { partial.dispose(); } catch (_) {}
    partial.dispose();
    if (bound.size || detached.join(',') !== 'Close,Error,Message,Connect') throw new Error('Incomplete wx disposal');
}

// 两个原始通道共享一个输出池。由宿主显式决定何时拒收、变可写、归还和断开。
async function runNetPeerSocketTest(wasm) {
    const peers = [];
    let unblock, rejectLateWrite;
    const source = {calls: 0, fail: false};
    const wxRandom = wasm.NetPeerRuntime.wxOptions({
        createTCPSocket() {},
        getRandomValues(request) {
            ++source.calls;
            // 平台可能异步返回长度数值。通过真实补充流程检查清零、拒绝和下一次恢复。
            const randomValues = source.fail ? request.length : new Uint8Array(request.length).fill(8 + source.calls);
            source.fail = false;
            Promise.resolve().then(() => request.success({randomValues}));
        }
    });
    const runtime = await wasm.NetPeerRuntime.create({
        hostBufferPools: {inputBytes: 32768, outputBytes: 32768},
        hostDrainTimeoutMs: 20,
        randomRefillBytes: 1,
        randomSource: wxRandom.randomSource,
        createSocket(events) {
            const peer = {events, calls: 0, closes: 0, disposed: 0};
            peers.push(peer);
            return {
                connect() { events.onConnect(); return 0; },
                write(bytes) {
                    ++peer.calls;
                    if (peer.rejectWrite) {
                        events.onError(-91);
                        if (peer.rejectWrite === 'return') return -1;
                        throw new Error('Synchronous socket write failure');
                    }
                    if (peer === peers[1]) return new Promise((resolve, reject) => { rejectLateWrite = reject; });
                    if (peer !== peers[0]) return 0;
                    if (peer.calls === 1) return false;
                    if (bytes.length !== 32768) throw new Error('Unexpected in-flight size');
                    return new Promise(resolve => { unblock = resolve; });
                },
                pauseRead() { if (peer.ended) throw new Error('Cannot change reads on a closed socket'); },
                close() { ++peer.closes; events.onClose(0); },
                dispose() { ++peer.disposed; }
            };
        }
    });
    runNetPeerGatewayConfigTest(wasm, runtime);
    runNetPeerSenderDiagnosticsTest(wasm, runtime);
    runNetPeerCallbackIntegersTest(wasm, runtime);
    for (const reason of [-83, -3, -2, -8]) await runNetPeerFileAckTest(wasm, runtime, reason, false);
    await runNetPeerFileAckTest(wasm, runtime, -2, true);
    await runNetPeerFinalOutputTest(wasm, 'lateEnd', runtime);
    await runNetPeerSenderFacadeTest(wasm, runtime);
    await runNetPeerConnectionInputTest(wasm, runtime);
    await runNetPeerWxOptionsTest(wasm);
    await runNetPeerEntropyTest(wasm, runtime, source);
    runNetPeerConnectionAliasTest(wasm, runtime);
    const step = n => { if (wasm._netPeerTestRawHost(n) !== 0) throw new Error('Byte host stage ' + n); };
    try {
        // 小堆也能使用接近编号上限的宿主实例。两个编号随后经真实 C 宿主表收发并释放。
        const firstHost = 0x7ffffff0;
        runtime.nextHost = firstHost - 1;
        step(0);
        if (!runtime.hosts.has(firstHost) || !runtime.hosts.has(firstHost + 1)) throw new Error('Opaque host ID round trip');
        // 读取仍暂停时先收到两端数据。显式可写及另一端归还共享缓冲都应独立通知。
        peers[0].events.onData(Uint8Array.of(19));
        peers[1].events.onData(Uint8Array.of(83));
        peers[0].events.onWritable();
        peers[0].events.onWritable();
        step(1);
        if (peers[1].calls !== 0 || typeof unblock !== 'function') throw new Error('Output pool admission');
        unblock();
        await Promise.resolve();
        step(2);
        peers[1].events.onClose(-83);
        rejectLateWrite(new Error('Late write failure'));
        await Promise.resolve();
        step(3);
        if (runtime.hosts.size !== 0) throw new Error('Opaque host ID release');
        if (peers[0].closes !== 1 || peers[1].closes !== 0 || peers.some(peer => peer.disposed !== 1))
            throw new Error('Unexpected socket close/dispose count');
        // 再建立一条通道，分别用宿主错误和后续关闭通知收敛，首个具体诊断须传回监听者。
        step(4);
        const pendingWrite = peer => {
            const host = runtime.hosts.get(runtime.nextHost);
            const result = wasm.__netPeerHostCallback(3, host.id, host.scratch, 1, 0, 0, 0, 0, 1n);
            if (result !== 1 || peer.calls) throw new Error('Pending terminal accepted a write or returned ' + result);
        };
        peers[2].events.onError(-91);
        pendingWrite(peers[2]);
        peers[2].events.onData(new Uint8Array(32769));
        peers[2].events.onConnect();
        peers[2].events.onClose(0);
        step(5);
        if (peers[2].closes || peers[2].disposed !== 1) throw new Error('Duplicate close after host fault and EOF');
        for (const mode of ['throw', 'return']) {
            step(4);
            const peer = peers[peers.length - 1];
            peer.rejectWrite = mode;
            step(9);
            step(5);
            if (peer.calls !== 1 || peer.closes !== 1 || peer.disposed !== 1) throw new Error('Synchronous write fault cleanup');
        }
        const budget = runtime.hostEventBudget;
        runtime.hostEventBudget = 1;
        try {
            step(4);
            const peer = peers[peers.length - 1];
            peer.events.onData(Uint8Array.of(19));
            peer.events.onData(Uint8Array.of(83));
            peer.events.onClose(-83);
            peer.ended = true;
            pendingWrite(peer);
            step(10); step(11); step(12);
            if (peer.calls || peer.closes || peer.disposed !== 1) throw new Error('Unexpected output or duplicate close during EOF drain');
        } finally { runtime.hostEventBudget = budget; }
        for (const recover of [true, false]) {
            step(4);
            const peer = peers[peers.length - 1];
            peer.events.onData(Uint8Array.of(19, 83));
            peer.events.onClose(0);
            peer.events.onError(-91);
            peer.events.onData(new Uint8Array(32769));
            step(6);
            if (!recover) await new Promise(resolve => setTimeout(resolve, runtime.hostDrainTimeoutMs + 5));
            step(recover ? 7 : 8);
            if (peer.closes || peer.disposed !== 1) throw new Error('Unexpected socket cleanup after paused EOF');
        }
    } finally { step(-1); }
}

// 外部模块用独立身份和不可访问的堆表示；任何读取都说明归属检查过晚。
function checkFileModuleOwner(wasm, otherModule) {
    let heapReads = 0, ioCalls = 0, lookups = 0, allocations = 0;
    const foreign = otherModule || {get HEAPU8() { ++heapReads; throw new Error('Unexpected foreign heap access'); }};
    const connection = {runtime: {module: foreign}};
    const io = {read() { ++ioCalls; }, close() { ++ioCalls; }};
    const allocate = wasm._malloc;
    try {
        wasm._malloc = () => { ++allocations; throw new Error('Unexpected allocation for a foreign adapter'); };
        for (const client of [connection, {connection() { ++lookups; return connection; }}]) {
            let rejected = false;
            try { new wasm.NetFileTransfer(client, new Uint8Array(160), 7, true, io); }
            catch (error) { rejected = error instanceof TypeError; }
            if (!rejected || ioCalls || heapReads || allocations) throw new Error('Expected rejection before allocation or I/O');
        }
    } finally {
        wasm._malloc = allocate;
    }
    if (lookups !== 1) throw new Error('Unexpected number of connection lookups');
}

async function runNetPeerHostTest(wasm, otherModule) {
    checkFileModuleOwner(wasm);
    if (otherModule) {
        if (wasm.HEAPU8.buffer === otherModule.HEAPU8.buffer) throw new Error('Expected independent module memories');
        checkFileModuleOwner(wasm, otherModule);
    }
    await runNetPeerSocketTest(wasm);
    await runNetPeerFileWriteTest(wasm);
    await runNetPeerFileWriteTest(wasm, 'lateCancel');
    for (const fault of ['heldAck', 'oomOnce', 'oomTimeout', 'oomCancel', 'blockedTimeout', 'sendError']) {
        await runNetPeerFinalOutputTest(wasm, fault);
    }
    for (const [reason, expected] of [[undefined, -7], [-83, -83], [-1, -1], [-0.5, -7], [0, -7], [-2147483649, -7]]) {
        await runNetPeerFileWriteTest(wasm, 'read', reason, expected);
    }
    await runNetPeerFileWriteTest(wasm, 'close');
    await runNetPeerFileWriteTest(wasm, 'inputClose', Object.assign(new Error('Input close failed'), {code: -9}), -9);
    await runNetPeerFileWriteTest(wasm, 'inputClose', -2147483648, -2147483648);
    await runNetPeerFileWriteTest(wasm, 'pendingClose');
    await runNetPeerFileWriteTest(wasm, 'cancelNotify');
    await runNetPeerFileWriteTest(wasm, 'cancelNoHandler');
    await runNetPeerFileWriteTest(wasm, 'cancelInvalid');
    await runNetPeerFileWriteTest(wasm, 'schedule');
    await runNetPeerHostileErrorTest(wasm, 'readCodeGetter');
    await runNetPeerHostileErrorTest(wasm, 'inputCloseCodeGetter');
    for (const property of ['ready', 'disposed']) {
        for (const readFailure of [false, true]) {
            await runNetPeerHostileErrorTest(wasm, property + (readFailure ? ' after read failure' : ' on cancel'),
                () => checkNetPeerCancelState(wasm, property, readFailure));
        }
    }
}

if (typeof module !== 'undefined' && module.exports) {
    module.exports = runNetPeerHostTest;
    if (require.main === module) {
        const entry = require('node:path').resolve(process.argv[2]);
        const wasmBinary = require('node:fs').readFileSync(entry.replace(/\.js$/, '.wasm'));
        // Node 可以在 Promise 仍悬而未决时正常退出，只有全部异步场景完成才能将退出码置为成功。
        process.exitCode = 1;
        const createModule = require(entry);
        Promise.all([createModule({wasmBinary}), createModule({wasmBinary})])
            .then(([wasm, other]) => runNetPeerHostTest(wasm, other)).then(() => {
            console.log('NetPeerWasmHost passed');
            process.exitCode = 0;
        }).catch(error => { console.error(error); });
    }
}