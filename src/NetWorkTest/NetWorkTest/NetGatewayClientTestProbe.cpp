#include "NetSenderTestClient.h"
#include "NetGatewayClientTestProbe.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>

NetUnavailableServiceTestListener::NetUnavailableServiceTestListener() :
m_unexpected(false), m_timeout(false)
{

}

void NetUnavailableServiceTestListener::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    (void)generation;
    (void)maxRequests;
    (void)maxChunk;
    m_unexpected = true;
}

void NetUnavailableServiceTestListener::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
    uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    (void)generation;
    (void)requestId;
    (void)product;
    (void)type;
    (void)status;
    (void)bytes;
    (void)size;
    m_unexpected = true;
}

bool NetUnavailableServiceTestListener::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    (void)generation;
    (void)frame;
    m_unexpected = true;
    return true;
}

void NetUnavailableServiceTestListener::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)generation;
    (void)detail;
    if (reason == NI_TIMEOUT)
    {
        m_timeout = true;
    }
}

int NetGatewayClientTestProbe::unavailableMain(const std::string& target)
{
    NetUnavailableServiceTestListener listener;
    std::vector<NetGatewayAddress> seeds;
    if (NetGatewayDiscovery::parseSeeds(target, seeds) != NI_OK || seeds.size() != 1)
    {
        return 2;
    }
    NetWorkClientSession session;
    NetInterfaceTlsOptions tls = {};
    tls.handshakeTimeoutMs = 5000;
    NetInterfaceHandshakeOptions hello = {};
    hello.m_platform = 1; hello.m_capabilities = 7; hello.m_maxRequests = 8; hello.m_maxChunk = 65536; hello.m_timeoutMs = 5000;
    NetTransport::randomBytes(hello.m_device, sizeof(hello.m_device));
    if (session.connectService(77, seeds[0].m_address.c_str(), seeds[0].m_port, tls, hello, &listener) != NI_OK) { return 3; }
    const auto begin = std::chrono::steady_clock::now();
    int64_t elapsed = 0;
    bool running = true;
    while (running && !listener.m_unexpected && elapsed < 34000)
    {
        running = session.poll();
        CSystem::sleepPrecise(2);
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();
    }
    session.shutdown();
    if (running || listener.m_unexpected || !listener.m_timeout || elapsed < 29500 || elapsed >= 34000) { return 4; }
    std::printf("UNAVAILABLE_SERVICE_PASS elapsed=%lld\n", static_cast<long long>(elapsed));
    return 0;
}

NetGatewayClientTestProbe::NetGatewayClientTestProbe() :
m_sender(nullptr), m_handle(nullptr), m_senderHandle(nullptr), m_payload(65537), m_generation(0), m_request(0),
    m_ready(false), m_sent(false), m_done(false), m_failed(false), m_recovery(false), m_rounds(0), m_closedCount(0)
{
    for (uint32_t index = 0; index < m_payload.size(); ++index) { m_payload[index] = static_cast<uint8_t>(index * 31 + 19); }
}

void NetGatewayClientTestProbe::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    m_failed = m_failed || m_ready || generation <= m_generation || maxRequests == 0 || maxChunk < 1024;
    m_generation = generation;
    m_ready = true;
    m_sent = false;
    if (m_recovery)
    {
        for (uint32_t index = 0; index < m_payload.size(); ++index) { m_payload[index] = static_cast<uint8_t>(index * 31 + generation); }
    }
    uint64_t rejected = 99;
    if (m_senderHandle != nullptr)
    {
        m_failed = m_failed || NetSender::instance().destroyClient(m_senderHandle) != NI_INVALID_STATE ||
            NetSender::instance().pollGateway(m_senderHandle) != NI_INVALID_STATE;
    }
    const int32_t invalid = m_senderHandle != nullptr ? NetSender::instance().beginRequest(m_senderHandle, 2, 0x7345, 5000, rejected) :
        m_handle != nullptr ? NetPeer::instance().clientBegin(m_handle, 2, 0x7345, 5000, &rejected) :
        m_sender->beginRequest(2, 0x7345, 5000, rejected);
    const int32_t begun = m_senderHandle != nullptr ? NetSender::instance().beginRequest(m_senderHandle, 1, 0x7345, 5000, m_request) :
        m_handle != nullptr ? NetPeer::instance().clientBegin(m_handle, 1, 0x7345, 5000, &m_request) :
        m_sender->beginRequest(1, 0x7345, 5000, m_request);
    if (invalid != NI_INVALID_ARGUMENT || rejected != 0 || begun != NI_OK)
    {
        m_failed = true;
    }
}

void NetGatewayClientTestProbe::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type,
    uint32_t status, const uint8_t* bytes, uint32_t size)
{
    m_failed = m_failed || m_done || generation != m_generation || requestId != m_request || product != 1 || type != 0x7345 ||
        status != NI_RPC_SUCCESS || size != m_payload.size() || bytes == nullptr;
    if (!m_failed && std::memcmp(bytes, m_payload.data(), m_payload.size()) != 0)
    {
        m_failed = true;
    }
    if (m_failed) { std::fprintf(stderr, "probe result mismatch generation=%llu expected=%llu request=%llu expected=%llu product=%u type=%u status=%u bytes=%u expected=%u\n",
        static_cast<unsigned long long>(generation), static_cast<unsigned long long>(m_generation), static_cast<unsigned long long>(requestId),
        static_cast<unsigned long long>(m_request), product, type, status, size, static_cast<unsigned>(m_payload.size())); }
    ++m_rounds;
    m_done = !m_recovery || m_rounds >= 2;
    if (m_recovery && !m_failed)
    {
        std::printf("RECOVERY_ROUND count=%u generation=%llu request=%llu\n", m_rounds,
            static_cast<unsigned long long>(generation), static_cast<unsigned long long>(requestId));
        std::fflush(stdout);
    }
}

bool NetGatewayClientTestProbe::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    (void)generation;
    (void)frame;
    m_failed = true;
    return true;
}

void NetGatewayClientTestProbe::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)detail;
    if (m_recovery)
    {
        if (reason != 0) { ++m_closedCount; }
        std::printf("RECOVERY_CLOSED generation=%llu reason=%d detail=%d\n",
            static_cast<unsigned long long>(generation), reason, detail);
        std::fflush(stdout);
        m_ready = false;
        if (reason == NI_NOT_SUPPORTED || reason == NI_INVALID_ARGUMENT || (reason == 0 && !m_done)) { m_failed = true; }
        return;
    }
    m_failed = m_failed || !m_done || generation != m_generation || reason != 0;
    if (m_failed) { std::fprintf(stderr, "desktop service client closed: reason=%d detail=%d ready=%u sent=%u done=%u\n",
        reason, detail, m_ready ? 1U : 0U, m_sent ? 1U : 0U, m_done ? 1U : 0U); }
}

int NetGatewayClientTestProbe::childMain(int32_t pid)
{
    NetGatewayClientTestProbe probe;
    NetSenderTestClient<NetSender, NetSenderClient>& sender = NetSenderTestClient<NetSender, NetSenderClient>::instance();
    probe.m_sender = &sender;
    if (sender.prepareGateway() != NI_OK) { return 61; }
    int result = 63;
    try
    {
        NetSenderGatewayOptions options = {};
        options.m_localGatewayPid = 0;
        if (NetSenderDesktopBackend::configureGateway(sender, pid) != NI_OK)
        {
            throw std::runtime_error("desktop gateway backend configuration");
        }
        options.m_network.structSize = sizeof(options.m_network);
        options.m_network.version = 1;
        options.m_network.platform = 1;
        options.m_network.capabilities = 7;
        options.m_network.maxRequests = 8;
        options.m_network.maxChunk = 65536;
        options.m_network.handshakeTimeoutMs = 5000;
        options.m_network.tls.handshakeTimeoutMs = 5000;
        if (NetTransport::randomBytes(options.m_network.device, sizeof(options.m_network.device)) == NI_OK &&
            sender.connectServiceGateway(1, options, &probe) == NI_OK)
        {
            const auto started = std::chrono::steady_clock::now();
            while (!probe.m_done && !probe.m_failed &&
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count() < 9000)
            {
                sender.pollGateway();
                if (probe.m_ready && !probe.m_sent && !probe.m_failed)
                {
                    const int32_t sent = sender.sendRequest(probe.m_request, probe.m_payload.data(), static_cast<uint32_t>(probe.m_payload.size()));
                    if (sent == NI_OK) { probe.m_sent = true; }
                    else if (sent != NI_LIMIT_EXCEEDED) { probe.m_failed = true; }
                }
                CSystem::Sleep(1);
            }
            result = probe.m_done && !probe.m_failed ? 0 : 64;
        }
    }
    catch (...) { }
    sender.uninitGateway();

    while (sender.pollGateway()) { CSystem::sleepPrecise(1); }
    sender.releaseGateway();

    if (result == 0 && !probe.m_failed) { std::puts("DESKTOP_SERVICE_CLIENT_PASS"); return 0; }
    return result == 0 ? 65 : result;
}

int NetGatewayClientTestProbe::facadeMain(int32_t pid)
{
    NetGatewayClientTestProbe probe;
    NetSender& sender = NetSender::instance();
    NetSenderClient* second = nullptr;
    if (sender.createClient(probe.m_senderHandle) != NI_OK) { return 67; }
    bool passed = sender.createClient(second) == NI_OK && second != nullptr;
    passed = sender.destroyClient(second) == NI_OK && passed;
    passed = sender.prepareGateway(probe.m_senderHandle) == NI_OK && passed;
    const uint32_t publicType = 0x7345;
    passed = passed && sender.configureCompression(probe.m_senderHandle, &publicType, 1) == NI_OK;
    NetSenderGatewayOptions options = {};
    options.m_network.structSize = sizeof(options.m_network);
    options.m_network.version = 1;
    options.m_network.platform = 1;
    options.m_network.capabilities = 7;
    options.m_network.maxRequests = 8;
    options.m_network.maxChunk = 65536;
    options.m_network.handshakeTimeoutMs = 5000;
    options.m_network.tls.handshakeTimeoutMs = 5000;
    passed = passed && NetTransport::randomBytes(options.m_network.device, sizeof(options.m_network.device)) == NI_OK &&
        NetSenderDesktopBackend::configureGateway(probe.m_senderHandle, pid) == NI_OK &&
        sender.connectServiceGateway(probe.m_senderHandle, 1, options, &probe) == NI_OK;
    const auto started = std::chrono::steady_clock::now();
    while (passed && !probe.m_done && !probe.m_failed &&
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count() < 9000)
    {
        passed = sender.pollGateway(probe.m_senderHandle) >= 0;
        if (probe.m_ready && !probe.m_sent && !probe.m_failed)
        {
            const int32_t sent = sender.postRequest(probe.m_senderHandle, probe.m_request, probe.m_payload.data(),
                static_cast<uint32_t>(probe.m_payload.size()));
            if (sent == NI_OK) { probe.m_sent = true; }
            else if (sent != NI_LIMIT_EXCEEDED) { probe.m_failed = true; }
        }
        CSystem::Sleep(1);
    }
    uint64_t generation = 0;
    bool ready = false;
    const int32_t observed = sender.gatewayState(probe.m_senderHandle, generation, ready);
    if (!passed || !probe.m_done || probe.m_failed || observed != NI_OK || ready != 1 || generation != probe.m_generation)
    { std::fprintf(stderr, "C++ facade before-close passed=%u done=%u failed=%u state=%d ready=%d generation=%llu expected=%llu\n", passed ? 1U : 0U,
        probe.m_done ? 1U : 0U, probe.m_failed ? 1U : 0U, observed, ready, static_cast<unsigned long long>(generation), static_cast<unsigned long long>(probe.m_generation)); }
    passed = passed && probe.m_done && !probe.m_failed && observed == NI_OK && ready == 1 && generation == probe.m_generation;
    sender.closeGateway(probe.m_senderHandle, 0);
    while (sender.pollGateway(probe.m_senderHandle) == NI_READY) { CSystem::sleepPrecise(1); }
    const int32_t destroyed = sender.destroyClient(probe.m_senderHandle);
    if (destroyed != NI_OK || probe.m_failed) { std::fprintf(stderr, "C++ facade destroy=%d failed=%u\n", destroyed, probe.m_failed ? 1U : 0U); }
    passed = destroyed == NI_OK && passed && !probe.m_failed;
    probe.m_senderHandle = nullptr;
    // 远端持有的旧共享内存映射在其后续poll才释放；资源尚占用时prepareGateway明确拒绝，测试有界重试。
    int32_t recreated = sender.createClient(second) == NI_OK ? NI_INVALID_STATE : NI_NO_MEMORY;
    const auto recreateStart = std::chrono::steady_clock::now();
    while (recreated == NI_INVALID_STATE && std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - recreateStart).count() < 3000)
    {
        recreated = sender.prepareGateway(second);
        if (recreated == NI_INVALID_STATE) { CSystem::sleepPrecise(1); }
    }
    const int32_t redestroyed = sender.destroyClient(second);
    if (recreated != NI_OK || redestroyed != NI_OK) { std::fprintf(stderr, "C++ facade recreate=%d destroy=%d\n", recreated, redestroyed); passed = false; }
    if (!passed) { return 68; }
    std::printf("CPP_SENDER_CLIENT_PASS automatic=%u\n", pid == 0 ? 1U : 0U);
    return 0;
}

int NetGatewayClientTestProbe::recoveryMain(const std::string& target, bool desktop)
{
    NetGatewayClientTestProbe probe;
    probe.m_recovery = true;
    NetSenderTestClient<NetSender, NetSenderClient>& sender = NetSenderTestClient<NetSender, NetSenderClient>::instance();
    int32_t started = NI_INVALID_ARGUMENT;
    if (desktop)
    {
        probe.m_sender = &sender;
        if (target.empty() || target.find_first_not_of("0123456789") != std::string::npos || target.size() > 10) { return 2; }
        uint64_t pid = 0;
        for (char digit : target) { pid = pid * 10 + digit - '0'; }
        if (pid > INT32_MAX || sender.prepareGateway() != NI_OK) { return 2; }
        {
            NetSenderGatewayOptions options = {};
            options.m_localGatewayPid = 0;
            if (NetSenderDesktopBackend::configureGateway(sender, static_cast<int32_t>(pid)) != NI_OK)
            {
                throw std::runtime_error("desktop gateway backend configuration");
            }
            options.m_network.structSize = sizeof(options.m_network); options.m_network.version = 1;
            options.m_network.platform = 1; options.m_network.capabilities = 7; options.m_network.maxRequests = 8;
            options.m_network.maxChunk = 65536; options.m_network.handshakeTimeoutMs = 5000; options.m_network.tls.handshakeTimeoutMs = 5000;
            NetTransport::randomBytes(options.m_network.device, sizeof(options.m_network.device));
            started = sender.connectServiceGateway(1, options, &probe);
        }
    }
    else
    {
        std::vector<NetGatewayAddress> seeds;
        if (NetGatewayDiscovery::parseSeeds(target, seeds) != NI_OK || seeds.size() != 1)
        {
            return 2;
        }
        NetWorkClientCallbacks callbacks = {};
        callbacks.structSize = sizeof(callbacks); callbacks.version = 1; callbacks.context = &probe;
        // 借用context指向的探针；generation为当前代次，requests/chunk为协商额度；仅更新观察值
        callbacks.ready = [](void* context, uint64_t generation, uint32_t requests, uint32_t chunk) {
            static_cast<NetGatewayClientTestProbe*>(context)->onSessionReady(generation, requests, chunk); };
        // 借用context及bytes至返回；generation/id/product/type/status按独立预期核对，size为正文长度
        callbacks.result = [](void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type,
            uint32_t status, const uint8_t* bytes, uint32_t size) {
            static_cast<NetGatewayClientTestProbe*>(context)->onSessionResult(generation, id, product, type, status, bytes, size); };
        // 借用context和frame至返回；返回1已消费、0要求保留；探针另行记录是否异常
        callbacks.frame = [](void* context, uint64_t generation, const NetInterfaceFrame* frame) -> int32_t {
            return static_cast<NetGatewayClientTestProbe*>(context)->onSessionFrame(generation, *frame) ? 1 : 0; };
        // 借用context至返回；generation为关闭代次，reason/detail为关闭原因及底层诊断
        callbacks.closed = [](void* context, uint64_t generation, int32_t reason, int32_t detail) {
            static_cast<NetGatewayClientTestProbe*>(context)->onSessionClosed(generation, reason, detail); };
        if (NetPeer::instance().clientCreate(&callbacks, &probe.m_handle) == NI_OK)
        {
            NetWorkClientOptions options = {};
            options.structSize = sizeof(options); options.version = 1; options.address = seeds[0].m_address.c_str(); options.port = seeds[0].m_port;
            options.platform = 2; options.capabilities = 7; options.maxRequests = 8; options.maxChunk = 65536;
            options.handshakeTimeoutMs = 5000; options.tls.handshakeTimeoutMs = 5000;
            NetTransport::randomBytes(options.device, sizeof(options.device));
            started = NetPeer::instance().clientConnectService(probe.m_handle, 1, &options);
        }
    }
    const auto begin = std::chrono::steady_clock::now();
    while (started == NI_OK && !probe.m_failed && !probe.m_done &&
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count() < 25000)
    {
        if (desktop) { sender.pollGateway(); } else { NetPeer::instance().clientPoll(probe.m_handle); }
        if (probe.m_ready && !probe.m_sent && !probe.m_failed)
        {
            const int32_t sent = desktop ? sender.sendRequest(probe.m_request, probe.m_payload.data(), static_cast<uint32_t>(probe.m_payload.size())) :
                NetPeer::instance().clientSend(probe.m_handle, probe.m_request, probe.m_payload.data(), static_cast<uint32_t>(probe.m_payload.size()));
            if (sent == NI_OK) { probe.m_sent = true; }
            else if (sent != NI_LIMIT_EXCEEDED) { probe.m_failed = true; }
        }
        CSystem::Sleep(1);
    }
    if (desktop)
    {
        sender.uninitGateway();

        while (sender.pollGateway()) { CSystem::sleepPrecise(1); }
        sender.releaseGateway();

    }
    else
    {
        NetPeer::instance().clientClose(probe.m_handle, 0);
        while (NetPeer::instance().clientPoll(probe.m_handle) == NI_READY) { CSystem::sleepPrecise(1); }
        NetPeer::instance().clientDestroy(probe.m_handle); probe.m_handle = nullptr;
    }
    if (started != NI_OK || probe.m_failed || !probe.m_done || probe.m_rounds != 2 || probe.m_closedCount == 0) { return 66; }
    std::printf("RECOVERY_CLIENT_PASS desktop=%u generations=%llu closed=%u\n", desktop ? 1U : 0U,
        static_cast<unsigned long long>(probe.m_generation), probe.m_closedCount);
    return 0;
}