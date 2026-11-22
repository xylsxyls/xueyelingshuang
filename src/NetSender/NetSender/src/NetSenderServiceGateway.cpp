#include "NetSenderServiceGateway.h"
#include "NetSenderServiceGatewayHelper.h"
#include "NetSenderMacro.h"
#include "CSystem/CSystemAPI.h"
#include "NetSenderServiceGatewayState.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetSenderServiceOutgoingTransport.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <sstream>

/** 管理元数据只接受完整uint64或非负int32
@param [in] message protobuf
@param [in] key 全局键
@param [out] result 整数
@return 是否有效
*/
static bool ServiceGatewayNumber(const ProtoMessage& message, int32_t key, uint64_t& result)
{
    Variant value;
    if (!message.getKeyValue(key, value)) { return false; }
    if (value.type() == Variant::UINT64) { result = static_cast<uint64_t>(value); return true; }
    if (value.type() != Variant::INT32 || static_cast<int32_t>(value) < 0) { return false; }
    result = static_cast<uint32_t>(static_cast<int32_t>(value));
    return true;
}

NetSenderServiceGatewayListener::~NetSenderServiceGatewayListener()
{

}

NetSenderServiceGateway::NetSenderServiceGateway(NetSenderLocalInbox& inbox) :
m_state(new NetSenderServiceGatewayState(inbox))
{

}

NetSenderServiceGateway::~NetSenderServiceGateway()
{
    shutdown();
    delete m_state;
}

int32_t NetSenderServiceGateway::configureClientPool(const LockFreeHashMapPoolConfig& config)
{
    if (m_state->m_phase != NET_SERVICE_IDLE)
    {
        return NI_INVALID_STATE;
    }
    if (config.maximumCapacity == 0 || config.maximumCapacity > UINT32_MAX)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::unique_ptr<NetObjectPool<NetSenderServiceGatewayClient>> pool(new NetObjectPool<NetSenderServiceGatewayClient>(config));
        if (pool->status() != NI_OK)
        {
            return pool->status();
        }
        m_state->m_clientPool.swap(pool);
        m_state->m_channels = config.maximumCapacity < 0 ? UINT32_MAX : static_cast<uint32_t>(config.maximumCapacity);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderServiceGateway::configureReceiveWindow(uint32_t bytes)
{
    if (m_state->m_phase != NET_SERVICE_IDLE)
    {
        return NI_INVALID_STATE;
    }
    NetInterfaceCredit window;
    const int32_t result = window.configure(bytes);
    if (result == NI_OK)
    {
        m_state->m_window = bytes;
    }
    return result;
}

int32_t NetSenderServiceGateway::connect(int32_t pid, uint32_t product, NetInterfaceLinkServiceProof proof, void* context,
    NetSenderServiceGatewayListener* listener, uint32_t limit, uint32_t weight, uint32_t channels)
{
    if (m_state->m_phase != NET_SERVICE_IDLE || listener == nullptr || proof == nullptr || limit == 0 ||
        weight == 0 || weight > 1000) { return NI_INVALID_ARGUMENT; }
    if (channels != 0)
    {
        LockFreeHashMapPoolConfig config;
        config.maximumCapacity = channels;
        config.initialCapacity = (std::min)(config.initialCapacity, static_cast<size_t>(channels));
        const int32_t configured = configureClientPool(config);
        if (configured != NI_OK)
        {
            return configured;
        }
    }
    if (m_state->m_clientPool->status() != NI_OK)
    {
        return m_state->m_clientPool->status();
    }
    if (pid <= 0 || !CSystem::isProcessLeaseReady("NetServerManager", static_cast<uint32_t>(pid)))
    {
        return NI_NOT_FOUND;
    }
    try
    {
        std::ostringstream name;
        name << "service." << product;
        m_state->m_name = name.str();
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
    if (!m_state->m_key.empty())
    {
        NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_state->m_key[0]), static_cast<uint32_t>(m_state->m_key.size()));
    }
    m_state->m_key.clear();
    m_state->m_request.clear();
    uint8_t keyBytes[NET_INTERFACE_IDENTITY_PEM_BYTES] = {};
    uint8_t requestBytes[NET_INTERFACE_IDENTITY_PEM_BYTES] = {};
    uint32_t keySize = 0;
    uint32_t requestSize = 0;
    int32_t created = NetInterface::certificateCreateRequest(reinterpret_cast<const uint8_t*>(m_state->m_name.data()),
        static_cast<uint32_t>(m_state->m_name.size()), NetSenderServiceGatewayHelper::serviceGatewayEntropy, nullptr, keyBytes, sizeof(keyBytes),
        &keySize, requestBytes, sizeof(requestBytes), &requestSize);
    if (created == NI_OK)
    {
        try
        {
            m_state->m_key.assign(reinterpret_cast<const char*>(keyBytes), keySize - 1);
            m_state->m_request.assign(reinterpret_cast<const char*>(requestBytes), requestSize - 1);
        }
        catch (...)
        {
            if (!m_state->m_key.empty())
            {
                NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_state->m_key[0]), static_cast<uint32_t>(m_state->m_key.size()));
            }
            m_state->m_key.clear();
            m_state->m_request.clear();
            created = NI_NO_MEMORY;
        }
    }
    NetInterface::wipe(keyBytes, sizeof(keyBytes));
    if (created != NI_OK) { return created; }
    m_state->m_phase = NET_SERVICE_CONNECTING;
    m_state->m_listener = listener;
    m_state->m_product = product;
    m_state->m_limit = limit;
    m_state->m_weight = weight;
    NetInterfaceLinkOptions options = {};
    options.m_version = 3;
    options.m_role = NI_LINK_SERVICE;
    options.m_name = m_state->m_name.c_str();
    options.m_tls.privateKey = reinterpret_cast<const uint8_t*>(m_state->m_key.c_str());
    options.m_tls.privateKeySize = static_cast<uint32_t>(m_state->m_key.size() + 1);
    options.m_tls.handshakeTimeoutMs = 5000;
    options.m_claims = reinterpret_cast<const uint8_t*>(m_state->m_request.data());
    options.m_claimsSize = static_cast<uint32_t>(m_state->m_request.size());
    options.m_serviceProof = proof;
    options.m_serviceContext = context;
    const int32_t connected = m_state->m_link.connect(pid, options, this, m_state->m_peer);
    if (connected != NI_OK)
    {
        m_state->m_phase = NET_SERVICE_CLOSED;
        m_state->m_listener = nullptr;
        m_state->m_link.shutdown();
    }
    return connected;
}

void NetSenderServiceGateway::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
{
    if (peer != m_state->m_peer || identity.m_role != NI_LINK_NODE || m_state->m_phase != NET_SERVICE_CONNECTING)
    {
        m_state->fail(NI_PROTOCOL_ERROR);
        return;
    }
    const int32_t granted = m_state->m_link.serviceGrant(peer, m_state->m_certificate);
    if (granted != NI_READY)
    {
        m_state->fail(granted < 0 ? granted : NI_PROTOCOL_ERROR);
        return;
    }
    m_state->m_nodeName = identity.m_name == nullptr ? "" : identity.m_name;
    if (identity.m_certificateSize != 0)
    {
        m_state->m_nodeCertificate.assign(reinterpret_cast<const char*>(identity.m_certificate), identity.m_certificateSize);
    }
    m_state->m_phase = NET_SERVICE_AUTHENTICATED;
}

bool NetSenderServiceGateway::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != m_state->m_peer || m_state->closing()) { return true; }
    ProtoMessage message;
    if (frame.m_payloadSize == 0 || !message.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)))
    {
        m_state->fail(NI_PROTOCOL_ERROR);
        return true;
    }
    if (frame.m_kind == NI_RESPONSE && frame.m_requestId == 1 && frame.m_messageType == NI_GATEWAY_SERVICE_READY &&
        m_state->m_phase == NET_SERVICE_REGISTERING)
    {
        if (message.getCode() != NI_OK) { m_state->fail(message.getCode()); return true; }
        m_state->m_phase = NET_SERVICE_READY;
        m_state->m_listener->onGatewayServiceReady();
        return true;
    }
    if (frame.m_kind == NI_RESPONSE && frame.m_messageType == NI_GATEWAY_OPEN && frame.m_requestId >= 2)
    {
        if (m_state->m_phase != NET_SERVICE_READY || frame.m_requestId > m_state->m_highestOutgoing + 1) { m_state->fail(NI_PROTOCOL_ERROR); return true; }
        if (m_state->m_outgoing != nullptr && frame.m_requestId == m_state->m_outgoing->m_request)
        {
            return m_state->m_outgoing->control(frame);
        }
        return true;
    }
    uint64_t channel = 0;
    if (m_state->m_phase != NET_SERVICE_READY || frame.m_kind != NI_EVENT || frame.m_requestId != 0 ||
        !ServiceGatewayNumber(message, NI_GATEWAY_CHANNEL, channel))
    {
        m_state->fail(NI_PROTOCOL_ERROR);
        return true;
    }
    if (channel < UINT64_C(0x8000000000000000))
    {
        if (channel == 0 || channel > m_state->m_highestOutgoing ||
            (frame.m_messageType != NI_GATEWAY_CREDIT && frame.m_messageType != NI_GATEWAY_CHANNEL_CLOSE))
        {
            m_state->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        if (m_state->m_outgoing != nullptr && channel == m_state->m_outgoing->m_channel)
        {
            return m_state->m_outgoing->control(frame);
        }
        return true;
    }
    if (frame.m_messageType == NI_GATEWAY_CREDIT)
    {
        uint64_t amount = 0;
        auto client = m_state->m_clients.find(channel);
        if (client == m_state->m_clients.end()) { return true; }
        if (!ServiceGatewayNumber(message, NI_GATEWAY_CREDIT_BYTES, amount) || amount > client->second->m_credit.capacity() ||
            client->second->m_credit.grant(static_cast<uint32_t>(amount)) != NI_OK) { m_state->fail(NI_PROTOCOL_ERROR); }
        return true;
    }
    if (frame.m_messageType == NI_GATEWAY_CHANNEL_CLOSE)
    {
        Variant reason;
        if (!message.getKeyValue(NI_GATEWAY_REASON, reason) || reason.type() != Variant::INT32)
        {
            m_state->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        m_state->close(channel, static_cast<int32_t>(reason), false);
        return true;
    }
    uint64_t product = 0;
    uint64_t role = 0;
    if (frame.m_messageType != NI_GATEWAY_SERVICE_OPEN || channel < UINT64_C(0x8000000000000000) ||
        channel <= m_state->m_highestClient ||
        !ServiceGatewayNumber(message, NI_GATEWAY_TARGET_PRODUCT, product) || product != m_state->m_product ||
        !ServiceGatewayNumber(message, NI_GATEWAY_SOURCE_ROLE, role) || role < NI_LINK_CLIENT || role > NI_LINK_SERVICE ||
        (product == 0 && role != NI_LINK_SERVICE))
    {
        m_state->fail(NI_PROTOCOL_ERROR);
        return true;
    }
    NetInterfaceTlsOptions tls = {};
    tls.server = 1;
    tls.certificate = reinterpret_cast<const uint8_t*>(m_state->m_certificate.c_str());
    tls.certificateSize = static_cast<uint32_t>(m_state->m_certificate.size() + 1);
    tls.privateKey = reinterpret_cast<const uint8_t*>(m_state->m_key.c_str());
    tls.privateKeySize = static_cast<uint32_t>(m_state->m_key.size() + 1);
    tls.entropy = NetSenderServiceGatewayHelper::serviceGatewayEntropy;
    tls.handshakeTimeoutMs = 5000;
    NetObjectReference<NetSenderServiceGatewayClient> client;
    const int32_t allocated = m_state->m_clientPool->create(client, tls, static_cast<uint32_t>(role));
    if (allocated != NI_OK)
    {
        // CLOSE和替换OPEN可在同一次poll出现；等owner归还正在关闭的真实槽后重试。
        if (allocated == NI_LIMIT_EXCEEDED)
        {
            for (const auto& item : m_state->m_clients)
            {
                if (item.second->m_closing)
                {
                    return false;
                }
            }
        }
        m_state->fail(allocated);
        return true;
    }
    uint32_t window = 0;
    if (NetInterfaceCredit::window(message, window) != NI_OK || client->m_credit.configure(window) != NI_OK)
    {
        m_state->fail(NI_PROTOCOL_ERROR); return true;
    }
    if (client->m_connection.status() < 0) { m_state->fail(client->m_connection.status()); return true; }
    m_state->m_clients.insert(std::make_pair(channel, std::move(client)));
    m_state->m_highestClient = channel;
    return true;
}

bool NetSenderServiceGateway::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (peer != m_state->m_peer || m_state->closing()) { return true; }
    if (record.m_channel < UINT64_C(0x8000000000000000))
    {
        if (record.m_channel == 0 || record.m_channel > m_state->m_highestOutgoing) { m_state->fail(NI_PROTOCOL_ERROR); return true; }
        if (m_state->m_outgoing != nullptr && record.m_channel == m_state->m_outgoing->m_channel)
        {
            return m_state->m_outgoing->record(record);
        }
        return true;
    }
    auto found = m_state->m_clients.find(record.m_channel);
    if (found == m_state->m_clients.end() || found->second->m_closing)
    {
        if (record.m_channel > m_state->m_highestClient) { m_state->fail(NI_PROTOCOL_ERROR); }
        return true;
    }
    if (record.m_product != m_state->m_product) { m_state->fail(NI_PROTOCOL_ERROR); return true; }
    if (record.m_kind == NI_TUNNEL_CLOSE)
    {
        const uint32_t reason = (static_cast<uint32_t>(record.m_data[0]) << 24) | (static_cast<uint32_t>(record.m_data[1]) << 16) |
            (static_cast<uint32_t>(record.m_data[2]) << 8) | record.m_data[3];
        m_state->close(record.m_channel, static_cast<int32_t>(reason), false);
        return true;
    }
    NetSenderServiceGatewayClient& client = *found->second;
    if (!client.m_input.empty())
    {
        if (client.m_input.size() + record.m_size > client.m_credit.capacity() || client.m_credit.received(record.m_size) != NI_OK)
        {
            m_state->close(record.m_channel, NI_LIMIT_EXCEEDED);
            return true;
        }
        client.m_input.insert(client.m_input.end(), record.m_data, record.m_data + record.m_size);
        return true;
    }
    if (client.m_credit.received(record.m_size) != NI_OK) { m_state->close(record.m_channel, NI_PROTOCOL_ERROR); return true; }
    uint32_t consumed = 0;
    const int32_t fed = client.m_connection.feed(record.m_data, record.m_size, consumed);
    if (fed < 0) { m_state->close(record.m_channel, fed); return true; }
    if (consumed != record.m_size) { client.m_input.assign(record.m_data + consumed, record.m_data + record.m_size); }
    client.m_lastReceive = NetSenderServiceGatewayHelper::serviceGatewayNow();
    return true;
}

void NetSenderServiceGateway::onLinkClosed(uint64_t peer, int32_t reason)
{
    if (peer == m_state->m_peer) { m_state->fail(reason); }
}

bool NetSenderServiceGateway::poll()
{
    return m_state->poll();
}

int32_t NetSenderServiceGateway::configureCompression(const uint32_t* types, uint32_t count)
{
    if (count != 0 && types == nullptr) { return NI_INVALID_ARGUMENT; }
    if (!m_state->m_clients.empty()) { return NI_INVALID_STATE; }
    for (uint32_t index = 0; index < count; ++index) { if (types[index] == 0) { return NI_INVALID_ARGUMENT; } }
    try
    {
        std::vector<uint32_t> copied;
        if (count != 0) { copied.assign(types, types + count); }
        m_state->m_compressionTypes.swap(copied);
        return NI_OK;
    }
    catch (...) { return NI_NO_MEMORY; }
}

int32_t NetSenderServiceGateway::send(uint64_t channel, const NetInterfaceFrame& frame)
{
    auto found = m_state->m_clients.find(channel);
    if (m_state->m_phase != NET_SERVICE_READY || found == m_state->m_clients.end() || !found->second->m_ready ||
        found->second->m_closing) { return NI_INVALID_STATE; }
    if ((frame.m_productId == 0 && !(m_state->m_product == 0 && frame.m_kind == NI_EVENT)) ||
        (m_state->m_product != 0 && frame.m_productId != m_state->m_product) ||
        frame.m_kind == NI_HELLO || frame.m_kind == NI_HELLO_REPLY) { return NI_INVALID_ARGUMENT; }
    return found->second->m_connection.send(frame);
}

void NetSenderServiceGateway::close(uint64_t channel, int32_t reason)
{
    m_state->close(channel, reason);
}

void NetSenderServiceGateway::shutdown()
{
    if (m_state->m_phase == NET_SERVICE_IDLE || m_state->m_phase == NET_SERVICE_CLOSED) { return; }
    m_state->fail(0);
    if (!m_state->m_polling) { poll(); }
}

bool NetSenderServiceGateway::ready() const
{
    return m_state->m_phase == NET_SERVICE_READY;
}

int32_t NetSenderServiceGateway::clientInfo(uint64_t channel, uint32_t fields[4]) const
{
    if (fields == nullptr) { return NI_INVALID_ARGUMENT; }
    std::memset(fields, 0, sizeof(uint32_t) * 4);
    const auto found = m_state->m_clients.find(channel);
    if (found == m_state->m_clients.end()) { return NI_NOT_FOUND; }
    if (!found->second->m_ready || found->second->m_closing || !found->second->m_handshake) { return NI_INVALID_STATE; }
    uint8_t nonce[16] = {};
    uint8_t device[16] = {};
    return found->second->m_handshake->peerInfo(fields, nonce, device);
}