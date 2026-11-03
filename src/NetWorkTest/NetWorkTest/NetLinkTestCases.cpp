#include "NetLinkTestCases.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetInterfaceTlsTestCases.h"

void NetLinkTestCases::creditWindow()
{
    NetInterfaceCredit sender;
    NetInterfaceCredit receiver;
    for (uint32_t index = 0; index < 4; ++index)
    {
        NetInterfaceTlsTestPair::require(sender.sent(16384) == NI_OK && receiver.received(16384) == NI_OK,
            "initial per-channel window rejected legal bytes");
    }
    NetInterfaceTlsTestPair::require(sender.available() == 0 && sender.sent(1) == NI_LIMIT_EXCEEDED && sender.status() == NI_OK &&
        sender.available() == 0 && receiver.pending() == 65536, "full window is unbounded or rejection changed accounting");
    NetInterfaceTlsTestPair::require(receiver.consume(32768) == NI_OK && receiver.pending() == 32768 && sender.grant(32768) == NI_OK &&
        sender.available() == 32768, "real receiver grant did not restore exactly consumed bytes");
    NetInterfaceTlsTestPair::require(sender.sent(16384) == NI_OK && receiver.received(16384) == NI_OK &&
        sender.sent(16384) == NI_OK && receiver.received(16384) == NI_OK && receiver.pending() == 65536,
        "window did not continue after credit replenishment");
    NetInterfaceTlsTestPair::require(receiver.received(1) == NI_PROTOCOL_ERROR && receiver.consume(1) == NI_PROTOCOL_ERROR &&
        receiver.status() == NI_PROTOCOL_ERROR && receiver.pending() == 0, "receiver allowed unbounded hidden buffering or recovered after violation");
    NetInterfaceCreditHandle handle = nullptr;
    NetInterfaceTlsTestPair::require(NetInterface::creditCreate(nullptr) == NI_INVALID_ARGUMENT &&
        NetInterface::creditCreate(&handle) == NI_OK && handle != nullptr, "credit C++ entry create invalid");
    try
    {
        uint32_t fields[4] = {0xaabbccdd, 99, 99, 0x12345678};
        NetInterfaceTlsTestPair::require(NetInterface::creditGet(handle, fields + 1) == NI_OK && fields[0] == 0xaabbccdd &&
            fields[1] == 65536 && fields[2] == 0 && fields[3] == 0x12345678 && NetInterface::creditSent(handle, 16384) == NI_OK &&
            NetInterface::creditGrant(handle, 16384) == NI_OK, "credit C++ entry bounds or legal grant failed");
        NetInterfaceTlsTestPair::require(NetInterface::creditGrant(handle, 1) == NI_PROTOCOL_ERROR &&
            NetInterface::creditGet(handle, fields + 1) == NI_PROTOCOL_ERROR && fields[1] == 0 && fields[2] == 0 &&
            NetInterface::creditGrant(handle, 16384) == NI_PROTOCOL_ERROR && NetInterface::creditSent(handle, 1) == NI_PROTOCOL_ERROR,
            "forged/duplicate grant expanded the sender window or failed to remain terminal");
    }
    catch (...) { NetInterface::creditDestroy(handle); throw; }
    NetInterface::creditDestroy(handle);
    NetInterface::creditDestroy(nullptr);
}

void NetLinkTestCases::portableLink()
{
    NetInterfaceLinkOptions options = {};
    options.m_role = NI_LINK_CLIENT;
    options.m_tls = NetInterfaceTlsTestPair::options(false, "ignored.invalid");
    NetInterfaceLinkHandle link = nullptr;
    NetInterfaceTlsTestPair::require(NetInterface::linkCreate(&options, &link) == NI_INVALID_ARGUMENT && link == nullptr,
        "v1 or missing link version accepted as v3");
    options.m_version = 3;
    options.m_tls.handshakeTimeoutMs = 100;
    NetInterfaceTlsTestPair::require(NetInterface::linkCreate(&options, &link) == NI_NEED_MORE && link != nullptr,
        "create portable fixed-root link failed");
    uint32_t fields[5] = {};
    const uint8_t* payload = nullptr;
    NetInterfaceTlsTestPair::require(NetInterface::linkPeekOutput(link, fields, &payload) == NI_READY &&
        fields[0] == NI_TUNNEL_HELLO && fields[4] == 0 && payload == nullptr,
        "link does not start with explicit outer protocol negotiation");
    NetInterfaceTlsTestPair::require(NetInterface::linkConsumeOutput(link) == NI_OK,
        "link outer negotiation not consumed");
    const uint8_t bytes[] = {0x00, 0xff};
    const uint32_t earlyData[] = {NI_TUNNEL_BYTES, 7, 1, 0, sizeof(bytes)};
    NetInterfaceTlsTestPair::require(NetInterface::linkSendRecord(link, earlyData, bytes) == NI_INVALID_STATE,
        "unauthenticated business record was sent");
    NetInterfaceTlsTestPair::require(NetInterface::linkProgress(link, 1000, 0) == NI_NEED_MORE &&
        NetInterface::linkProgress(link, 1100, 0) == NI_TIMEOUT && NetInterface::linkStatus(link) == NI_TIMEOUT,
        "management deadline was extended or ignored");
    NetInterface::linkDestroy(link);
    link = nullptr;
    NetInterfaceTlsTestPair::require(NetInterface::linkCreate(&options, &link) == NI_NEED_MORE &&
        NetInterface::linkProgress(link, 1000, 0) == NI_NEED_MORE && NetInterface::linkProgress(link, 999, 0) == NI_INVALID_ARGUMENT,
        "management clock regression accepted");
    NetInterface::linkDestroy(link);
    link = nullptr;
    NetInterfaceTlsTestPair::require(NetInterface::linkCreate(&options, &link) == NI_NEED_MORE,
        "create duplicate negotiation test failed");
    const uint32_t hello[] = {NI_TUNNEL_HELLO, 0, 0, 0, 0};
    NetInterfaceTlsTestPair::require(NetInterface::linkAccept(link, hello, nullptr) == NI_OK &&
        NetInterface::linkAccept(link, hello, nullptr) == NI_PROTOCOL_ERROR &&
        NetInterface::linkFinishInput(link) == NI_PROTOCOL_ERROR, "duplicate negotiation or persistent error accepted");
    NetInterface::linkDestroy(link);
    link = nullptr;
    NetInterfaceTlsTestPair::require(NetInterface::linkCreate(&options, &link) == NI_NEED_MORE &&
        NetInterface::linkPeekOutput(link, fields, &payload) == NI_READY && fields[0] == NI_TUNNEL_HELLO &&
        NetInterface::linkConsumeOutput(link) == NI_OK && NetInterface::linkPeekOutput(link, fields, &payload) >= 0 &&
        NetInterface::linkProgress(link, 1000000, 0) == NI_NEED_MORE &&
        NetInterface::linkProgress(link, 1000100, 0) == NI_TIMEOUT,
        "initial output probe started a zero-based TLS deadline");
    NetInterface::linkDestroy(link);
    link = nullptr;
    options.m_tls.entropy = NetInterfaceTlsTestPair::failedRandom;
    NetInterfaceTlsTestPair::require(NetInterface::linkCreate(&options, &link) < 0 && link == nullptr,
        "failed entropy created a usable management link");
    uint32_t peerFields[4] = {1,1,1,1};
    const uint8_t* name = bytes;
    const uint8_t* certificate = bytes;
    const uint8_t* claims = bytes;
    NetInterfaceTlsTestPair::require(NetInterface::linkPeerInfo(nullptr, peerFields, &name, &certificate, &claims) == NI_INVALID_ARGUMENT &&
        peerFields[0] == 0 && name == nullptr && certificate == nullptr && claims == nullptr,
        "invalid link handle left stale identity output");
}

void NetLinkTestCases::negotiatedCredit()
{
    NetInterfaceCredit credit;
    NetInterfaceTlsTestPair::require(credit.configure(1048576) == NI_OK && credit.capacity() == 1048576 &&
        credit.configure(65535) == NI_INVALID_ARGUMENT && credit.configure(1048577) == NI_INVALID_ARGUMENT &&
        credit.configure(65537) == NI_INVALID_ARGUMENT && credit.available() == 1048576, "invalid negotiated window changed capacity");
    for (uint32_t i = 0; i < 64; ++i)
    {
        NetInterfaceTlsTestPair::require(credit.sent(16384) == NI_OK && credit.received(16384) == NI_OK,
            "negotiated window still restricted to 64 KiB");
    }
    NetInterfaceTlsTestPair::require(credit.sent(1) == NI_LIMIT_EXCEEDED && credit.configure(65536) == NI_INVALID_STATE &&
        credit.consume(1048576) == NI_OK && credit.grant(1048576) == NI_OK && credit.available() == 1048576,
        "window overflow, late configure or grant accounting failed");
    ProtoMessage message;
    uint32_t window = 0;
    NetInterfaceTlsTestPair::require(NetInterfaceCredit::window(message, window) == NI_OK && window == 65536,
        "legacy peer did not retain 64 KiB default");
    message.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(1048576)));
    NetInterfaceTlsTestPair::require(NetInterfaceCredit::window(message, window) == NI_OK && window == 1048576,
        "valid negotiation ignored");
    NetInterfaceCreditHandle handle = nullptr;
    NetInterfaceTlsTestPair::require(NetInterface::creditCreate(&handle) == NI_OK, "credit create");
    const std::string bytes = message.toString();
    const int32_t configured = NetInterface::creditConfigureMessage(handle, reinterpret_cast<const uint8_t*>(bytes.data()),
        static_cast<uint32_t>(bytes.size()));
    uint32_t fields[2] = {};
    const int32_t queried = NetInterface::creditGet(handle, fields);
    NetInterface::creditDestroy(handle);
    NetInterfaceTlsTestPair::require(configured == NI_OK && queried == NI_OK && fields[0] == 1048576, "Wasm-compatible negotiation ABI");
    message.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(1048577)));
    NetInterfaceTlsTestPair::require(NetInterfaceCredit::window(message, window) == NI_PROTOCOL_ERROR, "oversize peer window accepted");
    message.setKeyValue(NI_GATEWAY_WINDOW, Variant(std::string("1048576")));
    NetInterfaceTlsTestPair::require(NetInterfaceCredit::window(message, window) == NI_PROTOCOL_ERROR, "wrong window field type accepted");
}

// NET_LINK_DRAIN_AUDIT_INTEGRATION
#include <cstdio>
#include <cstring>
#include <vector>
#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#define LINK_CHECK(condition) do { ++checks; if (!(condition)) { std::printf("Link audit failed line=%d checks=%u\n", __LINE__, checks); return 1; } } while (0)

/** 独立启用此main；identity.dat必须来自隔离测试，不得使用/分发正式身份或CA私钥
@param [in] argc 参数数量
@param [in] argv 可选身份路径；Wasm测试由测试构建嵌入到默认虚拟路径，正式库绝不嵌入
@return 0通过，其他失败
*/
int NetLinkTestCases::inputDrainAudit(int argc, char** argv)
{
    uint32_t checks = 0;
    try
    {
        NetInterfaceLinkTestIdentity identity;
        LINK_CHECK(identity.load(argc > 1 ? argv[1] : "/gateway-test-identity.dat"));
        uint8_t payload[16384];
        for (uint32_t index = 0; index < sizeof(payload); ++index)
        {
            payload[index] = static_cast<uint8_t>(index * 9 + 3);
        }
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemState before;
        _CrtMemCheckpoint(&before);
#endif
        for (uint32_t scenario = 0; scenario < 10; ++scenario)
        {
            NetInterfaceLinkTestPair pair(identity);
            LINK_CHECK(pair.handshake());
            NetInterfaceTunnelRecord record = {};
            record.m_kind = NI_TUNNEL_BYTES;
            record.m_product = 7;
            record.m_channel = 1;
            record.m_data = payload;
            record.m_size = scenario == 3 ? sizeof(payload) : 33;
            if (scenario != 1 && scenario != 9)
            {
                LINK_CHECK(pair.m_server.sendRecord(record) == NI_OK);
            }
            if (scenario == 1 || scenario == 2 || scenario == 9)
            {
                NetInterfaceFrame frame = {};
                frame.m_kind = NI_EVENT;
                frame.m_messageType = 991;
                frame.m_payload = payload;
                frame.m_payloadSize = 33;
                LINK_CHECK(pair.m_server.sendControl(frame) == NI_OK);
            }
            for (uint32_t step = 0; step < 32; ++step)
            {
                const int32_t moved = NetInterfaceLinkTestPair::move(pair.m_server, pair.m_client);
                LINK_CHECK(moved >= 0);
                if (moved == NI_NEED_MORE)
                {
                    break;
                }
            }
            if (scenario == 8 || scenario == 9)
            {
                // 本端存在大量未交付输出时，不能阻止已经到达的输入独立排空。
                NetInterfaceFrame outgoing = {};
                outgoing.m_kind = NI_REQUEST;
                outgoing.m_messageType = 992;
                outgoing.m_payload = payload;
                outgoing.m_payloadSize = sizeof(payload);
                for (uint32_t index = 0; index < 60; ++index)
                {
                    outgoing.m_requestId = index + 1;
                    LINK_CHECK(pair.m_client.sendControl(outgoing) == NI_OK);
                }
                for (uint32_t step = 0; step < 8; ++step)
                {
                    LINK_CHECK(pair.m_client.progress(1000) == NI_READY);
                }
            }
            int32_t status = pair.m_client.drainInput(1001);
            LINK_CHECK(status == NI_READY);
            LINK_CHECK(pair.m_client.sendRecord(record) == NI_INVALID_STATE);
            LINK_CHECK(pair.m_client.accept(record) == NI_INVALID_STATE);
            NetInterfaceFrame blockedSend = {};
            blockedSend.m_kind = NI_EVENT;
            blockedSend.m_messageType = 991;
            LINK_CHECK(pair.m_client.sendControl(blockedSend) == NI_INVALID_STATE && pair.m_client.outputState() == NI_INVALID_STATE);
            NetInterfaceTunnelRecord forbidden = {};
            LINK_CHECK(pair.m_client.peekOutput(forbidden) == NI_INVALID_STATE && forbidden.m_data == nullptr);
            if (scenario == 4)
            {
                for (uint32_t now = 1002; now < 6001; now += 271)
                {
                    LINK_CHECK(pair.m_client.drainInput(now) == NI_READY);
                }
                LINK_CHECK(pair.m_client.drainInput(6001) == NI_TIMEOUT);
                continue;
            }
            if (scenario == 5)
            {
                LINK_CHECK(pair.m_client.drainInput(1000) == NI_INVALID_ARGUMENT);
                continue;
            }
            if (scenario == 6)
            {
                LINK_CHECK(pair.m_client.finishInput() == NI_TRUNCATED);
                continue;
            }
            if (scenario == 7)
            {
                for (uint32_t retry = 0; retry < 16; ++retry)
                {
                    NetInterfaceTunnelRecord view = {};
                    LINK_CHECK(pair.m_client.drainInput(1001 + retry) == NI_READY && pair.m_client.peekRecord(view) == NI_READY &&
                        view.m_size == record.m_size && std::memcmp(view.m_data, payload, view.m_size) == 0);
                }
            }
            uint32_t records = 0;
            uint32_t controls = 0;
            for (uint32_t step = 0; step < 32 && status >= 0; ++step)
            {
                NetInterfaceFrame frame = {};
                if (pair.m_client.peekControl(frame) == NI_READY)
                {
                    LINK_CHECK(frame.m_messageType == 991 && frame.m_payloadSize == 33 && std::memcmp(frame.m_payload, payload, 33) == 0);
                    LINK_CHECK(pair.m_client.consumeControl() == NI_OK);
                    ++controls;
                }
                else
                {
                    NetInterfaceTunnelRecord view = {};
                    if (pair.m_client.peekRecord(view) == NI_READY)
                    {
                        LINK_CHECK(view.m_size == record.m_size && view.m_product == 7 && view.m_channel == 1 &&
                            std::memcmp(view.m_data, payload, view.m_size) == 0);
                        LINK_CHECK(pair.m_client.consumeRecord() == NI_OK);
                        ++records;
                    }
                }
                status = pair.m_client.drainInput(scenario == 7 ? 1017 : 1001);
            }
            LINK_CHECK(status == NI_TRUNCATED && records == (scenario == 1 || scenario == 9 ? 0U : 1U) &&
                controls == (scenario == 1 || scenario == 2 || scenario == 9 ? 1U : 0U));
            LINK_CHECK(pair.m_client.drainInput(7000) == NI_TRUNCATED);
        }
        // 最大管理帧的TLS密文逐字节交付；最后一个字节仅入输入块，随后直接EOF。
        {
            NetInterfaceLinkTestPair pair(identity);
            LINK_CHECK(pair.handshake());
            NetInterfaceFrame outgoing = {};
            outgoing.m_kind = NI_REQUEST;
            outgoing.m_messageType = 993;
            outgoing.m_requestId = 7;
            outgoing.m_payload = payload;
            outgoing.m_payloadSize = sizeof(payload);
            LINK_CHECK(pair.m_server.sendControl(outgoing) == NI_OK);
            uint32_t records = 0;
            for (uint32_t step = 0; step < 16; ++step)
            {
                NetInterfaceTunnelRecord record = {};
                const int32_t ready = pair.m_server.peekOutput(record);
                if (ready != NI_READY)
                {
                    LINK_CHECK(ready == NI_NEED_MORE);
                    break;
                }
                LINK_CHECK(record.m_kind == NI_TUNNEL_CONTROL);
                for (uint32_t offset = 0; offset < record.m_size; ++offset)
                {
                    LINK_CHECK(pair.m_client.progress(1000) == NI_READY);
                    NetInterfaceTunnelRecord fragment = record;
                    fragment.m_data = record.m_data + offset;
                    fragment.m_size = 1;
                    LINK_CHECK(pair.m_client.accept(fragment) == NI_OK);
                }
                LINK_CHECK(pair.m_server.consumeOutput() == NI_OK);
                ++records;
            }
            LINK_CHECK(records >= 2 && pair.m_server.outputState() == NI_OK);
            LINK_CHECK(pair.m_client.drainInput(1001) == NI_READY);
            NetInterfaceFrame incoming = {};
            LINK_CHECK(pair.m_client.peekControl(incoming) == NI_READY && incoming.m_messageType == 993 && incoming.m_requestId == 7 &&
                incoming.m_payloadSize == sizeof(payload) && std::memcmp(incoming.m_payload, payload, sizeof(payload)) == 0);
            LINK_CHECK(pair.m_client.consumeControl() == NI_OK);
            LINK_CHECK(pair.m_client.drainInput(1001) == NI_TRUNCATED);
        }
        // 排空前并未认证的连接绝不因最后的挑战/证明进入ready。
        {
            NetInterfaceLink client(identity.options(false));
            LINK_CHECK(client.drainInput(1000) == NI_TRUNCATED && client.progress(1001) == NI_TRUNCATED);
        }
        // 一次TLS密文中包含超过单轮64条预算的完整管理帧，逐条消费至全部排空。
        {
            NetInterfaceLinkTestPair pair(identity);
            LINK_CHECK(pair.handshake());
            for (uint32_t index = 0; index < 80; ++index)
            {
                NetInterfaceFrame frame = {};
                frame.m_kind = static_cast<uint16_t>(index < 64 ? NI_REQUEST : NI_EVENT);
                frame.m_messageType = 991;
                frame.m_requestId = index + 1;
                frame.m_payload = payload;
                frame.m_payloadSize = 3;
                LINK_CHECK(pair.m_server.sendControl(frame) == NI_OK);
            }
            for (uint32_t step = 0; step < 8; ++step)
            {
                LINK_CHECK(pair.m_server.progress(1000) == NI_READY);
            }
            LINK_CHECK(NetInterfaceLinkTestPair::move(pair.m_server, pair.m_client) == NI_OK);
            LINK_CHECK(pair.m_server.outputState() == NI_OK);
            uint32_t count = 0;
            bool seen[80] = {};
            int32_t status = pair.m_client.drainInput(1001);
            for (uint32_t step = 0; step < 256 && status >= 0; ++step)
            {
                NetInterfaceFrame frame = {};
                if (pair.m_client.peekControl(frame) == NI_READY)
                {
                    LINK_CHECK(frame.m_requestId >= 1 && frame.m_requestId <= 80 && !seen[frame.m_requestId - 1]);
                    seen[frame.m_requestId - 1] = true;
                    LINK_CHECK(frame.m_payloadSize == 3 && std::memcmp(frame.m_payload, payload, 3) == 0);
                    LINK_CHECK(pair.m_client.consumeControl() == NI_OK);
                    ++count;
                }
                status = pair.m_client.drainInput(1001);
            }
            if (status != NI_TRUNCATED || count != 80)
            {
                std::printf("drain80 status=%d count=%u\n", status, count);
            }
            LINK_CHECK(status == NI_TRUNCATED && count == 80);
        }
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemState after;
        _CrtMemState difference;
        _CrtMemCheckpoint(&after);
        _CrtMemDifference(&difference, &before, &after);
        LINK_CHECK(difference.lCounts[_NORMAL_BLOCK] == 0 && difference.lSizes[_NORMAL_BLOCK] == 0);
#endif
        std::printf("Link input drain checks=%u PASS\n", checks);
        return 0;
    }
    catch (...)
    {
        std::printf("Link input drain unexpected exception\n");
        return 2;
    }
}

#undef LINK_CHECK
#ifndef __EMSCRIPTEN__
#include "NetSender/NetSenderAPI.h"

#define SENDER_LINK_CHECK(condition) do { ++checks; if (!(condition)) { std::printf("Sender link failed line=%d checks=%u\n", __LINE__, checks); return 1; } } while (0)

/** 同库白盒测试原始END回调与协议排空；真实跨进程路径另由NetWorkTest联调验证
@param [in] argc 参数数量
@param [in] argv 第一个参数为隔离身份路径
@return 0通过
*/
int NetLinkTestCases::senderDrainAudit(int argc, char** argv)
{
    uint32_t checks = 0;
    NetInterfaceLinkTestIdentity identity;
    SENDER_LINK_CHECK(argc == 2 && identity.load(argv[1]));
#if defined(_WIN32) && defined(_DEBUG)
    _CrtMemState before;
    _CrtMemCheckpoint(&before);
#endif
    for (uint32_t mode = 0; mode < 6; ++mode)
    {
        NetSenderLocalInbox inbox;
        NetSenderLinkAuditListener listener;
        NetSenderLinkState state(inbox);
        listener.m_state = &state;
        listener.m_mode = mode;
        SENDER_LINK_CHECK(state.configure(identity.options(false), &listener) == NI_OK);
        state.onRawLocalReady(1, 0);
        SENDER_LINK_CHECK(state.m_peers.size() == 1);
        NetInterfaceLink& client = state.m_peers.begin()->second->m_link;
        NetInterfaceLink server(identity.options(true));
        for (uint32_t step = 0; step < 10000; ++step)
        {
            SENDER_LINK_CHECK(client.progress(1000) >= 0 && server.progress(1000) >= 0);
            SENDER_LINK_CHECK(NetInterfaceLinkTestPair::move(client, server) >= 0 && NetInterfaceLinkTestPair::move(server, client) >= 0);
            if (client.status() == NI_READY && server.status() == NI_READY)
            {
                break;
            }
        }
        SENDER_LINK_CHECK(client.status() == NI_READY && server.status() == NI_READY);
        // 已认证且上层已观察ready是本白盒测试的入口条件；不启用任何进程接收器。
        state.m_peers.begin()->second->m_announced = true;
        NetInterfaceLinkPeer identityView = {};
        SENDER_LINK_CHECK(client.peerInfo(identityView) == NI_READY);
        listener.onLinkReady(1, identityView);
        const uint8_t payload[] = {9, 7, 5, 3};
        if (mode != 1)
        {
            NetInterfaceTunnelRecord record = {};
            record.m_kind = NI_TUNNEL_BYTES;
            record.m_product = 7;
            record.m_channel = 1;
            record.m_data = payload;
            record.m_size = sizeof(payload);
            SENDER_LINK_CHECK(server.sendRecord(record) == NI_OK);
        }
        if (mode != 0)
        {
            NetInterfaceFrame frame = {};
            frame.m_kind = NI_EVENT;
            frame.m_messageType = 991;
            frame.m_payload = payload;
            frame.m_payloadSize = sizeof(payload);
            SENDER_LINK_CHECK(server.sendControl(frame) == NI_OK);
        }
        for (uint32_t step = 0; step < 16; ++step)
        {
            NetInterfaceTunnelRecord record = {};
            const int32_t ready = server.peekOutput(record);
            if (ready != NI_READY)
            {
                SENDER_LINK_CHECK(ready == NI_NEED_MORE);
                break;
            }
            SENDER_LINK_CHECK(state.onRawLocalRecord(1, record));
            SENDER_LINK_CHECK(server.consumeOutput() == NI_OK);
        }
        state.onRawLocalClosed(1, mode == 4 ? NI_INVALID_STATE : NI_OK);
        SENDER_LINK_CHECK(state.m_peers.size() == 1 && listener.m_closed == 0);
        for (uint32_t step = 0; step < 16 && !state.m_peers.empty(); ++step)
        {
            state.pump(1, *state.m_peers.begin()->second);
            state.collectClosed();
        }
        SENDER_LINK_CHECK(state.m_peers.empty() && listener.m_closed == 1 && listener.m_valid);
        SENDER_LINK_CHECK(listener.m_records == (mode == 1 ? 0U : 1U) && listener.m_controls == (mode == 0 ? 0U : 1U));
        SENDER_LINK_CHECK(listener.m_reason == (mode == 4 ? NI_INVALID_STATE : mode == 5 ? NI_CANCELLED : NI_TRUNCATED));
        SENDER_LINK_CHECK(mode != 3 || listener.m_attempts == 4);
        state.onRawLocalClosed(1, NI_TIMEOUT);
        state.collectClosed();
        SENDER_LINK_CHECK(listener.m_closed == 1);
    }
#if defined(_WIN32) && defined(_DEBUG)
    _CrtMemState after;
    _CrtMemState difference;
    _CrtMemCheckpoint(&after);
    _CrtMemDifference(&difference, &before, &after);
    SENDER_LINK_CHECK(difference.lCounts[_NORMAL_BLOCK] == 0 && difference.lSizes[_NORMAL_BLOCK] == 0);
#endif
    std::printf("Sender link drain checks=%u PASS\n", checks);
    return 0;
}

#undef SENDER_LINK_CHECK
#endif

// NET_LINK_QUEUE_AUDIT_INTEGRATION
#include <chrono>

#if defined(_WIN32) && defined(_DEBUG)
static int32_t s_linkQueueFailAfter = -1;
static uint32_t s_linkQueueAllocations = 0;
/** 测试窗口内计数或拒绝普通堆分配，不改变CRT内部记录
@param [in] operation CRT分配动作
@param [in] data 旧分配指针
@param [in] size 新大小
@param [in] block 分配类型
@param [in] request 分配编号
@param [in] file 来源
@param [in] line 行号
@return 1允许，0拒绝
*/
static int __cdecl LinkQueueAllocationHook(int operation, void* data, size_t size, int block, long request,
    const unsigned char* file, int line)
{
    (void)data;
    (void)size;
    (void)request;
    (void)file;
    (void)line;
    if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
    {
        ++s_linkQueueAllocations;
        if (s_linkQueueFailAfter == 0)
        {
            return 0;
        }
        if (s_linkQueueFailAfter > 0)
        {
            --s_linkQueueFailAfter;
        }
    }
    return 1;
}

#endif

#define LINK_QUEUE_CHECK(condition) do { ++checks; if (!(condition)) { std::printf("Link output queue failed line=%d checks=%u\n", __LINE__, checks); return 1; } } while (0)

/** 真实管理认证后验证输出容量、复制/FIFO、非法空载荷、失败重试及同尺寸缓冲复用
@param [in] argc 参数数量
@param [in] argv 隔离identity.dat，另可指定--benchmark只跑受控提交耗时
@return 0通过
*/
int NetLinkTestCases::outputQueueAudit(int argc, char** argv)
{
    uint32_t checks = 0;
    try
    {
        NetInterfaceLinkTestIdentity identity;
#ifdef __EMSCRIPTEN__
        LINK_QUEUE_CHECK(identity.load(argc > 1 ? argv[1] : "/gateway-test-identity.dat"));
#else
        LINK_QUEUE_CHECK(argc >= 2 && identity.load(argv[1]));
#endif
        const bool benchmark = argc == 3 && std::strcmp(argv[2], "--benchmark") == 0;
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemState before;
        _CrtMemCheckpoint(&before);
#endif
        {
            NetInterfaceLinkTestPair pair(identity);
            LINK_QUEUE_CHECK(pair.handshake());
            uint8_t payload[16384];
            for (uint32_t index = 0; index < sizeof(payload); ++index)
            {
                payload[index] = static_cast<uint8_t>(index * 13 + 7);
            }
            NetInterfaceTunnelRecord record = {};
            record.m_kind = NI_TUNNEL_BYTES;
            record.m_product = 7;
            record.m_channel = 1;
            record.m_data = payload;
            record.m_size = sizeof(payload);
            NetInterfaceTunnelRecord output = {};
            if (!benchmark)
            {
                for (uint32_t round = 0; round < 17; ++round)
                {
                    for (uint32_t index = 0; index < 4; ++index)
                    {
                        record.m_channel = 1 + round * 4 + index;
                        LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
                    }
                    LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_LIMIT_EXCEEDED);
                    for (uint32_t index = 0; index < 4; ++index)
                    {
                        LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_kind == NI_TUNNEL_BYTES &&
                            output.m_channel == 1 + round * 4 + index && output.m_product == 7 && output.m_size == sizeof(payload) &&
                            std::memcmp(output.m_data, payload, sizeof(payload)) == 0);
                        const uint8_t* borrowed = output.m_data;
                        LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && borrowed == output.m_data);
                        LINK_QUEUE_CHECK(pair.m_client.consumeOutput() == NI_OK);
                    }
                    LINK_QUEUE_CHECK(pair.m_client.outputState() == NI_OK && pair.m_client.peekOutput(output) == NI_NEED_MORE &&
                        pair.m_client.consumeOutput() == NI_INVALID_STATE);
                }
                // CLOSE必须带4字节原因；先验证空载荷拒绝，再验证32槽及回绕。
                record.m_kind = NI_TUNNEL_CLOSE;
                record.m_data = nullptr;
                record.m_size = 0;
                LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_INVALID_ARGUMENT && pair.m_client.outputState() == NI_OK);
                record.m_data = payload;
                record.m_size = 4;
                for (uint32_t round = 0; round < 3; ++round)
                {
                    for (uint32_t index = 0; index < 32; ++index)
                    {
                        record.m_channel = 1000 + round * 32 + index;
                        LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
                    }
                    LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_LIMIT_EXCEEDED);
                    for (uint32_t index = 0; index < 32; ++index)
                    {
                        LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_kind == NI_TUNNEL_CLOSE &&
                            output.m_channel == 1000 + round * 32 + index && output.m_size == 4 && std::memcmp(output.m_data, payload, 4) == 0);
                        LINK_QUEUE_CHECK(pair.m_client.consumeOutput() == NI_OK);
                    }
                }
                record.m_kind = NI_TUNNEL_BYTES;
                record.m_size = 37;
                record.m_data = payload;
                const uint8_t original = payload[0];
                LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
                payload[0] ^= 0xff;
                LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_data[0] == original);
                payload[0] = original;
                // 借用队首作为新输入合法；接纳不得破坏原借用或让新包引用旧存储。
                output.m_channel = 5000;
                LINK_QUEUE_CHECK(pair.m_client.sendRecord(output) == NI_OK && pair.m_client.consumeOutput() == NI_OK);
                LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_channel == 5000 &&
                    output.m_size == record.m_size && std::memcmp(output.m_data, payload, record.m_size) == 0);
                LINK_QUEUE_CHECK(pair.m_client.consumeOutput() == NI_OK);
#if defined(_WIN32) && defined(_DEBUG)
                // 换尺寸强制需要新正文；失败不占槽、不扣字节、不丢已有队首。
                record.m_size = 19;
                LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
                record.m_size = 911;
                s_linkQueueFailAfter = 0;
                _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(LinkQueueAllocationHook);
                const int32_t failed = pair.m_client.sendRecord(record);
                _CrtSetAllocHook(previous);
                s_linkQueueFailAfter = -1;
                LINK_QUEUE_CHECK(failed == NI_NO_MEMORY && pair.m_client.status() == NI_READY);
                LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_size == 19 && pair.m_client.consumeOutput() == NI_OK);
                LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
                LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_size == 911 && pair.m_client.consumeOutput() == NI_OK);
#endif
            }
            const uint32_t sizes[] = {32, 1024, 16384};
            for (uint32_t group = 0; group < 3; ++group)
            {
                record.m_kind = NI_TUNNEL_BYTES;
                record.m_channel = group + 1;
                record.m_data = payload;
                record.m_size = sizes[group];
                LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK && pair.m_client.peekOutput(output) == NI_READY &&
                    pair.m_client.consumeOutput() == NI_OK);
#if defined(_WIN32) && defined(_DEBUG)
                s_linkQueueAllocations = 0;
                _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(LinkQueueAllocationHook);
#endif
                const auto started = std::chrono::steady_clock::now();
                bool valid = true;
                for (uint32_t index = 0; index < 100000; ++index)
                {
                    if (pair.m_client.sendRecord(record) != NI_OK || pair.m_client.peekOutput(output) != NI_READY ||
                        output.m_size != record.m_size || output.m_channel != record.m_channel ||
                        std::memcmp(output.m_data, payload, record.m_size) != 0 || pair.m_client.consumeOutput() != NI_OK)
                    {
                        valid = false;
                        break;
                    }
                }
                const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started).count();
#if defined(_WIN32) && defined(_DEBUG)
                _CrtSetAllocHook(previous);
                std::printf("Link queue bytes=%u allocations=%u\n", record.m_size, s_linkQueueAllocations);
                LINK_QUEUE_CHECK(s_linkQueueAllocations == 0);
#endif
                LINK_QUEUE_CHECK(valid && pair.m_client.outputState() == NI_OK);
                std::printf("Link queue bytes=%u records=100000 microseconds=%lld\n", record.m_size, static_cast<long long>(elapsed));
            }
            // 非空析构：剩余大正文、关闭原因及单份复用缓冲均归本实例释放。
            record.m_size = 16384;
            LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK && pair.m_client.sendRecord(record) == NI_OK);
            record.m_kind = NI_TUNNEL_CLOSE;
            record.m_data = payload;
            record.m_size = 4;
            LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
        }
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemState after;
        _CrtMemState difference;
        _CrtMemCheckpoint(&after);
        _CrtMemDifference(&difference, &before, &after);
        LINK_QUEUE_CHECK(difference.lCounts[_NORMAL_BLOCK] == 0 && difference.lSizes[_NORMAL_BLOCK] == 0);
#endif
        std::printf("Link output queue checks=%u PASS\n", checks);
        return 0;
    }
    catch (...)
    {
        std::printf("Link output queue unexpected exception\n");
        return 2;
    }
}

#undef LINK_QUEUE_CHECK

// NET_LINK_AUTH_AUDIT_INTEGRATION

#define LINK_AUTH_CHECK(condition) do { ++checks; if (!(condition)) { std::printf("Link auth failed mode=%u line=%d checks=%u\n", mode, __LINE__, checks); return 1; } } while (0)

/** 使用真实TLS构造畸形管理证明；预期独立于认证实现，正文不输出
@param [in] argc 参数数量
@param [in] argv 隔离identity.dat路径
@return 0通过
*/
int NetLinkTestCases::authenticationAudit(int argc, char** argv)
{
    uint32_t checks = 0;
    uint32_t mode = 0;
    try
    {
        NetInterfaceLinkTestIdentity identity;
#ifdef __EMSCRIPTEN__
        LINK_AUTH_CHECK(identity.load(argc > 1 ? argv[1] : "/gateway-test-identity.dat"));
#else
        LINK_AUTH_CHECK(argc == 2 && identity.load(argv[1]));
#endif
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemState before;
        _CrtMemCheckpoint(&before);
#endif
        {
            // 读取待发输出不应偷偷用0启动TLS期限；计时起点属于首次显式progress。
            {
                NetInterfaceLink link(identity.options(false));
                NetInterfaceTunnelRecord output = {};
                LINK_AUTH_CHECK(link.peekOutput(output) == NI_READY && output.m_kind == NI_TUNNEL_HELLO &&
                    link.consumeOutput() == NI_OK && link.peekOutput(output) >= 0);
                const int32_t firstProgress = link.progress(1000000);
                std::printf("Link auth first explicit progress=%d\n", firstProgress);
                LINK_AUTH_CHECK(firstProgress == NI_NEED_MORE && link.progress(1004999) == NI_NEED_MORE &&
                    link.progress(1005000) == NI_TIMEOUT);
            }
            std::vector<uint8_t> previousProof;
            for (mode = 0; mode < 24; ++mode)
            {
                NetInterfaceLinkAuthTestPair pair(identity, true);
                LINK_AUTH_CHECK(pair.start());
                std::vector<uint8_t> challenge;
                LINK_AUTH_CHECK(pair.receive(NI_HELLO, NET_INTERFACE_LINK_CHALLENGE, challenge) && challenge.size() == 32);
                std::vector<uint8_t> body(88, 0);
                std::memcpy(body.data(), "NLH3", 4);
                NetInterfaceLinkAuthTestPair::word(body.data() + 4, NI_LINK_CLIENT);
                std::memcpy(body.data() + 24, challenge.data(), 32);
                std::memset(body.data() + 56, 0x39, 32);
                NetInterfaceFrame frame = {};
                frame.m_kind = NI_HELLO;
                frame.m_messageType = NET_INTERFACE_LINK_PROOF;
                switch (mode)
                {
                case 1:
                    body[0] ^= 1;
                    break;
                case 2:
                    frame.m_kind = NI_EVENT;
                    break;
                case 3:
                    frame.m_messageType = NET_INTERFACE_LINK_CHALLENGE;
                    break;
                case 4:
                    frame.m_productId = 7;
                    break;
                case 5:
                    frame.m_requestId = 1;
                    break;
                case 6:
                    body.resize(87);
                    break;
                case 7:
                    body[24] ^= 1;
                    break;
                case 8:
                    NetInterfaceLinkAuthTestPair::word(body.data() + 4, 0);
                    break;
                case 9:
                    NetInterfaceLinkAuthTestPair::word(body.data() + 4, NI_LINK_SERVICE);
                    break;
                case 10:
                    NetInterfaceLinkAuthTestPair::word(body.data() + 4, NI_LINK_NODE);
                    break;
                case 11:
                    NetInterfaceLinkAuthTestPair::word(body.data() + 8, UINT32_MAX);
                    break;
                case 12:
                    NetInterfaceLinkAuthTestPair::word(body.data() + 12, UINT32_MAX);
                    break;
                case 13:
                    NetInterfaceLinkAuthTestPair::word(body.data() + 16, 4097);
                    break;
                case 14:
                    NetInterfaceLinkAuthTestPair::word(body.data() + 20, 81);
                    break;
                case 15:
                    NetInterfaceLinkAuthTestPair::word(body.data() + 8, 1);
                    body.push_back('x');
                    break;
                case 16:
                    NetInterfaceLinkAuthTestPair::word(body.data() + 12, 1);
                    body.push_back('x');
                    break;
                case 17:
                    NetInterfaceLinkAuthTestPair::word(body.data() + 20, 1);
                    body.push_back('x');
                    break;
                case 18:
                    NetInterfaceLinkAuthTestPair::word(body.data() + 16, 4);
                    body.insert(body.end(), 4, 0x2d);
                    break;
                case 19:
                    body.push_back(0);
                    break;
                case 20:
                    LINK_AUTH_CHECK(previousProof.size() == 88 && std::memcmp(previousProof.data() + 24, challenge.data(), 32) != 0);
                    body = previousProof;
                    break;
                case 22:
                    frame.m_kind = NI_REQUEST;
                    frame.m_requestId = 1;
                    frame.m_messageType = 998;
                    break;
                case 23:
                    frame.m_kind = NI_PING;
                    frame.m_requestId = 1;
                    frame.m_messageType = NET_INTERFACE_LINK_HEARTBEAT;
                    body.clear();
                    break;
                default: break;
                }
                frame.m_payload = body.empty() ? nullptr : body.data();
                frame.m_payloadSize = static_cast<uint32_t>(body.size());
                LINK_AUTH_CHECK(pair.m_raw.send(frame) == NI_OK);
                const int32_t result = pair.settle();
                const bool allowed = mode == 0 || mode == 18 || mode == 21;
                LINK_AUTH_CHECK(result == (allowed ? NI_READY : NI_PROTOCOL_ERROR));
                NetInterfaceLinkPeer peer = {};
                if (allowed)
                {
                    LINK_AUTH_CHECK(pair.m_link.peerInfo(peer) == NI_READY && peer.m_role == NI_LINK_CLIENT &&
                        peer.m_claimsSize == (mode == 18 ? 4U : 0U));
                    if (mode == 0)
                    {
                        previousProof = body;
                    }
                    if (mode == 21)
                    {
                        LINK_AUTH_CHECK(pair.m_raw.send(frame) == NI_OK);
                        int32_t duplicated = NI_READY;
                        for (uint32_t retry = 0; retry < 100 && duplicated >= 0; ++retry)
                        {
                            duplicated = pair.step();
                        }
                        LINK_AUTH_CHECK(duplicated == NI_PROTOCOL_ERROR);
                    }
                }
                else
                {
                    LINK_AUTH_CHECK(pair.m_link.peerInfo(peer) == NI_PROTOCOL_ERROR && peer.m_role == 0 &&
                        peer.m_name == nullptr && pair.m_link.progress(2000) == NI_PROTOCOL_ERROR);
                }
            }
            for (mode = 24; mode < 33; ++mode)
            {
                NetInterfaceLinkAuthTestPair pair(identity, false);
                LINK_AUTH_CHECK(pair.start());
                for (uint32_t step = 0; step < 10000 && pair.m_raw.status() != NET_INTERFACE_TLS_READY; ++step)
                {
                    LINK_AUTH_CHECK(pair.step() >= 0);
                }
                LINK_AUTH_CHECK(pair.m_raw.status() == NET_INTERFACE_TLS_READY && pair.m_link.status() == NI_NEED_MORE);
                std::vector<uint8_t> challenge(32, 0x47);
                NetInterfaceFrame frame = {};
                frame.m_kind = NI_HELLO;
                frame.m_messageType = NET_INTERFACE_LINK_CHALLENGE;
                switch (mode)
                {
                case 25:
                    frame.m_requestId = 1;
                    break;
                case 26:
                    frame.m_productId = 7;
                    break;
                case 27:
                    frame.m_messageType = NET_INTERFACE_LINK_PROOF;
                    break;
                case 28:
                    frame.m_kind = NI_EVENT;
                    break;
                case 29:
                    challenge.resize(31);
                    break;
                case 30:
                    challenge.push_back(0);
                    break;
                case 31:
                    frame.m_messageType = 998;
                    break;
                case 32:
                    frame.m_kind = NI_HELLO_REPLY;
                    break;
                default: break;
                }
                frame.m_payload = challenge.data();
                frame.m_payloadSize = static_cast<uint32_t>(challenge.size());
                LINK_AUTH_CHECK(pair.m_raw.send(frame) == NI_OK);
                if (mode == 24)
                {
                    std::vector<uint8_t> proof;
                    LINK_AUTH_CHECK(pair.receive(NI_HELLO, NET_INTERFACE_LINK_PROOF, proof) && proof.size() == 88 &&
                        std::memcmp(proof.data(), "NLH3", 4) == 0 && std::memcmp(proof.data() + 24, challenge.data(), 32) == 0);
                    // 正确双随机数仍不够：没有可信节点证书和签名的接受消息必须拒绝。
                    std::vector<uint8_t> accepted(88, 0);
                    std::memcpy(accepted.data(), "NLA3", 4);
                    NetInterfaceLinkAuthTestPair::word(accepted.data() + 4, NI_LINK_CLIENT);
                    std::memcpy(accepted.data() + 24, challenge.data(), 32);
                    std::memcpy(accepted.data() + 56, proof.data() + 56, 32);
                    frame.m_kind = NI_HELLO_REPLY;
                    frame.m_messageType = NET_INTERFACE_LINK_ACCEPT;
                    frame.m_payload = accepted.data();
                    frame.m_payloadSize = static_cast<uint32_t>(accepted.size());
                    LINK_AUTH_CHECK(pair.m_raw.send(frame) == NI_OK && pair.settle() == NI_PROTOCOL_ERROR);
                }
                else
                {
                    LINK_AUTH_CHECK(pair.settle() == NI_PROTOCOL_ERROR);
                }
            }
            // 节点双向身份由实际签名验证；CLIENT不因管理TLS成功就获得NODE身份。
            mode = 33;
            {
                NetInterfaceLinkOptions serverOptions = identity.options(true);
                serverOptions.m_peerRoles = 1U << NI_LINK_NODE;
                NetInterfaceLinkOptions clientOptions = identity.options(true);
                clientOptions.m_tls.server = 0;
                clientOptions.m_peerRoles = 0;
                NetInterfaceLink server(serverOptions);
                NetInterfaceLink client(clientOptions);
                for (uint32_t step = 0; step < 10000; ++step)
                {
                    LINK_AUTH_CHECK(client.progress(1000) >= 0 && server.progress(1000) >= 0 &&
                        NetInterfaceLinkTestPair::move(client, server) >= 0 && NetInterfaceLinkTestPair::move(server, client) >= 0);
                    if (client.status() == NI_READY && server.status() == NI_READY)
                    {
                        break;
                    }
                }
                NetInterfaceLinkPeer peer = {};
                LINK_AUTH_CHECK(client.status() == NI_READY && server.peerInfo(peer) == NI_READY && peer.m_role == NI_LINK_NODE &&
                    std::strcmp(peer.m_name, identity.m_name.c_str()) == 0 && peer.m_certificateSize == identity.m_certificate.size());
                LINK_AUTH_CHECK(client.peerInfo(peer) == NI_READY && peer.m_role == NI_LINK_NODE);
            }
            // 服务首次接入不能缺宿主验证；非法名称、缺少本机证明均在启动阶段拒绝。
            mode = 34;
            {
                NetInterfaceLinkOptions options = identity.options(true);
                options.m_peerRoles |= 1U << NI_LINK_SERVICE;
                NetInterfaceLink noVerifier(options);
                LINK_AUTH_CHECK(noVerifier.status() == NI_INVALID_ARGUMENT);
                options = identity.options(false);
                options.m_role = NI_LINK_SERVICE;
                options.m_name = "service.7";
                options.m_tls.privateKey = reinterpret_cast<const uint8_t*>(identity.m_key.c_str());
                options.m_tls.privateKeySize = static_cast<uint32_t>(identity.m_key.size() + 1);
                NetInterfaceLink noProof(options);
                LINK_AUTH_CHECK(noProof.status() == NI_INVALID_ARGUMENT);
                options = identity.options(true);
                const std::string differentName = "node.00000000000000000000000000000000";
                LINK_AUTH_CHECK(differentName != identity.m_name);
                options.m_name = differentName.c_str();
                NetInterfaceLink wrongIdentity(options);
                LINK_AUTH_CHECK(wrongIdentity.status() == NI_PROTOCOL_ERROR);
            }
        }
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemState after;
        _CrtMemState difference;
        _CrtMemCheckpoint(&after);
        _CrtMemDifference(&difference, &before, &after);
        LINK_AUTH_CHECK(difference.lCounts[_NORMAL_BLOCK] == 0 && difference.lSizes[_NORMAL_BLOCK] == 0);
#endif
        std::printf("Link auth checks=%u PASS\n", checks);
        return 0;
    }
    catch (...)
    {
        std::printf("Link auth unexpected exception mode=%u\n", mode);
        return 2;
    }
}

#undef LINK_AUTH_CHECK