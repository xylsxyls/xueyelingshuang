#include "NetSenderDispatchMap.h"
#include "NetInterfaceMacro.h"

NetSenderDispatchMap::NetSenderDispatchMap()
{

}

int32_t NetSenderDispatchMap::insert(uint64_t wire, uint64_t local)
{
    if (wire == 0 || local == 0 || m_wire.find(wire) != m_wire.end() || m_local.find(local) != m_local.end())
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        m_wire.insert(std::make_pair(wire, local));
        try
        {
            m_local.insert(std::make_pair(local, wire));
        }
        catch (...)
        {
            m_wire.erase(wire);
            return NI_NO_MEMORY;
        }
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

uint64_t NetSenderDispatchMap::local(uint64_t wire) const
{
    const auto found = m_wire.find(wire);
    return found == m_wire.end() ? 0 : found->second;
}

uint64_t NetSenderDispatchMap::wire(uint64_t local) const
{
    const auto found = m_local.find(local);
    return found == m_local.end() ? 0 : found->second;
}

void NetSenderDispatchMap::erase(uint64_t wire)
{
    const auto found = m_wire.find(wire);
    if (found != m_wire.end())
    {
        m_local.erase(found->second);
        m_wire.erase(found);
    }
}

void NetSenderDispatchMap::clear()
{
    m_wire.clear();
    m_local.clear();
}

uint32_t NetSenderDispatchMap::size() const
{
    return m_wire.size() > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(m_wire.size());
}