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

static_assert(sizeof(ProcessWorkPacketSlot<64>) == 64, "small packet slot stride");
static_assert(sizeof(ProcessWorkPacketSlot<128>) == 128, "large packet slot stride");

template<size_t SlotBytes>
ProcessWorkPacketSlot<SlotBytes>::ProcessWorkPacketSlot() :
m_used(0)
{
    std::memset(m_data, 0, sizeof(m_data));
}

/** 创建类型化槽数组，每槽的所有者和正文一次初始化
@param [in] count 槽数，0不分配
@param [in] slotBytes 64或128，内部核验乘法范围
@return 存储起点或nullptr；必须以同一slotBytes归还
*/
static char* AllocatePacketSlots(size_t count, uint32_t slotBytes)
{
    if (count == 0 || (slotBytes != 64 && slotBytes != 128) || count > (std::numeric_limits<size_t>::max)() / slotBytes)
    {
        return nullptr;
    }
    return slotBytes == 64 ? reinterpret_cast<char*>(new (std::nothrow) ProcessWorkPacketSlot<64>[count]) :
        reinterpret_cast<char*>(new (std::nothrow) ProcessWorkPacketSlot<128>[count]);
}

/** 静止期按原数组类型归还槽存储
@param [in] slots AllocatePacketSlots的结果，可空
@param [in] slotBytes 与分配时相同的64或128
*/
static void FreePacketSlots(char* slots, uint32_t slotBytes)
{
    if (slotBytes == 64)
    {
        delete[] reinterpret_cast<ProcessWorkPacketSlot<64>*>(slots);
    }
    else
    {
        delete[] reinterpret_cast<ProcessWorkPacketSlot<128>*>(slots);
    }
}

ProcessWorkPacketBlock::ProcessWorkPacketBlock(size_t count, uint32_t slotBytes) :
m_slots(AllocatePacketSlots(count, slotBytes)), m_count(count), m_slotBytes(slotBytes), m_next(nullptr)
{

}

ProcessWorkPacketBlock::~ProcessWorkPacketBlock()
{
    FreePacketSlots(m_slots, m_slotBytes);
}

ProcessWorkPacketPool::ProcessWorkPacketPool() :
m_slots(nullptr), m_capacity(0), m_slotBytes(128), m_blocks(nullptr), m_total(0), m_reserved(0), m_grows(0)
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
    FreePacketSlots(m_slots, m_slotBytes);
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
        config.m_postBufferSlotBytes = 64;
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
    if (!ProcessWorkMemory::valid(config) || capacity > (std::numeric_limits<size_t>::max)() / config.m_postBufferSlotBytes ||
        !m_blocks.is_lock_free() || !m_total.is_lock_free() || !m_reserved.is_lock_free() || !m_grows.is_lock_free())
    {
        return false;
    }
    try
    {
        // slots为本次候选数组，可空；失败时按原类型释放，无返回值。
        auto releaseCandidate = [&config](char* slots)
        {
            FreePacketSlots(slots, config.m_postBufferSlotBytes);
        };
        std::unique_ptr<char, decltype(releaseCandidate)> candidate(AllocatePacketSlots(capacity, config.m_postBufferSlotBytes), releaseCandidate);
        if (capacity != 0 && candidate == nullptr)
        {
            return false;
        }
        m_config = config;
        clear();
        m_slots = candidate.release();
        m_capacity = capacity;
        m_slotBytes = config.m_postBufferSlotBytes;
        m_total.store(capacity);
        m_reserved.store(capacity);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

/** 尝试上次成功位置之后的空槽，失败交回完整扫描，不隐藏其他线程释放的容量
@param [in,out] storage 生命周期稳定的类型化槽数组，可空
@param [in] count 数组槽数；越界提示和首次使用均交给完整路径重新定位
@return 独占正文地址，或未取得时nullptr；不改变失败调用的所有权
*/
template<uint32_t SlotBytes>
static char* TakePacketHint(char* storage, size_t count)
{
    const uint32_t index = s_packetCursor;
    if (storage == nullptr || index == 0 || index >= count)
    {
        return nullptr;
    }
    ProcessWorkPacketSlot<SlotBytes>& slot = reinterpret_cast<ProcessWorkPacketSlot<SlotBytes>*>(storage)[index];
    uint32_t empty = 0;
    if (slot.m_used.load(std::memory_order_relaxed) != 0 ||
        !slot.m_used.compare_exchange_strong(empty, 1, std::memory_order_acquire, std::memory_order_relaxed))
    {
        return nullptr;
    }
    const uint32_t kWindowMask = SlotBytes == 128 ? 511 : 255;
    s_packetCursor = ((index + 1) & kWindowMask) == 0 ? index - kWindowMask : index + 1;
    return slot.m_data;
}

char* ProcessWorkPacketPool::acquire(size_t bytes)
{
    if (bytes > 60 && (m_slotBytes != 128 || bytes > 124))
    {
        return nullptr;
    }
    // 热槽一次命中时避免进入完整遍历及取模；首次、竞争和积压仍使用原全池扫描。
    char* found = m_slotBytes == 128 ? TakePacketHint<128>(m_slots, m_capacity) : TakePacketHint<64>(m_slots, m_capacity);
    if (found != nullptr)
    {
        return found;
    }
    found = take(m_slots, m_capacity);
    if (found != nullptr)
    {
        return found;
    }
    for (ProcessWorkPacketBlock* block = m_blocks.load(std::memory_order_acquire); block != nullptr; block = block->m_next)
    {
        found = m_slotBytes == 128 ? TakePacketHint<128>(block->m_slots, block->m_count) :
            TakePacketHint<64>(block->m_slots, block->m_count);
        if (found == nullptr)
        {
            found = take(block->m_slots, block->m_count);
        }
        if (found != nullptr)
        {
            return found;
        }
    }
    return grow();
}

/** 从固定步长数组取得一个槽；编译期步长避免逐包动态乘法和重复读取配置
@param [in,out] slots 生命周期稳定的数组，64或128字节步长
@param [in] count 槽数，忙时扫描全数组，不等待单个持有者
@return 独占正文地址，或本次未取得时nullptr
*/
template<uint32_t SlotBytes>
static char* TakePacketSlots(char* slots, size_t count)
{
    if (slots == nullptr || count == 0)
    {
        return nullptr;
    }
    if (s_packetCursor == 0)
    {
        s_packetCursor = CSystem::SystemThreadId() * 2654435761U + 1;
    }
    ProcessWorkPacketSlot<SlotBytes>* typed = reinterpret_cast<ProcessWorkPacketSlot<SlotBytes>*>(slots);
    const size_t kWindowMask = SlotBytes == 128 ? 511 : 255;
    size_t index = s_packetCursor % count;
    // 每个槽至多观察一次，不等待已占用槽；有足够预留时不因局部窗口暂满转去new。
    for (size_t i = 0; i < count; ++i)
    {
        ProcessWorkPacketSlot<SlotBytes>& slot = typed[index];
        uint32_t empty = 0;
        if (slot.m_used.load(std::memory_order_relaxed) == 0 &&
            slot.m_used.compare_exchange_strong(empty, 1, std::memory_order_acquire, std::memory_order_relaxed))
        {
            // 空闲时反复使用这一小段热槽；积压越过小段后继续向前，不钉住私有额度。
            s_packetCursor = static_cast<uint32_t>(((index + 1) & kWindowMask) == 0 ? index - kWindowMask : index + 1);
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

char* ProcessWorkPacketPool::take(char* storage, size_t count)
{
    return m_slotBytes == 128 ? TakePacketSlots<128>(storage, count) : TakePacketSlots<64>(storage, count);
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

/** 验证精确正文起点并归还对应类型的独占槽
@param [in,out] slots 稳定槽数组，可空
@param [in] count 数组槽数
@param [in] allocation 待归还正文，只比较地址，匹配后才访问所有者
@return true属于数组且已归还，false交给其他块或堆处理
*/
template<size_t SlotBytes>
static bool ReturnPacketSlot(char* slots, size_t count, char* allocation)
{
    const uintptr_t address = reinterpret_cast<uintptr_t>(allocation);
    const uintptr_t begin = reinterpret_cast<uintptr_t>(slots);
    if (slots == nullptr || address < begin)
    {
        return false;
    }
    const uintptr_t offset = address - begin;
    if (offset >= (static_cast<uint64_t>(count) * SlotBytes) ||
        (offset & (SlotBytes - 1)) != offsetof(ProcessWorkPacketSlot<SlotBytes>, m_data))
    {
        return false;
    }
    reinterpret_cast<ProcessWorkPacketSlot<SlotBytes>*>(allocation - offsetof(ProcessWorkPacketSlot<SlotBytes>, m_data))->m_used.store(0, std::memory_order_release);
    return true;
}

bool ProcessWorkPacketPool::returnSlot(char* storage, size_t count, char* allocation)
{
    return m_slotBytes == 128 ? ReturnPacketSlot<128>(storage, count, allocation) : ReturnPacketSlot<64>(storage, count, allocation);
}

uint64_t ProcessWorkPacketPool::capacity() const
{
    return m_total.load(std::memory_order_acquire);
}

char* ProcessWorkPacketPool::grow()
{
    const uint64_t physicalLimit = ((std::numeric_limits<size_t>::max)() - sizeof(ProcessWorkPacketBlock)) / m_slotBytes;
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
        if (!ProcessWorkMemory::allows(m_config, (amount * m_slotBytes) + sizeof(ProcessWorkPacketBlock)))
        {
            m_reserved.fetch_sub(amount, std::memory_order_release);
            return nullptr;
        }
        std::unique_ptr<ProcessWorkPacketBlock> block(new (std::nothrow) ProcessWorkPacketBlock(static_cast<size_t>(amount), m_slotBytes));
        if (!block || block->m_slots == nullptr)
        {
            m_reserved.fetch_sub(amount, std::memory_order_release);
            return nullptr;
        }
        // 给触发者保留一个真实槽，其他生产者不能在其返回之前抢走此次进展。
        char* answer = nullptr;
        if (m_slotBytes == 64)
        {
            ProcessWorkPacketSlot<64>* first = reinterpret_cast<ProcessWorkPacketSlot<64>*>(block->m_slots);
            first->m_used.store(1, std::memory_order_relaxed);
            answer = first->m_data;
        }
        else
        {
            ProcessWorkPacketSlot<128>* first = reinterpret_cast<ProcessWorkPacketSlot<128>*>(block->m_slots);
            first->m_used.store(1, std::memory_order_relaxed);
            answer = first->m_data;
        }
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