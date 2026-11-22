#include "NetEndpointPools.h"
#include "NetInterfaceMacro.h"
#include "NetEndpointRouterState.h"

NetEndpointPools::NetEndpointPools()
{
    m_endpoints.initialCapacity = m_files.initialCapacity = 128;
    m_endpoints.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetEndpointPeer>>);
    m_files.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetEndpointFileStream>>);
    m_endpoints.directoryCapacity = m_files.directoryCapacity = 128;
    m_endpoints.lookupCacheCapacity = m_files.lookupCacheCapacity = 0;
}