#include "SemaphoreEventPool.h"
#include "SemaphoreWake.h"
#include "SemaphoreDeadline.h"
#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>
#include <system_error>

// 退役栈引用的最高位只记录重新入栈责任，不是互斥标记；任何操作都不等待其清零
static const uint32_t kSemaphoreRetiredPending = uint32_t(1) << 31;
static const uint32_t kSemaphoreRetiredReferences = kSemaphoreRetiredPending - 1;

SemaphorePoolConfig::SemaphorePoolConfig() :
initialCapacity(16),
freeThreshold(0),
maxCapacity(-1)
{

}

SemaphoreEventRecord::SemaphoreEventRecord() :
m_next(0),
m_state(3),
m_retiredReferences(0),
m_freeNext(nullptr),
m_retiredNext(nullptr),
m_idleSince(0)
{

}

SemaphoreEventPool::SemaphoreEventPool(const SemaphorePoolConfig& config) :
m_config(config),
m_head(0),
m_free(nullptr),
m_retired(nullptr),
m_readers(0),
m_allocated(0),
m_capacity(0),
m_freeCount(0),
m_retiredCount(0),
m_unreservedCapacity(0),
m_lastGrowStatus(SEMAPHORE_OK),
m_grows(0),
m_lastGrow(0)
{
    const size_t addressLimit = ((std::numeric_limits<size_t>::max)() - sizeof(*this)) / sizeof(SemaphoreEventRecord);
    if (config.maxCapacity < -1 || config.initialCapacity > addressLimit ||
        (config.maxCapacity != -1 && static_cast<uint64_t>(config.maxCapacity) < config.initialCapacity))
    {
        throw std::invalid_argument("Semaphore event pool capacity");
    }
    SemaphoreEventRecord probe;
    if (!m_head.is_lock_free() || !m_free.is_lock_free() || !m_readers.is_lock_free() || !m_capacity.is_lock_free() ||
        !probe.m_state.is_lock_free())
    {
        throw std::system_error(std::make_error_code(std::errc::not_supported));
    }
    const SemaphoreResult result = allocate(config.initialCapacity, true);
    if (result.status != SEMAPHORE_OK)
    {
        throw std::bad_alloc();
    }
}

SemaphoreEventPool::~SemaphoreEventPool()
{
    // 外部静止且没有未结束等待；已结束记录都已从登记链摘除。
    sweep();
    while (m_retired.load() != nullptr)
    {
        size_t budget = 64;
        reclaim(budget);
    }
    SemaphoreEventRecord* node = m_free.exchange(nullptr);
    while (node != nullptr)
    {
        SemaphoreEventRecord* next = node->m_freeNext;
        delete node;
        node = next;
    }
}

bool SemaphoreEventPool::enter()
{
    uint64_t readers = m_readers.load();
    do
    {
        if (readers == UINT64_MAX)
        {
            return false;
        }
    } while (!m_readers.compare_exchange_weak(readers, readers + 1));
    return true;
}

void SemaphoreEventPool::leave()
{
    m_readers.fetch_sub(1);
}

SemaphoreEventRecord* SemaphoreEventPool::takeFree()
{
    SemaphoreEventRecord* node = m_free.load();
    while (node != nullptr)
    {
        SemaphoreEventRecord* next = node->m_freeNext;
        if (m_free.compare_exchange_weak(node, next))
        {
            m_freeCount.fetch_sub(1);
            return node;
        }
    }
    return nullptr;
}

void SemaphoreEventPool::putFree(SemaphoreEventRecord* first, SemaphoreEventRecord* last)
{
    SemaphoreEventRecord* head = m_free.load();
    do
    {
        last->m_freeNext = head;
    } while (!m_free.compare_exchange_weak(head, first));
}

void SemaphoreEventPool::retire(SemaphoreEventRecord* record)
{
    m_retiredCount.fetch_add(1);
    // 新退役记录尚未发布，上一轮栈借用在安全复用前已归零；直接初始化栈引用。
    record->m_retiredReferences.store(1, std::memory_order_relaxed);
    SemaphoreEventRecord* head = m_retired.load();
    do
    {
        record->m_retiredNext = head;
    } while (!m_retired.compare_exchange_weak(head, record));
}

void SemaphoreEventPool::queueRetired(SemaphoreEventRecord* record)
{
    if (record->m_retiredReferences.fetch_add(kSemaphoreRetiredPending) == 0)
    {
        publishRetired(record);
    }
}

void SemaphoreEventPool::publishRetired(SemaphoreEventRecord* record)
{
    SemaphoreEventRecord* head = m_retired.load();
    for (;;)
    {
        // 引用为零才可改next。发布自身的一份引用后，旧的头快照也能安全借用它。
        record->m_retiredNext = head;
        record->m_retiredReferences.store(1, std::memory_order_release);
        if (m_retired.compare_exchange_strong(head, record))
        {
            return;
        }
        // 发布失败先恢复入栈责任并撤销自身引用；仍有借用者时由最后离开者接手。
        if (record->m_retiredReferences.fetch_add(kSemaphoreRetiredPending - 1) != 1)
        {
            return;
        }
    }
}

SemaphoreEventRecord* SemaphoreEventPool::takeRetired(size_t& budget)
{
    SemaphoreEventRecord* head = m_retired.load();
    while (head != nullptr && budget != 0)
    {
        --budget;
        uint32_t references = head->m_retiredReferences.load();
        if ((references & kSemaphoreRetiredReferences) == kSemaphoreRetiredReferences)
        {
            return nullptr;
        }
        if ((references & kSemaphoreRetiredReferences) == 0 ||
            !head->m_retiredReferences.compare_exchange_strong(references, references + 1))
        {
            head = m_retired.load();
            continue;
        }
        SemaphoreEventRecord* borrowed = head;
        SemaphoreEventRecord* next = borrowed->m_retiredNext;
        if (m_retired.compare_exchange_strong(head, next))
        {
            // 同时撤销栈引用和本次借用；剩余旧借用者仍由全池读者保护覆盖。
            borrowed->m_retiredReferences.fetch_sub(2);
            return borrowed;
        }
        if (borrowed->m_retiredReferences.fetch_sub(1) == kSemaphoreRetiredPending + 1)
        {
            publishRetired(borrowed);
        }
    }
    return nullptr;
}

void SemaphoreEventPool::sweep()
{
    for (;;)
    {
        std::atomic<uintptr_t>* previous = &m_head;
        uintptr_t current = previous->load();
        bool restart = false;
        while (current != 0)
        {
            SemaphoreEventRecord* node = reinterpret_cast<SemaphoreEventRecord*>(current);
            const uintptr_t next = node->m_next.load();
            if ((next & 1) != 0)
            {
                if (!previous->compare_exchange_strong(current, next & ~uintptr_t(1)))
                {
                    restart = true;
                    break;
                }
                retire(node);
                current = next & ~uintptr_t(1);
            }
            else
            {
                previous = &node->m_next;
                current = next;
            }
        }
        if (!restart)
        {
            return;
        }
    }
}

size_t SemaphoreEventPool::reclaim(size_t& budget, bool entered)
{
    if (budget == 0 || m_retired.load() == nullptr)
    {
        if (entered)
        {
            leave();
        }
        return 0;
    }
    if (!entered && !enter())
    {
        return 0;
    }
    SemaphoreEventRecord* batch[64];
    size_t count = 0;
    while (count < 64 && budget != 0)
    {
        SemaphoreEventRecord* record = takeRetired(budget);
        if (record == nullptr)
        {
            break;
        }
        batch[count++] = record;
    }
    leave();
    // 先摘除，再观察读者为零；新的读者只能从不包含本批记录的入口开始。
    if (m_readers.load() != 0)
    {
        for (size_t index = 0; index < count; ++index)
        {
            queueRetired(batch[index]);
        }
        return 0;
    }
    size_t released = 0;
    SemaphoreEventRecord* firstFree = nullptr;
    SemaphoreEventRecord* lastFree = nullptr;
    const uint64_t timestamp = m_config.shrink ? SemaphoreDeadline::now() : 0;
    size_t reclaimed = 0;
    for (size_t index = 0; index < count; ++index)
    {
        SemaphoreEventRecord* record = batch[index];
        const uint32_t state = record->m_state.load(std::memory_order_acquire);
        if (state < 3)
        {
            // 帮助摘除不代表等待者已经结束访问；它发布状态3前只能重新退役。
            queueRetired(record);
            continue;
        }
        ++reclaimed;
        if (state == 4)
        {
            delete record;
            ++released;
        }
        else
        {
            if (record->m_idleSince == 0)
            {
                record->m_idleSince = timestamp;
            }
            record->m_freeNext = firstFree;
            firstFree = record;
            if (lastFree == nullptr)
            {
                lastFree = record;
            }
        }
    }
    if (reclaimed != 0)
    {
        m_retiredCount.fetch_sub(reclaimed);
    }
    if (released != 0)
    {
        m_capacity.fetch_sub(released);
        m_allocated.fetch_sub(released);
    }
    if (firstFree != nullptr)
    {
        // 安全批次统一入账并发布，其他线程不能读到尚在重连的freeNext。
        m_freeCount.fetch_add(reclaimed - released);
        putFree(firstFree, lastFree);
    }
    return released;
}

SemaphoreResult SemaphoreEventPool::allocate(size_t count, bool initial)
{
    if (count == 0)
    {
        return SemaphoreResult(initial ? SEMAPHORE_OK : SEMAPHORE_POLICY);
    }
    size_t limit = ((std::numeric_limits<size_t>::max)() - sizeof(*this)) / sizeof(SemaphoreEventRecord);
    if (m_config.maxCapacity != -1)
    {
        limit = static_cast<size_t>((std::min)(uint64_t(limit), static_cast<uint64_t>(m_config.maxCapacity)));
    }
    size_t allocated = m_allocated.load();
    size_t reserved = 0;
    do
    {
        if (allocated >= limit)
        {
            return SemaphoreResult(m_capacity.load() < limit ? SEMAPHORE_RETRY : SEMAPHORE_CAPACITY);
        }
        reserved = (std::min)(count, limit - allocated);
    } while (!m_allocated.compare_exchange_weak(allocated, allocated + reserved));
    SemaphoreEventRecord* first = nullptr;
    SemaphoreEventRecord* last = nullptr;
    const uint64_t timestamp = m_config.shrink ? SemaphoreDeadline::now() : 0;
    for (size_t index = 0; index < reserved; ++index)
    {
        SemaphoreEventRecord* node = nullptr;
        {
            // 记录构造不抛异常；申请失败直接回滚未发布批次，不用异常作正常分支
            node = new (std::nothrow) SemaphoreEventRecord;
        }
        if (node == nullptr)
        {
            while (first != nullptr)
            {
                SemaphoreEventRecord* next = first->m_freeNext;
                delete first;
                first = next;
            }
            m_allocated.fetch_sub(reserved);
            return SemaphoreResult(SEMAPHORE_NO_MEMORY);
        }
        node->m_idleSince = timestamp;
        node->m_freeNext = first;
        first = node;
        if (last == nullptr)
        {
            last = node;
        }
    }
    m_freeCount.fetch_add(reserved);
    putFree(first, last);
    // 批次接入可用链之后才转为已发布容量，暂停发布者不能提前耗尽可用额度
    // 运行期调用者持有读者保护，新增容量入账前不会物理缩容
    m_capacity.fetch_add(reserved);
    m_unreservedCapacity.fetch_add(reserved);
    if (!initial)
    {
        const uint64_t timestamp = SemaphoreDeadline::now();
        uint64_t previous = m_lastGrow.load();
        while (previous < timestamp && !m_lastGrow.compare_exchange_weak(previous, timestamp))
        {

        }
        uint64_t count = m_grows.load();
        while (count != UINT64_MAX && !m_grows.compare_exchange_weak(count, count + 1))
        {

        }
    }
    return SemaphoreResult();
}

SemaphoreResult SemaphoreEventPool::grow()
{
    const SemaphoreGrowContext context = {m_config.initialCapacity, m_grows.load(), m_capacity.load()};
    size_t count = 0;
    try
    {
        count = m_config.grow ? m_config.grow(context) : (std::max)(size_t(16), context.currentCapacity / 2);
    }
    catch (...)
    {
        m_lastGrowStatus.store(SEMAPHORE_POLICY);
        return SemaphoreResult(SEMAPHORE_POLICY);
    }
    const SemaphoreResult result = allocate(count, false);
    m_lastGrowStatus.store(result.status);
    return result;
}

SemaphoreResult SemaphoreEventPool::begin(SemaphoreEventRecord*& record)
{
    record = nullptr;
    size_t budget = 64;
    reclaim(budget);
    if (!enter())
    {
        return SemaphoreResult(SEMAPHORE_CAPACITY);
    }
    sweep();
    record = takeFree();
    bool evaluated = false;
    if (record == nullptr)
    {
        evaluated = true;
        const SemaphoreResult result = grow();
        record = takeFree();
        if (record == nullptr)
        {
            leave();
            return result.status == SEMAPHORE_OK ? SemaphoreResult(SEMAPHORE_RETRY) : result;
        }
    }
    if (!evaluated && m_config.freeThreshold > 0 && m_freeCount.load() <= m_config.freeThreshold)
    {
        // 提前增长失败不撤销已经取得的记录；回调每次需求最多执行一次。
        grow();
    }
    record->m_idleSince = 0;
    // 记录尚未登记；后面的head发布CAS向读者发布这两个初值。
    record->m_state.store(0, std::memory_order_relaxed);
    uintptr_t head = m_head.load();
    do
    {
        record->m_next.store(head, std::memory_order_relaxed);
    } while (!m_head.compare_exchange_weak(head, reinterpret_cast<uintptr_t>(record)));
    leave();
    return SemaphoreResult();
}

bool SemaphoreEventPool::finish(SemaphoreEventRecord* record)
{
    // 只有本等待者能发布状态3；帮手可以先摘链，但在此之前不能回收记录。
    uint32_t waiting = 0;
    record->m_state.compare_exchange_strong(waiting, 2);
    const bool notified = waiting == 1;
    const uintptr_t next = record->m_next.fetch_or(uintptr_t(1)) & ~uintptr_t(1);
    uintptr_t expected = reinterpret_cast<uintptr_t>(record);
    if (m_head.compare_exchange_strong(expected, next))
    {
        // 标记后的next不能再被其他帮助者改写；只有摘除成功者拥有回收权。
        record->m_state.store(3, std::memory_order_release);
        if (m_readers.load() == 0)
        {
            record->m_idleSince = m_config.shrink ? SemaphoreDeadline::now() : 0;
            m_freeCount.fetch_add(1);
            putFree(record, record);
        }
        else
        {
            retire(record);
        }
        return notified;
    }
    // 这是本次等待者对record的最后一次访问；随后仅在读者区内帮助处理当前登记链。
    record->m_state.store(3, std::memory_order_release);
    if (enter())
    {
        sweep();
        size_t budget = 64;
        reclaim(budget, true);
    }
    return notified;
}

SemaphoreResult SemaphoreEventPool::notify(bool all, SemaphoreWake* wake)
{
    if (!enter())
    {
        return SemaphoreResult(SEMAPHORE_CAPACITY);
    }
    // 这一head快照固定广播集合；并发新登记者只能链接在它之前。
    uintptr_t current = m_head.load();
    bool delivered = false;
    SemaphoreResult result;
    while (current != 0)
    {
        SemaphoreEventRecord* node = reinterpret_cast<SemaphoreEventRecord*>(current);
        uint32_t waiting = 0;
        if (node->m_state.compare_exchange_strong(waiting, 1))
        {
            delivered = true;
            if (wake != nullptr)
            {
                // 读者保护持续到系统唤醒返回，等待者退出也不能提前回收或复用这个地址
                const SemaphoreResult woke = wake->notify(&node->m_state);
                if (result.status == SEMAPHORE_OK && woke.status != SEMAPHORE_OK)
                {
                    result = woke;
                }
            }
            if (!all)
            {
                break;
            }
        }
        current = node->m_next.load() & ~uintptr_t(1);
    }
    leave();
    result.committed = delivered;
    return result;
}

SemaphoreResult SemaphoreEventPool::maintain(size_t& released)
{
    size_t reclaimBudget = 64;
    released = reclaim(reclaimBudget);
    if (!m_config.shrink)
    {
        return SemaphoreResult();
    }
    if (!enter())
    {
        return SemaphoreResult(SEMAPHORE_CAPACITY);
    }
    const uint64_t grows = m_grows.load();
    const uint64_t timestamp = SemaphoreDeadline::now();
    const uint64_t last = m_lastGrow.load();
    const SemaphoreShrinkContext context = {m_config.initialCapacity, grows, m_capacity.load(), m_freeCount.load(),
        timestamp >= last ? timestamp - last : 0, grows != 0};
    SemaphoreShrinkDecision decision = {};
    try
    {
        decision = m_config.shrink(context);
    }
    catch (...)
    {
        leave();
        return SemaphoreResult(SEMAPHORE_POLICY);
    }
    if (decision.idleMilliseconds == 0)
    {
        leave();
        return SemaphoreResult();
    }
    const size_t budget = (std::min)(size_t(64), decision.shrinkCount);
    for (size_t index = 0; index < budget; ++index)
    {
        SemaphoreEventRecord* node = takeFree();
        if (node == nullptr)
        {
            break;
        }
        // 尚未达空闲期的候选仍走退役链，避免直接push造成空闲栈ABA。
        bool shrink = timestamp >= node->m_idleSince && timestamp - node->m_idleSince >= decision.idleMilliseconds;
        size_t available = m_unreservedCapacity.load();
        while (shrink)
        {
            if (available <= m_config.initialCapacity)
            {
                shrink = false;
                break;
            }
            if (m_unreservedCapacity.compare_exchange_weak(available, available - 1))
            {
                break;
            }
        }
        node->m_state.store(shrink ? 4 : 3, std::memory_order_relaxed);
        retire(node);
    }
    released += reclaim(reclaimBudget, true);
    return SemaphoreResult();
}

SemaphorePoolStats SemaphoreEventPool::statistics() const
{
    SemaphorePoolStats result = {};
    result.capacity = m_capacity.load();
    result.freeCapacity = m_freeCount.load();
    result.retiredCapacity = m_retiredCount.load();
    const size_t allocated = m_allocated.load();
    result.reservedCapacity = allocated > result.capacity ? allocated - result.capacity : 0;
    result.retainedBytes = sizeof(*this) + allocated * sizeof(SemaphoreEventRecord);
    result.successfulGrowCount = m_grows.load();
    result.lastGrowStatus = static_cast<SemaphoreStatus>(m_lastGrowStatus.load());
    return result;
}