#include "NetSenderServiceProxyState.h"
#include "NetSenderMacro.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <chrono>
#include <cstring>

NetSenderProxyCall::NetSenderProxyCall() :
m_phase(NET_SERVICE_PROXY_FREE),
m_root(0),
m_child(0),
m_deadline(0),
m_cancelUntil(0),
m_product(0),
m_type(0),
m_status(0),
m_size(0),
m_previous(UINT32_MAX),
m_next(UINT32_MAX)
{
    std::memset(&m_origin, 0, sizeof(m_origin));
    std::memset(m_header, 0, sizeof(m_header));
}

NetSenderProxyCall::~NetSenderProxyCall()
{
    release();
}

bool NetSenderProxyCall::assign(NetBufferPool& bodies, const NetInterfaceRpcView& rpc)
{
    if (m_size != 0 || m_wire.data() != nullptr || rpc.m_size > 1048560 || (rpc.m_size != 0 && rpc.m_data == nullptr))
    {
        return false;
    }
    if (rpc.m_size != 0)
    {
        const int32_t result = bodies.allocate(16 + rpc.m_size, m_wire);
        if (result != NI_OK)
        {
            return false;
        }
        std::memcpy(m_wire.data() + 16, rpc.m_data, rpc.m_size);
    }
    m_size = rpc.m_size;
    return true;
}

void NetSenderProxyCall::release()
{
    if (m_wire.data() != nullptr)
    {
        NetInterface::wipe(m_wire.data(), 16 + m_size);
        m_wire.reset();
    }
    NetInterface::wipe(m_header, sizeof(m_header));
    m_size = 0;
}

void NetSenderProxyCall::clear()
{
    release();
    std::memset(&m_origin, 0, sizeof(m_origin));
    m_phase = NET_SERVICE_PROXY_FREE;
    m_root = m_child = m_deadline = m_cancelUntil = 0;
    m_product = m_type = m_status = 0;
    m_previous = m_next = UINT32_MAX;
}

bool NetSenderProxyCall::matches(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin) const
{
    return m_product == frame.m_productId && m_type == frame.m_messageType &&
        m_origin.m_endpoint == origin.m_endpoint && m_origin.m_platform == origin.m_platform &&
        std::memcmp(m_origin.m_device, origin.m_device, sizeof(origin.m_device)) == 0;
}

NetSenderProxySlot::NetSenderProxySlot() :
m_next(UINT32_MAX)
{

}

/** 按实际调用记录槽大小计算默认存储，不设置固定调用数量门槛
@return 可在接纳前由宿主替换的记录池配置
*/
static LockFreeHashMapPoolConfig ProxyRecords()
{
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 128;
    config.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetSenderProxyCall>>);
    config.directoryCapacity = 128;
    config.lookupCacheCapacity = 0;
    return config;
}

NetSenderServiceProxyState::NetSenderServiceProxyState(NetSenderService& service) :
m_service(&service),
m_bodies(new NetBufferPool(NetBufferPoolConfig())),
m_records(new NetObjectPool<NetSenderProxyCall>(ProxyRecords())),
m_free(UINT32_MAX),
m_used(false),
m_first(UINT32_MAX),
m_last(UINT32_MAX),
m_count(0),
m_bytes(0),
m_valid(m_bodies->status() == NI_OK && m_records->status() == NI_OK),
m_polling(false)
{

}

int32_t NetSenderServiceProxyState::configurePools(const NetBufferPoolConfig& bodies, const LockFreeHashMapPoolConfig& records)
{
    if (m_used || m_polling)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetBufferPool> preparedBodies(new NetBufferPool(bodies));
        if (preparedBodies->status() != NI_OK) { return preparedBodies->status(); }
        std::unique_ptr<NetObjectPool<NetSenderProxyCall>> preparedRecords(new NetObjectPool<NetSenderProxyCall>(records));
        if (preparedRecords->status() != NI_OK) { return preparedRecords->status(); }
        m_bodies.swap(preparedBodies);
        m_records.swap(preparedRecords);
        m_valid = true;
        return NI_OK;
    }
    catch (...) { return NI_NO_MEMORY; }
}

uint32_t NetSenderServiceProxyState::acquire(const NetInterfaceRpcView& rpc)
{
    NetObjectReference<NetSenderProxyCall> owned;
    if (m_records->create(owned) != NI_OK || !owned->assign(*m_bodies, rpc))
    {
        return UINT32_MAX;
    }
    uint32_t index = m_free;
    if (index == UINT32_MAX)
    {
        if (m_calls.size() >= UINT32_MAX) { return UINT32_MAX; }
        try
        {
            index = static_cast<uint32_t>(m_calls.size());
            m_calls.push_back(NetSenderProxySlot());
        }
        catch (...) { return UINT32_MAX; }
    }
    else
    {
        m_free = m_calls[index].m_next;
    }
    m_calls[index].m_value = std::move(owned);
    return index;
}

NetSenderProxyCall& NetSenderServiceProxyState::call(uint32_t index)
{
    return *m_calls[index].m_value;
}

const NetSenderProxyCall& NetSenderServiceProxyState::call(uint32_t index) const
{
    return *m_calls[index].m_value;
}

void NetSenderServiceProxyState::reset()
{
    while (m_first != UINT32_MAX)
    {
        erase(m_first);
    }
    m_valid = m_bodies->status() == NI_OK && m_records->status() == NI_OK;
}

void NetSenderServiceProxyState::fail()
{
    reset();
    m_valid = false;
}

uint32_t NetSenderServiceProxyState::find(uint64_t id, bool root) const
{
    for (uint32_t index = m_first; index != UINT32_MAX; index = this->call(index).m_next)
    {
        if ((root ? this->call(index).m_root : this->call(index).m_child) == id)
        {
            return index;
        }
    }
    return UINT32_MAX;
}

void NetSenderServiceProxyState::append(uint32_t index)
{
    NetSenderProxyCall& call = this->call(index);
    call.m_previous = m_last;
    call.m_next = UINT32_MAX;
    if (m_last == UINT32_MAX)
    {
        m_first = index;
    }
    else
    {
        this->call(m_last).m_next = index;
    }
    m_last = index;
    m_used = true;
    ++m_count;
    m_bytes += call.m_size;
}

void NetSenderServiceProxyState::erase(uint32_t index)
{
    NetSenderProxyCall& call = this->call(index);
    if (call.m_previous == UINT32_MAX)
    {
        m_first = call.m_next;
    }
    else
    {
        this->call(call.m_previous).m_next = call.m_next;
    }
    if (call.m_next == UINT32_MAX)
    {
        m_last = call.m_previous;
    }
    else
    {
        this->call(call.m_next).m_previous = call.m_previous;
    }
    --m_count;
    m_bytes -= call.m_size;
    m_calls[index].m_value.reset();
    m_calls[index].m_next = m_free;
    m_free = index;
}

bool NetSenderServiceProxyState::prepare(uint32_t index, uint64_t now, NetInterfaceFrame& frame)
{
    std::memset(&frame, 0, sizeof(frame));
    NetSenderProxyCall& call = this->call(index);
    if (now >= call.m_deadline && call.m_phase != NET_SERVICE_PROXY_CANCEL)
    {
        if (call.m_phase == NET_SERVICE_PROXY_WAITING)
        {
            call.m_phase = NET_SERVICE_PROXY_CANCEL;
            call.m_cancelUntil = call.m_deadline + 5000;
        }
        else
        {
            erase(index);
            return false;
        }
    }
    if (call.m_phase == NET_SERVICE_PROXY_WAITING)
    {
        return false;
    }
    frame.m_productId = call.m_product;
    frame.m_messageType = call.m_type;
    frame.m_requestId = call.m_child;
    if (call.m_phase == NET_SERVICE_PROXY_CANCEL)
    {
        if (now >= call.m_cancelUntil)
        {
            erase(index);
            return false;
        }
        frame.m_kind = NI_CANCEL;
        return true;
    }
    const bool reply = call.m_phase == NET_SERVICE_PROXY_REPLY;
    if (!reply && call.m_child == 0 && m_service->nextRequestId(call.m_child) != NI_OK)
    {
        fail();
        return false;
    }
    frame.m_kind = static_cast<uint16_t>(reply ? NI_RESPONSE : NI_REQUEST);
    frame.m_requestId = reply ? call.m_root : call.m_child;
    uint8_t* wire = call.m_wire.data() != nullptr ? call.m_wire.data() : call.m_header;
    uint32_t required = 0;
    // 正文已在+16处拥有，仅改写头；避免重复制和memcpy同地址重叠。
    if (NetInterfaceRpc::encode(frame.m_kind, reply ? call.m_status : static_cast<uint32_t>(call.m_deadline - now),
        reply ? 0 : call.m_root, nullptr, 0, wire, 16, required) != NI_OK)
    {
        fail();
        return false;
    }
    frame.m_payload = wire;
    frame.m_payloadSize = 16 + call.m_size;
    return true;
}

bool NetSenderServiceProxyState::finish(uint32_t index, int32_t result)
{
    NetSenderProxyCall& call = this->call(index);
    if (result == NI_LIMIT_EXCEEDED)
    {
        if (call.m_phase == NET_SERVICE_PROXY_QUEUED)
        {
            // 尚未交付的号不复用，下次发送现场重新取号；不阻挡其他处理器共用服务。
            call.m_child = 0;
        }
        return call.m_phase == NET_SERVICE_PROXY_QUEUED;
    }
    if (result == NI_NOT_FOUND)
    {
        erase(index);
    }
    else if (result != NI_OK)
    {
        fail();
    }
    else if (call.m_phase == NET_SERVICE_PROXY_QUEUED)
    {
        m_bytes -= call.m_size;
        call.release();
        call.m_phase = NET_SERVICE_PROXY_WAITING;
    }
    else
    {
        erase(index);
    }
    return false;
}