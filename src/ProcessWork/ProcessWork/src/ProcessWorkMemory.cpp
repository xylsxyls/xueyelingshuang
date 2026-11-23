#include "ProcessWorkMemory.h"
#include "ProcessWorkReceiveQueue.h"
#include "CSystem/CSystemAPI.h"
#include <algorithm>
#include <limits>

ProcessWorkQueueOptions::ProcessWorkQueueOptions() :
m_distributedCounters(-1), m_maintenanceInterval(64), m_maintenanceBudget(8), m_capacityRetries(4096),
m_capacitySleepMilliseconds(1), m_consumerYieldInterval(4096), m_cancelableReservations(false)
{

}

LockFreeQueueResult ProcessWorkQueueOptions::configureProducerCounters(bool distributed)
{
    m_distributedCounters = distributed ? 1 : 0;
    return LockFreeQueueSuccess;
}

LockFreeQueueResult ProcessWorkQueueOptions::configureCancelableReservations(bool enabled)
{
    m_cancelableReservations = enabled;
    return LockFreeQueueSuccess;
}

LockFreeQueueResult ProcessWorkQueueOptions::configureEntryOnly(bool enabled)
{
    return enabled ? LockFreeQueueInvalidConfiguration : LockFreeQueueSuccess;
}

LockFreeQueueResult ProcessWorkQueueOptions::configureAutomaticMaintenance(uint32_t interval, size_t workBudget)
{
    if (interval == 0 || workBudget == 0)
    {
        return LockFreeQueueInvalidConfiguration;
    }
    m_maintenanceInterval = interval;
    m_maintenanceBudget = workBudget;
    return LockFreeQueueSuccess;
}

LockFreeQueueResult ProcessWorkQueueOptions::configureCapacityWait(uint32_t retriesBeforeSleep, uint32_t sleepMilliseconds)
{
    if (sleepMilliseconds == 0)
    {
        return LockFreeQueueInvalidConfiguration;
    }
    m_capacityRetries = retriesBeforeSleep;
    m_capacitySleepMilliseconds = sleepMilliseconds;
    return LockFreeQueueSuccess;
}

LockFreeQueueResult ProcessWorkQueueOptions::configureConsumerYieldInterval(uint32_t operations)
{
    if (operations == 0 || operations > 65536)
    {
        return LockFreeQueueInvalidConfiguration;
    }
    m_consumerYieldInterval = operations;
    return LockFreeQueueSuccess;
}

bool ProcessWorkQueueOptions::valid() const
{
    return m_distributedCounters >= -1 && m_distributedCounters <= 1 && m_maintenanceInterval != 0 &&
        m_maintenanceBudget != 0 && m_capacitySleepMilliseconds != 0 &&
        m_consumerYieldInterval != 0 && m_consumerYieldInterval <= 65536;
}

ProcessWorkMemoryConfig::ProcessWorkMemoryConfig() :
m_reserveBytes(256ULL * 1024 * 1024), m_reservePercent(10),
m_growthBytes(4ULL * 1024 * 1024), m_retryMilliseconds(100), m_localQueueBytes(-1),
m_localQueueInitialCapacity(64), m_receiveBatchReserve(0), m_receiveBatchBytes(65536 + 32),
m_receiveBatchMaximumBytes(-1), m_receiveBatchCacheLimit(8), m_postBufferReserve(0), m_postBufferSlotBytes(128), m_postBufferMaximum(-1)
{

}

bool ProcessWorkMemoryConfig::setLocalQueueLimitMB(int64_t megabytes)
{
    const int64_t bytesPerMB = 1024 * 1024;
    if (megabytes != -1 && (megabytes <= 0 || megabytes > INT64_MAX / bytesPerMB))
    {
        return false;
    }
    m_localQueueBytes = megabytes == -1 ? -1 : megabytes * bytesPerMB;
    return true;
}

/** 校验接收批次大小、保留数量和硬上限，用除法避免预留数乘法溢出
@param [in] bytes 标准记录区大小
@param [in] maximum 实际存储最大字节数，-1不限
@param [in] cache 已归还批次的缓存数量，-1不限
@param [in] reserve 启动时预留的标准批次数
@return true表示启动预留和至少一个完整标准批次能放入上限
*/
static bool ValidReceiveBatchPool(uint32_t bytes, int64_t maximum, int64_t cache, uint32_t reserve)
{
    if (bytes < 17 || bytes > INT32_MAX || cache < -1 || maximum < -1)
    {
        return false;
    }
    const uint64_t limit = maximum == -1 ? static_cast<uint64_t>(INT64_MAX) : static_cast<uint64_t>(maximum);
    return limit / (static_cast<uint64_t>(bytes) + sizeof(ProcessWorkReceiveBatch)) >= (std::max)(uint32_t(1), reserve);
}

bool ProcessWorkMemoryConfig::setReceiveBatchPool(uint32_t batchBytes, int64_t maximumBytes, int64_t cacheLimit)
{
    if (!ValidReceiveBatchPool(batchBytes, maximumBytes, cacheLimit, m_receiveBatchReserve))
    {
        return false;
    }
    m_receiveBatchBytes = batchBytes;
    m_receiveBatchMaximumBytes = maximumBytes;
    m_receiveBatchCacheLimit = cacheLimit;
    return true;
}

bool ProcessWorkMemory::valid(const ProcessWorkMemoryConfig& config)
{
    return config.m_reservePercent <= 90 && config.m_growthBytes != 0 && config.m_retryMilliseconds != 0 &&
        config.m_localQueueBytes >= -1 && config.m_localQueueBytes != 0 &&
        config.m_localQueueInitialCapacity != 0 &&
        ValidReceiveBatchPool(config.m_receiveBatchBytes, config.m_receiveBatchMaximumBytes, config.m_receiveBatchCacheLimit, config.m_receiveBatchReserve) &&
        config.m_receiveBatchReserve <= (std::numeric_limits<uint32_t>::max)() - 32 && config.m_postBufferMaximum >= -1 &&
        (config.m_postBufferSlotBytes == 64 || config.m_postBufferSlotBytes == 128) &&
        (config.m_postBufferMaximum == -1 || config.m_postBufferReserve <= static_cast<uint64_t>(config.m_postBufferMaximum));
}

bool ProcessWorkMemory::queueConfig(const ProcessWorkMemoryConfig& config, bool freeQueue, LockFreeQueuePoolConfig& pool)
{
    const LockFreeQueuePoolConfig& selected = freeQueue ? config.m_freeQueuePool : config.m_queuePool;
    if (config.m_localQueueInitialCapacity != 64 && selected.initialCapacity != 64 &&
        selected.initialCapacity != config.m_localQueueInitialCapacity)
    {
        return false;
    }
    try
    {
        pool = selected;
        if (config.m_localQueueInitialCapacity != 64)
        {
            pool.initialCapacity = config.m_localQueueInitialCapacity;
        }
        return true;
    }
    catch (...)
    {
        // 复制业务提供的增长通知可能失败，调用方仅在成功后使用输出配置。
        return false;
    }
}

bool ProcessWorkMemory::allows(const ProcessWorkMemoryConfig& config, uint64_t bytes)
{
    if (!valid(config))
    {
        return false;
    }
    uint64_t available = 0;
    uint64_t total = 0;
    uint64_t virtualBytes = 0;
    if (!CSystem::queryMemoryAvailability(total, available, virtualBytes))
    {
        return false;
    }
    // Windows 单独给进程虚拟地址空间留出余量；其他原生平台未提供这项限制。
    const uint64_t virtualReserve = 16ULL * 1024 * 1024;
    if (virtualBytes != UINT64_MAX && (virtualBytes <= virtualReserve || bytes > virtualBytes - virtualReserve))
    {
        return false;
    }
    const uint64_t reserve = (std::max)(config.m_reserveBytes, total / 100 * config.m_reservePercent);
    return reserve < available && bytes <= available - reserve;
}