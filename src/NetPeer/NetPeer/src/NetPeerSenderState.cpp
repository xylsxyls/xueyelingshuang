#include "NetPeerMacro.h"
#include "NetPeer.h"
#include "NetPeerSenderState.h"
#include "NetPeerClientState.h"

NetPeerSenderState::NetPeerSenderState() :
m_client(nullptr),
m_listener(nullptr),
m_lastError(NI_INVALID_STATE)
{

}

NetPeerClientCallbacks NetPeerSenderState::callbacks()
{
    NetPeerClientCallbacks value = {};
    value.structSize = sizeof(value);
    value.version = 1;
    value.context = this;
    value.ready = ready;
    value.result = result;
    value.frame = frame;
    value.closed = closed;
    return value;
}

int32_t NetPeerSenderState::connect(uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    if (m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    if (listener == nullptr || product > INT32_MAX || options.m_localGatewayPid != 0 ||
        options.m_network.structSize != sizeof(NetPeerClientOptions) || options.m_network.version != 1)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (!m_client->inactive())
    {
        return NI_INVALID_STATE;
    }
    m_listener = listener;
    return product == 0 ? NetPeer::instance().clientConnect(m_client, &options.m_network) :
        NetPeer::instance().clientConnectService(m_client, product, &options.m_network);
}

void NetPeerSenderState::ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
{
    static_cast<NetPeerSenderState*>(context)->m_listener->onSessionReady(generation, requests, chunk);
}

void NetPeerSenderState::result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type,
    uint32_t status, const uint8_t* bytes, uint32_t size)
{
    static_cast<NetPeerSenderState*>(context)->m_listener->onSessionResult(generation, id, product, type, status, bytes, size);
}

int32_t NetPeerSenderState::frame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
{
    return static_cast<NetPeerSenderState*>(context)->m_listener->onSessionFrame(generation, *frame) ? 1 : 0;
}

void NetPeerSenderState::closed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    static_cast<NetPeerSenderState*>(context)->m_listener->onSessionClosed(generation, reason, detail);
}