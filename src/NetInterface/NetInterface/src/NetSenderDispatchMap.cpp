#include "NetSenderDispatchMap.h"
#include "NetInterfaceMacro.h"
#include "NetSenderDispatch.h"
#include <cstring>

#if !defined(__EMSCRIPTEN__)
#include <chrono>
#include <thread>
#endif

NetSenderDispatchMap::NetSenderDispatchMap()
{
    clear();
}

uint32_t NetSenderDispatchMap::hash(uint64_t key)
{
    return static_cast<uint32_t>(key ^ (key >> 32)) & 255;
}

uint32_t NetSenderDispatchMap::bucket(uint64_t key, bool byLocal) const
{
    const uint16_t* index = byLocal ? m_local : m_wire;
    uint32_t position = hash(key);
    for (uint32_t step = 0; step < 256; ++step)
    {
        const uint32_t slot = index[position];
        if (slot == 0 || (byLocal ? m_entries[slot - 1].m_local : m_entries[slot - 1].m_wire) == key)
        {
            return position;
        }
        position = (position + 1) & 255;
    }
    return 256;
}

bool NetSenderDispatchMap::insert(uint64_t wire, uint64_t local)
{
    if (wire == 0 || local == 0 || m_free == 0)
    {
        return false;
    }
    const uint32_t wireBucket = bucket(wire, false);
    const uint32_t localBucket = bucket(local, true);
    if (wireBucket == 256 || localBucket == 256 || m_wire[wireBucket] != 0 || m_local[localBucket] != 0)
    {
        return false;
    }
    const uint32_t slot = m_free;
    NetSenderDispatchEntry& entry = m_entries[slot - 1];
    m_free = static_cast<uint32_t>(entry.m_local);
    entry.m_wire = wire;
    entry.m_local = local;
    m_wire[wireBucket] = static_cast<uint16_t>(slot);
    m_local[localBucket] = static_cast<uint16_t>(slot);
    ++m_count;
    return true;
}

uint64_t NetSenderDispatchMap::local(uint64_t wire) const
{
    const uint32_t position = bucket(wire, false);
    return position == 256 || m_wire[position] == 0 ? 0 : m_entries[m_wire[position] - 1].m_local;
}

uint64_t NetSenderDispatchMap::wire(uint64_t local) const
{
    const uint32_t position = bucket(local, true);
    return position == 256 || m_local[position] == 0 ? 0 : m_entries[m_local[position] - 1].m_wire;
}

void NetSenderDispatchMap::remove(uint64_t key, bool byLocal)
{
    uint16_t* index = byLocal ? m_local : m_wire;
    uint32_t hole = bucket(key, byLocal);
    if (hole == 256 || index[hole] == 0)
    {
        return;
    }
    uint32_t next = (hole + 1) & 255;
    for (uint32_t step = 0; step < 256 && index[next] != 0; ++step)
    {
        const NetSenderDispatchEntry& entry = m_entries[index[next] - 1];
        const uint32_t origin = hash(byLocal ? entry.m_local : entry.m_wire);
        // 仅把探测路径经过空洞的槽前移，跨数组末端仍按环形距离比较。
        if (((hole - origin) & 255) < ((next - origin) & 255))
        {
            index[hole] = index[next];
            hole = next;
        }
        next = (next + 1) & 255;
    }
    index[hole] = 0;
}

void NetSenderDispatchMap::erase(uint64_t wire)
{
    const uint32_t position = bucket(wire, false);
    if (position == 256 || m_wire[position] == 0)
    {
        return;
    }
    const uint32_t slot = m_wire[position];
    NetSenderDispatchEntry& entry = m_entries[slot - 1];
    remove(entry.m_wire, false);
    remove(entry.m_local, true);
    entry.m_wire = 0;
    entry.m_local = m_free;
    m_free = slot;
    --m_count;
}

void NetSenderDispatchMap::clear()
{
    std::memset(m_wire, 0, sizeof(m_wire));
    std::memset(m_local, 0, sizeof(m_local));
    for (uint32_t slot = 0; slot < 128; ++slot)
    {
        m_entries[slot].m_wire = 0;
        m_entries[slot].m_local = slot == 127 ? 0 : slot + 2;
    }
    m_free = 1;
    m_count = 0;
}

uint32_t NetSenderDispatchMap::size() const
{
    return m_count;
}