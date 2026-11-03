#include "NetGatewayLocalServer.h"
#include <chrono>
#include <cstring>

NetGatewayLocalServer::NetGatewayLocalServer() :
m_local(m_inbox),
m_gateway(new NetGatewayServer(this)),
m_receiving(false),
m_stopping(false)
{

}

NetGatewayLocalServer::~NetGatewayLocalServer()
{
    shutdown();
    for (auto& entry : m_credentials)
    {
        volatile uint8_t* secret = entry.second.m_secret;
        for (uint32_t index = 0; index < 32; ++index)
        {
            secret[index] = 0;
        }
    }
}

int32_t NetGatewayLocalServer::entropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return NetTransport::randomBytes(output, size);
}

uint64_t NetGatewayLocalServer::now()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

int32_t NetGatewayLocalServer::startLocal(const NetInterfaceTlsOptions& tls, const std::vector<NetGatewayServiceCredential>& credentials)
{
    if (m_receiving || m_stopping || !m_credentials.empty() || credentials.empty() || credentials.size() > 32)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::map<uint32_t, NetGatewayServiceCredential> checked;
        for (const NetGatewayServiceCredential& credential : credentials)
        {
            if (NetInterfaceService::hello(credential.m_secret, 32, nullptr, 0) != NI_BUFFER_TOO_SMALL ||
                !checked.insert(std::make_pair(credential.m_product, credential)).second)
            {
                return NI_INVALID_ARGUMENT;
            }
        }
        NetInterfaceTlsOptions options = tls;
        if (options.entropy == nullptr)
        {
            options.entropy = &NetGatewayLocalServer::entropy;
            options.entropyContext = nullptr;
        }
        ProcessWork& work = ProcessWork::instance();
        if (work.receiveBufferCount() != 0)
        {
            return NI_INVALID_STATE;
        }
        work.addProcessReceiveCallback(&m_inbox);
        if (!work.initReceive(16452, 64))
        {
            m_local.shutdown();
            work.uninitReceive();
            return NI_INVALID_STATE;
        }
        m_receiving = true;
        // 接收器先就绪，再开放本地TLS；失败只回收本对象刚取得的资源。
        const int32_t opened = m_local.listen(options, this, 64);
        if (opened != NI_OK)
        {
            shutdown();
            return opened;
        }
        m_credentials.swap(checked);
        return NI_OK;
    }
    catch (...)
    {
        shutdown();
        return NI_NO_MEMORY;
    }
}

bool NetGatewayLocalServer::poll()
{
    if (m_stopping)
    {
        return false;
    }
    if (m_receiving)
    {
        const uint64_t current = now();
        for (const auto& entry : m_peers)
        {
            if (entry.second.m_endpoint == 0 && current >= entry.second.m_deadline)
            {
                m_local.close(entry.first, NI_TIMEOUT);
            }
        }
        if (!m_local.poll())
        {
            shutdown();
            return false;
        }
    }
    return m_gateway->poll();
}

void NetGatewayLocalServer::onLocalReady(uint64_t peer, int32_t pid)
{
    (void)pid;
    NetGatewayLocalPeer state = {};
    state.m_deadline = now() + 5000;
    if (!m_peers.insert(std::make_pair(peer, state)).second)
    {
        m_local.close(peer, NI_PROTOCOL_ERROR);
    }
}

bool NetGatewayLocalServer::onLocalFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    const auto found = m_peers.find(peer);
    if (found == m_peers.end() || m_stopping)
    {
        m_local.close(peer, NI_INVALID_STATE);
        return true;
    }
    if (found->second.m_endpoint == 0)
    {
        const auto credential = m_credentials.find(frame.m_productId);
        uint32_t limit = 0;
        if (now() >= found->second.m_deadline || credential == m_credentials.end() ||
            NetInterfaceService::authenticate(frame, credential->second.m_secret, limit) != NI_OK)
        {
            m_local.close(peer, NI_PROTOCOL_ERROR);
            return true;
        }
        uint64_t endpoint = 0;
        const int32_t added = m_gateway->addService(frame.m_productId, frame.m_productId == 0, limit, endpoint);
        if (added != NI_OK)
        {
            m_local.close(peer, added);
            return true;
        }
        found->second.m_endpoint = endpoint;
        m_routes.insert(std::make_pair(endpoint, peer));
        NetInterfaceFrame response = {};
        response.m_kind = NI_HELLO_REPLY;
        response.m_messageType = 3;
        response.m_productId = frame.m_productId;
        response.m_requestId = endpoint;
        const int32_t sent = m_local.send(peer, response);
        if (sent != NI_OK)
        {
            m_gateway->closeEndpoint(endpoint, sent);
        }
        return true;
    }
    const int32_t received = m_gateway->receiveService(found->second.m_endpoint, frame);
    if (received == NI_LIMIT_EXCEEDED && (frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK || frame.m_kind == NI_EVENT))
    {
        return false;
    }
    if (received != NI_OK)
    {
        m_local.close(peer, received);
    }
    return true;
}

void NetGatewayLocalServer::onLocalClosed(uint64_t peer, int32_t reason)
{
    const auto found = m_peers.find(peer);
    if (found != m_peers.end())
    {
        const uint64_t endpoint = found->second.m_endpoint;
        m_peers.erase(found);
        if (endpoint != 0)
        {
            m_routes.erase(endpoint);
            m_gateway->closeEndpoint(endpoint, reason);
        }
    }
}

int32_t NetGatewayLocalServer::send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetGatewayOrigin& origin)
{
    const auto found = m_routes.find(endpoint);
    if (m_stopping || found == m_routes.end() || frame.m_payloadSize > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT)
    {
        return NI_NOT_FOUND;
    }
    try
    {
        std::vector<uint8_t> payload(frame.m_payloadSize + 32);
        uint32_t required = 0;
        const int32_t encoded = NetInterfaceService::wrap(origin, frame.m_payload, frame.m_payloadSize, payload.data(),
                                                          static_cast<uint32_t>(payload.size()), required);
        if (encoded != NI_OK)
        {
            return encoded;
        }
        NetInterfaceFrame wrapped = frame;
        wrapped.m_payload = payload.data();
        wrapped.m_payloadSize = required;
        return m_local.send(found->second, wrapped);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetGatewayLocalServer::close(uint64_t endpoint)
{
    const auto found = m_routes.find(endpoint);
    if (found != m_routes.end())
    {
        m_local.close(found->second, NI_CANCELLED);
    }
}

void NetGatewayLocalServer::shutdown()
{
    if (m_stopping)
    {
        return;
    }
    m_stopping = true;
    m_gateway->shutdown();
    m_local.shutdown();
    if (m_receiving)
    {
        ProcessWork::instance().uninitReceive();
        m_receiving = false;
    }
}

NetGatewayServer& NetGatewayLocalServer::gateway()
{
    return *m_gateway;
}

uint32_t NetGatewayLocalServer::serviceCount() const
{
    return static_cast<uint32_t>(m_routes.size());
}