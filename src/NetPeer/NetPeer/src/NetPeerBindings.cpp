#include "NetPeerMacro.h"
#if defined(__EMSCRIPTEN__)
#include "NetPeerBindings.h"
#include "NetSenderC.h"
#include <cstddef>
#include <emscripten/emscripten.h>

/** 为JS运行环境配置客户端备用入口；属于NetPeer宿主配置，不实现业务协议
@param [in] handle 本模块C门面创建的客户端
@param [in] seeds 可空的网关列表，调用期间借用
@param [in] cachePath 可空的公开缓存路径，调用期间借用
@return NI_OK或配置、生命周期错误
*/
extern "C" EMSCRIPTEN_KEEPALIVE int32_t netPeerWasmConfigureGateways(NetSenderClientHandle handle,
    const char* seeds, const char* cachePath)
{
    return netSenderClientConfigureGateways(handle, seeds, cachePath);
}

// 编译期宿主入口只做ABI转发；uint64连接号独立传递，不经uintptr_t截断。
EM_JS(int32_t, NetPeerCallJsHost, (int32_t operation, uintptr_t a, uintptr_t b, uintptr_t c,
    uintptr_t d, uintptr_t e, uintptr_t f, uintptr_t g, uint64_t peer), {
    return Module.__netPeerHostCallback(operation, a, b, c, d, e, f, g, peer);
});

// 编译期客户端入口保留BigInt代次和请求号，context只用于查找所属Sender。
EM_JS(int32_t, NetPeerCallJsClient, (int32_t event, void* context, uint64_t generation,
    uint64_t request, uint32_t product, uint32_t type, int32_t status, const void* bytes, uint32_t size), {
    return Module.__netPeerClientCallback(event, context, generation, request, product, type, status, bytes, size);
});

/** 创建JS字节宿主，失败保持输出为空
@param [in] provider 提供方上下文
@param [in] observer 事件接收上下文
@param [in] connected,data,writable,closed 已编译的原生事件入口
@param [out] instance 创建的实例标识
@return 0成功，负数失败
*/
static int32_t NetPeerJsCreate(void* provider, void* observer, NetHostConnected connected, NetHostData data,
    NetHostWritable writable, NetHostClosed closed, void** instance)
{
    *instance = nullptr;
    return NetPeerCallJsHost(0, reinterpret_cast<uintptr_t>(provider), reinterpret_cast<uintptr_t>(observer),
        reinterpret_cast<uintptr_t>(connected), reinterpret_cast<uintptr_t>(data), reinterpret_cast<uintptr_t>(writable),
        reinterpret_cast<uintptr_t>(closed), reinterpret_cast<uintptr_t>(instance), 0);
}

/** 连接JS宿主
@param [in] instance,address,port 宿主实例及目标地址端口
@param [out] peer 连接号，失败为0
@return 0成功，负数失败
*/
static int32_t NetPeerJsConnect(void* instance, const char* address, uint32_t port, uint64_t* peer)
{
    *peer = 0;
    return NetPeerCallJsHost(1, reinterpret_cast<uintptr_t>(instance), reinterpret_cast<uintptr_t>(address),
        port, reinterpret_cast<uintptr_t>(peer), 0, 0, 0, 0);
}

/** 非阻塞推进JS宿主
@param [in] instance 宿主实例
@return 1活动，0停止，负数失败
*/
static int32_t NetPeerJsPoll(void* instance)
{
    return NetPeerCallJsHost(2, reinterpret_cast<uintptr_t>(instance), 0, 0, 0, 0, 0, 0, 0);
}

/** 将原始字节交给JS网络接口
@param [in] instance,peer 宿主及连接号
@param [in] data,size 本次借用的字节和长度
@return 0已交付，1背压，负数失败
*/
static int32_t NetPeerJsWrite(void* instance, uint64_t peer, const uint8_t* data, uint32_t size)
{
    return NetPeerCallJsHost(3, reinterpret_cast<uintptr_t>(instance), reinterpret_cast<uintptr_t>(data), size, 0, 0, 0, 0, peer);
}

/** 设置JS宿主读取暂停
@param [in] instance,peer 宿主及连接号
@param [in] paused 0继续，1暂停
@return 0成功，负数失败
*/
static int32_t NetPeerJsPause(void* instance, uint64_t peer, int32_t paused)
{
    return NetPeerCallJsHost(4, reinterpret_cast<uintptr_t>(instance), static_cast<uint32_t>(paused), 0, 0, 0, 0, 0, peer);
}

/** 请求关闭JS连接
@param [in] instance,peer,reason 宿主、连接号和关闭原因
@return 0成功，负数失败
*/
static int32_t NetPeerJsClose(void* instance, uint64_t peer, int32_t reason)
{
    return NetPeerCallJsHost(5, reinterpret_cast<uintptr_t>(instance), static_cast<uint32_t>(reason), 0, 0, 0, 0, 0, peer);
}

/** 停止JS宿主
@param [in] instance 宿主实例
@return 0成功，负数失败
*/
static int32_t NetPeerJsShutdown(void* instance)
{
    return NetPeerCallJsHost(6, reinterpret_cast<uintptr_t>(instance), 0, 0, 0, 0, 0, 0, 0);
}

/** 在事件收敛后释放JS宿主
@param [in] instance 宿主实例
@return 0已释放，负数仍保留
*/
static int32_t NetPeerJsDestroy(void* instance)
{
    return NetPeerCallJsHost(7, reinterpret_cast<uintptr_t>(instance), 0, 0, 0, 0, 0, 0, 0);
}

/** 查询实际监听端口
@param [in] instance 宿主实例
@return 端口，未监听为0
*/
static uint32_t NetPeerJsLocalPort(void* instance)
{
    return static_cast<uint32_t>(NetPeerCallJsHost(8, reinterpret_cast<uintptr_t>(instance), 0, 0, 0, 0, 0, 0, 0));
}

/** 查询包含关闭中的连接数
@param [in] instance 宿主实例
@return 连接资源数
*/
static uint32_t NetPeerJsPeerCount(void* instance)
{
    return static_cast<uint32_t>(NetPeerCallJsHost(9, reinterpret_cast<uintptr_t>(instance), 0, 0, 0, 0, 0, 0, 0));
}

/** 从JS强随机池读取
@param [in] context 提供方上下文
@param [out] output,size 输出缓冲及长度
@return 0完整成功，负数失败
*/
static int32_t NetPeerJsEntropy(void* context, uint8_t* output, uint32_t size)
{
    return NetPeerCallJsHost(10, reinterpret_cast<uintptr_t>(context), reinterpret_cast<uintptr_t>(output), size, 0, 0, 0, 0, 0);
}

/** 转发会话就绪
@param [in] context,generation 所属Sender标识及连接代次
@param [in] requests,chunk 协商额度
*/
static void NetPeerJsReady(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
{
    NetPeerCallJsClient(0, context, generation, 0, requests, chunk, 0, nullptr, 0);
}

/** 转发请求结果，字节只借用至JS完成复制
@param [in] context,generation 所属Sender及连接代次
@param [in] request,product,type,status 原始请求标识及结果
@param [in] bytes,size 结果字节及长度
*/
static void NetPeerJsResult(void* context, uint64_t generation, uint64_t request, uint32_t product,
    uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    NetPeerCallJsClient(1, context, generation, request, product, type, static_cast<int32_t>(status), bytes, size);
}

/** 转发应用帧
@param [in] context,generation 所属Sender及连接代次
@param [in] frame 借用到返回的原生帧
@return 1已处理，0背压
*/
static int32_t NetPeerJsFrame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
{
    return NetPeerCallJsClient(2, context, generation, 0, 0, 0, 0, frame, 0);
}

/** 转发连接关闭
@param [in] context,generation 所属Sender及连接代次
@param [in] reason,detail 关闭原因及附加信息
*/
static void NetPeerJsClosed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    NetPeerCallJsClient(3, context, generation, 0, 0, 0, reason, nullptr, static_cast<uint32_t>(detail));
}

// 正式JS绑定由编译器写入NetPeerAPI.js，只转换ABI及原始宿主I/O。
EM_JS_DEPS(net_peer_bindings, "$getWasmTableEntry");
EM_JS(void, NetPeerInstallJsBindings, (const uint32_t* layout, const uintptr_t* entries), {
    const module = Module;
    const fields = ["callbacksSize","callbacks_context","callbacks_ready","callbacks_result","callbacks_frame","callbacks_closed","optionsSize","options_address","options_port","options_tls","options_platform","options_capabilities","options_maxRequests","options_maxChunk","options_handshakeTimeoutMs","options_device","tlsSize","tls_server","tls_authority","tls_authoritySize","tls_certificate","tls_certificateSize","tls_privateKey","tls_privateKeySize","tls_hostname","tls_entropy","tls_entropyContext","tls_handshakeTimeoutMs","frameSize","frame_m_productId","frame_m_requestId","frame_m_messageType","frame_m_kind","frame_m_payloadSize","frame_m_payload","hostSize","host_context","host_create","host_listen","host_connect","host_poll","host_write","host_pauseRead","host_close","host_shutdown","host_destroy","host_localPort","host_peerCount","host_entropy"];
    const offsets = Object.freeze(Object.fromEntries(fields.map((key, i) => [key, module.HEAPU32[Number(layout) / 4 + i]])));
    const pointerBytes = module._netPeerPointerBytes();
    const callbackStorage = (offsets.callbacksSize + 7) & ~7;
    const nativePointer = value => pointerBytes === 8 ? BigInt(value) : value;
    const index = value => {
        const number = Number(value);
        if (!Number.isSafeInteger(number) || number < 0 || number > module.HEAPU8.length) throw new RangeError('Invalid Wasm pointer');
        return number;
    };
    const view = () => new DataView(module.HEAPU8.buffer);
    const pointer = address => pointerBytes === 8 ? index(view().getBigUint64(address, true)) : index(view().getUint32(address, true));
    const putPointer = (address, value) => {
        if (pointerBytes === 8) view().setBigUint64(address, BigInt(value), true);
        else view().setUint32(address, index(value), true);
    };
    const u32 = (address, value) => view().setUint32(address, value, true);
    const integer = (value, min, max) => Number.isInteger(value) && value >= min && value <= max;
    const requestNumber = value => {
        if (typeof value !== 'bigint' || value < 0n || value > 0xffffffffffffffffn) throw new RangeError('uint64 BigInt is required');
        return value;
    };
    const binary = value => value instanceof Uint8Array ? value : value instanceof ArrayBuffer ? new Uint8Array(value) : null;
    const allocate = size => {
        const result = module._malloc(Math.max(1, size));
        if (!result) throw new Error('Wasm memory allocation failed');
        module.HEAPU8.fill(0, result, result + Math.max(1, size));
        return result;
    };
    const wipeFree = (address, size) => {
        if (address) { module.HEAPU8.fill(0, address, address + Math.max(1, size)); module._free(address); }
    };
    const textBytes = value => {
        if (typeof value !== 'string' || value.indexOf('\0') >= 0 || value.length > 4096) throw new RangeError('Invalid UTF-8 input');
        const text = unescape(encodeURIComponent(value));
        const bytes = new Uint8Array(text.length + 1);
        for (let i = 0; i < text.length; ++i) bytes[i] = text.charCodeAt(i);
        return bytes;
    };
    const textAt = address => {
        address = index(address);
        let text = '';
        for (let i = 0; i < 4097 && address + i < module.HEAPU8.length; ++i) {
            const byte = module.HEAPU8[address + i];
            if (!byte) return decodeURIComponent(escape(text));
            text += String.fromCharCode(byte);
        }
        throw new RangeError('Unterminated host address');
    };
    const frameAt = address => {
        address = index(address);
        const size = view().getUint32(address + offsets.frame_m_payloadSize, true);
        const payload = pointer(address + offsets.frame_m_payload);
        if (size > 16777216 || payload + size > module.HEAPU8.length) throw new RangeError('Invalid frame payload');
        return { product: view().getUint32(address + offsets.frame_m_productId, true),
            requestId: view().getBigUint64(address + offsets.frame_m_requestId, true),
            type: view().getUint32(address + offsets.frame_m_messageType, true),
            kind: view().getUint16(address + offsets.frame_m_kind, true), bytes: module.HEAPU8.slice(payload, payload + size) };
    };
    const senderStates = new WeakMap();
    const senderContexts = new Map();
    let nextSenderContext = 0;
    let runtimeInstance = null;
    let activeHostCallbacks = null;
    const hostNames = ['create', 'connect', 'poll', 'write', 'pauseRead', 'close', 'shutdown', 'destroy', 'localPort', 'peerCount', 'entropy'];
    // 这里只读取编译器已放入主模块函数表的入口，不增表项、不动态编译Wasm。
    const fixedEntries = Array.from({ length: 15 }, (ignored, i) => pointer(Number(entries) + i * pointerBytes));
    module.__netPeerHostCallback = (operation, a, b, c, d, e, f, g, peer) => {
        if (!activeHostCallbacks) return operation === 8 || operation === 9 ? 0 : -1;
        try {
            const fn = activeHostCallbacks.functions[operation];
            if (operation === 0) return fn(a, b, c, d, e, f, g);
            if (operation === 1) return fn(a, b, Number(c), d);
            if (operation === 3) return fn(a, peer, b, Number(c));
            if (operation === 4 || operation === 5) return fn(a, peer, Number(b) | 0);
            if (operation === 10) return fn(a, b, Number(c));
            return fn(a);
        } catch (error) {
            activeHostCallbacks.runtime.lastBindingError = error;
            return operation === 8 || operation === 9 ? 0 : -1;
        }
    };
    module.__netPeerClientCallback = (event, context, generation, request, product, type, status, bytes, size) => {
        const state = senderContexts.get(Number(context));
        // 标识不复用；失败登记、已销毁及旧Sender的迟到回调均不能投递给新对象。
        if (!state || !state.handle) return event === 2 ? 1 : 0;
        ++state.depth;
        try {
            if (event === 0) state.listener.onSessionReady(generation, product, type);
            else if (event === 1) state.listener.onSessionResult(generation, request, product, type, status,
                module.HEAPU8.slice(index(bytes), index(bytes) + size));
            else if (event === 2) return state.listener.onSessionFrame(generation, frameAt(bytes)) === false ? 0 : 1;
            else state.listener.onSessionClosed(generation, status, size | 0);
            return 0;
        } catch (error) {
            state.runtime.lastBindingError = error; state.error = -7;
            module._netPeerSenderClientClose(state.handle, -7);
            return event === 2 ? 1 : 0;
        } finally { --state.depth; }
    };

    // 单个JS执行者使用的Sender门面；句柄及回调存储由所属runtime管理。
    class NetSenderClient {
        // 线程：仅所属JS执行者串行操作；禁止回调中调用，变更生命周期前须等待在途send及回调收敛。
        constructor(runtime, listener) {
            if (!(runtime instanceof NetPeerRuntime) || runtime !== runtimeInstance) throw new TypeError('Use this module NetPeerRuntime');
            senderStates.set(this, { runtime, listener, handle: 0, memory: 0, context: 0, error: -7, depth: 0 });
        }
        // 线程：仅所属JS执行者串行操作；禁止回调中调用，变更生命周期前须等待在途send及回调收敛。

        // 线程：仅所属JS执行者串行操作；禁止回调中调用，变更生命周期前须等待在途send及回调收敛。
        prepareGateway() {
            const state = senderStates.get(this);
            if (state.depth) return this._record(-7);
            if (state.handle) { state.error = 0; return 0; }
            if (nextSenderContext >= 0x7fffffff) return this._record(-3);
            const context = ++nextSenderContext;
            let memory = 0;
            try {
                memory = allocate(callbackStorage + 16);
                u32(memory, offsets.callbacksSize); u32(memory + 4, 1);
                putPointer(memory + offsets.callbacks_context, context);
                ['ready', 'result', 'frame', 'closed'].forEach((name, i) =>
                    putPointer(memory + offsets['callbacks_' + name], fixedEntries[hostNames.length + i]));
                senderContexts.set(context, state);
                const resultCode = module._netPeerSenderClientCreate(memory, memory + callbackStorage);
                if (resultCode !== 0) {
                    senderContexts.delete(context); wipeFree(memory, callbackStorage + 16);
                    state.error = resultCode; return resultCode;
                }
                state.handle = pointer(memory + callbackStorage); state.memory = memory; state.context = context; state.error = 0;
                ++state.runtime.clients;
                return 0;
            } catch (error) {
                senderContexts.delete(context); wipeFree(memory, callbackStorage + 16);
                state.runtime.lastBindingError = error; state.error = -5; return -5;
            }
        }
        // 线程：仅所属JS执行者串行操作；禁止回调中调用，变更生命周期前须等待在途send及回调收敛。
        connectGateway(options, listener) { return this._connect(0, options, listener); }
        // 线程：仅所属JS执行者串行操作；禁止回调中调用，变更生命周期前须等待在途send及回调收敛。
        connectServiceGateway(product, options, listener) {
            if (!integer(product, 1, 0x7fffffff)) return this._record(-1);
            return this._connect(product, options, listener);
        }
        _connect(product, options, listener) {
            const state = senderStates.get(this);
            if (!state.handle || state.depth) return this._record(-7);
            listener = listener || state.listener;
            if (!listener || !['onSessionReady', 'onSessionResult', 'onSessionFrame', 'onSessionClosed'].every(name => typeof listener[name] === 'function')) return this._record(-1);
            const pid = options && (options.localGatewayPid || options.m_localGatewayPid || 0);
            options = options && (options.network || options.m_network || options);
            if (!options || pid !== 0) return this._record(-1);
            let memory = 0;
            const owned = [];
            try {
                const device = binary(options.device);
                const capabilities = options.capabilities === undefined ? 7 : options.capabilities;
                const requests = options.maxRequests === undefined ? 32 : options.maxRequests;
                const chunk = options.maxChunk === undefined ? 65536 : options.maxChunk;
                const timeout = options.timeoutMs === undefined ? 5000 : options.timeoutMs;
                if (!device || device.length !== 16 || !device.some(value => value !== 0) || !integer(options.platform, 1, 3) ||
                    !integer(options.port, 1, 65535) || !integer(capabilities, 0, 0xffffffff) || !integer(requests, 1, 128) ||
                    !integer(chunk, 1024, 65536) || !integer(timeout, 100, 60000)) return this._record(-1);
                memory = allocate(offsets.optionsSize);
                const copy = bytes => { const address = allocate(bytes.length); owned.push([address, bytes.length]); module.HEAPU8.set(bytes, address); return address; };
                putPointer(memory + offsets.options_address, copy(textBytes(options.address)));
                u32(memory, offsets.optionsSize); u32(memory + 4, 1); u32(memory + offsets.options_port, options.port);
                u32(memory + offsets.options_platform, options.platform); u32(memory + offsets.options_capabilities, capabilities);
                u32(memory + offsets.options_maxRequests, requests); u32(memory + offsets.options_maxChunk, chunk);
                u32(memory + offsets.options_handshakeTimeoutMs, timeout); module.HEAPU8.set(device, memory + offsets.options_device);
                const tls = options.tls || {};
                const tlsAddress = memory + offsets.options_tls;
                const tlsTimeout = tls.timeoutMs === undefined ? timeout : tls.timeoutMs;
                if (!integer(tlsTimeout, 100, 60000)) return this._record(-1);
                u32(tlsAddress + offsets.tls_handshakeTimeoutMs, tlsTimeout);
                if (tls.hostname) putPointer(tlsAddress + offsets.tls_hostname, copy(textBytes(tls.hostname)));
                for (const name of ['authority', 'certificate', 'privateKey']) {
                    if (tls[name] !== undefined) {
                        const bytes = binary(tls[name]);
                        if (!bytes) return this._record(-1);
                        putPointer(tlsAddress + offsets['tls_' + name], copy(bytes));
                        u32(tlsAddress + offsets['tls_' + name + 'Size'], bytes.length);
                    }
                }
                const previousListener = state.listener;
                state.listener = listener;
                const result = product ? module._netPeerSenderClientConnectService(state.handle, product, 0, memory) : module._netPeerSenderClientConnect(state.handle, memory);
                if (result !== 0) state.listener = previousListener;
                return this._record(result);
            } catch (error) { return this._record(-1); }
            finally { owned.forEach(item => wipeFree(item[0], item[1])); wipeFree(memory, offsets.optionsSize); }
        }
        // 线程：仅所属JS执行者串行操作；禁止回调中调用，变更生命周期前须等待在途send及回调收敛。
        configureCompression(types) {
            const state = senderStates.get(this);
            if (!state.handle) return this._record(-7);
            if (!Array.isArray(types) || types.length > 64 || types.some(value => !integer(value, 1, 0xffffffff))) return this._record(-1);
            let memory = 0;
            try {
                memory = allocate(types.length * 4);
                types.forEach((value, i) => u32(memory + i * 4, value));
                return this._record(module._netPeerSenderClientConfigureCompression(state.handle, memory, types.length));
            } catch (error) { return this._record(-5); }
            finally { wipeFree(memory, types.length * 4); }
        }
        // 线程：仅所属JS执行者非重入推进，禁止在本对象协议回调中调用。
        pollGateway() {
            const state = senderStates.get(this);
            if (!state.handle) return false;
            const result = module._netPeerSenderClientPoll(state.handle);
            if (result < 0) this._record(result);
            return result === 2;
        }
        // 线程：仅所属JS执行者提交，可在回调中调用；禁止跨Worker共享实例或句柄。
        beginRequest(product, type, timeoutMs, output) {
            const state = senderStates.get(this);
            let result = -7, requestId = 0n;
            if (state.handle) {
                if (!integer(product, 1, 0xffffffff) || !integer(type, 0, 0xffffffff) || !integer(timeoutMs, 1, 60000)) result = -1;
                else {
                    const scratch = state.memory + callbackStorage;
                    result = module._netPeerSenderClientBegin(state.handle, product, type, timeoutMs, scratch);
                    requestId = view().getBigUint64(scratch, true);
                }
            }
            this._record(result);
            if (output) { output.requestId = requestId; return result; }
            return { result, requestId };
        }
        // 同一JS执行者按成功send／post的提交先后交付，begin仅预留；响应完成顺序由业务决定。
        // 复制入队后返回数值结果，宿主随后poll，不能跨Worker共用实例。
        // 线程：仅所属JS执行者提交，可在回调中调用；禁止跨Worker共享实例或句柄。
        postRequest(requestId, bytes) {
            const state = senderStates.get(this);
            bytes = binary(bytes);
            if (!state.handle) return this._record(-7);
            if (!bytes || bytes.length > 1048560) return this._record(-1);
            let memory = 0;
            try {
                requestNumber(requestId); memory = allocate(bytes.length); module.HEAPU8.set(bytes, memory);
                return this._record(module._netPeerSenderClientPost(state.handle, requestId, memory, bytes.length));
            } catch (error) { return this._record(-1); }
            finally { wipeFree(memory, bytes.length); }
        }
        // send返回Promise；await结果为0才表示交付网络宿主，绝不阻塞唯一JS线程。
        // 线程：仅所属JS执行者调用，可同时等待多个Promise；禁止在本对象协议回调中调用，不能忙等。
        async sendRequest(requestId, bytes) {
            const state = senderStates.get(this);
            bytes = binary(bytes);
            if (!state.handle || state.depth) return this._record(-7);
            if (!bytes || bytes.length > 1048560) return this._record(-1);
            let memory = 0;
            let ticket = 0n;
            try {
                requestNumber(requestId); memory = allocate(bytes.length + 8); module.HEAPU8.set(bytes, memory + 8);
                const accepted = module._netPeerSenderClientSendBegin(state.handle, requestId, memory + 8, bytes.length, memory);
                if (accepted !== 0) return this._record(accepted);
                ticket = view().getBigUint64(memory, true);
            } catch (error) { return this._record(-1); }
            finally { wipeFree(memory, bytes.length + 8); }
            return this._waitSend(ticket);
        }
        // 同一JS执行者调用；await到0才表示全部交给网络库，禁止协议回调中等待。
        // 线程：仅所属JS执行者调用，可同时等待多个Promise；禁止在本对象协议回调中调用，不能忙等。
        async sendData(frame) {
            const state = senderStates.get(this);
            if (!state.handle || state.depth) return this._record(-7);
            const bytes = frame && binary(frame.bytes);
            if (!bytes || bytes.length > 65664 || !integer(frame.kind, 0, 65535) || !integer(frame.product, 1, 0xffffffff) || !integer(frame.type, 0, 0xffffffff)) return this._record(-1);
            let memory = 0;
            let ticket = 0n;
            try {
                requestNumber(frame.requestId); memory = allocate(offsets.frameSize + bytes.length + 15);
                u32(memory + offsets.frame_m_productId, frame.product); view().setBigUint64(memory + offsets.frame_m_requestId, frame.requestId, true);
                u32(memory + offsets.frame_m_messageType, frame.type); view().setUint16(memory + offsets.frame_m_kind, frame.kind, true);
                u32(memory + offsets.frame_m_payloadSize, bytes.length); putPointer(memory + offsets.frame_m_payload, memory + offsets.frameSize);
                module.HEAPU8.set(bytes, memory + offsets.frameSize);
                const output = memory + ((offsets.frameSize + bytes.length + 7) & ~7);
                const accepted = module._netPeerSenderClientSendDataBegin(state.handle, memory, output);
                if (accepted !== 0) return this._record(accepted);
                ticket = view().getBigUint64(output, true);
            } catch (error) { return this._record(-1); }
            finally { wipeFree(memory, offsets.frameSize + bytes.length + 15); }
            return this._waitSend(ticket);
        }
        async _waitSend(ticket) {
            const state = senderStates.get(this);
            for (;;) {
                const result = module._netPeerSenderClientSendResult(state.handle, ticket);
                if (result !== 2) return this._record(result);
                this.pollGateway();
                // 允许socket完成、取消、关闭和其他客户端的事件获得执行机会。
                await new Promise(resolve => setTimeout(resolve, 0));
            }
        }
        // 线程：仅所属JS执行者提交，可在回调中调用；禁止跨Worker共享实例或句柄。
        cancelRequest(requestId) {
            const state = senderStates.get(this);
            if (!state.handle) return this._record(-7);
            try { return this._record(module._netPeerSenderClientCancel(state.handle, requestNumber(requestId))); }
            catch (error) { return this._record(-1); }
        }
        // 载荷在返回前复制；后续事件循环推进发送，不要求后台线程。
        // 线程：仅所属JS执行者提交，可在回调中调用；禁止跨Worker共享实例或句柄。
        postData(frame) {
            const state = senderStates.get(this);
            if (!state.handle) return this._record(-7);
            const bytes = frame && binary(frame.bytes);
            if (!bytes || bytes.length > 65664 || !integer(frame.kind, 0, 65535) || !integer(frame.product, 1, 0xffffffff) || !integer(frame.type, 0, 0xffffffff)) return this._record(-1);
            let memory = 0;
            try {
                requestNumber(frame.requestId); memory = allocate(offsets.frameSize + bytes.length);
                u32(memory + offsets.frame_m_productId, frame.product); view().setBigUint64(memory + offsets.frame_m_requestId, frame.requestId, true);
                u32(memory + offsets.frame_m_messageType, frame.type); view().setUint16(memory + offsets.frame_m_kind, frame.kind, true);
                u32(memory + offsets.frame_m_payloadSize, bytes.length); putPointer(memory + offsets.frame_m_payload, memory + offsets.frameSize);
                module.HEAPU8.set(bytes, memory + offsets.frameSize);
                return this._record(module._netPeerSenderClientPostData(state.handle, memory));
            } catch (error) { return this._record(-1); }
            finally { wipeFree(memory, offsets.frameSize + bytes.length); }
        }
        // 线程：仅所属JS执行者提交，可在回调中调用；禁止跨Worker共享实例或句柄。
        closeGateway(reason = 0) {
            const state = senderStates.get(this);
            if (state.handle) this._record(module._netPeerSenderClientClose(state.handle, reason));
        }
        // 线程：仅所属JS执行者提交，可在回调中调用；禁止跨Worker共享实例或句柄。
        uninitGateway() { this.closeGateway(0); }
        // 线程：仅所属JS执行者串行操作；禁止回调中调用，变更生命周期前须等待在途send及回调收敛。
        releaseGateway() {
            const state = senderStates.get(this);
            if (!state.handle) return 0;
            if (state.depth) return this._record(-7);
            const result = module._netPeerSenderClientDestroy(state.handle);
            if (result !== 0) return this._record(result);
            state.handle = 0; senderContexts.delete(state.context); state.context = 0;
            wipeFree(state.memory, callbackStorage + 16); state.memory = 0; state.error = -7;
            --state.runtime.clients;
            return 0;
        }
        // 线程：仅所属JS执行者查询，可在回调中调用；各操作以直接返回值为准。
        gatewayReady() {
            const state = senderStates.get(this);
            if (!state.handle) return false;
            const scratch = state.memory + callbackStorage;
            return module._netPeerSenderClientState(state.handle, scratch, scratch + 8) === 0 && view().getInt32(scratch + 8, true) === 1;
        }
        // 线程：仅所属JS执行者查询，可在回调中调用；各操作以直接返回值为准。
        gatewayGeneration() {
            const state = senderStates.get(this);
            if (!state.handle) return 0n;
            const scratch = state.memory + callbackStorage;
            return module._netPeerSenderClientState(state.handle, scratch, scratch + 8) === 0 ? view().getBigUint64(scratch, true) : 0n;
        }
        // 线程：仅所属JS执行者查询，可在回调中调用；各操作以直接返回值为准。
        lastGatewayError() { return senderStates.get(this).error; }
        _record(result) { senderStates.get(this).error = result; return result; }
    }

    // 每个Wasm模块唯一的JS宿主，管理字节连接、强随机池和Sender生命周期。
    class NetPeerRuntime {
        constructor(options) {
            if (runtimeInstance) throw new Error('One native byte host is allowed per module');
            if (!options || typeof options.createSocket !== 'function' || typeof options.randomSource !== 'function') throw new TypeError('Raw socket and strong random hosts are required');
            this.module = module; this.options = options; this.randomSource = options.randomSource;
            this.random = new Uint8Array(0); this.randomOffset = 0; this.refilling = null; this.randomError = null;
            this.clients = 0; this.hosts = new Map(); this.nextHost = 0; this.memory = 0; this.lastBindingError = null;
        }
        /** 准备强随机池并安装本模块唯一字节宿主
        @param [in] options 连接工厂及强随机回调，runtime销毁前保持有效
        @return 就绪runtime的Promise；配置或随机源失败时拒绝
        */
        static async create(options) {
            const runtime = new NetPeerRuntime(options);
            try {
                await runtime.refillRandom();
                runtime.installHost();
            } catch (error) { runtime.random.fill(0); throw error; }
            runtimeInstance = runtime;
            module.wasmBits = pointerBytes * 8; module.wasmBigInt = true;
            return runtime;
        }
        createSender(listener) { return NetSender.instance().createClient(this, listener); }
        async refillRandom() {
            if (this.refilling) return this.refilling;
            this.refilling = Promise.resolve().then(() => this.randomSource(8192)).then(value => {
                const bytes = binary(value);
                if (!bytes || bytes.length !== 8192) throw new Error('Strong random source returned a short buffer');
                const remaining = this.random.subarray(this.randomOffset);
                if (remaining.length > 8192) throw new Error('Random host budget exceeded');
                const next = new Uint8Array(remaining.length + bytes.length); next.set(remaining); next.set(bytes, remaining.length);
                this.random.fill(0); bytes.fill(0); this.random = next; this.randomOffset = 0; this.randomError = null;
            }).catch(error => { this.randomError = error; throw error; }).finally(() => { this.refilling = null; });
            return this.refilling;
        }
        copyRandom(address, size) {
            address = index(address);
            if (!integer(size, 0, 8192) || address + size > module.HEAPU8.length) return -1;
            if (this.randomError || size > this.random.length - this.randomOffset) { module.HEAPU8.fill(0, address, address + size); return -1; }
            module.HEAPU8.set(this.random.subarray(this.randomOffset, this.randomOffset + size), address);
            this.random.fill(0, this.randomOffset, this.randomOffset + size); this.randomOffset += size;
            if (this.random.length - this.randomOffset < 4096 && !this.refilling) this.refillRandom().catch(() => {});
            return 0;
        }
        configureGateways(sender, seeds, cachePath = '') {
            const state = senderStates.get(sender);
            if (!state || state.runtime !== this || !state.handle) return -7;
            const owned = [];
            try {
                const copy = value => { const bytes = textBytes(value); const address = allocate(bytes.length); owned.push([address, bytes.length]); module.HEAPU8.set(bytes, address); return address; };
                return module._netPeerWasmConfigureGateways(state.handle, copy(seeds), copy(cachePath));
            } catch (error) { return -1; }
            finally { owned.forEach(item => wipeFree(item[0], item[1])); }
        }
        installHost() {
            if (runtimeInstance || activeHostCallbacks) throw new Error('One native byte host is allowed per module');
            const functions = [];
            this.memory = allocate(offsets.hostSize);
            u32(this.memory, offsets.hostSize); u32(this.memory + 4, 1);
            const add = (name, fn) => {
                functions.push(fn); putPointer(this.memory + offsets['host_' + name], fixedEntries[hostNames.indexOf(name)]);
            };
            const hostAt = value => this.hosts.get(Number(value));
            const notify = (host, callback, ...args) => getWasmTableEntry(Number(callback))(nativePointer(host.observer), ...args);
            const queue = (host, kind, bytes, reason) => {
                if (host.dead || host.ended) return;
                try {
                if (kind === 'data') {
                    bytes = binary(bytes);
                    if (!bytes || host.bytes + bytes.length > 4194304 || host.events.length + Math.ceil(bytes.length / 16384) > 512) {
                        host.fault = -3; return;
                    }
                    for (let at = 0; at < bytes.length; at += 16384) { const copy = bytes.slice(at, at + 16384); host.events.push({ kind, bytes: copy }); host.bytes += copy.length; }
                } else {
                    if (kind === 'writable' && host.writablePending) return;
                    if (host.events.length >= 1024) { host.fault = -3; return; }
                    if (kind === 'writable') host.writablePending = true;
                    if (kind === 'closed') host.ended = true;
                    host.events.push({ kind, reason: reason || 0 });
                }
                } catch (error) { host.fault = -5; }
            };
            const fail = (host, reason) => {
                if (host.closeReported) return;
                host.closeReported = true; host.ended = true; host.events = []; host.bytes = 0; host.output = []; host.outputBytes = 0;
                try { host.socket.close(); } catch (error) {}
                notify(host, host.closed, 1n, reason);
            };
            try {
                add('create', (provider, observer, connected, data, writable, closed, output) => {
                    if (this.nextHost >= 0x7fffffff) return -1;
                    const host = { id: ++this.nextHost, observer: Number(observer), connected, data, writable, closed, socket: null,
                        scratch: 0, events: [], bytes: 0, output: [], outputBytes: 0, writing: false, writablePending: false,
                        started: false, ready: false, paused: false, ended: false, closeReported: false, dead: false, stopping: false, polling: false, fault: 0 };
                    try {
                        host.scratch = allocate(16384);
                        host.socket = this.options.createSocket({ onConnect: () => queue(host, 'connected'), onData: bytes => queue(host, 'data', bytes),
                            onWritable: () => queue(host, 'writable'), onClose: reason => queue(host, 'closed', null, reason), onError: () => { if (!host.dead) host.fault = -7; } });
                        if (!host.socket || typeof host.socket.connect !== 'function' || typeof host.socket.write !== 'function' || typeof host.socket.close !== 'function') throw new TypeError('Raw host is incomplete');
                        this.hosts.set(host.id, host); putPointer(index(output), host.id); return 0;
                    } catch (error) { host.dead = true; if (host.socket) { try { host.socket.close(); if (typeof host.socket.dispose === 'function') host.socket.dispose(); } catch (ignored) {} } wipeFree(host.scratch, 16384); putPointer(index(output), 0); return -1; }
                });
                add('connect', (instance, address, port, peer) => {
                    const host = hostAt(instance); putPointer(index(peer), 0);
                    if (!host || host.started || host.stopping) return -1;
                    host.started = true;
                    try { const result = host.socket.connect(textAt(address), port); if (result < 0) return -1; view().setBigUint64(index(peer), 1n, true); return 0; }
                    catch (error) { host.fault = -7; return -1; }
                });
                add('poll', instance => {
                    const host = hostAt(instance);
                    if (!host || host.polling) return -1;
                    host.polling = true;
                    try {
                        if (host.fault) { fail(host, host.fault); return 0; }
                        for (let step = 0; step < 256 && host.events.length; ++step) {
                            const event = host.events[0];
                            if (event.kind === 'data' && host.paused && !host.stopping) break;
                            host.events.shift();
                            if (event.kind === 'connected') { host.ready = true; notify(host, host.connected, 1n); }
                            else if (event.kind === 'data') {
                                host.bytes -= event.bytes.length;
                                if (!host.stopping) { module.HEAPU8.set(event.bytes, host.scratch); notify(host, host.data, 1n, nativePointer(host.scratch), event.bytes.length); }
                            } else if (event.kind === 'writable') { host.writablePending = false; notify(host, host.writable, 1n); }
                            else { fail(host, event.reason); break; }
                        }
                        if (host.fault) fail(host, host.fault);
                        return host.closeReported ? 0 : 1;
                    } finally { host.polling = false; }
                });
                add('write', (instance, peer, bytes, size) => {
                    const host = hostAt(instance); bytes = index(bytes);
                    if (!host || peer !== 1n || !host.ready || host.stopping || host.ended || size > 32768 || bytes + size > module.HEAPU8.length) return -1;
                    if (host.output.length >= 64 || host.outputBytes + size > 65536) return 1;
                    let copy;
                    try { copy = module.HEAPU8.slice(bytes, bytes + size); host.output.push(copy); host.outputBytes += size; }
                    catch (error) { return -1; }
                    let retained = true;
                    const completed = () => {
                        if (!retained || host.dead || host.closeReported) return;
                        retained = false;
                        const at = host.output.indexOf(copy);
                        if (at >= 0) { host.output.splice(at, 1); host.outputBytes -= size; }
                        queue(host, 'writable');
                    };
                    try {
                        // write返回0前已真实调用网络库；output仅保留在途内存与预算，不是待调用的发送队列。
                        const result = host.socket.write(copy);
                        if (result && typeof result.then === 'function') {
                            result.then(completed, () => { completed(); if (!host.dead) host.fault = -7; });
                        } else {
                            completed();
                            if (result === 1 || result === false) return 1;
                            if (typeof result === 'number' && result < 0) return result;
                        }
                        return 0;
                    } catch (error) { completed(); host.fault = -7; return -1; }
                });
                add('pauseRead', (instance, peer, paused) => {
                    const host = hostAt(instance); if (!host || peer !== 1n || (paused !== 0 && paused !== 1)) return -1;
                    host.paused = !!paused;
                    if (typeof host.socket.pauseRead === 'function') host.socket.pauseRead(!!paused);
                    return 0;
                });
                const stop = (instance, reason) => {
                    const host = hostAt(instance); if (!host) return -1;
                    if (!host.stopping) {
                        host.stopping = true; host.events = host.events.filter(event => event.kind === 'closed'); host.bytes = 0;
                        try { host.socket.close(); } catch (error) { host.fault = -7; }
                        if (!host.ended) queue(host, 'closed', null, reason);
                    }
                    return 0;
                };
                add('close', (instance, peer, reason) => peer === 1n ? stop(instance, reason) : 0);
                add('shutdown', instance => stop(instance, 0));
                add('destroy', instance => {
                    const host = hostAt(instance); if (!host) return 0;
                    if (host.polling) return -1;
                    if (!host.stopping) stop(instance, 0);
                    host.dead = true;
                    if (typeof host.socket.dispose === 'function') host.socket.dispose();
                    this.hosts.delete(host.id); wipeFree(host.scratch, 16384); host.scratch = 0; host.events = []; host.output = []; return 0;
                });
                add('localPort', () => 0);
                add('peerCount', instance => { const host = hostAt(instance); return host && host.started && !host.closeReported ? 1 : 0; });
                add('entropy', (context, bytes, size) => this.copyRandom(bytes, size));
                activeHostCallbacks = { runtime: this, functions };
                const result = module._netPeerConfigureHost(this.memory);
                if (result !== 0) throw new Error('NetPeer host configuration failed: ' + result);
            } catch (error) {
                if (activeHostCallbacks && activeHostCallbacks.runtime === this) activeHostCallbacks = null;
                this.lastBindingError = error; wipeFree(this.memory, offsets.hostSize); this.memory = 0; throw error;
            }
        }
        /** 将微信TCP及强随机接口转换成字节宿主选项
        @param [in] wx 含createTCPSocket和getRandomValues的平台对象
        @return 可传入create的选项；缺少所需接口时抛出
        */
        static wxOptions(wx) {
            if (!wx || typeof wx.createTCPSocket !== 'function' || typeof wx.getRandomValues !== 'function') throw new Error('wx TCP and strong random APIs are required');
            return {
                randomSource: length => new Promise((resolve, reject) => wx.getRandomValues({ length, success: result => resolve(new Uint8Array(result.randomValues)), fail: reject })),
                createSocket: events => {
                    const socket = wx.createTCPSocket(); let dead = false;
                    const connected = () => { if (!dead) events.onConnect(); };
                    const data = result => { if (!dead) events.onData(new Uint8Array(result.message)); };
                    const error = result => { if (!dead) events.onError(result); };
                    const closed = () => { if (!dead) events.onClose(0); };
                    socket.onConnect(connected); socket.onMessage(data); socket.onError(error); socket.onClose(closed);
                    return {
                        connect: (address, port) => socket.connect({ address, port }),
                        write: bytes => socket.write(bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength)),
                        close: () => socket.close(),
                        dispose: () => {
                            dead = true;
                            socket.offConnect(connected); socket.offMessage(data); socket.offError(error); socket.offClose(closed);
                        }
                    };
                }
            };
        }
    }
    const readPointer = (ignored, address) => pointer(address);
    // Thin host bridge: C++ owns framing, tokens, hashes, offset/window checks and
    // completion. io is borrowed and implements async read/write/flush/commit/close.
    // Never run synchronous filesystem calls in socket callbacks.
    class NetFileTransfer {
        constructor(client, metadata, product, sender, io) {
            this.client = typeof client.connection === 'function' ? client.connection() : client;
            this.module = this.client.runtime.module;
            this.io = io;
            this.sender = !!sender;
            this.handle = 0;
            this.memory = 0;
            this.busy = false;
            this.closed = false;
            this.terminal = null;
            this.error = null;
            this.timer = null;
            this.pollQueued = false;
            this.primeOffset = 0;
            this.completed = new Promise(resolve => { this.resolveCompleted = resolve; });
            if (!(metadata instanceof Uint8Array) || metadata.length !== 160 || !io || typeof io.read !== 'function' ||
                typeof io.close !== 'function' || (!sender && (typeof io.write !== 'function' || typeof io.flush !== 'function' ||
                typeof io.commit !== 'function'))) throw new Error('File host adapter is incomplete');
            try {
                this.memory = this.module._malloc(66000);
                if (!this.memory) throw new Error('File memory budget exceeded');
                this.module.HEAPU8.set(metadata, this.memory + 128);
                this.check(this.module._netPeerFileInfoFields(this.memory + 128, 160, this.memory,
                    this.memory + 48, this.memory + 64, this.memory + 96));
                const fields = this.module.HEAPU32.slice(this.memory >>> 2, (this.memory + 48) >>> 2);
                this.resume = this.safeOffset(fields[6], fields[7]);
                this.stream = [fields[8], fields[9]];
                this.product = product;
                const now = this.time();
                this.check(this.module._netPeerFileFlowCreateWire(this.memory + 128, 160, product, sender ? 1 : 0,
                    now[0], now[1], 15000, this.memory));
                this.handle = readPointer(this.module, this.memory);
                // Optional host capability. Old cores and hosts keep single-block
                // writes; DATA and ACK retain the same wire format and durability.
                this.batchWrites = false;
                const batch = Math.min(fields[3], Math.floor((Number(io.maxWriteBytes) || fields[2]) / fields[2]));
                if (!sender && batch > 1 && typeof this.module._netPeerFileFlowSetWriteBatchChunks === 'function') {
                    this.check(this.module._netPeerFileFlowSetWriteBatchChunks(this.handle, batch));
                    this.batchWrites = true;
                }
            } catch (error) {
                this.destroy();
                throw error;
            }
        }

        check(result) {
            if (result < 0) {
                const error = new Error('File operation failed: ' + result);
                error.code = result;
                throw error;
            }
            return result;
        }

        time() {
            const now = Math.floor(this.client.options.clock());
            if (!Number.isSafeInteger(now) || now < 0) throw new Error('Invalid monotonic file clock');
            return [now >>> 0, Math.floor(now / 4294967296)];
        }

        safeOffset(low, high) {
            const value = low + high * 4294967296;
            if (!Number.isSafeInteger(value)) throw new Error('Host filesystem offset exceeds its safe range');
            return value;
        }

        // Return false without side effects when C++ has an unfinished durable write;
        // NetWxClient retains the current complete frame and retries it later.
        accept(frame) {
            if (!this.stream || frame.product !== this.product || frame.requestLow !== this.stream[0] || frame.requestHigh !== this.stream[1]) return false;
            if (this.closed || !this.handle) return true;
            const bytes = frame.payload;
            if (!(bytes instanceof Uint8Array) || bytes.length > 65624) throw new Error('File frame exceeds budget');
            this.module.HEAPU32.set([frame.kind, frame.product, frame.requestLow, frame.requestHigh, frame.type, bytes.length], this.memory >>> 2);
            this.module.HEAPU8.set(bytes, this.memory + 320);
            const result = this.module._netPeerFileFlowAccept(this.handle, this.memory, this.memory + 320);
            if (result === -3) return false;
            this.check(result);
            // Drain this socket delivery before crossing the host file bridge.
            // A timer can be clamped to one second in the simulator/background;
            // partial windows must also progress without waiting for that timer.
            if (this.batchWrites) this.queuePoll();
            else this.poll();
            return true;
        }

        queuePoll() {
            if (this.closed || this.pollQueued) return;
            this.pollQueued = true;
            Promise.resolve().then(() => { this.pollQueued = false; this.poll(); });
        }

        schedule() {
            if (!this.closed && this.timer === null) this.timer = setTimeout(() => { this.timer = null; this.poll(); }, 25);
        }

        job(work, completed) {
            this.busy = true;
            Promise.resolve().then(work).then(value => {
                if (!this.closed) completed(value);
            }).catch(error => { this.error = error; this.cancel(error.code < 0 ? error.code : -4); }).finally(() => {
                this.busy = false;
                if (!this.closed) { this.client.schedulePump(); this.poll(); }
                else if (this.finishing) this.closeFinished();
            });
        }

        poll() {
            if (this.closed || !this.handle) return;
            try {
                const now = this.time();
                const progress = this.module._netPeerFileFlowProgress(this.handle, now[0], now[1]);
                this.check(progress);
                if (!this.busy && this.primeOffset < this.resume) {
                    const offset = this.primeOffset;
                    const size = Math.min(65536, this.resume - offset);
                    this.job(() => this.io.read(offset, size), bytes => {
                        if (!(bytes instanceof Uint8Array) || bytes.length !== size) throw new Error('Short file prefix');
                        this.module.HEAPU8.set(bytes, this.memory + 320);
                        this.check(this.module._netPeerFileFlowPrime(this.handle, offset >>> 0, Math.floor(offset / 4294967296),
                            this.memory + 320, size));
                        this.primeOffset += size;
                    });
                } else if (!this.busy && this.sender) {
                    const result = this.module._netPeerFileFlowRead(this.handle, this.memory);
                    this.check(result);
                    if (result === 2) {
                        const fields = this.module.HEAPU32.slice(this.memory >>> 2, (this.memory + 12) >>> 2);
                        const offset = this.safeOffset(fields[0], fields[1]);
                        this.job(() => this.io.read(offset, fields[2]), bytes => {
                            if (!(bytes instanceof Uint8Array) || bytes.length !== fields[2]) throw new Error('Short file block');
                            this.module.HEAPU8.set(bytes, this.memory + 320);
                            this.check(this.module._netPeerFileFlowProduced(this.handle, fields[0], fields[1], this.memory + 320, bytes.length));
                        });
                    }
                } else if (!this.busy) {
                    const written = this.module._netPeerFileFlowWrite(this.handle, this.memory, this.memory + 24);
                    this.check(written);
                    if (written === 2) {
                        const fields = this.module.HEAPU32.slice(this.memory >>> 2, (this.memory + 12) >>> 2);
                        const pointer = readPointer(this.module, this.memory + 24);
                        const bytes = this.module.HEAPU8.slice(pointer, pointer + fields[2]);
                        const offset = this.safeOffset(fields[0], fields[1]);
                        this.job(async () => { await this.io.write(offset, bytes); await this.io.flush(); }, () => {
                            bytes.fill(0);
                            this.check(this.module._netPeerFileFlowWritten(this.handle, fields[0], fields[1], fields[2], 0));
                        });
                    } else if (this.module._netPeerFileFlowTakeCommit(this.handle) === 2) {
                        this.job(async () => {
                            await this.io.flush();
                            if (this.closed) return;
                            await this.io.close();
                            this.ioClosed = true;
                            if (this.closed) return;
                            await this.io.commit();
                        }, () => {
                            this.check(this.module._netPeerFileFlowCommitted(this.handle, 0));
                        });
                    }
                }
                const output = this.module._netPeerFileFlowPeek(this.handle, this.memory, this.memory + 24);
                this.check(output);
                if (output === 2) {
                    const fields = Array.from(this.module.HEAPU32.slice(this.memory >>> 2, (this.memory + 24) >>> 2));
                    const pointer = readPointer(this.module, this.memory + 24);
                    const bytes = this.module.HEAPU8.slice(pointer, pointer + fields[5]);
                    try {
                        this.client.sendFrame(fields.slice(0, 5), bytes);
                        this.check(this.module._netPeerFileFlowConsume(this.handle));
                    } catch (error) {
                        if (error.code !== -3) throw error;
                    } finally { bytes.fill(0); }
                } else if (progress === 2 && !this.busy) {
                    this.terminal = 0;
                    this.finish();
                    return;
                }
                this.schedule();
            } catch (error) { this.error = error; this.cancel(error.code < 0 ? error.code : -4); }
        }

        cancel(reason = -10) {
            if (this.closed) return;
            this.terminal = reason < 0 ? reason : -10;
            if (this.handle) this.module._netPeerFileFlowCancel(this.handle, this.terminal);
            if (this.client.ready && !this.client.disposed && this.stream) {
                try { this.client.sendFrame([6, this.product, this.stream[0], this.stream[1], 60005], new Uint8Array(0)); }
                catch (_) { this.client.fail(new Error('File cancellation could not be admitted')); }
            }
            this.finish();
        }

        finish() {
            if (this.finishing) return;
            this.finishing = true;
            clearTimeout(this.timer);
            this.timer = null;
            // An in-flight host job invokes closeFinished from its own settlement.
            // Do not wait for a clamped timer to release the file/network slot.
            this.closed = true;
            this.closeFinished();
        }

        closeFinished() {
            if (this.busy || this.closing) return;
            this.closing = true;
            Promise.resolve().then(() => this.ioClosed ? null : this.io.close()).catch(error => {
                if (!this.terminal) { this.error = error; this.terminal = -7; }
            })
                .finally(() => this.destroy());
        }

        destroy() {
            if (this.busy) { this.cancel(); return; }
            this.closed = true;
            clearTimeout(this.timer);
            if (this.handle) this.module._netPeerFileFlowDestroy(this.handle);
            if (this.memory) {
                this.module.HEAPU8.fill(0, this.memory, this.memory + 66000);
                this.module._free(this.memory);
            }
            this.handle = this.memory = 0;
            if (this.resolveCompleted) {
                this.resolveCompleted(this.terminal);
                this.resolveCompleted = null;
            }
        }

        done() {
            return this.closed && !this.busy && !this.handle;
        }

        wait() { return this.completed; }
    }

    module.NetFileTransfer = NetFileTransfer;
    // 模块唯一无状态入口；具体句柄由createClient产生，C包装仍转发原生NetSender
    const senderEntryToken = {};
    class NetSender {
        constructor(token) {
            if (token !== senderEntryToken) throw new TypeError('Use NetSender.instance().createClient(runtime, listener)');
        }
        static instance() { return senderEntry; }
        // 唯一JS执行者创建工作对象；不创建线程、不启动连接，随后prepareGateway
        createClient(runtime, listener) { return new NetSenderClient(runtime, listener); }
        // 成功后调用方丢弃client；失败保留，等待异步发送和回调收敛后重试
        destroyClient(client) { return client == null ? 0 : client.releaseGateway(); }
        prepareGateway(client) { return client.prepareGateway(); }
        connectGateway(client, options, listener) { return client.connectGateway(options, listener); }
        connectServiceGateway(client, product, options, listener) { return client.connectServiceGateway(product, options, listener); }
        configureCompression(client, types) { return client.configureCompression(types); }
        // 统一状态码：2仍运行、0已停止、负数错误，禁止用布尔值把错误当运行中
        pollGateway(client) {
            const state = senderStates.get(client);
            if (!state || !state.handle) return -7;
            const result = module._netPeerSenderClientPoll(state.handle);
            if (result < 0) client._record(result);
            return result;
        }
        beginRequest(client, product, type, timeoutMs, output) { return client.beginRequest(product, type, timeoutMs, output); }
        postRequest(client, requestId, bytes) { return client.postRequest(requestId, bytes); }
        sendRequest(client, requestId, bytes) { return client.sendRequest(requestId, bytes); }
        cancelRequest(client, requestId) { return client.cancelRequest(requestId); }
        postData(client, frame) { return client.postData(frame); }
        sendData(client, frame) { return client.sendData(frame); }
        closeGateway(client, reason = 0) { client.closeGateway(reason); }
        uninitGateway(client) { client.uninitGateway(); }
        gatewayReady(client) { return client.gatewayReady(); }
        gatewayGeneration(client) { return client.gatewayGeneration(); }
        lastGatewayError(client) { return client.lastGatewayError(); }
    }
    const senderEntry = Object.freeze(new NetSender(senderEntryToken));
    module.NetSender = NetSender;
    module.NetPeerRuntime = NetPeerRuntime;
    module.netPeerABI = Object.freeze({ pointerBytes, offsets });
});

/** 模块初始化时安装正式绑定，结构偏移均由目标编译器计算，不创建任何宿主资源。
*/
__attribute__((constructor)) static void NetPeerInitializeJsBindings()
{
    const uint32_t layout[] = {
        static_cast<uint32_t>(sizeof(NetInterfaceClientCallbacks)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientCallbacks, context)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientCallbacks, ready)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientCallbacks, result)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientCallbacks, frame)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientCallbacks, closed)),
        static_cast<uint32_t>(sizeof(NetInterfaceClientOptions)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientOptions, address)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientOptions, port)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientOptions, tls)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientOptions, platform)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientOptions, capabilities)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientOptions, maxRequests)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientOptions, maxChunk)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientOptions, handshakeTimeoutMs)),
        static_cast<uint32_t>(offsetof(NetInterfaceClientOptions, device)),
        static_cast<uint32_t>(sizeof(NetInterfaceTlsOptions)),
        static_cast<uint32_t>(offsetof(NetInterfaceTlsOptions, server)),
        static_cast<uint32_t>(offsetof(NetInterfaceTlsOptions, authority)),
        static_cast<uint32_t>(offsetof(NetInterfaceTlsOptions, authoritySize)),
        static_cast<uint32_t>(offsetof(NetInterfaceTlsOptions, certificate)),
        static_cast<uint32_t>(offsetof(NetInterfaceTlsOptions, certificateSize)),
        static_cast<uint32_t>(offsetof(NetInterfaceTlsOptions, privateKey)),
        static_cast<uint32_t>(offsetof(NetInterfaceTlsOptions, privateKeySize)),
        static_cast<uint32_t>(offsetof(NetInterfaceTlsOptions, hostname)),
        static_cast<uint32_t>(offsetof(NetInterfaceTlsOptions, entropy)),
        static_cast<uint32_t>(offsetof(NetInterfaceTlsOptions, entropyContext)),
        static_cast<uint32_t>(offsetof(NetInterfaceTlsOptions, handshakeTimeoutMs)),
        static_cast<uint32_t>(sizeof(NetInterfaceFrame)),
        static_cast<uint32_t>(offsetof(NetInterfaceFrame, m_productId)),
        static_cast<uint32_t>(offsetof(NetInterfaceFrame, m_requestId)),
        static_cast<uint32_t>(offsetof(NetInterfaceFrame, m_messageType)),
        static_cast<uint32_t>(offsetof(NetInterfaceFrame, m_kind)),
        static_cast<uint32_t>(offsetof(NetInterfaceFrame, m_payloadSize)),
        static_cast<uint32_t>(offsetof(NetInterfaceFrame, m_payload)),
        static_cast<uint32_t>(sizeof(NetHostFunctions)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, context)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, create)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, listen)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, connect)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, poll)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, write)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, pauseRead)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, close)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, shutdown)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, destroy)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, localPort)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, peerCount)),
        static_cast<uint32_t>(offsetof(NetHostFunctions, entropy))
    };
    const uintptr_t entries[] = {
        reinterpret_cast<uintptr_t>(&NetPeerJsCreate), reinterpret_cast<uintptr_t>(&NetPeerJsConnect),
        reinterpret_cast<uintptr_t>(&NetPeerJsPoll), reinterpret_cast<uintptr_t>(&NetPeerJsWrite),
        reinterpret_cast<uintptr_t>(&NetPeerJsPause), reinterpret_cast<uintptr_t>(&NetPeerJsClose),
        reinterpret_cast<uintptr_t>(&NetPeerJsShutdown), reinterpret_cast<uintptr_t>(&NetPeerJsDestroy),
        reinterpret_cast<uintptr_t>(&NetPeerJsLocalPort), reinterpret_cast<uintptr_t>(&NetPeerJsPeerCount),
        reinterpret_cast<uintptr_t>(&NetPeerJsEntropy), reinterpret_cast<uintptr_t>(&NetPeerJsReady),
        reinterpret_cast<uintptr_t>(&NetPeerJsResult), reinterpret_cast<uintptr_t>(&NetPeerJsFrame),
        reinterpret_cast<uintptr_t>(&NetPeerJsClosed)
    };
    NetPeerInstallJsBindings(layout, entries);
}
#endif