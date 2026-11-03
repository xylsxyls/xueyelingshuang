#include "NetInterfaceConnectionTestCasesHelper.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <memory>
#include "NetInterfaceConnectionTestCases.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetPeer/NetPeerAPI.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetWorkTestCases.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <algorithm>
#include <cstring>
#include <stdexcept>

NetGatewayChannelTestTransport::NetGatewayChannelTestTransport() :
m_listener(nullptr), m_connectResult(NI_OK), m_zeroPeer(false), m_busy(false), m_pollThrow(false),
m_stopThrow(false), m_closeDuringStop(false), m_announced(false), m_stops(0), m_controls(0), m_bytes(0), m_controlSize(0)
{

}

int32_t NetGatewayChannelTestTransport::connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
    NetLinkTransportListener* listener, uint64_t& peer)
{
    (void)address;
    (void)port;
    (void)options;
    m_listener = listener;
    peer = m_zeroPeer ? 0 : 7;
    return m_connectResult;
}

bool NetGatewayChannelTestTransport::poll()
{
    if (m_pollThrow)
    {
        m_pollThrow = false;
        throw 1;
    }
    if (!m_announced && m_listener != nullptr)
    {
        m_announced = true;
        NetInterfaceLinkPeer identity = {};
        identity.m_role = NI_LINK_NODE;
        m_listener->onLinkReady(7, identity);
    }
    return m_listener != nullptr;
}

int32_t NetGatewayChannelTestTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != 7 || frame.m_payload == nullptr || frame.m_payloadSize > sizeof(m_control))
    {
        return NI_PROTOCOL_ERROR;
    }
    ++m_controls;
    m_controlSize = frame.m_payloadSize;
    std::memcpy(m_control, frame.m_payload, frame.m_payloadSize);
    return m_busy ? NI_LIMIT_EXCEEDED : NI_OK;
}

int32_t NetGatewayChannelTestTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (peer != 7 || record.m_channel != 1 || record.m_size == 0 || record.m_size > 16384 || record.m_data == nullptr)
    {
        return NI_PROTOCOL_ERROR;
    }
    m_bytes += record.m_size;
    return NI_OK;
}

void NetGatewayChannelTestTransport::shutdown()
{
    ++m_stops;
    NetLinkTransportListener* listener = m_listener;
    m_listener = nullptr;
    if (m_closeDuringStop && listener != nullptr)
    {
        listener->onLinkClosed(7, NI_TIMEOUT);
    }
    if (m_stopThrow)
    {
        m_stopThrow = false;
        throw 2;
    }
}

NetGatewayChannelTestListener::NetGatewayChannelTestListener() :
m_channel(nullptr), m_closed(0), m_other(0), m_reason(0), m_throw(false)
{

}

void NetGatewayChannelTestListener::onReady(uint64_t peer)
{
    (void)peer;
    ++m_other;
}

bool NetGatewayChannelTestListener::onFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    (void)peer;
    (void)frame;
    ++m_other;
    return true;
}

void NetGatewayChannelTestListener::onClosed(uint64_t peer, int32_t reason, int32_t detail)
{
    (void)peer;
    (void)detail;
    ++m_closed;
    m_reason = reason;
    if (m_channel != nullptr)
    {
        m_channel->close(1, NI_TIMEOUT);
        m_channel->shutdown();
        m_channel->poll();
    }
    if (m_throw)
    {
        throw 3;
    }
}

NetClientSessionTestTransport::NetClientSessionTestTransport(NetClientSessionTestCounters& counters) :
m_counters(&counters)
{

}

NetClientSessionTestTransport::~NetClientSessionTestTransport()
{
    ++m_counters->m_destroyed;
}

int32_t NetClientSessionTestTransport::connect(const char* address, uint32_t port,
    const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener, uint64_t& peer)
{
    (void)address;
    (void)port;
    (void)options;
    (void)listener;
    ++m_counters->m_connected;
    peer = 0;
    return m_counters->m_connectResult;
}

bool NetClientSessionTestTransport::poll()
{
    return false;
}

int32_t NetClientSessionTestTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    (void)peer;
    (void)frame;
    return NI_INVALID_STATE;
}

int32_t NetClientSessionTestTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    (void)peer;
    (void)record;
    return NI_INVALID_STATE;
}

void NetClientSessionTestTransport::shutdown()
{
    ++m_counters->m_stopped;
}

NetClientSessionTestFactory::NetClientSessionTestFactory(NetClientSessionTestCounters& counters) :
m_counters(&counters)
{

}

NetGatewayTransport* NetClientSessionTestFactory::create()
{
    ++m_counters->m_created;
    if (m_counters->m_factoryMode == 1)
    {
        return nullptr;
    }
    if (m_counters->m_factoryMode == 2)
    {
        throw 1;
    }
    return new NetClientSessionTestTransport(*m_counters);
}

NetClientSessionTestListener::NetClientSessionTestListener() :
m_other(0),
m_nestedSession(nullptr),
m_nestedRequest(0),
m_nestedExecuting(false),
m_nestedCancelOk(false),
m_reenterClosed(false),
m_throwClosed(false),
m_throwResult(false)
{

}

void NetClientSessionTestListener::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    (void)generation;
    (void)maxRequests;
    (void)maxChunk;
    ++m_other;
}

void NetClientSessionTestListener::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
    uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    (void)generation;
    (void)requestId;
    (void)product;
    (void)type;
    (void)status;
    (void)bytes;
    (void)size;
    ++m_other;
    m_resultIds.push_back(requestId);
    m_resultStatus.push_back(status);
    m_events.push_back(requestId);
    if (m_throwResult)
    {
        throw 1;
    }
    if (m_nestedSession != nullptr && m_nestedRequest != 0)
    {
        const uint64_t nested = m_nestedRequest;
        m_nestedRequest = 0;
        m_nestedCancelOk = m_nestedSession->cancel(nested) == NI_OK;
        m_nestedExecuting = m_nestedSession->executing();
    }
}

bool NetClientSessionTestListener::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    (void)generation;
    (void)frame;
    ++m_other;
    return false;
}

void NetClientSessionTestListener::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)detail;
    m_events.push_back(UINT64_MAX);
    m_generations.push_back(generation);
    m_reasons.push_back(reason);
    if (m_reenterClosed && m_nestedSession != nullptr)
    {
        m_nestedExecuting = m_nestedSession->executing();
        m_nestedSession->close(NI_TIMEOUT);
        m_nestedSession->shutdown();
        m_nestedSession->poll();
        m_nestedCancelOk = m_nestedSession->bindGateway(nullptr) == NI_INVALID_STATE;
    }
    if (m_throwClosed)
    {
        throw 1;
    }
}

int32_t NetClientSessionTestListener::entropy(void* context, uint8_t* bytes, uint32_t size)
{
    (void)context;
    std::memset(bytes, 0x41, size);
    return 0;
}

NetClientSessionTestDiscovery::NetClientSessionTestDiscovery() :
m_available(true),
m_invalidPoint(0),
m_throw(false),
m_drain(false),
m_shutdownStarted(false),
m_remaining(0),
m_fault(0)
{

}

int32_t NetClientSessionTestDiscovery::start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath,
    const NetInterfaceTlsOptions& tls)
{
    (void)seeds;
    (void)cachePath;
    (void)tls;
    m_shutdownStarted = false;
    m_remaining = m_drain ? 2 : 0;
    if ((m_fault & 1) != 0)
    {
        m_fault &= ~1U;
        throw 1;
    }
    return (m_fault & 32) != 0 ? NI_INVALID_ARGUMENT : NI_OK;
}

bool NetClientSessionTestDiscovery::poll()
{
    if ((m_fault & 4) != 0)
    {
        m_fault &= ~4U;
        throw 1;
    }
    if (m_shutdownStarted && m_remaining != 0)
    {
        --m_remaining;
    }
    return m_remaining != 0;
}

void NetClientSessionTestDiscovery::shutdown()
{
    m_shutdownStarted = true;
    if ((m_fault & 2) != 0)
    {
        m_fault &= ~2U;
        throw 1;
    }
}

bool NetClientSessionTestDiscovery::stopped() const
{
    if ((m_fault & 16) != 0)
    {
        throw 1;
    }
    return m_remaining == 0;
}

bool NetClientSessionTestDiscovery::choose(NetClientDiscoveryPoint& point)
{
    if ((m_fault & 8) != 0)
    {
        m_fault &= ~8U;
        throw 1;
    }
    if (m_invalidPoint == 1)
    {
        point.m_structSize = 0;
    }
    else if (m_invalidPoint == 2)
    {
        point.m_version = 2;
    }
    else if (m_invalidPoint == 3)
    {
        std::memset(point.m_address, '1', sizeof(point.m_address));
    }
    else if (m_invalidPoint == 4)
    {
        point.m_port = 0;
    }
    return m_available;
}

NetClientSessionTestChannel::NetClientSessionTestChannel(const NetInterfaceHandshakeOptions& options) :
m_fault(0),
m_sendCount(0),
m_observed(nullptr),
m_observedSize(0),
m_handshake(new NetInterfaceHandshake(options, NetInterfaceConnectionTestCasesHelper::recoveryTestNow())),
m_listener(nullptr),
m_announced(false),
m_closing(false),
m_reason(0),
m_queued(false),
m_queuedFrame()
{

}

NetClientSessionTestChannel::~NetClientSessionTestChannel()
{
    shutdown();
}

int32_t NetClientSessionTestChannel::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
    NetTransportListener* listener, uint64_t& peer)
{
    (void)address;
    (void)port;
    (void)options;
    peer = 0;
    if (m_listener != nullptr || listener == nullptr)
    {
        return NI_INVALID_STATE;
    }
    m_listener = listener;
    if ((m_fault & 256) != 0)
    {
        throw 1;
    }
    peer = (m_fault & 128) != 0 ? 0 : 1;
    return (m_fault & 512) != 0 ? NI_NOT_FOUND : NI_OK;
}

bool NetClientSessionTestChannel::poll()
{
    if ((m_fault & 64) != 0)
    {
        m_fault &= ~64U;
        throw 1;
    }
    if (m_closing)
    {
        shutdown();
        return false;
    }
    if (m_listener == nullptr)
    {
        return false;
    }
    if (!m_announced)
    {
        m_announced = true;
        m_listener->onReady(1);
    }
    NetInterfaceFrame frame = {};
    if (!m_closing && m_handshake->peekOutput(frame) == NI_READY && m_listener->onFrame(1, frame))
    {
        m_handshake->consumeOutput();
    }
    if (m_queued && !m_closing && m_listener->onFrame(1, m_queuedFrame))
    {
        m_queued = false;
        m_queuedBody.clear();
    }
    return true;
}

int32_t NetClientSessionTestChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != 1 || m_listener == nullptr || m_closing)
    {
        return NI_INVALID_STATE;
    }
    ++m_sendCount;
    if (frame.m_kind == NI_REQUEST)
    {
        m_observed = frame.m_payload;
        m_observedSize = frame.m_payloadSize;
        if ((m_fault & 1) != 0)
        {
            throw 1;
        }
        return (m_fault & 1024) != 0 ? NI_LIMIT_EXCEEDED : NI_OK;
    }
    if (frame.m_kind == NI_CANCEL || frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK)
    {
        if ((m_fault & (frame.m_kind == NI_CANCEL ? 2U : 4U)) != 0)
        {
            throw 1;
        }
        return NI_OK;
    }
    const int32_t accepted = m_handshake->accept(frame, NetInterfaceConnectionTestCasesHelper::recoveryTestNow());
    return accepted < 0 ? accepted : NI_OK;
}

void NetClientSessionTestChannel::close(uint64_t peer, int32_t reason)
{
    if (peer == 1 && !m_closing)
    {
        m_closing = true;
        m_reason = reason;
    }
    if ((m_fault & 32) != 0)
    {
        throw 1;
    }
}

void NetClientSessionTestChannel::shutdown()
{
    m_closing = true;
    NetTransportListener* listener = m_listener;
    m_listener = nullptr;
    if (listener != nullptr)
    {
        listener->onClosed(1, m_reason, 0);
    }
}

int32_t NetClientSessionTestChannel::configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated)
{
    (void)peer;
    (void)types;
    (void)count;
    (void)negotiated;
    if ((m_fault & 8) != 0)
    {
        throw 1;
    }
    return NI_OK;
}

int32_t NetClientSessionTestChannel::outputState(uint64_t peer) const
{
    (void)peer;
    if ((m_fault & 16) != 0)
    {
        throw 1;
    }
    return NI_OK;
}

void NetClientSessionTestChannel::queueFrame(const NetInterfaceFrame& frame)
{
    m_queuedBody.clear();
    if (frame.m_payloadSize != 0)
    {
        m_queuedBody.assign(frame.m_payload, frame.m_payload + frame.m_payloadSize);
    }
    m_queuedFrame = frame;
    m_queuedFrame.m_payload = m_queuedBody.empty() ? nullptr : m_queuedBody.data();
    m_queued = true;
}

void NetClientSessionTestDiscovery::failed(const NetClientDiscoveryPoint& point)
{
    (void)point;
    if (m_throw)
    {
        throw 1;
    }
}