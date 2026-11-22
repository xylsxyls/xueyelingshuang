#include "ProcessWorkMemory.h"
#include <algorithm>
#include <limits>
#ifdef _WIN32
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

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
m_localQueueInitialCapacity(64), m_receiveBatchReserve(0), m_postBufferReserve(0), m_postBufferSlotBytes(128), m_postBufferMaximum(-1)
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

bool ProcessWorkMemory::valid(const ProcessWorkMemoryConfig& config)
{
    return config.m_reservePercent <= 90 && config.m_growthBytes != 0 && config.m_retryMilliseconds != 0 &&
        config.m_localQueueBytes >= -1 && config.m_localQueueBytes != 0 &&
        config.m_localQueueInitialCapacity != 0 &&
        config.m_receiveBatchReserve <= (std::numeric_limits<uint32_t>::max)() - 32 && config.m_postBufferMaximum >= -1 &&
        (config.m_postBufferSlotBytes == 64 || config.m_postBufferSlotBytes == 128) &&
        (config.m_postBufferMaximum == -1 || config.m_postBufferReserve <= static_cast<uint64_t>(config.m_postBufferMaximum));
}

bool ProcessWorkMemory::queueConfig(const ProcessWorkMemoryConfig& config, bool freeQueue, LockFreeQueuePoolConfig& pool)
{
    pool = freeQueue ? config.m_freeQueuePool : config.m_queuePool;
    if (config.m_localQueueInitialCapacity != 64)
    {
        if (pool.initialCapacity != 64 && pool.initialCapacity != config.m_localQueueInitialCapacity)
        {
            return false;
        }
        pool.initialCapacity = config.m_localQueueInitialCapacity;
    }
    return true;
}

bool ProcessWorkMemory::allows(const ProcessWorkMemoryConfig& config, uint64_t bytes)
{
    if (!valid(config))
    {
        return false;
    }
    uint64_t available = 0;
    uint64_t total = 0;
#ifdef _WIN32
    MEMORYSTATUSEX state = {};
    state.dwLength = sizeof(state);
    if (!GlobalMemoryStatusEx(&state))
    {
        return false;
    }
    total = state.ullTotalPhys;
    // 32位进程的虚拟地址余量与整机物理内存比例不是同一个预算。
    const uint64_t virtualReserve = 16ULL * 1024 * 1024;
    if (state.ullAvailVirtual <= virtualReserve || bytes > state.ullAvailVirtual - virtualReserve)
    {
        return false;
    }
    available = (std::min)(state.ullAvailPhys, state.ullAvailPageFile);
#elif defined(__APPLE__)
    size_t size = sizeof(total);
    if (sysctlbyname("hw.memsize", &total, &size, nullptr, 0) != 0)
    {
        return false;
    }
    vm_statistics64_data_t statistics = {};
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    const mach_port_t host = mach_host_self();
    const kern_return_t result = host_statistics64(host, HOST_VM_INFO64,
        reinterpret_cast<host_info64_t>(&statistics), &count);
    mach_port_deallocate(mach_task_self(), host);
    if (result != KERN_SUCCESS)
    {
        return false;
    }
    available = (static_cast<uint64_t>(statistics.free_count) + statistics.inactive_count) * vm_page_size;
#elif defined(__linux__) && !defined(__EMSCRIPTEN__)
    const long page = sysconf(_SC_PAGESIZE);
    const long freePages = sysconf(_SC_AVPHYS_PAGES);
    const long totalPages = sysconf(_SC_PHYS_PAGES);
    if (page <= 0 || freePages < 0 || totalPages <= 0)
    {
        return false;
    }
    total = static_cast<uint64_t>(totalPages) * page;
    available = static_cast<uint64_t>(freePages) * page;
#else
    return false;
#endif
    const uint64_t reserve = (std::max)(config.m_reserveBytes, total / 100 * config.m_reservePercent);
    return reserve < available && bytes <= available - reserve;
}