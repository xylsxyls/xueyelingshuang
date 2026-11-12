#pragma once
#include <cstring>

inline uint32_t LibuvBase::producerToken()
{
    return m_producerToken != 0 ? m_producerToken : allocateProducerToken();
}

inline void LibuvBase::copySmall(char* target, const char* source, uint32_t size)
{
    if (size == 16)
    {
        std::memcpy(target, source, 16);
        return;
    }
    if (size >= 8)
    {
        std::memcpy(target, source, 8);
        target += 8;
        source += 8;
        size -= 8;
    }
    if (size >= 4)
    {
        std::memcpy(target, source, 4);
        target += 4;
        source += 4;
        size -= 4;
    }
    if (size >= 2)
    {
        std::memcpy(target, source, 2);
        target += 2;
        source += 2;
        size -= 2;
    }
    if (size != 0)
    {
        *target = *source;
    }
}

// 短小的借用、预算及初始化操作保持同一实现，供编译器在逐条发送路径内联
inline bool LibuvLifecycleGate::valid() const
{
    return m_calls.is_lock_free();
}

inline bool LibuvLifecycleGate::acquire()
{
    return tryAcquire(true) == 0;
}

inline int32_t LibuvLifecycleGate::tryAcquire(bool retryContention)
{
    if ((m_calls.load(std::memory_order_acquire) & 0x80000000U) != 0)
    {
        return UV_ECANCELED;
    }
    const uint32_t token = LibuvBase::producerToken();
    if (token != UINT32_MAX)
    {
        LibuvLifecycleSlot& slot = m_slots[token % 32];
        uint32_t owner = slot.m_token.load(std::memory_order_acquire);
        if (owner == 0 && slot.m_token.compare_exchange_strong(owner, token,
            std::memory_order_acq_rel, std::memory_order_acquire))
        {
            owner = token;
        }
        if (owner == token)
        {
            const uint32_t count = slot.m_calls.load(std::memory_order_relaxed);
            if (count == 0x7fffffffU)
            {
                return UV_EOVERFLOW;
            }
            // 发布借用与关闭扫描同属SC序：若关闭先看到零，下面必须看到关闭；
            // 若本次已验证开放，关闭扫描必须等本槽归零，不能释放借用中的上下文。
            slot.m_calls.store(count + 1, std::memory_order_seq_cst);
            if ((m_calls.load(std::memory_order_seq_cst) & 0x80000000U) != 0)
            {
                slot.m_calls.store(count, std::memory_order_release);
                return UV_ECANCELED;
            }
            return 0;
        }
    }
    uint32_t count = m_calls.load(std::memory_order_acquire);
    uint32_t attempts = 0;
    while (count < 0x7fffffffU)
    {
        if (m_calls.compare_exchange_weak(count, count + 1, std::memory_order_acquire, std::memory_order_acquire))
        {
            return 0;
        }
        if (!retryContention && count < 0x7fffffffU)
        {
            return UV_EAGAIN;
        }
        LibuvBase::WaitForContention(attempts);
    }
    return (count & 0x80000000U) != 0 ? UV_ECANCELED : UV_EOVERFLOW;
}

inline void LibuvLifecycleGate::release()
{
    const uint32_t token = LibuvBase::producerToken();
    if (token != UINT32_MAX)
    {
        LibuvLifecycleSlot& slot = m_slots[token % 32];
        if (slot.m_token.load(std::memory_order_acquire) == token)
        {
            slot.m_calls.store(slot.m_calls.load(std::memory_order_relaxed) - 1,
                std::memory_order_release);
            return;
        }
    }
    m_calls.fetch_sub(1, std::memory_order_release);
}

inline LibuvCall::LibuvCall(LibuvLifecycleGate& gate, bool retryContention) :
m_gate(gate),
m_status(gate.tryAcquire(retryContention))
{

}

inline LibuvCall::~LibuvCall()
{
    if (m_status == 0)
    {
        m_gate.release();
    }
}

inline bool LibuvCall::accepted() const
{
    return m_status == 0;
}

inline int32_t LibuvCall::status() const
{
    return m_status;
}

inline LibuvPendingSend::LibuvPendingSend() :
m_buffer(nullptr),
m_target(nullptr),
m_generation(0),
m_close(false),
m_charge(0),
m_wireSize(0)
{
    std::memset(m_inlineBytes, 0, sizeof(m_inlineBytes));
}

inline int32_t LibuvBase::tryReserveSend(uint32_t charge, bool retryContention)
{
    (void)retryContention;
    // 一次原子预留避免多发送线程反复竞争CAS；失败项立即归还，尚未发布数据。
    // 每次调用由32个有界分片或31位后备计数保护，临时加数总和远小于UINT64_MAX。
    const uint64_t kLimit = m_settings.m_send.m_maximumPendingBytes;
    if (charge == 0 || charge > kLimit)
    {
        return UV_EINVAL;
    }
    const uint64_t used = m_pendingBytes.fetch_add(charge, std::memory_order_relaxed);
    if (used <= kLimit - charge)
    {
        return 0;
    }
    m_pendingBytes.fetch_sub(charge, std::memory_order_relaxed);
    return UV_ENOBUFS;
}

inline void LibuvBase::releaseSend(uint32_t charge)
{
    if (charge != 0)
    {
        m_pendingBytes.fetch_sub(charge, std::memory_order_relaxed);
    }
}

inline bool LibuvSendLane::push(const LibuvPendingSend& pending)
{
    if (m_write - m_seenRead >= m_capacity)
    {
        m_seenRead = m_consumed.load(std::memory_order_acquire);
        if (m_write - m_seenRead >= m_capacity)
        {
            return false;
        }
    }
    m_items[m_write & (m_capacity - 1)] = pending;
    ++m_write;
    m_published.store(m_write, std::memory_order_release);
    // 独立发布每一条消息；跨平台保证发布先于uv_async_send，不能仅靠编译器屏障。
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

inline LockFreeQueueResult LibuvBase::tryQueueSend(const LibuvPendingSend& pending, bool retryContention)
{
    const uint32_t token = producerToken();
    if (token != UINT32_MAX && m_sendLane.m_capacity != 0)
    {
        uint32_t owner = m_sendLane.m_token.load(std::memory_order_acquire);
        if (owner == 0 && m_sendLane.m_token.compare_exchange_strong(owner, token,
            std::memory_order_acq_rel, std::memory_order_acquire))
        {
            owner = token;
        }
        if (owner == token)
        {
            return m_sendLane.push(pending) ? LockFreeQueueSuccess : LockFreeQueueCapacityLimit;
        }
    }
    if (token != UINT32_MAX && m_settings.m_send.m_producerCacheCapacity != 0)
    {
        const uint32_t index = m_producerMask != UINT32_MAX ? token & m_producerMask :
            token % m_settings.m_send.m_producerCacheCapacity;
        LibuvSendProducer& slot = m_sendProducers[index];
        uint32_t owner = slot.m_token.load(std::memory_order_acquire);
        if (owner == 0 && slot.m_token.compare_exchange_strong(owner, token,
            std::memory_order_acq_rel, std::memory_order_acquire))
        {
            owner = token;
        }
        // 令牌不复用、不换主；其他线程绝不访问此槽的非原子上下文，不等待暂停的缓存拥有者。
        if (owner == token && slot.m_lane.m_capacity != 0)
        {
            return slot.m_lane.push(pending) ? LockFreeQueueSuccess : LockFreeQueueCapacityLimit;
        }
        if (owner == token && !slot.m_busy)
        {
            slot.m_busy = true;
            if (!slot.m_producer.attached())
            {
                m_queue->tryAcquireProducer(slot.m_producer, retryContention, false);
            }
            if (slot.m_producer.attached())
            {
                const LockFreeQueueResult result = slot.m_producer.tryPushAndFence(pending, retryContention, false);
                slot.m_busy = false;
                return result;
            }
            slot.m_busy = false;
        }
    }
    return m_queue->tryPushAndFence(pending, retryContention, false);
}