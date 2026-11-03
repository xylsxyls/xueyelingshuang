#include "NetPeerMacro.h"
#include "NetPeerClientSession.h"
#include "NetHostSocket.h"
#include "NetTransportEntropy.h"

NetPeerClientSession::NetPeerClientSession() :
m_direct(nullptr)
{
    bindGateway(&m_factory, &m_discovery);
}

NetPeerClientSession::~NetPeerClientSession()
{
    detachBindingsForDestruction();
    delete m_direct;
}

int32_t NetPeerClientSession::bindChannel(NetClientChannel* channel)
{
    const int32_t result = NetClientSession::bindChannel(channel);
    if (result == NI_OK)
    {
        delete m_direct;
        m_direct = nullptr;
    }
    return result;
}

int32_t NetPeerClientSession::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls,
    const NetInterfaceHandshakeOptions& handshake, ::NetSessionListener* listener)
{
    if (m_direct || !hasBoundChannel())
    {
        if (!NetHostSocket::defaultSupports(false))
        {
            return NI_NOT_SUPPORTED;
        }
        const int32_t detached = NetClientSession::bindChannel(nullptr);
        if (detached != NI_OK)
        {
            return detached;
        }
        try
        {
            delete m_direct;
            m_direct = nullptr;
            m_direct = new NetTransport();
        }
        catch (...)
        {
            return NI_NO_MEMORY;
        }
        const int32_t bound = NetClientSession::bindChannel(m_direct);
        if (bound != NI_OK)
        {
            return bound;
        }
    }
    NetInterfaceTlsOptions configured = tls;
    if (configured.entropy == nullptr)
    {
        configured.entropy = NetTransportEntropy;
        configured.entropyContext = nullptr;
    }
    return NetClientSession::connect(address, port, configured, handshake, listener);
}

int32_t NetPeerClientSession::connectService(uint32_t product, const char* address, uint32_t port,
    const NetInterfaceTlsOptions& tls, const NetInterfaceHandshakeOptions& handshake,
    ::NetSessionListener* listener, NetGatewayTransportFactory* factory)
{
    if (factory == nullptr && boundGatewayFactory() == &m_factory && !NetHostSocket::defaultSupports(false))
    {
        return NI_NOT_SUPPORTED;
    }
    const int32_t detached = NetClientSession::bindChannel(nullptr);
    if (detached != NI_OK)
    {
        return detached;
    }
    delete m_direct;
    m_direct = nullptr;
    NetInterfaceTlsOptions configured = tls;
    if (configured.entropy == nullptr)
    {
        configured.entropy = NetTransportEntropy;
        configured.entropyContext = nullptr;
    }
    return NetClientSession::connectService(product, address, port, configured, handshake, listener, factory);
}