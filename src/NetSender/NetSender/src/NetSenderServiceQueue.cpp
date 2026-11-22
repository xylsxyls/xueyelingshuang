#include "NetSenderMacro.h"
#include "NetSenderServiceQueue.h"
#include <cstring>

NetSenderServiceManagedFrame::NetSenderServiceManagedFrame() :
m_frame(),
m_origin(),
m_endpoint(0),
m_created(0),
m_blocked(0),
m_channel(0),
m_next(UINT32_MAX),
m_previous(UINT32_MAX),
m_lane(6)
{

}

NetSenderServiceManagedFrame::~NetSenderServiceManagedFrame()
{
    if (m_bytes.data() != nullptr)
    {
        NetInterface::wipe(m_bytes.data(), m_frame.m_payloadSize);
    }
}

int32_t NetSenderServiceManagedFrame::view(uint64_t now, NetInterfaceFrame& frame, std::vector<uint8_t>& scratch) const
{
    frame = m_frame;
    frame.m_payload = m_bytes.data();
    if (frame.m_kind != NI_REQUEST)
    {
        return NI_READY;
    }
    NetInterfaceRpcView rpc = {};
    if (NetInterfaceRpc::parse(frame, rpc) != NI_OK || now < m_created)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (now - m_created >= rpc.m_value)
    {
        return NI_TIMEOUT;
    }
    try
    {
        scratch.resize(m_frame.m_payloadSize);
        uint32_t required = 0;
        const int32_t encoded = NetInterfaceRpc::encode(NI_REQUEST, rpc.m_value - static_cast<uint32_t>(now - m_created),
            rpc.m_parent, rpc.m_data, rpc.m_size, scratch.data(), static_cast<uint32_t>(scratch.size()), required);
        if (encoded != NI_OK)
        {
            return encoded;
        }
        frame.m_payload = scratch.data();
        frame.m_payloadSize = required;
        return NI_READY;
    }
    catch (...)
    {
        frame = NetInterfaceFrame();
        return NI_NO_MEMORY;
    }
}

NetSenderServiceQueueEntry::NetSenderServiceQueueEntry() :
m_next(UINT32_MAX)
{

}

LockFreeHashMapPoolConfig NetSenderServiceQueue::defaultRecords()
{
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 128;
    config.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetSenderServiceManagedFrame>>);
    config.directoryCapacity = 128;
    config.lookupCacheCapacity = 0;
    return config;
}

NetSenderServiceQueue::NetSenderServiceQueue() :
m_bodies(new NetBufferPool(NetBufferPoolConfig())),
m_records(new NetObjectPool<NetSenderServiceManagedFrame>(defaultRecords())),
m_free(UINT32_MAX),
m_count(0),
m_bytes(0),
m_used(false)
{
    for (uint32_t lane = 0; lane < 3; ++lane)
    {
        m_heads[lane] = m_tails[lane] = UINT32_MAX;
        m_sizes[lane] = 0;
    }
}

NetSenderServiceQueue::~NetSenderServiceQueue()
{
    clear();
    // 宿主必须先结束借用；残余目录引用先于两个实际池自动释放。
}

int32_t NetSenderServiceQueue::configurePools(const NetBufferPoolConfig& bodies, const LockFreeHashMapPoolConfig& records)
{
    if (m_used)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetBufferPool> preparedBodies(new NetBufferPool(bodies));
        if (preparedBodies->status() != NI_OK)
        {
            return preparedBodies->status();
        }
        std::unique_ptr<NetObjectPool<NetSenderServiceManagedFrame>> preparedRecords(
            new NetObjectPool<NetSenderServiceManagedFrame>(records));
        if (preparedRecords->status() != NI_OK)
        {
            return preparedRecords->status();
        }
        m_bodies.swap(preparedBodies);
        m_records.swap(preparedRecords);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderServiceQueue::status() const
{
    return m_bodies->status() == NI_OK ? m_records->status() : m_bodies->status();
}

int32_t NetSenderServiceQueue::room(uint32_t count, uint64_t bytes)
{
    if (count == 0)
    {
        return bytes == 0 ? NI_OK : NI_INVALID_ARGUMENT;
    }
    if (bytes % count != 0 || bytes / count > 1048704)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::vector<NetObjectReference<NetSenderServiceManagedFrame>> reserved(count);
        for (uint32_t index = 0; index < count; ++index)
        {
            int32_t result = m_records->create(reserved[index]);
            if (result == NI_OK)
            {
                result = m_bodies->allocate(static_cast<uint32_t>(bytes / count), reserved[index]->m_bytes);
            }
            if (result != NI_OK)
            {
                return result;
            }
        }
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderServiceQueue::push(uint32_t lane, uint64_t endpoint, const NetInterfaceFrame& frame,
    const NetInterfaceOrigin& origin, uint64_t now)
{
    if (lane >= 3 || (frame.m_payloadSize != 0 && frame.m_payload == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (frame.m_kind == NI_CANCEL)
    {
        for (uint32_t index = m_heads[lane]; index != UINT32_MAX; index = m_items[index].m_value->m_next)
        {
            const NetSenderServiceManagedFrame& pending = *m_items[index].m_value;
            if (pending.m_endpoint == endpoint && pending.m_frame.m_kind == NI_REQUEST &&
                pending.m_frame.m_requestId == frame.m_requestId && pending.m_frame.m_productId == frame.m_productId &&
                pending.m_frame.m_messageType == frame.m_messageType)
            {
                release(index);
                return NI_OK;
            }
        }
    }
    NetObjectReference<NetSenderServiceManagedFrame> owned;
    int32_t result = m_records->create(owned);
    if (result == NI_OK)
    {
        result = m_bodies->allocate(frame.m_payloadSize, owned->m_bytes);
    }
    if (result != NI_OK)
    {
        return result;
    }
    if (frame.m_payloadSize != 0)
    {
        std::memcpy(owned->m_bytes.data(), frame.m_payload, frame.m_payloadSize);
    }
    // 后续目录分配可能失败；先登记有效长度，使回滚析构也擦除已经复制的正文。
    owned->m_frame.m_payloadSize = frame.m_payloadSize;
    uint32_t index = m_free;
    if (index == UINT32_MAX)
    {
        if (m_items.size() >= UINT32_MAX)
        {
            return NI_LIMIT_EXCEEDED;
        }
        try
        {
            index = static_cast<uint32_t>(m_items.size());
            m_items.push_back(NetSenderServiceQueueEntry());
        }
        catch (...)
        {
            return NI_NO_MEMORY;
        }
    }
    else
    {
        m_free = m_items[index].m_next;
    }
    m_items[index].m_value = std::move(owned);
    NetSenderServiceManagedFrame& item = *m_items[index].m_value;
    item.m_frame = frame;
    item.m_frame.m_payload = nullptr;
    item.m_origin = origin;
    item.m_endpoint = endpoint;
    item.m_created = now;
    item.m_blocked = item.m_channel = 0;
    item.m_next = UINT32_MAX;
    item.m_previous = m_tails[lane];
    item.m_lane = lane;
    if (m_tails[lane] != UINT32_MAX)
    {
        m_items[m_tails[lane]].m_value->m_next = index;
    }
    else
    {
        m_heads[lane] = index;
    }
    m_tails[lane] = index;
    ++m_sizes[lane];
    ++m_count;
    m_bytes += frame.m_payloadSize;
    m_used = true;
    return NI_OK;
}

uint32_t NetSenderServiceQueue::first(uint32_t lane) const
{
    return lane < 3 ? m_heads[lane] : UINT32_MAX;
}

uint32_t NetSenderServiceQueue::last(uint32_t lane) const
{
    return lane < 3 ? m_tails[lane] : UINT32_MAX;
}

NetSenderServiceManagedFrame* NetSenderServiceQueue::get(uint32_t index)
{
    return index < m_items.size() && m_items[index].m_value ? m_items[index].m_value.get() : nullptr;
}

const NetSenderServiceManagedFrame* NetSenderServiceQueue::get(uint32_t index) const
{
    return index < m_items.size() && m_items[index].m_value ? m_items[index].m_value.get() : nullptr;
}

void NetSenderServiceQueue::detach(uint32_t index)
{
    NetSenderServiceManagedFrame& item = *m_items[index].m_value;
    const uint32_t lane = item.m_lane;
    if (item.m_previous != UINT32_MAX)
    {
        m_items[item.m_previous].m_value->m_next = item.m_next;
    }
    else
    {
        m_heads[lane] = item.m_next;
    }
    if (item.m_next != UINT32_MAX)
    {
        m_items[item.m_next].m_value->m_previous = item.m_previous;
    }
    else
    {
        m_tails[lane] = item.m_previous;
    }
    --m_sizes[lane];
    item.m_next = item.m_previous = UINT32_MAX;
    item.m_lane += 3;
}

NetSenderServiceManagedFrame* NetSenderServiceQueue::borrow(uint32_t index)
{
    NetSenderServiceManagedFrame* item = get(index);
    if (item == nullptr || item->m_lane >= 3)
    {
        return nullptr;
    }
    detach(index);
    return item;
}

bool NetSenderServiceQueue::restore(uint32_t index)
{
    NetSenderServiceManagedFrame* item = get(index);
    if (item == nullptr || item->m_lane < 3)
    {
        return false;
    }
    const uint32_t lane = item->m_lane - 3;
    item->m_lane = lane;
    item->m_next = m_heads[lane];
    item->m_previous = UINT32_MAX;
    if (m_heads[lane] != UINT32_MAX)
    {
        m_items[m_heads[lane]].m_value->m_previous = index;
    }
    else
    {
        m_tails[lane] = index;
    }
    m_heads[lane] = index;
    ++m_sizes[lane];
    return true;
}

void NetSenderServiceQueue::release(uint32_t index)
{
    NetSenderServiceManagedFrame* item = get(index);
    if (item == nullptr)
    {
        return;
    }
    if (item->m_lane < 3)
    {
        detach(index);
    }
    m_bytes -= item->m_frame.m_payloadSize;
    --m_count;
    m_items[index].m_value.reset();
    m_items[index].m_next = m_free;
    m_free = index;
}

void NetSenderServiceQueue::clear(uint32_t lane)
{
    if (lane < 3)
    {
        while (m_heads[lane] != UINT32_MAX)
        {
            release(m_heads[lane]);
        }
    }
}

void NetSenderServiceQueue::clear()
{
    for (uint32_t lane = 0; lane < 3; ++lane)
    {
        clear(lane);
    }
}

uint32_t NetSenderServiceQueue::size(uint32_t lane) const
{
    return lane < 3 ? m_sizes[lane] : 0;
}

uint32_t NetSenderServiceQueue::size() const
{
    return m_count;
}

uint64_t NetSenderServiceQueue::bytes() const
{
    return m_bytes;
}

NetSenderServiceQueueLoan::NetSenderServiceQueueLoan(NetSenderServiceQueue& queue, uint32_t index) :
m_queue(queue),
m_index(index)
{

}

NetSenderServiceQueueLoan::~NetSenderServiceQueueLoan()
{
    release();
}

void NetSenderServiceQueueLoan::release()
{
    m_queue.release(m_index);
    m_index = UINT32_MAX;
}

bool NetSenderServiceQueueLoan::restore()
{
    if (!m_queue.restore(m_index))
    {
        return false;
    }
    m_index = UINT32_MAX;
    return true;
}

NetSenderServiceQueueScratch::~NetSenderServiceQueueScratch()
{
    if (!m_request.empty())
    {
        NetInterface::wipe(m_request.data(), static_cast<uint32_t>(m_request.size()));
    }
    if (!m_event.empty())
    {
        NetInterface::wipe(m_event.data(), static_cast<uint32_t>(m_event.size()));
    }
    if (!m_wrapped.empty())
    {
        NetInterface::wipe(m_wrapped.data(), static_cast<uint32_t>(m_wrapped.size()));
    }
}