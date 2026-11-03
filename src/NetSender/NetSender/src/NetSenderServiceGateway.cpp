#include "NetSenderServiceGateway.h"
#include "NetSenderServiceGatewayHelper.h"
#include "NetSenderMacro.h"
#include "CSystem/CSystemAPI.h"
#include "NetSenderServiceGatewayState.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetSenderServiceOutgoingTransport.h"
#include "ProtoMessage/ProtoMessageAPI.h"
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

int32_t NetSenderServiceGateway::connect(int32_t pid, uint32_t product, NetInterfaceLinkServiceProof proof, void* context,
    NetSenderServiceGatewayListener* listener, uint32_t limit, uint32_t weight, uint32_t channels)
{
    if (m_state->m_phase != NET_SERVICE_IDLE || listener == nullptr || proof == nullptr || limit == 0 || limit > 128 ||
        weight == 0 || weight > 1000 || channels > 512) { return NI_INVALID_ARGUMENT; }
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
    m_state->m_channels = channels == 0 ? limit : channels;
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
    if (m_state->m_clients.size() >= m_state->m_channels)
    {
        // CLOSE and replacement OPEN can be decoded in the same link poll.
        // Let the outer poll retire closing clients before retrying this OPEN;
        // the gateway has already released that slot, so this is not a breach.
        for (const auto& item : m_state->m_clients)
        {
            if (item.second->m_closing) { return false; }
        }
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
    std::unique_ptr<NetSenderServiceGatewayClient> client(new NetSenderServiceGatewayClient(tls, static_cast<uint32_t>(role)));
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
    if (count > 64 || (count != 0 && types == nullptr)) { return NI_INVALID_ARGUMENT; }
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

// NET_SERVICE_GATEWAY_UNIT_BEGIN
// #include "NetSender/NetSenderServiceGatewayTest.h"
// #include "NetSender/NetSenderServiceGatewayState.h"
// #include "CSystem/CSystemAPI.h"
// #include <cstdio>
// #include <stdexcept>
// #include <chrono>
// #include <test/certs.h>
// #include <google/protobuf/stubs/common.h>
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
// #include <crtdbg.h>
// static int32_t s_serviceGatewayRejectAfter = -1;
// static uint32_t s_serviceGatewayAllocations = 0;
//
// /** 本隔离main内逐次拒绝普通分配，验证注册和关闭代码失败后不会卡在polling
// @param [in] operation CRT操作
// @param [in] block 堆块类别；其他参数遵从CRT原型
// @return 是否允许本次操作
// */
// static int __cdecl ServiceGatewayMemoryHook(int operation, void*, size_t, int block, long, const unsigned char*, int)
// {
//     if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
//     {
//         ++s_serviceGatewayAllocations;
//         if (s_serviceGatewayRejectAfter == 0)
//         {
//             return 0;
//         }
//         if (s_serviceGatewayRejectAfter > 0)
//         {
//             --s_serviceGatewayRejectAfter;
//         }
//     }
//     return 1;
// }
// #endif
//
// NetSenderServiceGatewayTest::NetSenderServiceGatewayTest(NetSenderServiceGatewayState& state) :
// m_state(&state),
// m_clients(0),
// m_services(0),
// m_throw(false),
// m_bad(false)
// {
//
// }
//
// void NetSenderServiceGatewayTest::onGatewayServiceReady()
// {
//     m_bad = true;
// }
//
// void NetSenderServiceGatewayTest::onGatewayClientReady(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin)
// {
//     (void)channel;
//     (void)role;
//     (void)origin;
//     m_bad = true;
// }
//
// bool NetSenderServiceGatewayTest::onGatewayClientFrame(uint64_t channel, uint32_t role,
//     const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin)
// {
//     (void)channel;
//     (void)role;
//     (void)frame;
//     (void)origin;
//     m_bad = true;
//     return true;
// }
//
// void NetSenderServiceGatewayTest::onGatewayClientClosed(uint64_t channel, int32_t reason)
// {
//     (void)reason;
//     ++m_clients;
//     m_bad = m_bad || m_state->m_clients.count(channel) != 0 || m_services != 0;
//     if (m_state->closing())
//     {
//         m_bad = m_bad || m_state->poll();
//     }
//     if (m_throw)
//     {
//         throw 7;
//     }
// }
//
// void NetSenderServiceGatewayTest::onGatewayServiceClosed(int32_t reason)
// {
//     ++m_services;
//     m_bad = m_bad || !m_state->m_clients.empty() || m_state->m_phase != NET_SERVICE_CLOSED ||
//         m_state->m_reason != reason || !m_state->m_key.empty() || m_state->poll();
//     m_state->fail(NI_PROTOCOL_ERROR);
//     m_bad = m_bad || m_state->m_reason != reason;
//     if (m_throw)
//     {
//         throw 8;
//     }
// }
//
// /** 建立真实TLS拥有者但不联网，供停止/析构路径测试
// @param [in,out] state 实际服务状态
// @param [in] count 需要回收的客户端数
// @return 是否创建成功
// */
// static bool ServiceGatewayCreateClients(NetSenderServiceGatewayState& state, uint32_t count)
// {
//     for (uint32_t index = 0; index < count; ++index)
//     {
//         NetInterfaceTlsOptions tls = {};
//         tls.entropy = CSystem::secureRandomBytesCallback;
//         tls.handshakeTimeoutMs = 5000;
//         tls.authority = reinterpret_cast<const uint8_t*>(mbedtls_test_ca_crt_rsa_sha256_pem);
//         tls.authoritySize = static_cast<uint32_t>(mbedtls_test_ca_crt_rsa_sha256_pem_len);
//         tls.hostname = "localhost";
//         std::unique_ptr<NetSenderServiceGatewayClient> client(new NetSenderServiceGatewayClient(tls, NI_LINK_CLIENT));
//         if (client->m_connection.status() < 0)
//         {
//             return false;
//         }
//         state.m_clients.insert(std::make_pair(index + 1, std::move(client)));
//     }
//     return true;
// }
//
// bool NetSenderServiceGatewayTest::run()
// {
//     const uint64_t now = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
//         std::chrono::steady_clock::now().time_since_epoch()).count());
//     uint32_t cycles = 1;
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
//     cycles = 32;
// #endif
//     for (uint32_t mode = 0; mode < 3; ++mode)
//     {
//         for (uint32_t step = 0; step < cycles; ++step)
//         {
//             NetSenderLocalInbox inbox;
//             NetSenderServiceGatewayState state(inbox);
//             NetSenderServiceGatewayTest listener(state);
//             state.m_listener = &listener;
//             state.m_phase = mode == 0 ? NET_SERVICE_AUTHENTICATED : NET_SERVICE_READY;
//             state.m_channels = 8;
//             state.m_weight = 1;
//             state.m_key.assign(64, 'q');
//             if (mode == 1)
//             {
//                 state.m_outgoingClose = 1;
//             }
//             if (mode == 2)
//             {
//                 if (!ServiceGatewayCreateClients(state, 2))
//                 {
//                     return false;
//                 }
//                 state.close(1, NI_CANCELLED);
//                 state.close(2, NI_CANCELLED, false);
//             }
//             bool escaped = false;
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
//             s_serviceGatewayRejectAfter = static_cast<int32_t>(step);
//             const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(ServiceGatewayMemoryHook);
// #endif
//             try
//             {
//                 state.poll();
//             }
//             catch (...)
//             {
//                 escaped = true;
//             }
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
//             _CrtSetAllocHook(previous);
//             s_serviceGatewayRejectAfter = -1;
//             if (step == 0 && state.m_reason != NI_NO_MEMORY)
//             {
//                 return false;
//             }
// #endif
//             if (escaped || state.m_polling || state.m_phase != NET_SERVICE_CLOSED || state.poll() ||
//                 listener.m_bad || listener.m_services != 1 || listener.m_clients != (mode == 2 ? 2U : 0U))
//             {
//                 std::printf("Service gateway failure mode=%u step=%u escaped=%u phase=%u polling=%u clients=%u services=%u\n",
//                     mode, step, escaped ? 1U : 0U, state.m_phase, state.m_polling ? 1U : 0U, listener.m_clients, listener.m_services);
//                 return false;
//             }
//         }
//     }
//     for (uint32_t throws = 0; throws < 2; ++throws)
//     {
//         NetSenderLocalInbox inbox;
//         NetSenderServiceGatewayState state(inbox);
//         NetSenderServiceGatewayTest listener(state);
//         listener.m_throw = throws != 0;
//         state.m_listener = &listener;
//         state.m_phase = NET_SERVICE_READY;
//         if (!ServiceGatewayCreateClients(state, 8))
//         {
//             return false;
//         }
//         state.fail(NI_CANCELLED);
//         state.fail(NI_PROTOCOL_ERROR);
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
//         s_serviceGatewayRejectAfter = 0;
//         s_serviceGatewayAllocations = 0;
//         const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(ServiceGatewayMemoryHook);
// #endif
//         const bool active = state.poll();
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_GATEWAY_NO_HEAP)
//         _CrtSetAllocHook(previous);
//         s_serviceGatewayRejectAfter = -1;
//         if (s_serviceGatewayAllocations != 0)
//         {
//             return false;
//         }
// #endif
//         if (active || state.m_polling || state.m_phase != NET_SERVICE_CLOSED || state.m_reason != NI_CANCELLED ||
//             listener.m_bad || listener.m_clients != 8 || listener.m_services != 1)
//         {
//             return false;
//         }
//     }
//     {
//         NetSenderLocalInbox inbox;
//         NetSenderServiceGatewayState state(inbox);
//         NetSenderServiceGatewayTest listener(state);
//         state.m_listener = &listener;
//         if (state.poll() || state.m_polling || state.closing())
//         {
//             return false;
//         }
//         state.m_phase = NET_SERVICE_READY;
//         if (!ServiceGatewayCreateClients(state, 2))
//         {
//             return false;
//         }
//         state.close(1, NI_CANCELLED, false);
//         state.collectClosed(now);
//         if (listener.m_clients != 1 || listener.m_services != 0 || state.m_clients.count(2) != 1 ||
//             state.m_phase != NET_SERVICE_READY || listener.m_bad)
//         {
//             return false;
//         }
//         state.fail(0);
//         if (state.poll() || listener.m_clients != 2 || listener.m_services != 1 || listener.m_bad)
//         {
//             return false;
//         }
//     }
//     {
//         NetSenderLocalInbox inbox;
//         NetSenderServiceGatewayState state(inbox);
//         NetSenderServiceGatewayTest listener(state);
//         state.m_listener = &listener;
//         state.m_phase = NET_SERVICE_READY;
//         listener.m_throw = true;
//         if (!ServiceGatewayCreateClients(state, 2))
//         {
//             return false;
//         }
//         // 先略过活动的1，再在2的终态回调中失败；必须在本轮回收1，不能遗留到析构。
//         state.close(2, NI_CANCELLED, false);
//         state.m_polling = true;
//         state.collectClosed(now);
//         state.m_polling = false;
//         if (!state.m_clients.empty() || listener.m_clients != 2 || listener.m_bad ||
//             state.m_reason != NI_INVALID_STATE || state.poll() || listener.m_services != 1)
//         {
//             return false;
//         }
//     }
//     std::printf("Service gateway lifecycle fault_cycles=%u PASS\n", cycles * 3);
//     return true;
// }
//
// #ifndef NET_SERVICE_GATEWAY_NO_HEAP
// /** 库内服务生命周期测试，正式构建保持注释
// @return 0通过，其他为失败
// */
// int main()
// {
//     try
//     {
//         std::printf("Service gateway lifecycle starting\n");
// #if defined(_WIN32) && defined(_DEBUG)
//         _CrtMemState before;
//         _CrtMemState after;
//         _CrtMemState difference;
//         _CrtMemCheckpoint(&before);
// #endif
//         if (!NetSenderServiceGatewayTest::run())
//         {
//             return 1;
//         }
// #if defined(_WIN32) && defined(_DEBUG)
//         // 独立main在全部对象销毁后回收protobuf全局资源，不能提前预热隐藏冷启动失败。
//         google::protobuf::ShutdownProtobufLibrary();
//         _CrtMemCheckpoint(&after);
//         _CrtMemDifference(&difference, &before, &after);
//         if (difference.lCounts[_NORMAL_BLOCK] != 0 || difference.lSizes[_NORMAL_BLOCK] != 0)
//         {
//             std::printf("Service gateway heap blocks=%ld bytes=%ld FAIL\n", difference.lCounts[_NORMAL_BLOCK], difference.lSizes[_NORMAL_BLOCK]);
//             return 2;
//         }
// #endif
//         std::printf("Service gateway lifecycle PASS\n");
//         return 0;
//     }
//     catch (...)
//     {
//         return 3;
//     }
// }
// #endif
// NET_SERVICE_GATEWAY_UNIT_END

// NET_SERVICE_CLOCK_UNIT_BEGIN
// #include "NetSender.h"
// #include "NetSender/NetSenderServiceGatewayTest.h"
// #include "NetSender/NetSenderServiceGatewayState.h"
// #include "CSystem/CSystemAPI.h"
// #include <test/certs.h>
// #include <cstdio>
//
// bool NetSenderServiceGatewayTest::clockBoundary()
// {
//     NetSenderLocalInbox inbox;
//     NetSenderServiceGatewayState state(inbox);
//     state.m_phase = NET_SERVICE_READY;
//     NetInterfaceTlsOptions tls = {};
//     tls.server = 1;
//     tls.entropy = CSystem::secureRandomBytesCallback;
//     tls.handshakeTimeoutMs = 60000;
//     tls.certificate = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_crt_rsa_sha256_pem);
//     tls.certificateSize = static_cast<uint32_t>(mbedtls_test_srv_crt_rsa_sha256_pem_len);
//     tls.privateKey = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_key_rsa_pem);
//     tls.privateKeySize = static_cast<uint32_t>(mbedtls_test_srv_key_rsa_pem_len);
//     std::unique_ptr<NetSenderServiceGatewayClient> entry(new NetSenderServiceGatewayClient(tls, NI_LINK_CLIENT));
//     if (entry->m_connection.status() < 0) { return false; }
//     state.m_clients.insert(std::make_pair(UINT64_C(1), std::move(entry)));
//     NetSenderServiceGatewayClient& client = *state.m_clients.begin()->second;
//     // 模拟link.poll回调在本轮时间之后1ms更新活动，不伪装TLS已握手。
//     client.m_lastReceive = 1001;
//     const uint64_t times[] = {1000, 1001, 46000, 46001};
//     for (uint32_t index = 0; index < 4; ++index)
//     {
//         state.pump(1, client, times[index]);
//         if (client.m_closing != (index == 3) || (index == 3 && client.m_reason != NI_TIMEOUT))
//         {
//             std::printf("Service clock now=%llu last=%llu closed=%u reason=%d FAIL\n",
//                 static_cast<unsigned long long>(times[index]), static_cast<unsigned long long>(client.m_lastReceive),
//                 client.m_closing ? 1U : 0U, client.m_reason);
//             return false;
//         }
//     }
//     return true;
// }
//
// #ifndef NET_SERVICE_CLOCK_NO_MAIN
// int main()
// {
//     try
//     {
//         if (!NetSenderServiceGatewayTest::clockBoundary())
//{
//    return 1;
//}

//         std::puts("Service callback clock future/equal/timeout boundaries PASS");
//         return 0;
//     }
//     catch (...)
//     {
//         return 2;
//     }
// }
// #endif
// NET_SERVICE_CLOCK_UNIT_END