#include "NetPeerClientState.h"
#include "NetPeer.h"
#include <cstring>

NetPeerClientOpaque::NetPeerClientOpaque(const NetPeerClientCallbacks& callbacks) :
m_callbacks(callbacks),
m_dispatch(&m_session),
m_polling(false),
m_callbackDepth(0)
{

}

NetPeerClientOpaque::~NetPeerClientOpaque()
{
    m_dispatch.shutdown();
}

void NetPeerClientOpaque::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    ++m_callbackDepth;
    try
    {
        m_callbacks.ready(m_callbacks.context, generation, maxRequests, maxChunk);
    }
    catch (...)
    {
        m_session.close(NI_INVALID_STATE);
    }
    --m_callbackDepth;
}

void NetPeerClientOpaque::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
                                          const uint8_t* data, uint32_t size)
{
    ++m_callbackDepth;
    try
    {
        m_callbacks.result(m_callbacks.context, generation, requestId, product, type, status, data, size);
    }
    catch (...)
    {
        m_session.close(NI_INVALID_STATE);
    }
    --m_callbackDepth;
}

bool NetPeerClientOpaque::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    ++m_callbackDepth;
    bool accepted = true;
    try
    {
        accepted = m_callbacks.frame(m_callbacks.context, generation, &frame) != 0;
    }
    catch (...)
    {
        m_session.close(NI_INVALID_STATE);
    }
    --m_callbackDepth;
    return accepted;
}

void NetPeerClientOpaque::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    ++m_callbackDepth;
    try
    {
        m_callbacks.closed(m_callbacks.context, generation, reason, detail);
    }
    catch (...)
    {

    }
    --m_callbackDepth;
}

bool NetPeerClientOpaque::inactive() const
{
    return m_callbackDepth == 0 && !m_polling && m_dispatch.quiescent();
}

int32_t NetPeerClientOpaque::connect(const NetPeerClientOptions& options, uint32_t product)
{
    if (!inactive())
    {
        return NI_INVALID_STATE;
    }
    if (options.platform < 1 || options.platform > 3)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceHandshakeOptions handshake = {};
    handshake.m_platform = options.platform;
    handshake.m_capabilities = options.capabilities;
    handshake.m_maxRequests = options.maxRequests;
    handshake.m_maxChunk = options.maxChunk;
    handshake.m_timeoutMs = options.handshakeTimeoutMs;
    std::memcpy(handshake.m_device, options.device, sizeof(handshake.m_device));
    try
    {
        if (product != 0 || m_session.serviceMode())
        {
            const int32_t detached = m_session.bindChannel(nullptr);
            if (detached != NI_OK)
            {
                return detached;
            }
        }
        int32_t result = m_dispatch.prepare(this);
        if (result != NI_OK)
        {
            return result;
        }
        result = product == 0 ? m_session.connect(options.address, options.port, options.tls, handshake, m_dispatch.listener()) :
            m_session.connectService(product, options.address, options.port, options.tls, handshake, m_dispatch.listener());
        if (result == NI_OK)
        {
            result = m_dispatch.started(m_session.generation(), product);
            if (result != NI_OK)
            {
                // 会话已接纳而本地代次未能启动时也必须收敛，不留下不可使用的连接。
                m_session.close(result);
            }
        }
        return result;
    }
    catch (...)
    {
        m_session.close(NI_NO_MEMORY);
        return NI_NO_MEMORY;
    }
}