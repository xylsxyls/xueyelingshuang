#include "NetRelayPoolConfig.h"
#include "NetRelayTransportState.h"

NetRelayPoolConfig::NetRelayPoolConfig()
{
    m_dataRecords.initialCapacity = 128;
    m_dataRecords.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetRelayTransportWrite>>);
    m_dataRecords.directoryCapacity = 128;
    m_dataRecords.lookupCacheCapacity = 0;
    m_controlRecords = m_dataRecords;
}