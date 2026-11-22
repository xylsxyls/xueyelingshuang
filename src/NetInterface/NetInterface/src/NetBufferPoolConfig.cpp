#include "NetBufferPoolConfig.h"
#include "NetInterfaceMacro.h"
#include "NetBufferPool.h"
#include <algorithm>

NetBufferPoolConfig::NetBufferPoolConfig() :
m_status(NI_OK)
{
    const size_t sizes[] = {
        sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetBufferPoolBlock<64>>>),
        sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetBufferPoolBlock<1024>>>),
        sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetBufferPoolBlock<16448>>>),
        sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetBufferPoolBlock<131072>>>),
        sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetBufferPoolBlock<1048704>>>)
    };
    for (size_t index = 0; index < 5; ++index)
    {
        m_classes[index].initialCapacity = index == 0 ? 128 : 0;
        m_classes[index].maximumCapacity = 8388608 / sizes[index];
        m_classes[index].directoryCapacity = 128;
        m_classes[index].lookupCacheCapacity = 0;
    }
    try
    {
        for (size_t index = 1; index < 5; ++index)
        {
            const size_t initialGrowth = (std::max)(size_t(1), size_t(65536) / sizes[index]);
            // context是只读容量快照，返回本次新增槽数；较大正文首次只分配少量槽。
            m_classes[index].growPolicy = [initialGrowth](const LockFreeHashMapGrowContext& context) -> size_t
            {
                return (std::max)(initialGrowth, context.currentCapacity / 2);
            };
        }
    }
    catch (...)
    {
        m_status = NI_NO_MEMORY;
    }
}

int32_t NetBufferPoolConfig::status() const
{
    return m_status;
}