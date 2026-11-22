#include "NetSenderMacro.h"
#include "NetSenderLocalHub.h"
#include "NetSenderLocalPacket.h"
#include "CSystem/CSystemAPI.h"

/** CAS竞争时定期让出CPU，不把瞬时竞争变成生命周期失败
@param [in,out] retries 当前调用的失败次数
*/
static void NetLocalHubContention(uint32_t& retries)
{
    if (++retries == 64)
    {
        retries = 0;
        CSystem::Sleep(0);
    }
}

NetSenderLocalHubSlot::NetSenderLocalHubSlot() :
m_box(nullptr),
m_next(nullptr),
m_storage(nullptr),
m_readers(0xc0000000U)
{

}

bool NetSenderLocalHubSlot::acquire()
{
    uint32_t state = m_readers.load(std::memory_order_acquire);
    uint32_t retries = 0;
    while (state < 0x3fffffffU)
    {
        if (m_readers.compare_exchange_weak(state, state + 1, std::memory_order_acquire, std::memory_order_acquire))
        {
            return true;
        }
        NetLocalHubContention(retries);
    }
    return false;
}

void NetSenderLocalHubSlot::release()
{
    m_readers.fetch_sub(1, std::memory_order_release);
}

/** 默认只预热128个实际借用槽，最大存储按8MiB槽大小计算
@return 可由初始化配置替换的底层池参数
*/
static LockFreeHashMapPoolConfig NetLocalHubPool()
{
    LockFreeHashMapPoolConfig result;
    result.initialCapacity = 128;
    result.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetSenderLocalHubSlot>>);
    result.directoryCapacity = 128;
    result.lookupCacheCapacity = 0;
    return result;
}

NetSenderLocalHub::NetSenderLocalHub() :
m_slots(new NetObjectPool<NetSenderLocalHubSlot>(NetLocalHubPool())), m_first(nullptr), m_ownership(NET_LOCAL_HUB_IDLE)
{

}

NetSenderLocalHub::~NetSenderLocalHub()
{
    NetSenderLocalHubSlot* slot = m_first.load();
    while (slot != nullptr)
    {
        NetSenderLocalHubSlot* next = slot->m_next;
        m_slots->release(slot->m_storage);
        slot = next;
    }
}

int32_t NetSenderLocalHub::configurePool(const LockFreeHashMapPoolConfig& config)
{
    if (m_ownership.load() != NET_LOCAL_HUB_IDLE) { return NI_INVALID_STATE; }
    try
    {
        std::unique_ptr<NetObjectPool<NetSenderLocalHubSlot>> prepared(new NetObjectPool<NetSenderLocalHubSlot>(config));
        if (prepared->status() != NI_OK) { return prepared->status(); }
        NetSenderLocalHubSlot* slot = m_first.exchange(nullptr);
        while (slot != nullptr)
        {
            NetSenderLocalHubSlot* next = slot->m_next;
            m_slots->release(slot->m_storage);
            slot = next;
        }
        m_slots.swap(prepared);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

NetSenderLocalHub& NetSenderLocalHub::instance()
{
    static NetSenderLocalHub s_hub;
    return s_hub;
}

int32_t NetSenderLocalHub::attach(NetSenderLocalInbox* inbox)
{
    if (inbox == nullptr) { return NI_INVALID_ARGUMENT; }
    // 同一个箱的生命周期仍由其owner串行，不同箱可并行。
    for (NetSenderLocalHubSlot* slot = m_first.load(); slot != nullptr; slot = slot->m_next)
    {
        if (slot->m_box.load() == inbox) { return NI_OK; }
    }
    uint64_t state = m_ownership.load();
    uint32_t retries = 0;
    for (;;)
    {
        const uint64_t phase = state & 7;
        if ((phase != NET_LOCAL_HUB_MANAGED && phase != NET_LOCAL_HUB_EXTERNAL) || ((state >> 3) & 0x1fffffffU) == 0)
        {
            return NI_INVALID_STATE;
        }
        if ((state >> 32) == UINT32_MAX) { return NI_LIMIT_EXCEEDED; }
        if (m_ownership.compare_exchange_weak(state, state + (UINT64_C(1) << 32))) { break; }
        NetLocalHubContention(retries);
    }
    for (NetSenderLocalHubSlot* slot = m_first.load(); slot != nullptr; slot = slot->m_next)
    {
        uint32_t empty = 0xc0000000U;
        if (slot->m_readers.compare_exchange_strong(empty, 0x40000000U))
        {
            slot->m_box.store(inbox, std::memory_order_relaxed);
            slot->m_readers.store(0, std::memory_order_release);
            return NI_OK;
        }
    }
    NetObjectReference<NetSenderLocalHubSlot> allocated;
    const int32_t result = m_slots->create(allocated);
    if (result == NI_OK)
    {
        allocated->m_storage = allocated.storage();
        allocated->m_box.store(inbox);
        allocated->m_readers.store(0);
        NetSenderLocalHubSlot* first = m_first.load();
        retries = 0;
        do
        {
            allocated->m_next = first;
            if (m_first.compare_exchange_weak(first, allocated.get(), std::memory_order_release, std::memory_order_relaxed))
            {
                allocated.release();
                return NI_OK;
            }
            NetLocalHubContention(retries);
        } while (true);
    }
    m_ownership.fetch_sub(UINT64_C(1) << 32);
    stopUnused();
    return result;
}

void NetSenderLocalHub::detach(NetSenderLocalInbox* inbox)
{
    if (inbox != nullptr)
    {
        for (NetSenderLocalHubSlot* current = m_first.load(); current != nullptr; current = current->m_next)
        {
            NetSenderLocalHubSlot& slot = *current;
            if (slot.m_box.load() != inbox)
            {
                continue;
            }
            slot.m_readers.fetch_or(0x80000000U, std::memory_order_acq_rel);
            while (slot.m_readers.load(std::memory_order_acquire) != 0x80000000U)
            {
                // 回收只能等已经取得的借用；从该箱自身回调调用detach仍属非法生命周期重入。
                CSystem::Sleep(1);
            }
            slot.m_box.store(nullptr);
            slot.m_readers.store(0xc0000000U, std::memory_order_release);
            m_ownership.fetch_sub(UINT64_C(1) << 32);
            break;
        }
    }
    stopUnused();
}

int32_t NetSenderLocalHub::prepare(bool external)
{
    uint64_t state = m_ownership.load();
    uint32_t retries = 0;
    bool registered = false;
    for (;;)
    {
        const uint64_t phase = state & 7;
        if (external && phase == NET_LOCAL_HUB_EXTERNAL)
        {
            return NI_OK;
        }
        if (!external && (phase == NET_LOCAL_HUB_MANAGED || phase == NET_LOCAL_HUB_EXTERNAL))
        {
            if (((state >> 3) & 0x1fffffffU) == 0x1fffffffU)
            {
                return NI_LIMIT_EXCEEDED;
            }
            if (m_ownership.compare_exchange_weak(state, state + 8))
            {
                // 先取得租约再读接收器，防止最后release并发uninit期间读取其配置。
                int32_t checked = NI_OK;
                try
                {
                    if (ProcessWork::instance().receiveBufferCount() == 0)
                    {
                        checked = NI_INVALID_STATE;
                    }
                    else if (ProcessWork::instance().receivePayloadCapacity() < kNetSenderLocalReceiveBytes - 12)
                    {
                        checked = NI_BUFFER_TOO_SMALL;
                    }
                }
                catch (...)
                {
                    checked = NI_INVALID_STATE;
                }
                if (checked != NI_OK)
                {
                    release();
                }
                return checked;
            }
            NetLocalHubContention(retries);
            continue;
        }
        if (state != NET_LOCAL_HUB_IDLE && state != NET_LOCAL_HUB_REGISTERED)
        {
            return NI_INVALID_STATE;
        }
        registered = state == NET_LOCAL_HUB_REGISTERED;
        if (m_ownership.compare_exchange_weak(state, NET_LOCAL_HUB_STARTING))
        {
            break;
        }
        NetLocalHubContention(retries);
    }
    int32_t result = NI_INVALID_STATE;
    try
    {
        if (ProcessWork::instance().receiveBufferCount() == 0)
        {
            if (!registered)
            {
                registered = ProcessWork::instance().addProcessReceiveCallback(this);
            }
            result = !registered ? NI_NO_MEMORY :
                (external || ProcessWork::instance().initReceive(kNetSenderLocalReceiveBytes, 64) ? NI_OK : NI_INVALID_STATE);
        }
    }
    catch (...)
    {
        result = NI_NO_MEMORY;
    }
    // STARTING期间其他调用立即拒绝；只有本事务可以发布启动结果。
    m_ownership.store(result == NI_OK ? (external ? NET_LOCAL_HUB_EXTERNAL : NET_LOCAL_HUB_MANAGED + 8) :
        (registered ? NET_LOCAL_HUB_REGISTERED : NET_LOCAL_HUB_IDLE));
    return result;
}

int32_t NetSenderLocalHub::acquire()
{
    return prepare(false);
}

int32_t NetSenderLocalHub::prepareExternal()
{
    return prepare(true);
}

int32_t NetSenderLocalHub::externalStopped()
{
    uint64_t state = NET_LOCAL_HUB_EXTERNAL;
    if (!m_ownership.compare_exchange_strong(state, NET_LOCAL_HUB_STOPPING))
    {
        return NI_INVALID_STATE;
    }
    const bool stopped = ProcessWork::instance().receiveBufferCount() == 0;
    m_ownership.store(stopped ? NET_LOCAL_HUB_IDLE : NET_LOCAL_HUB_EXTERNAL);
    return stopped ? NI_OK : NI_INVALID_STATE;
}

void NetSenderLocalHub::release()
{
    uint64_t state = m_ownership.load();
    uint32_t retries = 0;
    while (((state >> 3) & 0x1fffffffU) != 0)
    {
        if (m_ownership.compare_exchange_weak(state, state - 8))
        {
            break;
        }
        NetLocalHubContention(retries);
    }
    stopUnused();
}

void NetSenderLocalHub::stopUnused()
{
    uint64_t state = NET_LOCAL_HUB_MANAGED;
    if (!m_ownership.compare_exchange_strong(state, NET_LOCAL_HUB_STOPPING))
    {
        return;
    }
    try
    {
        ProcessWork::instance().uninitReceive();
    }
    catch (...)
    {
        // 未确认停止时保持STOPPING，不能把未知接收器让给新实例。
        return;
    }
    // 依赖的关闭接口可能拒绝回调内自回收；未确认任务收敛时不能开放下一代。
    if (ProcessWork::instance().receiveBufferCount() == 0)
    {
        m_ownership.store(NET_LOCAL_HUB_IDLE);
    }
}

void NetSenderLocalHub::receive(int32_t sendPid, const char* bytes, int32_t size, MessageType type)
{
    if (static_cast<int32_t>(type) != NET_SENDER_LOCAL_MESSAGE || sendPid <= 0 || bytes == nullptr || size < 56 || size > 16440)
    {
        return;
    }
    for (NetSenderLocalHubSlot* current = m_first.load(); current != nullptr; current = current->m_next)
    {
        NetSenderLocalHubSlot& slot = *current;
        if (!slot.acquire())
        {
            continue;
        }
        try
        {
            slot.m_box.load(std::memory_order_relaxed)->receive(sendPid, bytes, size, type);
        }
        catch (...)
        {
            // 一个箱的错误不影响其他箱；无论成功失败都归还本次借用。
        }
        slot.release();
    }
}