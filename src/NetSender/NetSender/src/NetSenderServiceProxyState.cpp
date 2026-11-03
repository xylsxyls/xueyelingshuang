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
m_previous(32),
m_next(32)
{
    std::memset(&m_origin, 0, sizeof(m_origin));
    std::memset(m_header, 0, sizeof(m_header));
}

NetSenderProxyCall::~NetSenderProxyCall()
{
    release();
}

bool NetSenderProxyCall::assign(const NetInterfaceRpcView& rpc)
{
    if (rpc.m_size != 0)
    {
        try
        {
            std::unique_ptr<uint8_t[]> wire(new uint8_t[16 + rpc.m_size]);
            std::memcpy(wire.get() + 16, rpc.m_data, rpc.m_size);
            m_wire.swap(wire);
        }
        catch (...)
        {
            return false;
        }
    }
    m_size = rpc.m_size;
    return true;
}

void NetSenderProxyCall::release()
{
    if (m_wire)
    {
        NetInterface::wipe(m_wire.get(), 16 + m_size);
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
    m_previous = m_next = 32;
}

bool NetSenderProxyCall::matches(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin) const
{
    return m_product == frame.m_productId && m_type == frame.m_messageType &&
        m_origin.m_endpoint == origin.m_endpoint && m_origin.m_platform == origin.m_platform &&
        std::memcmp(m_origin.m_device, origin.m_device, sizeof(origin.m_device)) == 0;
}

NetSenderServiceProxyState::NetSenderServiceProxyState(NetSenderService& service) :
m_service(&service),
m_first(32),
m_last(32),
m_count(0),
m_bytes(0),
m_valid(true),
m_polling(false)
{

}

void NetSenderServiceProxyState::reset()
{
    for (uint32_t index = 0; index < 32; ++index)
    {
        m_calls[index].clear();
    }
    m_first = m_last = 32;
    m_count = m_bytes = 0;
    m_valid = true;
}

void NetSenderServiceProxyState::fail()
{
    reset();
    m_valid = false;
}

uint32_t NetSenderServiceProxyState::find(uint64_t id, bool root) const
{
    for (uint32_t index = m_first; index != 32; index = m_calls[index].m_next)
    {
        if ((root ? m_calls[index].m_root : m_calls[index].m_child) == id)
        {
            return index;
        }
    }
    return 32;
}

void NetSenderServiceProxyState::append(uint32_t index)
{
    NetSenderProxyCall& call = m_calls[index];
    call.m_previous = m_last;
    call.m_next = 32;
    if (m_last == 32)
    {
        m_first = index;
    }
    else
    {
        m_calls[m_last].m_next = index;
    }
    m_last = index;
    ++m_count;
    m_bytes += call.m_size;
}

void NetSenderServiceProxyState::erase(uint32_t index)
{
    NetSenderProxyCall& call = m_calls[index];
    if (call.m_previous == 32)
    {
        m_first = call.m_next;
    }
    else
    {
        m_calls[call.m_previous].m_next = call.m_next;
    }
    if (call.m_next == 32)
    {
        m_last = call.m_previous;
    }
    else
    {
        m_calls[call.m_next].m_previous = call.m_previous;
    }
    --m_count;
    m_bytes -= call.m_size;
    call.clear();
}

bool NetSenderServiceProxyState::prepare(uint32_t index, uint64_t now, NetInterfaceFrame& frame)
{
    std::memset(&frame, 0, sizeof(frame));
    NetSenderProxyCall& call = m_calls[index];
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
    uint8_t* wire = call.m_wire ? call.m_wire.get() : call.m_header;
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
    NetSenderProxyCall& call = m_calls[index];
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