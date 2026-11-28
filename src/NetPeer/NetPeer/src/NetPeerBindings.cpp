#include "NetPeerMacro.h"
#if defined(__EMSCRIPTEN__)
#include "NetPeerBindings.h"
#include "NetSenderC.h"
#include "NetSender.h"
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

extern "C" EMSCRIPTEN_KEEPALIVE int32_t netPeerWasmFileFlowAccept(NetInterfaceFileFlowHandle handle,
    const uint32_t* fields, const uint8_t* payload)
{
    return NetSender::instance().fileProtocol().fileFlowAcceptForDispatch(handle, fields, payload);
}

// 编译期宿主入口只做ABI转发；uint64连接号独立传递，不经uintptr_t截断。
EM_JS(int32_t, NetPeerCallJsHost, (int32_t operation, uintptr_t a, uintptr_t b, uintptr_t c,
    uintptr_t d, uintptr_t e, uintptr_t f, uintptr_t g, uint64_t peer), {
    return Module.__netPeerHostCallback(operation, a, b, c, d, e, f, g, peer);
});

// Wasm的i32/i64进入JS时按有符号值交付；入口恢复无符号字段，关闭原因保留有符号值。
EM_JS(int32_t, NetPeerCallJsClient, (int32_t event, void* context, uint64_t generation,
    uint64_t request, uint32_t product, uint32_t type, int32_t status, const void* bytes, uint32_t size), {
    return Module.__netPeerClientCallback(event, context, BigInt.asUintN(64, generation),
        BigInt.asUintN(64, request), product >>> 0, type >>> 0, event === 1 ? status >>> 0 : status, bytes, size >>> 0);
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
    // uintptr_t 还可以存放不透明宿主编号或函数表索引。只有数据指针受线性内存长度约束。
    const readWord = address => pointerBytes === 8 ? view().getBigUint64(address, true) : view().getUint32(address, true);
    const putWord = (address, value) => {
        if (pointerBytes === 8) view().setBigUint64(address, BigInt(value), true);
        else view().setUint32(address, value, true);
    };
    const pointer = address => index(readWord(address));
    const putPointer = (address, value) => putWord(address, index(value));
    const u32 = (address, value) => view().setUint32(address, value, true);
    // 配置属性在调用处只读一次，仅 undefined 选用缺省值。
    const optionOr = (value, fallback) => value === undefined ? fallback : value;
    const integer = (value, min, max) => Number.isInteger(value) && value >= min && value <= max;
    const requestNumber = value => {
        if (typeof value !== 'bigint' || value < 0n || value > 0xffffffffffffffffn) throw new RangeError('uint64 BigInt is required');
        return value;
    };
    const typedArray = Object.getPrototypeOf(Uint8Array.prototype);
    const typedName = Object.getOwnPropertyDescriptor(typedArray, Symbol.toStringTag).get;
    const typedBuffer = Object.getOwnPropertyDescriptor(typedArray, 'buffer').get;
    const typedOffset = Object.getOwnPropertyDescriptor(typedArray, 'byteOffset').get;
    const typedSize = Object.getOwnPropertyDescriptor(typedArray, 'byteLength').get;
    const bufferSize = Object.getOwnPropertyDescriptor(ArrayBuffer.prototype, 'byteLength').get;
    // 内建访问器识别其他 JS 上下文的真实类型，固定视图范围也会拒绝已分离的缓冲。
    const byteView = value => typedName.call(value) === 'Uint8Array' ?
        new Uint8Array(typedBuffer.call(value), typedOffset.call(value), typedSize.call(value)) : null;
    const binary = value => {
        if (ArrayBuffer.isView(value)) return byteView(value);
        try { bufferSize.call(value); }
        catch (_) { return null; }
        return new Uint8Array(value);
    };
    const allocationFailure = Object.freeze(Object.assign(new Error('Wasm memory allocation failed'), {code: -5}));
    const allocate = size => {
        let address = 0;
        try {
            address = module._malloc(Math.max(1, size));
            if (!address) throw allocationFailure;
            module.HEAPU8.fill(0, address, address + Math.max(1, size));
            return address;
        } catch (error) {
            if (address) module._free(address);
            throw allocationFailure;
        }
    };
    const argumentError = error => error === allocationFailure ? -5 : -1;
    // 先登记清理槽，再分配模块内存。后续分配失败时仍能释放此前的全部副本。
    const allocateOwned = (owned, size) => {
        const slot = [0, size];
        owned.push(slot);
        slot[0] = allocate(size);
        return slot[0];
    };
    const wipeFree = (address, size) => {
        if (address) { module.HEAPU8.fill(0, address, address + Math.max(1, size)); module._free(address); }
    };
    const textBytes = (value, maxBytes = Infinity) => {
        if (typeof value !== 'string' || value.length > maxBytes || value.indexOf('\0') >= 0) throw new RangeError('Invalid UTF-8 input');
        const text = unescape(encodeURIComponent(value));
        if (text.length > maxBytes) throw new RangeError('Text exceeds UTF-8 byte limit');
        const bytes = new Uint8Array(text.length + 1);
        for (let i = 0; i < text.length; ++i) bytes[i] = text.charCodeAt(i);
        return bytes;
    };
    const hostAddressLimit = 4096;
    const textAt = address => {
        address = index(address);
        let text = '';
        for (let i = 0; i <= hostAddressLimit && address + i < module.HEAPU8.length; ++i) {
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
    const fixedEntries = Array.from({ length: 15 }, (ignored, i) => readWord(index(entries) + i * pointerBytes));
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
            const listener = state.handlers || state.listener;
            if (event === 0) listener.onSessionReady(generation, product, type);
            else if (event === 1) listener.onSessionResult(generation, request, product, type, status,
                module.HEAPU8.slice(index(bytes), index(bytes) + size));
            else if (event === 2) return listener.onSessionFrame(generation, frameAt(bytes)) === false ? 0 : 1;
            else listener.onSessionClosed(generation, status, size | 0);
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
        // 准备当前客户端的原生句柄和回调表；限所属执行者使用。
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
                putWord(memory + offsets.callbacks_context, context);
                ['ready', 'result', 'frame', 'closed'].forEach((name, i) =>
                    putWord(memory + offsets['callbacks_' + name], fixedEntries[hostNames.length + i]));
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
        // 借用options和listener构造一次连接申请，product为0时走直连；返回本地接纳结果。
        _connect(product, options, listener) {
            const state = senderStates.get(this);
            if (!state.handle || state.depth) return this._record(-7);
            const previousListener = state.listener, previousHandlers = state.handlers;
            let memory = 0;
            const owned = [];
            ++state.depth;
            try {
                listener = listener || previousListener;
                if (!listener) return this._record(-1);
                const handlers = {};
                for (const name of ['onSessionReady', 'onSessionResult', 'onSessionFrame', 'onSessionClosed']) {
                    const method = listener[name];
                    if (typeof method !== 'function') return this._record(-1);
                    handlers[name] = Function.prototype.bind.call(method, listener);
                }
                if (!options) return this._record(-1);
                const primaryPid = options.localGatewayPid;
                const pid = primaryPid === undefined ? optionOr(options.m_localGatewayPid, 0) : primaryPid;
                if (pid !== 0) return this._record(-1);
                const network = options.network;
                options = network === undefined ? optionOr(options.m_network, options) : network;
                if (!options) return this._record(-1);
                const deviceValue = options.device, address = options.address, port = options.port, platform = options.platform;
                const capabilities = optionOr(options.capabilities, 7), requests = optionOr(options.maxRequests, 32);
                const chunk = optionOr(options.maxChunk, 65536), timeout = optionOr(options.timeoutMs, 5000);
                if (!integer(platform, 1, 3) ||
                    !integer(port, 1, 65535) || !integer(capabilities, 0, 0xffffffff) || !integer(requests, 1, 0xffffffff) ||
                    !integer(chunk, 1024, 65536) || !integer(timeout, 100, 60000)) return this._record(-1);
                const tls = options.tls || {};
                const tlsTimeout = optionOr(tls.timeoutMs, timeout), hostname = tls.hostname;
                if (!integer(tlsTimeout, 100, 60000)) return this._record(-1);
                // 服务连接的目标由原生 NetClientDiscoveryPoint 保存；直连将地址交给字节宿主。
                const addressBytes = textBytes(address, product ? 39 : hostAddressLimit);
                if (addressBytes.length === 1) return this._record(-1);
                const hostnameBytes = hostname ? textBytes(hostname, product ? Infinity : 253) : null;
                if (!product) {
                    if (!hostnameBytes) return this._record(-1);
                    // 与原生 TLS 的证书名称一致，允许一个末尾根点，连字符仅用于标签内部。
                    const labels = (hostname.endsWith('.') ? hostname.slice(0, -1) : hostname).split('.');
                    if (labels.some(label => !/^[a-zA-Z0-9-]{1,63}$/.test(label) ||
                        label.startsWith('-') || label.endsWith('-'))) return this._record(-1);
                }
                const authority = tls.authority, certificate = tls.certificate, privateKey = tls.privateKey;
                // 所有用户 getter 已返回。先检查输入视图仍有效，随后 malloc 扩容时按模块偏移恢复借用。
                const device = binary(deviceValue);
                if (!device || device.length !== 16 || !device.some(value => value !== 0)) return this._record(-1);
                const deviceOffset = device.buffer === module.HEAPU8.buffer ? device.byteOffset : -1;
                if (!product && authority === undefined) return this._record(-1);
                const tlsInputs = [];
                for (const [name, value] of [['authority', authority], ['certificate', certificate], ['privateKey', privateKey]]) {
                    if (value !== undefined) {
                        const bytes = binary(value);
                        if (!bytes || (!product && name === 'authority' && (bytes.length === 0 || bytes.length > 131072)))
                            return this._record(-1);
                        tlsInputs.push({ name, bytes, size: bytes.length,
                            offset: bytes.buffer === module.HEAPU8.buffer ? bytes.byteOffset : -1 });
                    }
                }
                memory = allocate(offsets.optionsSize);
                const copy = bytes => { const address = allocateOwned(owned, bytes.length); module.HEAPU8.set(bytes, address); return address; };
                putPointer(memory + offsets.options_address, copy(addressBytes));
                u32(memory, offsets.optionsSize); u32(memory + 4, 1); u32(memory + offsets.options_port, port);
                u32(memory + offsets.options_platform, platform); u32(memory + offsets.options_capabilities, capabilities);
                u32(memory + offsets.options_maxRequests, requests); u32(memory + offsets.options_maxChunk, chunk);
                u32(memory + offsets.options_handshakeTimeoutMs, timeout);
                module.HEAPU8.set(deviceOffset < 0 ? device : module.HEAPU8.subarray(deviceOffset, deviceOffset + 16), memory + offsets.options_device);
                const tlsAddress = memory + offsets.options_tls;
                u32(tlsAddress + offsets.tls_handshakeTimeoutMs, tlsTimeout);
                if (hostnameBytes) putPointer(tlsAddress + offsets.tls_hostname, copy(hostnameBytes));
                for (const input of tlsInputs) {
                    const address = allocateOwned(owned, input.size);
                    module.HEAPU8.set(input.offset < 0 ? input.bytes : module.HEAPU8.subarray(input.offset, input.offset + input.size), address);
                    putPointer(tlsAddress + offsets['tls_' + input.name], address);
                    u32(tlsAddress + offsets['tls_' + input.name + 'Size'], input.size);
                }
                state.listener = listener; state.handlers = handlers;
                const result = product ? module._netPeerSenderClientConnectService(state.handle, product, 0, memory) : module._netPeerSenderClientConnect(state.handle, memory);
                if (result !== 0) { state.listener = previousListener; state.handlers = previousHandlers; }
                return this._record(result);
            } catch (error) { state.listener = previousListener; state.handlers = previousHandlers; return this._record(argumentError(error)); }
            finally { owned.forEach(item => wipeFree(item[0], item[1])); wipeFree(memory, offsets.optionsSize); --state.depth; }
        }
        // 线程：仅所属JS执行者串行操作；禁止回调中调用，变更生命周期前须等待在途send及回调收敛。
        configureCompression(types) {
            const state = senderStates.get(this);
            if (!state.handle) return this._record(-7);
            let memory = 0, byteSize = 0;
            ++state.depth;
            try {
                if (!Array.isArray(types)) return this._record(-1);
                const count = types.length;
                if (!integer(count, 0, Math.floor(0xffffffff / 4))) return this._record(-1);
                // 固定本次分配的长度；调用方的 getter 可改变数组，每项只读取一次。
                byteSize = count * 4;
                try { memory = allocate(byteSize); }
                catch (error) { return this._record(-5); }
                for (let i = 0; i < count; ++i) {
                    const value = types[i];
                    if (!integer(value, 1, 0xffffffff)) return this._record(-1);
                    u32(memory + i * 4, value);
                }
                return this._record(module._netPeerSenderClientConfigureCompression(state.handle, memory, count));
            } catch (error) { return this._record(-1); }
            finally { wipeFree(memory, byteSize); --state.depth; }
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
            const hasOutput = output !== undefined && output !== null;
            if (hasOutput && typeof output !== 'object' && typeof output !== 'function') return this._record(-1);
            let result = -7, requestId = 0n;
            if (state.handle) {
                if (!integer(product, 1, 0xffffffff) || !integer(type, 0, 0xffffffff) || !integer(timeoutMs, 1, 60000)) result = -1;
                else {
                    const scratch = state.memory + callbackStorage;
                    result = module._netPeerSenderClientBegin(state.handle, product, type, timeoutMs, scratch);
                    requestId = view().getBigUint64(scratch, true);
                }
            }
            if (hasOutput) {
                // 属性写入可进入用户 setter，返回前仍借用当前客户端。
                ++state.depth;
                try { output.requestId = requestId; }
                catch (error) {
                    // 编号未能交付给调用方，撤销本次成功的预留，归还对应的请求槽。
                    if (result === 0) module._netPeerSenderClientCancel(state.handle, requestId);
                    return this._record(-1);
                } finally { --state.depth; }
                return this._record(result);
            }
            this._record(result);
            return { result, requestId };
        }
        // 同一JS执行者按成功send／post的提交先后交付，begin仅预留；响应完成顺序由业务决定。
        // 复制入队后返回数值结果，宿主随后poll，不能跨Worker共用实例。
        // 线程：仅所属JS执行者提交，可在回调中调用；禁止跨Worker共享实例或句柄。
        postRequest(requestId, bytes) {
            const state = senderStates.get(this);
            if (!state.handle) return this._record(-7);
            let memory = 0, size = 0;
            try {
                // 已转移的 ArrayBuffer 在创建视图时抛错；有效的空正文仍须提交给原生入口。
                bytes = binary(bytes);
                if (!bytes) return this._record(-1);
                size = bytes.length;
                if (size > 1048560) return this._record(-1);
                const offset = bytes.buffer === module.HEAPU8.buffer ? bytes.byteOffset : -1;
                requestNumber(requestId); memory = allocate(size);
                // malloc可以扩容并分离旧堆视图；保存偏移后重新取视图，不额外复制普通JS输入。
                module.HEAPU8.set(offset < 0 ? bytes : module.HEAPU8.subarray(offset, offset + size), memory);
                return this._record(module._netPeerSenderClientPost(state.handle, requestId, memory, size));
            } catch (error) { return this._record(argumentError(error)); }
            finally { wipeFree(memory, size); }
        }
        // send返回Promise；await结果为0才表示交付网络宿主，绝不阻塞唯一JS线程。
        // 线程：仅所属JS执行者调用，可同时等待多个Promise；禁止在本对象协议回调中调用，不能忙等。
        async sendRequest(requestId, bytes) {
            const state = senderStates.get(this);
            if (!state.handle || state.depth) return this._record(-7);
            let memory = 0, size = 0;
            let ticket = 0n;
            try {
                bytes = binary(bytes);
                if (!bytes) return this._record(-1);
                size = bytes.length;
                if (size > 1048560) return this._record(-1);
                const offset = bytes.buffer === module.HEAPU8.buffer ? bytes.byteOffset : -1;
                requestNumber(requestId); memory = allocate(size + 8);
                module.HEAPU8.set(offset < 0 ? bytes : module.HEAPU8.subarray(offset, offset + size), memory + 8);
                const accepted = module._netPeerSenderClientSendBegin(state.handle, requestId, memory + 8, size, memory);
                if (accepted !== 0) return this._record(accepted);
                ticket = view().getBigUint64(memory, true);
            } catch (error) { return this._record(argumentError(error)); }
            finally { wipeFree(memory, size + 8); }
            return this._waitSend(ticket);
        }
        // 同一JS执行者调用；await到0才表示全部交给网络库，禁止协议回调中等待。
        // 线程：仅所属JS执行者调用，可同时等待多个Promise；禁止在本对象协议回调中调用，不能忙等。
        async sendData(frame) {
            const result = this._submitData(frame, true);
            return typeof result === 'bigint' ? this._waitSend(result) : result;
        }
        // 等待指定交付票据进入终态，两次查询之间让出事件循环。
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
        postData(frame) { return this._submitData(frame, false); }
        // 串行借用当前句柄，复制一帧。post 返回状态码，等待交付的 send 成功时返回 BigInt 票据。
        _submitData(frame, waitForDelivery) {
            const state = senderStates.get(this);
            if (!state.handle || (waitForDelivery && state.depth)) return this._record(-7);
            let memory = 0, memorySize = 0;
            ++state.depth;
            try {
                // getter 属于调用方，可能抛错或修改其它字段。每个字段只读一次，按同一组值校验和编码。
                const kind = frame.kind, product = frame.product, type = frame.type;
                const requestId = requestNumber(frame.requestId);
                const bytes = binary(frame.bytes);
                if (!bytes || !integer(kind, 0, 65535) || !integer(product, 1, 0xffffffff) ||
                    !integer(type, 0, 0xffffffff)) return this._record(-1);
                const size = bytes.length;
                if (size > 65664) return this._record(-1);
                const offset = bytes.buffer === module.HEAPU8.buffer ? bytes.byteOffset : -1;
                memorySize = offsets.frameSize + size + (waitForDelivery ? 15 : 0);
                try { memory = allocate(memorySize); }
                catch (error) { return this._record(-5); }
                u32(memory + offsets.frame_m_productId, product);
                view().setBigUint64(memory + offsets.frame_m_requestId, requestId, true);
                u32(memory + offsets.frame_m_messageType, type);
                view().setUint16(memory + offsets.frame_m_kind, kind, true);
                u32(memory + offsets.frame_m_payloadSize, size);
                putPointer(memory + offsets.frame_m_payload, memory + offsets.frameSize);
                module.HEAPU8.set(offset < 0 ? bytes : module.HEAPU8.subarray(offset, offset + size), memory + offsets.frameSize);
                if (!waitForDelivery) return this._record(module._netPeerSenderClientPostData(state.handle, memory));
                const output = memory + ((offsets.frameSize + size + 7) & ~7);
                const result = module._netPeerSenderClientSendDataBegin(state.handle, memory, output);
                return result === 0 ? view().getBigUint64(output, true) : this._record(result);
            } catch (error) { return this._record(-1); }
            finally { wipeFree(memory, memorySize); --state.depth; }
        }
        // 线程：仅所属JS执行者提交，可在回调中调用；禁止跨Worker共享实例或句柄。
        closeGateway(reason = 0) {
            const state = senderStates.get(this);
            // 关闭只推进生命周期，保留此前的准备、配置或业务提交诊断。
            if (state.handle) module._netPeerSenderClientClose(state.handle, reason);
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
        // 保存本实例的最近结果并原样返回。
        _record(result) { senderStates.get(this).error = result; return result; }
    }

    // JS宿主独占的实际缓冲池，存储按大小类复用；异步网络写完成前不归还。
    class NetHostBufferPool {
        /** 配置本执行者的实际ArrayBuffer存储上限，默认8MiB，不按消息条数计费 */
        constructor(maximum = 8388608) {
            if (!Number.isSafeInteger(maximum) || (maximum !== -1 && maximum < 32768)) throw new TypeError('Invalid host buffer pool capacity');
            this.maximum = maximum === -1 ? Infinity : maximum; this.reserved = 0;
            this.classes = [64, 1024, 16384, 32768]; this.free = this.classes.map(() => []);
        }
        /** 借出一个实际缓冲；无可用容量时返回null，分配异常交给调用边界 */
        acquire(size) {
            const at = this.classes.findIndex(value => value >= size);
            if (at < 0) return null;
            if (this.free[at].length) return this.free[at].pop();
            const capacity = this.classes[at];
            // 其他大小类的空闲缓冲可以回收，避免闲置大块阻止小块分配。
            for (let slot = 0; this.reserved + capacity > this.maximum && slot < this.free.length; ++slot) {
                while (this.free[slot].length && this.reserved + capacity > this.maximum) {
                    this.free[slot].pop(); this.reserved -= this.classes[slot];
                }
            }
            if (this.reserved + capacity > this.maximum) return null;
            const bytes = new Uint8Array(capacity); this.reserved += capacity; return bytes;
        }
        /** 归还一次真实缓冲，擦除旧内容；调用者之后不得保留可变借用 */
        release(bytes) {
            bytes.fill(0);
            const at = this.classes.indexOf(bytes.length);
            try { this.free[at].push(bytes); }
            catch (error) { this.reserved -= bytes.length; }
        }
    }
    // 每个Wasm模块唯一的JS宿主，管理字节连接、强随机池和Sender生命周期。
    class NetPeerRuntime {
        // 检查字节宿主与预算，创建当前模块的输入、输出和随机缓冲。
        constructor(options) {
            if (runtimeInstance) throw new Error('One native byte host is allowed per module');
            if (!options || typeof options.createSocket !== 'function' || typeof options.randomSource !== 'function') throw new TypeError('Raw socket and strong random hosts are required');
            this.module = module; this.options = options; this.randomSource = options.randomSource;
            // 单次宿主轮询交付的事件个数，可在创建时调节公平性。
            this.hostEventBudget = optionOr(options.hostEventBudget, 256);
            if (!integer(this.hostEventBudget, 1, 0xffffffff)) throw new RangeError('Invalid host event budget');
            // 剩余熵量低于此值时异步补充一批，单批大小由宿主随机接口确定。
            this.randomRefillBytes = optionOr(options.randomRefillBytes, 4096);
            if (!integer(this.randomRefillBytes, 1, 8192)) throw new RangeError('Invalid random refill threshold');
            // 已收到远端关闭而暂停的数据仍在队头时，允许消费者恢复的毫秒数。
            this.hostDrainTimeoutMs = optionOr(options.hostDrainTimeoutMs, 5000);
            if (!integer(this.hostDrainTimeoutMs, 1, 2147483647)) throw new RangeError('Invalid host drain timeout');
            const pools = options.hostBufferPools || {};
            this.inputPool = new NetHostBufferPool(pools.inputBytes);
            this.outputPool = new NetHostBufferPool(pools.outputBytes);
            // 当前因同模块共享输出池耗尽而等候可写通知的宿主。
            this.outputWaiters = new Set();
            this.random = new Uint8Array(0); this.randomOffset = 0; this.refilling = null; this.randomFailed = false;
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
        // 借用监听器并创建一个受当前运行时驱动的客户端。
        createSender(listener) { return NetSender.instance().createClient(this, listener); }
        // 合并同时进行的补充请求，将宿主返回的强随机字节转入本地储备。
        async refillRandom() {
            if (this.refilling) return this.refilling;
            if (this.random.length - this.randomOffset > 8192) return;
            this.refilling = Promise.resolve().then(() => this.randomSource(8192)).then(value => {
                const bytes = binary(value);
                if (!bytes || bytes.length !== 8192) throw new Error('Strong random source returned a short buffer');
                const remaining = this.random.subarray(this.randomOffset);
                const next = new Uint8Array(remaining.length + bytes.length); next.set(remaining); next.set(bytes, remaining.length);
                this.random.fill(0); bytes.fill(0); this.random = next; this.randomOffset = 0; this.randomFailed = false;
            }).catch(error => { this.randomFailed = true; throw error; }).finally(() => { this.refilling = null; });
            return this.refilling;
        }
        // 按字节向模块堆中的目标范围交付随机数据，成功后擦除已消费区域。
        copyRandom(address, size) {
            address = index(address);
            if (!integer(size, 0, 8192) || address + size > module.HEAPU8.length) return -1;
            if (size === 0) return 0;
            if (this.randomFailed || size > this.random.length - this.randomOffset) {
                module.HEAPU8.fill(0, address, address + size);
                if (!this.refilling) this.refillRandom().catch(() => {});
                return -1;
            }
            module.HEAPU8.set(this.random.subarray(this.randomOffset, this.randomOffset + size), address);
            this.random.fill(0, this.randomOffset, this.randomOffset + size); this.randomOffset += size;
            if (this.random.length - this.randomOffset < this.randomRefillBytes && !this.refilling) this.refillRandom().catch(() => {});
            return 0;
        }
        // 为属于该运行时的客户端复制种子和可选缓存路径。
        configureGateways(sender, seeds, cachePath = '') {
            const state = senderStates.get(sender);
            if (!state || state.runtime !== this) return -7;
            if (!state.handle) return sender._record(-7);
            const owned = [];
            try {
                const seedBytes = textBytes(seeds, 4096), pathBytes = textBytes(cachePath, 4096);
                const copy = bytes => { const address = allocateOwned(owned, bytes.length); module.HEAPU8.set(bytes, address); return address; };
                return sender._record(module._netPeerWasmConfigureGateways(state.handle, copy(seedBytes), copy(pathBytes)));
            } catch (error) { return sender._record(argumentError(error)); }
            finally { owned.forEach(item => wipeFree(item[0], item[1])); }
        }
        // 登记本模块的原始字节回调表，失败时回收临时表内存。
        installHost() {
            if (runtimeInstance || activeHostCallbacks) throw new Error('One native byte host is allowed per module');
            const functions = [];
            this.memory = allocate(offsets.hostSize);
            u32(this.memory, offsets.hostSize); u32(this.memory + 4, 1);
            const add = (name, fn) => {
                functions.push(fn); putWord(this.memory + offsets['host_' + name], fixedEntries[hostNames.indexOf(name)]);
            };
            const hostAt = value => this.hosts.get(Number(value));
            const notify = (host, callback, ...args) => getWasmTableEntry(pointerBytes === 4 ? callback >>> 0 : Number(callback))(
                nativePointer(host.observer), ...args);
            // 记录第一次未收敛的宿主错误，后续异常或回调不能覆盖关闭详情。
            const recordFault = (host, reason) => {
                if (!host.dead && !host.ended && !host.fault) host.fault = reason;
            };
            const queue = (host, kind, bytes, reason) => {
                if (host.dead || host.ended) return;
                if (host.fault) {
                    // 远端已经关闭时不再要求底层重复关闭，但保留首次错误。
                    if (kind === 'closed') host.ended = true;
                    return;
                }
                try {
                if (kind === 'data') {
                    bytes = binary(bytes);
                    if (!bytes) { recordFault(host, -1); return; }
                    for (let at = 0; at < bytes.length; at += 16384) {
                        const size = Math.min(16384, bytes.length - at);
                        const copy = this.inputPool.acquire(size);
                        if (!copy) { recordFault(host, -3); return; }
                        try { copy.set(bytes.subarray(at, at + size)); host.events.push({ kind, bytes: copy, size }); }
                        catch (error) { this.inputPool.release(copy); throw error; }
                    }
                } else {
                    if (kind === 'writable') {
                        host.writablePending = true;
                        // 暂停读期间可提前交付通知；同一排队标记也可承接下一次可写。
                        if (host.writableQueued) return;
                        host.writableQueued = true;
                    }
                    if (kind === 'connected') { if (host.connectedQueued) { recordFault(host, -7); return; } host.connectedQueued = true; }
                    host.events.push({ kind, reason: reason || 0 });
                    if (kind === 'closed') host.ended = true;
                }
                } catch (error) { recordFault(host, -5); }
            };
            const discardInput = host => {
                if (host.drainTimer !== null) {
                    clearTimeout(host.drainTimer);
                    host.drainTimer = null;
                }
                for (let at = host.eventHead; at < host.events.length; ++at) {
                    if (host.events[at].kind === 'data') this.inputPool.release(host.events[at].bytes);
                }
                host.events = []; host.eventHead = 0;
                host.writablePending = false; host.writableQueued = false;
            };
            const fail = (host, reason) => {
                if (host.closeReported) return;
                const closeSocket = !host.ended && !host.stopping;
                host.closeReported = true; host.ended = true; this.outputWaiters.delete(host); discardInput(host);
                if (closeSocket) { try { host.socket.close(); } catch (error) {} }
                notify(host, host.closed, 1n, reason);
            };
            try {
                add('create', (provider, observer, connected, data, writable, closed, output) => {
                    if (this.nextHost >= 0x7fffffff) return -1;
                    const host = { id: ++this.nextHost, observer: Number(observer), connected, data, writable, closed, socket: null,
                        scratch: 0, events: [], eventHead: 0, connectedQueued: false, writablePending: false, writableQueued: false,
                        drainTimer: null, drainExpired: false,
                        started: false, ready: false, paused: false, ended: false, closeReported: false, dead: false, stopping: false, polling: false, fault: 0 };
                    try {
                        host.scratch = allocate(16384);
                        host.socket = this.options.createSocket({ onConnect: () => queue(host, 'connected'), onData: bytes => queue(host, 'data', bytes),
                            onWritable: () => queue(host, 'writable'), onClose: reason => queue(host, 'closed', null, reason), onError: reason => {
                                recordFault(host, integer(reason, -2147483648, -1) ? reason : -7);
                            } });
                        if (!host.socket || typeof host.socket.connect !== 'function' || typeof host.socket.write !== 'function' || typeof host.socket.close !== 'function') throw new TypeError('Raw host is incomplete');
                        this.hosts.set(host.id, host); putWord(index(output), host.id); return 0;
                    } catch (error) { host.dead = true; if (host.socket) { try { host.socket.close(); } catch (_) {} try { if (typeof host.socket.dispose === 'function') host.socket.dispose(); } catch (_) {} } this.hosts.delete(host.id); discardInput(host); wipeFree(host.scratch, 16384); putWord(index(output), 0); return -1; }
                });
                add('connect', (instance, address, port, peer) => {
                    const host = hostAt(instance); view().setBigUint64(index(peer), 0n, true);
                    if (!host || host.started || host.stopping) return -1;
                    host.started = true;
                    try { const result = host.socket.connect(textAt(address), port); if (result < 0) return -1; view().setBigUint64(index(peer), 1n, true); return 0; }
                    catch (error) { recordFault(host, -7); return -1; }
                });
                add('poll', instance => {
                    const host = hostAt(instance);
                    if (!host || host.polling) return -1;
                    host.polling = true;
                    try {
                        if (host.fault) { fail(host, host.fault); return 0; }
                        for (let step = 0; !host.fault && step < this.hostEventBudget && host.eventHead < host.events.length; ++step) {
                            const event = host.events[host.eventHead];
                            if (event.kind === 'data' && host.paused && !host.stopping) {
                                // EOF 已是最后一个事件。期限内保持顺序，超时才丢弃无法交付的尾部。
                                if (host.ended) {
                                    if (host.drainExpired) {
                                        const eof = host.events[host.events.length - 1];
                                        fail(host, eof.reason || -6);
                                        break;
                                    } else if (host.drainTimer === null) {
                                        host.drainTimer = setTimeout(() => { host.drainExpired = true; }, this.hostDrainTimeoutMs);
                                    }
                                }
                                if (host.writablePending) {
                                    host.writablePending = false;
                                    notify(host, host.writable, 1n);
                                    continue;
                                }
                                break;
                            }
                            host.events[host.eventHead++] = null;
                            if (event.kind === 'connected') { host.ready = true; notify(host, host.connected, 1n); }
                            else if (event.kind === 'data') {
                                try {
                                    if (!host.stopping) { module.HEAPU8.set(event.bytes.subarray(0, event.size), host.scratch); notify(host, host.data, 1n, nativePointer(host.scratch), event.size); }
                                } finally { this.inputPool.release(event.bytes); }
                            } else if (event.kind === 'writable') {
                                host.writableQueued = false;
                                if (host.writablePending) { host.writablePending = false; notify(host, host.writable, 1n); }
                            }
                            else { fail(host, event.reason); break; }
                        }
                        if (host.eventHead === host.events.length) { host.events = []; host.eventHead = 0; }
                        else if (host.eventHead >= 4096 && host.eventHead >= host.events.length / 2) {
                            host.events = host.events.slice(host.eventHead); host.eventHead = 0;
                        }
                        if (host.fault) fail(host, host.fault);
                        return host.closeReported ? 0 : 1;
                    } finally { host.polling = false; }
                });
                add('write', (instance, peer, bytes, size) => {
                    const host = hostAt(instance); bytes = index(bytes);
                    if (!host || peer !== 1n || !host.ready || host.stopping || host.closeReported || size > 32768 || bytes + size > module.HEAPU8.length) return -1;
                    // JS 已登记终态，原生可能还在分轮接收先前的数据。等待 poll 交付关闭，不发起本地关闭。
                    if (host.ended || host.fault) return 1;
                    if (size === 0) return 0;
                    let copy;
                    try {
                        copy = this.outputPool.acquire(size);
                        if (!copy) { this.outputWaiters.add(host); return 1; }
                        this.outputWaiters.delete(host);
                        copy.set(module.HEAPU8.subarray(bytes, bytes + size));
                    } catch (error) { if (copy) this.outputPool.release(copy); return -1; }
                    let retained = true;
                    const completed = writable => {
                        if (!retained) return;
                        retained = false;
                        this.outputPool.release(copy);
                        for (const waiting of this.outputWaiters) queue(waiting, 'writable');
                        this.outputWaiters.clear();
                        if (writable && !host.dead && !host.closeReported) queue(host, 'writable');
                    };
                    try {
                        // Promise 完成前保留字节。同步拒绝时，等待真实的宿主可写通知。
                        const result = host.socket.write(copy.subarray(0, size));
                        if (result && typeof result.then === 'function') {
                            result.then(() => completed(true), () => { completed(false); recordFault(host, -7); });
                        } else {
                            const blocked = result === 1 || result === false;
                            const failed = typeof result === 'number' && result < 0;
                            completed(!blocked && !failed);
                            if (blocked) return 1;
                            if (failed) return host.ended || host.fault ? 1 : result;
                        }
                        return 0;
                    } catch (error) { completed(false); recordFault(host, -7); return 1; }
                });
                add('pauseRead', (instance, peer, paused) => {
                    const host = hostAt(instance); if (!host || peer !== 1n || (paused !== 0 && paused !== 1)) return -1;
                    host.paused = !!paused;
                    // EOF 后仍可恢复队尾的本地交付，已结束的底层 socket 不再切换读取状态。
                    if (!host.ended && !host.fault && typeof host.socket.pauseRead === 'function') host.socket.pauseRead(!!paused);
                    return 0;
                });
                const stop = (instance, reason) => {
                    const host = hostAt(instance); if (!host) return -1;
                    if (!host.stopping) {
                        host.stopping = true;
                        this.outputWaiters.delete(host);
                        const ending = host.events.slice(host.eventHead).filter(event => event.kind === 'closed');
                        discardInput(host); host.events = ending;
                        if (!host.ended) { try { host.socket.close(); } catch (error) { recordFault(host, -7); } }
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
                    try { if (typeof host.socket.dispose === 'function') host.socket.dispose(); } catch (error) { this.lastBindingError = error; }
                    this.hosts.delete(host.id); wipeFree(host.scratch, 16384); host.scratch = 0; discardInput(host); return 0;
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
            const wxBytes = value => {
                const bytes = binary(value);
                if (!bytes) throw new TypeError('wx byte payload must be an ArrayBuffer or Uint8Array');
                return bytes;
            };
            return {
                randomSource: length => new Promise((resolve, reject) => wx.getRandomValues({
                    length,
                    success: result => {
                        try {
                            const bytes = wxBytes(result.randomValues);
                            if (bytes.length !== length) throw new RangeError('wx random byte count mismatch');
                            resolve(bytes);
                        } catch (error) { reject(error); }
                    },
                    fail: reject
                })),
                createSocket: events => {
                    const socket = wx.createTCPSocket(); let dead = false;
                    const connected = () => { if (!dead) events.onConnect(); };
                    const data = result => {
                        if (dead) return;
                        let bytes;
                        try { bytes = wxBytes(result.message); }
                        catch (_) { events.onError(-7); return; }
                        events.onData(bytes);
                    };
                    const error = result => { if (!dead) events.onError(result); };
                    const closed = () => { if (!dead) events.onClose(0); };
                    const bindings = [['Connect', connected], ['Message', data], ['Error', error], ['Close', closed]];
                    let registered = 0;
                    const dispose = () => {
                        dead = true;
                        let firstError;
                        while (registered) {
                            const [name, callback] = bindings[--registered];
                            try { socket['off' + name](callback); }
                            catch (failure) { if (!firstError) firstError = failure; }
                        }
                        if (firstError) throw firstError;
                    };
                    try {
                        for (const [name, callback] of bindings) {
                            ++registered;
                            socket['on' + name](callback);
                        }
                    } catch (failure) {
                        try { dispose(); } catch (_) {}
                        try { socket.close(); } catch (_) {}
                        throw failure;
                    }
                    return {
                        connect: (address, port) => socket.connect({ address, port }),
                        write: value => socket.write(wxBytes(value).slice().buffer),
                        close: () => socket.close(),
                        dispose
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
        // 读取160字节文件元数据，绑定异步I/O并创建原生文件流。
        constructor(client, metadata, product, sender, io) {
            if (!integer(product, 1, 0xffffffff)) throw new RangeError('Invalid file product');
            const connection = client && client.connection;
            this.client = typeof connection === 'function' ? connection.call(client) : client;
            const runtime = this.client && this.client.runtime;
            // 句柄、临时缓冲与释放操作必须属于构造器所在的同一个 Wasm 模块。
            if (!runtime || runtime.module !== module) throw new TypeError('Use a connection from this Wasm module');
            this.module = module;
            this.io = io;
            // 宿主工作和输入会主动触发推进；此定时器负责空闲时检查文件期限。
            const pollInterval = io && io.pollIntervalMs;
            this.pollIntervalMs = pollInterval === undefined ? 25 : pollInterval;
            if (!integer(this.pollIntervalMs, 1, 2147483647)) throw new RangeError('Invalid file poll interval');
            this.sender = !!sender;
            this.handle = 0;
            this.memory = 0;
            this.busy = false;
            this.closeStarted = false;
            this.closed = false;
            this.terminal = null;
            this.error = null;
            this.timer = null;
            this.pollQueued = false;
            this.primeOffset = 0;
            this.completed = new Promise(resolve => { this.resolveCompleted = resolve; });
            metadata = byteView(metadata);
            if (!metadata || metadata.length !== 160 || !io || typeof io.read !== 'function' ||
                typeof io.close !== 'function' || (!sender && (typeof io.write !== 'function' || typeof io.flush !== 'function' ||
                typeof io.commit !== 'function'))) throw new Error('File host adapter is incomplete');
            try {
                const metadataOffset = metadata.buffer === this.module.HEAPU8.buffer ? metadata.byteOffset : -1;
                this.memory = allocate(66000);
                this.module.HEAPU8.set(metadataOffset < 0 ? metadata : this.module.HEAPU8.subarray(metadataOffset, metadataOffset + 160), this.memory + 128);
                this.check(this.module._netPeerFileInfoFields(this.memory + 128, 160, this.memory,
                    this.memory + 48, this.memory + 64, this.memory + 96));
                const fields = this.module.HEAPU32.slice(this.memory / 4, (this.memory + 48) / 4);
                this.resume = this.safeOffset(fields[6], fields[7]);
                this.stream = [fields[8], fields[9]];
                this.product = product;
                const now = this.time();
                const timeout = optionOr(io.progressTimeoutMs, 15000);
                if (!integer(timeout, 1000, 60000)) throw new RangeError('Invalid file progress timeout');
                this.progressTimeoutMs = timeout;
                this.finalOutputSince = null;
                this.check(this.module._netPeerFileFlowCreateWire(this.memory + 128, 160, product, sender ? 1 : 0,
                    now[0], now[1], timeout, this.memory));
                this.handle = readPointer(this.module, this.memory);
                // The host write budget and negotiated window jointly bound durable batches.
                this.batchWrites = false;
                // 接收端每次宿主写入的字节上限。-1允许完整协议窗口，缺省一次写一个协议块。
                this.maxWriteBytes = fields[2];
                const maximum = sender ? undefined : io.maxWriteBytes;
                if (maximum !== undefined) {
                    if (maximum === -1) this.maxWriteBytes = fields[2] * fields[3];
                    else if (Number.isSafeInteger(maximum) && maximum > 0) this.maxWriteBytes = maximum;
                    else throw new RangeError('Invalid maximum file write size');
                }
                const batch = Math.min(fields[3], Math.floor(this.maxWriteBytes / fields[2]));
                if (!sender && batch > 1 && typeof this.module._netPeerFileFlowSetWriteBatchChunks === 'function') {
                    this.check(this.module._netPeerFileFlowSetWriteBatchChunks(this.handle, batch));
                    this.batchWrites = true;
                }
            } catch (error) {
                this.destroy();
                throw error;
            }
        }

        // 负数结果转成带原始错误码的异常，成功结果原样返回。
        check(result) {
            if (result < 0) {
                const error = new Error('File operation failed: ' + result);
                error.code = result;
                throw error;
            }
            return result;
        }

        // 将宿主的非负单调毫秒拆成两个32位数传给C接口。
        time() {
            const now = Math.floor(this.client.options.clock());
            if (!Number.isSafeInteger(now) || now < 0) throw new Error('Invalid monotonic file clock');
            return [now >>> 0, Math.floor(now / 4294967296)];
        }

        // 将文件偏移的低高位重组为宿主可精确表示的整数。
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
            const bytes = byteView(frame.payload);
            if (!bytes || bytes.length > 65624) throw new Error('File frame exceeds budget');
            this.module.HEAPU32.set([frame.kind, frame.product, frame.requestLow, frame.requestHigh, frame.type, bytes.length], this.memory / 4);
            this.module.HEAPU8.set(bytes, this.memory + 320);
            const result = this.module._netPeerWasmFileFlowAccept(this.handle, this.memory, this.memory + 320);
            // 原生协议已成功，但宿主可能仍在等待最终 ACK 可写或文件关闭。消费同流迟到帧。
            // 这里的 NOT_FOUND 也可能是失败原因；仅成功终态可忽略，终态查询不推进时钟。
            if (result === -8 && this.module._netPeerFileFlowProgress(this.handle, 0, 0) === 2) return true;
            if (result === -3) return false;
            if (result === 2) {
                // 对端已经终止此流。终态查询不会推进时钟，也无需发回取消。
                this.terminal = this.module._netPeerFileFlowProgress(this.handle, 0, 0);
                this.finish();
                return true;
            }
            this.check(result);
            // Drain this socket delivery before crossing the host file bridge.
            // A timer can be clamped to one second in the simulator/background;
            // partial windows must also progress without waiting for that timer.
            if (this.batchWrites) this.queuePoll();
            else this.poll();
            return true;
        }

        // 合并同一轮的推进请求，交给微任务执行。
        queuePoll() {
            if (this.closed || this.pollQueued) return;
            this.pollQueued = true;
            Promise.resolve().then(() => { this.pollQueued = false; this.poll(); });
        }

        // 安排下一次文件流期限检查。
        schedule() {
            if (!this.closed && this.timer === null) this.timer = setTimeout(() => { this.timer = null; this.poll(); }, this.pollIntervalMs);
        }

        // 串行发起宿主异步工作，成功时把结果交给完成函数。
        job(work, completed) {
            this.busy = true;
            Promise.resolve().then(work).then(value => {
                if (!this.closed) completed(value);
            }).catch(error => { this.cancel(this.ioFailure(error)); }).finally(() => {
                this.busy = false;
                if (this.closed) { this.closeFinished(); return; }
                try { this.client.schedulePump(); }
                catch (error) { this.cancel(this.ioFailure(error)); return; }
                this.poll();
            });
        }

        // 宿主可用任意值拒绝 Promise；仅保留可由 C ABI 表示的负错误码。
        ioFailure(error) {
            this.error = error;
            // 拒绝值的属性也可能由宿主 getter 或 Proxy 实现，不能让错误转换再次抛出。
            let code;
            try { code = typeof error === 'number' ? error : error && error.code; }
            catch (_) { return -7; }
            return integer(code, -2147483648, -1) ? code : -7;
        }

        // 推进当前流的读写、协议输出、提交及关闭。
        poll() {
            if (this.closed || !this.handle) return;
            try {
                const now = this.time();
                const progress = this.module._netPeerFileFlowProgress(this.handle, now[0], now[1]);
                this.check(progress);
                const committed = progress === 2 && !this.sender;
                if (committed) {
                    // 协议已提交，仍需在有限时间内交付最终 ACK。
                    const current = this.safeOffset(now[0], now[1]);
                    if (this.finalOutputSince === null) this.finalOutputSince = current;
                    if (current - this.finalOutputSince >= this.progressTimeoutMs) { this.cancel(-9); return; }
                }
                if (!this.busy && this.primeOffset < this.resume) {
                    const offset = this.primeOffset;
                    const size = Math.min(65536, this.resume - offset);
                    this.job(() => this.io.read(offset, size), bytes => {
                        bytes = byteView(bytes);
                        if (!bytes || bytes.length !== size) throw new Error('Short file prefix');
                        this.module.HEAPU8.set(bytes, this.memory + 320);
                        this.check(this.module._netPeerFileFlowPrime(this.handle, offset >>> 0, Math.floor(offset / 4294967296),
                            this.memory + 320, size));
                        this.primeOffset += size;
                    });
                } else if (!this.busy && this.sender) {
                    const result = this.module._netPeerFileFlowRead(this.handle, this.memory);
                    this.check(result);
                    if (result === 2) {
                        const fields = this.module.HEAPU32.slice(this.memory / 4, (this.memory + 12) / 4);
                        const offset = this.safeOffset(fields[0], fields[1]);
                        this.job(() => this.io.read(offset, fields[2]), bytes => {
                            bytes = byteView(bytes);
                            if (!bytes || bytes.length !== fields[2]) throw new Error('Short file block');
                            this.module.HEAPU8.set(bytes, this.memory + 320);
                            this.check(this.module._netPeerFileFlowProduced(this.handle, fields[0], fields[1], this.memory + 320, bytes.length));
                        });
                    }
                } else if (!this.busy) {
                    const written = this.module._netPeerFileFlowWrite(this.handle, this.memory, this.memory + 24);
                    this.check(written);
                    if (written === 2) {
                        const fields = this.module.HEAPU32.slice(this.memory / 4, (this.memory + 12) / 4);
                        const pointer = readPointer(this.module, this.memory + 24);
                        const bytes = this.module.HEAPU8.slice(pointer, pointer + fields[2]);
                        const offset = this.safeOffset(fields[0], fields[1]);
                        this.job(async () => {
                            try {
                                for (let at = 0; at < bytes.length && !this.closed; at += this.maxWriteBytes) {
                                    await this.io.write(offset + at, bytes.subarray(at, at + this.maxWriteBytes));
                                }
                                if (!this.closed) await this.io.flush();
                            } finally { bytes.fill(0); }
                        }, () => {
                            this.check(this.module._netPeerFileFlowWritten(this.handle, fields[0], fields[1], fields[2], 0));
                        });
                    } else if (this.module._netPeerFileFlowTakeCommit(this.handle) === 2) {
                        this.job(async () => {
                            await this.io.flush();
                            if (this.closed) return;
                            this.closeStarted = true;
                            await this.io.close();
                            if (this.closed) return;
                            await this.io.commit();
                        }, () => {
                            this.check(this.module._netPeerFileFlowCommitted(this.handle, 0));
                        });
                    }
                }
                let output = this.module._netPeerFileFlowPeek(this.handle, this.memory, this.memory + 24);
                if (output === 2) {
                    const fields = Array.from(this.module.HEAPU32.slice(this.memory / 4, (this.memory + 24) / 4));
                    const pointer = readPointer(this.module, this.memory + 24);
                    const bytes = this.module.HEAPU8.slice(pointer, pointer + fields[5]);
                    try {
                        this.client.sendFrame(fields.slice(0, 5), bytes);
                        this.check(this.module._netPeerFileFlowConsume(this.handle));
                        // 旧的累计 ACK 可能早于提交就已借出。消费后由协议层确认还有无最终 ACK。
                        if (committed) output = this.module._netPeerFileFlowPeek(this.handle, this.memory, this.memory + 24);
                    } catch (error) {
                        if (!error || error.code !== -3) throw error;
                    } finally { bytes.fill(0); }
                }
                // 已提交文件的 ACK 分配失败可重试；其余阶段保留各自的错误。
                if (!(committed && output === -5)) this.check(output);
                if (output === 1 && progress === 2 && !this.busy) {
                    this.terminal = 0;
                    this.finish();
                    return;
                }
                this.schedule();
            } catch (error) { this.cancel(this.ioFailure(error)); }
        }

        // 取消本流并保留终态，继续收敛已提交的宿主工作。
        cancel(reason = -10) {
            if (this.closed) return;
            this.terminal = integer(reason, -2147483648, -1) ? reason : -10;
            if (this.handle) this.module._netPeerFileFlowCancel(this.handle, this.terminal);
            // 先进入关闭流程，宿主同步重入时保留第一次取消的原因。
            this.finish();
            try {
                if (!this.stream || !this.client.ready || this.client.disposed) return;
            } catch (notificationError) {
                // 无法确认连接代次仍可用，保留本地终态，不向这条连接发送或报告故障。
                if (!this.error) this.error = notificationError;
                return;
            }
            try { this.client.sendFrame([6, this.product, this.stream[0], this.stream[1], 60005], new Uint8Array(0)); }
            catch (_) {
                try {
                    const fail = this.client.fail;
                    if (typeof fail === 'function') fail.call(this.client, new Error('File cancellation could not be admitted'));
                } catch (notificationError) {
                    if (!this.error) this.error = notificationError;
                }
            }
        }

        // 停止定时推进，已在途的宿主工作完成后关闭文件。
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

        // 等待文件关闭操作结束，然后释放协议流。
        closeFinished() {
            if (this.busy || this.closing) return;
            this.closing = true;
            Promise.resolve().then(() => this.closeStarted ? null : this.io.close()).catch(error => {
                if (!this.terminal) this.terminal = this.ioFailure(error);
            })
                .finally(() => { this.closing = false; this.destroy(); });
        }

        // 宿主工作或关闭尚未完成时延后释放，最终擦除暂存区并通知等待者。
        destroy() {
            if (this.closing) return;
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

        // 检查文件流是否已停止并释放，且没有在途工作。
        done() {
            return this.closed && !this.busy && !this.handle;
        }

        // 返回等待文件流终态的Promise。
        wait() { return this.completed; }
    }

    module.NetFileTransfer = NetFileTransfer;
    // 模块唯一无状态入口；具体句柄由createClient产生，C包装仍转发原生NetSender
    const senderEntryToken = {};
    class NetSender {
        // 仅允许由instance使用内部令牌创建统一入口。
        constructor(token) {
            if (token !== senderEntryToken) throw new TypeError('Use NetSender.instance().createClient(runtime, listener)');
        }
        // 取得当前模块的统一发送入口。
        static instance() { return senderEntry; }
        // 唯一JS执行者创建工作对象；不创建线程、不启动连接，随后prepareGateway
        createClient(runtime, listener) { return new NetSenderClient(runtime, listener); }
        // 成功后调用方丢弃client；失败保留，等待异步发送和回调收敛后重试
        destroyClient(client) { return client == null ? 0 : client.releaseGateway(); }
        // 准备该客户端的原生句柄与回调。
        prepareGateway(client) { return client.prepareGateway(); }
        // 用options指定的身份和地址建立直连。
        connectGateway(client, options, listener) { return client.connectGateway(options, listener); }
        // 经网关连接指定product的业务服务。
        connectServiceGateway(client, product, options, listener) { return client.connectServiceGateway(product, options, listener); }
        // 设置该客户端允许协商压缩的公开消息类型。
        configureCompression(client, types) { return client.configureCompression(types); }
        // 统一状态码：2仍运行、0已停止、负数错误，禁止用布尔值把错误当运行中
        pollGateway(client) {
            const state = senderStates.get(client);
            if (!state) return -1;
            if (!state.handle) return 0;
            const result = module._netPeerSenderClientPoll(state.handle);
            if (result < 0) client._record(result);
            return result;
        }
        // 预留请求并将编号写入output，期限从预留开始。
        beginRequest(client, product, type, timeoutMs, output) { return client.beginRequest(product, type, timeoutMs, output); }
        // 复制正文并提交已经预留的请求。
        postRequest(client, requestId, bytes) { return client.postRequest(requestId, bytes); }
        // 返回等待此请求交付给网络宿主的Promise。
        sendRequest(client, requestId, bytes) { return client.sendRequest(requestId, bytes); }
        // 取消指定客户端的预留请求。
        cancelRequest(client, requestId) { return client.cancelRequest(requestId); }
        // 复制并接纳已授权的文件或数据帧。
        postData(client, frame) { return client.postData(frame); }
        // 返回等待该数据帧交付给网络宿主的Promise。
        sendData(client, frame) { return client.sendData(frame); }
        // 请求关闭连接并记录reason，宿主继续驱动收敛。
        closeGateway(client, reason = 0) { client.closeGateway(reason); }
        // 启动停止流程，之后释放客户端。
        uninitGateway(client) { client.uninitGateway(); }
        // 读取当前客户端是否可提交业务。
        gatewayReady(client) { return client.gatewayReady(); }
        // 读取客户端当前代次，用于识别陈旧回调。
        gatewayGeneration(client) { return client.gatewayGeneration(); }
        // 读取当前客户端最近保存的结果码。
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