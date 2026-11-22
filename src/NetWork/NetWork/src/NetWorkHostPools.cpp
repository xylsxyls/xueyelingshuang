#include "NetWorkMacro.h"
#include "NetWorkHostPools.h"
#include "NetWorkHostBridge.h"

NetWorkHostPools::NetWorkHostPools()
{
    m_mail.initialCapacity = m_records.initialCapacity = m_index.initialCapacity = 128;
    m_mail.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetWorkHostMail>>);
    m_records.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetWorkHostEvent>>);
    m_index.maximumCapacity = 65536;
    m_mail.directoryCapacity = m_records.directoryCapacity = m_index.directoryCapacity = 128;
    m_mail.lookupCacheCapacity = m_records.lookupCacheCapacity = m_index.lookupCacheCapacity = 0;
    m_incoming.initialCapacity = 128;
    m_incoming.maximumCapacity = 65536;
    m_incoming.maximumProducers = 128;
}