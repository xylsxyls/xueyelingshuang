#include "NetSenderMacro.h"
#include "NetSenderClientChannel.h"
#include "CSystem/CSystemAPI.h"

NetSenderClientChannel::NetSenderClientChannel(NetSenderLocalInbox& inbox, int32_t gatewayPid) :
m_inbox(&inbox),
m_local(nullptr),
m_listener(nullptr),
m_pid(gatewayPid),
m_peer(0),
m_closed(true)
{

}

NetSenderClientChannel::~NetSenderClientChannel()
{
    shutdown();
    delete m_local;
}

int32_t NetSenderClientChannel::Entropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return CSystem::secureRandomBytes(output, size) ? 0 : -1;
}

int32_t NetSenderClientChannel::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
                                        NetTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (!m_closed)
    {
        return NI_INVALID_STATE;
    }
    if (address == nullptr || port == 0 || listener == nullptr || m_pid <= 0 || options.server != 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        NetSenderLocal* local = new NetSenderLocal(*m_inbox);
        delete m_local;
        m_local = local;
        auto tls = options;
        if (tls.entropy == nullptr)
        {
            tls.entropy = &Entropy;
            tls.entropyContext = nullptr;
        }
        m_listener = listener;
        m_closed = false;
        const int32_t result = m_local->connect(m_pid, tls, this, m_peer);
        if (result != NI_OK)
        {
            m_closed = true;
            m_listener = nullptr;
            return result;
        }
        peer = m_peer;
        return NI_OK;
    }
    catch (...)
    {
        m_closed = true;
        m_listener = nullptr;
        return NI_NO_MEMORY;
    }
}

bool NetSenderClientChannel::poll()
{
    return !m_closed && m_local && m_local->poll() && !m_closed;
}

int32_t NetSenderClientChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
{
    return !m_closed && m_local ? m_local->send(peer, frame) : NI_INVALID_STATE;
}

void NetSenderClientChannel::close(uint64_t peer, int32_t reason)
{
    if (m_local)
    {
        m_local->close(peer, reason);
    }
}

void NetSenderClientChannel::shutdown()
{
    if (m_local)
    {
        m_local->shutdown();
    }
    m_closed = true;
    m_listener = nullptr;
}

void NetSenderClientChannel::onLocalReady(uint64_t peer, int32_t pid)
{
    if (m_listener == nullptr || peer != m_peer || pid != m_pid)
    {
        close(peer, NI_PROTOCOL_ERROR);
        return;
    }
    m_listener->onReady(peer);
}

bool NetSenderClientChannel::onLocalFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    return m_listener == nullptr || m_closed ? true : m_listener->onFrame(peer, frame);
}

void NetSenderClientChannel::onLocalClosed(uint64_t peer, int32_t reason)
{
    if (m_closed || peer != m_peer)
    {
        return;
    }
    m_closed = true;
    if (m_listener != nullptr)
    {
        m_listener->onClosed(peer, reason, 0);
    }
}

int32_t NetSenderClientChannel::outputState(uint64_t peer) const
{
    return !m_closed && m_local ? m_local->outputState(peer) : NI_INVALID_STATE;
}