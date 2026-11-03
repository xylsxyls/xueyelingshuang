#include "NetPeerMacro.h"
#include "NetDirectorySnapshot.h"

NetDirectorySnapshot::NetDirectorySnapshot() :
m_handle(nullptr)
{

}

NetDirectorySnapshot::~NetDirectorySnapshot()
{
    reset();
}

void NetDirectorySnapshot::reset()
{
    NetInterface::directoryDestroy(m_handle);
    m_handle = nullptr;
}

int32_t NetDirectorySnapshot::create(const NetInterfaceDirectoryNodeData* nodes, uint32_t count)
{
    reset();
    return NetInterface::directoryCreateNodes(nodes, count, &m_handle);
}

int32_t NetDirectorySnapshot::parse(const uint8_t* bytes, uint32_t size, uint64_t utcMs)
{
    reset();
    return utcMs == 0 ? NetInterface::directoryCreate(bytes, size, &m_handle) :
        NetInterface::directoryCreateCache(bytes, size, static_cast<uint32_t>(utcMs), static_cast<uint32_t>(utcMs >> 32), &m_handle);
}

int32_t NetDirectorySnapshot::copy(NetInterfaceDirectoryNodeData* nodes, uint32_t capacity, uint32_t& count) const
{
    return NetInterface::directoryCopyNodes(m_handle, nodes, capacity, &count);
}

int32_t NetDirectorySnapshot::values(std::vector<NetInterfaceDirectoryNodeData>& nodes) const
{
    uint32_t count = 0;
    const int32_t measured = NetInterface::directoryCount(m_handle, &count);
    if (measured != NI_OK || count == 0 || count > 64)
    {
        return measured < 0 ? measured : NI_PROTOCOL_ERROR;
    }
    try
    {
        std::vector<NetInterfaceDirectoryNodeData> copied(count);
        for (auto& node : copied)
        {
            node.m_structSize = sizeof(node);
            node.m_version = 1;
        }
        const int32_t result = copy(copied.data(), count, count);
        if (result == NI_OK)
        {
            nodes.swap(copied);
        }
        return result;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetDirectorySnapshot::cache(uint64_t utcMs, std::string& bytes) const
{
    bytes.clear();
    uint32_t required = 0;
    const int32_t measured = NetInterface::directoryCache(m_handle, static_cast<uint32_t>(utcMs),
        static_cast<uint32_t>(utcMs >> 32), nullptr, 0, &required);
    if (measured != NI_BUFFER_TOO_SMALL || required <= 48 || required > 16432)
    {
        return measured < 0 && measured != NI_BUFFER_TOO_SMALL ? measured : NI_PROTOCOL_ERROR;
    }
    try
    {
        const uint32_t capacity = required;
        bytes.resize(capacity);
        const int32_t copied = NetInterface::directoryCache(m_handle, static_cast<uint32_t>(utcMs),
            static_cast<uint32_t>(utcMs >> 32), reinterpret_cast<uint8_t*>(&bytes[0]), capacity, &required);
        if (copied != NI_OK || required > capacity || required <= 48)
        {
            bytes.clear();
            return copied != NI_OK ? copied : NI_PROTOCOL_ERROR;
        }
        bytes.resize(required);
        return NI_OK;
    }
    catch (...)
    {
        bytes.clear();
        return NI_NO_MEMORY;
    }
}