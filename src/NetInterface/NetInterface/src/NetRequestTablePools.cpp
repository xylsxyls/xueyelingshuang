#include "NetInterfaceMacro.h"
#include "NetRequestTablePools.h"
#include "NetInterfaceRequestsState.h"

NetRequestTablePools::NetRequestTablePools()
{
    m_requests.initialCapacity = 128;
    m_requests.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetInterfaceRequestEntry>>);
    m_requests.directoryCapacity = 128;
    m_requests.lookupCacheCapacity = 0;
    m_endpoints = m_requests;
    m_endpoints.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetInterfaceRequestEndpoint>>);
    m_cancellations = m_requests;
    m_cancellations.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<uint64_t>>);
}