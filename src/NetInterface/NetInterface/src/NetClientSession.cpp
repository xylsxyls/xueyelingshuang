#include "NetClientSession.h"
#include "NetInterfaceMacro.h"
#include "NetClientSessionState.h"
#include "NetInterface.h"
#include "NetClientDiscovery.h"
#include "NetInterfaceTransport.h"
#include "NetSession.h"
#include "NetGatewayClientChannel.h"
#include <algorithm>
#include <chrono>
#include <cstring>

/** 恢复期限只使用同一执行者的单调时间
@return 单调毫秒
*/
static uint64_t ClientSessionNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetClientSession::NetClientSession() :
m_state(new NetClientSessionState())
{

}

NetClientSession::~NetClientSession()
{
    shutdown();
    delete m_state;
}

bool NetClientSession::discoveryStopped() const
{
    if (m_state->m_discovery == nullptr)
    {
        return true;
    }
    try
    {
        return m_state->m_discovery->stopped();
    }
    catch (...)
    {
        // 无法确认停止时继续保留借用，不能伪造异步资源已经收敛。
        return false;
    }
}

void NetClientSession::stopDiscovery()
{
    if (m_state->m_discovery == nullptr)
    {
        return;
    }
    try
    {
        m_state->m_discovery->shutdown();
    }
    catch (...)
    {
        // 停止请求异常也必须检查真实状态；仍有资源则交给后续poll收敛。
    }
    if (discoveryStopped())
    {
        m_state->m_discovery = nullptr;
    }
}

void NetClientSession::releaseAttempt()
{
    // 调用方先解绑NetSession；先销毁借用运输的通道，最后销毁运输。
    if (m_state->m_channel)
    {
        m_state->m_channel.reset();
    }
    else if (m_state->m_transport)
    {
        try
        {
            m_state->m_transport->shutdown();
        }
        catch (...)
        {
            // 工厂已创建而通道未创建时也执行一次停止；运输析构负责自身资源。
        }
    }
    m_state->m_transport.reset();
}

int32_t NetClientSession::bindGateway(NetGatewayTransportFactory* factory, NetClientDiscovery* discovery)
{
    if (m_state->active() || executing())
    {
        return NI_INVALID_STATE;
    }
    if (factory == nullptr && discovery != nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    shutdown();
    if (!quiescent())
    {
        return NI_INVALID_STATE;
    }
    m_state->m_providerFactory = factory;
    m_state->m_providerDiscovery = discovery;
    return NI_OK;
}

int32_t NetClientSession::configureGateways(const std::string& seeds, const std::string& cachePath)
{
    if (m_state->active() || executing())
    {
        return NI_INVALID_STATE;
    }
    if (cachePath.size() > 4096 || cachePath.find('\0') != std::string::npos)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::vector<NetGatewayAddress> points;
        const int32_t parsed = seeds.empty() ? NI_OK : NetClientDiscovery::parseSeeds(seeds, points);
        if (parsed != NI_OK)
        {
            return parsed;
        }
        std::string path(cachePath);
        m_state->m_seeds.swap(points);
        m_state->m_cachePath.swap(path);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetClientSession::configureRequestPool(const LockFreeHashMapPoolConfig& config)
{
    if (m_state->active() || m_state->m_callback)
    {
        return NI_INVALID_STATE;
    }
    return m_state->m_session.configureRequestPool(config);
}

int32_t NetClientSession::configureCompression(const uint32_t* types, uint32_t count)
{
    return m_state->active() || executing() ? NI_INVALID_STATE :
        m_state->m_session.configureCompression(types, count);
}

int32_t NetClientSession::bindChannel(NetClientChannel* channel)
{
    if (m_state->active() || executing())
    {
        return NI_INVALID_STATE;
    }
    shutdown();
    if (!quiescent())
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = m_state->m_session.bindChannel(channel);
    if (result == NI_OK)
    {
        m_state->m_external = channel;
    }
    return result;
}

int32_t NetClientSession::configureConnectionPools(const NetConnectionPoolConfig& config)
{
    if (m_state->m_generation != 0 || m_state->active() || executing())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetConnectionPoolConfig> copy(new NetConnectionPoolConfig(config));
        m_state->m_connectionPools.swap(copy);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetClientSession::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls,
    const NetInterfaceHandshakeOptions& handshake, NetSessionListener* listener)
{
    if (!quiescent() || m_state->m_generation == UINT64_MAX)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_connectionPools)
    {
        const int32_t configured = m_state->m_external == nullptr ? NI_NOT_SUPPORTED :
            m_state->m_external->configureConnectionPools(*m_state->m_connectionPools);
        if (configured != NI_OK)
        {
            return configured;
        }
    }
    const int32_t result = m_state->m_session.connect(address, port, tls, handshake, listener == nullptr ? nullptr : this);
    if (result == NI_OK)
    {
        reserveGeneration();
        m_state->m_listener = listener;
        m_state->m_service = false;
        m_state->m_phase = NET_CLIENT_CONNECTING;
    }
    return result;
}

int32_t NetClientSession::connectService(uint32_t product, const char* address, uint32_t port,
    const NetInterfaceTlsOptions& tls, const NetInterfaceHandshakeOptions& handshake,
    NetSessionListener* listener, NetGatewayTransportFactory* factory)
{
    if (!quiescent() || m_state->m_generation == UINT64_MAX)
    {
        return NI_INVALID_STATE;
    }
    if (listener == nullptr || address == nullptr || port == 0 || port > 65535 || tls.server != 0 ||
        tls.handshakeTimeoutMs < 100 || tls.handshakeTimeoutMs > 60000 || handshake.m_server != 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    uint32_t addressSize = 0;
    while (addressSize < sizeof(m_state->m_destination.m_address) && address[addressSize] != 0)
    {
        ++addressSize;
    }
    if (addressSize == 0 || addressSize == sizeof(m_state->m_destination.m_address))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetGatewayTransportFactory* selected = factory == nullptr ? m_state->m_providerFactory : factory;
    NetClientDiscovery* discovery = factory == nullptr ? m_state->m_providerDiscovery : nullptr;
    if (selected == nullptr || (factory == nullptr && discovery == nullptr &&
        (!m_state->m_seeds.empty() || !m_state->m_cachePath.empty())))
    {
        return NI_NOT_SUPPORTED;
    }
    NetInterfaceHandshakeOptions validation = handshake;
    validation.m_capabilities |= 24;
    if (NetInterface::entropyBytes(tls.entropy, tls.entropyContext, validation.m_nonce, sizeof(validation.m_nonce)) != NI_OK)
    {
        return NI_INVALID_STATE;
    }
    const uint64_t now = ClientSessionNow();
    NetInterfaceHandshake checked(validation, now);
    if (checked.progress(now) < 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t detached = m_state->m_session.bindChannel(nullptr);
    if (detached != NI_OK)
    {
        return detached;
    }
    releaseAttempt();
    m_state->m_external = nullptr;
    stopDiscovery();
    if (m_state->m_discovery != nullptr)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        if (discovery != nullptr)
        {
            std::vector<NetGatewayAddress> points = m_state->m_seeds;
            if (points.empty())
            {
                NetGatewayAddress point;
                point.m_address = address;
                point.m_port = port;
                points.push_back(point);
            }
            // 先登记借用，start中途失败仍能停止和推进其已接纳的异步资源。
            m_state->m_discovery = discovery;
            const int32_t started = discovery->start(points, m_state->m_cachePath, tls);
            if (started != NI_OK)
            {
                stopDiscovery();
                return started;
            }
        }
    }
    catch (...)
    {
        stopDiscovery();
        return NI_NO_MEMORY;
    }
    NetClientDiscoveryPoint destination = {};
    destination.m_structSize = sizeof(destination);
    destination.m_version = 1;
    std::memcpy(destination.m_address, address, addressSize);
    destination.m_port = port;
    m_state->m_destination = destination;
    m_state->m_factory = selected;
    m_state->m_listener = listener;
    m_state->m_tls = NetInterfaceTlsOptions();
    m_state->m_tls.entropy = tls.entropy;
    m_state->m_tls.entropyContext = tls.entropyContext;
    m_state->m_tls.handshakeTimeoutMs = tls.handshakeTimeoutMs;
    m_state->m_handshake = handshake;
    m_state->m_handshake.m_capabilities |= 24;
    m_state->m_product = product;
    m_state->m_service = true;
    m_state->m_retryAt = 0;
    m_state->m_delay = 250;
    m_state->m_recoveryDeadline = now + 30000;
    reserveGeneration();
    return NI_OK;
}

bool NetClientSession::reserveGeneration()
{
    if (m_state->m_generation == UINT64_MAX)
    {
        m_state->m_phase = NET_CLIENT_CLOSED;
        return false;
    }
    ++m_state->m_generation;
    m_state->m_phase = NET_CLIENT_PENDING;
    m_state->m_closeReason = 0;
    return true;
}

void NetClientSession::failAttempt(int32_t reason)
{
    close(reason);
    if (m_state->m_phase == NET_CLIENT_CANCELLING)
    {
        m_state->m_session.bindChannel(nullptr);
        releaseAttempt();
        onSessionClosed(m_state->m_generation, m_state->m_closeReason, 0);
    }
}

void NetClientSession::expireRecovery(uint64_t nowMs)
{
    if (!m_state->expired(nowMs))
    {
        return;
    }
    if (m_state->m_phase == NET_CLIENT_BACKOFF && !reserveGeneration())
    {
        return;
    }
    failAttempt(NI_TIMEOUT);
}

void NetClientSession::attempt()
{
    if (m_state->m_phase == NET_CLIENT_BACKOFF && !reserveGeneration())
    {
        return;
    }
    if (m_state->m_discovery != nullptr && !m_state->m_discovery->choose(m_state->m_destination))
    {
        m_state->m_retryAt = ClientSessionNow() + 250;
        return;
    }
    if (m_state->m_discovery != nullptr && !NetClientDiscovery::validPoint(m_state->m_destination))
    {
        failAttempt(NI_PROTOCOL_ERROR);
        return;
    }
    const int32_t detached = m_state->m_session.bindChannel(nullptr);
    if (detached != NI_OK)
    {
        failAttempt(detached);
        return;
    }
    releaseAttempt();
    m_state->m_transport.reset(m_state->m_factory->create());
    if (!m_state->m_transport)
    {
        failAttempt(NI_NO_MEMORY);
        return;
    }
    m_state->m_channel.reset(new NetGatewayClientChannel(m_state->m_product, m_state->m_transport.get()));
    if (m_state->m_connectionPools)
    {
        const int32_t configured = m_state->m_channel->configureConnectionPools(*m_state->m_connectionPools);
        if (configured != NI_OK)
        {
            failAttempt(configured);
            return;
        }
    }
    const int32_t bound = m_state->m_session.bindChannel(m_state->m_channel.get());
    if (bound != NI_OK)
    {
        failAttempt(bound);
        return;
    }
    expireRecovery(ClientSessionNow());
    if (m_state->m_phase != NET_CLIENT_PENDING)
    {
        return;
    }
    m_state->m_phase = NET_CLIENT_CONNECTING;
    const int32_t started = m_state->m_session.connect(m_state->m_destination.m_address,
        m_state->m_destination.m_port, m_state->m_tls, m_state->m_handshake, this);
    if (started != NI_OK)
    {
        onSessionClosed(m_state->m_generation, started, 0);
    }
}

bool NetClientSession::poll()
{
    if (executing())
    {
        return m_state->active() || m_state->m_discovery != nullptr;
    }
    m_state->m_polling = true;
    try
    {
        expireRecovery(ClientSessionNow());
        if (m_state->m_phase == NET_CLIENT_CANCELLING)
        {
            onSessionClosed(m_state->m_generation, m_state->m_closeReason, 0);
        }
        if (!m_state->recovering())
        {
            stopDiscovery();
        }
        if (m_state->m_discovery != nullptr)
        {
            m_state->m_discovery->poll();
            if (!m_state->recovering() && discoveryStopped())
            {
                m_state->m_discovery = nullptr;
            }
        }
        expireRecovery(ClientSessionNow());
        m_state->m_session.poll();
        expireRecovery(ClientSessionNow());
        if ((m_state->m_phase == NET_CLIENT_PENDING || m_state->m_phase == NET_CLIENT_BACKOFF) &&
            m_state->m_service && ClientSessionNow() >= m_state->m_retryAt)
        {
            attempt();
        }
    }
    catch (...)
    {
        failAttempt(NI_NO_MEMORY);
    }
    m_state->m_polling = false;
    return m_state->active() || !discoveryStopped();
}

void NetClientSession::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    (void)generation;
    expireRecovery(ClientSessionNow());
    if (m_state->m_phase != NET_CLIENT_CONNECTING)
    {
        return;
    }
    m_state->m_phase = NET_CLIENT_READY;
    m_state->m_recoveryDeadline = 0;
    m_state->m_delay = 250;
    const bool callback = m_state->m_callback;
    m_state->m_callback = true;
    try
    {
        m_state->m_listener->onSessionReady(m_state->m_generation, maxRequests, maxChunk);
    }
    catch (...)
    {
        close(NI_INVALID_STATE);
    }
    m_state->m_callback = callback;
}

void NetClientSession::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type,
    uint32_t status, const uint8_t* bytes, uint32_t size)
{
    (void)generation;
    const bool callback = m_state->m_callback;
    m_state->m_callback = true;
    try
    {
        if (m_state->m_listener != nullptr)
        {
            m_state->m_listener->onSessionResult(m_state->m_generation, requestId, product, type, status, bytes, size);
        }
    }
    catch (...)
    {
        close(NI_INVALID_STATE);
    }
    m_state->m_callback = callback;
}

bool NetClientSession::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    (void)generation;
    if (m_state->m_phase != NET_CLIENT_READY)
    {
        return false;
    }
    bool accepted = false;
    const bool callback = m_state->m_callback;
    m_state->m_callback = true;
    try
    {
        accepted = m_state->m_listener->onSessionFrame(m_state->m_generation, frame);
    }
    catch (...)
    {
        close(NI_INVALID_STATE);
    }
    m_state->m_callback = callback;
    return accepted;
}

void NetClientSession::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)generation;
    if (!m_state->active() || m_state->m_phase == NET_CLIENT_BACKOFF)
    {
        return;
    }
    const bool retry = m_state->recovering();
    const bool local = m_state->m_phase == NET_CLIENT_CANCELLING || m_state->m_phase == NET_CLIENT_STOPPING;
    m_state->m_phase = NET_CLIENT_CLOSED;
    if (local)
    {
        reason = m_state->m_closeReason;
        detail = 0;
    }
    const uint64_t now = ClientSessionNow();
    if (retry && reason != NI_INVALID_ARGUMENT && reason != NI_NOT_SUPPORTED && reason != NI_NO_MEMORY)
    {
        if (m_state->m_recoveryDeadline == 0)
        {
            m_state->m_recoveryDeadline = now + 30000;
        }
        if (now >= m_state->m_recoveryDeadline)
        {
            reason = NI_TIMEOUT;
            detail = 0;
        }
        else
        {
            uint32_t random = 0;
            if (NetInterface::entropyBytes(m_state->m_tls.entropy, m_state->m_tls.entropyContext,
                reinterpret_cast<uint8_t*>(&random), sizeof(random)) != NI_OK)
            {
                reason = NI_INVALID_STATE;
                detail = 0;
            }
            else
            {
                m_state->m_retryAt = now + m_state->m_delay / 2 + random % m_state->m_delay;
                m_state->m_delay = (std::min)(m_state->m_delay * 2, 5000U);
                m_state->m_phase = NET_CLIENT_BACKOFF;
                if (m_state->m_discovery != nullptr)
                {
                    try
                    {
                        m_state->m_discovery->failed(m_state->m_destination);
                    }
                    catch (...)
                    {
                        m_state->m_phase = NET_CLIENT_CLOSED;
                        reason = NI_NO_MEMORY;
                        detail = 0;
                    }
                }
            }
        }
    }
    const bool callback = m_state->m_callback;
    m_state->m_callback = true;
    try
    {
        if (m_state->m_listener != nullptr)
        {
            m_state->m_listener->onSessionClosed(m_state->m_generation, reason, detail);
        }
    }
    catch (...)
    {
        m_state->m_phase = NET_CLIENT_CLOSED;
    }
    m_state->m_callback = callback;
}

void NetClientSession::close(int32_t reason)
{
    if (m_state->m_phase == NET_CLIENT_PENDING)
    {
        m_state->m_phase = NET_CLIENT_CANCELLING;
        m_state->m_closeReason = reason;
    }
    else if (m_state->m_phase == NET_CLIENT_BACKOFF)
    {
        m_state->m_phase = NET_CLIENT_CLOSED;
    }
    else if (m_state->m_phase == NET_CLIENT_CONNECTING || m_state->m_phase == NET_CLIENT_READY)
    {
        m_state->m_phase = NET_CLIENT_STOPPING;
        m_state->m_closeReason = reason;
        m_state->m_session.close(reason);
    }
}

void NetClientSession::shutdown()
{
    if (executing())
    {
        close();
        return;
    }
    close();
    m_state->m_session.shutdown();
    if (m_state->m_phase == NET_CLIENT_CANCELLING || m_state->m_phase == NET_CLIENT_STOPPING)
    {
        onSessionClosed(m_state->m_generation, m_state->m_closeReason, 0);
    }
    m_state->m_session.bindChannel(nullptr);
    releaseAttempt();
    stopDiscovery();
    m_state->m_external = nullptr;
    m_state->m_factory = nullptr;
    m_state->m_listener = nullptr;
    m_state->m_service = false;
}

bool NetClientSession::ready() const
{
    return m_state->m_phase == NET_CLIENT_READY && m_state->m_session.ready();
}

bool NetClientSession::executing() const
{
    return m_state->m_polling || m_state->m_callback;
}

bool NetClientSession::quiescent() const
{
    return !executing() && !m_state->active() && discoveryStopped();
}

uint64_t NetClientSession::generation() const
{
    return m_state->m_generation;
}

bool NetClientSession::serviceMode() const
{
    return m_state->m_service;
}

bool NetClientSession::hasBoundChannel() const
{
    return m_state->m_external != nullptr;
}

NetGatewayTransportFactory* NetClientSession::boundGatewayFactory() const
{
    return m_state->m_providerFactory;
}

void NetClientSession::detachBindingsForDestruction()
{
    shutdown();
    m_state->m_discovery = nullptr;
    m_state->m_providerDiscovery = nullptr;
    m_state->m_providerFactory = nullptr;
}

int32_t NetClientSession::beginRequest(uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId)
{
    requestId = 0;
    return m_state->m_service && product != m_state->m_product ? NI_INVALID_ARGUMENT :
        m_state->m_session.beginRequest(product, type, timeoutMs, requestId);
}

int32_t NetClientSession::beginRequestAt(uint64_t requestId, uint32_t product, uint32_t type, uint64_t startedMs, uint32_t timeoutMs)
{
    return m_state->m_service && product != m_state->m_product ? NI_INVALID_ARGUMENT :
        m_state->m_session.beginRequestAt(requestId, product, type, startedMs, timeoutMs);
}

int32_t NetClientSession::sendRequest(uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    return m_state->m_session.sendRequest(requestId, bytes, size);
}

int32_t NetClientSession::cancel(uint64_t requestId)
{
    return m_state->m_session.cancel(requestId);
}

int32_t NetClientSession::sendData(const NetInterfaceFrame& frame)
{
    return m_state->m_service && frame.m_productId != m_state->m_product ? NI_INVALID_ARGUMENT :
        m_state->m_session.sendData(frame);
}

int32_t NetClientSession::outputState() const
{
    return m_state->m_session.outputState();
}