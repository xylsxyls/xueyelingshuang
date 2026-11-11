#include "ProcessWorkPacketPool.h"
#include "CSystem/CSystemAPI.h"
#include <cstring>
#include <limits>
#include <new>
#include <algorithm>
#include <memory>
#include <thread>

#ifdef _MSC_VER
static __declspec(thread) uint32_t s_packetCursor = 0;
#else
static thread_local uint32_t s_packetCursor = 0;
#endif

static_assert(sizeof(ProcessWorkPacketSlot) == 64, "small packet slot stride");

ProcessWorkPacketSlot::ProcessWorkPacketSlot() :
m_used(0)
{
    std::memset(m_data, 0, sizeof(m_data));
}

ProcessWorkPacketBlock::ProcessWorkPacketBlock(size_t count) :
m_slots(new (std::nothrow) ProcessWorkPacketSlot[count]), m_count(count), m_next(nullptr)
{

}

ProcessWorkPacketBlock::~ProcessWorkPacketBlock()
{
    delete[] m_slots;
}

ProcessWorkPacketPool::ProcessWorkPacketPool() :
m_slots(nullptr), m_capacity(0), m_blocks(nullptr), m_total(0), m_reserved(0), m_grows(0)
{

}

ProcessWorkPacketPool::~ProcessWorkPacketPool()
{
    clear();
}

void ProcessWorkPacketPool::clear()
{
    ProcessWorkPacketBlock* block = m_blocks.exchange(nullptr);
    while (block != nullptr)
    {
        ProcessWorkPacketBlock* next = block->m_next;
        delete block;
        block = next;
    }
    delete[] m_slots;
    m_slots = nullptr;
    m_capacity = 0;
    m_total.store(0);
    m_reserved.store(0);
    m_grows.store(0);
}

bool ProcessWorkPacketPool::configure(uint32_t capacity)
{
    try
    {
        ProcessWorkMemoryConfig config;
        config.m_postBufferReserve = capacity;
        config.m_postBufferMaximum = capacity;
        return configure(config);
    }
    catch (...)
    {
        return false;
    }
}

bool ProcessWorkPacketPool::configure(const ProcessWorkMemoryConfig& config)
{
    const uint32_t capacity = config.m_postBufferReserve;
    if (!ProcessWorkMemory::valid(config) || capacity > (std::numeric_limits<size_t>::max)() / sizeof(ProcessWorkPacketSlot) ||
        !m_blocks.is_lock_free() || !m_total.is_lock_free() || !m_reserved.is_lock_free() || !m_grows.is_lock_free())
    {
        return false;
    }
    try
    {
        std::unique_ptr<ProcessWorkPacketSlot[]> candidate(capacity == 0 ? nullptr : new (std::nothrow) ProcessWorkPacketSlot[capacity]);
        if (capacity != 0 && candidate == nullptr)
        {
            return false;
        }
        m_config = config;
        clear();
        m_slots = candidate.release();
        m_capacity = capacity;
        m_total.store(capacity);
        m_reserved.store(capacity);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

char* ProcessWorkPacketPool::acquire(size_t bytes)
{
    if (bytes > sizeof(m_slots[0].m_data))
    {
        return nullptr;
    }
    char* found = take(m_slots, m_capacity);
    if (found != nullptr)
    {
        return found;
    }
    for (ProcessWorkPacketBlock* block = m_blocks.load(std::memory_order_acquire); block != nullptr; block = block->m_next)
    {
        found = take(block->m_slots, block->m_count);
        if (found != nullptr)
        {
            return found;
        }
    }
    return grow();
}

char* ProcessWorkPacketPool::take(ProcessWorkPacketSlot* slots, size_t count)
{
    if (slots == nullptr || count == 0)
    {
        return nullptr;
    }
    if (s_packetCursor == 0)
    {
        s_packetCursor = CSystem::SystemThreadId() * 2654435761U + 1;
    }
    size_t index = s_packetCursor % count;
    // 每个槽至多观察一次，不等待已占用槽；有足够预留时不因局部窗口暂满转去new。
    for (size_t i = 0; i < count; ++i)
    {
        ProcessWorkPacketSlot& slot = slots[index];
        uint32_t empty = 0;
        if (slot.m_used.load(std::memory_order_relaxed) == 0 &&
            slot.m_used.compare_exchange_strong(empty, 1, std::memory_order_acquire, std::memory_order_relaxed))
        {
            // 空闲时反复使用这一小段热槽；积压越过小段后继续向前，不钉住私有额度。
            s_packetCursor = static_cast<uint32_t>(((index + 1) & 255U) == 0 ? index - 255 : index + 1);
            return slot.m_data;
        }
        index = index + 1 == count ? 0 : index + 1;
        if ((i & 4095U) == 4095U)
        {
            std::this_thread::yield();
        }
    }
    s_packetCursor = static_cast<uint32_t>(index);
    return nullptr;
}

bool ProcessWorkPacketPool::release(char* allocation)
{
    if (returnSlot(m_slots, m_capacity, allocation))
    {
        return true;
    }
    for (ProcessWorkPacketBlock* block = m_blocks.load(std::memory_order_acquire); block != nullptr; block = block->m_next)
    {
        if (returnSlot(block->m_slots, block->m_count, allocation))
        {
            return true;
        }
    }
    return false;
}

bool ProcessWorkPacketPool::returnSlot(ProcessWorkPacketSlot* slots, size_t count, char* allocation)
{
    const uintptr_t address = reinterpret_cast<uintptr_t>(allocation);
    const uintptr_t begin = reinterpret_cast<uintptr_t>(slots);
    if (slots == nullptr || address < begin)
    {
        return false;
    }
    const uintptr_t offset = address - begin;
    if (offset >= static_cast<uint64_t>(count) * sizeof(ProcessWorkPacketSlot) ||
        offset % sizeof(ProcessWorkPacketSlot) != offsetof(ProcessWorkPacketSlot, m_data))
    {
        return false;
    }
    slots[offset / sizeof(ProcessWorkPacketSlot)].m_used.store(0, std::memory_order_release);
    return true;
}

uint64_t ProcessWorkPacketPool::capacity() const
{
    return m_total.load(std::memory_order_acquire);
}

char* ProcessWorkPacketPool::grow()
{
    const uint64_t physicalLimit = ((std::numeric_limits<size_t>::max)() - sizeof(ProcessWorkPacketBlock)) / sizeof(ProcessWorkPacketSlot);
    uint64_t maximum = m_config.m_postBufferMaximum == -1 ? physicalLimit :
        (std::min)(physicalLimit, static_cast<uint64_t>(m_config.m_postBufferMaximum));
    uint64_t reserved = m_reserved.load(std::memory_order_acquire);
    if (reserved >= maximum)
    {
        return nullptr;
    }
    ProcessWorkMemoryGrowContext context;
    context.m_kind = ProcessWorkPostBuffers;
    context.m_initialCapacity = m_config.m_postBufferReserve;
    context.m_currentCapacity = m_total.load(std::memory_order_acquire);
    context.m_successfulGrowCount = m_grows.load(std::memory_order_acquire);
    context.m_requiredGrowth = 1;
    uint64_t amount = 0;
    try
    {
        amount = m_config.m_policy ? m_config.m_policy->grow(context) : (std::max)(uint64_t(64), context.m_currentCapacity);
    }
    catch (...)
    {
        return nullptr;
    }
    if (amount == 0)
    {
        return nullptr;
    }
    uint32_t attempts = 0;
    for (;;)
    {
        if (reserved >= maximum)
        {
            return nullptr;
        }
        const uint64_t granted = (std::min)(amount, maximum - reserved);
        if (m_reserved.compare_exchange_weak(reserved, reserved + granted, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            amount = granted;
            break;
        }
        try
        {
            if (++attempts % 64 == 0)
            {
                CSystem::Sleep(1);
            }
            else
            {
                std::this_thread::yield();
            }
        }
        catch (...)
        {
            return nullptr;
        }
    }
    try
    {
        if (!ProcessWorkMemory::allows(m_config, amount * sizeof(ProcessWorkPacketSlot) + sizeof(ProcessWorkPacketBlock)))
        {
            m_reserved.fetch_sub(amount, std::memory_order_release);
            return nullptr;
        }
        std::unique_ptr<ProcessWorkPacketBlock> block(new (std::nothrow) ProcessWorkPacketBlock(static_cast<size_t>(amount)));
        if (!block || block->m_slots == nullptr)
        {
            m_reserved.fetch_sub(amount, std::memory_order_release);
            return nullptr;
        }
        // 给触发者保留一个真实槽，其他生产者不能在其返回之前抢走此次进展。
        block->m_slots[0].m_used.store(1, std::memory_order_relaxed);
        char* answer = block->m_slots[0].m_data;
        ProcessWorkPacketBlock* head = m_blocks.load(std::memory_order_acquire);
        for (;;)
        {
            block->m_next = head;
            if (m_blocks.compare_exchange_weak(head, block.get(), std::memory_order_acq_rel, std::memory_order_acquire))
            {
                break;
            }
            if (++attempts % 64 == 0)
            {
                CSystem::Sleep(1);
            }
            else
            {
                std::this_thread::yield();
            }
        }
        block.release();
        ProcessWorkMemoryGrowth growth;
        growth.m_kind = ProcessWorkPostBuffers;
        growth.m_previousCapacity = m_total.fetch_add(amount, std::memory_order_acq_rel);
        growth.m_addedCapacity = amount;
        growth.m_currentCapacity = growth.m_previousCapacity + amount;
        growth.m_successfulGrowCount = m_grows.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (m_config.m_policy)
        {
            try
            {
                m_config.m_policy->grown(growth);
            }
            catch (...)
            {
                // 块已发布且一个槽归申请者；通知异常不得回滚所有权。
            }
        }
        return answer;
    }
    catch (...)
    {
        m_reserved.fetch_sub(amount, std::memory_order_release);
        return nullptr;
    }
}