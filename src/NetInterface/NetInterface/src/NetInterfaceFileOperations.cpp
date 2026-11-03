#include "NetInterfaceMacro.h"
#include "NetInterfaceFileFlowState.h"
#include "NetInterface.h"
#include "NetInterfaceFileGuard.h"
#include <cstring>

NetInterfaceFileFlowOpaque::NetInterfaceFileFlowOpaque(const NetInterfaceFileInfo& info, uint32_t product, bool sender, uint64_t nowMs,
                                                       uint32_t timeoutMs) :
m_flow(info, product, sender, nowMs, timeoutMs)
{

}

int32_t NetInterface::fileParseInfo(const uint8_t* bytes, uint32_t size, NetInterfaceFileInfo* info)
{
    return info != nullptr ? NetInterfaceFile::parseInfo(bytes, size, *info) : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileInfoFields(const uint8_t* bytes, uint32_t size, uint32_t* fields, uint8_t* transfer, uint8_t* token, uint8_t* hash)
{
    if (NetInterfaceFileGuard::overlap(fields, 12 * sizeof(uint32_t), bytes, size) ||
        NetInterfaceFileGuard::overlap(transfer, 16, bytes, size) ||
        NetInterfaceFileGuard::overlap(token, 32, bytes, size) ||
        NetInterfaceFileGuard::overlap(hash, 32, bytes, size) ||
        NetInterfaceFileGuard::overlap(fields, 12 * sizeof(uint32_t), transfer, 16) ||
        NetInterfaceFileGuard::overlap(fields, 12 * sizeof(uint32_t), token, 32) ||
        NetInterfaceFileGuard::overlap(fields, 12 * sizeof(uint32_t), hash, 32) ||
        NetInterfaceFileGuard::overlap(transfer, 16, token, 32) ||
        NetInterfaceFileGuard::overlap(transfer, 16, hash, 32) ||
        NetInterfaceFileGuard::overlap(token, 32, hash, 32))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (fields != nullptr)
    {
        std::memset(fields, 0, sizeof(uint32_t) * 12);
    }
    if (transfer != nullptr)
    {
        std::memset(transfer, 0, 16);
    }
    if (token != nullptr)
    {
        std::memset(token, 0, 32);
    }
    if (hash != nullptr)
    {
        std::memset(hash, 0, 32);
    }
    if (fields == nullptr || transfer == nullptr || token == nullptr || hash == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFileInfo info = {};
    NetInterfaceFileGuard wipe(&info, sizeof(info));
    const int32_t result = NetInterfaceFile::parseInfo(bytes, size, info);
    if (result != NI_OK)
    {
        return result;
    }
    fields[0] = info.direction;
    fields[1] = info.state;
    fields[2] = info.chunk;
    fields[3] = info.window;
    fields[4] = static_cast<uint32_t>(info.total);
    fields[5] = static_cast<uint32_t>(info.total >> 32);
    fields[6] = static_cast<uint32_t>(info.offset);
    fields[7] = static_cast<uint32_t>(info.offset >> 32);
    fields[8] = static_cast<uint32_t>(info.stream);
    fields[9] = static_cast<uint32_t>(info.stream >> 32);
    fields[10] = static_cast<uint32_t>(info.expiresUtc);
    fields[11] = static_cast<uint32_t>(info.expiresUtc >> 32);
    std::memcpy(transfer, info.transfer, 16);
    std::memcpy(token, info.token, 32);
    std::memcpy(hash, info.hash, 32);
    return NI_OK;
}

int32_t NetInterface::fileFlowCreate(const NetInterfaceFileInfo* info, uint32_t product, uint32_t sender, uint32_t timeLow, uint32_t timeHigh,
                                   uint32_t timeoutMs, NetInterfaceFileFlowHandle* handle)
{
    if (NetInterfaceFileGuard::overlap(handle, sizeof(*handle), info, sizeof(*info)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle != nullptr)
    {
        *handle = nullptr;
    }
    if (handle == nullptr || info == nullptr || sender > 1)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        const uint64_t now = (static_cast<uint64_t>(timeHigh) << 32) | timeLow;
        *handle = new NetInterfaceFileFlowOpaque(*info, product, sender != 0, now, timeoutMs);
        const int32_t status = (*handle)->m_flow.progress(now);
        if (status < 0)
        {
            delete *handle;
            *handle = nullptr;
            return status;
        }
        return NI_OK;
    }
    catch (...)
    {
        delete *handle;
        *handle = nullptr;
        return NI_NO_MEMORY;
    }
}

void NetInterface::fileFlowDestroy(NetInterfaceFileFlowHandle handle)
{
    delete handle;
}

int32_t NetInterface::fileFlowCreateWire(const uint8_t* bytes, uint32_t size, uint32_t product, uint32_t sender, uint32_t timeLow,
                                       uint32_t timeHigh, uint32_t timeoutMs, NetInterfaceFileFlowHandle* handle)
{
    if (NetInterfaceFileGuard::overlap(handle, sizeof(*handle), bytes, size))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle != nullptr)
    {
        *handle = nullptr;
    }
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFileInfo info = {};
    NetInterfaceFileGuard wipe(&info, sizeof(info));
    const int32_t parsed = NetInterfaceFile::parseInfo(bytes, size, info);
    if (parsed != NI_OK)
    {
        return parsed;
    }
    const int32_t result = NetInterface::fileFlowCreate(&info, product, sender, timeLow, timeHigh, timeoutMs, handle);
    return result;
}

int32_t NetInterface::fileFlowPrime(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh, const uint8_t* bytes,
                                  uint32_t size)
{
    return handle != nullptr ? handle->m_flow.prime((static_cast<uint64_t>(offsetHigh) << 32) | offsetLow, bytes, size)
                             : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowProgress(NetInterfaceFileFlowHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    return handle != nullptr ? handle->m_flow.progress((static_cast<uint64_t>(timeHigh) << 32) | timeLow) : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowRead(NetInterfaceFileFlowHandle handle, uint32_t* fields)
{
    if (fields != nullptr)
    {
        std::memset(fields, 0, sizeof(uint32_t) * 3);
    }
    if (handle == nullptr || fields == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    uint64_t offset = 0;
    uint32_t size = 0;
    const int32_t result = handle->m_flow.readRequest(offset, size);
    if (result == NI_READY)
    {
        fields[0] = static_cast<uint32_t>(offset);
        fields[1] = static_cast<uint32_t>(offset >> 32);
        fields[2] = size;
    }
    return result;
}

int32_t NetInterface::fileFlowProduced(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh, const uint8_t* bytes,
                                     uint32_t size)
{
    return handle != nullptr ? handle->m_flow.produced((static_cast<uint64_t>(offsetHigh) << 32) | offsetLow, bytes, size)
                             : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowAccept(NetInterfaceFileFlowHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    if (handle == nullptr || fields == nullptr || fields[0] > UINT16_MAX || (fields[5] != 0 && payload == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(fields[0]);
    frame.m_productId = fields[1];
    frame.m_requestId = (static_cast<uint64_t>(fields[3]) << 32) | fields[2];
    frame.m_messageType = fields[4];
    frame.m_payloadSize = fields[5];
    frame.m_payload = payload;
    return handle->m_flow.accept(frame);
}

int32_t NetInterface::fileFlowWrite(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** bytes)
{
    if (NetInterfaceFileGuard::overlap(fields, 3 * sizeof(uint32_t), bytes, sizeof(*bytes)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (fields != nullptr)
    {
        std::memset(fields, 0, sizeof(uint32_t) * 3);
    }
    if (bytes != nullptr)
    {
        *bytes = nullptr;
    }
    if (handle == nullptr || fields == nullptr || bytes == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    uint64_t offset = 0;
    uint32_t size = 0;
    const int32_t result = handle->m_flow.writeRequest(offset, *bytes, size);
    if (result == NI_READY)
    {
        fields[0] = static_cast<uint32_t>(offset);
        fields[1] = static_cast<uint32_t>(offset >> 32);
        fields[2] = size;
    }
    return result;
}

int32_t NetInterface::fileFlowWritten(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh, uint32_t size,
                                    int32_t result)
{
    return handle != nullptr ? handle->m_flow.written((static_cast<uint64_t>(offsetHigh) << 32) | offsetLow, size, result)
                             : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowTakeCommit(NetInterfaceFileFlowHandle handle)
{
    return handle != nullptr ? handle->m_flow.takeCommit() : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowCommitted(NetInterfaceFileFlowHandle handle, int32_t result)
{
    return handle != nullptr ? handle->m_flow.committed(result) : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowPeek(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (NetInterfaceFileGuard::overlap(fields, 6 * sizeof(uint32_t), payload, sizeof(*payload)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (fields != nullptr)
    {
        std::memset(fields, 0, sizeof(uint32_t) * 6);
    }
    if (payload != nullptr)
    {
        *payload = nullptr;
    }
    if (handle == nullptr || fields == nullptr || payload == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    const int32_t result = handle->m_flow.peekOutput(frame);
    if (result == NI_READY)
    {
        fields[0] = frame.m_kind;
        fields[1] = frame.m_productId;
        fields[2] = static_cast<uint32_t>(frame.m_requestId);
        fields[3] = static_cast<uint32_t>(frame.m_requestId >> 32);
        fields[4] = frame.m_messageType;
        fields[5] = frame.m_payloadSize;
        *payload = frame.m_payload;
    }
    return result;
}

int32_t NetInterface::fileFlowConsume(NetInterfaceFileFlowHandle handle)
{
    return handle != nullptr ? handle->m_flow.consumeOutput() : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowCancel(NetInterfaceFileFlowHandle handle, int32_t reason)
{
    if (handle == nullptr || reason >= 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    handle->m_flow.cancel(reason);
    return NI_OK;
}

int32_t NetInterface::fileFlowSetWriteBatchChunks(NetInterfaceFileFlowHandle handle, uint32_t chunks)
{
    return handle ? handle->m_flow.setWriteBatchChunks(chunks) : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowSnapshot(NetInterfaceFileFlowHandle handle, NetInterfaceFileInfo* info, int32_t* result)
{
    if (NetInterfaceFileGuard::overlap(info, sizeof(*info), result, sizeof(*result)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (info != nullptr)
    {
        std::memset(info, 0, sizeof(*info));
    }
    if (result != nullptr)
    {
        *result = NI_INVALID_ARGUMENT;
    }
    if (handle == nullptr || info == nullptr || result == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    handle->m_flow.snapshot(*info, *result);
    return NI_OK;
}

int32_t NetInterface::fileFlowSnapshotWire(NetInterfaceFileFlowHandle handle, uint8_t* bytes, uint32_t capacity, int32_t* result)
{
    if (NetInterfaceFileGuard::overlap(bytes, capacity, result, sizeof(*result)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (result)
    {
        *result = NI_INVALID_STATE;
    }
    if (!handle || !result || (!bytes && capacity))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (capacity < NET_INTERFACE_FILE_INFO_SIZE)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    NetInterfaceFileInfo info = {};
    NetInterfaceFileGuard wipe(&info, sizeof(info));
    handle->m_flow.snapshot(info, *result);
    return NetInterfaceFile::encodeInfo(info, bytes, capacity);
}