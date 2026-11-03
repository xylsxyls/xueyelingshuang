#include <ProtoMessage/ProtoMessageAPI.h>
#define NET_SERVICE_OUTGOING_NO_HEAP
#include <LibuvTcp/LibuvTcpAPI.h>
#include "NetWorkConcurrencyTestCases.h"
#include <NetWork/NetWorkAPI.h>
#include <NetSender/NetSenderAPI.h>
#include <test/certs.h>
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetWorkTestCases.h"
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_OUTGOING_NO_HEAP)
#include <crtdbg.h>
static int32_t s_outgoingRejectAfter = -1;

/** 本main逐位置持续拒绝普通堆分配，验证失败没有提交半条OPEN
@param [in] operation CRT分配操作；其他参数使用CRT原型
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 是否允许分配
*/
static int __cdecl OutgoingMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
    {
        if (s_outgoingRejectAfter == 0)
        {
            return 0;
        }
        if (s_outgoingRejectAfter > 0)
        {
            --s_outgoingRejectAfter;
        }
    }
    return 1;
}
#endif

#include "NetSender/NetSenderAPI.h"
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

NetSenderServiceOutgoingTest::NetSenderServiceOutgoingTest(NetSenderServiceOutgoingTransport& value) :
m_transport(&value),
m_mode(0),
m_ready(0),
m_closed(0),
m_controls(0),
m_records(0),
m_bad(false)
{

}

void NetSenderServiceOutgoingTest::prepare(NetSenderServiceGateway& gateway)
{
    gateway.m_state->m_phase = NET_SERVICE_READY;
    gateway.m_state->m_peer = 7;
    gateway.m_state->m_highestOutgoing = 5;
    gateway.m_state->m_nodeName = "test-node";
}

void NetSenderServiceOutgoingTest::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
{
    ++m_ready;
    m_bad = m_bad || peer != 6 || identity.m_role != NI_LINK_NODE || identity.m_name == nullptr ||
        std::strcmp(identity.m_name, "test-node") != 0 || !m_transport->poll();
    if (m_mode == 1)
    {
        throw 31;
    }
    if (m_mode == 2)
    {
        m_transport->shutdown();
    }
}

bool NetSenderServiceOutgoingTest::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    ++m_controls;
    ProtoMessage message;
    Variant channel;
    m_bad = m_bad || peer != 6 || frame.m_payload == nullptr ||
        !message.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize));
    if (message.getCode() == NI_OK)
    {
        m_bad = m_bad || !message.getKeyValue(NI_GATEWAY_CHANNEL, channel) || channel.type() != Variant::UINT64 ||
            static_cast<uint64_t>(channel) != 1;
    }
    if (frame.m_kind == NI_RESPONSE)
    {
        m_bad = m_bad || frame.m_requestId != 1;
    }
    if (m_mode == 4)
    {
        m_transport->shutdown();
    }
    if (m_mode == 5)
    {
        throw 32;
    }
    return m_mode != 3;
}

bool NetSenderServiceOutgoingTest::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    ++m_records;
    const uint8_t expected[] = {9, 0, 7, 3};
    m_bad = m_bad || peer != 6 || record.m_channel != 1 || record.m_product != 9 ||
        record.m_size != sizeof(expected) || std::memcmp(record.m_data, expected, sizeof(expected)) != 0;
    if (m_mode == 6)
    {
        throw 33;
    }
    return m_mode != 3;
}

void NetSenderServiceOutgoingTest::onLinkClosed(uint64_t peer, int32_t reason)
{
    ++m_closed;
    m_bad = m_bad || peer != 6 || reason >= 0 || m_transport->m_phase != NET_SERVICE_OUTGOING_CLOSED ||
        m_transport->m_owner->m_outgoing == m_transport || m_transport->poll() || m_transport->outputState(peer) != NI_INVALID_STATE;
    if (m_mode == 7)
    {
        throw 34;
    }
}

bool NetSenderServiceOutgoingTest::run()
{
    uint32_t checks = 0;
#define OUTGOING_CHECK(value) do { ++checks; if (!(value)) { std::printf("Outgoing failed line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); return false; } } while (0)
    NetInterfaceLinkOptions options = {};
    options.m_version = 3;
    options.m_role = NI_LINK_CLIENT;
    for (uint32_t mode = 0; mode < 3; ++mode)
    {
        NetSenderLocalInbox inbox;
        NetSenderServiceGateway gateway(inbox);
        prepare(gateway);
        NetSenderServiceOutgoingTransport transport(gateway);
        NetSenderServiceOutgoingTest listener(transport);
        listener.m_mode = mode;
        uint64_t peer = 99;
        OUTGOING_CHECK(transport.outputState(1) == NI_INVALID_STATE && !transport.poll());
        OUTGOING_CHECK(transport.connect(nullptr, 1, options, &listener, peer) == NI_INVALID_ARGUMENT && peer == 0);
        OUTGOING_CHECK(transport.connect("local", 65536, options, &listener, peer) == NI_INVALID_ARGUMENT && peer == 0);
        OUTGOING_CHECK(transport.connect("", 1, options, &listener, peer) == NI_INVALID_ARGUMENT && peer == 0);
        OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_OK && peer == 6);
        OUTGOING_CHECK(transport.poll() == (mode == 0) && listener.m_ready == 1 && !listener.m_bad);
        OUTGOING_CHECK(transport.poll() == (mode == 0) && listener.m_ready == 1);
        OUTGOING_CHECK(listener.m_closed == (mode == 1 ? 1U : 0U));
        transport.shutdown();
        transport.shutdown();
        OUTGOING_CHECK(!transport.poll() && gateway.m_state->m_outgoing == nullptr && gateway.m_state->m_outgoingClose == 0);
        OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_INVALID_ARGUMENT && peer == 0);
    }
    {
        NetSenderLocalInbox inbox;
        NetSenderServiceGateway gateway(inbox);
        prepare(gateway);
        NetSenderServiceOutgoingTransport transport(gateway);
        NetSenderServiceOutgoingTest listener(transport);
        uint64_t peer = 0;
        gateway.m_state->m_outgoingClose = 4;
        OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_INVALID_STATE && peer == 0);
        gateway.m_state->m_outgoingClose = 0;
        gateway.m_state->m_highestOutgoing = UINT64_C(0x7ffffffffffffffe);
        OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_LIMIT_EXCEEDED && peer == 0);
        gateway.m_state->m_highestOutgoing = 5;
        OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_OK && transport.poll());
        ProtoMessage request;
        request.setKeyValue(NI_GATEWAY_CHANNEL, Variant(UINT64_C(1)));
        request.setKeyValue(NI_GATEWAY_TARGET_PRODUCT, Variant(UINT64_C(9)));
        const std::string bytes = request.toString();
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_REQUEST;
        frame.m_requestId = 1;
        frame.m_messageType = NI_GATEWAY_OPEN;
        frame.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
        frame.m_payloadSize = static_cast<uint32_t>(bytes.size());
        for (uint32_t invalid = 0; invalid < 7; ++invalid)
        {
            NetInterfaceFrame bad = frame;
            if (invalid == 0) bad.m_payload = nullptr;
            if (invalid == 1) bad.m_payloadSize = 0;
            if (invalid == 2) bad.m_payloadSize = 16385;
            if (invalid == 3) bad.m_productId = 9;
            if (invalid == 4) bad.m_requestId = 2;
            if (invalid == 5) bad.m_kind = NI_EVENT;
            if (invalid == 6) bad.m_messageType = NI_GATEWAY_CREDIT;
            OUTGOING_CHECK(transport.sendControl(peer, bad) == NI_INVALID_ARGUMENT && transport.m_phase == NET_SERVICE_OUTGOING_ANNOUNCED);
        }
        OUTGOING_CHECK(transport.sendControl(peer + 1, frame) == NI_INVALID_STATE);
        OUTGOING_CHECK(transport.sendControl(peer, frame) == NI_INVALID_STATE && transport.m_product == 0);
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_OUTGOING_NO_HEAP)
        for (uint32_t step = 0; step < 40; ++step)
        {
            s_outgoingRejectAfter = static_cast<int32_t>(step);
            const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(OutgoingMemoryHook);
            int32_t result = NI_OK;
            bool escaped = false;
            try { result = transport.sendControl(peer, frame); }
            catch (...) { escaped = true; }
            _CrtSetAllocHook(previous);
            s_outgoingRejectAfter = -1;
            OUTGOING_CHECK(!escaped && result < 0 && transport.m_phase == NET_SERVICE_OUTGOING_ANNOUNCED && transport.m_product == 0);
            OUTGOING_CHECK(step != 0 || result == NI_NO_MEMORY);
        }
#endif
        transport.shutdown();
        OUTGOING_CHECK(gateway.m_state->m_outgoingClose == 0);
    }
    for (uint32_t mode = 0; mode < 8; ++mode)
    {
        NetSenderLocalInbox inbox;
        NetSenderServiceGateway gateway(inbox);
        prepare(gateway);
        NetSenderServiceOutgoingTransport transport(gateway);
        NetSenderServiceOutgoingTest listener(transport);
        uint64_t peer = 0;
        OUTGOING_CHECK(transport.connect("local", 1, options, &listener, peer) == NI_OK && transport.poll());
        // 本白盒直接进入OPEN等待，以独立验证响应映射；真实请求提交在进程集成测试验证。
        transport.m_phase = NET_SERVICE_OUTGOING_OPEN_WAIT;
        transport.m_product = 9;
        listener.m_mode = mode;
        ProtoMessage reply;
        reply.setKeyValue(NI_GATEWAY_CHANNEL, Variant(peer));
        const std::string bytes = reply.toString();
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_RESPONSE;
        frame.m_requestId = peer + 1;
        frame.m_messageType = NI_GATEWAY_OPEN;
        frame.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
        frame.m_payloadSize = static_cast<uint32_t>(bytes.size());
        const bool accepted = transport.control(frame);
        OUTGOING_CHECK(accepted == (mode != 3) && !listener.m_bad && listener.m_controls == 1);
        if (mode == 3)
        {
            OUTGOING_CHECK(transport.m_phase == NET_SERVICE_OUTGOING_OPEN_WAIT);
            listener.m_mode = 0;
            OUTGOING_CHECK(transport.control(frame) && transport.m_phase == NET_SERVICE_OUTGOING_OPEN);
            listener.m_mode = 3;
        }
        if (mode == 4 || mode == 5)
        {
            OUTGOING_CHECK(transport.m_phase == NET_SERVICE_OUTGOING_CLOSED && gateway.m_state->m_outgoingClose == peer);
            OUTGOING_CHECK(listener.m_closed == (mode == 5 ? 1U : 0U));
        }
        else
        {
            OUTGOING_CHECK(transport.m_phase == NET_SERVICE_OUTGOING_OPEN);
            OUTGOING_CHECK(transport.outputState(peer) == gateway.m_state->m_link.outputState(7));
            const uint8_t payload[] = {9, 0, 7, 3};
            NetInterfaceTunnelRecord record = {};
            record.m_kind = NI_TUNNEL_BYTES;
            record.m_channel = peer;
            record.m_product = 9;
            record.m_data = payload;
            record.m_size = sizeof(payload);
            OUTGOING_CHECK(transport.record(record) == (mode != 3) && listener.m_records == 1 && !listener.m_bad);
            if (mode == 6)
            {
                OUTGOING_CHECK(transport.m_phase == NET_SERVICE_OUTGOING_CLOSED && listener.m_closed == 1 && gateway.m_state->m_outgoingClose == peer);
            }
            else
            {
                record.m_channel = 1;
                record.m_data = nullptr;
                OUTGOING_CHECK(transport.sendRecord(peer, record) == NI_INVALID_ARGUMENT);
                transport.closed(NI_TIMEOUT);
                transport.closed(NI_TRUNCATED);
                OUTGOING_CHECK(listener.m_closed == 1 && gateway.m_state->m_outgoingClose == 0 && !listener.m_bad);
            }
        }
        transport.shutdown();
        OUTGOING_CHECK(!transport.poll() && gateway.m_state->m_outgoing == nullptr);
    }
    std::printf("Service outgoing checks=%u PASS\n", checks);
#undef OUTGOING_CHECK
    return true;
}