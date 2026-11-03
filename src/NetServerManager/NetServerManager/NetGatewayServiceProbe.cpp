#include "NetGatewayServiceProbe.h"
#include "NetServerEnrollment.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <chrono>
#include <cstdio>
#include <cstring>

NetGatewayServiceProbe::NetGatewayServiceProbe() : m_service(nullptr), m_request(0), m_product(1), m_round(0),
    m_internalTest(false), m_sessionReady(false), m_sent(false), m_done(false), m_ready(false), m_failed(false)
{

}

void NetGatewayServiceProbe::onGatewayServiceReady()
{
    m_failed = m_failed || m_ready;
    m_ready = true;
    std::printf("SERVICE_PROBE_READY pid=%u\n", static_cast<uint32_t>(CSystem::currentProcessPid()));
    std::fflush(stdout);
}

void NetGatewayServiceProbe::onGatewayClientReady(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin)
{
    if (channel == 0 || (m_product == 0 ? role != NI_LINK_SERVICE : role != NI_LINK_CLIENT) ||
        origin.m_endpoint == 0 || origin.m_platform == 0)
    {
        std::fprintf(stderr, "SERVICE_PROBE_REJECT line=%d channel=%llu role=%u\n", __LINE__, static_cast<unsigned long long>(channel), role);
        m_failed = true;
        m_service->close(channel, NI_PROTOCOL_ERROR);
    }
}

bool NetGatewayServiceProbe::onGatewayClientFrame(uint64_t channel, uint32_t role, const NetInterfaceFrame& frame,
    const NetInterfaceOrigin& origin)
{
    (void)origin;
    NetInterfaceRpcView rpc = {};
    if ((m_product == 0 ? role != NI_LINK_SERVICE : role != NI_LINK_CLIENT) || frame.m_kind != NI_REQUEST || frame.m_productId != 1 ||
        NetInterfaceRpc::parse(frame, rpc) != NI_OK || rpc.m_parent != 0)
    {
        std::fprintf(stderr, "SERVICE_PROBE_REJECT line=%d channel=%llu role=%u\n", __LINE__, static_cast<unsigned long long>(channel), role);
        m_failed = true;
        m_service->close(channel, NI_PROTOCOL_ERROR);
        return true;
    }
    if (frame.m_messageType == 0x7350)
    {
        if (m_stalled.insert(channel).second)
        {
            std::printf("SERVICE_PROBE_STALLED channel=%llu\n", static_cast<unsigned long long>(channel));
            std::fflush(stdout);
        }
        return false;
    }
    uint32_t required = 0;
    if (NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, rpc.m_data, rpc.m_size, nullptr, 0, required) != NI_BUFFER_TOO_SMALL)
    {
        m_failed = true;
        return true;
    }
    std::vector<uint8_t> bytes(required);
    if (NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, rpc.m_data, rpc.m_size, bytes.data(), required, required) != NI_OK)
    {
        m_failed = true;
        return true;
    }
    NetInterfaceFrame reply = frame;
    reply.m_kind = NI_RESPONSE;
    reply.m_payload = bytes.data();
    reply.m_payloadSize = required;
    const int32_t sent = m_service->send(channel, reply);
    if (sent == NI_LIMIT_EXCEEDED) { return false; }
    if (sent != NI_OK)
    {
        std::fprintf(stderr, "SERVICE_PROBE_SEND_FAILED channel=%llu result=%d\n", static_cast<unsigned long long>(channel), sent);
        m_failed = true;
        return true;
    }
    std::printf("SERVICE_PROBE_ECHO pid=%u size=%u\n", static_cast<uint32_t>(CSystem::currentProcessPid()), rpc.m_size);
    std::fflush(stdout);
    return true;
}

void NetGatewayServiceProbe::onGatewayClientClosed(uint64_t channel, int32_t reason)
{
    if (m_stalled.erase(channel) != 0)
    {
        std::printf("SERVICE_PROBE_STALL_CLOSED reason=%d\n", reason);
        std::fflush(stdout);
    }
}

void NetGatewayServiceProbe::onGatewayServiceClosed(int32_t reason)
{
    if (reason != 0)
    {
        std::fprintf(stderr, "SERVICE_PROBE_CLOSED reason=%d\n", reason);
        m_failed = true;
    }
}

void NetGatewayServiceProbe::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    if (generation == 0 || maxRequests == 0 || maxChunk < 1024 || m_sessionReady ||
        m_session->beginRequest(1, 0x7346, 5000, m_request) != NI_OK) { m_failed = true; return; }
    m_sessionReady = true;
}

void NetGatewayServiceProbe::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type,
    uint32_t status, const uint8_t* bytes, uint32_t size)
{
    if (generation == 0 || requestId != m_request || product != 1 || type != 0x7346 || status != NI_RPC_SUCCESS ||
        m_done || bytes == nullptr || size != m_payload.size() || std::memcmp(bytes, m_payload.data(), size) != 0)
    {
        m_failed = true;
    }
    m_done = true;
}

bool NetGatewayServiceProbe::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    (void)generation;
    (void)frame;
    m_failed = true;
    return true;
}

void NetGatewayServiceProbe::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)generation;
    if (!m_done || reason != 0)
    {
        m_failed = true;
        std::fprintf(stderr, "Internal service client failed: reason=%d detail=%d round=%u\n", reason, detail, m_round);
    }
}

void NetGatewayServiceProbe::pollInternal()
{
    if (!m_internalTest || !m_ready || m_failed || m_round >= 2) { return; }
    if (!m_session)
    {
        m_payload.resize(65537 + m_round * 13107);
        for (uint32_t index = 0; index < m_payload.size(); ++index) { m_payload[index] = static_cast<uint8_t>(index * 29 + m_round); }
        m_transport.reset(new NetSenderServiceOutgoingTransport(*m_service));
        m_channel.reset(new NetGatewayClientChannel(0, m_transport.get()));
        m_session.reset(new NetSession());
        NetInterfaceTlsOptions tls = {};
        tls.entropy = CSystem::secureRandomBytesCallback;
        tls.handshakeTimeoutMs = 5000;
        NetInterfaceHandshakeOptions hello = {};
        hello.m_platform = 1;
        hello.m_capabilities = 7;
        hello.m_maxRequests = 8;
        hello.m_maxChunk = 65536;
        hello.m_timeoutMs = 5000;
        if (NetInterface::entropyBytes(CSystem::secureRandomBytesCallback, nullptr, hello.m_device, sizeof(hello.m_device)) != NI_OK ||
            m_session->bindChannel(m_channel.get()) != NI_OK || m_session->connect("local", 1, tls, hello, this) != NI_OK)
        {
            m_failed = true;
            return;
        }
    }
    m_session->poll();
    if (m_sessionReady && !m_sent && !m_failed)
    {
        const int32_t result = m_session->sendRequest(m_request, m_payload.data(), static_cast<uint32_t>(m_payload.size()));
        if (result == NI_OK) { m_sent = true; }
        else if (result != NI_LIMIT_EXCEEDED) { m_failed = true; }
    }
    if (m_done && !m_failed)
    {
        m_session->shutdown();
        m_session.reset();
        m_channel.reset();
        m_transport.reset();
        m_sessionReady = false;
        m_sent = false;
        m_done = false;
        ++m_round;
        if (m_round == 2 && !m_failed) { std::puts("SERVICE_PROBE_INTERNAL_PASS rounds=2"); std::fflush(stdout); }
    }
}

int NetGatewayServiceProbe::run(int32_t pid, const std::string& stopFile, uint32_t duration, uint32_t product, bool internalTest)
{
    if (pid <= 0 || duration == 0 || duration > MaximumDurationMs || stopFile.empty() ||
        CSystem::fileExists(CStringManager::Utf8ToUnicode(stopFile)))
    {
        return 2;
    }
    NetSenderLocalInbox inbox;
    NetGatewayServiceProbe probe;
    NetSenderServiceGateway service(inbox);
    probe.m_service = &service;
    probe.m_product = product;
    probe.m_internalTest = internalTest;
    ProcessWork::instance().addProcessReceiveCallback(&inbox);
    if (!ProcessWork::instance().initReceive(16452, 64))
    {
        ProcessWork::instance().uninitReceive();
        return 1;
    }
    int result = 1;
    try
    {
        const uint32_t publicType = 0x7345;
        if (service.configureCompression(&publicType, 1) == NI_OK &&
            service.connect(pid, product, NetServerEnrollment::proof, nullptr, &probe, 8) == NI_OK)
        {
            const auto started = std::chrono::steady_clock::now();
            bool paused = false;
            while (!probe.m_failed &&
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count() < duration &&
                !CSystem::fileExists(CStringManager::Utf8ToUnicode(stopFile)))
            {
                // Test-only gate: queue real IPC CLOSE/OPEN before one service poll.
                const bool pause = CSystem::fileExists(CStringManager::Utf8ToUnicode(stopFile + ".pause"));
                if (pause && !paused) { std::puts("SERVICE_PROBE_PAUSED"); std::fflush(stdout); }
                paused = pause;
                if (pause) { CSystem::sleepPrecise(1); continue; }
                if (!service.poll()) { break; }
                probe.pollInternal();
                CSystem::sleepPrecise(1);
            }
            result = probe.m_ready && !probe.m_failed && (!internalTest || probe.m_round == 2) ? 0 : 1;
        }
    }
    catch (...) { }
    if (probe.m_session) { probe.m_session->shutdown(); }
    probe.m_session.reset();
    probe.m_channel.reset();
    probe.m_transport.reset();
    service.shutdown();
    ProcessWork::instance().uninitReceive();
    return result;
}