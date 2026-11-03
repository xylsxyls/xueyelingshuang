#include "NetSenderFileC.h"
#include "NetSender.h"

int32_t netPeerFileControlEncode(const uint32_t* fields, const uint8_t* transfer, const uint8_t* token,
    const uint8_t* hash, const uint8_t* key, uint32_t keySize,
    const uint8_t* authorization, uint32_t authorizationSize, uint8_t* output,
    uint32_t capacity, uint32_t* required)
{
    return NetSender::instance().fileProtocol().fileControlEncode(fields, transfer, token, hash, key, keySize, authorization, authorizationSize, output, capacity, required);
}

int32_t netPeerFileParseInfo(const uint8_t* bytes, uint32_t size, NetInterfaceFileInfo* info)
{
    return NetSender::instance().fileProtocol().fileParseInfo(bytes, size, info);
}

int32_t netPeerFileInfoFields(const uint8_t* bytes, uint32_t size, uint32_t* fields, uint8_t* transfer,
    uint8_t* token, uint8_t* hash)
{
    return NetSender::instance().fileProtocol().fileInfoFields(bytes, size, fields, transfer, token, hash);
}

int32_t netPeerFileFlowCreate(const NetInterfaceFileInfo* info, uint32_t product, uint32_t sender,
    uint32_t timeLow, uint32_t timeHigh, uint32_t timeoutMs,
    NetInterfaceFileFlowHandle* handle)
{
    return NetSender::instance().fileProtocol().fileFlowCreate(info, product, sender, timeLow, timeHigh, timeoutMs, handle);
}

int32_t netPeerFileFlowCreateWire(const uint8_t* bytes, uint32_t size, uint32_t product, uint32_t sender,
    uint32_t timeLow, uint32_t timeHigh, uint32_t timeoutMs,
    NetInterfaceFileFlowHandle* handle)
{
    return NetSender::instance().fileProtocol().fileFlowCreateWire(bytes, size, product, sender, timeLow, timeHigh, timeoutMs, handle);
}

void netPeerFileFlowDestroy(NetInterfaceFileFlowHandle handle)
{
    NetSender::instance().fileProtocol().fileFlowDestroy(handle);
}

int32_t netPeerFileFlowSetWriteBatchChunks(NetInterfaceFileFlowHandle handle, uint32_t chunks)
{
    return NetSender::instance().fileProtocol().fileFlowSetWriteBatchChunks(handle, chunks);
}

int32_t netPeerFileFlowPrime(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
    const uint8_t* bytes, uint32_t size)
{
    return NetSender::instance().fileProtocol().fileFlowPrime(handle, offsetLow, offsetHigh, bytes, size);
}

int32_t netPeerFileFlowProgress(NetInterfaceFileFlowHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    return NetSender::instance().fileProtocol().fileFlowProgress(handle, timeLow, timeHigh);
}

int32_t netPeerFileFlowRead(NetInterfaceFileFlowHandle handle, uint32_t* fields)
{
    return NetSender::instance().fileProtocol().fileFlowRead(handle, fields);
}

int32_t netPeerFileFlowProduced(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
    const uint8_t* bytes, uint32_t size)
{
    return NetSender::instance().fileProtocol().fileFlowProduced(handle, offsetLow, offsetHigh, bytes, size);
}

int32_t netPeerFileFlowAccept(NetInterfaceFileFlowHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    return NetSender::instance().fileProtocol().fileFlowAccept(handle, fields, payload);
}

int32_t netPeerFileFlowWrite(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** bytes)
{
    return NetSender::instance().fileProtocol().fileFlowWrite(handle, fields, bytes);
}

int32_t netPeerFileFlowWritten(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
    uint32_t size, int32_t result)
{
    return NetSender::instance().fileProtocol().fileFlowWritten(handle, offsetLow, offsetHigh, size, result);
}

int32_t netPeerFileFlowTakeCommit(NetInterfaceFileFlowHandle handle)
{
    return NetSender::instance().fileProtocol().fileFlowTakeCommit(handle);
}

int32_t netPeerFileFlowCommitted(NetInterfaceFileFlowHandle handle, int32_t result)
{
    return NetSender::instance().fileProtocol().fileFlowCommitted(handle, result);
}

int32_t netPeerFileFlowPeek(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** payload)
{
    return NetSender::instance().fileProtocol().fileFlowPeek(handle, fields, payload);
}

int32_t netPeerFileFlowConsume(NetInterfaceFileFlowHandle handle)
{
    return NetSender::instance().fileProtocol().fileFlowConsume(handle);
}

int32_t netPeerFileFlowCancel(NetInterfaceFileFlowHandle handle, int32_t reason)
{
    return NetSender::instance().fileProtocol().fileFlowCancel(handle, reason);
}

int32_t netPeerFileFlowSnapshot(NetInterfaceFileFlowHandle handle, NetInterfaceFileInfo* info, int32_t* result)
{
    return NetSender::instance().fileProtocol().fileFlowSnapshot(handle, info, result);
}

int32_t netPeerFileFlowSnapshotWire(NetInterfaceFileFlowHandle handle, uint8_t* bytes, uint32_t capacity,
    int32_t* result)
{
    return NetSender::instance().fileProtocol().fileFlowSnapshotWire(handle, bytes, capacity, result);
}