#include "NetInterfaceMacro.h"
#include "NetInterfaceLinkState.h"
#include "NetInterface.h"
#include "NetInterfaceLink.h"
#include "NetInterfaceTunnel.h"
#include <algorithm>
#include <cstring>

NetInterfaceLink::NetInterfaceLink(const NetInterfaceLinkOptions& options) :
m_state(new NetInterfaceLinkState())
{
    try
    {
        m_state->configure(options);
    }
    catch (...)
    {
        m_state->fail(NI_NO_MEMORY);
    }
}

NetInterfaceLink::~NetInterfaceLink()
{
    delete m_state;
}

int32_t NetInterfaceLink::accept(const NetInterfaceTunnelRecord& record)
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase == NET_LINK_DRAINING)
    {
        return NI_INVALID_STATE;
    }
    uint32_t required = 0;
    if (NetInterfaceTunnel::encode(record, nullptr, 0, required) != NI_BUFFER_TOO_SMALL)
    {
        return m_state->fail(NI_PROTOCOL_ERROR);
    }
    if (record.m_kind == NI_TUNNEL_HELLO)
    {
        if (m_state->m_receivedHello)
        {
            return m_state->fail(NI_PROTOCOL_ERROR);
        }
        m_state->m_receivedHello = true;
        return NI_OK;
    }
    if (!m_state->m_receivedHello)
    {
        return m_state->fail(NI_PROTOCOL_ERROR);
    }
    try
    {
        if (record.m_kind == NI_TUNNEL_CONTROL)
        {
            if (!m_state->m_input.empty())
            {
                return NI_LIMIT_EXCEEDED;
            }
            m_state->m_input.assign(record.m_data, record.m_data + record.m_size);
            m_state->m_inputOffset = 0;
            return NI_OK;
        }
        if (m_state->status() != NI_READY)
        {
            return m_state->fail(NI_PROTOCOL_ERROR);
        }
        if (m_state->m_recordReady)
        {
            return NI_LIMIT_EXCEEDED;
        }
        m_state->m_receivedBytes.assign(record.m_data, record.m_data + record.m_size);
        m_state->m_received = record;
        m_state->m_received.m_data = m_state->m_receivedBytes.data();
        m_state->m_recordReady = true;
        return NI_OK;
    }
    catch (...)
    {
        return m_state->fail(NI_NO_MEMORY);
    }
}

int32_t NetInterfaceLink::progress(uint64_t now)
{
    try
    {
        return m_state->progress(now);
    }
    catch (...)
    {
        return m_state->fail(NI_NO_MEMORY);
    }
}

int32_t NetInterfaceLink::peekOutput(NetInterfaceTunnelRecord& record)
{
    record = NetInterfaceTunnelRecord();
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase == NET_LINK_DRAINING)
    {
        return NI_INVALID_STATE;
    }
    if (!m_state->m_outputPeeked)
    {
        NetInterfaceTunnelRecord next = {};
        if (!m_state->m_sentHello)
        {
            next.m_kind = NI_TUNNEL_HELLO;
        }
        else
        {
            if (!m_state->m_clockStarted)
            {
                // peek没有时钟参数；首个明确的progress负责启动TLS和管理总期限。
                return NI_NEED_MORE;
            }
            const int32_t progressed = m_state->m_control->progress(m_state->m_lastTime);
            if (progressed < 0)
            {
                return m_state->fail(progressed);
            }
            const uint8_t* output = nullptr;
            uint32_t size = 0;
            const int32_t result = m_state->m_control->peekOutput(output, size);
            if (result < 0)
            {
                return m_state->fail(result);
            }
            if (size != 0)
            {
                next.m_kind = NI_TUNNEL_CONTROL;
                next.m_data = output;
                next.m_size = (std::min)(size, NET_INTERFACE_TUNNEL_PAYLOAD_LIMIT);
            }
            else if (m_state->m_outputCount != 0)
            {
                NetInterfaceLinkWrite& front = m_state->m_output[m_state->m_outputHead];
                next = front.m_record;
                next.m_data = front.m_bytes.get();
            }
            else
            {
                return NI_NEED_MORE;
            }
        }
        m_state->m_peeked = next;
        m_state->m_outputPeeked = true;
    }
    record = m_state->m_peeked;
    return NI_READY;
}

int32_t NetInterfaceLink::consumeOutput()
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (!m_state->m_outputPeeked)
    {
        return NI_INVALID_STATE;
    }
    const NetInterfaceTunnelRecord record = m_state->m_peeked;
    if (record.m_kind == NI_TUNNEL_HELLO)
    {
        m_state->m_sentHello = true;
    }
    else if (record.m_kind == NI_TUNNEL_CONTROL)
    {
        const int32_t result = m_state->m_control->consumeOutput(record.m_size);
        if (result < 0)
        {
            return m_state->fail(result);
        }
    }
    else
    {
        m_state->consumeQueuedRecord();
    }
    m_state->m_peeked = NetInterfaceTunnelRecord();
    m_state->m_outputPeeked = false;
    return NI_OK;
}

int32_t NetInterfaceLink::sendRecord(const NetInterfaceTunnelRecord& record)
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase == NET_LINK_DRAINING)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->status() != NI_READY)
    {
        return NI_INVALID_STATE;
    }
    uint32_t required = 0;
    if ((record.m_kind != NI_TUNNEL_BYTES && record.m_kind != NI_TUNNEL_CLOSE) ||
        NetInterfaceTunnel::encode(record, nullptr, 0, required) != NI_BUFFER_TOO_SMALL)
    {
        return NI_INVALID_ARGUMENT;
    }
    return m_state->queueRecord(record);
}

int32_t NetInterfaceLink::peekRecord(NetInterfaceTunnelRecord& record) const
{
    record = NetInterfaceTunnelRecord();
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (!m_state->m_recordReady)
    {
        return NI_NEED_MORE;
    }
    record = m_state->m_received;
    return NI_READY;
}

int32_t NetInterfaceLink::consumeRecord()
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (!m_state->m_recordReady)
    {
        return NI_INVALID_STATE;
    }
    m_state->m_recordReady = false;
    m_state->m_received = NetInterfaceTunnelRecord();
    m_state->m_receivedBytes.clear();
    return NI_OK;
}

int32_t NetInterfaceLink::sendControl(const NetInterfaceFrame& frame)
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase == NET_LINK_DRAINING)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->status() != NI_READY)
    {
        return NI_INVALID_STATE;
    }
    if (frame.m_productId != 0 || frame.m_payloadSize > 16384 ||
        (frame.m_kind != NI_REQUEST && frame.m_kind != NI_RESPONSE && frame.m_kind != NI_EVENT && frame.m_kind != NI_CANCEL) ||
        (frame.m_messageType >= NET_INTERFACE_LINK_CHALLENGE && frame.m_messageType <= NET_INTERFACE_LINK_HEARTBEAT))
    {
        return NI_INVALID_ARGUMENT;
    }
    return m_state->m_control->send(frame);
}

int32_t NetInterfaceLink::peekControl(NetInterfaceFrame& frame) const
{
    frame = NetInterfaceFrame();
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->status() != NI_READY)
    {
        return NI_NEED_MORE;
    }
    if (m_state->m_phase == NET_LINK_DRAINING && m_state->m_drainResult < 0)
    {
        // TLS终态不得越过已经由外层接纳、尚待上层消费的业务记录。
        return NI_NEED_MORE;
    }
    const int32_t result = m_state->m_control->peek(frame);
    if (result == NI_READY && frame.m_productId != 0)
    {
        frame = NetInterfaceFrame();
        return m_state->fail(NI_PROTOCOL_ERROR);
    }
    if (result == NI_READY && (frame.m_kind == NI_PING || frame.m_kind == NI_PONG ||
        frame.m_kind == NI_HELLO || frame.m_kind == NI_HELLO_REPLY))
    {
        frame = NetInterfaceFrame();
        return NI_NEED_MORE;
    }
    return result;
}

int32_t NetInterfaceLink::consumeControl()
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    NetInterfaceFrame frame = {};
    if (peekControl(frame) != NI_READY)
    {
        return NI_INVALID_STATE;
    }
    m_state->m_activity = m_state->m_lastTime;
    return m_state->m_control->consume();
}

int32_t NetInterfaceLink::peerInfo(NetInterfaceLinkPeer& peer) const
{
    peer = NetInterfaceLinkPeer();
    if (m_state->status() != NI_READY)
    {
        return m_state->status();
    }
    peer.m_role = m_state->m_peerRole;
    peer.m_name = m_state->m_peerName.c_str();
    peer.m_certificate = m_state->m_peerCertificate.empty() ? nullptr :
        reinterpret_cast<const uint8_t*>(m_state->m_peerCertificate.data());
    peer.m_certificateSize = static_cast<uint32_t>(m_state->m_peerCertificate.size());
    peer.m_claims = m_state->m_peerClaims.empty() ? nullptr : m_state->m_peerClaims.data();
    peer.m_claimsSize = static_cast<uint32_t>(m_state->m_peerClaims.size());
    return NI_READY;
}

int32_t NetInterfaceLink::serviceGrant(const uint8_t*& bytes, uint32_t& size) const
{
    bytes = nullptr;
    size = 0;
    if (m_state->status() != NI_READY)
    {
        return m_state->status();
    }
    if (m_state->m_options.m_role != NI_LINK_SERVICE)
    {
        return NI_INVALID_STATE;
    }
    bytes = m_state->m_grant.data();
    size = static_cast<uint32_t>(m_state->m_grant.size());
    return NI_READY;
}

int32_t NetInterfaceLink::finishInput()
{
    return m_state->fail(NI_TRUNCATED);
}

int32_t NetInterfaceLink::drainInput(uint64_t nowMs)
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase != NET_LINK_DRAINING)
    {
        if (m_state->status() != NI_READY)
        {
            return m_state->fail(NI_TRUNCATED);
        }
        m_state->m_phase = NET_LINK_DRAINING;
        m_state->m_drainStarted = nowMs;
    }
    return progress(nowMs);
}

int32_t NetInterfaceLink::status() const
{
    return m_state->status();
}

int32_t NetInterfaceLink::outputState() const
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase == NET_LINK_DRAINING)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_outputCount != 0 || m_state->m_outputPeeked)
    {
        return NI_READY;
    }
    return m_state->m_control ? m_state->m_control->outputState() : NI_INVALID_STATE;
}

// NET_LINK_DRAIN_UNIT_BEGIN
// #include "NetInterface.h"
// #include "NetInterface/NetInterfaceLinkAuditTest.h"
// #include <cstdio>
// #include <cstring>
// #include <vector>
// #if defined(_WIN32) && defined(_DEBUG)
// #include <crtdbg.h>
// #endif
//
// NetInterfaceLinkTestIdentity::NetInterfaceLinkTestIdentity() :
// m_seed(0x47b9af13)
// {
//
// }
//
// NetInterfaceLinkTestIdentity::~NetInterfaceLinkTestIdentity()
// {
//     NetInterface::wipe(m_key.empty() ? nullptr : reinterpret_cast<uint8_t*>(&m_key[0]), static_cast<uint32_t>(m_key.size()));
// }
//
// bool NetInterfaceLinkTestIdentity::load(const char* path)
// {
//     FILE* file = std::fopen(path, "rb");
//     if (file == nullptr)
//     {
//         return false;
//     }
//     uint8_t bytes[24577] = {};
//     const size_t size = std::fread(bytes, 1, sizeof(bytes), file);
//     const bool readable = std::ferror(file) == 0 && size >= 48 && size <= 24576 && std::memcmp(bytes, "NGI3", 4) == 0;
//     std::fclose(file);
//     bool valid = false;
//     if (readable)
//     {
//         uint32_t lengths[3] = {};
//         for (uint32_t index = 0; index < 3; ++index)
//         {
//             for (uint32_t offset = 0; offset < 4; ++offset)
//             {
//                 lengths[index] = (lengths[index] << 8) | bytes[4 + index * 4 + offset];
//             }
//         }
//         uint8_t digest[32] = {};
//         if (lengths[0] == 37 && lengths[1] > 0 && lengths[1] <= 4096 && lengths[2] > 0 && lengths[2] <= 16384 &&
//             static_cast<uint64_t>(lengths[0]) + lengths[1] + lengths[2] + 48 == size &&
//             NetInterface::sha256(bytes, static_cast<uint32_t>(size) - 32, digest, sizeof(digest)) == NI_OK &&
//             std::memcmp(bytes + size - 32, digest, sizeof(digest)) == 0)
//         {
//             try
//             {
//                 m_name.assign(reinterpret_cast<const char*>(bytes + 16), lengths[0]);
//                 m_key.assign(reinterpret_cast<const char*>(bytes + 16 + lengths[0]), lengths[1]);
//                 m_certificate.assign(reinterpret_cast<const char*>(bytes + 16 + lengths[0] + lengths[1]), lengths[2]);
//                 valid = true;
//             }
//             catch (...)
//             {
//                 valid = false;
//             }
//         }
//     }
//     NetInterface::wipe(bytes, sizeof(bytes));
//     return valid;
// }
//
// int32_t NetInterfaceLinkTestIdentity::entropy(void* context, uint8_t* output, uint32_t size)
// {
//     NetInterfaceLinkTestIdentity* identity = static_cast<NetInterfaceLinkTestIdentity*>(context);
//     for (uint32_t index = 0; index < size; ++index)
//     {
//         identity->m_seed ^= identity->m_seed << 13;
//         identity->m_seed ^= identity->m_seed >> 17;
//         identity->m_seed ^= identity->m_seed << 5;
//         output[index] = static_cast<uint8_t>(identity->m_seed);
//     }
//     return 0;
// }
//
// NetInterfaceLinkOptions NetInterfaceLinkTestIdentity::options(bool server)
// {
//     NetInterfaceLinkOptions value = {};
//     value.m_version = 3;
//     value.m_role = server ? NI_LINK_NODE : NI_LINK_CLIENT;
//     value.m_name = server ? m_name.c_str() : nullptr;
//     value.m_peerRoles = server ? 1U << NI_LINK_CLIENT : 0;
//     value.m_tls.server = server ? 1 : 0;
//     value.m_tls.handshakeTimeoutMs = 5000;
//     value.m_tls.certificate = server ? reinterpret_cast<const uint8_t*>(m_certificate.c_str()) : nullptr;
//     value.m_tls.certificateSize = server ? static_cast<uint32_t>(m_certificate.size() + 1) : 0;
//     value.m_tls.privateKey = server ? reinterpret_cast<const uint8_t*>(m_key.c_str()) : nullptr;
//     value.m_tls.privateKeySize = server ? static_cast<uint32_t>(m_key.size() + 1) : 0;
//     value.m_tls.entropy = entropy;
//     value.m_tls.entropyContext = this;
//     return value;
// }
//
// NetInterfaceLinkTestPair::NetInterfaceLinkTestPair(NetInterfaceLinkTestIdentity& identity) :
// m_client(identity.options(false)),
// m_server(identity.options(true))
// {
//
// }
//
// bool NetInterfaceLinkTestPair::handshake()
// {
//     for (uint32_t step = 0; step < 10000; ++step)
//     {
//         if (m_client.progress(1000) < 0 || m_server.progress(1000) < 0 ||
//             move(m_client, m_server) < 0 || move(m_server, m_client) < 0)
//         {
//             return false;
//         }
//         if (m_client.status() == NI_READY && m_server.status() == NI_READY)
//         {
//             return true;
//         }
//     }
//     return false;
// }
//
// int32_t NetInterfaceLinkTestPair::move(NetInterfaceLink& from, NetInterfaceLink& to)
// {
//     NetInterfaceTunnelRecord record = {};
//     const int32_t ready = from.peekOutput(record);
//     if (ready != NI_READY)
//     {
//         return ready;
//     }
//     const int32_t accepted = to.accept(record);
//     if (accepted != NI_OK)
//     {
//         return accepted == NI_LIMIT_EXCEEDED ? NI_NEED_MORE : accepted;
//     }
//     return from.consumeOutput();
// }
//
// #define LINK_CHECK(condition) do { ++checks; if (!(condition)) { std::printf("Link audit failed line=%d checks=%u\n", __LINE__, checks); return 1; } } while (0)
//
// /** 独立启用此main；identity.dat必须来自隔离测试，不得使用/分发正式身份或CA私钥
// @param [in] argc 参数数量
// @param [in] argv 可选身份路径；Wasm测试由测试构建嵌入到默认虚拟路径，正式库绝不嵌入
// @return 0通过，其他失败
// */
// int main(int argc, char** argv)
// {
//     uint32_t checks = 0;
//     try
//     {
//         NetInterfaceLinkTestIdentity identity;
//         LINK_CHECK(identity.load(argc > 1 ? argv[1] : "/gateway-test-identity.dat"));
//         uint8_t payload[16384];
//         for (uint32_t index = 0; index < sizeof(payload); ++index)
//         {
//             payload[index] = static_cast<uint8_t>(index * 9 + 3);
//         }
// #if defined(_WIN32) && defined(_DEBUG)
//         _CrtMemState before;
//         _CrtMemCheckpoint(&before);
// #endif
//         for (uint32_t scenario = 0; scenario < 10; ++scenario)
//         {
//             NetInterfaceLinkTestPair pair(identity);
//             LINK_CHECK(pair.handshake());
//             NetInterfaceTunnelRecord record = {};
//             record.m_kind = NI_TUNNEL_BYTES;
//             record.m_product = 7;
//             record.m_channel = 1;
//             record.m_data = payload;
//             record.m_size = scenario == 3 ? sizeof(payload) : 33;
//             if (scenario != 1 && scenario != 9)
//             {
//                 LINK_CHECK(pair.m_server.sendRecord(record) == NI_OK);
//             }
//             if (scenario == 1 || scenario == 2 || scenario == 9)
//             {
//                 NetInterfaceFrame frame = {};
//                 frame.m_kind = NI_EVENT;
//                 frame.m_messageType = 991;
//                 frame.m_payload = payload;
//                 frame.m_payloadSize = 33;
//                 LINK_CHECK(pair.m_server.sendControl(frame) == NI_OK);
//             }
//             for (uint32_t step = 0; step < 32; ++step)
//             {
//                 const int32_t moved = NetInterfaceLinkTestPair::move(pair.m_server, pair.m_client);
//                 LINK_CHECK(moved >= 0);
//                 if (moved == NI_NEED_MORE)
//                 {
//                     break;
//                 }
//             }
//             if (scenario == 8 || scenario == 9)
//             {
//                 // 本端存在大量未交付输出时，不能阻止已经到达的输入独立排空。
//                 NetInterfaceFrame outgoing = {};
//                 outgoing.m_kind = NI_REQUEST;
//                 outgoing.m_messageType = 992;
//                 outgoing.m_payload = payload;
//                 outgoing.m_payloadSize = sizeof(payload);
//                 for (uint32_t index = 0; index < 60; ++index)
//                 {
//                     outgoing.m_requestId = index + 1;
//                     LINK_CHECK(pair.m_client.sendControl(outgoing) == NI_OK);
//                 }
//                 for (uint32_t step = 0; step < 8; ++step)
//                 {
//                     LINK_CHECK(pair.m_client.progress(1000) == NI_READY);
//                 }
//             }
//             int32_t status = pair.m_client.drainInput(1001);
//             LINK_CHECK(status == NI_READY);
//             LINK_CHECK(pair.m_client.sendRecord(record) == NI_INVALID_STATE);
//             LINK_CHECK(pair.m_client.accept(record) == NI_INVALID_STATE);
//             NetInterfaceFrame blockedSend = {};
//             blockedSend.m_kind = NI_EVENT;
//             blockedSend.m_messageType = 991;
//             LINK_CHECK(pair.m_client.sendControl(blockedSend) == NI_INVALID_STATE && pair.m_client.outputState() == NI_INVALID_STATE);
//             NetInterfaceTunnelRecord forbidden = {};
//             LINK_CHECK(pair.m_client.peekOutput(forbidden) == NI_INVALID_STATE && forbidden.m_data == nullptr);
//             if (scenario == 4)
//             {
//                 for (uint32_t now = 1002; now < 6001; now += 271)
//                 {
//                     LINK_CHECK(pair.m_client.drainInput(now) == NI_READY);
//                 }
//                 LINK_CHECK(pair.m_client.drainInput(6001) == NI_TIMEOUT);
//                 continue;
//             }
//             if (scenario == 5)
//             {
//                 LINK_CHECK(pair.m_client.drainInput(1000) == NI_INVALID_ARGUMENT);
//                 continue;
//             }
//             if (scenario == 6)
//             {
//                 LINK_CHECK(pair.m_client.finishInput() == NI_TRUNCATED);
//                 continue;
//             }
//             if (scenario == 7)
//             {
//                 for (uint32_t retry = 0; retry < 16; ++retry)
//                 {
//                     NetInterfaceTunnelRecord view = {};
//                     LINK_CHECK(pair.m_client.drainInput(1001 + retry) == NI_READY && pair.m_client.peekRecord(view) == NI_READY &&
//                         view.m_size == record.m_size && std::memcmp(view.m_data, payload, view.m_size) == 0);
//                 }
//             }
//             uint32_t records = 0;
//             uint32_t controls = 0;
//             for (uint32_t step = 0; step < 32 && status >= 0; ++step)
//             {
//                 NetInterfaceFrame frame = {};
//                 if (pair.m_client.peekControl(frame) == NI_READY)
//                 {
//                     LINK_CHECK(frame.m_messageType == 991 && frame.m_payloadSize == 33 && std::memcmp(frame.m_payload, payload, 33) == 0);
//                     LINK_CHECK(pair.m_client.consumeControl() == NI_OK);
//                     ++controls;
//                 }
//                 else
//                 {
//                     NetInterfaceTunnelRecord view = {};
//                     if (pair.m_client.peekRecord(view) == NI_READY)
//                     {
//                         LINK_CHECK(view.m_size == record.m_size && view.m_product == 7 && view.m_channel == 1 &&
//                             std::memcmp(view.m_data, payload, view.m_size) == 0);
//                         LINK_CHECK(pair.m_client.consumeRecord() == NI_OK);
//                         ++records;
//                     }
//                 }
//                 status = pair.m_client.drainInput(scenario == 7 ? 1017 : 1001);
//             }
//             LINK_CHECK(status == NI_TRUNCATED && records == (scenario == 1 || scenario == 9 ? 0U : 1U) &&
//                 controls == (scenario == 1 || scenario == 2 || scenario == 9 ? 1U : 0U));
//             LINK_CHECK(pair.m_client.drainInput(7000) == NI_TRUNCATED);
//         }
//         // 最大管理帧的TLS密文逐字节交付；最后一个字节仅入输入块，随后直接EOF。
//         {
//             NetInterfaceLinkTestPair pair(identity);
//             LINK_CHECK(pair.handshake());
//             NetInterfaceFrame outgoing = {};
//             outgoing.m_kind = NI_REQUEST;
//             outgoing.m_messageType = 993;
//             outgoing.m_requestId = 7;
//             outgoing.m_payload = payload;
//             outgoing.m_payloadSize = sizeof(payload);
//             LINK_CHECK(pair.m_server.sendControl(outgoing) == NI_OK);
//             uint32_t records = 0;
//             for (uint32_t step = 0; step < 16; ++step)
//             {
//                 NetInterfaceTunnelRecord record = {};
//                 const int32_t ready = pair.m_server.peekOutput(record);
//                 if (ready != NI_READY)
//                 {
//                     LINK_CHECK(ready == NI_NEED_MORE);
//                     break;
//                 }
//                 LINK_CHECK(record.m_kind == NI_TUNNEL_CONTROL);
//                 for (uint32_t offset = 0; offset < record.m_size; ++offset)
//                 {
//                     LINK_CHECK(pair.m_client.progress(1000) == NI_READY);
//                     NetInterfaceTunnelRecord fragment = record;
//                     fragment.m_data = record.m_data + offset;
//                     fragment.m_size = 1;
//                     LINK_CHECK(pair.m_client.accept(fragment) == NI_OK);
//                 }
//                 LINK_CHECK(pair.m_server.consumeOutput() == NI_OK);
//                 ++records;
//             }
//             LINK_CHECK(records >= 2 && pair.m_server.outputState() == NI_OK);
//             LINK_CHECK(pair.m_client.drainInput(1001) == NI_READY);
//             NetInterfaceFrame incoming = {};
//             LINK_CHECK(pair.m_client.peekControl(incoming) == NI_READY && incoming.m_messageType == 993 && incoming.m_requestId == 7 &&
//                 incoming.m_payloadSize == sizeof(payload) && std::memcmp(incoming.m_payload, payload, sizeof(payload)) == 0);
//             LINK_CHECK(pair.m_client.consumeControl() == NI_OK);
//             LINK_CHECK(pair.m_client.drainInput(1001) == NI_TRUNCATED);
//         }
//         // 排空前并未认证的连接绝不因最后的挑战/证明进入ready。
//         {
//             NetInterfaceLink client(identity.options(false));
//             LINK_CHECK(client.drainInput(1000) == NI_TRUNCATED && client.progress(1001) == NI_TRUNCATED);
//         }
//         // 一次TLS密文中包含超过单轮64条预算的完整管理帧，逐条消费至全部排空。
//         {
//             NetInterfaceLinkTestPair pair(identity);
//             LINK_CHECK(pair.handshake());
//             for (uint32_t index = 0; index < 80; ++index)
//             {
//                 NetInterfaceFrame frame = {};
//                 frame.m_kind = static_cast<uint16_t>(index < 64 ? NI_REQUEST : NI_EVENT);
//                 frame.m_messageType = 991;
//                 frame.m_requestId = index + 1;
//                 frame.m_payload = payload;
//                 frame.m_payloadSize = 3;
//                 LINK_CHECK(pair.m_server.sendControl(frame) == NI_OK);
//             }
//             for (uint32_t step = 0; step < 8; ++step)
//             {
//                 LINK_CHECK(pair.m_server.progress(1000) == NI_READY);
//             }
//             LINK_CHECK(NetInterfaceLinkTestPair::move(pair.m_server, pair.m_client) == NI_OK);
//             LINK_CHECK(pair.m_server.outputState() == NI_OK);
//             uint32_t count = 0;
//             bool seen[80] = {};
//             int32_t status = pair.m_client.drainInput(1001);
//             for (uint32_t step = 0; step < 256 && status >= 0; ++step)
//             {
//                 NetInterfaceFrame frame = {};
//                 if (pair.m_client.peekControl(frame) == NI_READY)
//                 {
//                     LINK_CHECK(frame.m_requestId >= 1 && frame.m_requestId <= 80 && !seen[frame.m_requestId - 1]);
//                     seen[frame.m_requestId - 1] = true;
//                     LINK_CHECK(frame.m_payloadSize == 3 && std::memcmp(frame.m_payload, payload, 3) == 0);
//                     LINK_CHECK(pair.m_client.consumeControl() == NI_OK);
//                     ++count;
//                 }
//                 status = pair.m_client.drainInput(1001);
//             }
//             if (status != NI_TRUNCATED || count != 80)
//             {
//                 std::printf("drain80 status=%d count=%u\n", status, count);
//             }
//             LINK_CHECK(status == NI_TRUNCATED && count == 80);
//         }
// #if defined(_WIN32) && defined(_DEBUG)
//         _CrtMemState after;
//         _CrtMemState difference;
//         _CrtMemCheckpoint(&after);
//         _CrtMemDifference(&difference, &before, &after);
//         LINK_CHECK(difference.lCounts[_NORMAL_BLOCK] == 0 && difference.lSizes[_NORMAL_BLOCK] == 0);
// #endif
//         std::printf("Link input drain checks=%u PASS\n", checks);
//         return 0;
//     }
//     catch (...)
//     {
//         std::printf("Link input drain unexpected exception\n");
//         return 2;
//     }
// }
// NET_LINK_DRAIN_UNIT_END

// NET_LINK_QUEUE_UNIT_BEGIN
// #include "NetInterface.h"
// #include "NetInterface/NetInterfaceLinkAuditTest.h"
// #include <cstdio>
// #include <cstring>
// #include <vector>
// #if defined(_WIN32) && defined(_DEBUG)
// #include <crtdbg.h>
// #endif
//
// NetInterfaceLinkTestIdentity::NetInterfaceLinkTestIdentity() :
// m_seed(0x47b9af13)
// {
//
// }
//
// NetInterfaceLinkTestIdentity::~NetInterfaceLinkTestIdentity()
// {
//     NetInterface::wipe(m_key.empty() ? nullptr : reinterpret_cast<uint8_t*>(&m_key[0]), static_cast<uint32_t>(m_key.size()));
// }
//
// bool NetInterfaceLinkTestIdentity::load(const char* path)
// {
//     FILE* file = std::fopen(path, "rb");
//     if (file == nullptr)
//     {
//         return false;
//     }
//     uint8_t bytes[24577] = {};
//     const size_t size = std::fread(bytes, 1, sizeof(bytes), file);
//     const bool readable = std::ferror(file) == 0 && size >= 48 && size <= 24576 && std::memcmp(bytes, "NGI3", 4) == 0;
//     std::fclose(file);
//     bool valid = false;
//     if (readable)
//     {
//         uint32_t lengths[3] = {};
//         for (uint32_t index = 0; index < 3; ++index)
//         {
//             for (uint32_t offset = 0; offset < 4; ++offset)
//             {
//                 lengths[index] = (lengths[index] << 8) | bytes[4 + index * 4 + offset];
//             }
//         }
//         uint8_t digest[32] = {};
//         if (lengths[0] == 37 && lengths[1] > 0 && lengths[1] <= 4096 && lengths[2] > 0 && lengths[2] <= 16384 &&
//             static_cast<uint64_t>(lengths[0]) + lengths[1] + lengths[2] + 48 == size &&
//             NetInterface::sha256(bytes, static_cast<uint32_t>(size) - 32, digest, sizeof(digest)) == NI_OK &&
//             std::memcmp(bytes + size - 32, digest, sizeof(digest)) == 0)
//         {
//             try
//             {
//                 m_name.assign(reinterpret_cast<const char*>(bytes + 16), lengths[0]);
//                 m_key.assign(reinterpret_cast<const char*>(bytes + 16 + lengths[0]), lengths[1]);
//                 m_certificate.assign(reinterpret_cast<const char*>(bytes + 16 + lengths[0] + lengths[1]), lengths[2]);
//                 valid = true;
//             }
//             catch (...)
//             {
//                 valid = false;
//             }
//         }
//     }
//     NetInterface::wipe(bytes, sizeof(bytes));
//     return valid;
// }
//
// int32_t NetInterfaceLinkTestIdentity::entropy(void* context, uint8_t* output, uint32_t size)
// {
//     NetInterfaceLinkTestIdentity* identity = static_cast<NetInterfaceLinkTestIdentity*>(context);
//     for (uint32_t index = 0; index < size; ++index)
//     {
//         identity->m_seed ^= identity->m_seed << 13;
//         identity->m_seed ^= identity->m_seed >> 17;
//         identity->m_seed ^= identity->m_seed << 5;
//         output[index] = static_cast<uint8_t>(identity->m_seed);
//     }
//     return 0;
// }
//
// NetInterfaceLinkOptions NetInterfaceLinkTestIdentity::options(bool server)
// {
//     NetInterfaceLinkOptions value = {};
//     value.m_version = 3;
//     value.m_role = server ? NI_LINK_NODE : NI_LINK_CLIENT;
//     value.m_name = server ? m_name.c_str() : nullptr;
//     value.m_peerRoles = server ? 1U << NI_LINK_CLIENT : 0;
//     value.m_tls.server = server ? 1 : 0;
//     value.m_tls.handshakeTimeoutMs = 5000;
//     value.m_tls.certificate = server ? reinterpret_cast<const uint8_t*>(m_certificate.c_str()) : nullptr;
//     value.m_tls.certificateSize = server ? static_cast<uint32_t>(m_certificate.size() + 1) : 0;
//     value.m_tls.privateKey = server ? reinterpret_cast<const uint8_t*>(m_key.c_str()) : nullptr;
//     value.m_tls.privateKeySize = server ? static_cast<uint32_t>(m_key.size() + 1) : 0;
//     value.m_tls.entropy = entropy;
//     value.m_tls.entropyContext = this;
//     return value;
// }
//
// NetInterfaceLinkTestPair::NetInterfaceLinkTestPair(NetInterfaceLinkTestIdentity& identity) :
// m_client(identity.options(false)),
// m_server(identity.options(true))
// {
//
// }
//
// bool NetInterfaceLinkTestPair::handshake()
// {
//     for (uint32_t step = 0; step < 10000; ++step)
//     {
//         if (m_client.progress(1000) < 0 || m_server.progress(1000) < 0 ||
//             move(m_client, m_server) < 0 || move(m_server, m_client) < 0)
//         {
//             return false;
//         }
//         if (m_client.status() == NI_READY && m_server.status() == NI_READY)
//         {
//             return true;
//         }
//     }
//     return false;
// }
//
// int32_t NetInterfaceLinkTestPair::move(NetInterfaceLink& from, NetInterfaceLink& to)
// {
//     NetInterfaceTunnelRecord record = {};
//     const int32_t ready = from.peekOutput(record);
//     if (ready != NI_READY)
//     {
//         return ready;
//     }
//     const int32_t accepted = to.accept(record);
//     if (accepted != NI_OK)
//     {
//         return accepted == NI_LIMIT_EXCEEDED ? NI_NEED_MORE : accepted;
//     }
//     return from.consumeOutput();
// }
//
// #include <chrono>
//
// #if defined(_WIN32) && defined(_DEBUG)
// static int32_t s_linkQueueFailAfter = -1;
// static uint32_t s_linkQueueAllocations = 0;
// /** 测试窗口内计数或拒绝普通堆分配，不改变CRT内部记录
// @param [in] operation CRT分配动作
// @param [in] data 旧分配指针
// @param [in] size 新大小
// @param [in] block 分配类型
// @param [in] request 分配编号
// @param [in] file 来源
// @param [in] line 行号
// @return 1允许，0拒绝
// */
// static int __cdecl LinkQueueAllocationHook(int operation, void* data, size_t size, int block, long request,
//     const unsigned char* file, int line)
// {
//     (void)data;
//     (void)size;
//     (void)request;
//     (void)file;
//     (void)line;
//     if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
//     {
//         ++s_linkQueueAllocations;
//         if (s_linkQueueFailAfter == 0)
//         {
//             return 0;
//         }
//         if (s_linkQueueFailAfter > 0)
//         {
//             --s_linkQueueFailAfter;
//         }
//     }
//     return 1;
// }
// #endif
//
// #define LINK_QUEUE_CHECK(condition) do { ++checks; if (!(condition)) { std::printf("Link output queue failed line=%d checks=%u\n", __LINE__, checks); return 1; } } while (0)
//
// /** 真实管理认证后验证输出容量、复制/FIFO、非法空载荷、失败重试及同尺寸缓冲复用
// @param [in] argc 参数数量
// @param [in] argv 隔离identity.dat，另可指定--benchmark只跑受控提交耗时
// @return 0通过
// */
// int main(int argc, char** argv)
// {
//     uint32_t checks = 0;
//     try
//     {
//         NetInterfaceLinkTestIdentity identity;
// #ifdef __EMSCRIPTEN__
//         LINK_QUEUE_CHECK(identity.load(argc > 1 ? argv[1] : "/gateway-test-identity.dat"));
// #else
//         LINK_QUEUE_CHECK(argc >= 2 && identity.load(argv[1]));
// #endif
//         const bool benchmark = argc == 3 && std::strcmp(argv[2], "--benchmark") == 0;
// #if defined(_WIN32) && defined(_DEBUG)
//         _CrtMemState before;
//         _CrtMemCheckpoint(&before);
// #endif
//         {
//             NetInterfaceLinkTestPair pair(identity);
//             LINK_QUEUE_CHECK(pair.handshake());
//             uint8_t payload[16384];
//             for (uint32_t index = 0; index < sizeof(payload); ++index)
//             {
//                 payload[index] = static_cast<uint8_t>(index * 13 + 7);
//             }
//             NetInterfaceTunnelRecord record = {};
//             record.m_kind = NI_TUNNEL_BYTES;
//             record.m_product = 7;
//             record.m_channel = 1;
//             record.m_data = payload;
//             record.m_size = sizeof(payload);
//             NetInterfaceTunnelRecord output = {};
//             if (!benchmark)
//             {
//                 for (uint32_t round = 0; round < 17; ++round)
//                 {
//                     for (uint32_t index = 0; index < 4; ++index)
//                     {
//                         record.m_channel = 1 + round * 4 + index;
//                         LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
//                     }
//                     LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_LIMIT_EXCEEDED);
//                     for (uint32_t index = 0; index < 4; ++index)
//                     {
//                         LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_kind == NI_TUNNEL_BYTES &&
//                             output.m_channel == 1 + round * 4 + index && output.m_product == 7 && output.m_size == sizeof(payload) &&
//                             std::memcmp(output.m_data, payload, sizeof(payload)) == 0);
//                         const uint8_t* borrowed = output.m_data;
//                         LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && borrowed == output.m_data);
//                         LINK_QUEUE_CHECK(pair.m_client.consumeOutput() == NI_OK);
//                     }
//                     LINK_QUEUE_CHECK(pair.m_client.outputState() == NI_OK && pair.m_client.peekOutput(output) == NI_NEED_MORE &&
//                         pair.m_client.consumeOutput() == NI_INVALID_STATE);
//                 }
//                 // CLOSE必须带4字节原因；先验证空载荷拒绝，再验证32槽及回绕。
//                 record.m_kind = NI_TUNNEL_CLOSE;
//                 record.m_data = nullptr;
//                 record.m_size = 0;
//                 LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_INVALID_ARGUMENT && pair.m_client.outputState() == NI_OK);
//                 record.m_data = payload;
//                 record.m_size = 4;
//                 for (uint32_t round = 0; round < 3; ++round)
//                 {
//                     for (uint32_t index = 0; index < 32; ++index)
//                     {
//                         record.m_channel = 1000 + round * 32 + index;
//                         LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
//                     }
//                     LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_LIMIT_EXCEEDED);
//                     for (uint32_t index = 0; index < 32; ++index)
//                     {
//                         LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_kind == NI_TUNNEL_CLOSE &&
//                             output.m_channel == 1000 + round * 32 + index && output.m_size == 4 && std::memcmp(output.m_data, payload, 4) == 0);
//                         LINK_QUEUE_CHECK(pair.m_client.consumeOutput() == NI_OK);
//                     }
//                 }
//                 record.m_kind = NI_TUNNEL_BYTES;
//                 record.m_size = 37;
//                 record.m_data = payload;
//                 const uint8_t original = payload[0];
//                 LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
//                 payload[0] ^= 0xff;
//                 LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_data[0] == original);
//                 payload[0] = original;
//                 // 借用队首作为新输入合法；接纳不得破坏原借用或让新包引用旧存储。
//                 output.m_channel = 5000;
//                 LINK_QUEUE_CHECK(pair.m_client.sendRecord(output) == NI_OK && pair.m_client.consumeOutput() == NI_OK);
//                 LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_channel == 5000 &&
//                     output.m_size == record.m_size && std::memcmp(output.m_data, payload, record.m_size) == 0);
//                 LINK_QUEUE_CHECK(pair.m_client.consumeOutput() == NI_OK);
// #if defined(_WIN32) && defined(_DEBUG)
//                 // 换尺寸强制需要新正文；失败不占槽、不扣字节、不丢已有队首。
//                 record.m_size = 19;
//                 LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
//                 record.m_size = 911;
//                 s_linkQueueFailAfter = 0;
//                 _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(LinkQueueAllocationHook);
//                 const int32_t failed = pair.m_client.sendRecord(record);
//                 _CrtSetAllocHook(previous);
//                 s_linkQueueFailAfter = -1;
//                 LINK_QUEUE_CHECK(failed == NI_NO_MEMORY && pair.m_client.status() == NI_READY);
//                 LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_size == 19 && pair.m_client.consumeOutput() == NI_OK);
//                 LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
//                 LINK_QUEUE_CHECK(pair.m_client.peekOutput(output) == NI_READY && output.m_size == 911 && pair.m_client.consumeOutput() == NI_OK);
// #endif
//             }
//             const uint32_t sizes[] = {32, 1024, 16384};
//             for (uint32_t group = 0; group < 3; ++group)
//             {
//                 record.m_kind = NI_TUNNEL_BYTES;
//                 record.m_channel = group + 1;
//                 record.m_data = payload;
//                 record.m_size = sizes[group];
//                 LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK && pair.m_client.peekOutput(output) == NI_READY &&
//                     pair.m_client.consumeOutput() == NI_OK);
// #if defined(_WIN32) && defined(_DEBUG)
//                 s_linkQueueAllocations = 0;
//                 _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(LinkQueueAllocationHook);
// #endif
//                 const auto started = std::chrono::steady_clock::now();
//                 bool valid = true;
//                 for (uint32_t index = 0; index < 100000; ++index)
//                 {
//                     if (pair.m_client.sendRecord(record) != NI_OK || pair.m_client.peekOutput(output) != NI_READY ||
//                         output.m_size != record.m_size || output.m_channel != record.m_channel ||
//                         std::memcmp(output.m_data, payload, record.m_size) != 0 || pair.m_client.consumeOutput() != NI_OK)
//                     {
//                         valid = false;
//                         break;
//                     }
//                 }
//                 const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started).count();
// #if defined(_WIN32) && defined(_DEBUG)
//                 _CrtSetAllocHook(previous);
//                 std::printf("Link queue bytes=%u allocations=%u\n", record.m_size, s_linkQueueAllocations);
//                 LINK_QUEUE_CHECK(s_linkQueueAllocations == 0);
// #endif
//                 LINK_QUEUE_CHECK(valid && pair.m_client.outputState() == NI_OK);
//                 std::printf("Link queue bytes=%u records=100000 microseconds=%lld\n", record.m_size, static_cast<long long>(elapsed));
//             }
//             // 非空析构：剩余大正文、关闭原因及单份复用缓冲均归本实例释放。
//             record.m_size = 16384;
//             LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK && pair.m_client.sendRecord(record) == NI_OK);
//             record.m_kind = NI_TUNNEL_CLOSE;
//             record.m_data = payload;
//             record.m_size = 4;
//             LINK_QUEUE_CHECK(pair.m_client.sendRecord(record) == NI_OK);
//         }
// #if defined(_WIN32) && defined(_DEBUG)
//         _CrtMemState after;
//         _CrtMemState difference;
//         _CrtMemCheckpoint(&after);
//         _CrtMemDifference(&difference, &before, &after);
//         LINK_QUEUE_CHECK(difference.lCounts[_NORMAL_BLOCK] == 0 && difference.lSizes[_NORMAL_BLOCK] == 0);
// #endif
//         std::printf("Link output queue checks=%u PASS\n", checks);
//         return 0;
//     }
//     catch (...)
//     {
//         std::printf("Link output queue unexpected exception\n");
//         return 2;
//     }
// }
// NET_LINK_QUEUE_UNIT_END