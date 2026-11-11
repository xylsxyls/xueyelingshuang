#include "ProcessWorkReceiveQueue.h"
#include <algorithm>
#include "ProcessWorkTime.h"
#include <cstring>
#include <limits>
#include <climits>
#include <new>

// 容纳64KiB正文、12字节包头、4字节批内长度和NUL，避免常用64KiB正文每次越界走临时分配。
static const size_t kReceiveBatchBytes = 65536 + 32;

/** 复制接收记录，短包以固定宽度首尾复制避免逐包调用通用memcpy
@param [out] target 至少bytes个可写字节，与source不重叠
@param [in] source 至少bytes个可读字节，允许未对齐；不会读取声明范围之外
@param [in] bytes 已校验的完整包头及正文长度，至少12字节
*/
static void CopyReceiveRecord(char* target, const char* source, size_t bytes)
{
    if (bytes <= 16)
    {
        std::memcpy(target, source, 8);
        std::memcpy(target + bytes - 8, source + bytes - 8, 8);
    }
    else if (bytes <= 32)
    {
        std::memcpy(target, source, 16);
        std::memcpy(target + bytes - 16, source + bytes - 16, 16);
    }
    else if (bytes <= 48)
    {
        std::memcpy(target, source, 16);
        std::memcpy(target + 16, source + 16, 16);
        std::memcpy(target + bytes - 16, source + bytes - 16, 16);
    }
    else
    {
        std::memcpy(target, source, bytes);
    }
}

ProcessWorkReceiveBatch::ProcessWorkReceiveBatch(size_t capacity) :
m_data(new (std::nothrow) char[capacity]), m_capacity(capacity), m_size(0)
{

}

ProcessWorkReceiveBatch::~ProcessWorkReceiveBatch()
{
    delete[] m_data;
}

ProcessWorkReceiveQueue::ProcessWorkReceiveQueue() :
m_building(nullptr), m_batchBytes(kReceiveBatchBytes), m_reserved(0), m_knownReleased(0), m_nextMemoryRetry(0), m_charged(0),
m_active(nullptr), m_readOffset(0), m_completed(0), m_released(0), m_cached(0), m_allocatedBytes(0), m_notified(false), m_storageGrows(0)
{

}

ProcessWorkReceiveQueue::~ProcessWorkReceiveQueue()
{
    clear();
}

bool ProcessWorkReceiveQueue::configure(const ProcessWorkMemoryConfig& config)
{
    try
    {
        if (hasPending() || !ProcessWorkMemory::valid(config))
        {
            return false;
        }
        clear();
        m_config = config;
        m_batchBytes = kReceiveBatchBytes;
        const uint64_t allocation = static_cast<uint64_t>(m_batchBytes) + sizeof(ProcessWorkReceiveBatch);
        const uint64_t reserve = allocation * config.m_receiveBatchReserve;
        LockFreeQueuePoolConfig pool;
        LockFreeQueuePoolConfig freePool;
        if (!ProcessWorkMemory::queueConfig(config, false, pool) || !ProcessWorkMemory::queueConfig(config, true, freePool))
        {
            return false;
        }
        if (config.m_receiveBatchReserve != 0)
        {
            pool.initialCapacity = (std::max)(pool.initialCapacity, static_cast<size_t>(config.m_receiveBatchReserve) + 32);
            freePool.initialCapacity = (std::max)(freePool.initialCapacity, static_cast<size_t>(config.m_receiveBatchReserve) + 32);
        }
        const uint64_t initial = (std::max)(static_cast<uint64_t>(pool.initialCapacity), static_cast<uint64_t>(freePool.initialCapacity));
        if (initial > (UINT64_MAX - 2 * 1024 * 1024 - static_cast<uint64_t>(config.m_receiveBatchReserve) * sizeof(ProcessWorkReceiveBatch*)) / 512)
        {
            return false;
        }
        // 两个指针队列及备用指针表也计入初始化前的系统压力检查，避免大预留绕过保护。
        const uint64_t queueReserve = static_cast<uint64_t>(initial) * 512 + 2 * 1024 * 1024 +
            static_cast<uint64_t>(config.m_receiveBatchReserve) * sizeof(ProcessWorkReceiveBatch*);
        if (reserve > UINT64_MAX - queueReserve ||
            ((reserve != 0 || initial > 64) && !ProcessWorkMemory::allows(config, reserve + queueReserve)))
        {
            return false;
        }
        m_ready.destroy();
        m_free.destroy();
        if (m_ready.configurePool(pool) != LockFreeQueueSuccess || m_free.configurePool(freePool) != LockFreeQueueSuccess ||
            m_ready.setGrowPolicy(config.m_queueGrow) != LockFreeQueueSuccess || m_free.setGrowPolicy(config.m_freeQueueGrow) != LockFreeQueueSuccess ||
            m_ready.setShrinkPolicy(config.m_queueShrink) != LockFreeQueueSuccess || m_free.setShrinkPolicy(config.m_freeQueueShrink) != LockFreeQueueSuccess ||
            m_ready.tryInit() != LockFreeQueueSuccess || m_free.tryInit() != LockFreeQueueSuccess)
        {
            m_ready.destroy();
            m_free.destroy();
            return false;
        }
        try
        {
            m_unused.reserve(config.m_receiveBatchReserve);
        }
        catch (...)
        {
            m_ready.destroy();
            m_free.destroy();
            return false;
        }
        for (uint32_t i = 0; i < config.m_receiveBatchReserve; ++i)
        {
            ProcessWorkReceiveBatch* batch = new (std::nothrow) ProcessWorkReceiveBatch(m_batchBytes);
            if (batch == nullptr || batch->m_data == nullptr)
            {
                delete batch;
                clear();
                m_ready.destroy();
                m_free.destroy();
                return false;
            }
            // 初始化期间触及预留正文页；性能计时不包含首次触页。
            std::memset(batch->m_data, 0, m_batchBytes);
            m_allocatedBytes.fetch_add(allocation, std::memory_order_relaxed);
            // 只在没有已归还热块时启用备用块，避免低积压也循环触及全部预分配内存。
            m_unused.push_back(batch);
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

ProcessWorkReceiveBatch* ProcessWorkReceiveQueue::acquire(size_t bytes)
{
    ProcessWorkReceiveBatch* batch = nullptr;
    if (bytes <= m_batchBytes && m_free.pop(&batch))
    {
        m_cached.fetch_sub(1, std::memory_order_relaxed);
        batch->m_size = 0;
        return batch;
    }
    if (bytes <= m_batchBytes && !m_unused.empty())
    {
        batch = m_unused.back();
        m_unused.pop_back();
        batch->m_size = 0;
        return batch;
    }
    size_t capacity = (std::max)(bytes, m_batchBytes);
    uint64_t allocation = static_cast<uint64_t>(capacity) + sizeof(ProcessWorkReceiveBatch);
    const uint64_t limit = static_cast<uint64_t>(INT64_MAX);
    if (allocation > limit)
    {
        return nullptr;
    }
    // 只保护实际存储总量的数值范围；必要时回收空闲块，不调整或占用消息积压额度。
    while (m_allocatedBytes.load(std::memory_order_acquire) > limit - allocation)
    {
        if (!m_unused.empty())
        {
            batch = m_unused.back();
            m_unused.pop_back();
        }
        else if (m_free.pop(&batch))
        {
            m_cached.fetch_sub(1, std::memory_order_relaxed);
        }
        else
        {
            return nullptr;
        }
        destroy(batch);
    }
    const uint64_t now = ProcessWorkTime::milliseconds();
    if (now < m_nextMemoryRetry)
    {
        return nullptr;
    }
    try
    {
        const uint64_t before = m_allocatedBytes.load(std::memory_order_acquire);
        if (m_config.m_policy)
        {
            ProcessWorkMemoryGrowContext context;
            context.m_kind = ProcessWorkReceiveBuffers;
            context.m_initialCapacity = (m_batchBytes + sizeof(ProcessWorkReceiveBatch)) * static_cast<uint64_t>(m_config.m_receiveBatchReserve);
            context.m_currentCapacity = before;
            context.m_successfulGrowCount = m_storageGrows;
            context.m_requiredGrowth = allocation;
            const uint64_t amount = (std::min)(m_config.m_policy->grow(context), limit - before);
            if (amount < allocation || amount > (std::numeric_limits<size_t>::max)())
            {
                m_nextMemoryRetry = now + m_config.m_retryMilliseconds;
                return nullptr;
            }
            allocation = amount;
            capacity = static_cast<size_t>(allocation - sizeof(ProcessWorkReceiveBatch));
        }
        if (!ProcessWorkMemory::allows(m_config, allocation))
        {
            m_nextMemoryRetry = now + m_config.m_retryMilliseconds;
            return nullptr;
        }
        batch = new (std::nothrow) ProcessWorkReceiveBatch(capacity);
        if (batch == nullptr || batch->m_data == nullptr)
        {
            delete batch;
            m_nextMemoryRetry = now + m_config.m_retryMilliseconds;
            return nullptr;
        }
        const uint64_t previous = m_allocatedBytes.fetch_add(allocation, std::memory_order_release);
        ++m_storageGrows;
        if (m_config.m_policy)
        {
            ProcessWorkMemoryGrowth growth;
            growth.m_kind = ProcessWorkReceiveBuffers;
            growth.m_previousCapacity = previous;
            growth.m_addedCapacity = allocation;
            growth.m_currentCapacity = previous + allocation;
            growth.m_successfulGrowCount = m_storageGrows;
            try
            {
                m_config.m_policy->grown(growth);
            }
            catch (...)
            {
                // 实际存储已建立；观察者异常不影响消息接纳，不重复释放或通知。
            }
        }
        return batch;
    }
    catch (...)
    {
        m_nextMemoryRetry = now + m_config.m_retryMilliseconds;
        return nullptr;
    }
}

bool ProcessWorkReceiveQueue::append(const char* source, size_t bytes)
{
    if (source == nullptr || bytes < 12 || bytes > static_cast<size_t>(INT32_MAX) ||
        bytes > (std::numeric_limits<size_t>::max)() - 5)
    {
        return false;
    }
    const bool limited = m_config.m_localQueueBytes != -1;
    const uint64_t charge = limited ? static_cast<uint64_t>(bytes) + 265 : 1;
    if (limited)
    {
        const uint64_t maximum = static_cast<uint64_t>(m_config.m_localQueueBytes);
        if (charge > maximum)
        {
            return false;
        }
        if (m_reserved - m_knownReleased > maximum - charge)
        {
            m_knownReleased = m_released.load(std::memory_order_acquire);
            if (m_reserved - m_knownReleased > maximum - charge)
            {
                return false;
            }
        }
    }
    const size_t record = bytes + 5;
    if (m_building != nullptr && m_building->m_capacity - m_building->m_size < record && !flush())
    {
        return false;
    }
    if (m_building == nullptr)
    {
        m_building = acquire(record);
        if (m_building == nullptr)
        {
            return false;
        }
    }
    const uint32_t length = static_cast<uint32_t>(bytes + 1);
    char* target = m_building->m_data + m_building->m_size;
    std::memcpy(target, &length, sizeof(length));
    CopyReceiveRecord(target + sizeof(length), source, bytes);
    target[sizeof(length) + bytes] = '\0';
    m_building->m_size += record;
    m_reserved += charge;
    m_charged.store(m_reserved, std::memory_order_release);
    return true;
}

bool ProcessWorkReceiveQueue::flush()
{
    if (m_building == nullptr)
    {
        return true;
    }
    if (m_ready.tryPushAndFence(m_building) != LockFreeQueueSuccess)
    {
        return false;
    }
    m_building = nullptr;
    if (!m_notified.load(std::memory_order_acquire) && !m_notified.exchange(true, std::memory_order_acq_rel))
    {
        m_notification.trySignal(true, false);
    }
    return true;
}

void ProcessWorkReceiveQueue::recycle(ProcessWorkReceiveBatch* batch)
{
    if (batch->m_capacity == m_batchBytes &&
        m_cached.load(std::memory_order_relaxed) < (std::max)(uint32_t(8), m_config.m_receiveBatchReserve))
    {
        m_cached.fetch_add(1, std::memory_order_relaxed);
        if (m_free.tryPush(batch) == LockFreeQueueSuccess)
        {
            return;
        }
        m_cached.fetch_sub(1, std::memory_order_relaxed);
    }
    destroy(batch);
}

void ProcessWorkReceiveQueue::destroy(ProcessWorkReceiveBatch* batch)
{
    if (batch != nullptr)
    {
        const uint64_t allocation = static_cast<uint64_t>(batch->m_capacity) + sizeof(ProcessWorkReceiveBatch);
        delete batch;
        m_allocatedBytes.fetch_sub(allocation, std::memory_order_release);
    }
}

void ProcessWorkReceiveQueue::wake()
{
    m_notification.trySignal(true, false);
}

void ProcessWorkReceiveQueue::clear()
{
    while (!m_unused.empty())
    {
        destroy(m_unused.back());
        m_unused.pop_back();
    }
    destroy(m_building);
    m_building = nullptr;
    destroy(m_active);
    m_active = nullptr;
    ProcessWorkReceiveBatch* batch = nullptr;
    while (m_ready.pop(&batch))
    {
        destroy(batch);
    }
    while (m_free.pop(&batch))
    {
        destroy(batch);
    }
    m_readOffset = 0;
    m_reserved = 0;
    m_knownReleased = 0;
    m_completed = 0;
    m_nextMemoryRetry = 0;
    m_charged.store(0);
    m_released.store(0);
    m_cached.store(0);
    m_allocatedBytes.store(0);
    m_storageGrows = 0;
    m_notified.store(false);
    m_notification.reset();
}

uint64_t ProcessWorkReceiveQueue::used() const
{
    if (m_config.m_localQueueBytes == -1)
    {
        return 0;
    }
    // 先读取完成端再读取发布端，避免查询途中消费赶上旧发布快照导致无符号下溢。
    const uint64_t released = m_released.load(std::memory_order_acquire);
    return m_charged.load(std::memory_order_acquire) - released;
}

bool ProcessWorkReceiveQueue::hasPending() const
{
    const uint64_t released = m_released.load(std::memory_order_acquire);
    return m_charged.load(std::memory_order_acquire) != released;
}

Semaphore& ProcessWorkReceiveQueue::notification()
{
    return m_notification;
}