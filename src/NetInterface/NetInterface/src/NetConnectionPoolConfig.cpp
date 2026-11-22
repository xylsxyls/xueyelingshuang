#include "NetConnectionPoolConfig.h"
#include "NetInterfaceMacro.h"
#include "NetConnectionQueue.h"

NetConnectionPoolConfig::NetConnectionPoolConfig()
{
    m_dataRecords.initialCapacity = 128;
    m_dataRecords.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetConnectionWrite>>);
    m_dataRecords.directoryCapacity = 128;
    m_dataRecords.lookupCacheCapacity = 0;
    m_controlRecords = m_dataRecords;
}