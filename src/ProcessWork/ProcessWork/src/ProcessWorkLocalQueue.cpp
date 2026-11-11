#include "ProcessWorkLocalQueue.h"
#include <algorithm>
#include "ProcessWorkTime.h"
#include "CSystem/CSystemAPI.h"
#include <cstring>
#include <limits>
#include <new>

ProcessWorkLocalQueue::ProcessWorkLocalQueue() :
m_notified(false), m_pending(0), m_nextMemoryCheck(0), m_memoryAvailable(true), m_prefixBatches(0)
{

}

ProcessWorkLocalQueue::~ProcessWorkLocalQueue()
{
    clear();
}

bool ProcessWorkLocalQueue::configure(const ProcessWorkMemoryConfig& config)
{
    try
    {
        if (m_pending.load() != 0 || !ProcessWorkMemory::valid(config))
        {
            return false;
        }
        LockFreeQueuePoolConfig pool;
        if (!ProcessWorkMemory::queueConfig(config, false, pool))
        {
            return false;
        }
        const uint64_t reserve = static_cast<uint64_t>(config.m_postBufferReserve) * sizeof(ProcessWorkPacketSlot);
        // 指针队列还包含节点、发布段和线程记录，不能只检查小包正文池就一次申请巨型队列。
        if (pool.initialCapacity > (UINT64_MAX - 1024 * 1024) / 256)
        {
            return false;
        }
        const uint64_t queueReserve = static_cast<uint64_t>(pool.initialCapacity) * 256 + 1024 * 1024;
        if (reserve > UINT64_MAX - queueReserve ||
            ((reserve != 0 || pool.initialCapacity > 64) && !ProcessWorkMemory::allows(config, reserve + queueReserve)) ||
            !m_packetPool.configure(config))
        {
            return false;
        }
        m_config = config;
        m_nextMemoryCheck.store(0);
        m_memoryAvailable.store(true);
        m_values.destroy();
        if (m_values.configurePool(pool) != LockFreeQueueSuccess ||
            m_values.setGrowPolicy(config.m_queueGrow) != LockFreeQueueSuccess ||
            m_values.setShrinkPolicy(config.m_queueShrink) != LockFreeQueueSuccess || m_values.tryInit() != LockFreeQueueSuccess)
        {
            m_values.destroy();
            m_packetPool.configure(0);
            return false;
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

char* ProcessWorkLocalQueue::allocate(size_t bytes, LockFreeQueueResult& result)
{
    return allocate(bytes, result, false);
}

char* ProcessWorkLocalQueue::allocate(size_t bytes, LockFreeQueueResult& result, bool retryContention)
{
    result = LockFreeQueueInvalidConfiguration;
    if (bytes > (std::numeric_limits<size_t>::max)() - sizeof(uint64_t) ||
        static_cast<uint64_t>(bytes) > (std::numeric_limits<uint64_t>::max)() - 264)
    {
        return nullptr;
    }
    const bool limited = m_config.m_localQueueBytes != -1;
    // 无限额度只保留对象存活计数，不计算正文的积压字节，也不携带计费前缀。
    const uint64_t charge = limited ? static_cast<uint64_t>(bytes) + 264 : 1;
    uint64_t used = m_pending.load(std::memory_order_relaxed);
    if (charge > static_cast<uint64_t>(INT64_MAX))
    {
        result = LockFreeQueueInvalidConfiguration;
        return nullptr;
    }
    const uint64_t hardLimit = limited ? static_cast<uint64_t>(m_config.m_localQueueBytes) : UINT64_MAX;
    if (charge > hardLimit)
    {
        // 单项永远放不进固定上限，等待消费者不能解决，返回参数错误避免普通post永久重试。
        return nullptr;
    }
    if (used > hardLimit - charge)
    {
        result = LockFreeQueueCapacityLimit;
        return nullptr;
    }
    if (m_nextMemoryCheck.load(std::memory_order_relaxed) == 0 || !m_memoryAvailable.load(std::memory_order_relaxed) ||
        (limited ? (used >> 22) != ((used + charge) >> 22) : (used & 4095U) == 0) || bytes >= 4 * 1024 * 1024)
    {
        refreshMemory();
    }
    if (!m_memoryAvailable.load(std::memory_order_relaxed) ||
        (bytes >= 4 * 1024 * 1024 && !ProcessWorkMemory::allows(m_config, bytes)))
    {
        result = LockFreeQueueResourceUnavailable;
        return nullptr;
    }
    uint32_t attempts = 0;
    uint32_t contention = 0;
    while (retryContention || attempts++ < 16)
    {
        if (used > hardLimit - charge)
        {
            result = LockFreeQueueCapacityLimit;
            return nullptr;
        }
        if (limited && !m_pending.compare_exchange_strong(used, used + charge, std::memory_order_acq_rel, std::memory_order_relaxed))
        {
            if (retryContention && ++contention == 64)
            {
                // 普通post不因竞争次数拒绝；连续失败时实际休眠，给同优先级消费者运行机会。
                CSystem::Sleep(1);
                contention = 0;
            }
            continue;
        }
        if (!limited)
        {
            // 无额度争抢，仅记录尚未释放的所有权；不因其他生产者更新计数而重试。
            m_pending.fetch_add(1, std::memory_order_relaxed);
        }
        const size_t prefix = limited ? sizeof(uint64_t) : 0;
        char* allocation = m_packetPool.acquire(bytes + prefix);
        if (allocation == nullptr)
        {
            allocation = new (std::nothrow) char[bytes + prefix];
        }
        if (allocation != nullptr)
        {
            if (limited)
            {
                std::memcpy(allocation, &charge, sizeof(charge));
            }
            result = LockFreeQueueSuccess;
            return allocation + prefix;
        }
        m_pending.fetch_sub(charge, std::memory_order_acq_rel);
        result = LockFreeQueueAllocationFailure;
        return nullptr;
    }
    // 最后一次CAS也可能观察到别的生产者占满预算，满载不能误报竞争Busy。
    used = m_pending.load(std::memory_order_relaxed);
    result = used > hardLimit - charge ? LockFreeQueueCapacityLimit : LockFreeQueueTemporaryUnavailable;
    return nullptr;
}

bool ProcessWorkLocalQueue::hasPending() const
{
    return m_pending.load(std::memory_order_acquire) != 0;
}

void ProcessWorkLocalQueue::release(char* packet)
{
    if (packet != nullptr)
    {
        const bool limited = m_config.m_localQueueBytes != -1;
        char* allocation = packet - (limited ? sizeof(uint64_t) : 0);
        uint64_t charge = 1;
        if (limited)
        {
            std::memcpy(&charge, allocation, sizeof(charge));
        }
        if (!m_packetPool.release(allocation))
        {
            delete[] allocation;
        }
        m_pending.fetch_sub(charge, std::memory_order_acq_rel);
    }
}

void ProcessWorkLocalQueue::releasePrefix(char** packets, size_t count)
{
    if (packets == nullptr)
    {
        return;
    }
    uint64_t total = 0;
    for (size_t i = 0; i < count; ++i)
    {
        if (packets[i] == nullptr)
        {
            continue;
        }
        const bool limited = m_config.m_localQueueBytes != -1;
        char* allocation = packets[i] - (limited ? sizeof(uint64_t) : 0);
        uint64_t charge = 1;
        if (limited)
        {
            std::memcpy(&charge, allocation, sizeof(charge));
        }
        total += charge;
        packets[i] = nullptr;
        if (!m_packetPool.release(allocation))
        {
            delete[] allocation;
        }
    }
    if (total != 0)
    {
        m_pending.fetch_sub(total, std::memory_order_acq_rel);
    }
}

LockFreeQueueResult ProcessWorkLocalQueue::submit(char* packet)
{
    if (packet == nullptr)
    {
        return LockFreeQueueInvalidConfiguration;
    }
    const LockFreeQueueResult result = m_values.tryPushAndFence(packet);
    if (result == LockFreeQueueSuccess)
    {
        notify();
    }
    return result;
}

bool ProcessWorkLocalQueue::take(char** packet, uint32_t milliseconds)
{
    if (packet == nullptr)
    {
        return false;
    }
    if (m_values.pop(packet))
    {
        return true;
    }
    m_notified.store(false, std::memory_order_seq_cst);
    if (m_values.pop(packet))
    {
        return true;
    }
    m_notification.waitResult(milliseconds);
    return m_values.pop(packet);
}

bool ProcessWorkLocalQueue::pop(char** packet)
{
    return packet != nullptr && m_values.pop(packet);
}

bool ProcessWorkLocalQueue::takePrefix(char** packets, size_t maximum, size_t& consumed, uint32_t milliseconds)
{
    consumed = 0;
    if (packets == nullptr || maximum == 0 || maximum > 64 || !take(packets, milliseconds))
    {
        return false;
    }
    consumed = 1;
    if (maximum > 1)
    {
        size_t additional = 0;
        m_values.popReadyPrefix(packets + 1, maximum - 1, additional);
        consumed += additional;
    }
    if ((++m_prefixBatches & 15U) == 0)
    {
        m_values.maintain(8);
    }
    return true;
}

void ProcessWorkLocalQueue::notify()
{
    if (!m_notified.load(std::memory_order_acquire) && !m_notified.exchange(true, std::memory_order_acq_rel))
    {
        m_notification.trySignal(true, false);
    }
}

void ProcessWorkLocalQueue::wake()
{
    m_notification.trySignal(true, false);
}

void ProcessWorkLocalQueue::clear()
{
    char* packet = nullptr;
    while (m_values.pop(&packet))
    {
        release(packet);
    }
    m_notified.store(false);
    m_prefixBatches = 0;
    m_notification.reset();
}

bool ProcessWorkLocalQueue::empty() const
{
    return m_values.empty();
}

uint64_t ProcessWorkLocalQueue::used() const
{
    return m_config.m_localQueueBytes == -1 ? 0 : m_pending.load(std::memory_order_relaxed);
}

void ProcessWorkLocalQueue::refreshMemory()
{
    try
    {
        const uint64_t now = ProcessWorkTime::milliseconds();
        uint64_t next = m_nextMemoryCheck.load(std::memory_order_relaxed);
        if (now < next || !m_nextMemoryCheck.compare_exchange_strong(next, now + m_config.m_retryMilliseconds, std::memory_order_relaxed))
        {
            return;
        }
        const uint64_t checkBytes = m_config.m_localQueueBytes == -1 ? uint64_t(4) * 1024 * 1024 :
            (std::min)(static_cast<uint64_t>(m_config.m_localQueueBytes), uint64_t(4) * 1024 * 1024);
        m_memoryAvailable.store(ProcessWorkMemory::allows(m_config, checkBytes), std::memory_order_relaxed);
    }
    catch (...)
    {
        m_memoryAvailable.store(false, std::memory_order_relaxed);
    }
}