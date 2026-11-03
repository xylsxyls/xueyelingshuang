#include "NetFileProtocol.h"
#include "NetInterface.h"

int32_t NetFileProtocol::fileControlEncode(const uint32_t* fields, const uint8_t* transfer, const uint8_t* token,
    const uint8_t* hash, const uint8_t* key, uint32_t keySize,
    const uint8_t* authorization, uint32_t authorizationSize, uint8_t* output,
    uint32_t capacity, uint32_t* required)
{
    return NetInterface::fileControlEncode(fields, transfer, token, hash, key, keySize, authorization, authorizationSize, output, capacity, required);
}

int32_t NetFileProtocol::fileParseInfo(const uint8_t* bytes, uint32_t size, NetInterfaceFileInfo* info)
{
    return NetInterface::fileParseInfo(bytes, size, info);
}

int32_t NetFileProtocol::fileInfoFields(const uint8_t* bytes, uint32_t size, uint32_t* fields, uint8_t* transfer,
    uint8_t* token, uint8_t* hash)
{
    return NetInterface::fileInfoFields(bytes, size, fields, transfer, token, hash);
}

int32_t NetFileProtocol::fileFlowCreate(const NetInterfaceFileInfo* info, uint32_t product, uint32_t sender,
    uint32_t timeLow, uint32_t timeHigh, uint32_t timeoutMs,
    NetInterfaceFileFlowHandle* handle)
{
    return NetInterface::fileFlowCreate(info, product, sender, timeLow, timeHigh, timeoutMs, handle);
}

int32_t NetFileProtocol::fileFlowCreateWire(const uint8_t* bytes, uint32_t size, uint32_t product, uint32_t sender,
    uint32_t timeLow, uint32_t timeHigh, uint32_t timeoutMs,
    NetInterfaceFileFlowHandle* handle)
{
    return NetInterface::fileFlowCreateWire(bytes, size, product, sender, timeLow, timeHigh, timeoutMs, handle);
}

void NetFileProtocol::fileFlowDestroy(NetInterfaceFileFlowHandle handle)
{
    NetInterface::fileFlowDestroy(handle);
}

int32_t NetFileProtocol::fileFlowSetWriteBatchChunks(NetInterfaceFileFlowHandle handle, uint32_t chunks)
{
    return NetInterface::fileFlowSetWriteBatchChunks(handle, chunks);
}

int32_t NetFileProtocol::fileFlowPrime(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
    const uint8_t* bytes, uint32_t size)
{
    return NetInterface::fileFlowPrime(handle, offsetLow, offsetHigh, bytes, size);
}

int32_t NetFileProtocol::fileFlowProgress(NetInterfaceFileFlowHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    return NetInterface::fileFlowProgress(handle, timeLow, timeHigh);
}

int32_t NetFileProtocol::fileFlowRead(NetInterfaceFileFlowHandle handle, uint32_t* fields)
{
    return NetInterface::fileFlowRead(handle, fields);
}

int32_t NetFileProtocol::fileFlowProduced(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
    const uint8_t* bytes, uint32_t size)
{
    return NetInterface::fileFlowProduced(handle, offsetLow, offsetHigh, bytes, size);
}

int32_t NetFileProtocol::fileFlowAccept(NetInterfaceFileFlowHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    return NetInterface::fileFlowAccept(handle, fields, payload);
}

int32_t NetFileProtocol::fileFlowWrite(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** bytes)
{
    return NetInterface::fileFlowWrite(handle, fields, bytes);
}

int32_t NetFileProtocol::fileFlowWritten(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
    uint32_t size, int32_t result)
{
    return NetInterface::fileFlowWritten(handle, offsetLow, offsetHigh, size, result);
}

int32_t NetFileProtocol::fileFlowTakeCommit(NetInterfaceFileFlowHandle handle)
{
    return NetInterface::fileFlowTakeCommit(handle);
}

int32_t NetFileProtocol::fileFlowCommitted(NetInterfaceFileFlowHandle handle, int32_t result)
{
    return NetInterface::fileFlowCommitted(handle, result);
}

int32_t NetFileProtocol::fileFlowPeek(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** payload)
{
    return NetInterface::fileFlowPeek(handle, fields, payload);
}

int32_t NetFileProtocol::fileFlowConsume(NetInterfaceFileFlowHandle handle)
{
    return NetInterface::fileFlowConsume(handle);
}

int32_t NetFileProtocol::fileFlowCancel(NetInterfaceFileFlowHandle handle, int32_t reason)
{
    return NetInterface::fileFlowCancel(handle, reason);
}

int32_t NetFileProtocol::fileFlowSnapshot(NetInterfaceFileFlowHandle handle, NetInterfaceFileInfo* info, int32_t* result)
{
    return NetInterface::fileFlowSnapshot(handle, info, result);
}

int32_t NetFileProtocol::fileFlowSnapshotWire(NetInterfaceFileFlowHandle handle, uint8_t* bytes, uint32_t capacity,
    int32_t* result)
{
    return NetInterface::fileFlowSnapshotWire(handle, bytes, capacity, result);
}