#include "NetSenderMacro.h"
#include "NetSenderServiceOutgoingTransport.h"
#include "NetSenderServiceGatewayState.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"

NetSenderServiceOutgoingTransport::NetSenderServiceOutgoingTransport(NetSenderServiceGateway& gateway) :
m_owner(gateway.m_state),
m_listener(nullptr),
m_channel(0),
m_request(0),
m_product(0),
m_phase(NET_SERVICE_OUTGOING_IDLE),
m_polling(false)
{

}

NetSenderServiceOutgoingTransport::~NetSenderServiceOutgoingTransport()
{
    shutdown();
}

int32_t NetSenderServiceOutgoingTransport::connect(const char* address, uint32_t port,
    const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (m_phase != NET_SERVICE_OUTGOING_IDLE || listener == nullptr || address == nullptr || address[0] == 0 ||
        port == 0 || port > 65535 || options.m_version != 3 || options.m_role != NI_LINK_CLIENT || options.m_tls.server != 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_owner->m_phase != NET_SERVICE_READY || m_owner->m_outgoing != nullptr || m_owner->m_outgoingClose != 0)
    {
        return NI_INVALID_STATE;
    }
    if (m_owner->m_highestOutgoing >= UINT64_C(0x7ffffffffffffffe))
    {
        return NI_LIMIT_EXCEEDED;
    }
    m_channel = ++m_owner->m_highestOutgoing;
    m_request = m_channel + 1;
    m_owner->m_outgoing = this;
    m_listener = listener;
    m_phase = NET_SERVICE_OUTGOING_ATTACHED;
    peer = m_channel;
    return NI_OK;
}

bool NetSenderServiceOutgoingTransport::poll()
{
    if (m_listener == nullptr)
    {
        return false;
    }
    if (m_polling)
    {
        return !m_owner->closing();
    }
    m_polling = true;
    if (m_owner->closing())
    {
        closed(m_owner->m_reason);
    }
    else if (m_phase == NET_SERVICE_OUTGOING_ATTACHED)
    {
        // 先提交通知阶段，回调可同步提交OPEN或停止本通道。
        m_phase = NET_SERVICE_OUTGOING_ANNOUNCED;
        NetInterfaceLinkPeer identity = {};
        identity.m_role = NI_LINK_NODE;
        identity.m_name = m_owner->m_nodeName.c_str();
        identity.m_certificate = reinterpret_cast<const uint8_t*>(m_owner->m_nodeCertificate.data());
        identity.m_certificateSize = static_cast<uint32_t>(m_owner->m_nodeCertificate.size());
        try
        {
            m_listener->onLinkReady(m_channel, identity);
        }
        catch (...)
        {
            closed(NI_INVALID_STATE, true);
        }
    }
    m_polling = false;
    return m_listener != nullptr;
}

int32_t NetSenderServiceOutgoingTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != m_channel || m_listener == nullptr || m_owner->closing() ||
        (m_phase != NET_SERVICE_OUTGOING_ANNOUNCED && m_phase != NET_SERVICE_OUTGOING_OPEN))
    {
        return NI_INVALID_STATE;
    }
    const bool opening = frame.m_kind == NI_REQUEST && frame.m_messageType == NI_GATEWAY_OPEN && frame.m_requestId == 1 &&
        m_phase == NET_SERVICE_OUTGOING_ANNOUNCED;
    const bool credit = frame.m_kind == NI_EVENT && frame.m_messageType == NI_GATEWAY_CREDIT && frame.m_requestId == 0 &&
        m_phase == NET_SERVICE_OUTGOING_OPEN;
    if ((!opening && !credit) || frame.m_productId != 0 || frame.m_payload == nullptr || frame.m_payloadSize == 0 || frame.m_payloadSize > 16384)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        ProtoMessage message;
        if (!message.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)))
        {
            return NI_INVALID_ARGUMENT;
        }
        Variant channel;
        if (!message.getKeyValue(NI_GATEWAY_CHANNEL, channel) || channel.type() != Variant::UINT64 || static_cast<uint64_t>(channel) != 1)
        {
            return NI_INVALID_ARGUMENT;
        }
        uint32_t product = m_product;
        if (opening)
        {
            Variant target;
            if (!message.getKeyValue(NI_GATEWAY_TARGET_PRODUCT, target) || target.type() != Variant::UINT64 ||
                static_cast<uint64_t>(target) > UINT32_MAX)
            {
                return NI_INVALID_ARGUMENT;
            }
            product = static_cast<uint32_t>(static_cast<uint64_t>(target));
        }
        message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(m_channel));
        std::string bytes;
        if (!message.toString(bytes))
        {
            return NI_PROTOCOL_ERROR;
        }
        NetInterfaceFrame mapped = frame;
        if (opening)
        {
            mapped.m_requestId = m_request;
        }
        mapped.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
        mapped.m_payloadSize = static_cast<uint32_t>(bytes.size());
        const int32_t result = m_owner->m_link.sendControl(m_owner->m_peer, mapped);
        if (result == NI_OK && opening)
        {
            m_product = product;
            m_phase = NET_SERVICE_OUTGOING_OPEN_WAIT;
        }
        return result;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderServiceOutgoingTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (peer != m_channel || m_listener == nullptr || m_phase != NET_SERVICE_OUTGOING_OPEN || m_owner->closing())
    {
        return NI_INVALID_STATE;
    }
    if (record.m_channel != 1 || record.m_product != m_product || record.m_kind != NI_TUNNEL_BYTES ||
        record.m_data == nullptr || record.m_size == 0 || record.m_size > NET_INTERFACE_TUNNEL_PAYLOAD_LIMIT)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceTunnelRecord mapped = record;
    mapped.m_channel = m_channel;
    return m_owner->m_link.sendRecord(m_owner->m_peer, mapped);
}

bool NetSenderServiceOutgoingTransport::control(const NetInterfaceFrame& frame)
{
    if (m_listener == nullptr)
    {
        return true;
    }
    const bool opening = frame.m_kind == NI_RESPONSE && frame.m_requestId == m_request && frame.m_messageType == NI_GATEWAY_OPEN &&
        m_phase == NET_SERVICE_OUTGOING_OPEN_WAIT;
    const bool control = frame.m_kind == NI_EVENT && frame.m_requestId == 0 && m_phase == NET_SERVICE_OUTGOING_OPEN &&
        (frame.m_messageType == NI_GATEWAY_CREDIT || frame.m_messageType == NI_GATEWAY_CHANNEL_CLOSE);
    if ((!opening && !control) || frame.m_productId != 0 || frame.m_payload == nullptr || frame.m_payloadSize == 0 || frame.m_payloadSize > 16384)
    {
        m_owner->fail(NI_PROTOCOL_ERROR);
        return true;
    }
    try
    {
        ProtoMessage message;
        if (!message.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)))
        {
            m_owner->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        const bool opened = opening && message.getCode() == NI_OK;
        if (!opening || opened)
        {
            Variant channel;
            if (!message.getKeyValue(NI_GATEWAY_CHANNEL, channel) || channel.type() != Variant::UINT64 || static_cast<uint64_t>(channel) != m_channel)
            {
                m_owner->fail(NI_PROTOCOL_ERROR);
                return true;
            }
            message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(UINT64_C(1)));
        }
        std::string bytes;
        if (!message.toString(bytes))
        {
            m_owner->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        NetInterfaceFrame mapped = frame;
        if (opening)
        {
            mapped.m_requestId = 1;
        }
        mapped.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
        mapped.m_payloadSize = static_cast<uint32_t>(bytes.size());
        bool accepted = false;
        try
        {
            accepted = m_listener->onLinkControl(m_channel, mapped);
        }
        catch (...)
        {
            closed(NI_INVALID_STATE, true);
            return true;
        }
        if (accepted && opened && m_phase == NET_SERVICE_OUTGOING_OPEN_WAIT)
        {
            m_phase = NET_SERVICE_OUTGOING_OPEN;
        }
        return accepted;
    }
    catch (...)
    {
        m_owner->fail(NI_NO_MEMORY);
        return true;
    }
}

bool NetSenderServiceOutgoingTransport::record(const NetInterfaceTunnelRecord& record)
{
    if (m_listener == nullptr)
    {
        return true;
    }
    if (m_phase != NET_SERVICE_OUTGOING_OPEN || record.m_channel != m_channel || record.m_product != m_product ||
        record.m_data == nullptr || record.m_size == 0 || record.m_size > NET_INTERFACE_TUNNEL_PAYLOAD_LIMIT ||
        (record.m_kind != NI_TUNNEL_BYTES && (record.m_kind != NI_TUNNEL_CLOSE || record.m_size != 4)))
    {
        m_owner->fail(NI_PROTOCOL_ERROR);
        return true;
    }
    if (record.m_kind == NI_TUNNEL_CLOSE)
    {
        closed(NI_TRUNCATED);
        return true;
    }
    NetInterfaceTunnelRecord mapped = record;
    mapped.m_channel = 1;
    try
    {
        return m_listener->onLinkRecord(m_channel, mapped);
    }
    catch (...)
    {
        closed(NI_INVALID_STATE, true);
        return true;
    }
}

void NetSenderServiceOutgoingTransport::detach(bool notifyRemote)
{
    const bool pending = m_phase == NET_SERVICE_OUTGOING_OPEN_WAIT || m_phase == NET_SERVICE_OUTGOING_OPEN;
    m_phase = NET_SERVICE_OUTGOING_CLOSED;
    m_listener = nullptr;
    if (m_owner->m_outgoing == this)
    {
        m_owner->m_outgoing = nullptr;
        if (notifyRemote && pending && !m_owner->closing())
        {
            m_owner->m_outgoingClose = m_channel;
            m_owner->m_outgoingCloseSince = 0;
        }
    }
}

void NetSenderServiceOutgoingTransport::closed(int32_t reason, bool notifyRemote)
{
    NetLinkTransportListener* listener = m_listener;
    detach(notifyRemote);
    if (listener != nullptr)
    {
        try
        {
            listener->onLinkClosed(m_channel, reason < 0 ? reason : NI_TRUNCATED);
        }
        catch (...)
        {
            // 终态已提交且借用已解除，回调失败不阻止所属服务继续回收。
        }
    }
}

void NetSenderServiceOutgoingTransport::shutdown()
{
    if (m_phase != NET_SERVICE_OUTGOING_IDLE)
    {
        detach(true);
    }
}

int32_t NetSenderServiceOutgoingTransport::outputState(uint64_t peer) const
{
    if (peer != m_channel || m_listener == nullptr || m_owner->closing() ||
        m_phase < NET_SERVICE_OUTGOING_ANNOUNCED || m_phase >= NET_SERVICE_OUTGOING_CLOSED)
    {
        return NI_INVALID_STATE;
    }
    return m_owner->m_link.outputState(m_owner->m_peer);
}

// NET_SERVICE_OUTGOING_UNIT_BEGIN
// #include "NetSender/NetSenderServiceOutgoingTest.h"
// #include "NetSender/NetSenderServiceGatewayState.h"
// #include "ProtoMessage/ProtoMessageAPI.h"
// #include <google/protobuf/stubs/common.h>
// #include <cstdio>
// #include <cstring>
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_OUTGOING_NO_HEAP)
// #include <crtdbg.h>
// static int32_t s_outgoingRejectAfter = -1;
// 
// /** 本main逐位置持续拒绝普通堆分配，验证失败没有提交半条OPEN
// @param [in] operation CRT分配操作；其他参数使用CRT原型
// @return 是否允许分配
// */
// static int __cdecl OutgoingMemoryHook(int operation, void*, size_t, int block, long, const unsigned char*, int)
// {
//     if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
//     {
//         if (s_outgoingRejectAfter == 0)
//         {
//             return 0;
//         }
//         if (s_outgoingRejectAfter > 0)
//         {
//             --s_outgoingRejectAfter;
//         }
//     }
//     return 1;
// }
// #endif
// 
// NetSenderServiceOutgoingTest::NetSenderServiceOutgoingTest(NetSenderServiceOutgoingTransport& value) :
// m_transport(&value),
// m_mode(0),
// m_ready(0),
// m_closed(0),
// m_controls(0),
// m_records(0),
// m_bad(false)
// {
// 
// }
// 
// void NetSenderServiceOutgoingTest::prepare(NetSenderServiceGateway& gateway)
// {
//     gateway.m_state->m_phase = NET_SERVICE_READY;
//     gateway.m_state->m_peer = 7;
//     gateway.m_state->m_highestOutgoing = 5;
//     gateway.m_state->m_nodeName = "test-node";
// }
// 
// void NetSenderServiceOutgoingTest::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
// {
//     ++m_ready;
//     m_bad = m_bad || peer != 6 || identity.m_role != NI_LINK_NODE || identity.m_name == nullptr ||
//         std::strcmp(identity.m_name, "test-node") != 0 || !m_transport->poll();
//     if (m_mode == 1)
//     {
//         throw 31;
//     }
//     if (m_mode == 2)
//     {
//         m_transport->shutdown();
//     }
// }
// 
// bool NetSenderServiceOutgoingTest::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
// {
//     ++m_controls;
//     ProtoMessage message;
//     Variant channel;
//     m_bad = m_bad || peer != 6 || frame.m_payload == nullptr ||
//         !message.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize));
//     if (message.getCode() == NI_OK)
//     {
//         m_bad = m_bad || !message.getKeyValue(NI_GATEWAY_CHANNEL, channel) || channel.type() != Variant::UINT64 ||
//             static_cast<uint64_t>(channel) != 1;
//     }
//     if (frame.m_kind == NI_RESPONSE)
//     {
//         m_bad = m_bad || frame.m_requestId != 1;
//     }
//     if (m_mode == 4)
//     {
//         m_transport->shutdown();
//     }
//     if (m_mode == 5)
//     {
//         throw 32;
//     }
//     return m_mode != 3;
// }
// 
// bool NetSenderServiceOutgoingTest::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
// {
//     ++m_records;
//     const uint8_t expected[] = {9, 0, 7, 3};
//     m_bad = m_bad || peer != 6 || record.m_channel != 1 || record.m_product != 9 ||
//         record.m_size != sizeof(expected) || std::memcmp(record.m_data, expected, sizeof(expected)) != 0;
//     if (m_mode == 6)
//     {
//         throw 33;
//     }
//     return m_mode != 3;
// }
// 
// void NetSenderServiceOutgoingTest::onLinkClosed(uint64_t peer, int32_t reason)
// {
//     ++m_closed;
//     m_bad = m_bad || peer != 6 || reason >= 0 || m_transport->m_phase != NET_SERVICE_OUTGOING_CLOSED ||
//         m_transport->m_owner->m_outgoing == m_transport || m_transport->poll() || m_transport->outputState(peer) != NI_INVALID_STATE;
//     if (m_mode == 7)
//     {
//         throw 34;
//     }
// }
// 
// bool NetSenderServiceOutgoingTest::run()
// {
//     uint32_t checks = 0;
// #define OUTGOING_CHECK(value) do { ++checks; if (!(value)) { std::printf("Outgoing failed line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); return false; } } while (0)
//     NetInterfaceLinkOptions options = {};
//     options.m_version = 3;
//     options.m_role = NI_LINK_CLIENT;
//     for (uint32_t mode = 0; mode < 3; ++mode)
//     {
//         NetSenderLocalInbox inbox;
//         NetSenderServiceGateway gateway(inbox);
//         prepare(gateway);
//         NetSenderServiceOutgoingTransport transport(gateway);
//         NetSenderServiceOutgoingTest listener(transport);
//         listener.m_mode = mode;
//         uint64_t peer = 99;
//         OUTGOING_CHECK(transport.outputState(1) == NI_INVALID_STATE && !transport.poll());
//         OUTGOING_CHECK(transport.connect(nullptr, 1, options, &listener, peer) == NI_INVALID_ARGUMENT && peer == 0);
//         OUTGOING_CHECK(transport.connect("local", 65536, options, &listener, peer) == NI_INVALID_ARGUMENT && peer == 0);
//         OUTGOING_CHECK(transport.connect("", 1, options, &listener, peer) == NI_INVALID_ARGUMENT && peer == 0);
//         OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_OK && peer == 6);
//         OUTGOING_CHECK(transport.poll() == (mode == 0) && listener.m_ready == 1 && !listener.m_bad);
//         OUTGOING_CHECK(transport.poll() == (mode == 0) && listener.m_ready == 1);
//         OUTGOING_CHECK(listener.m_closed == (mode == 1 ? 1U : 0U));
//         transport.shutdown();
//         transport.shutdown();
//         OUTGOING_CHECK(!transport.poll() && gateway.m_state->m_outgoing == nullptr && gateway.m_state->m_outgoingClose == 0);
//         OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_INVALID_ARGUMENT && peer == 0);
//     }
//     {
//         NetSenderLocalInbox inbox;
//         NetSenderServiceGateway gateway(inbox);
//         prepare(gateway);
//         NetSenderServiceOutgoingTransport transport(gateway);
//         NetSenderServiceOutgoingTest listener(transport);
//         uint64_t peer = 0;
//         gateway.m_state->m_outgoingClose = 4;
//         OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_INVALID_STATE && peer == 0);
//         gateway.m_state->m_outgoingClose = 0;
//         gateway.m_state->m_highestOutgoing = UINT64_C(0x7ffffffffffffffe);
//         OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_LIMIT_EXCEEDED && peer == 0);
//         gateway.m_state->m_highestOutgoing = 5;
//         OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_OK && transport.poll());
//         ProtoMessage request;
//         request.setKeyValue(NI_GATEWAY_CHANNEL, Variant(UINT64_C(1)));
//         request.setKeyValue(NI_GATEWAY_TARGET_PRODUCT, Variant(UINT64_C(9)));
//         const std::string bytes = request.toString();
//         NetInterfaceFrame frame = {};
//         frame.m_kind = NI_REQUEST;
//         frame.m_requestId = 1;
//         frame.m_messageType = NI_GATEWAY_OPEN;
//         frame.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
//         frame.m_payloadSize = static_cast<uint32_t>(bytes.size());
//         for (uint32_t invalid = 0; invalid < 7; ++invalid)
//         {
//             NetInterfaceFrame bad = frame;
//             if (invalid == 0) bad.m_payload = nullptr;
//             if (invalid == 1) bad.m_payloadSize = 0;
//             if (invalid == 2) bad.m_payloadSize = 16385;
//             if (invalid == 3) bad.m_productId = 9;
//             if (invalid == 4) bad.m_requestId = 2;
//             if (invalid == 5) bad.m_kind = NI_EVENT;
//             if (invalid == 6) bad.m_messageType = NI_GATEWAY_CREDIT;
//             OUTGOING_CHECK(transport.sendControl(peer, bad) == NI_INVALID_ARGUMENT && transport.m_phase == NET_SERVICE_OUTGOING_ANNOUNCED);
//         }
//         OUTGOING_CHECK(transport.sendControl(peer + 1, frame) == NI_INVALID_STATE);
//         OUTGOING_CHECK(transport.sendControl(peer, frame) == NI_INVALID_STATE && transport.m_product == 0);
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_OUTGOING_NO_HEAP)
//         for (uint32_t step = 0; step < 40; ++step)
//         {
//             s_outgoingRejectAfter = static_cast<int32_t>(step);
//             const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(OutgoingMemoryHook);
//             int32_t result = NI_OK;
//             bool escaped = false;
//             try { result = transport.sendControl(peer, frame); }
//             catch (...) { escaped = true; }
//             _CrtSetAllocHook(previous);
//             s_outgoingRejectAfter = -1;
//             OUTGOING_CHECK(!escaped && result < 0 && transport.m_phase == NET_SERVICE_OUTGOING_ANNOUNCED && transport.m_product == 0);
//             OUTGOING_CHECK(step != 0 || result == NI_NO_MEMORY);
//         }
// #endif
//         transport.shutdown();
//         OUTGOING_CHECK(gateway.m_state->m_outgoingClose == 0);
//     }
//     for (uint32_t mode = 0; mode < 8; ++mode)
//     {
//         NetSenderLocalInbox inbox;
//         NetSenderServiceGateway gateway(inbox);
//         prepare(gateway);
//         NetSenderServiceOutgoingTransport transport(gateway);
//         NetSenderServiceOutgoingTest listener(transport);
//         uint64_t peer = 0;
//         OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_OK && transport.poll());
//         // 本白盒直接进入OPEN等待，以独立验证响应映射；真实请求提交在进程集成测试验证。
//         transport.m_phase = NET_SERVICE_OUTGOING_OPEN_WAIT;
//         transport.m_product = 9;
//         listener.m_mode = mode;
//         ProtoMessage reply;
//         reply.setKeyValue(NI_GATEWAY_CHANNEL, Variant(peer));
//         const std::string bytes = reply.toString();
//         NetInterfaceFrame frame = {};
//         frame.m_kind = NI_RESPONSE;
//         frame.m_requestId = peer + 1;
//         frame.m_messageType = NI_GATEWAY_OPEN;
//         frame.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
//         frame.m_payloadSize = static_cast<uint32_t>(bytes.size());
//         const bool accepted = transport.control(frame);
//         OUTGOING_CHECK(accepted == (mode != 3) && !listener.m_bad && listener.m_controls == 1);
//         if (mode == 3)
//         {
//             OUTGOING_CHECK(transport.m_phase == NET_SERVICE_OUTGOING_OPEN_WAIT);
//             listener.m_mode = 0;
//             OUTGOING_CHECK(transport.control(frame) && transport.m_phase == NET_SERVICE_OUTGOING_OPEN);
//             listener.m_mode = 3;
//         }
//         if (mode == 4 || mode == 5)
//         {
//             OUTGOING_CHECK(transport.m_phase == NET_SERVICE_OUTGOING_CLOSED && gateway.m_state->m_outgoingClose == peer);
//             OUTGOING_CHECK(listener.m_closed == (mode == 5 ? 1U : 0U));
//         }
//         else
//         {
//             OUTGOING_CHECK(transport.m_phase == NET_SERVICE_OUTGOING_OPEN);
//             OUTGOING_CHECK(transport.outputState(peer) == gateway.m_state->m_link.outputState(7));
//             const uint8_t payload[] = {9, 0, 7, 3};
//             NetInterfaceTunnelRecord record = {};
//             record.m_kind = NI_TUNNEL_BYTES;
//             record.m_channel = peer;
//             record.m_product = 9;
//             record.m_data = payload;
//             record.m_size = sizeof(payload);
//             OUTGOING_CHECK(transport.record(record) == (mode != 3) && listener.m_records == 1 && !listener.m_bad);
//             if (mode == 6)
//             {
//                 OUTGOING_CHECK(transport.m_phase == NET_SERVICE_OUTGOING_CLOSED && listener.m_closed == 1 && gateway.m_state->m_outgoingClose == peer);
//             }
//             else
//             {
//                 record.m_channel = 1;
//                 record.m_data = nullptr;
//                 OUTGOING_CHECK(transport.sendRecord(peer, record) == NI_INVALID_ARGUMENT);
//                 transport.closed(NI_TIMEOUT);
//                 transport.closed(NI_TRUNCATED);
//                 OUTGOING_CHECK(listener.m_closed == 1 && gateway.m_state->m_outgoingClose == 0 && !listener.m_bad);
//             }
//         }
//         transport.shutdown();
//         OUTGOING_CHECK(!transport.poll() && gateway.m_state->m_outgoing == nullptr);
//     }
//     std::printf("Service outgoing checks=%u PASS\n", checks);
// #undef OUTGOING_CHECK
//     return true;
// }
// 
// #ifndef NET_SERVICE_OUTGOING_NO_HEAP
// /** 库内独立测试main，正式构建保持逐行注释
// @return 0通过，其他为失败
// */
// int main()
// {
//     try
//     {
// #if defined(_WIN32) && defined(_DEBUG)
//         _CrtMemState before, after;
//         _CrtMemCheckpoint(&before);
// #endif
//         if (!NetSenderServiceOutgoingTest::run()) return 1;
// #if defined(_WIN32) && defined(_DEBUG)
//         google::protobuf::ShutdownProtobufLibrary();
//         _CrtMemCheckpoint(&after);
//         const long long blocks = static_cast<long long>(after.lCounts[_NORMAL_BLOCK]) - before.lCounts[_NORMAL_BLOCK];
//         const long long bytes = static_cast<long long>(after.lSizes[_NORMAL_BLOCK]) - before.lSizes[_NORMAL_BLOCK];
//         if (blocks != 0 || bytes != 0)
//         {
//             std::printf("Outgoing heap blocks=%lld bytes=%lld FAIL\n", blocks, bytes);
//             return 2;
//         }
// #endif
//         std::printf("Service outgoing boundary and lifecycle PASS\n");
//         return 0;
//     }
//     catch (...)
//     {
//         return 3;
//     }
// }
// #endif
// NET_SERVICE_OUTGOING_UNIT_END