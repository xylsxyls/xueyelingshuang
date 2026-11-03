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

int32_t NetClientSession::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls,
    const NetInterfaceHandshakeOptions& handshake, NetSessionListener* listener)
{
    if (!quiescent() || m_state->m_generation == UINT64_MAX)
    {
        return NI_INVALID_STATE;
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
    validation.m_capabilities |= 8;
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
    m_state->m_handshake.m_capabilities |= 8;
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

// NET_CLIENT_RECOVERY_UNIT_BEGIN
//#include "NetClientSessionTest.h"
//#ifdef STATIC_LIB
//#include "NetClientSessionState.h"
//#endif
//#include <cstdio>
//#include <cstring>
//#include <chrono>
//#if defined(_WIN32) && defined(_DEBUG)
//#include <crtdbg.h>
//#endif
//
//NetClientSessionTestTransport::NetClientSessionTestTransport(NetClientSessionTestCounters& counters) :
//m_counters(&counters)
//{
//
//}
//
//NetClientSessionTestTransport::~NetClientSessionTestTransport()
//{
//    ++m_counters->m_destroyed;
//}
//
//int32_t NetClientSessionTestTransport::connect(const char* address, uint32_t port,
//    const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener, uint64_t& peer)
//{
//    (void)address;
//    (void)port;
//    (void)options;
//    (void)listener;
//    ++m_counters->m_connected;
//    peer = 0;
//    return m_counters->m_connectResult;
//}
//
//bool NetClientSessionTestTransport::poll()
//{
//    return false;
//}
//
//int32_t NetClientSessionTestTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
//{
//    (void)peer;
//    (void)frame;
//    return NI_INVALID_STATE;
//}
//
//int32_t NetClientSessionTestTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
//{
//    (void)peer;
//    (void)record;
//    return NI_INVALID_STATE;
//}
//
//void NetClientSessionTestTransport::shutdown()
//{
//    ++m_counters->m_stopped;
//}
//
//NetClientSessionTestFactory::NetClientSessionTestFactory(NetClientSessionTestCounters& counters) :
//m_counters(&counters)
//{
//
//}
//
//NetGatewayTransport* NetClientSessionTestFactory::create()
//{
//    ++m_counters->m_created;
//    if (m_counters->m_factoryMode == 1)
//    {
//        return nullptr;
//    }
//    if (m_counters->m_factoryMode == 2)
//    {
//        throw 1;
//    }
//    return new NetClientSessionTestTransport(*m_counters);
//}
//
//NetClientSessionTestListener::NetClientSessionTestListener() :
//m_other(0),
//m_nestedSession(nullptr),
//m_nestedRequest(0),
//m_nestedExecuting(false),
//m_nestedCancelOk(false),
//m_reenterClosed(false),
//m_throwClosed(false),
//m_throwResult(false)
//{
//
//}
//
//void NetClientSessionTestListener::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
//{
//    (void)generation;
//    (void)maxRequests;
//    (void)maxChunk;
//    ++m_other;
//}
//
//void NetClientSessionTestListener::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
//    uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
//{
//    (void)generation;
//    (void)requestId;
//    (void)product;
//    (void)type;
//    (void)status;
//    (void)bytes;
//    (void)size;
//    ++m_other;
//    m_resultIds.push_back(requestId);
//    m_resultStatus.push_back(status);
//    m_events.push_back(requestId);
//    if (m_throwResult)
//    {
//        throw 1;
//    }
//    if (m_nestedSession != nullptr && m_nestedRequest != 0)
//    {
//        const uint64_t nested = m_nestedRequest;
//        m_nestedRequest = 0;
//        m_nestedCancelOk = m_nestedSession->cancel(nested) == NI_OK;
//        m_nestedExecuting = m_nestedSession->executing();
//    }
//}
//
//bool NetClientSessionTestListener::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
//{
//    (void)generation;
//    (void)frame;
//    ++m_other;
//    return false;
//}
//
//void NetClientSessionTestListener::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
//{
//    (void)detail;
//    m_events.push_back(UINT64_MAX);
//    m_generations.push_back(generation);
//    m_reasons.push_back(reason);
//    if (m_reenterClosed && m_nestedSession != nullptr)
//    {
//        m_nestedExecuting = m_nestedSession->executing();
//        m_nestedSession->close(NI_TIMEOUT);
//        m_nestedSession->shutdown();
//        m_nestedSession->poll();
//        m_nestedCancelOk = m_nestedSession->bindGateway(nullptr) == NI_INVALID_STATE;
//    }
//    if (m_throwClosed)
//    {
//        throw 1;
//    }
//}
//
//int32_t NetClientSessionTestListener::entropy(void* context, uint8_t* bytes, uint32_t size)
//{
//    (void)context;
//    std::memset(bytes, 0x41, size);
//    return 0;
//}
//
//NetClientSessionTestDiscovery::NetClientSessionTestDiscovery() :
//m_available(true),
//m_invalidPoint(0),
//m_throw(false),
//m_drain(false),
//m_shutdownStarted(false),
//m_remaining(0),
//m_fault(0)
//{
//
//}
//
//int32_t NetClientSessionTestDiscovery::start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath,
//    const NetInterfaceTlsOptions& tls)
//{
//    (void)seeds;
//    (void)cachePath;
//    (void)tls;
//    m_shutdownStarted = false;
//    m_remaining = m_drain ? 2 : 0;
//    if ((m_fault & 1) != 0)
//    {
//        m_fault &= ~1U;
//        throw 1;
//    }
//    return (m_fault & 32) != 0 ? NI_INVALID_ARGUMENT : NI_OK;
//}
//
//bool NetClientSessionTestDiscovery::poll()
//{
//    if ((m_fault & 4) != 0)
//    {
//        m_fault &= ~4U;
//        throw 1;
//    }
//    if (m_shutdownStarted && m_remaining != 0)
//    {
//        --m_remaining;
//    }
//    return m_remaining != 0;
//}
//
//void NetClientSessionTestDiscovery::shutdown()
//{
//    m_shutdownStarted = true;
//    if ((m_fault & 2) != 0)
//    {
//        m_fault &= ~2U;
//        throw 1;
//    }
//}
//
//bool NetClientSessionTestDiscovery::stopped() const
//{
//    if ((m_fault & 16) != 0)
//    {
//        throw 1;
//    }
//    return m_remaining == 0;
//}
//
//bool NetClientSessionTestDiscovery::choose(NetClientDiscoveryPoint& point)
//{
//    if ((m_fault & 8) != 0)
//    {
//        m_fault &= ~8U;
//        throw 1;
//    }
//    if (m_invalidPoint == 1)
//    {
//        point.m_structSize = 0;
//    }
//    else if (m_invalidPoint == 2)
//    {
//        point.m_version = 2;
//    }
//    else if (m_invalidPoint == 3)
//    {
//        std::memset(point.m_address, '1', sizeof(point.m_address));
//    }
//    else if (m_invalidPoint == 4)
//    {
//        point.m_port = 0;
//    }
//    return m_available;
//}
//
///** 与真实会话相同的单调时间，仅供已验证帧的握手Mock
//@return 单调毫秒
//*/
//static uint64_t RecoveryTestNow()
//{
//    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
//        std::chrono::steady_clock::now().time_since_epoch()).count());
//}
//
//NetClientSessionTestChannel::NetClientSessionTestChannel(const NetInterfaceHandshakeOptions& options) :
//m_fault(0),
//m_sendCount(0),
//m_observed(nullptr),
//m_observedSize(0),
//m_handshake(new NetInterfaceHandshake(options, RecoveryTestNow())),
//m_listener(nullptr),
//m_announced(false),
//m_closing(false),
//m_reason(0),
//m_queued(false),
//m_queuedFrame()
//{
//
//}
//
//NetClientSessionTestChannel::~NetClientSessionTestChannel()
//{
//    shutdown();
//}
//
//int32_t NetClientSessionTestChannel::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
//    NetTransportListener* listener, uint64_t& peer)
//{
//    (void)address;
//    (void)port;
//    (void)options;
//    peer = 0;
//    if (m_listener != nullptr || listener == nullptr)
//    {
//        return NI_INVALID_STATE;
//    }
//    m_listener = listener;
//    if ((m_fault & 256) != 0)
//    {
//        throw 1;
//    }
//    peer = (m_fault & 128) != 0 ? 0 : 1;
//    return (m_fault & 512) != 0 ? NI_NOT_FOUND : NI_OK;
//}
//
//bool NetClientSessionTestChannel::poll()
//{
//    if ((m_fault & 64) != 0)
//    {
//        m_fault &= ~64U;
//        throw 1;
//    }
//    if (m_closing)
//    {
//        shutdown();
//        return false;
//    }
//    if (m_listener == nullptr)
//    {
//        return false;
//    }
//    if (!m_announced)
//    {
//        m_announced = true;
//        m_listener->onReady(1);
//    }
//    NetInterfaceFrame frame = {};
//    if (!m_closing && m_handshake->peekOutput(frame) == NI_READY && m_listener->onFrame(1, frame))
//    {
//        m_handshake->consumeOutput();
//    }
//    if (m_queued && !m_closing && m_listener->onFrame(1, m_queuedFrame))
//    {
//        m_queued = false;
//        m_queuedBody.clear();
//    }
//    return true;
//}
//
//int32_t NetClientSessionTestChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
//{
//    if (peer != 1 || m_listener == nullptr || m_closing)
//    {
//        return NI_INVALID_STATE;
//    }
//    ++m_sendCount;
//    if (frame.m_kind == NI_REQUEST)
//    {
//        m_observed = frame.m_payload;
//        m_observedSize = frame.m_payloadSize;
//        if ((m_fault & 1) != 0)
//        {
//            throw 1;
//        }
//        return (m_fault & 1024) != 0 ? NI_LIMIT_EXCEEDED : NI_OK;
//    }
//    if (frame.m_kind == NI_CANCEL || frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK)
//    {
//        if ((m_fault & (frame.m_kind == NI_CANCEL ? 2U : 4U)) != 0)
//        {
//            throw 1;
//        }
//        return NI_OK;
//    }
//    const int32_t accepted = m_handshake->accept(frame, RecoveryTestNow());
//    return accepted < 0 ? accepted : NI_OK;
//}
//
//void NetClientSessionTestChannel::close(uint64_t peer, int32_t reason)
//{
//    if (peer == 1 && !m_closing)
//    {
//        m_closing = true;
//        m_reason = reason;
//    }
//    if ((m_fault & 32) != 0)
//    {
//        throw 1;
//    }
//}
//
//void NetClientSessionTestChannel::shutdown()
//{
//    m_closing = true;
//    NetTransportListener* listener = m_listener;
//    m_listener = nullptr;
//    if (listener != nullptr)
//    {
//        listener->onClosed(1, m_reason, 0);
//    }
//}
//
//
//int32_t NetClientSessionTestChannel::configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated)
//{
//    (void)peer;
//    (void)types;
//    (void)count;
//    (void)negotiated;
//    if ((m_fault & 8) != 0)
//    {
//        throw 1;
//    }
//    return NI_OK;
//}
//
//int32_t NetClientSessionTestChannel::outputState(uint64_t peer) const
//{
//    (void)peer;
//    if ((m_fault & 16) != 0)
//    {
//        throw 1;
//    }
//    return NI_OK;
//}
//
//void NetClientSessionTestChannel::queueFrame(const NetInterfaceFrame& frame)
//{
//    m_queuedBody.clear();
//    if (frame.m_payloadSize != 0)
//    {
//        m_queuedBody.assign(frame.m_payload, frame.m_payload + frame.m_payloadSize);
//    }
//    m_queuedFrame = frame;
//    m_queuedFrame.m_payload = m_queuedBody.empty() ? nullptr : m_queuedBody.data();
//    m_queued = true;
//}
//
//void NetClientSessionTestDiscovery::failed(const NetClientDiscoveryPoint& point)
//{
//    (void)point;
//    if (m_throw)
//    {
//        throw 1;
//    }
//}
//
//int main()
//{
//    uint32_t assertions = 0;
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState before;
//    _CrtMemCheckpoint(&before);
//#endif
//#define RECOVERY_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
//
//    {
//#ifdef STATIC_LIB
//        NetClientSessionState state;
//        const NetClientSessionPhase phases[] = { NET_CLIENT_IDLE, NET_CLIENT_PENDING, NET_CLIENT_CONNECTING,
//            NET_CLIENT_READY, NET_CLIENT_BACKOFF, NET_CLIENT_CANCELLING, NET_CLIENT_STOPPING, NET_CLIENT_CLOSED };
//        for (uint32_t index = 0; index < 8; ++index)
//        {
//            state.m_phase = phases[index];
//            state.m_service = true;
//            state.m_recoveryDeadline = 30000;
//            const bool recovering = index >= 1 && index <= 4;
//            RECOVERY_CHECK(state.recovering() == recovering && !state.expired(29999));
//            RECOVERY_CHECK(state.expired(30000) == recovering && state.expired(30001) == recovering);
//            RECOVERY_CHECK(state.active() == (index != 0 && index != 7));
//            state.m_recoveryDeadline = 0;
//            RECOVERY_CHECK(!state.expired(UINT64_MAX));
//            state.m_service = false;
//            state.m_recoveryDeadline = 1;
//            RECOVERY_CHECK(!state.recovering() && !state.expired(UINT64_MAX));
//        }
//#endif
//        NetClientDiscoveryPoint points[64] = {};
//        for (auto& point : points)
//        {
//            point.m_structSize = sizeof(point);
//            point.m_version = 1;
//        }
//        uint32_t pointCount = 0;
//        RECOVERY_CHECK(NetClientDiscovery::parseSeeds("127.0.0.1:1,[::1]:65535,127.0.0.1:1", points, 64, pointCount) == NI_OK);
//        RECOVERY_CHECK(pointCount == 2 && points[0].m_port == 1 && points[1].m_port == 65535);
//        RECOVERY_CHECK(NetClientDiscovery::parseSeeds("127.0.0.1:1", nullptr, 0, pointCount) == NI_BUFFER_TOO_SMALL && pointCount == 1);
//        const NetClientDiscoveryPoint original = points[0];
//        RECOVERY_CHECK(NetClientDiscovery::parseSeeds("127.0.0.1:2", points, 0, pointCount) == NI_BUFFER_TOO_SMALL && points[0].m_port == original.m_port);
//        points[0].m_version = 2;
//        RECOVERY_CHECK(NetClientDiscovery::parseSeeds("127.0.0.1:1", points, 64, pointCount) == NI_INVALID_ARGUMENT && pointCount == 0);
//        points[0] = original;
//        RECOVERY_CHECK(NetClientDiscovery::validPoint(points[0]));
//        const char* invalid[] = { "", ",", "127.0.0.1:1,", "localhost:1", "127.0.0.1:0", "127.0.0.1:65536",
//            "[::1]1", "[::1]:", "::1:1", "127.0.0.1:1x", "127.0.0.1:1,,127.0.0.1:2" };
//        for (uint32_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index)
//        {
//            RECOVERY_CHECK(NetClientDiscovery::parseSeeds(invalid[index], points, 64, pointCount) == NI_INVALID_ARGUMENT && pointCount == 0);
//        }
//        std::string seeds;
//        for (uint32_t index = 0; index < 64; ++index)
//        {
//            seeds += index == 0 ? "127.0.0.1:1" : ",127.0.0.1:1";
//        }
//        RECOVERY_CHECK(NetClientDiscovery::parseSeeds(seeds, points, 64, pointCount) == NI_OK && pointCount == 1);
//        RECOVERY_CHECK(NetClientDiscovery::parseSeeds(seeds + ",127.0.0.1:1", points, 64, pointCount) == NI_INVALID_ARGUMENT && pointCount == 0);
//        NetInterfaceTlsOptions tls = {};
//        tls.handshakeTimeoutMs = 1000;
//        tls.entropy = NetClientSessionTestListener::entropy;
//        NetInterfaceHandshakeOptions hello = {};
//        hello.m_platform = 1;
//        hello.m_capabilities = 7;
//        hello.m_maxRequests = 4;
//        hello.m_maxChunk = 4096;
//        hello.m_timeoutMs = 1000;
//        std::memset(hello.m_device, 0x31, sizeof(hello.m_device));
//        NetClientSessionTestCounters counters = {};
//        counters.m_connectResult = NI_INVALID_ARGUMENT;
//        NetClientSessionTestFactory factory(counters);
//        NetClientSessionTestDiscovery discovery;
//        NetClientSessionTestListener listener;
//        NetClientSession session;
//        RECOVERY_CHECK(session.generation() == 0);
//        const std::string oversizedAddress(40, '1');
//        RECOVERY_CHECK(session.connectService(101, oversizedAddress.c_str(), 1, tls, hello, &listener) == NI_INVALID_ARGUMENT && session.generation() == 0);
//        RECOVERY_CHECK(session.connectService(101, "127.0.0.1", 1, tls, hello, &listener) == NI_NOT_SUPPORTED);
//        RECOVERY_CHECK(session.generation() == 0 && listener.m_generations.empty());
//        RECOVERY_CHECK(session.bindGateway(nullptr, &discovery) == NI_INVALID_ARGUMENT);
//        RECOVERY_CHECK(session.bindGateway(&factory) == NI_OK);
//        RECOVERY_CHECK(session.connectService(101, "127.0.0.1", 1, tls, hello, &listener) == NI_OK);
//        RECOVERY_CHECK(session.generation() == 1 && listener.m_generations.empty() && counters.m_created == 0);
//        session.close(NI_CANCELLED);
//        session.close(NI_TIMEOUT);
//        RECOVERY_CHECK(session.connectService(101, "127.0.0.1", 1, tls, hello, &listener) == NI_INVALID_STATE);
//        RECOVERY_CHECK(!session.poll());
//        RECOVERY_CHECK(listener.m_generations.size() == 1 && listener.m_generations[0] == 1 && listener.m_reasons[0] == NI_CANCELLED);
//        RECOVERY_CHECK(counters.m_created == 0 && !session.poll());
//        session.shutdown();
//        for (uint32_t mode = 0; mode < 3; ++mode)
//        {
//            counters.m_factoryMode = mode;
//            RECOVERY_CHECK(session.connectService(101, "127.0.0.1", 1, tls, hello, &listener) == NI_OK);
//            RECOVERY_CHECK(!session.poll());
//            RECOVERY_CHECK(listener.m_generations.size() == mode + 2);
//            RECOVERY_CHECK(listener.m_generations.back() == mode + 2 && session.generation() == mode + 2);
//            RECOVERY_CHECK(listener.m_reasons.back() == (mode == 0 ? NI_INVALID_ARGUMENT : NI_NO_MEMORY));
//            session.close();
//            session.shutdown();
//            RECOVERY_CHECK(listener.m_generations.size() == mode + 2);
//        }
//        RECOVERY_CHECK(counters.m_created == 3 && counters.m_destroyed == 1 && counters.m_connected == 1);
//        RECOVERY_CHECK(session.bindGateway(&factory, &discovery) == NI_OK);
//        counters.m_factoryMode = 0;
//        discovery.m_available = false;
//        RECOVERY_CHECK(session.connectService(102, "127.0.0.1", 1, tls, hello, &listener) == NI_OK);
//        RECOVERY_CHECK(session.poll() && counters.m_created == 3);
//        session.close(NI_TIMEOUT);
//        RECOVERY_CHECK(!session.poll());
//        RECOVERY_CHECK(listener.m_generations.size() == 5 && listener.m_reasons.back() == NI_TIMEOUT && listener.m_generations.back() == 5);
//        session.shutdown();
//        discovery.m_available = true;
//        discovery.m_throw = true;
//        counters.m_connectResult = NI_INVALID_STATE;
//        RECOVERY_CHECK(session.connectService(103, "127.0.0.1", 1, tls, hello, &listener) == NI_OK);
//        RECOVERY_CHECK(!session.poll());
//        RECOVERY_CHECK(listener.m_generations.size() == 6 && listener.m_reasons.back() == NI_NO_MEMORY && listener.m_generations.back() == 6);
//        session.shutdown();
//        discovery.m_throw = false;
//        RECOVERY_CHECK(session.connectService(103, "127.0.0.1", 1, tls, hello, &listener) == NI_OK);
//        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
//        while (listener.m_generations.size() < 8 && std::chrono::steady_clock::now() < deadline)
//        {
//            session.poll();
//        }
//        RECOVERY_CHECK(listener.m_generations.size() == 8);
//        RECOVERY_CHECK(listener.m_generations[6] == 7 && listener.m_generations[7] == 8);
//        session.close();
//        session.shutdown();
//        RECOVERY_CHECK(listener.m_generations.size() == 8 && listener.m_other == 0);
//        RECOVERY_CHECK(counters.m_created == 6 && counters.m_destroyed == 4 && counters.m_connected == 4);
//        RECOVERY_CHECK(!session.poll());
//        RECOVERY_CHECK(session.connectService(101, "127.0.0.1", 1, tls, hello, &listener) == NI_OK);
//        session.shutdown();
//        RECOVERY_CHECK(listener.m_generations.size() == 9 && listener.m_generations.back() == 9 && listener.m_reasons.back() == 0);
//        RECOVERY_CHECK(session.generation() == 9 && counters.m_created == 6);
//        discovery.m_drain = true;
//        RECOVERY_CHECK(session.connectService(101, "127.0.0.1", 1, tls, hello, &listener) == NI_OK);
//        session.close(NI_CANCELLED);
//        RECOVERY_CHECK(session.poll());
//        RECOVERY_CHECK(listener.m_generations.size() == 10 && listener.m_generations.back() == 10);
//        RECOVERY_CHECK(!session.quiescent());
//        RECOVERY_CHECK(session.bindGateway(nullptr) == NI_INVALID_STATE);
//        RECOVERY_CHECK(!session.poll() && session.quiescent());
//        RECOVERY_CHECK(session.bindGateway(nullptr) == NI_OK && listener.m_generations.size() == 10);
//        // 外部主动cancel会同步交付结果；内层cancel返回后仍处于外层回调，不能允许释放
//        NetInterfaceHandshakeOptions serverHello = hello;
//        serverHello.m_server = 1;
//        serverHello.m_platform = 0;
//        std::memset(serverHello.m_device, 0, sizeof(serverHello.m_device));
//        std::memset(serverHello.m_nonce, 0x73, sizeof(serverHello.m_nonce));
//        NetClientSessionTestChannel callbackChannel(serverHello);
//        NetClientSessionTestListener nestedListener;
//        NetClientSession nestedSession;
//        nestedListener.m_nestedSession = &nestedSession;
//        RECOVERY_CHECK(nestedSession.bindChannel(&callbackChannel) == NI_OK);
//        RECOVERY_CHECK(nestedSession.connect("mock", 1, tls, hello, &nestedListener) == NI_OK);
//        for (uint32_t loop = 0; loop < 8 && !nestedSession.ready(); ++loop)
//        {
//            nestedSession.poll();
//        }
//        RECOVERY_CHECK(nestedSession.ready() && nestedListener.m_other == 1);
//        uint64_t firstRequest = 0;
//        uint64_t secondRequest = 0;
//        RECOVERY_CHECK(nestedSession.beginRequest(101, 11, 1000, firstRequest) == NI_OK);
//        RECOVERY_CHECK(nestedSession.beginRequest(101, 12, 1000, secondRequest) == NI_OK && firstRequest != secondRequest);
//        nestedListener.m_nestedRequest = secondRequest;
//        RECOVERY_CHECK(nestedSession.cancel(firstRequest) == NI_OK);
//        RECOVERY_CHECK(nestedListener.m_nestedCancelOk && nestedListener.m_other == 3);
//        RECOVERY_CHECK(nestedListener.m_nestedExecuting && !nestedSession.executing());
//        nestedSession.shutdown();
//        RECOVERY_CHECK(nestedSession.quiescent() && nestedListener.m_generations.size() == 1);
//        for (uint32_t mode = 1; mode <= 4; ++mode)
//        {
//            NetClientSessionTestCounters invalidCounters = {};
//            NetClientSessionTestFactory invalidFactory(invalidCounters);
//            NetClientSessionTestDiscovery invalidDiscovery;
//            invalidDiscovery.m_invalidPoint = mode;
//            NetClientSessionTestListener invalidListener;
//            NetClientSession invalidSession;
//            RECOVERY_CHECK(invalidSession.bindGateway(&invalidFactory, &invalidDiscovery) == NI_OK);
//            RECOVERY_CHECK(invalidSession.connectService(101, "127.0.0.1", 1, tls, hello, &invalidListener) == NI_OK);
//            RECOVERY_CHECK(!invalidSession.poll());
//            RECOVERY_CHECK(invalidCounters.m_created == 0 && invalidListener.m_generations.size() == 1 &&
//                invalidListener.m_reasons[0] == NI_PROTOCOL_ERROR);
//            invalidSession.shutdown();
//            RECOVERY_CHECK(invalidSession.quiescent());
//        }
//
//        const uint32_t startFaults[] = { 1, 3, 32, 34 };
//        for (uint32_t index = 0; index < 4; ++index)
//        {
//            NetClientSessionTestCounters c = {};
//            NetClientSessionTestFactory f(c);
//            NetClientSessionTestDiscovery d;
//            NetClientSessionTestListener l;
//            NetClientSession s;
//            d.m_fault = startFaults[index];
//            d.m_drain = true;
//            RECOVERY_CHECK(s.bindGateway(&f, &d) == NI_OK);
//            RECOVERY_CHECK(s.connectService(101, "127.0.0.1", 1, tls, hello, &l) ==
//                (index < 2 ? NI_NO_MEMORY : NI_INVALID_ARGUMENT));
//            RECOVERY_CHECK(s.generation() == 0 && l.m_generations.empty() && c.m_created == 0);
//            RECOVERY_CHECK(!s.quiescent() && s.bindGateway(nullptr) == NI_INVALID_STATE);
//            RECOVERY_CHECK(s.poll() && !s.poll() && s.quiescent());
//            RECOVERY_CHECK(s.bindGateway(nullptr) == NI_OK && l.m_generations.empty());
//        }
//        const uint32_t activeFaults[] = { 4, 8, 2, 16 };
//        for (uint32_t index = 0; index < 4; ++index)
//        {
//            NetClientSessionTestCounters c = {};
//            NetClientSessionTestFactory f(c);
//            NetClientSessionTestDiscovery d;
//            NetClientSessionTestListener l;
//            NetClientSession s;
//            d.m_drain = true;
//            l.m_nestedSession = &s;
//            l.m_reenterClosed = true;
//            l.m_throwClosed = index == 1;
//            RECOVERY_CHECK(s.bindGateway(&f, &d) == NI_OK);
//            RECOVERY_CHECK(s.connectService(101, "127.0.0.1", 1, tls, hello, &l) == NI_OK);
//            d.m_fault = activeFaults[index];
//            if (index >= 2)
//            {
//                s.close(NI_CANCELLED);
//                s.close(NI_TIMEOUT);
//            }
//            RECOVERY_CHECK(s.poll() && l.m_generations.size() == 1 && l.m_generations[0] == 1 &&
//                l.m_reasons[0] == (index < 2 ? NI_NO_MEMORY : NI_CANCELLED));
//            for (uint32_t loop = 0; loop < 8; ++loop)
//            {
//                s.poll();
//                RECOVERY_CHECK(!s.executing() && l.m_generations.size() == 1);
//            }
//            RECOVERY_CHECK(l.m_nestedExecuting && l.m_nestedCancelOk && c.m_created == 0);
//            if (index == 3)
//            {
//                RECOVERY_CHECK(!s.quiescent() && s.bindGateway(nullptr) == NI_INVALID_STATE);
//                d.m_fault = 0;
//            }
//            RECOVERY_CHECK(!s.poll() && s.quiescent());
//            s.shutdown();
//            RECOVERY_CHECK(l.m_generations.size() == 1 && l.m_other == 0 && s.bindGateway(nullptr) == NI_OK);
//        }
//        NetClientSessionTestCounters fixedCounters = {};
//        NetClientSessionTestFactory fixedFactory(fixedCounters);
//        NetClientSessionTestDiscovery unusedDiscovery;
//        unusedDiscovery.m_fault = 1;
//        NetClientSessionTestListener fixedListener;
//        NetClientSession fixedSession;
//        RECOVERY_CHECK(fixedSession.bindGateway(&fixedFactory, &unusedDiscovery) == NI_OK);
//        RECOVERY_CHECK(fixedSession.configureGateways("127.0.0.1:1234", "") == NI_OK);
//        RECOVERY_CHECK(fixedSession.connectService(101, "local-ipc", 1, tls, hello, &fixedListener, &fixedFactory) == NI_OK);
//        NetInterfaceFrame wrongProduct = {};
//        wrongProduct.m_productId = 102;
//        RECOVERY_CHECK(fixedSession.sendData(wrongProduct) == NI_INVALID_ARGUMENT && unusedDiscovery.m_fault == 1);
//        fixedSession.close(NI_CANCELLED);
//        RECOVERY_CHECK(!fixedSession.poll() && fixedListener.m_generations.size() == 1);
//    }
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState after;
//    _CrtMemCheckpoint(&after);
//    RECOVERY_CHECK(after.lCounts[_NORMAL_BLOCK] == before.lCounts[_NORMAL_BLOCK] &&
//        after.lSizes[_NORMAL_BLOCK] == before.lSizes[_NORMAL_BLOCK]);
//#endif
//    std::printf("Client recovery lifecycle checks=%u PASS\n", assertions);
//    return 0;
//#undef RECOVERY_CHECK
//}
// NET_CLIENT_RECOVERY_UNIT_END