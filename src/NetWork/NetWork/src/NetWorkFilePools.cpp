#include "NetWorkFilePools.h"
#ifndef __EMSCRIPTEN__
#include "NetNativeFileIoState.h"
#endif

NetWorkFilePools::NetWorkFilePools()
{
    m_entries.initialCapacity = m_files.initialCapacity = 128;
#ifndef __EMSCRIPTEN__
    m_entries.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetFileIoEntry>>);
    m_files.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetFileIoHandle>>);
#endif
    m_entries.directoryCapacity = m_files.directoryCapacity = 128;
    m_entries.lookupCacheCapacity = m_files.lookupCacheCapacity = 0;
    m_queue.initialCapacity = 128;
    m_queue.maximumCapacity = 65536;
    m_queue.maximumProducers = 4;
}