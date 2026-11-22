#include "NetSenderLocalInboxPools.h"

NetSenderLocalInboxPools::NetSenderLocalInboxPools()
{
    m_queue.initialCapacity = 128;
    m_queue.maximumCapacity = 65536;
    m_queue.maximumProducers = 128;
    m_routes.initialCapacity = 128;
    m_routes.maximumCapacity = 65536;
    m_routes.directoryCapacity = 128;
    m_routes.lookupCacheCapacity = 0;
}