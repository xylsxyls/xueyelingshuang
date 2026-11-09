#include "SemaphoreCount.h"
#include "SemaphoreWake.h"

SemaphoreCount::SemaphoreCount(const SemaphorePoolConfig& config) :
SemaphoreEventPool(config)
{

}

SemaphoreResult SemaphoreCount::begin(std::atomic<uint64_t>& state, SemaphoreEventRecord*& record, bool probe)
{
    record = nullptr;
    size_t budget = 64;
    SemaphoreEventPool::reclaim(budget);
    if (!SemaphoreEventPool::enter())
    {
        return SemaphoreResult(SEMAPHORE_CAPACITY);
    }
    const SemaphoreResult result = beginProtected(state, record, probe);
    SemaphoreEventPool::leave();
    return result;
}

SemaphoreResult SemaphoreCount::signal(std::atomic<uint64_t>& state, SemaphoreWake& wake, bool notify)
{
    size_t budget = 64;
    SemaphoreEventPool::reclaim(budget);
    if (!SemaphoreEventPool::enter())
    {
        return SemaphoreResult(SEMAPHORE_CAPACITY);
    }
    const SemaphoreResult result = signalProtected(state, wake, notify);
    SemaphoreEventPool::leave();
    return result;
}

bool SemaphoreCount::finish(std::atomic<uint64_t>& state, SemaphoreEventRecord* record)
{
    uint32_t waiting = 0;
    record->m_state.compare_exchange_strong(waiting, 2);
    const bool notified = waiting == 1;
    record->m_next.fetch_or(uintptr_t(1));
    if (SemaphoreEventPool::enter())
    {
        sweep(state);
        record->m_state.store(3, std::memory_order_release);
        size_t budget = 64;
        SemaphoreEventPool::reclaim(budget, true);
    }
    else
    {
        // 最后一次访问后其他通知者仍可帮助摘除；不等待一个暂停的读者。
        record->m_state.store(3, std::memory_order_release);
    }
    return notified;
}

void SemaphoreCount::clear(std::atomic<uint64_t>& state)
{
    const uint64_t current = state.exchange(0);
    if (current <= directLimit())
    {
        return;
    }
    SemaphoreEventRecord* record = node(current);
    if ((current & 1) != 0)
    {
        discard(record, true);
        return;
    }
    while (record != nullptr)
    {
        SemaphoreEventRecord* next = reinterpret_cast<SemaphoreEventRecord*>(record->m_next.load() & ~uintptr_t(1));
        discard(record);
        record = next;
    }
}

SemaphorePoolStats SemaphoreCount::statistics() const
{
    return SemaphoreEventPool::statistics();
}

SemaphoreResult SemaphoreCount::maintain(size_t& released)
{
    return SemaphoreEventPool::maintain(released);
}

uint64_t SemaphoreCount::directLimit()
{
    return UINT64_MAX >> 1;
}

SemaphoreResult SemaphoreCount::acquire(SemaphoreEventRecord*& record)
{
    record = SemaphoreEventPool::takeFree();
    bool evaluated = false;
    if (record == nullptr)
    {
        evaluated = true;
        const SemaphoreResult grown = SemaphoreEventPool::grow();
        record = SemaphoreEventPool::takeFree();
        if (record == nullptr)
        {
            return grown.status == SEMAPHORE_OK ? SemaphoreResult(SEMAPHORE_RETRY) : grown;
        }
    }
    const uint64_t address = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(record));
    if (address > directLimit() || (address & 3) != 0)
    {
        discard(record);
        record = nullptr;
        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
    }
    if (!evaluated && SemaphoreEventPool::m_config.freeThreshold > 0 &&
        SemaphoreEventPool::m_freeCount.load() <= SemaphoreEventPool::m_config.freeThreshold)
    {
        // 与事件池遵守同一阈值合同；提前扩容失败不撤销已经取得的记录。
        SemaphoreEventPool::grow();
    }
    record->m_state.store(0, std::memory_order_relaxed);
    record->m_idleSince = 0;
    return SemaphoreResult();
}

void SemaphoreCount::discard(SemaphoreEventRecord* record, bool large)
{
    record->m_state.store(large ? 5 : 3, std::memory_order_release);
    SemaphoreEventPool::retire(record);
}

SemaphoreResult SemaphoreCount::beginProtected(
    std::atomic<uint64_t>& state, SemaphoreEventRecord*& record, bool probe)
{
    SemaphoreEventRecord* candidate = nullptr;
    for (;;)
    {
        uint64_t current = state.load();
        if (current != 0 && current <= directLimit())
        {
            if (!state.compare_exchange_weak(current, current - 1))
            {
                continue;
            }
            if (candidate != nullptr)
            {
                discard(candidate);
            }
            return SemaphoreResult(SEMAPHORE_OK, true);
        }
        if (current > directLimit() && (current & 1) != 0)
        {
            const SemaphoreResult result = replaceLarge(state, current, node(current)->m_idleSince - 1);
            if (result.status == SEMAPHORE_RETRY)
            {
                continue;
            }
            if (candidate != nullptr)
            {
                discard(candidate);
            }
            return result;
        }
        if (probe)
        {
            return SemaphoreResult(SEMAPHORE_TIMEOUT);
        }
        if (candidate == nullptr)
        {
            const SemaphoreResult result = acquire(candidate);
            if (result.status != SEMAPHORE_OK)
            {
                return result;
            }
        }
        candidate->m_next.store(current == 0 ? 0 : reinterpret_cast<uintptr_t>(node(current)), std::memory_order_relaxed);
        if (state.compare_exchange_weak(current, encode(candidate)))
        {
            record = candidate;
            return SemaphoreResult();
        }
    }
}

SemaphoreResult SemaphoreCount::signalProtected(std::atomic<uint64_t>& state, SemaphoreWake& wake, bool notify)
{
    for (;;)
    {
        uint64_t current = state.load();
        if (current < directLimit())
        {
            if (state.compare_exchange_weak(current, current + 1))
            {
                // 登记链可能刚被其他线程清空，普通竞争等待者仍需收到公共提示。
                SemaphoreResult result = notify ? wake.notify() : SemaphoreResult();
                result.committed = true;
                return result;
            }
            continue;
        }
        if (current == directLimit() || (current & 1) != 0)
        {
            const uint64_t count = current == directLimit() ? current : node(current)->m_idleSince;
            if (count == UINT64_MAX)
            {
                return SemaphoreResult(SEMAPHORE_OVERFLOW);
            }
            const SemaphoreResult result = replaceLarge(state, current, count + 1);
            if (result.status != SEMAPHORE_RETRY)
            {
                if (result.committed && notify)
                {
                    SemaphoreResult woke = wake.notify();
                    woke.committed = true;
                    return woke;
                }
                return result;
            }
            continue;
        }
        SemaphoreEventRecord* selected = SemaphoreEventPool::oldest(reinterpret_cast<uintptr_t>(node(current)));
        if (selected == nullptr)
        {
            sweep(state);
            continue;
        }
        uint32_t waiting = 0;
        if (!selected->m_state.compare_exchange_strong(waiting, 1))
        {
            continue;
        }
        // 此CAS已经交付本次许可。即使接收者暂停或系统提示失败，后来者也不能夺走。
        SemaphoreResult result = notify ? wake.notify(wake.canWaitAddress() ? &selected->m_state : nullptr) : SemaphoreResult();
        result.committed = true;
        sweep(state);
        return result;
    }
}

SemaphoreResult SemaphoreCount::replaceLarge(std::atomic<uint64_t>& state, uint64_t expected, uint64_t value)
{
    SemaphoreEventRecord* replacement = nullptr;
    uint64_t desired = value;
    if (value > directLimit())
    {
        const SemaphoreResult result = acquire(replacement);
        if (result.status != SEMAPHORE_OK)
        {
            return result;
        }
        // 大计数记录发布后不再修改正文；CAS替换后再由读者静止协议回收。
        replacement->m_idleSince = value;
        desired = encode(replacement, true);
    }
    const uint64_t previous = expected;
    if (!state.compare_exchange_strong(expected, desired))
    {
        if (replacement != nullptr)
        {
            discard(replacement, true);
        }
        return SemaphoreResult(SEMAPHORE_RETRY);
    }
    if (previous > directLimit())
    {
        discard(node(previous), true);
    }
    return SemaphoreResult(SEMAPHORE_OK, true);
}

void SemaphoreCount::sweep(std::atomic<uint64_t>& state)
{
    for (;;)
    {
        uint64_t root = state.load();
        if (root <= directLimit() || (root & 1) != 0)
        {
            return;
        }
        std::atomic<uintptr_t>* previous = nullptr;
        uintptr_t current = reinterpret_cast<uintptr_t>(node(root));
        bool restart = false;
        while (current != 0)
        {
            SemaphoreEventRecord* item = reinterpret_cast<SemaphoreEventRecord*>(current);
            uintptr_t next = item->m_next.load();
            if (item->m_state.load() != 0)
            {
                next = item->m_next.fetch_or(uintptr_t(1)) | uintptr_t(1);
            }
            if ((next & 1) != 0)
            {
                const uintptr_t successor = next & ~uintptr_t(1);
                bool removed = false;
                if (previous == nullptr)
                {
                    uint64_t expected = encode(item);
                    removed = state.compare_exchange_strong(expected,
                        successor == 0 ? 0 : encode(reinterpret_cast<SemaphoreEventRecord*>(successor)));
                }
                else
                {
                    removed = previous->compare_exchange_strong(current, successor);
                }
                if (!removed)
                {
                    restart = true;
                    break;
                }
                SemaphoreEventPool::retire(item);
                current = successor;
            }
            else
            {
                previous = &item->m_next;
                current = next;
            }
        }
        if (!restart)
        {
            return;
        }
    }
}

SemaphoreEventRecord* SemaphoreCount::node(uint64_t state)
{
    return reinterpret_cast<SemaphoreEventRecord*>(static_cast<uintptr_t>(state & directLimit() & ~uint64_t(1)));
}

uint64_t SemaphoreCount::encode(SemaphoreEventRecord* record, bool large)
{
    return (uint64_t(1) << 63) | static_cast<uint64_t>(reinterpret_cast<uintptr_t>(record)) | (large ? 1 : 0);
}