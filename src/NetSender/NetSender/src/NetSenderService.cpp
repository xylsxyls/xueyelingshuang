#include "NetSenderService.h"
#include "NetSenderMacro.h"
#include "CSystem/CSystemAPI.h"
#include "NetSenderServiceState.h"
#include <chrono>
#include "NetSenderServiceManaged.h"
#include <cstring>

/** 本地服务认证单调时钟
@return 毫秒
*/
static uint64_t ServiceNow()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetSenderServiceListener::~NetSenderServiceListener()
{

}

NetSenderService::NetSenderService(NetSenderLocalInbox& inbox) :
m_state(new NetSenderServiceState(inbox))
{

}

NetSenderService::~NetSenderService()
{
    shutdown();
    delete m_state;
}

int32_t NetSenderService::connect(int32_t pid, const NetInterfaceTlsOptions& tls, uint32_t product, const uint8_t* secret,
                                  NetSenderServiceListener* listener, uint32_t limit)
{
    if (m_state->m_started || m_state->m_closed || listener == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t encoded = NetInterfaceService::hello(secret, limit, m_state->m_hello, sizeof(m_state->m_hello));
    if (encoded != NI_OK)
    {
        return encoded;
    }
    m_state->m_product = product;
    m_state->m_listener = listener;
    m_state->m_started = true;
    const int32_t result = m_state->m_local.connect(pid, tls, this, m_state->m_peer);
    if (result != NI_OK)
    {
        m_state->m_closed = true;
        m_state->wipe();
    }
    return result;
}

bool NetSenderService::poll()
{
    if (m_state->m_managed) { return m_state->m_managed->poll(); }
    if (!m_state->m_started || m_state->m_closed)
    {
        return false;
    }
    if (m_state->m_endpoint == 0 && m_state->m_deadline != 0 && ServiceNow() >= m_state->m_deadline)
    {
        m_state->m_local.close(m_state->m_peer, NI_TIMEOUT);
    }
    return m_state->m_local.poll() && !m_state->m_closed;
}

void NetSenderService::onLocalReady(uint64_t peer, int32_t pid)
{
    (void)pid;
    if (peer != m_state->m_peer || m_state->m_deadline != 0)
    {
        m_state->m_local.close(peer, NI_PROTOCOL_ERROR);
        return;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_HELLO;
    frame.m_messageType = 3;
    frame.m_productId = m_state->m_product;
    frame.m_payload = m_state->m_hello;
    frame.m_payloadSize = sizeof(m_state->m_hello);
    m_state->m_deadline = ServiceNow() + 5000;
    const int32_t sent = m_state->m_local.send(peer, frame);
    m_state->wipe();
    if (sent != NI_OK)
    {
        m_state->m_local.close(peer, sent);
    }
}

bool NetSenderService::onLocalFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (m_state->m_endpoint == 0)
    {
        if (m_state->m_deadline == 0 || ServiceNow() >= m_state->m_deadline || frame.m_kind != NI_HELLO_REPLY || frame.m_messageType != 3 ||
            frame.m_productId != m_state->m_product || frame.m_requestId == 0 || frame.m_payloadSize != 0)
        {
            m_state->m_local.close(peer, NI_PROTOCOL_ERROR);
            return true;
        }
        m_state->m_endpoint = frame.m_requestId;
        m_state->m_listener->onServiceReady(m_state->m_endpoint);
        return true;
    }
    NetInterfaceOrigin origin = {};
    NetInterfaceFrame business = {};
    if (NetInterfaceService::unwrap(frame, origin, business) != NI_OK)
    {
        m_state->m_local.close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    return m_state->m_listener->onServiceFrame(business, origin);
}

void NetSenderService::onLocalClosed(uint64_t peer, int32_t reason)
{
    if (peer == m_state->m_peer && !m_state->m_closed)
    {
        m_state->m_closed = true;
        m_state->m_endpoint = 0;
        m_state->wipe();
        m_state->m_listener->onServiceClosed(reason);
    }
}

int32_t NetSenderService::configureCompression(const uint32_t* types, uint32_t count)
{
    return m_state->m_managed ? m_state->m_managed->configureCompression(types, count) : NI_NOT_SUPPORTED;
}

int32_t NetSenderService::send(const NetInterfaceFrame& frame)
{
    if (endpoint() == 0)
    {
        return NI_INVALID_STATE;
    }
    if (frame.m_kind == NI_CANCEL)
    {
        NetInterfaceFrame unused = {};
        if (NetInterfaceRpc::control(frame, unused) != NI_OK)
        {
            return NI_PROTOCOL_ERROR;
        }
    }
    else if (frame.m_kind == NI_DATA)
    {
        NetInterfaceFileData data = {};
        if (NetInterfaceFile::parseData(frame, data) != NI_OK)
        {
            return NI_PROTOCOL_ERROR;
        }
    }
    else if (frame.m_kind == NI_DATA_ACK)
    {
        NetInterfaceFileAck ack = {};
        if (NetInterfaceFile::parseAck(frame, ack) != NI_OK)
        {
            return NI_PROTOCOL_ERROR;
        }
    }
    else if (frame.m_kind == NI_EVENT)
    {
        if (frame.m_messageType == NI_FILE_CANCEL && frame.m_payloadSize >= 4 && frame.m_payload != nullptr &&
            std::memcmp(frame.m_payload, "NFN2", 4) == 0)
        {
            NetInterfaceFileNotice notice = {};
            if (NetInterfaceFile::parseNotice(frame, notice) != NI_OK)
            {
                return NI_PROTOCOL_ERROR;
            }
        }
        else
        {
            NetInterfaceEventView event = {};
            if (NetInterfaceEvent::parse(frame, event) != NI_OK)
            {
                return NI_PROTOCOL_ERROR;
            }
        }
    }
    else
    {
        NetInterfaceRpcView rpc = {};
        if (NetInterfaceRpc::parse(frame, rpc) != NI_OK || (frame.m_kind == NI_REQUEST && rpc.m_parent == 0))
        {
            return NI_PROTOCOL_ERROR;
        }
    }
    if (frame.m_kind == NI_REQUEST && frame.m_requestId <= m_state->m_lastSent)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t sent = m_state->m_managed ? m_state->m_managed->send(frame) : m_state->m_local.send(m_state->m_peer, frame);
    if (sent == NI_OK && frame.m_kind == NI_REQUEST)
    {
        m_state->m_lastSent = frame.m_requestId;
        if (frame.m_requestId > m_state->m_lastAllocated)
        {
            m_state->m_lastAllocated = frame.m_requestId;
        }
    }
    return sent;
}

int32_t NetSenderService::nextRequestId(uint64_t& requestId)
{
    requestId = 0;
    if (endpoint() == 0)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_lastAllocated == UINT64_MAX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    requestId = ++m_state->m_lastAllocated;
    return NI_OK;
}

void NetSenderService::shutdown()
{
    if (m_state->m_managed) { m_state->m_managed->shutdown(); }
    m_state->m_local.shutdown();
    m_state->wipe();
    m_state->m_closed = true;
    m_state->m_endpoint = 0;
}

uint64_t NetSenderService::endpoint() const
{
    return m_state->m_managed ? m_state->m_managed->endpoint() : m_state->m_endpoint;
}

int32_t NetSenderService::sendEvent(uint64_t target, uint32_t product, uint32_t type, const uint8_t* bytes, uint32_t size, bool revokeScope)
{
    if (endpoint() == 0)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_lastEvent == UINT64_MAX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    if (m_state->m_product != 0 && product != m_state->m_product)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (revokeScope && type != NI_FILE_REVOKE_SCOPE)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        uint32_t required = 0;
        const int32_t probe = NetInterfaceEvent::encode(target, revokeScope ? 1 : 0, bytes, size, nullptr, 0, required);
        if (probe != NI_BUFFER_TOO_SMALL)
        {
            return probe;
        }
        std::vector<uint8_t> payload(required);
        const int32_t encoded = NetInterfaceEvent::encode(target, revokeScope ? 1 : 0, bytes, size, payload.data(),
                                                          static_cast<uint32_t>(payload.size()), required);
        if (encoded != NI_OK)
        {
            return encoded;
        }
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_EVENT;
        frame.m_productId = product;
        frame.m_messageType = type;
        frame.m_requestId = m_state->m_lastEvent + 1;
        frame.m_payload = payload.data();
        frame.m_payloadSize = required;
        const int32_t result = send(frame);
        if (result == NI_OK)
        {
            ++m_state->m_lastEvent;
        }
        NetInterface::wipe(payload.data(), static_cast<uint32_t>(payload.size()));
        return result;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderService::connectManaged(int32_t pid, uint32_t product, NetInterfaceLinkServiceProof proof, void* context,
    NetSenderServiceListener* listener, uint32_t limit, uint32_t weight, uint32_t channels)
{
    if (m_state->m_started || m_state->m_closed || listener == nullptr) { return NI_INVALID_STATE; }
    m_state->m_product = product;
    m_state->m_listener = listener;
    m_state->m_managed.reset(new NetSenderServiceManaged(*m_state->m_inbox));
    m_state->m_started = true;
    const int32_t result = m_state->m_managed->connect(pid, product, proof, context, listener, limit, weight, channels);
    if (result != NI_OK) { m_state->m_closed = true; }
    return result;
}

int32_t NetSenderService::findGateway(int32_t specified, int32_t& pid)
{
    pid = 0;
    if (specified < 0) { return NI_INVALID_ARGUMENT; }
#ifdef _WIN32
    const char* name = "NetServerManager1.1.exe";
#else
    const char* name = "NetServerManager1.1";
#endif
    const std::vector<uint32_t> candidates = specified == 0 ? CSystem::processPid(std::string(name)) :
        std::vector<uint32_t>(1, static_cast<uint32_t>(specified));
    for (uint32_t candidate : candidates)
    {
        if (!CSystem::isProcessLeaseReady("NetServerManager", candidate))
        {
            continue;
        }
        if (pid != 0) { pid = 0; return NI_INVALID_STATE; }
        pid = static_cast<int32_t>(candidate);
    }
    return pid == 0 ? NI_NOT_FOUND : NI_OK;
}