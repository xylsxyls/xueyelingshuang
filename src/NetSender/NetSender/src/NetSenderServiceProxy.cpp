#include "NetSenderServiceProxy.h"
#include "NetSenderMacro.h"
#include "NetSenderServiceProxyState.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <chrono>
#include <cstring>

/** 取得本执行者的单调毫秒；期限不使用业务上报UTC
@return 单调毫秒
*/
static uint64_t ProxyNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetSenderServiceProxy::NetSenderServiceProxy(NetSenderService& service) :
m_state(new NetSenderServiceProxyState(service))
{

}

NetSenderServiceProxy::~NetSenderServiceProxy()
{
    reset();
    delete m_state;
}

int32_t NetSenderServiceProxy::configurePools(const NetBufferPoolConfig& bodies, const LockFreeHashMapPoolConfig& records)
{
    return m_state->configurePools(bodies, records);
}

bool NetSenderServiceProxy::valid() const
{
    return m_state->m_valid;
}

void NetSenderServiceProxy::reset()
{
    m_state->reset();
}

bool NetSenderServiceProxy::accept(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin)
{
    if (!m_state->m_valid)
    {
        return true;
    }
    if (origin.m_endpoint == 0)
    {
        m_state->fail();
        return true;
    }
    if (frame.m_kind == NI_CANCEL)
    {
        NetInterfaceFrame unused = {};
        const uint32_t index = m_state->find(frame.m_requestId, true);
        if (NetInterfaceRpc::control(frame, unused) != NI_OK ||
            (index != UINT32_MAX && !m_state->call(index).matches(frame, origin)))
        {
            m_state->fail();
        }
        else if (index != UINT32_MAX)
        {
            NetSenderProxyCall& call = m_state->call(index);
            if (call.m_phase == NET_SERVICE_PROXY_WAITING)
            {
                call.m_phase = NET_SERVICE_PROXY_CANCEL;
                call.m_cancelUntil = ProxyNow() + 5000;
            }
            else if (call.m_phase != NET_SERVICE_PROXY_CANCEL)
            {
                m_state->erase(index);
            }
        }
        return true;
    }
    NetInterfaceRpcView rpc = {};
    if (NetInterfaceRpc::parse(frame, rpc) != NI_OK)
    {
        m_state->fail();
        return true;
    }
    if (frame.m_kind == NI_RESPONSE)
    {
        const uint32_t index = m_state->find(frame.m_requestId, false);
        if (index == UINT32_MAX)
        {
            return true;
        }
        NetSenderProxyCall& call = m_state->call(index);
        if (!call.matches(frame, origin) ||
            (call.m_phase != NET_SERVICE_PROXY_WAITING && call.m_phase != NET_SERVICE_PROXY_CANCEL))
        {
            m_state->fail();
            return true;
        }
        if (call.m_phase == NET_SERVICE_PROXY_CANCEL)
        {
            m_state->erase(index);
            return true;
        }
        if (!call.assign(*m_state->m_bodies, rpc))
        {
            return false;
        }
        m_state->m_bytes += rpc.m_size;
        call.m_phase = NET_SERVICE_PROXY_REPLY;
        call.m_status = rpc.m_value;
        return true;
    }
    if (rpc.m_parent != 0 || m_state->find(frame.m_requestId, true) != UINT32_MAX)
    {
        m_state->fail();
        return true;
    }
    const uint32_t index = m_state->acquire(rpc);
    if (index == UINT32_MAX)
    {
        return false;
    }
    NetSenderProxyCall& call = m_state->call(index);
    call.m_origin = origin;
    call.m_root = frame.m_requestId;
    call.m_product = frame.m_productId;
    call.m_type = frame.m_messageType;
    call.m_deadline = ProxyNow() + rpc.m_value;
    call.m_phase = NET_SERVICE_PROXY_QUEUED;
    m_state->append(index);
    return true;
}

void NetSenderServiceProxy::poll()
{
    if (!m_state->m_valid || m_state->m_polling)
    {
        return;
    }
    m_state->m_polling = true;
    bool blocked = false;
    for (uint32_t index = m_state->m_first; m_state->m_valid && index != UINT32_MAX;)
    {
        const uint32_t next = m_state->call(index).m_next;
        NetInterfaceFrame frame = {};
        const uint64_t now = ProxyNow();
        const bool queued = m_state->call(index).m_phase == NET_SERVICE_PROXY_QUEUED;
        if (!(blocked && queued && now < m_state->call(index).m_deadline) && m_state->prepare(index, now, frame))
        {
            int32_t result = NI_INVALID_STATE;
            try
            {
                result = m_state->m_service->send(frame);
            }
            catch (...)
            {
                // 下层未声明异常接纳结果，终止本代；不能盲目重发可能已经接纳的请求。
                m_state->fail();
                break;
            }
            if (!m_state->m_valid) { break; }
            blocked = m_state->finish(index, result) || blocked;
        }
        index = next;
    }
    m_state->m_polling = false;
}