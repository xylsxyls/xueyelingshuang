#include "NetProtocolTestBindings.h"
#if defined(__EMSCRIPTEN__)

int32_t netTestInterfaceConnectionConfigureCompression(NetInterfaceConnectionHandle handle,
        const uint32_t* types, uint32_t count, int32_t negotiated)
{
    return NetInterface::connectionConfigureCompression(handle, types, count, negotiated);
}

int32_t netTestInterfaceConnectionConsume(NetInterfaceConnectionHandle handle)
{
    return NetInterface::connectionConsume(handle);
}

int32_t netTestInterfaceConnectionConsumeOutput(NetInterfaceConnectionHandle handle, uint32_t size)
{
    return NetInterface::connectionConsumeOutput(handle, size);
}

void netTestInterfaceConnectionDestroy(NetInterfaceConnectionHandle handle)
{
    NetInterface::connectionDestroy(handle);
}

int32_t netTestInterfaceConnectionFeed(NetInterfaceConnectionHandle handle, const uint8_t* data, uint32_t size,
                                                       uint32_t* consumed)
{
    return NetInterface::connectionFeed(handle, data, size, consumed);
}

int32_t netTestInterfaceConnectionFinishInput(NetInterfaceConnectionHandle handle)
{
    return NetInterface::connectionFinishInput(handle);
}

int32_t netTestInterfaceConnectionGetStatus(NetInterfaceConnectionHandle handle, int32_t* detail)
{
    return NetInterface::connectionGetStatus(handle, detail);
}

int32_t netTestInterfaceConnectionPeek(NetInterfaceConnectionHandle handle, uint32_t* fields, const uint8_t** payload)
{
    return NetInterface::connectionPeek(handle, fields, payload);
}

int32_t netTestInterfaceConnectionPeekOutput(NetInterfaceConnectionHandle handle, const uint8_t** data, uint32_t* size)
{
    return NetInterface::connectionPeekOutput(handle, data, size);
}

int32_t netTestInterfaceConnectionProgress(NetInterfaceConnectionHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    return NetInterface::connectionProgress(handle, timeLow, timeHigh);
}

int32_t netTestInterfaceConnectionSend(NetInterfaceConnectionHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    return NetInterface::connectionSend(handle, fields, payload);
}

int32_t netTestInterfaceCreditConfigureMessage(NetInterfaceCreditHandle handle, const uint8_t* input, uint32_t size)
{
    return NetInterface::creditConfigureMessage(handle, input, size);
}

int32_t netTestInterfaceCreditConsume(NetInterfaceCreditHandle handle, uint32_t size)
{
    return NetInterface::creditConsume(handle, size);
}

int32_t netTestInterfaceCreditCreate(NetInterfaceCreditHandle* handle)
{
    return NetInterface::creditCreate(handle);
}

void netTestInterfaceCreditDestroy(NetInterfaceCreditHandle handle)
{
    NetInterface::creditDestroy(handle);
}

int32_t netTestInterfaceCreditGet(NetInterfaceCreditHandle handle, uint32_t* fields)
{
    return NetInterface::creditGet(handle, fields);
}

int32_t netTestInterfaceCreditGrant(NetInterfaceCreditHandle handle, uint32_t size)
{
    return NetInterface::creditGrant(handle, size);
}

int32_t netTestInterfaceCreditReceived(NetInterfaceCreditHandle handle, uint32_t size)
{
    return NetInterface::creditReceived(handle, size);
}

int32_t netTestInterfaceCreditSent(NetInterfaceCreditHandle handle, uint32_t size)
{
    return NetInterface::creditSent(handle, size);
}

int32_t netTestInterfaceDirectoryCache(NetInterfaceDirectoryHandle handle, uint32_t utcLow, uint32_t utcHigh,
        uint8_t* output, uint32_t capacity, uint32_t* required)
{
    return NetInterface::directoryCache(handle, utcLow, utcHigh, output, capacity, required);
}

int32_t netTestInterfaceDirectoryCount(NetInterfaceDirectoryHandle handle, uint32_t* count)
{
    return NetInterface::directoryCount(handle, count);
}

int32_t netTestInterfaceDirectoryCreate(const uint8_t* data, uint32_t size, NetInterfaceDirectoryHandle* handle)
{
    return NetInterface::directoryCreate(data, size, handle);
}

int32_t netTestInterfaceDirectoryCreateCache(const uint8_t* data, uint32_t size, uint32_t utcLow,
        uint32_t utcHigh, NetInterfaceDirectoryHandle* handle)
{
    return NetInterface::directoryCreateCache(data, size, utcLow, utcHigh, handle);
}

void netTestInterfaceDirectoryDestroy(NetInterfaceDirectoryHandle handle)
{
    NetInterface::directoryDestroy(handle);
}

int32_t netTestInterfaceDirectoryNode(NetInterfaceDirectoryHandle handle, uint32_t index,
        char* name, uint32_t nameCapacity, char* address, uint32_t addressCapacity, uint32_t* fields)
{
    return NetInterface::directoryNode(handle, index, name, nameCapacity, address, addressCapacity, fields);
}

int32_t netTestInterfaceFileFlowAccept(NetInterfaceFileFlowHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    return NetInterface::fileFlowAccept(handle, fields, payload);
}

int32_t netTestInterfaceFileFlowCancel(NetInterfaceFileFlowHandle handle, int32_t reason)
{
    return NetInterface::fileFlowCancel(handle, reason);
}

int32_t netTestInterfaceFileFlowCommitted(NetInterfaceFileFlowHandle handle, int32_t result)
{
    return NetInterface::fileFlowCommitted(handle, result);
}

int32_t netTestInterfaceFileFlowConsume(NetInterfaceFileFlowHandle handle)
{
    return NetInterface::fileFlowConsume(handle);
}

int32_t netTestInterfaceFileFlowCreateWire(const uint8_t* bytes, uint32_t size, uint32_t product, uint32_t sender,
                                                           uint32_t timeLow, uint32_t timeHigh, uint32_t timeoutMs,
                                                           NetInterfaceFileFlowHandle* handle)
{
    return NetInterface::fileFlowCreateWire(bytes, size, product, sender, timeLow, timeHigh, timeoutMs, handle);
}

void netTestInterfaceFileFlowDestroy(NetInterfaceFileFlowHandle handle)
{
    NetInterface::fileFlowDestroy(handle);
}

int32_t netTestInterfaceFileFlowPeek(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** payload)
{
    return NetInterface::fileFlowPeek(handle, fields, payload);
}

int32_t netTestInterfaceFileFlowPrime(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
                                                      const uint8_t* bytes, uint32_t size)
{
    return NetInterface::fileFlowPrime(handle, offsetLow, offsetHigh, bytes, size);
}

int32_t netTestInterfaceFileFlowProduced(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
                                                         const uint8_t* bytes, uint32_t size)
{
    return NetInterface::fileFlowProduced(handle, offsetLow, offsetHigh, bytes, size);
}

int32_t netTestInterfaceFileFlowProgress(NetInterfaceFileFlowHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    return NetInterface::fileFlowProgress(handle, timeLow, timeHigh);
}

int32_t netTestInterfaceFileFlowRead(NetInterfaceFileFlowHandle handle, uint32_t* fields)
{
    return NetInterface::fileFlowRead(handle, fields);
}

int32_t netTestInterfaceFileFlowSetWriteBatchChunks(NetInterfaceFileFlowHandle handle, uint32_t chunks)
{
    return NetInterface::fileFlowSetWriteBatchChunks(handle, chunks);
}

int32_t netTestInterfaceFileFlowTakeCommit(NetInterfaceFileFlowHandle handle)
{
    return NetInterface::fileFlowTakeCommit(handle);
}

int32_t netTestInterfaceFileFlowWrite(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** bytes)
{
    return NetInterface::fileFlowWrite(handle, fields, bytes);
}

int32_t netTestInterfaceFileFlowWritten(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
                                                        uint32_t size, int32_t result)
{
    return NetInterface::fileFlowWritten(handle, offsetLow, offsetHigh, size, result);
}

int32_t netTestInterfaceFileInfoFields(const uint8_t* bytes, uint32_t size, uint32_t* fields, uint8_t* transfer,
                                                       uint8_t* token, uint8_t* hash)
{
    return NetInterface::fileInfoFields(bytes, size, fields, transfer, token, hash);
}

int32_t netTestInterfaceHandshakeAccept(NetInterfaceHandshakeHandle handle, const uint32_t* fields, const uint8_t* payload,
                                                        uint32_t timeLow, uint32_t timeHigh)
{
    return NetInterface::handshakeAccept(handle, fields, payload, timeLow, timeHigh);
}

int32_t netTestInterfaceHandshakeConsumeOutput(NetInterfaceHandshakeHandle handle)
{
    return NetInterface::handshakeConsumeOutput(handle);
}

int32_t netTestInterfaceHandshakeCreate(const uint32_t* options, uint32_t optionCount, const uint8_t* identities,
                                                        uint32_t identitySize, uint32_t timeLow, uint32_t timeHigh,
                                                        NetInterfaceHandshakeHandle* handle)
{
    return NetInterface::handshakeCreate(options, optionCount, identities, identitySize, timeLow, timeHigh, handle);
}

void netTestInterfaceHandshakeDestroy(NetInterfaceHandshakeHandle handle)
{
    NetInterface::handshakeDestroy(handle);
}

int32_t netTestInterfaceHandshakePeekOutput(NetInterfaceHandshakeHandle handle, uint32_t* fields, const uint8_t** payload)
{
    return NetInterface::handshakePeekOutput(handle, fields, payload);
}

int32_t netTestInterfaceHandshakePeerInfo(NetInterfaceHandshakeHandle handle, uint32_t* fields, uint8_t* identities)
{
    return NetInterface::handshakePeerInfo(handle, fields, identities);
}

int32_t netTestInterfaceHandshakeProgress(NetInterfaceHandshakeHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    return NetInterface::handshakeProgress(handle, timeLow, timeHigh);
}

int32_t netTestInterfaceLinkAccept(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    return NetInterface::linkAccept(handle, fields, payload);
}

int32_t netTestInterfaceLinkConsumeControl(NetInterfaceLinkHandle handle)
{
    return NetInterface::linkConsumeControl(handle);
}

int32_t netTestInterfaceLinkConsumeOutput(NetInterfaceLinkHandle handle)
{
    return NetInterface::linkConsumeOutput(handle);
}

int32_t netTestInterfaceLinkConsumeRecord(NetInterfaceLinkHandle handle)
{
    return NetInterface::linkConsumeRecord(handle);
}

int32_t netTestInterfaceLinkCreate(const NetInterfaceLinkOptions* options, NetInterfaceLinkHandle* handle)
{
    return NetInterface::linkCreate(options, handle);
}

void netTestInterfaceLinkDestroy(NetInterfaceLinkHandle handle)
{
    NetInterface::linkDestroy(handle);
}

int32_t netTestInterfaceLinkPeekControl(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload)
{
    return NetInterface::linkPeekControl(handle, fields, payload);
}

int32_t netTestInterfaceLinkPeekOutput(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload)
{
    return NetInterface::linkPeekOutput(handle, fields, payload);
}

int32_t netTestInterfaceLinkPeekRecord(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload)
{
    return NetInterface::linkPeekRecord(handle, fields, payload);
}

int32_t netTestInterfaceLinkProgress(NetInterfaceLinkHandle handle, uint32_t low, uint32_t high)
{
    return NetInterface::linkProgress(handle, low, high);
}

int32_t netTestInterfaceLinkSendControl(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    return NetInterface::linkSendControl(handle, fields, payload);
}

int32_t netTestInterfaceLinkSendRecord(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    return NetInterface::linkSendRecord(handle, fields, payload);
}

int32_t netTestInterfaceLinkStatus(NetInterfaceLinkHandle handle)
{
    return NetInterface::linkStatus(handle);
}

int32_t netTestInterfaceMessageClear(NetInterfaceMessageHandle handle)
{
    return NetInterface::messageClear(handle);
}

int32_t netTestInterfaceMessageCreate(NetInterfaceMessageHandle* handle)
{
    return NetInterface::messageCreate(handle);
}

void netTestInterfaceMessageDestroy(NetInterfaceMessageHandle handle)
{
    NetInterface::messageDestroy(handle);
}

int32_t netTestInterfaceMessageEncode(NetInterfaceMessageHandle handle, uint8_t* output, uint32_t capacity,
                                                      uint32_t* required)
{
    return NetInterface::messageEncode(handle, output, capacity, required);
}

int32_t netTestInterfaceMessageGetCode(NetInterfaceMessageHandle handle, int32_t* code, uint8_t* error, uint32_t capacity,
                                                       uint32_t* required)
{
    return NetInterface::messageGetCode(handle, code, error, capacity, required);
}

int32_t netTestInterfaceMessageGetValue(NetInterfaceMessageHandle handle, int32_t group, int32_t key, int32_t* type,
                                                        uint8_t* output, uint32_t capacity, uint32_t* required)
{
    return NetInterface::messageGetValue(handle, group, key, type, output, capacity, required);
}

int32_t netTestInterfaceMessageParse(NetInterfaceMessageHandle handle, const uint8_t* input, uint32_t size)
{
    return NetInterface::messageParse(handle, input, size);
}

int32_t netTestInterfaceMessageSetCode(NetInterfaceMessageHandle handle, int32_t code, const uint8_t* error, uint32_t size)
{
    return NetInterface::messageSetCode(handle, code, error, size);
}

int32_t netTestInterfaceMessageSetValue(NetInterfaceMessageHandle handle, int32_t group, int32_t key, int32_t type,
                                                        const uint8_t* value, uint32_t size)
{
    return NetInterface::messageSetValue(handle, group, key, type, value, size);
}

int32_t netTestInterfaceRequestsAddEndpoint(NetInterfaceRequestsHandle handle, uint32_t low, uint32_t high,
                                                            uint32_t service, uint32_t product, uint32_t limit, uint32_t internalOnly)
{
    return NetInterface::requestsAddEndpoint(handle, low, high, service, product, limit, internalOnly);
}

int32_t netTestInterfaceRequestsBegin(NetInterfaceRequestsHandle handle, const uint32_t* input, uint32_t inputCount,
                                                      uint32_t autoId, uint32_t* output)
{
    return NetInterface::requestsBegin(handle, input, inputCount, autoId, output);
}

int32_t netTestInterfaceRequestsCancel(NetInterfaceRequestsHandle handle, uint32_t sourceLow, uint32_t sourceHigh,
                                                       uint32_t originalLow, uint32_t originalHigh, uint32_t* output)
{
    return NetInterface::requestsCancel(handle, sourceLow, sourceHigh, originalLow, originalHigh, output);
}

int32_t netTestInterfaceRequestsComplete(NetInterfaceRequestsHandle handle, const uint32_t* input, uint32_t inputCount,
                                                         uint32_t* output)
{
    return NetInterface::requestsComplete(handle, input, inputCount, output);
}

int32_t netTestInterfaceRequestsCount(NetInterfaceRequestsHandle handle)
{
    return NetInterface::requestsCount(handle);
}

int32_t netTestInterfaceRequestsCreate(uint32_t capacity, NetInterfaceRequestsHandle* handle)
{
    return NetInterface::requestsCreate(capacity, handle);
}

void netTestInterfaceRequestsDestroy(NetInterfaceRequestsHandle handle)
{
    NetInterface::requestsDestroy(handle);
}

int32_t netTestInterfaceRequestsTake(NetInterfaceRequestsHandle handle, uint32_t operation, uint32_t keyLow,
                                                     uint32_t keyHigh, uint32_t* output)
{
    return NetInterface::requestsTake(handle, operation, keyLow, keyHigh, output);
}

int32_t netTestInterfaceRpcControl(const uint32_t* fields, uint32_t count, uint32_t* reply)
{
    return NetInterface::rpcControl(fields, count, reply);
}

int32_t netTestInterfaceRpcEncode(uint32_t kind, uint32_t value, uint32_t parentLow, uint32_t parentHigh,
                                                  const uint8_t* data, uint32_t size, uint8_t* output, uint32_t capacity,
                                                  uint32_t* required)
{
    return NetInterface::rpcEncode(kind, value, parentLow, parentHigh, data, size, output, capacity, required);
}

int32_t netTestInterfaceRpcParse(const uint32_t* frameFields, uint32_t count, const uint8_t* data, uint32_t* fields)
{
    return NetInterface::rpcParse(frameFields, count, data, fields);
}

int32_t netTestInterfaceTunnelConsume(NetInterfaceTunnelHandle handle)
{
    return NetInterface::tunnelConsume(handle);
}

int32_t netTestInterfaceTunnelCreate(NetInterfaceTunnelHandle* handle)
{
    return NetInterface::tunnelCreate(handle);
}

void netTestInterfaceTunnelDestroy(NetInterfaceTunnelHandle handle)
{
    NetInterface::tunnelDestroy(handle);
}

int32_t netTestInterfaceTunnelEncode(const uint32_t* fields, const uint8_t* payload,
    uint8_t* output, uint32_t capacity, uint32_t* required)
{
    return NetInterface::tunnelEncode(fields, payload, output, capacity, required);
}

int32_t netTestInterfaceTunnelFeed(NetInterfaceTunnelHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed)
{
    return NetInterface::tunnelFeed(handle, data, size, consumed);
}

int32_t netTestInterfaceTunnelPeek(NetInterfaceTunnelHandle handle, uint32_t* fields, const uint8_t** payload)
{
    return NetInterface::tunnelPeek(handle, fields, payload);
}

// 强随机由测试宿主提供；只做ABI转换，不伪造TLS成功或重写任何协议算法。
EM_JS(int32_t, NetProtocolTestEntropy, (uint8_t* output, uint32_t size), {
    return typeof Module.netProtocolTestEntropy === 'function' ? Module.netProtocolTestEntropy(output, size) : -1;
});

/** 适配NetInterface的随机回调签名
@param [in] context 不使用的测试上下文
@param [out] output 待填入的真实随机字节
@param [in] size 字节数
@return 0成功或宿主错误
*/
static int32_t ProtocolTestEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return NetProtocolTestEntropy(output, size);
}

int32_t netTestInterfaceWasmConnectionCreate(const uint8_t* authority,
    uint32_t authoritySize, const char* hostname, uint32_t timeoutMs,
    NetInterfaceConnectionHandle* handle, int32_t* detail)
{
    NetInterfaceTlsOptions options = {};
    options.authority = authority;
    options.authoritySize = authoritySize;
    options.hostname = hostname;
    options.handshakeTimeoutMs = timeoutMs;
    options.entropy = ProtocolTestEntropy;
    return NetInterface::connectionCreate(&options, handle, detail);
}

int32_t netTestInterfaceWasmServiceConnectionCreate(uint32_t product,
    uint32_t timeoutMs, NetInterfaceConnectionHandle* handle, int32_t* detail)
{
    return NetInterface::connectionCreateServiceClient(product, ProtocolTestEntropy, nullptr, timeoutMs, handle, detail);
}

int32_t netTestInterfaceWasmLinkCreateClient(const uint8_t* reserved,
    uint32_t reservedSize, uint32_t timeoutMs, NetInterfaceLinkHandle* handle)
{
    if (reserved != nullptr || reservedSize != 0)
    {
        if (handle != nullptr) { *handle = nullptr; }
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceLinkOptions options = {};
    options.m_version = 3;
    options.m_role = NI_LINK_CLIENT;
    options.m_tls.handshakeTimeoutMs = timeoutMs;
    options.m_tls.entropy = ProtocolTestEntropy;
    return NetInterface::linkCreate(&options, handle);
}
#endif