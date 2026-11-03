#include "NetWorkConcurrencyTestCases.h"
#include <LibuvTcp/LibuvTcpAPI.h>
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

NetWorkConcurrencySink::NetWorkConcurrencySink() : m_verifyOrder(false)
{
    std::memset(m_next, 0, sizeof(m_next));
}

int32_t NetWorkConcurrencySink::send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin)
{
    (void)origin;
    NetWorkConcurrencyMessage copy = {};
    copy.m_endpoint = endpoint; copy.m_frame = frame;
    if (frame.m_payloadSize != 0) { copy.m_bytes.assign(frame.m_payload, frame.m_payload + frame.m_payloadSize); }
    m_messages.push_back(std::move(copy));
    return NI_OK;
}

void NetWorkConcurrencySink::close(uint64_t endpoint)
{
    (void)endpoint;
}

bool NetWorkConcurrencySink::respond(NetPeerEndpointRouter& router)
{
    for (size_t i = 0; i < m_messages.size(); ++i)
    {
        NetWorkConcurrencyMessage& message = m_messages[i];
        if (message.m_frame.m_kind != NI_REQUEST) { continue; }
        message.m_frame.m_payload = message.m_bytes.data();
        NetInterfaceRpcView rpc = {};
        if (NetInterfaceRpc::parse(message.m_frame, rpc) != NI_OK || rpc.m_size != (m_verifyOrder ? 3U : 1U) || rpc.m_data[0] != 7)
        {
            return false;
        }
        if (m_verifyOrder)
        {
            const uint32_t producer = rpc.m_data[1];
            if (producer >= 4 || rpc.m_data[2] != m_next[producer]++) { return false; }
        }
        std::vector<uint8_t> payload(rpc.m_size + 16); uint32_t required = 0;
        if (NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, rpc.m_data, rpc.m_size, payload.data(), static_cast<uint32_t>(payload.size()), required) != NI_OK)
        {
            return false;
        }
        message.m_frame.m_kind = NI_RESPONSE; message.m_frame.m_payload = payload.data(); message.m_frame.m_payloadSize = required;
        const int32_t result = router.receiveService(message.m_endpoint, message.m_frame);
        if (result != NI_OK && result != NI_NOT_FOUND) { return false; }
    }
    m_messages.clear();
    return true;
}

NetWorkConcurrencyContext::NetWorkConcurrencyContext() :
m_client(nullptr), m_mainThread(CSystem::SystemThreadId()), m_callbackThread(0), m_generation(0), m_ready(0), m_closed(0), m_done(0), m_results(0), m_bad(false), m_throwReady(false), m_verifyOrder(false)
{

}

void NetWorkConcurrencyContext::ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
{
    NetWorkConcurrencyContext* test = static_cast<NetWorkConcurrencyContext*>(context);
    test->m_bad = test->m_bad.load() || generation == 0 || requests != 32 || chunk != 4096 || CSystem::SystemThreadId() == test->m_mainThread ||
        NetWork::instance().clientDestroy(test->m_client) != NI_INVALID_STATE || NetWork::instance().clientPoll(test->m_client) != NI_INVALID_STATE ||
        NetWork::instance().clientSend(test->m_client, 0, nullptr, 0) != NI_INVALID_STATE;
    test->m_callbackThread = CSystem::SystemThreadId();
    test->m_generation = generation;
    ++test->m_ready;
    if (test->m_throwReady)
    {
        throw std::runtime_error("controlled ready callback failure");
    }
}

void NetWorkConcurrencyContext::result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    NetWorkConcurrencyContext* test = static_cast<NetWorkConcurrencyContext*>(context);
    if (generation != test->m_generation.load() || product != 101 || type != 700 || CSystem::SystemThreadId() != test->m_callbackThread.load() ||
        (status != NI_RPC_SUCCESS && status != NI_RPC_CANCELLED) || (status == NI_RPC_SUCCESS && (size != (test->m_verifyOrder ? 3U : 1U) || bytes == nullptr || bytes[0] != 7))) { test->m_bad = true; }
    if (test->m_bad.load())
    {
        std::printf("CONCURRENT_RESULT_FAILURE id=%llu generation=%llu current=%llu status=%u size=%u byte=%u product=%u type=%u\n",
            static_cast<unsigned long long>(id), static_cast<unsigned long long>(generation),
            static_cast<unsigned long long>(test->m_generation.load()), status, size,
            bytes == nullptr || size == 0 ? 0U : bytes[0], product, type);
    }
    {
        std::lock_guard<std::mutex> lock(test->m_mutex);
        if (!test->m_terminal.insert(std::make_pair(id, status)).second)
        {
            test->m_bad = true;
        }
    }
    ++test->m_results;
}

int32_t NetWorkConcurrencyContext::frame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
{
    (void)generation; (void)frame; static_cast<NetWorkConcurrencyContext*>(context)->m_bad = true; return 1;
}

void NetWorkConcurrencyContext::closed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    (void)detail;
    NetWorkConcurrencyContext* test = static_cast<NetWorkConcurrencyContext*>(context);
    if (generation != test->m_generation.load() || reason != (test->m_throwReady ? NI_INVALID_STATE : NI_OK) || CSystem::SystemThreadId() != test->m_callbackThread.load())
    {
        std::printf("CONCURRENT_CLOSE_FAILURE reason=%d detail=%d generation=%llu\n", reason, detail, static_cast<unsigned long long>(generation));
        test->m_bad = true;
    }
    ++test->m_closed;
}

NetWorkConcurrencyProducer::NetWorkConcurrencyProducer(NetWorkConcurrencyContext* context, uint32_t producer) : m_context(context), m_producer(producer)
{

}

void NetWorkConcurrencyProducer::DoTask()
{
    for (uint32_t i = 0; i < 8; ++i)
    {
        uint64_t id = 0; uint8_t bytes[3] = {7, static_cast<uint8_t>(m_producer), static_cast<uint8_t>(i)};
        const uint32_t size = m_context->m_verifyOrder ? 3 : 1;
        if (NetWork::instance().clientBegin(m_context->m_client, 101, 700, 5000, &id) != NI_OK || id == 0) { m_context->m_bad = true; break; }
        int32_t submitted = NI_LIMIT_EXCEEDED;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (submitted == NI_LIMIT_EXCEEDED && std::chrono::steady_clock::now() < deadline)
        {
            const uint64_t sendStarted = NetWorkConcurrencyTestCases::microseconds();
            // 不同生产者混合同步交付与异步投递，等待不能持有协议提交门。
            submitted = i % 2 == 0 ? NetWork::instance().clientSend(m_context->m_client, id, bytes, size) :
                NetWork::instance().clientPost(m_context->m_client, id, bytes, size);
            if (submitted == NI_OK && i % 2 == 0)
            {
                const uint32_t micros = static_cast<uint32_t>(NetWorkConcurrencyTestCases::microseconds() - sendStarted);
                std::lock_guard<std::mutex> lock(m_context->m_mutex);
                m_context->m_sendMicros.push_back(micros);
            }
            if (submitted == NI_LIMIT_EXCEEDED) { CSystem::sleepPrecise(1); }
        }
        if (submitted != NI_OK)
        {
            std::printf("CONCURRENT_SUBMISSION_FAILURE index=%u id=%llu result=%d\n", i, static_cast<unsigned long long>(id), submitted);
            m_context->m_bad = true; break;
        }
        std::memset(bytes, 99, sizeof(bytes));
        if (!m_context->m_verifyOrder && i % 3 == 0)
        {
            const int32_t cancelled = NetWork::instance().clientCancel(m_context->m_client, id);
            if (cancelled != NI_OK && cancelled != NI_NOT_FOUND) { m_context->m_bad = true; }
        }
        std::lock_guard<std::mutex> lock(m_context->m_mutex);
        m_context->m_accepted.push_back(id);
    }
    ++m_context->m_done;
}

NetWorkConcurrencyOwner::NetWorkConcurrencyOwner(NetWorkConcurrencyContext& context) : m_context(&context)
{
    std::memset(m_producers, 0, sizeof(m_producers));
}

NetWorkConcurrencyOwner::~NetWorkConcurrencyOwner()
{
    for (uint32_t index = 0; index < 4; ++index)
    {
        if (m_producers[index] != 0) { CTaskThreadManager::Instance().Uninit(m_producers[index]); }
    }
    if (m_context->m_client != nullptr)
    {
        NetWork::instance().clientClose(m_context->m_client, 0);
        while (NetWork::instance().clientPoll(m_context->m_client) != NI_OK || NetWork::instance().clientDestroy(m_context->m_client) != NI_OK)
        {
            CSystem::sleepPrecise(1);
        }
        m_context->m_client = nullptr;
    }
}

void NetWorkConcurrencyTestCases::threadedClients()
{
    runThreadedClients(false);
}

void NetWorkConcurrencyTestCases::threadedOrder()
{
    runThreadedClients(true);
}

void NetWorkConcurrencyTestCases::runThreadedClients(bool verifyOrder)
{
    uint32_t checks = 0;
#define THREAD_CHECK(condition) { netTestCheckpoint(); ++checks; if (!(condition)) { throw std::runtime_error("NetWork concurrent check: " #condition); } }
    const int32_t baseline = CTaskThreadManager::Instance().Count();
    NetWorkConcurrencyContext test;
    test.m_verifyOrder = verifyOrder;
    NetInterfaceClientCallbacks callbacks = {};
    callbacks.structSize = sizeof(callbacks); callbacks.version = 1; callbacks.context = &test;
    callbacks.ready = test.ready; callbacks.result = test.result; callbacks.frame = test.frame; callbacks.closed = test.closed;
    NetWorkConcurrencySink sink;
    sink.m_verifyOrder = verifyOrder;
    NetPeerEndpointRouter gateway(&sink);
    NetWorkConcurrencyOwner owner(test);
    THREAD_CHECK(NetWork::instance().clientCreate(&callbacks, &test.m_client) == NI_OK);
    NetInterfaceTlsOptions tls = {};
    tls.server = 1; tls.certificate = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_crt_rsa_sha256_pem);
    tls.certificateSize = static_cast<uint32_t>(mbedtls_test_srv_crt_rsa_sha256_pem_len);
    tls.privateKey = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_key_rsa_pem); tls.privateKeySize = static_cast<uint32_t>(mbedtls_test_srv_key_rsa_pem_len);
    tls.handshakeTimeoutMs = 5000;
    THREAD_CHECK(gateway.listen("127.0.0.1", 0, tls) == NI_OK);
    uint64_t service = 0;
    THREAD_CHECK(gateway.addService(101, false, 128, service) == NI_OK);
    NetInterfaceClientOptions options = {};
    options.structSize = sizeof(options); options.version = 1; options.address = "127.0.0.1"; options.port = gateway.localPort();
    options.platform = 1; options.capabilities = 7; options.maxRequests = 32; options.maxChunk = 4096; options.handshakeTimeoutMs = 5000;
    std::memset(options.device, 0x31, 16);
    options.tls.handshakeTimeoutMs = 5000; options.tls.hostname = "localhost";
    options.tls.authority = reinterpret_cast<const uint8_t*>(mbedtls_test_ca_crt_rsa_sha256_pem);
    options.tls.authoritySize = static_cast<uint32_t>(mbedtls_test_ca_crt_rsa_sha256_pem_len);
    for (uint32_t round = 0; round < 2; ++round)
    {
        std::memset(sink.m_next, 0, sizeof(sink.m_next));
        THREAD_CHECK(NetWork::instance().clientConnect(test.m_client, &options) == NI_OK);
        const uint64_t deadline = NetSenderSubmission::now() + 15000;
        while (test.m_ready.load() <= round && NetSenderSubmission::now() < deadline && !test.m_bad.load())
        {
            gateway.poll(); CSystem::sleepPrecise(1);
        }
        THREAD_CHECK(test.m_ready.load() == round + 1 && !test.m_bad.load());
        const uint64_t generation = test.m_generation.load();
        THREAD_CHECK(generation == round + 1);
        for (uint32_t i = 0; i < 4; ++i)
        {
            owner.m_producers[i] = CTaskThreadManager::Instance().Init();
            std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(owner.m_producers[i]);
            THREAD_CHECK(thread != nullptr);
            thread->PostTask(std::shared_ptr<CTask>(new NetWorkConcurrencyProducer(&test, i)));
        }
        // 此处刻意不调用clientPoll；实际工作必须由CTask独立驱动。
        while ((test.m_done.load() != (round + 1) * 4 || test.m_results.load() < (round + 1) * 38 - 6) && NetSenderSubmission::now() < deadline && !test.m_bad.load())
        {
            gateway.poll(); THREAD_CHECK(sink.respond(gateway)); CSystem::sleepPrecise(1);
        }
        for (uint32_t i = 0; i < 4; ++i) { CTaskThreadManager::Instance().Uninit(owner.m_producers[i]); owner.m_producers[i] = 0; }
        if (test.m_bad.load() || test.m_results.load() != (round + 1) * 38 - 6)
        {
            std::lock_guard<std::mutex> lock(test.m_mutex);
            std::printf("CONCURRENT_STATE round=%u done=%u results=%u accepted=%u terminals=%u bad=%u\n", round,
                test.m_done.load(), test.m_results.load(), static_cast<uint32_t>(test.m_accepted.size()),
                static_cast<uint32_t>(test.m_terminal.size()), test.m_bad.load() ? 1U : 0U);
        }
        THREAD_CHECK(!test.m_bad.load() && test.m_results.load() == (round + 1) * 38 - 6);
        if (verifyOrder) { for (uint32_t i = 0; i < 4; ++i) { THREAD_CHECK(sink.m_next[i] == 8); } }
        for (uint32_t i = 0; i < 6; ++i)
        {
            uint64_t id = 0;
            THREAD_CHECK(NetWork::instance().clientBegin(test.m_client, 101, 700, 5000, &id) == NI_OK);
            std::lock_guard<std::mutex> lock(test.m_mutex); test.m_accepted.push_back(id);
        }
        THREAD_CHECK(NetWork::instance().clientClose(test.m_client, 0) == NI_OK);
        while (NetWork::instance().clientPoll(test.m_client) != NI_OK && NetSenderSubmission::now() < deadline)
        {
            gateway.poll(); CSystem::sleepPrecise(1);
        }
        THREAD_CHECK(test.m_closed.load() == round + 1 && test.m_results.load() == (round + 1) * 38 && !test.m_bad.load());
    }
    THREAD_CHECK(NetWork::instance().clientDestroy(test.m_client) == NI_OK); test.m_client = nullptr;
    THREAD_CHECK(CTaskThreadManager::Instance().Count() == baseline);
    THREAD_CHECK(test.m_accepted.size() == 76 && test.m_terminal.size() == 76);
    for (size_t i = 0; i < test.m_accepted.size(); ++i) { THREAD_CHECK(test.m_terminal.count(test.m_accepted[i]) == 1); }
    gateway.shutdown();
    std::sort(test.m_sendMicros.begin(), test.m_sendMicros.end());
    THREAD_CHECK(test.m_sendMicros.size() == 32);
    std::printf("SYNC_SEND_TIMING samples=%u median_us=%u p95_us=%u max_us=%u\n",
        static_cast<uint32_t>(test.m_sendMicros.size()), test.m_sendMicros[16], test.m_sendMicros[30], test.m_sendMicros.back());
    if (verifyOrder) { std::printf("PER_PRODUCER_ORDER producers=4 rounds=2 messages=64 actual_arrival=PASS\n"); }
    std::printf("PASS NetWork CTask/TCP/TLS rounds=2 producers=8 unique=76 checks=%u\n", checks);
    #undef THREAD_CHECK
}

NetWorkAuditHost::NetWorkAuditHost(uint32_t mode) :
m_mode(mode), m_target(nullptr), m_connected(nullptr), m_data(nullptr), m_writable(nullptr), m_closed(nullptr),
m_stops(0), m_destroyed(0), m_destroyBusy(0), m_bridge(nullptr), m_retired(false), m_operations(0), m_bad(false)
{
    std::memset(m_inputs, 0, sizeof(m_inputs));
    std::memset(m_closes, 0, sizeof(m_closes));
}

NetHostFunctions NetWorkAuditHost::functions()
{
    NetHostFunctions value = {};
    value.structSize = sizeof(value);
    value.version = 1;
    value.context = this;
    value.create = create;
    value.connect = connect;
    value.poll = poll;
    value.write = write;
    value.pauseRead = peerOperation;
    value.close = peerOperation;
    value.shutdown = shutdown;
    value.destroy = destroy;
    value.localPort = query;
    value.peerCount = query;
    return value;
}

int32_t NetWorkAuditHost::create(void* provider, void* observer, NetHostConnected connected, NetHostData data,
    NetHostWritable writable, NetHostClosed closed, void** instance)
{
    NetWorkAuditHost* test = static_cast<NetWorkAuditHost*>(provider);
    test->m_target = observer;
    test->m_connected = connected;
    test->m_data = data;
    test->m_writable = writable;
    test->m_closed = closed;
    *instance = test;
    if (test->m_mode == 2)
    {
        throw std::runtime_error("controlled host partial creation");
    }
    return test->m_mode == 1 ? -41 : 0;
}

int32_t NetWorkAuditHost::connect(void* instance, const char* address, uint32_t port, uint64_t* peer)
{
    if (static_cast<NetWorkAuditHost*>(instance)->m_mode == 3) { throw std::runtime_error("controlled host connect"); }
    (void)address;
    (void)port;
    *peer = 1;
    return 0;
}

int32_t NetWorkAuditHost::poll(void* instance)
{
    NetWorkAuditHost* test = static_cast<NetWorkAuditHost*>(instance);
    if (test->m_mode == 3) { throw std::runtime_error("controlled host poll"); }
    return test->m_stops == 0 ? 1 : 0;
}

int32_t NetWorkAuditHost::shutdown(void* instance)
{
    if (static_cast<NetWorkAuditHost*>(instance)->m_mode == 3) { throw std::runtime_error("controlled host shutdown"); }
    ++static_cast<NetWorkAuditHost*>(instance)->m_stops;
    return 0;
}

int32_t NetWorkAuditHost::destroy(void* instance)
{
    if (static_cast<NetWorkAuditHost*>(instance)->m_mode == 3) { throw std::runtime_error("controlled host destroy"); }
    NetWorkAuditHost* test = static_cast<NetWorkAuditHost*>(instance);
    if (test->m_destroyBusy != 0)
    {
        --test->m_destroyBusy;
        return -42;
    }
    ++test->m_destroyed;
    return 0;
}

int32_t NetWorkAuditHost::write(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    if (static_cast<NetWorkAuditHost*>(instance)->m_mode == 3) { throw std::runtime_error("controlled host write"); }
    (void)peer;
    (void)bytes;
    (void)size;
    return 0;
}

int32_t NetWorkAuditHost::peerOperation(void* instance, uint64_t peer, int32_t value)
{
    if (static_cast<NetWorkAuditHost*>(instance)->m_mode == 3) { throw std::runtime_error("controlled host operation"); }
    NetWorkAuditHost* test = static_cast<NetWorkAuditHost*>(instance);
    ++test->m_operations;
    if (test->m_mode == 4 && test->m_retired && peer == 1) { return -43; }
    (void)value;
    return 0;
}

uint32_t NetWorkAuditHost::query(void* instance)
{
    if (static_cast<NetWorkAuditHost*>(instance)->m_mode == 3) { throw std::runtime_error("controlled host query"); }
    return 0;
}

void NetWorkAuditHost::connected(void* context, uint64_t peer)
{
    if (peer < 1 || peer > 2)
    {
        static_cast<NetWorkAuditHost*>(context)->m_bad = true;
    }
}

void NetWorkAuditHost::data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    NetWorkAuditHost* test = static_cast<NetWorkAuditHost*>(context);
    if (peer < 1 || peer > 2 || size != 1 || bytes == nullptr || bytes[0] != 7 || test->m_closes[peer - 1] != 0)
    {
        test->m_bad = true;
        return;
    }
    ++test->m_inputs[peer - 1];
    if (test->m_mode == 4)
    {
        if (NetWorkHostBridge::pauseRead(test->m_bridge, peer, 1) != 0 ||
            NetWorkHostBridge::pauseRead(test->m_bridge, peer, 0) != 0)
        {
            test->m_bad = true;
        }
    }
}

void NetWorkAuditHost::writable(void* context, uint64_t peer)
{
    (void)context;
    (void)peer;
}

void NetWorkAuditHost::closed(void* context, uint64_t peer, int32_t reason)
{
    NetWorkAuditHost* test = static_cast<NetWorkAuditHost*>(context);
    if (peer < 1 || peer > 2 || reason != NI_OK)
    {
        test->m_bad = true;
        return;
    }
    ++test->m_closes[peer - 1];
}

void NetWorkConcurrencyTestCases::hostCreationFailures()
{
    for (uint32_t mode = 1; mode <= 2; ++mode)
    {
        NetWorkAuditHost test(mode);
        NetHostFunctions host = test.functions();
        void* bridge = nullptr;
        const int32_t result = NetWorkHostBridge::create(&host, &test, test.connected, test.data, test.writable, test.closed, &bridge);
        if (result >= 0 || bridge != nullptr || test.m_stops != 1 || test.m_destroyed != 1 || test.m_bad)
        {
            throw std::runtime_error("partial host creation must stop and destroy its owned host");
        }
        netTestCheckpoint();
    }
    NetWorkAuditHost test(0);
    NetHostFunctions host = test.functions();
    void* bridge = nullptr;
    if (NetWorkHostBridge::create(&host, &test, test.connected, test.data, test.writable, test.closed, &bridge) != 0)
    {
        throw std::runtime_error("host boundary create");
    }
    test.m_mode = 3;
    bool isolated = false;
    try
    {
        uint64_t peer = 0;
        const uint8_t byte = 7;
        isolated = NetWorkHostBridge::connect(bridge, "127.0.0.1", 1, &peer) < 0 &&
            NetWorkHostBridge::write(bridge, 1, &byte, 1) < 0 && NetWorkHostBridge::pauseRead(bridge, 1, 1) < 0 &&
            NetWorkHostBridge::close(bridge, 1, 0) < 0 && NetWorkHostBridge::localPort(bridge) == 0 &&
            NetWorkHostBridge::peerCount(bridge) == 0 && NetWorkHostBridge::poll(bridge) < 0 &&
            NetWorkHostBridge::shutdown(bridge) < 0 && NetWorkHostBridge::destroy(bridge) < 0;
    }
    catch (...) { isolated = false; }
    test.m_mode = 0;
    const int32_t stopped = NetWorkHostBridge::shutdown(bridge);
    const int32_t destroyed = NetWorkHostBridge::destroy(bridge);
    if (!isolated || stopped != 0 || destroyed != 0 || test.m_stops != 1 || test.m_destroyed != 1)
    {
        throw std::runtime_error("raw host exceptions must return errors and leave cleanup usable");
    }
    std::printf("HOST_CREATION_FAILURES modes=2 cleanup=PASS raw_exceptions=9 PASS\n");
}

void NetWorkConcurrencyTestCases::pausedHostFairness()
{
    NetWorkAuditHost test(0);
    NetHostFunctions host = test.functions();
    void* bridge = nullptr;
    if (NetWorkHostBridge::create(&host, &test, test.connected, test.data, test.writable, test.closed, &bridge) != 0)
    {
        throw std::runtime_error("audit host create");
    }
    try
    {
        test.m_connected(test.m_target, 1);
        test.m_connected(test.m_target, 2);
        NetWorkHostBridge::poll(bridge);
        NetWorkHostBridge::pauseRead(bridge, 1, 1);
        const uint8_t byte = 7;
        for (uint32_t i = 0; i < 2000; ++i)
        {
            test.m_data(test.m_target, 1, &byte, 1);
        }
        for (uint32_t i = 0; i < 32; ++i)
        {
            test.m_data(test.m_target, 2, &byte, 1);
        }
        test.m_closed(test.m_target, 2, NI_OK);
        test.m_closed(test.m_target, 1, NI_OK);
        const auto started = std::chrono::steady_clock::now();
        NetWorkHostBridge::poll(bridge);
        const int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started).count();
        if (test.m_inputs[0] != 0 || test.m_inputs[1] != 32 || test.m_closes[0] != 0 || test.m_closes[1] != 1 || test.m_bad)
        {
            throw std::runtime_error("paused peer must not prevent another peer from finishing");
        }
        NetWorkHostBridge::pauseRead(bridge, 1, 0);
        for (uint32_t i = 0; i < 9; ++i)
        {
            NetWorkHostBridge::poll(bridge);
            netTestCheckpoint();
        }
        if (test.m_inputs[0] != 2000 || test.m_closes[0] != 1 || test.m_bad)
        {
            throw std::runtime_error("resumed peer must retain all original ordered bytes and one close");
        }
        std::printf("PAUSED_HOST_FAIRNESS blocked=2000 active=32 drain_us=%lld\n", static_cast<long long>(elapsed));
        NetWorkHostBridge::shutdown(bridge);
        if (NetWorkHostBridge::destroy(bridge) != 0)
        {
            throw std::runtime_error("audit host destroy");
        }
        bridge = nullptr;
    }
    catch (...)
    {
        if (bridge != nullptr)
        {
            NetWorkHostBridge::shutdown(bridge);
            NetWorkHostBridge::destroy(bridge);
        }
        throw;
    }
}

void NetWorkConcurrencyTestCases::lifecycleResources()
{
    const int32_t threads = CTaskThreadManager::Instance().Count();
#ifdef _WIN32
    DWORD baselineHandles = 0;
    DWORD finalHandles = 0;
    SIZE_T firstMemory = 0;
    SIZE_T lastMemory = 0;
    SIZE_T peakMemory = 0;
#endif
    uint32_t closed = 0;
    const auto started = std::chrono::steady_clock::now();
    for (uint32_t round = 0; round < 72; ++round)
    {
        {
            NetWorkConcurrencyContext test;
            test.m_throwReady = (round % 2) != 0;
            NetInterfaceClientCallbacks callbacks = {};
            callbacks.structSize = sizeof(callbacks);
            callbacks.version = 1;
            callbacks.context = &test;
            callbacks.ready = test.ready;
            callbacks.result = test.result;
            callbacks.frame = test.frame;
            callbacks.closed = test.closed;
            NetWorkConcurrencySink sink;
            NetPeerEndpointRouter gateway(&sink);
            NetWorkConcurrencyOwner owner(test);
            if (NetWork::instance().clientCreate(&callbacks, &test.m_client) != NI_OK)
            {
                throw std::runtime_error("resource cycle client create");
            }
            NetInterfaceTlsOptions tls = {};
            tls.server = 1;
            tls.certificate = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_crt_rsa_sha256_pem);
            tls.certificateSize = static_cast<uint32_t>(mbedtls_test_srv_crt_rsa_sha256_pem_len);
            tls.privateKey = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_key_rsa_pem);
            tls.privateKeySize = static_cast<uint32_t>(mbedtls_test_srv_key_rsa_pem_len);
            tls.handshakeTimeoutMs = 3000;
            uint64_t service = 0;
            if (gateway.listen("127.0.0.1", 0, tls) != NI_OK || gateway.addService(101, false, 32, service) != NI_OK)
            {
                throw std::runtime_error("resource cycle isolated router");
            }
            NetInterfaceClientOptions options = {};
            options.structSize = sizeof(options);
            options.version = 1;
            options.address = "127.0.0.1";
            options.port = gateway.localPort();
            options.platform = 1;
            options.capabilities = 7;
            options.maxRequests = 32;
            options.maxChunk = 4096;
            options.handshakeTimeoutMs = 3000;
            std::memset(options.device, 0x53, sizeof(options.device));
            options.tls.handshakeTimeoutMs = 3000;
            options.tls.hostname = "localhost";
            options.tls.authority = reinterpret_cast<const uint8_t*>(mbedtls_test_ca_crt_rsa_sha256_pem);
            options.tls.authoritySize = static_cast<uint32_t>(mbedtls_test_ca_crt_rsa_sha256_pem_len);
            if (NetWork::instance().clientConnect(test.m_client, &options) != NI_OK)
            {
                throw std::runtime_error("resource cycle connect");
            }
            const uint64_t deadline = NetSenderSubmission::now() + 4000;
            while (test.m_ready.load() == 0 && NetSenderSubmission::now() < deadline)
            {
                gateway.poll();
                CSystem::sleepPrecise(1);
                netTestCheckpoint();
            }
            if (test.m_ready.load() != 1 || test.m_bad.load())
            {
                throw std::runtime_error("resource cycle ready");
            }
            if (!test.m_throwReady)
            {
                NetWork::instance().clientClose(test.m_client, NI_OK);
            }
            while (NetWork::instance().clientPoll(test.m_client) != NI_OK && NetSenderSubmission::now() < deadline)
            {
                gateway.poll();
                CSystem::sleepPrecise(1);
                netTestCheckpoint();
            }
            if (test.m_closed.load() != 1 || test.m_bad.load() || NetWork::instance().clientDestroy(test.m_client) != NI_OK)
            {
                throw std::runtime_error("resource cycle close/destroy");
            }
            test.m_client = nullptr;
            ++closed;
            gateway.shutdown();
        }
        if (CTaskThreadManager::Instance().Count() != threads)
        {
            throw std::runtime_error("resource cycle left an owned CTask thread");
        }
#ifdef _WIN32
        DWORD handles = 0;
        PROCESS_MEMORY_COUNTERS_EX memory = {};
        memory.cb = sizeof(memory);
        if (!GetProcessHandleCount(GetCurrentProcess(), &handles) ||
            !GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory)))
        {
            throw std::runtime_error("resource counters unavailable");
        }
        if (round == 7)
        {
            baselineHandles = handles;
            firstMemory = memory.PrivateUsage;
        }
        if (round >= 8)
        {
            peakMemory = (std::max)(peakMemory, memory.PrivateUsage);
            lastMemory = memory.PrivateUsage;
            finalHandles = handles;
        }
#endif
    }
#ifdef _WIN32
    if (finalHandles > baselineHandles)
    {
        throw std::runtime_error("resource cycles left more process handles than their warm baseline");
    }
    std::printf("LIFECYCLE_RESOURCES warm=8 cycles=64 closed=%u callbacks_threw=36 handles=%u/%u private_bytes=%llu/%llu peak=%llu\n",
        closed, static_cast<uint32_t>(baselineHandles), static_cast<uint32_t>(finalHandles),
        static_cast<unsigned long long>(firstMemory), static_cast<unsigned long long>(lastMemory), static_cast<unsigned long long>(peakMemory));
#endif
    const int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
    std::printf("LIFECYCLE_RESOURCES completed=%u elapsed_ms=%lld threads=%d\n", closed, static_cast<long long>(elapsed), threads);
}

void* NetWorkAuditAllocation::allocate(size_t size)
{
    (void)size;
    return nullptr;
}

void* NetWorkAuditAllocation::reallocate(void* pointer, size_t size)
{
    (void)pointer;
    (void)size;
    return nullptr;
}

void* NetWorkAuditAllocation::allocateZero(size_t count, size_t size)
{
    (void)count;
    (void)size;
    return nullptr;
}

void NetWorkConcurrencyTestCases::loopInitializationFailure()
{
    std::unique_ptr<LibuvChannel> channel;
    if (uv_replace_allocator(NetWorkAuditAllocation::allocate, NetWorkAuditAllocation::reallocate,
        NetWorkAuditAllocation::allocateZero, std::free) != 0)
    {
        throw std::runtime_error("install isolated libuv allocation failure");
    }
    bool threw = false;
    try
    {
        channel.reset(new LibuvChannel());
    }
    catch (...)
    {
        threw = true;
    }
    const int32_t restored = uv_replace_allocator(std::malloc, std::realloc, std::calloc, std::free);
    if (restored != 0 || threw || !channel || channel->poll() || channel->localPort() != 0 || channel->peerCount() != 0)
    {
        throw std::runtime_error("libuv loop initialization must report an error without throwing or driving an invalid loop");
    }
    uint64_t peer = 99;
    if (channel->listen("127.0.0.1", 0, nullptr) != UV_ENOMEM ||
        channel->connect("127.0.0.1", 1, nullptr, peer) != UV_ENOMEM || peer != 0)
    {
        throw std::runtime_error("failed loop must return the original libuv error and clear the peer id");
    }
    channel->shutdown();
    channel.reset();
    std::printf("LIBUV_INIT_ALLOCATION_FAILURE reported_without_exception=PASS\n");
}

void NetWorkConcurrencyTestCases::deferredHostCleanup()
{
    for (uint32_t mode = 1; mode <= 2; ++mode)
    {
        NetWorkAuditHost test(mode);
        test.m_destroyBusy = 1;
        NetHostFunctions host = test.functions();
        void* bridge = nullptr;
        const int32_t result = NetWorkHostBridge::create(&host, &test, test.connected, test.data, test.writable, test.closed, &bridge);
        const bool retained = result < 0 && bridge != nullptr && test.m_destroyed == 0 && test.m_stops == 1;
        if (bridge != nullptr)
        {
            // 创建失败后的宿主仍可能产生迟到回调，桥必须保持有效并已禁止接纳。
            test.m_connected(test.m_target, 1);
            test.m_mode = 0;
            NetWorkHostBridge::shutdown(bridge);
            if (NetWorkHostBridge::destroy(bridge) != 0)
            {
                throw std::runtime_error("deferred host cleanup still refused after resource became quiescent");
            }
        }
        if (!retained || test.m_destroyed != 1 || test.m_bad)
        {
            throw std::runtime_error("partial create lost its only recoverable host/bridge cleanup handle");
        }
        netTestCheckpoint();
    }
}

void NetWorkConcurrencyTestCases::closedHostTail()
{
    NetWorkAuditHost test(4);
    NetHostFunctions host = test.functions();
    if (NetWorkHostBridge::create(&host, &test, test.connected, test.data, test.writable, test.closed, &test.m_bridge) != 0)
    {
        throw std::runtime_error("closed tail bridge create");
    }
    test.m_connected(test.m_target, 1);
    test.m_connected(test.m_target, 2);
    NetWorkHostBridge::poll(test.m_bridge);
    const uint8_t byte = 7;
    test.m_data(test.m_target, 1, &byte, 1);
    test.m_closed(test.m_target, 1, NI_OK);
    test.m_retired = true;
    NetWorkHostBridge::poll(test.m_bridge);
    const bool tail = !test.m_bad && test.m_inputs[0] == 1 && test.m_closes[0] == 1 && test.m_operations == 0;
    test.m_data(test.m_target, 2, &byte, 1);
    NetWorkHostBridge::poll(test.m_bridge);
    const bool healthy = !test.m_bad && test.m_inputs[1] == 1 && test.m_closes[1] == 0 && test.m_operations == 2 && test.m_stops == 0;
    test.m_closed(test.m_target, 2, NI_OK);
    NetWorkHostBridge::poll(test.m_bridge);
    NetWorkHostBridge::shutdown(test.m_bridge);
    const int32_t destroyed = NetWorkHostBridge::destroy(test.m_bridge);
    if (!tail || !healthy || destroyed != 0 || test.m_destroyed != 1 || test.m_closes[1] != 1)
    {
        throw std::runtime_error("closed raw peer tail must not pause a retired socket or stop healthy peers");
    }
    NetWorkAuditHost raw(4);
    NetHostFunctions provider = raw.functions();
    NetHostFunctions wrapper = provider;
    wrapper.context = &provider;
    wrapper.create = NetWorkHostBridge::create;
    wrapper.connect = NetWorkHostBridge::connect;
    wrapper.poll = NetWorkHostBridge::poll;
    wrapper.write = NetWorkHostBridge::write;
    wrapper.pauseRead = NetWorkHostBridge::pauseRead;
    wrapper.close = NetWorkHostBridge::close;
    wrapper.shutdown = NetWorkHostBridge::shutdown;
    wrapper.destroy = NetWorkHostBridge::destroy;
    wrapper.localPort = NetWorkHostBridge::localPort;
    wrapper.peerCount = NetWorkHostBridge::peerCount;
    NetWorkTailObserver observer;
    NetHostSocket socket(&wrapper);
    observer.m_socket = &socket;
    uint64_t peer = 0;
    if (socket.connect("127.0.0.1", 1, &observer, peer) != NI_OK || peer != 1)
    {
        throw std::runtime_error("tail socket create");
    }
    raw.m_connected(raw.m_target, 1);
    socket.poll();
    const uint32_t activeOperations = raw.m_operations;
    raw.m_data(raw.m_target, 1, &byte, 1);
    raw.m_closed(raw.m_target, 1, NI_OK);
    raw.m_retired = true;
    socket.poll();
    const bool ordered = observer.m_inputs == 1 && observer.m_closed == 1 && !observer.m_bad && raw.m_operations == activeOperations;
    socket.shutdown();
    const int32_t released = socket.uninit();
    if (!ordered || released != NI_OK || raw.m_destroyed != 1)
    {
        throw std::runtime_error("real socket must retain data then EOF without pausing a retired raw handle");
    }
}

NetWorkTailObserver::NetWorkTailObserver() : m_socket(nullptr), m_inputs(0), m_closed(0), m_bad(false)
{

}

void NetWorkTailObserver::onConnected(uint64_t peer)
{
    if (peer != 1 || m_socket->pauseRead(peer, false) != NI_OK) { m_bad = true; }
}

void NetWorkTailObserver::onData(uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    if (peer != 1 || bytes == nullptr || size != 1 || bytes[0] != 7 || m_closed != 0 ||
        m_socket->pauseRead(peer, true) != NI_OK || m_socket->pauseRead(peer, false) != NI_OK) { m_bad = true; }
    ++m_inputs;
}

void NetWorkTailObserver::onWritable(uint64_t peer)
{
    (void)peer;
}

void NetWorkTailObserver::onClosed(uint64_t peer, int32_t reason, int32_t detail)
{
    if (peer != 1 || reason != NI_OK || detail != 0 || m_inputs != 1 || m_closed != 0) { m_bad = true; }
    ++m_closed;
}

void NetWorkConcurrencyTestCases::synchronousFrameExpiry()
{
    uint32_t checks = 0;
#define EXPIRY_CHECK(value) { ++checks; if (!(value)) { std::printf("SYNC_EXPIRY_FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); throw std::runtime_error("synchronous frame expiry violated"); } }
    NetSenderSubmission beforePoll, beforeAdvance, staged;
    NetSenderSubmission* queues[2] = {&beforePoll, &beforeAdvance};
    uint64_t tickets[2][3] = {};
    uint64_t stagedTicket = 0;
    uint8_t bytes[16] = {7};
    NetInterfaceFrame frame = {};
    frame.m_productId = 101;
    frame.m_requestId = 900;
    frame.m_messageType = NI_FILE_DATA_TYPE;
    frame.m_payload = bytes;
    frame.m_payloadSize = sizeof(bytes);
    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        NetSenderSubmission& queue = *queues[mode];
        EXPIRY_CHECK(queue.start(1, 101) == NI_OK && queue.ready(1, 8, 65536) == NI_OK);
        frame.m_kind = NI_DATA;
        EXPIRY_CHECK(queue.dataSynchronous(frame, tickets[mode][0]) == NI_OK);
        frame.m_kind = NI_DATA_ACK;
        EXPIRY_CHECK(queue.dataSynchronous(frame, tickets[mode][1]) == NI_OK);
        frame.m_kind = NI_CANCEL;
        frame.m_payload = nullptr; frame.m_payloadSize = 0;
        EXPIRY_CHECK(queue.dataSynchronous(frame, tickets[mode][2]) == NI_OK);
        frame.m_payload = bytes; frame.m_payloadSize = sizeof(bytes);
        frame.m_requestId = 901;
        frame.m_kind = NI_DATA;
        for (uint32_t index = 0; index < 63; ++index) { EXPIRY_CHECK(queue.data(frame) == NI_OK); }
        EXPIRY_CHECK(queue.data(frame) == NI_LIMIT_EXCEEDED);
        frame.m_kind = NI_DATA_ACK;
        for (uint32_t index = 0; index < 14; ++index) { EXPIRY_CHECK(queue.data(frame) == NI_OK); }
        EXPIRY_CHECK(queue.data(frame) == NI_LIMIT_EXCEEDED);
        EXPIRY_CHECK(queue.beginPoll() == NI_OK && queue.queuedBytes() == 64 * sizeof(bytes));
        frame.m_requestId = 900;
    }
    EXPIRY_CHECK(staged.start(1, 101) == NI_OK && staged.ready(1, 8, 65536) == NI_OK);
    frame.m_kind = NI_DATA;
    EXPIRY_CHECK(staged.dataSynchronous(frame, stagedTicket) == NI_OK && staged.beginPoll() == NI_OK);
    NetSenderSubmissionCommand command = {};
    EXPIRY_CHECK(staged.next(command) == NI_READY && command.m_frame.m_requestId == 900);
    staged.finish(NI_OK);
    // 共用同一次实际五秒期限；期间没有owner推进，不依赖私有时钟或改写实现状态。
    const uint64_t waitUntil = NetSenderSubmission::now() + 5010;
    while (NetSenderSubmission::now() < waitUntil)
    {
        netTestCheckpoint();
        CSystem::sleepPrecise(10);
    }
    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        NetSenderSubmission& queue = *queues[mode];
        if (mode != 0) { queue.advanceSynchronous(NI_READY); }
        uint32_t received = 0;
        // mode0首先next，mode1首先检查票据，两种顺序均不能把过期帧交给下层。
        while (queue.next(command) == NI_READY)
        {
            EXPIRY_CHECK(command.m_kind == NET_SUBMIT_DATA && command.m_frame.m_requestId == 901);
            EXPIRY_CHECK(command.m_frame.m_payloadSize == sizeof(bytes) && command.m_frame.m_payload[0] == 7);
            queue.finish(NI_OK);
            ++received;
        }
        EXPIRY_CHECK(received == 77 && queue.queuedBytes() == 0);
        for (uint32_t index = 0; index < 3; ++index)
        {
            EXPIRY_CHECK(queue.synchronousResult(tickets[mode][index], true) == NI_TIMEOUT);
            EXPIRY_CHECK(queue.synchronousResult(tickets[mode][index], false) == NI_NOT_FOUND);
        }
        uint64_t generation = 0; bool ready = false;
        queue.state(generation, ready);
        EXPIRY_CHECK(generation == 1 && ready && queue.idle());
        // 旧帧的字节和槽位必须全部返还，不能用关闭健康连接掩盖资源遗留。
        frame.m_requestId = 902; frame.m_kind = NI_DATA;
        for (uint32_t index = 0; index < 64; ++index) { EXPIRY_CHECK(queue.data(frame) == NI_OK); }
        EXPIRY_CHECK(queue.data(frame) == NI_LIMIT_EXCEEDED);
        frame.m_kind = NI_DATA_ACK;
        for (uint32_t index = 0; index < 16; ++index) { EXPIRY_CHECK(queue.data(frame) == NI_OK); }
        EXPIRY_CHECK(queue.data(frame) == NI_LIMIT_EXCEEDED);
        queue.close(0); queue.disconnected(1);
        EXPIRY_CHECK(queue.idle() && queue.queuedBytes() == 0);
    }
    // 已被协议接纳的帧可能存在已发前缀，仍按既有约定关闭本代而不重放。
    staged.advanceSynchronous(NI_READY);
    EXPIRY_CHECK(staged.synchronousResult(stagedTicket, true) == NI_TIMEOUT);
    int32_t reason = 0;
    EXPIRY_CHECK(staged.closeRequested(reason) && reason == NI_TIMEOUT);
    staged.disconnected(1);
    EXPIRY_CHECK(staged.idle());
    std::printf("SYNC_FRAME_EXPIRY checks=%u expired=6 preserved_posts=154 budgets_restored=2 staged_closed=1 PASS\n", checks);
#undef EXPIRY_CHECK
}

NetWorkWakeProducer::NetWorkWakeProducer(NetWorkClientOpaque* client) : m_started(false), m_done(false), m_client(client)
{

}

void NetWorkWakeProducer::DoTask()
{
    m_started = true;
    NetWorkClientOpaque::wake(m_client);
    m_done = true;
}

uint64_t NetWorkConcurrencyTestCases::microseconds()
{
#ifdef _WIN32
    LARGE_INTEGER ticks = {};
    LARGE_INTEGER frequency = {};
    if (!QueryPerformanceCounter(&ticks) || !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
    {
        throw std::runtime_error("performance timer unavailable");
    }
    return static_cast<uint64_t>((ticks.QuadPart / frequency.QuadPart) * 1000000 +
        (ticks.QuadPart % frequency.QuadPart) * 1000000 / frequency.QuadPart);
#else
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}

void NetWorkConcurrencyTestCases::ownerWakeup()
{
    NetWorkClientCallbacks callbacks = {};
    NetWorkClientOpaque client(callbacks);
    const int32_t before = CTaskThreadManager::Instance().Count();
    const uint32_t threadId = CTaskThreadManager::Instance().Init();
    bool delivered = false;
    bool completed = false;
    try
    {
        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
        if (!thread) { throw std::runtime_error("wake producer creation failed"); }
        std::shared_ptr<NetWorkWakeProducer> producer(new NetWorkWakeProducer(&client));
        std::unique_lock<NetWorkGate> lock(client.m_wakeGate);
        // 与真实owner相同：先检查并消费通知，再决定进入等待。
        if (client.m_notified.exchange(false)) { throw std::runtime_error("unexpected initial notification"); }
        thread->PostTask(producer);
        const uint64_t deadline = NetSenderSubmission::now() + 1000;
        while (!producer->m_started.load() && NetSenderSubmission::now() < deadline)
        {
            CSystem::sleepPrecise(1);
        }
        if (!producer->m_started.load()) { throw std::runtime_error("wake producer did not start"); }
        // 旧实现会在持锁期间完成通知；正确实现与wait的原子解锁／等待配对。
        const uint64_t overlap = NetSenderSubmission::now() + 20;
        while (!producer->m_done.load() && NetSenderSubmission::now() < overlap)
        {
            CSystem::sleepPrecise(1);
        }
        const uint64_t wakeDeadline = NetSenderSubmission::now() + 100;
        while (!client.m_notified.load() && NetSenderSubmission::now() < wakeDeadline)
        {
            if (!client.m_wakeGate.waitFor(2))
            {
                throw std::runtime_error("wake system wait failed");
            }
        }
        delivered = client.m_notified.load();
        lock.unlock();
        CTaskThreadManager::Instance().Uninit(threadId);
        completed = producer->m_done.load();
    }
    catch (...)
    {
        CTaskThreadManager::Instance().Uninit(threadId);
        throw;
    }
    if (!delivered || !completed || !client.m_notified.load() || CTaskThreadManager::Instance().Count() != before)
    {
        throw std::runtime_error("NetWork owner lost notification between predicate and wait");
    }
    std::printf("OWNER_WAKEUP notification_delivered=1 producer_reaped=1 PASS\n");
}

NetFileStopProducer::NetFileStopProducer(const std::shared_ptr<NetFileIoState>& state) :
m_started(false), m_done(false), m_state(state)
{

}

void NetFileStopProducer::DoTask()
{
    NetFileIoTask task(m_state);
    m_started = true;
    task.StopTask();
    m_done = true;
}

void NetWorkConcurrencyTestCases::fileStopWakeup()
{
    std::shared_ptr<NetFileIoState> state(new NetFileIoState());
    const int32_t before = CTaskThreadManager::Instance().Count();
    const uint32_t id = CTaskThreadManager::Instance().Init();
    bool delivered = false;
    bool completed = false;
    try
    {
        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(id);
        if (!thread) { throw std::runtime_error("file stop producer creation failed"); }
        std::shared_ptr<NetFileStopProducer> producer(new NetFileStopProducer(state));
        std::unique_lock<NetWorkGate> lock(state->m_gate);
        if (state->m_stop.load() || !state->m_queued.empty()) { throw std::runtime_error("file test initial predicate invalid"); }
        thread->PostTask(producer);
        const uint64_t deadline = NetSenderSubmission::now() + 1000;
        while (!producer->m_started.load() && NetSenderSubmission::now() < deadline)
        {
            CSystem::sleepPrecise(1);
        }
        if (!producer->m_started.load()) { throw std::runtime_error("file stop producer did not start"); }
        const uint64_t overlap = NetSenderSubmission::now() + 20;
        while (!producer->m_done.load() && NetSenderSubmission::now() < overlap)
        {
            CSystem::sleepPrecise(1);
        }
        // 使用与生产相同的原子解锁等待；独立测试进程由runner超时兜底。
        while (!state->m_stop.load())
        {
            delivered = state->m_gate.wait();
            if (!delivered)
            {
                break;
            }
        }
        lock.unlock();
        CTaskThreadManager::Instance().Uninit(id);
        completed = producer->m_done.load();
    }
    catch (...)
    {
        CTaskThreadManager::Instance().Uninit(id);
        throw;
    }
    if (!delivered || !completed || !state->m_stop.load() || CTaskThreadManager::Instance().Count() != before)
    {
        throw std::runtime_error("file Task stop notification was lost before wait");
    }
    std::printf("FILE_STOP_WAKEUP notification_delivered=1 producer_reaped=1 PASS\n");
}

void NetWorkConcurrencyTestCases::fileStopCycles()
{
    const int32_t before = CTaskThreadManager::Instance().Count();
    for (uint32_t index = 0; index < 200; ++index)
    {
        NetNativeFileIo io;
        if (io.init() != NI_OK) { throw std::runtime_error("file lifecycle create failed"); }
        // 交替让空闲执行者进入等待和在启动阶段停止，不建立用户文件。
        if (index % 2 != 0) { CSystem::sleepPrecise(1); }
        io.stop();
        const uint64_t deadline = NetSenderSubmission::now() + 1000;
        while (!io.done() && NetSenderSubmission::now() < deadline)
        {
            netTestCheckpoint(); CSystem::sleepPrecise(1);
        }
        const bool done = io.done();
        uint64_t requestId = 7;
        NetFileIoRequest request = {};
        request.id = 1; request.operation = NET_FILE_IO_CLOSE;
        const bool rejected = io.nextRequestId(requestId) == NI_INVALID_STATE && requestId == 0 &&
            io.submit(request) == NI_INVALID_STATE;
        io.uninit();
        if (!done || !rejected || CTaskThreadManager::Instance().Count() != before)
        {
            throw std::runtime_error("file stop did not finish, reject new work, or reclaim its Task");
        }
    }
    std::printf("FILE_STOP_CYCLES iterations=200 rejected_after_stop=200 task_count_restored=200 PASS\n");
}

void NetWorkConcurrencyTestCases::ipcInputOwnership()
{
    NetSenderLocalInbox inbox;
    NetSenderLocalState state(inbox);
    std::unique_ptr<NetSenderLocalPeer> peer(new NetSenderLocalPeer());
    peer->m_pid = 12345;
    peer->m_local[0] = 1;
    peer->m_remote[0] = 2;
    peer->m_remoteKnown = true;
    NetSenderLocalPeer* target = peer.get();
    state.m_peers.insert(std::make_pair(UINT64_C(1), std::move(peer)));
    std::vector<uint8_t> payload(16384);
    for (uint32_t index = 0; index < payload.size(); ++index) { payload[index] = static_cast<uint8_t>(index * 31); }
    std::vector<uint8_t> encoded(56 + payload.size());
    NetSenderLocalPacketView packet = {};
    packet.m_kind = NET_LOCAL_DATA;
    packet.m_source[0] = 2;
    packet.m_target[0] = 1;
    packet.m_data = payload.data(); packet.m_size = static_cast<uint32_t>(payload.size());
    uint32_t moved = 0;
    const uint64_t started = microseconds();
    for (uint32_t index = 0; index < 20000; ++index)
    {
        if (index % 128 == 0) { netTestCheckpoint(); }
        packet.m_packet = index + 1;
        uint32_t required = 0;
        if (NetSenderLocalPacket::encode(packet, encoded.data(), static_cast<uint32_t>(encoded.size()), required) != NI_OK)
        {
            throw std::runtime_error("ownership fixture encoding failed");
        }
        {
            NetSenderLocalInput input;
            input.m_pid = 12345;
            input.m_bytes = encoded;
            const uint8_t* allocation = input.m_bytes.data();
            state.accept(input);
            if (target->m_input.size() != 1) { throw std::runtime_error("IPC input disappeared or duplicated"); }
            if (target->m_input.front().m_bytes.data() == allocation) { ++moved; }
            // 收件箱弹出的输入生命期至此结束，通道不得借用已释放的输入对象。
        }
        const NetSenderLocalChunk& chunk = target->m_input.front();
        if (chunk.m_packet != index + 1 || chunk.m_offset > chunk.m_bytes.size() ||
            chunk.m_bytes.size() - chunk.m_offset != payload.size() ||
            std::memcmp(chunk.m_bytes.data() + chunk.m_offset, payload.data(), payload.size()) != 0 ||
            target->m_window.received(chunk.m_packet) != NI_OK)
        {
            throw std::runtime_error("IPC owned ciphertext, envelope offset or sequence corrupted");
        }
        target->m_input.pop_front();
    }
    std::printf("IPC_INPUT_OWNERSHIP packets=20000 payload_bytes=16384 moved=%u elapsed_us=%llu PASS\n",
        moved, static_cast<unsigned long long>(microseconds() - started));
}

void NetWorkConcurrencyTestCases::submissionOrder()
{
    NetSenderSubmission queue;
    uint32_t checks = 0;
#define ORDER_CHECK(value) { ++checks; if (!(value)) { std::printf("ORDER_FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); throw std::runtime_error("actual submission order violated"); } }
    ORDER_CHECK(queue.start(1, 101) == NI_OK && queue.ready(1, 8, 4096) == NI_OK);
    uint64_t low = 0, high = 0;
    ORDER_CHECK(queue.begin(101, 700, 5000, low) == NI_OK && queue.begin(101, 700, 5000, high) == NI_OK);
    uint8_t value = 7;
    ORDER_CHECK(queue.send(high, &value, 1) == NI_OK);
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_DATA; frame.m_productId = 101; frame.m_requestId = 7001;
    frame.m_payload = &value; frame.m_payloadSize = 1;
    ORDER_CHECK(queue.data(frame) == NI_OK);
    ORDER_CHECK(queue.send(low, &value, 1) == NI_OK);
    frame.m_kind = NI_DATA_ACK;
    ORDER_CHECK(queue.data(frame) == NI_OK);
    ORDER_CHECK(queue.beginPoll() == NI_OK);
    NetSenderSubmissionCommand command = {};
    const uint64_t expected[2] = {high, low};
    for (uint32_t index = 0; index < 2; ++index)
    {
        ORDER_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN && command.m_id == expected[index]);
        queue.finish(NI_OK);
        ORDER_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND && command.m_id == expected[index]);
        queue.finish(NI_OK);
        ORDER_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_DATA &&
            command.m_frame.m_kind == (index == 0 ? NI_DATA : NI_DATA_ACK));
        queue.finish(NI_OK);
    }
    ORDER_CHECK(queue.next(command) == NI_NEED_MORE);
    queue.close(0); queue.disconnected(1);
    while (queue.takeClosed(command)) {}
    ORDER_CHECK(queue.idle());
    std::printf("SUBMISSION_ORDER checks=%u PASS\n", checks);
#undef ORDER_CHECK
}

void NetWorkConcurrencyTestCases::fileCancelledRelease()
{
    std::shared_ptr<NetFileIoState> state(new NetFileIoState());
    const int32_t before = CTaskThreadManager::Instance().Count();
    const std::string root = CStringManager::AnsiToUtf8(CSystem::GetCurrentExePath());
    const std::string closePath = CSystem::joinPath(root, "cancel-close-owned.tmp");
    const std::string publishPath = CSystem::joinPath(root, "cancel-publish-owned.tmp");
    const std::string finalPath = CSystem::joinPath(root, "cancel-publish-final.bin");
    uint32_t threadId = 0;
    bool completed = false;
    bool released = false;
    try
    {
        if (CSystem::fileExists(CStringManager::Utf8ToUnicode(finalPath)))
        {
            throw std::runtime_error("cancelled release fixture already exists");
        }
        for (uint32_t index = 0; index < 2; ++index)
        {
            std::unique_ptr<NetFileIoHandle> file(new NetFileIoHandle());
            const std::string& path = index == 0 ? closePath : publishPath;
            if (!file->openTemporary(path.c_str())) { throw std::runtime_error("cancelled release temporary creation failed"); }
            state->m_files.insert(std::make_pair(index + UINT64_C(1), std::move(file)));
            NetFileIoRequest request = {};
            request.id = index + 1; request.file = index + 1;
            request.operation = index == 0 ? NET_FILE_IO_CLOSE : NET_FILE_IO_PUBLISH;
            request.path = path.c_str(); request.destination = finalPath.c_str();
            std::shared_ptr<NetFileIoEntry> entry(new NetFileIoEntry(request));
            entry->m_cancelled = true;
            state->m_entries.insert(std::make_pair(request.id, entry));
            state->m_queued.push_back(entry);
        }
        for (uint32_t index = 0; index < 2; ++index)
        {
            NetFileIoRequest request = {};
            request.id = index + 3; request.file = index + 1; request.operation = NET_FILE_IO_SIZE;
            std::shared_ptr<NetFileIoEntry> entry(new NetFileIoEntry(request));
            state->m_entries.insert(std::make_pair(request.id, entry));
            state->m_queued.push_back(entry);
        }
        threadId = CTaskThreadManager::Instance().Init();
        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
        if (!thread) { throw std::runtime_error("cancelled release Task creation failed"); }
        thread->PostTask(std::shared_ptr<CTask>(new NetFileIoTask(state)));
        thread.reset();
        const uint64_t deadline = NetSenderSubmission::now() + 3000;
        while (!completed && NetSenderSubmission::now() < deadline)
        {
            {
                std::lock_guard<NetWorkGate> lock(state->m_gate);
                completed = state->m_entries.find(4)->second->m_completed;
                if (completed)
                {
                    released = state->m_entries.find(1)->second->m_result.result == NI_CANCELLED &&
                        state->m_entries.find(2)->second->m_result.result == NI_CANCELLED &&
                        state->m_entries.find(3)->second->m_result.result == NI_NOT_FOUND &&
                        state->m_entries.find(4)->second->m_result.result == NI_NOT_FOUND;
                }
            }
            if (!completed) { netTestCheckpoint(); CSystem::sleepPrecise(1); }
        }
        released = released && !CSystem::fileExists(CStringManager::Utf8ToUnicode(closePath)) &&
            !CSystem::fileExists(CStringManager::Utf8ToUnicode(publishPath)) &&
            !CSystem::fileExists(CStringManager::Utf8ToUnicode(finalPath));
        NetFileIoTask(state).StopTask();
        CTaskThreadManager::Instance().Uninit(threadId);
        threadId = 0;
    }
    catch (...)
    {
        NetFileIoTask(state).StopTask();
        if (threadId != 0) { CTaskThreadManager::Instance().Uninit(threadId); }
        throw;
    }
    if (!completed || !released || CTaskThreadManager::Instance().Count() != before)
    {
        throw std::runtime_error("cancelled CLOSE/PUBLISH retained native handle before backend shutdown");
    }
    std::printf("FILE_CANCELLED_RELEASE close=NOT_FOUND publish=NOT_FOUND temporary_removed=1 final_absent=1 PASS\n");
}

void NetWorkConcurrencyTestCases::submissionLifetime()
{
    uint32_t checks = 0;
#define LIFETIME_CHECK(value) { ++checks; if (!(value)) { std::printf("SUBMISSION_LIFETIME_FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); throw std::runtime_error("submission generation or ownership violated"); } }
    NetSenderSubmission queue;
    uint8_t payload[4] = {19, 31, 47, 61};
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_DATA;
    frame.m_productId = 101;
    frame.m_requestId = 700;
    frame.m_payload = payload;
    frame.m_payloadSize = sizeof(payload);
    LIFETIME_CHECK(queue.start(1, 101) == NI_OK && queue.ready(1, 4, 4096) == NI_OK);
    LIFETIME_CHECK(queue.data(frame) == NI_OK);
    // 新握手不能把已有旧代DATA冒充为新连接输入。
    LIFETIME_CHECK(queue.ready(2, 4, 4096) == NI_PROTOCOL_ERROR);
    uint64_t generation = 0;
    bool ready = false;
    queue.state(generation, ready);
    LIFETIME_CHECK(ready && generation == 1);
    NetSenderSubmissionCommand command = {}, terminal = {};
    LIFETIME_CHECK(queue.beginPoll() == NI_OK && queue.next(command) == NI_READY);
    queue.disconnected(1);
    LIFETIME_CHECK(command.m_generation == 1 && command.m_frame.m_payloadSize == sizeof(payload));
    LIFETIME_CHECK(std::memcmp(command.m_frame.m_payload, payload, sizeof(payload)) == 0);
    LIFETIME_CHECK(!queue.idle() && queue.queuedBytes() == sizeof(payload));
    queue.finish(NI_CANCELLED);
    LIFETIME_CHECK(queue.idle() && queue.queuedBytes() == 0);
    LIFETIME_CHECK(queue.start(2, 101) == NI_OK && queue.ready(2, 4, 4096) == NI_OK);
    uint64_t ticket = 0;
    LIFETIME_CHECK(queue.dataSynchronous(frame, ticket) == NI_OK && ticket != 0);
    LIFETIME_CHECK(queue.beginPoll() == NI_OK && queue.next(command) == NI_READY);
    queue.finish(NI_OK);
    queue.advanceSynchronous(NI_OK);
    LIFETIME_CHECK(!queue.idle() && queue.synchronousResult(ticket, false) == NI_OK);
    // 未读回执仍属于原调用者；不能提前复用整个提交对象。
    LIFETIME_CHECK(queue.start(3, 101) == NI_INVALID_STATE);
    LIFETIME_CHECK(queue.ready(3, 4, 4096) == NI_OK && queue.synchronousResult(ticket, false) == NI_OK);
    LIFETIME_CHECK(queue.synchronousResult(ticket, true) == NI_OK && queue.idle());
    LIFETIME_CHECK(queue.start(3, 101) == NI_OK && queue.ready(3, 4, 4096) == NI_OK);
    uint64_t id = 0;
    LIFETIME_CHECK(queue.begin(101, 77, 5000, id) == NI_OK);
    LIFETIME_CHECK(queue.send(id, payload, sizeof(payload)) == NI_OK && queue.beginPoll() == NI_OK);
    LIFETIME_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN);
    queue.finish(NI_OK);
    LIFETIME_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND);
    LIFETIME_CHECK(!queue.complete(2, id, terminal) && queue.complete(3, id, terminal));
    LIFETIME_CHECK(!queue.complete(3, id, terminal));
    queue.close(NI_TIMEOUT);
    queue.close(NI_CANCELLED);
    queue.disconnected(3);
    LIFETIME_CHECK(command.m_size == sizeof(payload) && std::memcmp(command.m_bytes, payload, sizeof(payload)) == 0);
    queue.finish(NI_OK);
    int32_t reason = 0;
    LIFETIME_CHECK(queue.closeRequested(reason) && reason == NI_TIMEOUT);
    LIFETIME_CHECK(queue.idle() && queue.queuedBytes() == 0);
    std::printf("SUBMISSION_LIFETIME checks=%u PASS\n", checks);
#undef LIFETIME_CHECK
}

// NET_HOST_REDESIGN_TESTS

#include "NetPeer/NetPeerAPI.h"

void NetWorkConcurrencyTestCases::hostLifecycle()
{
#define SOCKET_CHECK(condition) if (!(condition)) { throw std::runtime_error(#condition); }
    {
        NetHostSocketTestDriver driver;
        NetHostFunctions functions = driver.functions();
        NetHostSocketTestListener listener;
        NetHostSocket server(&functions);
        listener.m_socket = &server;
        SOCKET_CHECK(server.listen("127.0.0.1", 0, &listener) == NI_OK && server.poll());
        driver.m_active = false;
        SOCKET_CHECK(!server.poll() && server.status() == NI_PROTOCOL_ERROR && listener.m_closed == 0);
        SOCKET_CHECK(server.uninit() == NI_OK);
    }
    // pauseRead允许同步报告EOF：已经收到的数据必须先于唯一终态交付。
    {
        NetHostSocketTestDriver driver;
        NetHostFunctions functions = driver.functions();
        NetHostSocketTestListener listener;
        listener.m_trace = true;
        NetHostSocket socket(&functions);
        listener.m_socket = &socket;
        uint64_t peer = 0;
        SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_OK);
        driver.m_connected(driver.m_observer, peer);
        socket.poll();
        SOCKET_CHECK(socket.pauseRead(peer, false) == NI_OK);
        const uint8_t data[] = {0x51, 0x52, 0x53};
        driver.m_closeOnPause = true;
        driver.m_data(driver.m_observer, peer, data, sizeof(data));
        SOCKET_CHECK(listener.m_data == 0 && listener.m_closed == 0);
        socket.poll();
        SOCKET_CHECK(listener.m_data == 1 && listener.m_closed == 1 && listener.m_bytes.size() == 3 &&
            std::memcmp(listener.m_bytes.data(), data, 3) == 0 && listener.m_events.size() == 3 &&
            listener.m_events[0] == 1 && listener.m_events[1] == 2 && listener.m_events[2] == 4);
        SOCKET_CHECK(socket.uninit() == NI_OK && driver.m_stops == 1 && driver.m_destroys == 1);
    }
    // 每个peer四类事件全部排满，不能依赖deque扩容或丢弃最后一个EOF。
    {
        NetHostSocketTestDriver driver;
        NetHostFunctions functions = driver.functions();
        NetHostSocketTestListener listener;
        listener.m_bytes.reserve(4194304);
        NetHostSocket socket(&functions);
        listener.m_socket = &socket;
        SOCKET_CHECK(socket.listen("127.0.0.1", 0, &listener, 256) == NI_OK);
        uint8_t data[16384];
        for (uint64_t peer = 1; peer <= 256; ++peer)
        {
            driver.m_connected(driver.m_observer, peer);
            SOCKET_CHECK(socket.pauseRead(peer, false) == NI_OK);
            std::memset(data, static_cast<uint8_t>(peer), sizeof(data));
            driver.m_data(driver.m_observer, peer, data, sizeof(data));
            driver.m_writable(driver.m_observer, peer);
            driver.m_closed(driver.m_observer, peer, 0);
        }
        SOCKET_CHECK(listener.m_connected == 0 && listener.m_data == 0 && listener.m_closed == 0 && socket.peerCount() == 256);
        socket.poll();
        SOCKET_CHECK(listener.m_valid && listener.m_connected == 256 && listener.m_data == 256 && listener.m_writable == 256 &&
            listener.m_closed == 256 && listener.m_bytes.size() == 4194304 && socket.peerCount() == 0);
        for (uint32_t index = 0; index < listener.m_bytes.size(); ++index)
        {
            SOCKET_CHECK(listener.m_bytes[index] == static_cast<uint8_t>(index / 16384 + 1));
        }
        SOCKET_CHECK(socket.uninit() == NI_OK);
    }
    // 回调中恢复读取可同步带入下一块，但不得覆盖当前借用；一轮最多交付2048个事件。
    {
        NetHostSocketTestDriver driver;
        NetHostFunctions functions = driver.functions();
        NetHostSocketTestListener listener;
        listener.m_resumeOnData = true;
        NetHostSocket socket(&functions);
        listener.m_socket = &socket;
        uint64_t peer = 0;
        SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_OK);
        driver.m_connected(driver.m_observer, peer);
        socket.poll();
        driver.m_dataOnResume = 2050;
        SOCKET_CHECK(socket.pauseRead(peer, false) == NI_OK);
        socket.poll();
        SOCKET_CHECK(listener.m_data == 2048 && listener.m_valid && listener.m_closed == 0);
        socket.poll();
        SOCKET_CHECK(listener.m_data == 2050 && listener.m_valid && listener.m_bytes.size() == 2050 * 64);
        for (uint32_t index = 0; index < listener.m_bytes.size(); ++index)
        {
            SOCKET_CHECK(listener.m_bytes[index] == static_cast<uint8_t>(2049 - index / 64));
        }
        socket.close(peer, NI_CANCELLED);
        socket.poll();
        SOCKET_CHECK(listener.m_closed == 1 && socket.uninit() == NI_OK);
    }
    // 函数表每个外部入口的异常均必须收敛，不留下永久执行标记。
    for (uint32_t fault = 1; fault <= 512; fault <<= 1)
    {
        NetHostSocketTestDriver driver;
        NetHostFunctions functions = driver.functions();
        NetHostSocketTestListener listener;
        NetHostSocket socket(&functions);
        listener.m_socket = &socket;
        uint64_t peer = 0;
        if (fault <= 4)
        {
            driver.m_fault = fault;
            const int32_t result = fault == 2 ? socket.listen("127.0.0.1", 0, &listener, 4) :
                socket.connect("127.0.0.1", 14327, &listener, peer);
            SOCKET_CHECK(result < 0 && peer == 0 && !socket.poll() && listener.m_connected == 0 && listener.m_closed == 0);
        }
        else
        {
            SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_OK);
            driver.m_connected(driver.m_observer, peer);
            socket.poll();
            driver.m_fault = fault;
            const uint8_t data = 1;
            if (fault == 8) { SOCKET_CHECK(!socket.poll()); }
            if (fault == 16) { SOCKET_CHECK(socket.write(peer, &data, 1) == NI_INVALID_STATE); }
            if (fault == 32) { SOCKET_CHECK(socket.pauseRead(peer, false) == NI_INVALID_STATE); }
            if (fault == 64) { socket.close(peer, NI_CANCELLED); }
            if (fault == 128) { socket.shutdown(); }
            if (fault == 256)
            {
                SOCKET_CHECK(socket.uninit() == NI_INVALID_STATE && driver.m_destroys == 1);
                driver.m_fault = 0;
            }
            if (fault == 512) { SOCKET_CHECK(socket.localPort() == 0); }
            socket.shutdown();
            SOCKET_CHECK(!socket.poll() && listener.m_closed == 1 && socket.peerCount() == 0);
        }
        SOCKET_CHECK(socket.uninit() == NI_OK && socket.uninit() == NI_OK && !socket.poll());
        SOCKET_CHECK(driver.m_destroys == (fault == 1 ? 0U : fault == 256 ? 2U : 1U));
    }
    // 上层数据回调异常只关闭它自己的peer，不影响同宿主上的另一连接。
    {
        NetHostSocketTestDriver driver;
        NetHostFunctions functions = driver.functions();
        NetHostSocketTestListener listener;
        NetHostSocket socket(&functions);
        listener.m_socket = &socket;
        SOCKET_CHECK(socket.listen("127.0.0.1", 0, &listener, 2) == NI_OK);
        driver.m_connected(driver.m_observer, 1);
        driver.m_connected(driver.m_observer, 2);
        socket.poll();
        listener.m_throwOnData = true;
        SOCKET_CHECK(socket.pauseRead(1, false) == NI_OK);
        const uint8_t byte = 0x34;
        driver.m_data(driver.m_observer, 1, &byte, 1);
        socket.poll();
        SOCKET_CHECK(listener.m_data == 1 && listener.m_closed == 1 && socket.peerCount() == 1);
        listener.m_throwOnData = false;
        SOCKET_CHECK(socket.pauseRead(2, false) == NI_OK);
        driver.m_data(driver.m_observer, 2, &byte, 1);
        socket.poll();
        SOCKET_CHECK(listener.m_data == 2 && listener.m_closed == 1 && socket.status() == NI_OK);
        SOCKET_CHECK(socket.uninit() == NI_OK && listener.m_closed == 2);
    }

    netTestCheckpoint();
#undef SOCKET_CHECK
}

// NET_PEER_FACADE_REDESIGN_TESTS

uint32_t NetPeerTest::m_created = 0;

#include "NetInterface/NetInterfaceAPI.h"

void NetWorkConcurrencyTestCases::peerFacadeLifecycle()
{
#define PEER_CHECK(condition) if (!(condition)) { throw std::runtime_error(#condition); }

    {
        NetPeerTest test;
        NetPeerClientCallbacks callbacks = test.callbacks();
        NetPeerClientHandle handle = nullptr;
        PEER_CHECK(NetPeer::instance().clientCreate(&callbacks, &handle) == NI_OK);
        test.m_client = handle;
        test.m_reenter = NetPeerTest::inspectC;
        NetPeerClientOptions options = NetPeerTest::options();
        uint64_t id = 99;
        PEER_CHECK(NetPeer::instance().clientBegin(handle, 101, 700, 1000, &id) == NI_INVALID_STATE && id == 0);
        PEER_CHECK(NetPeer::instance().clientConfigureGateways(handle, "bad", nullptr) == NI_INVALID_ARGUMENT);
        PEER_CHECK(NetPeer::instance().clientConfigureGateways(handle, "127.0.0.1:1", nullptr) == NI_OK);
        for (uint32_t iteration = 0; iteration < 64; ++iteration)
        {
            options.platform = iteration % 3 + 1;
            PEER_CHECK(NetPeer::instance().clientConnectService(handle, 101, &options) == NI_OK);
            uint64_t generation = 0;
            int32_t ready = 1;
            PEER_CHECK(NetPeer::instance().clientState(handle, &generation, &ready) == NI_OK && generation == iteration + 1 && ready == 0);
            PEER_CHECK(NetPeer::instance().clientDestroy(handle) == NI_INVALID_STATE);
            PEER_CHECK(NetPeer::instance().clientConnect(handle, &options) == NI_INVALID_STATE);
            PEER_CHECK(NetPeer::instance().clientConnectService(handle, 102, &options) == NI_INVALID_STATE);
            PEER_CHECK(NetPeer::instance().clientConfigureCompression(handle, nullptr, 0) == NI_INVALID_STATE);
            PEER_CHECK(NetPeer::instance().clientConfigureGateways(handle, nullptr, nullptr) == NI_INVALID_STATE);
            id = 99;
            PEER_CHECK(NetPeer::instance().clientBegin(handle, 101, 700, 1000, &id) == NI_INVALID_STATE && id == 0);
            test.m_throwEvents = (iteration & 1) ? 8 : 0;
            PEER_CHECK(NetPeer::instance().clientClose(handle, NI_CANCELLED) == NI_OK);
            PEER_CHECK(NetPeer::instance().clientPoll(handle) == NI_OK);
            PEER_CHECK(test.m_closed == iteration + 1 && test.m_generation == iteration + 1 && test.m_other == 0 && !test.m_bad);
            PEER_CHECK(NetPeer::instance().clientPoll(handle) == NI_OK && test.m_closed == iteration + 1);
        }
        const uint32_t invalidPlatforms[] = {0, 4, UINT32_MAX};
        for (uint32_t index = 0; index < 3; ++index)
        {
            options.platform = invalidPlatforms[index];
            PEER_CHECK(NetPeer::instance().clientConnectService(handle, 101, &options) == NI_INVALID_ARGUMENT);
        }
        PEER_CHECK(NetPeer::instance().clientConfigureCompression(handle, nullptr, 0) == NI_OK);
        PEER_CHECK(NetPeer::instance().clientDestroy(handle) == NI_OK);
        PEER_CHECK(NetPeer::instance().clientDestroy(nullptr) == NI_OK);
    }

    {
        NetInterfaceHandshakeOptions server = {};
        server.m_server = 1;
        server.m_capabilities = 7;
        server.m_maxRequests = 4;
        server.m_maxChunk = 4096;
        server.m_timeoutMs = 5000;
        std::memset(server.m_nonce, 0x43, sizeof(server.m_nonce));
        NetSenderDispatchTestChannel channel(server);
        NetPeerTest observer;
        NetPeerClientHandle handle = nullptr;
        const NetPeerClientCallbacks callbacks = observer.callbacks();
        PEER_CHECK(NetPeer::instance().clientCreate(&callbacks, &handle) == NI_OK);
        observer.m_client = handle;
        observer.m_reenter = NetPeerTest::inspectC;
        PEER_CHECK(handle->m_session.bindChannel(&channel) == NI_OK);
        const NetPeerClientOptions options = NetPeerTest::options();
        PEER_CHECK(NetPeer::instance().clientConnect(handle, &options) == NI_OK);
        for (uint32_t step = 0; step < 16 && observer.m_other == 0; ++step) { NetPeer::instance().clientPoll(handle); }
        PEER_CHECK(observer.m_other == 1 && !observer.m_bad);
        uint64_t generation = 0;
        int32_t ready = 0;
        PEER_CHECK(NetPeer::instance().clientState(handle, &generation, &ready) == NI_OK && ready == 1);
        std::vector<uint64_t> submitted;
        for (uint32_t round = 0; round < 128; ++round)
        {
            uint64_t ids[4] = {};
            for (uint32_t index = 0; index < 4; ++index)
            {
                PEER_CHECK(NetPeer::instance().clientBegin(handle, 101, 700, 1000, &ids[index]) == NI_OK);
            }
            for (uint32_t index = 0; index < 4; ++index)
            {
                uint8_t byte = static_cast<uint8_t>(index + 1);
                const uint64_t id = ids[3 - index];
                submitted.push_back(id);
                if (index != 3)
                {
                    PEER_CHECK(NetPeer::instance().clientPost(handle, id, &byte, 1) == NI_OK);
                    PEER_CHECK(channel.m_requests.size() == round * 4);
                }
                else
                {
                    uint64_t ticket = 0;
                    PEER_CHECK(NetPeer::instance().clientSendBegin(handle, id, &byte, 1, &ticket) == NI_OK && ticket != 0);
                    int32_t result = NI_READY;
                    for (uint32_t step = 0; step < 32 && result == NI_READY; ++step)
                    {
                        NetPeer::instance().clientPoll(handle);
                        result = NetPeer::instance().clientSendResult(handle, ticket);
                    }
                    PEER_CHECK(result == NI_OK && channel.m_requests.size() == (round + 1) * 4);
                    PEER_CHECK(NetPeer::instance().clientSendResult(handle, ticket) == NI_NOT_FOUND);
                }
                byte = 99;
            }
            for (uint32_t step = 0; step < 16 && observer.m_resultIds.size() != submitted.size(); ++step) { NetPeer::instance().clientPoll(handle); }
            PEER_CHECK(observer.m_resultIds == submitted && !observer.m_bad);
        }
        for (uint32_t index = 0; index < 512; ++index)
        {
            PEER_CHECK(channel.m_requests[index] == index + 1);
            PEER_CHECK(channel.m_bytes[index] == index % 4 + 1 && observer.m_resultBytes[index] == index % 4 + 1);
        }
        uint64_t cancelled = 0;
        PEER_CHECK(NetPeer::instance().clientBegin(handle, 101, 700, 1000, &cancelled) == NI_OK);
        PEER_CHECK(NetPeer::instance().clientCancel(handle, cancelled) == NI_OK);
        NetPeer::instance().clientPoll(handle);
        PEER_CHECK(observer.m_resultIds.size() == 513 && observer.m_resultIds.back() == cancelled && channel.m_requests.size() == 512);
        PEER_CHECK(NetPeer::instance().clientClose(handle, NI_CANCELLED) == NI_OK);
        PEER_CHECK(NetPeer::instance().clientPoll(handle) == NI_OK && observer.m_closed == 1 && !observer.m_bad);
        PEER_CHECK(NetPeer::instance().clientDestroy(handle) == NI_OK);
    }
    netTestCheckpoint();
#undef PEER_CHECK
}

// NET_DISPATCH_REDESIGN_TESTS

void NetWorkConcurrencyTestCases::dispatchMappingLifecycle()
{
#define DISPATCH_CHECK(condition) if (!(condition)) { throw std::runtime_error(#condition); }

    for (uint32_t mode = 0; mode < 3; ++mode)
    {
        NetInterfaceHandshakeOptions options = {};
        options.m_platform = 1;
        options.m_capabilities = 7;
        options.m_maxRequests = 128;
        options.m_maxChunk = 4096;
        options.m_timeoutMs = 5000;
        std::memset(options.m_device, 0x41, 16);
        std::memset(options.m_nonce, 0x42, 16);
        NetInterfaceHandshakeOptions server = options;
        server.m_server = 1;
        server.m_platform = 0;
        std::memset(server.m_device, 0, 16);
        std::memset(server.m_nonce, 0x43, 16);
        NetSenderDispatchAuditChannel channel(server);
        NetClientSession session;
        NetSenderDispatch dispatch(&session);
        NetSenderDispatchAuditListener listener;
        listener.m_dispatch = &dispatch;
        DISPATCH_CHECK(session.bindChannel(&channel) == NI_OK && dispatch.prepare(&listener) == NI_OK);
        NetInterfaceTlsOptions tls = {};
        tls.handshakeTimeoutMs = 5000;
        tls.entropy = listener.entropy;
        DISPATCH_CHECK(session.connect("mock", 1, tls, options, dispatch.listener()) == NI_OK);
        DISPATCH_CHECK(dispatch.started(session.generation(), 0) == NI_OK);
        for (uint32_t step = 0; step < 16 && listener.m_ready == 0; ++step) { dispatch.poll(); }
        DISPATCH_CHECK(listener.m_ready == 1 && !listener.m_bad);
        channel.m_autoReply = false;
        const uint32_t count = mode == 0 ? 128 : 2;
        uint64_t ids[128] = {};
        uint8_t byte = 7;
        for (uint32_t index = 0; index < count; ++index)
        {
            DISPATCH_CHECK(dispatch.submission().begin(101, 700, 5000, ids[index]) == NI_OK);
        }
        for (uint32_t index = 0; index < count; ++index)
        {
            DISPATCH_CHECK(dispatch.submission().send(ids[count - index - 1], &byte, 1) == NI_OK);
        }
        channel.m_busy = mode == 1;
        if (channel.m_busy)
        {
            for (uint32_t step = 0; step < 128; ++step) { dispatch.poll(); }
            DISPATCH_CHECK(channel.m_requests.empty() && listener.m_ids.empty());
            channel.m_busy = false;
        }
        for (uint32_t step = 0; step < 16 && channel.m_requests.size() != count; ++step) { dispatch.poll(); }
        DISPATCH_CHECK(channel.m_requests.size() == count && listener.m_ids.empty());
        const uint64_t generation = session.generation();
        // 注入不属于当前代的内部回调，不能摘除当前请求或把错误代次报告给业务。
        dispatch.listener()->onSessionResult(generation + 1, channel.m_requests[0], 101, 700, NI_RPC_SUCCESS, &byte, 1);
        dispatch.listener()->onSessionClosed(generation + 1, NI_INVALID_STATE, 0);
        dispatch.listener()->onSessionResult(generation, UINT64_MAX, 101, 700, NI_RPC_SUCCESS, &byte, 1);
        DISPATCH_CHECK(listener.m_ids.empty() && listener.m_closed == 0);
        uint64_t observed = 0;
        bool ready = false;
        dispatch.submission().state(observed, ready);
        DISPATCH_CHECK(observed == generation && ready);
        if (mode == 2)
        {
            for (uint32_t index = 0; index < count; ++index)
            {
                DISPATCH_CHECK(dispatch.submission().cancel(ids[index]) == NI_OK);
            }
            for (uint32_t step = 0; step < 8 && listener.m_ids.size() != count; ++step) { dispatch.poll(); }
            DISPATCH_CHECK(listener.m_ids.size() == count);
            for (uint32_t index = 0; index < count; ++index) { DISPATCH_CHECK(listener.m_status[index] == NI_RPC_CANCELLED); }
        }
        else
        {
            for (uint32_t index = 0; index < count; ++index)
            {
                channel.m_payload.resize(17);
                uint32_t required = 0;
                DISPATCH_CHECK(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, &byte, 1,
                    channel.m_payload.data(), 17, required) == NI_OK);
                channel.m_reply = NetInterfaceFrame();
                channel.m_reply.m_kind = NI_RESPONSE;
                channel.m_reply.m_productId = 101;
                channel.m_reply.m_messageType = 700;
                channel.m_reply.m_requestId = channel.m_requests[count - index - 1];
                channel.m_reply.m_payload = channel.m_payload.data();
                channel.m_reply.m_payloadSize = required;
                dispatch.poll();
                DISPATCH_CHECK(listener.m_ids.size() == index + 1 && listener.m_ids.back() == ids[index] && listener.m_status.back() == NI_RPC_SUCCESS);
            }
        }
        dispatch.shutdown();
        DISPATCH_CHECK(dispatch.quiescent() && listener.m_closed == 1 && !listener.m_bad);
    }
    netTestCheckpoint();
#undef DISPATCH_CHECK
}

// NET_FILE_FACADE_REDESIGN_TESTS
void NetWorkConcurrencyTestCases::fileFacadeLifecycle()
{
    // NetWorkTest保持进程唯一的真实原生后端；故障后端由库的独立注释main隔离验证。
    for (uint32_t round = 0; round < 32; ++round)
    {
        NetFileIo io;
        if (io.init() != NI_OK) { throw std::runtime_error("file facade initialize"); }
        bool valid = true;
        uint64_t id = 0;
        valid = valid && io.nextRequestId(id) == NI_OK && id != 0;
        NetFileIoRequest request = {};
        request.id = id;
        request.file = 999;
        request.operation = NET_FILE_IO_CLOSE;
        valid = valid && io.submit(request) == NI_OK;
        NetFileIoResult result = {};
        int32_t status = NI_NEED_MORE;
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (status == NI_NEED_MORE && std::chrono::steady_clock::now() < until)
        {
            status = io.peek(result);
            if (status == NI_NEED_MORE) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
        }
        valid = valid && status == NI_READY && result.id == id && result.file == 999 && result.operation == NET_FILE_IO_CLOSE &&
            result.result < 0 && result.size == 0 && result.bytes == nullptr;
        if (status == NI_READY) { valid = valid && io.consume(id) == NI_OK; }
        io.stop();
        id = 99;
        valid = valid && io.nextRequestId(id) == NI_INVALID_STATE && id == 0 && io.submit(request) == NI_INVALID_STATE;
        const auto stopped = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (!io.done() && std::chrono::steady_clock::now() < stopped)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        valid = valid && io.done();
        io.uninit();
        if (!valid) { throw std::runtime_error("file facade task output, stop or cleanup contract"); }
        netTestCheckpoint();
    }
}

// NET_FILE_PROTOCOL_REDESIGN_TESTS
#define NET_FILE_AUDIT_NO_HEAP 1
#include <vector>
#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#include <malloc.h>
#endif

#undef NET_FILE_AUDIT_NO_HEAP
#undef FILE_VERIFY

void NetWorkConcurrencyTestCases::fileProtocolLifecycle()
{
    if (!NetInterfaceFileAuditTest::run())
    {
        throw std::runtime_error("file codec, persistent prefix, final commit or duplicate boundary");
    }
    netTestCheckpoint();
}

#define NET_SERVICE_QUEUE_NO_HEAP
#include "NetSender/NetSenderAPI.h"
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_QUEUE_NO_HEAP)
#include <crtdbg.h>
#include <malloc.h>
#endif

// 只在隔离测试程序中计数，不进入库或业务进程。
static uint32_t s_serviceQueueChecks = 0;
#define SERVICE_QUEUE_CHECK(value) { ++s_serviceQueueChecks; if (!(value)) { std::printf("SERVICE_QUEUE_FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); return false; } }

/** 检查三队列共有额度、拥有期、取消和期限，使用真实队列而非替代实现
@return 全部边界是否满足
*/
static bool ServiceQueueBoundaries()
{
    NetSenderServiceQueue queue;
    NetInterfaceFrame frame = {};
    NetInterfaceOrigin origin = {};
    origin.m_endpoint = 91;
    uint8_t bytes[32];
    std::memset(bytes, 0x59, sizeof(bytes));
    frame.m_kind = NI_EVENT;
    frame.m_productId = 101;
    frame.m_messageType = 700;
    frame.m_payload = bytes;
    frame.m_payloadSize = sizeof(bytes);
    SERVICE_QUEUE_CHECK(queue.size() == 0 && queue.bytes() == 0 && queue.room(256, 8 * 1048576));
    SERVICE_QUEUE_CHECK(!queue.room(257, 0) && !queue.room(1, UINT64_MAX));
    SERVICE_QUEUE_CHECK(queue.first(3) == 256 && queue.last(3) == 256 && queue.size(3) == 0);
    SERVICE_QUEUE_CHECK(queue.get(256) == nullptr && queue.borrow(256) == nullptr && !queue.restore(256));
    SERVICE_QUEUE_CHECK(queue.push(3, 1, frame, origin, 1000) == NI_INVALID_ARGUMENT);
    frame.m_payload = nullptr;
    SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_INVALID_ARGUMENT);
    frame.m_payload = bytes;
    frame.m_payloadSize = UINT32_MAX;
    SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_LIMIT_EXCEEDED);
    frame.m_payloadSize = sizeof(bytes);
    for (uint32_t index = 0; index < 256; ++index)
    {
        frame.m_requestId = index + 1;
        SERVICE_QUEUE_CHECK(queue.push(index % 3, index + 1, frame, origin, 1000) == NI_OK);
    }
    SERVICE_QUEUE_CHECK(queue.size() == 256 && queue.bytes() == 256 * sizeof(bytes));
    SERVICE_QUEUE_CHECK(queue.push(1, 1, frame, origin, 1000) == NI_LIMIT_EXCEEDED);
    std::memset(bytes, 0xa9, sizeof(bytes));
    for (uint32_t lane = 0; lane < 3; ++lane)
    {
        uint64_t expected = lane + 1;
        while (queue.first(lane) != 256)
        {
            const uint32_t index = queue.first(lane);
            NetSenderServiceManagedFrame* item = queue.borrow(index);
            SERVICE_QUEUE_CHECK(item != nullptr && item->m_frame.m_requestId == expected && item->m_bytes[0] == 0x59);
            SERVICE_QUEUE_CHECK(queue.borrow(index) == nullptr);
            const uint32_t owned = queue.size();
            SERVICE_QUEUE_CHECK(queue.restore(index) && queue.size() == owned && queue.first(lane) == index);
            SERVICE_QUEUE_CHECK(!queue.restore(index));
            queue.release(index);
            queue.release(index);
            SERVICE_QUEUE_CHECK(queue.size() + 1 == owned && queue.get(index) == nullptr);
            expected += 3;
        }
        SERVICE_QUEUE_CHECK(queue.last(lane) == 256 && queue.size(lane) == 0);
    }
    SERVICE_QUEUE_CHECK(queue.size() == 0 && queue.bytes() == 0);

    // 无分配归队与清空：清空其他消息仍须保留回调手里的正文及预算。
    for (uint32_t round = 0; round < 1000; ++round)
    {
        SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
        const uint32_t index = queue.first(0);
        NetSenderServiceManagedFrame* borrowed = queue.borrow(index);
        SERVICE_QUEUE_CHECK(borrowed != nullptr);
        {
            NetSenderServiceQueueLoan loan(queue, index);
            SERVICE_QUEUE_CHECK(queue.push(1, 2, frame, origin, 1000) == NI_OK);
            SERVICE_QUEUE_CHECK(queue.push(2, 3, frame, origin, 1000) == NI_OK);
            queue.clear();
            SERVICE_QUEUE_CHECK(queue.size() == 1 && queue.bytes() == sizeof(bytes) && borrowed->m_bytes[0] == 0xa9);
            SERVICE_QUEUE_CHECK(loan.restore() && queue.size(0) == 1 && queue.size() == 1);
        }
        queue.clear();
        SERVICE_QUEUE_CHECK(queue.size() == 0 && queue.bytes() == 0);
    }
    SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
    {
        const uint32_t index = queue.first(0);
        SERVICE_QUEUE_CHECK(queue.borrow(index) != nullptr);
        NetSenderServiceQueueLoan loan(queue, index);
        loan.release();
        SERVICE_QUEUE_CHECK(queue.size() == 0);
        SERVICE_QUEUE_CHECK(queue.push(0, 2, frame, origin, 1000) == NI_OK);
    }
    SERVICE_QUEUE_CHECK(queue.size() == 1 && queue.get(queue.first(0))->m_endpoint == 2);
    queue.clear();
    bool caught = false;
    try
    {
        SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
        const uint32_t index = queue.first(0);
        SERVICE_QUEUE_CHECK(queue.borrow(index) != nullptr);
        NetSenderServiceQueueLoan loan(queue, index);
        throw std::runtime_error("controlled callback failure");
    }
    catch (...)
    {
        caught = true;
    }
    SERVICE_QUEUE_CHECK(caught && queue.size() == 0 && queue.bytes() == 0);

    std::vector<uint8_t> large(1048576, 0x59);
    frame.m_payload = large.data();
    frame.m_payloadSize = static_cast<uint32_t>(large.size());
    for (uint32_t index = 0; index < 8; ++index)
    {
        SERVICE_QUEUE_CHECK(queue.push(index % 3, index, frame, origin, 1000) == NI_OK);
    }
    const uint32_t borrowedIndex = queue.first(0);
    SERVICE_QUEUE_CHECK(queue.borrow(borrowedIndex) != nullptr);
    SERVICE_QUEUE_CHECK(queue.size() == 8 && queue.bytes() == 8 * 1048576);
    SERVICE_QUEUE_CHECK(queue.push(0, 9, frame, origin, 1000) == NI_LIMIT_EXCEEDED);
    queue.clear();
    SERVICE_QUEUE_CHECK(queue.size() == 1 && queue.bytes() == 1048576);
    queue.release(borrowedIndex);
    SERVICE_QUEUE_CHECK(queue.size() == 0 && queue.bytes() == 0);

    frame.m_payload = nullptr;
    frame.m_payloadSize = 0;
    for (uint32_t index = 0; index < 256; ++index)
    {
        SERVICE_QUEUE_CHECK(queue.push(0, index, frame, origin, 1000) == NI_OK);
    }
    SERVICE_QUEUE_CHECK(queue.size() == 256 && queue.bytes() == 0);
    SERVICE_QUEUE_CHECK(queue.push(0, 9, frame, origin, 1000) == NI_LIMIT_EXCEEDED);
    queue.clear();

    // 取消仅删除完整匹配的待交付请求，不能取消别的端点、产品或命令。
    uint8_t request[48] = {};
    uint32_t required = 0;
    SERVICE_QUEUE_CHECK(NetInterfaceRpc::encode(NI_REQUEST, 5000, 0, bytes, sizeof(bytes), request, sizeof(request), required) == NI_OK);
    frame.m_kind = NI_REQUEST;
    frame.m_payload = request;
    frame.m_payloadSize = required;
    for (uint32_t index = 0; index < 5; ++index)
    {
        frame.m_requestId = index + 1;
        SERVICE_QUEUE_CHECK(queue.push(0, 7, frame, origin, 1000) == NI_OK);
    }
    NetInterfaceFrame cancel = frame;
    cancel.m_kind = NI_CANCEL;
    cancel.m_payload = nullptr;
    cancel.m_payloadSize = 0;
    const uint32_t cancelled[3] = {1, 3, 5};
    for (uint32_t index = 0; index < 3; ++index)
    {
        cancel.m_requestId = cancelled[index];
        SERVICE_QUEUE_CHECK(queue.push(0, 7, cancel, origin, 1000) == NI_OK && queue.size() == 4 - index);
    }
    SERVICE_QUEUE_CHECK(queue.get(queue.first(0))->m_frame.m_requestId == 2);
    SERVICE_QUEUE_CHECK(queue.get(queue.last(0))->m_frame.m_requestId == 4);
    cancel.m_requestId = 2;
    SERVICE_QUEUE_CHECK(queue.push(0, 8, cancel, origin, 1000) == NI_OK && queue.size() == 3);
    ++cancel.m_productId;
    SERVICE_QUEUE_CHECK(queue.push(0, 7, cancel, origin, 1000) == NI_OK && queue.size() == 4);
    --cancel.m_productId;
    ++cancel.m_messageType;
    SERVICE_QUEUE_CHECK(queue.push(0, 7, cancel, origin, 1000) == NI_OK && queue.size() == 5);
    --cancel.m_messageType;
    SERVICE_QUEUE_CHECK(queue.push(1, 7, cancel, origin, 1000) == NI_OK && queue.size() == 6);
    const uint32_t active = queue.first(0);
    SERVICE_QUEUE_CHECK(queue.borrow(active) != nullptr);
    SERVICE_QUEUE_CHECK(queue.push(0, 7, cancel, origin, 1000) == NI_OK && queue.size() == 7);
    queue.clear();
    SERVICE_QUEUE_CHECK(queue.size() == 1);
    {
        NetSenderServiceQueueScratch scratch;
        NetInterfaceFrame view = {};
        NetInterfaceRpcView rpc = {};
        SERVICE_QUEUE_CHECK(queue.get(active)->view(999, view, scratch.m_request) == NI_PROTOCOL_ERROR);
        SERVICE_QUEUE_CHECK(queue.get(active)->view(2000, view, scratch.m_request) == NI_READY);
        SERVICE_QUEUE_CHECK(NetInterfaceRpc::parse(view, rpc) == NI_OK && rpc.m_value == 4000 && rpc.m_size == 32);
        SERVICE_QUEUE_CHECK(queue.restore(active) && queue.borrow(active) != nullptr);
        SERVICE_QUEUE_CHECK(queue.get(active)->view(5999, view, scratch.m_request) == NI_READY);
        SERVICE_QUEUE_CHECK(NetInterfaceRpc::parse(view, rpc) == NI_OK && rpc.m_value == 1);
        SERVICE_QUEUE_CHECK(queue.get(active)->view(6000, view, scratch.m_request) == NI_TIMEOUT);
    }
    queue.release(active);
    frame.m_payloadSize = 1;
    SERVICE_QUEUE_CHECK(queue.push(0, 7, frame, origin, 1000) == NI_OK);
    {
        NetSenderServiceQueueScratch scratch;
        NetInterfaceFrame view = {};
        SERVICE_QUEUE_CHECK(queue.get(queue.first(0))->view(1000, view, scratch.m_request) == NI_PROTOCOL_ERROR);
    }
    queue.clear();
    return true;
}

#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_QUEUE_NO_HEAP)
// 隔离main的CRT观察器，不在有其他业务线程的NetWorkTest中启用。
static int32_t s_serviceQueueRejectAfter = -1;
static uint32_t s_serviceQueueAllocations = 0;
static uint32_t s_serviceQueueExposed = 0;

/** 只拦截普通堆块，核对失败回滚、零分配清理和释放前正文擦除
@param [in] operation CRT操作类别
@param [in] pointer 释放时仍有效的堆块
@param [in] block 堆块类别
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 允许本次操作时为1
*/
static int __cdecl ServiceQueueMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if (block != _NORMAL_BLOCK)
    {
        return 1;
    }
    if (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC)
    {
        ++s_serviceQueueAllocations;
        if (s_serviceQueueRejectAfter == 0)
        {
            return 0;
        }
        if (s_serviceQueueRejectAfter > 0)
        {
            --s_serviceQueueRejectAfter;
        }
    }
    if (operation == _HOOK_FREE && pointer != nullptr && _msize_dbg(pointer, _NORMAL_BLOCK) >= 32)
    {
        const uint8_t* bytes = static_cast<const uint8_t*>(pointer);
        uint32_t index = 0;
        while (index < 32 && bytes[index] == 0x59)
        {
            ++index;
        }
        if (index == 32)
        {
            ++s_serviceQueueExposed;
        }
    }
    return 1;
}

/** 在真实静态库内拒绝分配，回滚后必须可重试；释放借用不准再次分配
@return 内存路径是否通过
*/
static bool ServiceQueueMemory()
{
    uint8_t bytes[8192];
    std::memset(bytes, 0x59, sizeof(bytes));
    NetInterfaceOrigin origin = {};
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_EVENT;
    frame.m_payload = bytes;
    frame.m_payloadSize = sizeof(bytes);
    frame.m_productId = 101;
    frame.m_messageType = 700;
    frame.m_requestId = 1;
    NetSenderServiceQueue queue;
    s_serviceQueueRejectAfter = 0;
    _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(ServiceQueueMemoryHook);
    const int32_t rejected = queue.push(0, 1, frame, origin, 1000);
    _CrtSetAllocHook(previous);
    s_serviceQueueRejectAfter = -1;
    SERVICE_QUEUE_CHECK(rejected == NI_NO_MEMORY && queue.size() == 0 && queue.bytes() == 0);
    SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
    const uint32_t index = queue.first(0);
    SERVICE_QUEUE_CHECK(queue.borrow(index) != nullptr);
    s_serviceQueueRejectAfter = 0;
    s_serviceQueueAllocations = s_serviceQueueExposed = 0;
    previous = _CrtSetAllocHook(ServiceQueueMemoryHook);
    const bool restored = queue.restore(index);
    queue.clear();
    _CrtSetAllocHook(previous);
    s_serviceQueueRejectAfter = -1;
    SERVICE_QUEUE_CHECK(restored && s_serviceQueueAllocations == 0 && s_serviceQueueExposed == 0 && queue.size() == 0);

    frame.m_payloadSize = 1024;
    SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
    queue.clear();
    s_serviceQueueAllocations = 0;
    s_serviceQueueRejectAfter = 0;
    bool warm = true;
    previous = _CrtSetAllocHook(ServiceQueueMemoryHook);
    for (uint32_t round = 0; round < 100000; ++round)
    {
        warm = warm && queue.push(round % 3, 1, frame, origin, 1000) == NI_OK;
        const uint32_t current = queue.first(round % 3);
        warm = warm && queue.borrow(current) != nullptr;
        queue.release(current);
    }
    _CrtSetAllocHook(previous);
    s_serviceQueueRejectAfter = -1;
    SERVICE_QUEUE_CHECK(warm && s_serviceQueueAllocations == 0 && queue.size() == 0 && queue.bytes() == 0);

    uint8_t request[48] = {};
    uint32_t required = 0;
    SERVICE_QUEUE_CHECK(NetInterfaceRpc::encode(NI_REQUEST, 5000, 0, bytes, 32, request, sizeof(request), required) == NI_OK);
    frame.m_kind = NI_REQUEST;
    frame.m_payload = request;
    frame.m_payloadSize = required;
    SERVICE_QUEUE_CHECK(queue.push(0, 1, frame, origin, 1000) == NI_OK);
    {
        NetSenderServiceQueueScratch scratch;
        NetInterfaceFrame view = {};
        s_serviceQueueRejectAfter = 0;
        previous = _CrtSetAllocHook(ServiceQueueMemoryHook);
        const int32_t result = queue.get(queue.first(0))->view(1001, view, scratch.m_request);
        _CrtSetAllocHook(previous);
        s_serviceQueueRejectAfter = -1;
        SERVICE_QUEUE_CHECK(result == NI_NO_MEMORY && queue.size() == 1 && queue.bytes() == required);
        SERVICE_QUEUE_CHECK(queue.get(queue.first(0))->view(1001, view, scratch.m_request) == NI_READY);
    }
    queue.clear();
    bool caught = false;
    previous = _CrtSetAllocHook(ServiceQueueMemoryHook);
    try
    {
        NetSenderServiceQueueScratch scratch;
        scratch.m_request.assign(bytes, bytes + sizeof(bytes));
        scratch.m_event.assign(bytes, bytes + sizeof(bytes));
        scratch.m_wrapped.assign(bytes, bytes + sizeof(bytes));
        throw std::runtime_error("controlled scratch failure");
    }
    catch (...)
    {
        caught = true;
    }
    _CrtSetAllocHook(previous);
    SERVICE_QUEUE_CHECK(caught && s_serviceQueueExposed == 0);
    return true;
}

#endif

/** 汇总真实队列的持有期和资源预算测试
@return 是否通过
*/
static bool ServiceQueueAudit()
{
    s_serviceQueueChecks = 0;
    SERVICE_QUEUE_CHECK(ServiceQueueBoundaries());
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_QUEUE_NO_HEAP)
    SERVICE_QUEUE_CHECK(ServiceQueueMemory());
#endif
    return true;
}

#ifndef NET_SERVICE_QUEUE_NO_HEAP
/** 库内手工测试入口，正式构建保持整段注释
@return 0通过，其余表示失败
*/
int main()
{
    try
    {
        std::printf("Service queue audit starting\n");
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemState before;
        _CrtMemState after;
        _CrtMemState difference;
        _CrtMemCheckpoint(&before);
#endif
        if (!ServiceQueueAudit())
        {
            return 1;
        }
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemCheckpoint(&after);
        _CrtMemDifference(&difference, &before, &after);
        if (difference.lCounts[_NORMAL_BLOCK] != 0 || difference.lSizes[_NORMAL_BLOCK] != 0)
        {
            std::printf("Service queue heap blocks=%ld bytes=%ld FAIL\n", difference.lCounts[_NORMAL_BLOCK], difference.lSizes[_NORMAL_BLOCK]);
            return 2;
        }
#endif
        std::printf("Service queue audit checks=%u PASS\n", s_serviceQueueChecks);
        return 0;
    }
    catch (...)
    {
        return 3;
    }
}

#endif
#undef SERVICE_QUEUE_CHECK

#undef NET_SERVICE_QUEUE_NO_HEAP

void NetWorkConcurrencyTestCases::serviceQueueOwnership()
{
    if (!ServiceQueueAudit())
    {
        throw std::runtime_error("service queue ownership or bounded resource regression");
    }
    netTestCheckpoint();
}

#define NET_SERVICE_GATEWAY_NO_HEAP
#include <google/protobuf/stubs/common.h>

#ifndef NET_SERVICE_GATEWAY_NO_HEAP
/** 库内服务生命周期测试，正式构建保持注释
@return 0通过，其他为失败
*/
int main()
{
    try
    {
        std::printf("Service gateway lifecycle starting\n");
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemState before;
        _CrtMemState after;
        _CrtMemState difference;
        _CrtMemCheckpoint(&before);
#endif
        if (!NetSenderServiceGatewayTest::run())
        {
            return 1;
        }
#if defined(_WIN32) && defined(_DEBUG)
        // 独立main在全部对象销毁后回收protobuf全局资源，不能提前预热隐藏冷启动失败。
        google::protobuf::ShutdownProtobufLibrary();
        _CrtMemCheckpoint(&after);
        _CrtMemDifference(&difference, &before, &after);
        if (difference.lCounts[_NORMAL_BLOCK] != 0 || difference.lSizes[_NORMAL_BLOCK] != 0)
        {
            std::printf("Service gateway heap blocks=%ld bytes=%ld FAIL\n", difference.lCounts[_NORMAL_BLOCK], difference.lSizes[_NORMAL_BLOCK]);
            return 2;
        }
#endif
        std::printf("Service gateway lifecycle PASS\n");
        return 0;
    }
    catch (...)
    {
        return 3;
    }
}

#endif

#undef NET_SERVICE_GATEWAY_NO_HEAP

void NetWorkConcurrencyTestCases::serviceGatewayLifecycle()
{
    if (!NetSenderServiceGatewayTest::run())
    {
        throw std::runtime_error("service gateway lifecycle or callback convergence failed");
    }
    netTestCheckpoint();
}

#define NET_SERVICE_OUTGOING_NO_HEAP
#include "ProtoMessage/ProtoMessageAPI.h"

#ifndef NET_SERVICE_OUTGOING_NO_HEAP
/** 库内独立测试main，正式构建保持逐行注释
@return 0通过，其他为失败
*/
int main()
{
    try
    {
#if defined(_WIN32) && defined(_DEBUG)
        _CrtMemState before, after;
        _CrtMemCheckpoint(&before);
#endif
        if (!NetSenderServiceOutgoingTest::run()) return 1;
#if defined(_WIN32) && defined(_DEBUG)
        google::protobuf::ShutdownProtobufLibrary();
        _CrtMemCheckpoint(&after);
        const long long blocks = static_cast<long long>(after.lCounts[_NORMAL_BLOCK]) - before.lCounts[_NORMAL_BLOCK];
        const long long bytes = static_cast<long long>(after.lSizes[_NORMAL_BLOCK]) - before.lSizes[_NORMAL_BLOCK];
        if (blocks != 0 || bytes != 0)
        {
            std::printf("Outgoing heap blocks=%lld bytes=%lld FAIL\n", blocks, bytes);
            return 2;
        }
#endif
        std::printf("Service outgoing boundary and lifecycle PASS\n");
        return 0;
    }
    catch (...)
    {
        return 3;
    }
}

#endif
#undef NET_SERVICE_OUTGOING_NO_HEAP

void NetWorkConcurrencyTestCases::serviceOutgoingLifecycle()
{
    if (!NetSenderServiceOutgoingTest::run())
    {
        throw std::runtime_error("service outgoing boundary or lifecycle failed");
    }
    netTestCheckpoint();
}

#define NET_SERVICE_PROXY_NO_MAIN
#define NET_SERVICE_PROXY_NO_HEAP

#ifndef NET_SERVICE_PROXY_NO_MAIN
int main()
{
    try
    {
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_PROXY_NO_HEAP)
        _CrtMemState before = {}, after = {};
        _CrtMemCheckpoint(&before);
#endif
        if (!NetSenderServiceProxyTest::run())
        {
            return 1;
        }

#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_SERVICE_PROXY_NO_HEAP)
        _CrtMemCheckpoint(&after);
        const long long blocks = static_cast<long long>(after.lCounts[_NORMAL_BLOCK]) - before.lCounts[_NORMAL_BLOCK];
        const long long bytes = static_cast<long long>(after.lSizes[_NORMAL_BLOCK]) - before.lSizes[_NORMAL_BLOCK];
        if (blocks != 0 || bytes != 0)
        {
            std::printf("Service proxy heap blocks=%lld bytes=%lld FAIL\n", blocks, bytes);
            return 2;
        }
#endif
        std::puts("Service proxy fixed FIFO, budget and exception recovery PASS");
        return 0;
    }
    catch (...)
    {
        return 3;
    }
}

#endif

#undef NET_SERVICE_PROXY_NO_MAIN
#undef NET_SERVICE_PROXY_NO_HEAP

void NetWorkConcurrencyTestCases::serviceProxyLifecycle()
{
    if (!NetSenderServiceProxyTest::run())
    {
        throw std::runtime_error("service proxy bounded FIFO or lifecycle failed");
    }
    netTestCheckpoint();
}

#define NET_SERVICE_CLOCK_NO_MAIN

#ifndef NET_SERVICE_CLOCK_NO_MAIN
int main()
{
    try
    {
        if (!NetSenderServiceGatewayTest::clockBoundary())
        {
            return 1;
        }
        std::puts("Service callback clock future/equal/timeout boundaries PASS");
        return 0;
    }
    catch (...)
    {
        return 2;
    }
}

#endif

#undef NET_SERVICE_CLOCK_NO_MAIN

void NetWorkConcurrencyTestCases::serviceCallbackClock()
{
    if (!NetSenderServiceGatewayTest::clockBoundary())
    {
        throw std::runtime_error("service callback clock boundary failed");
    }
    netTestCheckpoint();
}

// 与NetWorkHostBridge.cpp注释main同源，进程内回归不替换全局堆钩子。
#define NET_WORK_HOST_NO_MAIN
#define NET_WORK_HOST_NO_HEAP
#include <CSystem/CSystemAPI.h>

#ifndef NET_WORK_HOST_NO_MAIN
int main()
{
    const int32_t result = NetWorkHostBridgeTest::audit();
    if (result == 0) { std::printf("PASS\n"); }
    return result;
}

#endif
#undef NET_WORK_HOST_NO_MAIN
#undef NET_WORK_HOST_NO_HEAP
#undef HOST_CHECK

void NetWorkConcurrencyTestCases::hostBridgeLifecycle()
{
    if (NetWorkHostBridgeTest::audit() != 0)
    {
        throw std::runtime_error("native host bridge lifetime or isolation audit failed");
    }
    netTestCheckpoint();
}

#define NATIVE_CLEANUP_NO_MAIN
#define NATIVE_CLEANUP_NO_HEAP
#include <CStringManager/CStringManagerAPI.h>
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
#include <crtdbg.h>
// 测试专用分配预算：-1不限，0开始拒绝普通堆分配
static std::atomic<int32_t> s_cleanupRejectAfter(-1);
// 当前注入窗口观察到的普通堆分配次数
static std::atomic<uint32_t> s_cleanupAllocations(0);
/** 隔离main统计或拒绝普通堆分配
@param [in] operation CRT操作
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 非零允许分配，0拒绝
*/
static int __cdecl CleanupMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
    {
        ++s_cleanupAllocations;
        if (s_cleanupRejectAfter == 0)
        {
            return 0;
        }
        if (s_cleanupRejectAfter > 0)
        {
            --s_cleanupRejectAfter;
        }
    }
    return 1;
}

#endif

/** 在独占测试目录检查Unicode临时文件的低内存关闭和工作线程回收
@return 是否全部通过
*/
static bool NativeCleanupAudit()
{
    uint32_t checks = 0;
#define CLEANUP_CHECK(value) do { ++checks; if (!(value)) { std::printf("Native cleanup line=%d FAIL\n", __LINE__); return false; } } while (0)
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
    {
        NetFileIoState cold;
        CLEANUP_CHECK(cold.m_gate.valid());
        bool escaped = false;
        s_cleanupRejectAfter = 0;
        s_cleanupAllocations = 0;
        const _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(CleanupMemoryHook);
        try
        {
            cold.stop();
            cold.stop();
        }
        catch (...)
        {
            escaped = true;
        }
        _CrtSetAllocHook(oldHook);
        s_cleanupRejectAfter = -1;
        CLEANUP_CHECK(!escaped && cold.m_stop.load() && s_cleanupAllocations == 0);
    }
#endif
    const std::string path = CSystem::canonicalPathUtf8(CStringManager::UnicodeToUtf8(L"native-cleanup-\u6e05\u7406.tmp"), false);
    const std::wstring wide = CStringManager::Utf8ToUnicode(path);
    CLEANUP_CHECK(!path.empty() && !CSystem::fileExists(wide));
    for (uint32_t mode = 0; mode < 3; ++mode)
    {
        std::unique_ptr<NetFileIoHandle> file(new NetFileIoHandle());
        CLEANUP_CHECK(file->openTemporary(path.c_str()) && CSystem::fileExists(wide));
        bool escaped = false;
        bool cleaned = false;
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
        s_cleanupRejectAfter = 0;
        s_cleanupAllocations = 0;
        const _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(CleanupMemoryHook);
#endif
        try
        {
            if (mode == 0)
            {
                cleaned = file->close() && file->close();
            }
            else
            {
                file.reset();
                cleaned = true;
            }
        }
        catch (...)
        {
            escaped = true;
        }
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
        _CrtSetAllocHook(oldHook);
        s_cleanupRejectAfter = -1;
        CLEANUP_CHECK(s_cleanupAllocations == 0);
#endif
        CLEANUP_CHECK(!escaped && cleaned && !CSystem::fileExists(wide));
    }
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
    _CrtMemState before, after;
    _CrtMemCheckpoint(&before);
    uint32_t rejected = 0;
    uint32_t accepted = 0;
    for (int32_t position = 0; position < 80; ++position)
    {
        NetFileIoHandle file;
        bool opened = false;
        s_cleanupRejectAfter = position;
        const _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(CleanupMemoryHook);
        try
        {
            opened = file.openTemporary(path.c_str());
        }
        catch (...)
        {
            // 创建路径的分配允许失败，但尚未取得临时文件删除责任。
        }
        _CrtSetAllocHook(oldHook);
        s_cleanupRejectAfter = -1;
        if (opened)
        {
            ++accepted;
            CLEANUP_CHECK(CSystem::fileExists(wide));
        }
        else
        {
            ++rejected;
            CLEANUP_CHECK(!CSystem::fileExists(wide));
        }
        CLEANUP_CHECK(file.close() && !CSystem::fileExists(wide));
    }
    _CrtMemCheckpoint(&after);
    CLEANUP_CHECK(rejected != 0 && accepted != 0);
    CLEANUP_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
    std::printf("NATIVE_CLEANUP allocation_positions=80 rejected=%u accepted=%u cleanup_allocations=0 heap_delta=0\n", rejected, accepted);
#endif
    // 实际异步文件执行者持有临时句柄；在持续分配失败期间停止并释放。
    NetNativeFileIo io;
    CLEANUP_CHECK(io.init() == NI_OK);
    NetFileIoRequest request = {};
    request.operation = NET_FILE_IO_OPEN_TEMPORARY;
    request.path = path.c_str();
    CLEANUP_CHECK(io.nextRequestId(request.id) == NI_OK && io.submit(request) == NI_OK);
    NetFileIoResult result = {};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (io.peek(result) != NI_READY && std::chrono::steady_clock::now() < deadline)
    {
        CSystem::sleepPrecise(1);
    }
    CLEANUP_CHECK(result.id == request.id && result.result == NI_OK && result.file != 0);
    CLEANUP_CHECK(io.consume(result.id) == NI_OK && CSystem::fileExists(wide));
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
    s_cleanupRejectAfter = 0;
    const _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(CleanupMemoryHook);
#endif
    const auto stopDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    io.stop();
    while (!io.done() && std::chrono::steady_clock::now() < stopDeadline)
    {
        CSystem::sleepPrecise(1);
    }
    const bool done = io.done();
#if defined(_WIN32) && defined(_DEBUG) && !defined(NATIVE_CLEANUP_NO_HEAP)
    _CrtSetAllocHook(oldHook);
    s_cleanupRejectAfter = -1;
#endif
    io.uninit();
    CLEANUP_CHECK(done && !CSystem::fileExists(wide));
    // 覆盖通知先到/执行者先等待两种时序；每次生命周期独立，不保留后台线程。
    for (uint32_t round = 0; round < 16; ++round)
    {
        NetNativeFileIo idle;
        CLEANUP_CHECK(idle.init() == NI_OK);
        if ((round & 1U) != 0)
        {
            CSystem::sleepPrecise(1);
        }
        idle.stop();
        idle.stop();
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!idle.done() && std::chrono::steady_clock::now() < until)
        {
            CSystem::sleepPrecise(1);
        }
        CLEANUP_CHECK(idle.done());
        idle.uninit();
    }
    // 结果未consume前仍计入容量；执行者再快也不能使未消费结果无限增长。
    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        NetNativeFileIo bounded;
        CLEANUP_CHECK(bounded.init() == NI_OK);
        const uint32_t capacity = mode == 0 ? 64U : 32U;
        std::vector<uint8_t> input(mode == 0 ? 0 : 65536, 0x53);
        for (uint32_t item = 0; item < capacity; ++item)
        {
            NetFileIoRequest queued = {};
            queued.operation = mode == 0 ? NET_FILE_IO_SIZE : NET_FILE_IO_WRITE;
            queued.file = UINT64_MAX;
            queued.size = static_cast<uint32_t>(input.size());
            queued.bytes = input.empty() ? nullptr : input.data();
            CLEANUP_CHECK(bounded.nextRequestId(queued.id) == NI_OK && queued.id == item + 1);
            CLEANUP_CHECK(bounded.submit(queued) == NI_OK);
        }
        NetFileIoRequest overflow = {};
        overflow.operation = mode == 0 ? NET_FILE_IO_SIZE : NET_FILE_IO_WRITE;
        overflow.size = static_cast<uint32_t>(input.size());
        overflow.bytes = input.empty() ? nullptr : input.data();
        CLEANUP_CHECK(bounded.nextRequestId(overflow.id) == NI_OK);
        CLEANUP_CHECK(bounded.submit(overflow) == NI_LIMIT_EXCEEDED);
        bounded.stop();
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!bounded.done() && std::chrono::steady_clock::now() < until)
        {
            CSystem::sleepPrecise(1);
        }
        CLEANUP_CHECK(bounded.done());
        for (uint32_t item = 0; item < capacity; ++item)
        {
            NetFileIoResult completed = {};
            CLEANUP_CHECK(bounded.peek(completed) == NI_READY && completed.id == item + 1);
            CLEANUP_CHECK(completed.result == NI_NOT_FOUND || completed.result == NI_CANCELLED);
            CLEANUP_CHECK(bounded.consume(completed.id) == NI_OK);
        }
        NetFileIoResult empty = {};
        CLEANUP_CHECK(bounded.peek(empty) == NI_NEED_MORE);
        CLEANUP_CHECK(bounded.submit(overflow) == NI_INVALID_STATE);
        bounded.uninit();
    }
    std::printf("Native file cleanup checks=%u PASS\n", checks);
    return true;
#undef CLEANUP_CHECK
}

#ifndef NATIVE_CLEANUP_NO_MAIN
int main()
{
    try
    {
        return NativeCleanupAudit() ? 0 : 1;
    }
    catch (...)
    {
        std::puts("Native file cleanup unexpected exception FAIL");
        return 2;
    }
}

#endif
#undef NATIVE_CLEANUP_NO_MAIN
#undef NATIVE_CLEANUP_NO_HEAP

void NetWorkConcurrencyTestCases::nativeFileCleanup()
{
    if (!NativeCleanupAudit())
    {
        throw std::runtime_error("native temporary file cleanup audit failed");
    }
    netTestCheckpoint();
}

#define CSTRING_CONVERSION_NO_MAIN
#define CSTRING_CONVERSION_NO_HEAP
#include <new>
#ifdef _WIN32
#include <windows.h>
#endif
#if defined(_WIN32) && defined(_DEBUG) && !defined(CSTRING_CONVERSION_NO_HEAP)
#include <crtdbg.h>
// 独立main串行调用编码接口，计数器不供其他线程访问。
static int32_t s_conversionRejectAfter = -1;
static uint32_t s_conversionAllocations = 0;
static uint32_t s_conversionPayloadAllocations = 0;
/** 对编码分配逐点持续拒绝，验证异常收敛和结果缓冲的唯一所有权
@param [in] operation CRT操作
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 非零允许分配，零拒绝
*/
static int __cdecl ConversionMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)request;
    (void)filename;
    (void)line;
    if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
    {
        ++s_conversionAllocations;
        if (size >= 256)
        {
            ++s_conversionPayloadAllocations;
        }
        if (s_conversionRejectAfter == 0)
        {
            return 0;
        }
        if (s_conversionRejectAfter > 0)
        {
            --s_conversionRejectAfter;
        }
    }
    return 1;
}

#endif

/** 检查六种编码的内容、内嵌零、系统替代规则和低内存收敛
@return 全部通过返回true
*/
static bool CStringConversionAudit()
{
    uint32_t checks = 0;
#define CONVERSION_CHECK(value) do { ++checks; if (!(value)) { std::printf("CString conversion line=%d FAIL\n", __LINE__); return false; } } while (0)
    const std::string narrow(256, 'a');
    const std::wstring wide(256, L'a');
    CONVERSION_CHECK(CStringManager::AnsiToUnicode(narrow) == wide);
    CONVERSION_CHECK(CStringManager::UnicodeToAnsi(wide) == narrow);
    CONVERSION_CHECK(CStringManager::Utf8ToUnicode(narrow) == wide);
    CONVERSION_CHECK(CStringManager::UnicodeToUtf8(wide) == narrow);
    CONVERSION_CHECK(CStringManager::AnsiToUtf8(narrow) == narrow);
    CONVERSION_CHECK(CStringManager::Utf8ToAnsi(narrow) == narrow);
    CONVERSION_CHECK(CStringManager::AnsiToUnicode("").empty());
    CONVERSION_CHECK(CStringManager::UnicodeToAnsi(L"").empty());
    CONVERSION_CHECK(CStringManager::Utf8ToUnicode("").empty());
    CONVERSION_CHECK(CStringManager::UnicodeToUtf8(L"").empty());
    CONVERSION_CHECK(CStringManager::AnsiToUtf8("").empty());
    CONVERSION_CHECK(CStringManager::Utf8ToAnsi("").empty());
    const std::string zeroBytes("abc\0def", 7);
    const std::wstring zeroWide(L"abc\0def", 7);
    CONVERSION_CHECK(CStringManager::AnsiToUnicode(zeroBytes) == zeroWide);
    CONVERSION_CHECK(CStringManager::UnicodeToAnsi(zeroWide) == zeroBytes);
    CONVERSION_CHECK(CStringManager::Utf8ToUnicode(zeroBytes) == zeroWide);
    CONVERSION_CHECK(CStringManager::UnicodeToUtf8(zeroWide) == zeroBytes);
    CONVERSION_CHECK(CStringManager::AnsiToUtf8(zeroBytes) == zeroBytes);
    CONVERSION_CHECK(CStringManager::Utf8ToAnsi(zeroBytes) == zeroBytes);
#ifdef _WIN32
    const wchar_t unicodeData[] = { L'A', 0, 0x4e2d, 0xd83d, 0xde00, L'Z' };
    const std::wstring unicode(unicodeData, 6);
    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        const UINT codePage = mode == 0 ? CP_UTF8 : CP_ACP;
        const int bytes = WideCharToMultiByte(codePage, 0, unicode.data(), 6, nullptr, 0, nullptr, nullptr);
        CONVERSION_CHECK(bytes > 0);
        std::string expected(static_cast<size_t>(bytes), '\0');
        CONVERSION_CHECK(WideCharToMultiByte(codePage, 0, unicode.data(), 6, &expected[0], bytes, nullptr, nullptr) == bytes);
        const std::string actual = mode == 0 ? CStringManager::UnicodeToUtf8(unicode) : CStringManager::UnicodeToAnsi(unicode);
        CONVERSION_CHECK(actual == expected);
        const int characters = MultiByteToWideChar(codePage, 0, expected.data(), bytes, nullptr, 0);
        CONVERSION_CHECK(characters > 0);
        std::wstring back(static_cast<size_t>(characters), L'\0');
        CONVERSION_CHECK(MultiByteToWideChar(codePage, 0, expected.data(), bytes, &back[0], characters) == characters);
        CONVERSION_CHECK((mode == 0 ? CStringManager::Utf8ToUnicode(expected) : CStringManager::AnsiToUnicode(expected)) == back);
        if (mode == 0)
        {
            CONVERSION_CHECK(back == unicode);
        }
    }
    // flags=0沿用系统替代规则；修复所有权时不暗中改成严格拒绝输入。
    const std::string invalid("\xff\xc0\x80\0Z", 5);
    wchar_t expectedInvalid[16] = {};
    const int invalidCount = MultiByteToWideChar(CP_UTF8, 0, invalid.data(), 5, expectedInvalid, 16);
    CONVERSION_CHECK(invalidCount > 0);
    CONVERSION_CHECK(CStringManager::Utf8ToUnicode(invalid) == std::wstring(expectedInvalid, static_cast<size_t>(invalidCount)));
#endif
#if defined(_WIN32) && defined(_DEBUG) && !defined(CSTRING_CONVERSION_NO_HEAP)
    uint32_t rejected = 0;
    uint32_t accepted = 0;
    uint32_t allocatorExceptions = 0;
    for (uint32_t mode = 0; mode < 6; ++mode)
    {
        for (int32_t position = 0; position < 8; ++position)
        {
            _CrtMemState before, after;
            _CrtMemCheckpoint(&before);
            bool escaped = false;
            bool allocationFailed = false;
            bool full = false;
            bool empty = false;
            s_conversionRejectAfter = position;
            s_conversionAllocations = 0;
            s_conversionPayloadAllocations = 0;
            const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(ConversionMemoryHook);
            try
            {
                if (mode == 0 || mode == 2)
                {
                    std::wstring result;
                    if (mode == 0)
                    {
                        result = CStringManager::Utf8ToUnicode(narrow);
                    }
                    else
                    {
                        result = CStringManager::AnsiToUnicode(narrow);
                    }
                    full = result == wide;
                    empty = result.empty();
                }
                else
                {
                    std::string result;
                    if (mode == 1)
                    {
                        result = CStringManager::UnicodeToUtf8(wide);
                    }
                    else if (mode == 3)
                    {
                        result = CStringManager::UnicodeToAnsi(wide);
                    }
                    else if (mode == 4)
                    {
                        result = CStringManager::AnsiToUtf8(narrow);
                    }
                    else
                    {
                        result = CStringManager::Utf8ToAnsi(narrow);
                    }
                    full = result == narrow;
                    empty = result.empty();
                }
            }
            catch (const std::bad_alloc&)
            {
                // VS2013 Debug连空字符串的迭代器代理都需要分配；边界捕获此分配异常。
                allocationFailed = true;
            }
            catch (...)
            {
                escaped = true;
            }
            _CrtSetAllocHook(old);
            s_conversionRejectAfter = -1;
            _CrtMemCheckpoint(&after);
            if (escaped || (!full && !empty && !allocationFailed))
            {
                std::printf("CSTRING_FAULT mode=%u position=%d escaped=%u allocations=%u\n", mode, position, escaped ? 1U : 0U, s_conversionAllocations);
            }
            CONVERSION_CHECK(!escaped && (full || empty || allocationFailed));
            if (before.lCounts[_NORMAL_BLOCK] != after.lCounts[_NORMAL_BLOCK] || before.lSizes[_NORMAL_BLOCK] != after.lSizes[_NORMAL_BLOCK])
            {
                std::printf("CSTRING_HEAP mode=%u position=%d blocks=%lld bytes=%lld allocation_failed=%u\n", mode, position,
                    static_cast<long long>(after.lCounts[_NORMAL_BLOCK]) - static_cast<long long>(before.lCounts[_NORMAL_BLOCK]),
                    static_cast<long long>(after.lSizes[_NORMAL_BLOCK]) - static_cast<long long>(before.lSizes[_NORMAL_BLOCK]), allocationFailed ? 1U : 0U);
            }
            CONVERSION_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
            if (full)
            {
                ++accepted;
                CONVERSION_CHECK(s_conversionPayloadAllocations == (mode < 4 ? 1U : 2U));
            }
            else
            {
                ++rejected;
                allocatorExceptions += allocationFailed ? 1U : 0U;
            }
        }
    }
    CONVERSION_CHECK(accepted > 0 && rejected >= 8);
    std::printf("CSTRING_CONVERSION allocation_positions=48 accepted=%u rejected=%u allocator_exceptions=%u heap_delta=0 direct_payload_allocations=1\n", accepted, rejected, allocatorExceptions);
#endif
    std::printf("CString conversion checks=%u PASS\n", checks);
    return true;
#undef CONVERSION_CHECK
}

#ifndef CSTRING_CONVERSION_NO_MAIN
/** 独立回归入口；启用前解除整段注释，测试后恢复
@return 零表示通过
*/
int main()
{
    try
    {
        return CStringConversionAudit() ? 0 : 1;
    }
    catch (...)
    {
        std::puts("CString conversion unexpected exception FAIL");
        return 2;
    }
}

#endif
#undef CSTRING_CONVERSION_NO_MAIN
#undef CSTRING_CONVERSION_NO_HEAP

void NetWorkConcurrencyTestCases::stringConversion()
{
    if (!CStringConversionAudit())
    {
        throw std::runtime_error("string conversion audit failed");
    }
    netTestCheckpoint();
}

#define LIBUV_ACCEPT_NO_MAIN
#define LIBUV_ACCEPT_NO_HEAP
#if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_ACCEPT_NO_HEAP)
#include <crtdbg.h>
// 仅独立main在唯一poll线程安装，业务回调不分配测试观测节点。
static uint32_t s_acceptFaultMode = 0;
static uint32_t s_acceptRejected = 0;
static bool s_acceptPeerAllocated = false;
/** 只拒绝封装层连接对象或其紧随的map登记，第三方网络事件仍正常推进
@param [in] operation CRT操作，其余参数沿用CRT原型
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 是否允许当前分配
*/
static int __cdecl AcceptMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)request;
    (void)filename;
    (void)line;
    if (operation != _HOOK_ALLOC || block != _NORMAL_BLOCK)
    {
        return 1;
    }
    if (s_acceptFaultMode == 2 && s_acceptPeerAllocated)
    {
        s_acceptPeerAllocated = false;
        ++s_acceptRejected;
        return 0;
    }
    if (size == sizeof(LibuvChannelPeer))
    {
        if (s_acceptFaultMode == 1)
        {
            ++s_acceptRejected;
            return 0;
        }
        s_acceptPeerAllocated = s_acceptFaultMode == 2;
    }
    return 1;
}

#endif

/** 记录真正由服务端接纳的连接号
@param [in] context 测试状态
@param [in] peer 连接号
*/
static void AcceptConnected(void* context, uint64_t peer)
{
    LibuvAcceptTestState* state = static_cast<LibuvAcceptTestState*>(context);
    state->m_peer = peer;
    ++state->m_connected;
}

/** 按累计字节位置核对内容，不依赖TCP分包方式
@param [in] context 测试状态
@param [in] peer 收到数据的连接
@param [in] bytes 借用输入
@param [in] size 实际长度
*/
static void AcceptData(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    LibuvAcceptTestState* state = static_cast<LibuvAcceptTestState*>(context);
    state->m_valid = state->m_valid && peer == state->m_peer && size != 0 && bytes != nullptr;
    for (uint32_t index = 0; index < size; ++index)
    {
        state->m_valid = state->m_valid && bytes[index] == static_cast<uint8_t>((state->m_received + index) % 251);
    }
    state->m_received += size;
}

/** 本用例不使用写完成推动后续工作
@param [in] context 测试状态
@param [in] peer 连接号
*/
static void AcceptWritable(void* context, uint64_t peer)
{
    (void)context;
    (void)peer;
}

/** 统计终态且不在回调中销毁宿主
@param [in] context 测试状态
@param [in] peer 连接号
@param [in] reason 终态原因
*/
static void AcceptClosed(void* context, uint64_t peer, int32_t reason)
{
    (void)peer;
    (void)reason;
    ++static_cast<LibuvAcceptTestState*>(context)->m_closed;
}

/** 推进本用例的四个独占实例，不改变调用线程
@param [in] hosts 服务端和三个客户端，可含空实例
*/
static void AcceptTick(void** hosts)
{
    for (uint32_t index = 0; index < 4; ++index)
    {
        if (hosts[index] != nullptr)
        {
            LibuvTcp::hostPoll(hosts[index]);
        }
    }
    CSystem::sleepPrecise(1);
}

/** 在临时端口验证接纳恢复及满员拒绝不影响已有连接
@param [in] mode 0正常，1拒绝连接对象，2拒绝登记节点；分配注入仅独立Debug main启用
@return 是否通过并已关闭全部实例
*/
static bool LibuvAcceptRound(uint32_t mode)
{
    void* hosts[4] = {};
    LibuvAcceptTestState states[4];
    bool valid = true;
    uint32_t checks = 0;
#define ACCEPT_CHECK(value) { ++checks; if (!(value)) { std::printf("Libuv accept mode=%u line=%d FAIL\n", mode, __LINE__); valid = false; break; } }
    do
    {
        for (uint32_t index = 0; index < 4; ++index)
        {
            if (LibuvTcp::hostCreate(nullptr, &states[index], AcceptConnected, AcceptData, AcceptWritable, AcceptClosed, &hosts[index]) != 0)
            {
                valid = false;
                break;
            }
        }
        ACCEPT_CHECK(valid);
        ACCEPT_CHECK(LibuvTcp::hostListen(hosts[0], "127.0.0.1", 0, 1) == 0);
        const uint32_t port = LibuvTcp::hostLocalPort(hosts[0]);
        uint64_t peer = 0;
        ACCEPT_CHECK(LibuvTcp::hostConnect(hosts[1], "127.0.0.1", port, &peer) == 0);
#if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_ACCEPT_NO_HEAP)
        if (mode != 0)
        {
            s_acceptFaultMode = mode;
            s_acceptRejected = 0;
            s_acceptPeerAllocated = false;
            for (uint32_t tick = 0; tick < 32; ++tick)
            {
                LibuvTcp::hostPoll(hosts[1]);
                const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(AcceptMemoryHook);
                LibuvTcp::hostPoll(hosts[0]);
                _CrtSetAllocHook(old);
                CSystem::sleepPrecise(1);
            }
            s_acceptFaultMode = 0;
            ACCEPT_CHECK(s_acceptRejected != 0 && states[0].m_connected == 0 && states[1].m_connected == 1);
        }
#endif
        // 不创建第二个连接刺激事件；只在资源恢复后poll，第一条连接必须能自行接纳。
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while ((states[0].m_connected != 1 || states[1].m_connected != 1) && std::chrono::steady_clock::now() < deadline)
        {
            AcceptTick(hosts);
        }
        ACCEPT_CHECK(states[0].m_connected == 1 && states[1].m_connected == 1);
        uint8_t bytes[16384];
        for (uint32_t index = 0; index < sizeof(bytes); ++index)
        {
            bytes[index] = static_cast<uint8_t>(index % 251);
        }
        ACCEPT_CHECK(LibuvTcp::hostWrite(hosts[1], states[1].m_peer, bytes, sizeof(bytes)) == 0);
        deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (states[0].m_received != sizeof(bytes) && std::chrono::steady_clock::now() < deadline)
        {
            AcceptTick(hosts);
        }
        ACCEPT_CHECK(states[0].m_valid && states[0].m_received == sizeof(bytes));
        const uint64_t firstPeer = states[0].m_peer;
        ACCEPT_CHECK(LibuvTcp::hostConnect(hosts[2], "127.0.0.1", port, &peer) == 0);
        deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (states[2].m_closed == 0 && std::chrono::steady_clock::now() < deadline)
        {
            AcceptTick(hosts);
        }
        ACCEPT_CHECK(states[2].m_closed == 1 && states[0].m_closed == 0 && LibuvTcp::hostPeerCount(hosts[0]) == 1);
        ACCEPT_CHECK(LibuvTcp::hostClose(hosts[0], firstPeer, 0) == 0);
        deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while ((states[0].m_closed != 1 || states[1].m_closed != 1) && std::chrono::steady_clock::now() < deadline)
        {
            AcceptTick(hosts);
        }
        ACCEPT_CHECK(states[0].m_closed == 1 && states[1].m_closed == 1 && LibuvTcp::hostPeerCount(hosts[0]) == 0);
        ACCEPT_CHECK(LibuvTcp::hostConnect(hosts[3], "127.0.0.1", port, &peer) == 0);
        deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (states[0].m_connected != 2 && std::chrono::steady_clock::now() < deadline)
        {
            AcceptTick(hosts);
        }
        ACCEPT_CHECK(states[0].m_connected == 2 && states[0].m_peer > firstPeer);
    } while (false);
    for (uint32_t index = 4; index != 0; --index)
    {
        if (hosts[index - 1] != nullptr && LibuvTcp::hostDestroy(hosts[index - 1]) != 0)
        {
            valid = false;
        }
    }
    valid = valid && states[0].m_closed == states[0].m_connected && states[0].m_valid;
    std::printf("Libuv accept mode=%u checks=%u received=%llu %s\n", mode, checks,
        static_cast<unsigned long long>(states[0].m_received), valid ? "PASS" : "FAIL");
    return valid;
#undef ACCEPT_CHECK
}

/** 执行真实TCP恢复及Debug逐层分配拒绝
@return 是否全部通过
*/
static bool LibuvAcceptAudit()
{
    if (!LibuvAcceptRound(0))
    {
        return false;
    }
#if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_ACCEPT_NO_HEAP)
    for (uint32_t mode = 1; mode <= 2; ++mode)
    {
        _CrtMemState before, after;
        _CrtMemCheckpoint(&before);
        if (!LibuvAcceptRound(mode))
        {
            return false;
        }
        _CrtMemCheckpoint(&after);
        if (before.lCounts[_NORMAL_BLOCK] != after.lCounts[_NORMAL_BLOCK] || before.lSizes[_NORMAL_BLOCK] != after.lSizes[_NORMAL_BLOCK])
        {
            std::puts("Libuv accept heap delta FAIL");
            return false;
        }
    }
#endif
    std::puts("Libuv accept recovery PASS");
    return true;
}

#ifndef LIBUV_ACCEPT_NO_MAIN
/** 独立测试入口，正式库中整段保持注释
@return 零表示通过
*/
int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    return LibuvAcceptAudit() ? 0 : 1;
}

#endif

#undef LIBUV_ACCEPT_NO_MAIN
#undef LIBUV_ACCEPT_NO_HEAP

void NetWorkConcurrencyTestCases::libuvAcceptRecovery()
{
    if (!LibuvAcceptAudit())
    {
        throw std::runtime_error("libuv accept recovery or capacity audit failed");
    }
    netTestCheckpoint();
}

#define LIBUV_LIFECYCLE_NO_MAIN
#define LIBUV_LIFECYCLE_NO_HEAP
#include <memory>
#if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_LIFECYCLE_NO_HEAP)
#include <crtdbg.h>
// 独立main唯一调用线程使用，不与其他CRT故障注入并行。
static uint32_t s_lifecycleAllocations = 0;
/** 拒绝全部普通堆分配，验证清理操作不依赖新的内存
@param [in] operation CRT操作
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 是否允许当前操作
*/
static int __cdecl LifecycleMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
    {
        ++s_lifecycleAllocations;
        return 0;
    }
    return 1;
}

#endif

/** 验证未启动和已监听实例在内存耗尽时仍能停止、收敛和析构
@return 全部通过为true
*/
static bool LibuvLifecycleAudit()
{
    uint32_t checks = 0;
#define LIFECYCLE_CHECK(value) { ++checks; if (!(value)) { std::printf("Libuv lifecycle line=%d FAIL\n", __LINE__); return false; } }
    for (uint32_t mode = 0; mode < 3; ++mode)
    {
        std::unique_ptr<LibuvTcp> owner(new LibuvTcp());
        if (mode != 0)
        {
            LIFECYCLE_CHECK(owner->initServer(0));
        }
        bool escaped = false;
        bool released = false;
#if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_LIFECYCLE_NO_HEAP)
        s_lifecycleAllocations = 0;
        const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(LifecycleMemoryHook);
#endif
        try
        {
            if (mode != 2)
            {
                owner->stop();
                owner->stop();
                owner->send(nullptr);
                owner->close(nullptr);
                owner->loop();
                released = owner->loopPtr() == nullptr && !owner->isClient() && owner->m_libuv == nullptr;
            }
            else
            {
                // 未调用loop，析构仍负责关闭初始化阶段已经取得的TCP和唤醒句柄。
                released = true;
            }
            owner.reset();
        }
        catch (...)
        {
            escaped = true;
        }
#if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_LIFECYCLE_NO_HEAP)
        _CrtSetAllocHook(old);
        LIFECYCLE_CHECK(s_lifecycleAllocations == 0);
#endif
        LIFECYCLE_CHECK(!escaped && released && !owner);
    }
    std::printf("Libuv lifecycle cold/initialized/destructor checks=%u PASS\n", checks);
    return true;
#undef LIFECYCLE_CHECK
}

#ifndef LIBUV_LIFECYCLE_NO_MAIN
/** 独立测试入口；正式库保留整段注释
@return 零表示成功
*/
int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    return LibuvLifecycleAudit() ? 0 : 1;
}

#endif

#undef LIBUV_LIFECYCLE_NO_MAIN
#undef LIBUV_LIFECYCLE_NO_HEAP

void NetWorkConcurrencyTestCases::libuvLifecycle()
{
    if (!LibuvLifecycleAudit())
    {
        throw std::runtime_error("libuv lifecycle audit failed");
    }
    netTestCheckpoint();
}

#define WORK_LIFECYCLE_NO_MAIN
#define WORK_LIFECYCLE_NO_HEAP
#if defined(_WIN32) && defined(_DEBUG) && !defined(WORK_LIFECYCLE_NO_HEAP)
#include <crtdbg.h>
// 独立main只在本调用线程故障注入，不与后台任务或其他注入并行。
static uint32_t s_workAllocations = 0;
/** 拒绝全部普通堆分配
@param [in] operation 本次分配操作
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 是否允许
*/
static int __cdecl WorkLifecycleMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
    {
        ++s_workAllocations;
        return 0;
    }
    return 1;
}

#endif

// 无连接、无任务的测试句柄不允许产生任何业务回调。
static uint32_t s_workUnexpectedCallbacks = 0;
/** 记录非预期就绪
@param [in] context 调用者上下文，本用例不使用
@param [in] generation 会话代次，本用例不应产生
@param [in] requests 协商请求数，本用例不使用
@param [in] chunk 协商分块字节数，本用例不使用
*/
static void WorkLifecycleReady(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
{
    (void)context;
    (void)generation;
    (void)requests;
    (void)chunk;
    ++s_workUnexpectedCallbacks;
}

/** 记录非预期结果
@param [in] context 调用者上下文，本用例不使用
@param [in] generation 会话代次，本用例不应产生
@param [in] requestId 请求号，本用例未发送请求
@param [in] product 产品号，本用例不使用
@param [in] type 业务类型，本用例不使用
@param [in] status 终态码，本用例不使用
@param [in] bytes 仅借用至返回的正文，本用例不读取
@param [in] size 正文字节数，本用例不读取
*/
static void WorkLifecycleResult(void* context, uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    (void)context;
    (void)generation;
    (void)requestId;
    (void)product;
    (void)type;
    (void)status;
    (void)bytes;
    (void)size;
    ++s_workUnexpectedCallbacks;
}

/** 记录非预期数据
@param [in] context 调用者上下文，本用例不使用
@param [in] generation 会话代次，本用例不应产生
@param [in] frame 仅借用至返回的帧，本用例不读取
@return 1已消费；本用例另行记录非预期回调
*/
static int32_t WorkLifecycleFrame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
{
    (void)context;
    (void)generation;
    (void)frame;
    ++s_workUnexpectedCallbacks;
    return 1;
}

/** 记录非预期关闭
@param [in] context 调用者上下文，本用例不使用
@param [in] generation 会话代次，本用例不应产生
@param [in] reason 关闭原因，本用例不使用
@param [in] detail 底层诊断码，本用例不使用
*/
static void WorkLifecycleClosed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    (void)context;
    (void)generation;
    (void)reason;
    (void)detail;
    ++s_workUnexpectedCallbacks;
}

/** 验证第一次使用同步门时即使内存不足，也可关闭、观察和销毁未连接客户端
@return 全部独立预期成立为true
*/
static bool NetWorkLifecycleAudit()
{
    uint32_t checks = 0;
    s_workUnexpectedCallbacks = 0;
#define WORK_LIFECYCLE_CHECK(value) { ++checks; if (!(value)) { std::printf("Work lifecycle line=%d FAIL\n", __LINE__); return false; } }
    for (uint32_t round = 0; round < 16; ++round)
    {
#if defined(_WIN32) && defined(_DEBUG) && !defined(WORK_LIFECYCLE_NO_HEAP)
        _CrtMemState before = {};
        _CrtMemCheckpoint(&before);
#endif
        NetWorkClientCallbacks callbacks = {};
        callbacks.structSize = sizeof(callbacks);
        callbacks.version = 1;
        callbacks.ready = WorkLifecycleReady;
        callbacks.result = WorkLifecycleResult;
        callbacks.frame = WorkLifecycleFrame;
        callbacks.closed = WorkLifecycleClosed;
        NetWorkClientHandle client = nullptr;
        WORK_LIFECYCLE_CHECK(NetWork::instance().clientCreate(&callbacks, &client) == NI_OK && client != nullptr);
        uint64_t generation = 99;
        int32_t ready = 99;
#if defined(_WIN32) && defined(_DEBUG) && !defined(WORK_LIFECYCLE_NO_HEAP)
        s_workAllocations = 0;
        const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(WorkLifecycleMemoryHook);
#endif
        const int32_t closed = NetWork::instance().clientClose(client, NI_CANCELLED);
        const int32_t repeated = NetWork::instance().clientClose(client, NI_INVALID_STATE);
        const int32_t state = NetWork::instance().clientState(client, &generation, &ready);
        const int32_t polled = NetWork::instance().clientPoll(client);
        const int32_t destroyed = NetWork::instance().clientDestroy(client);
#if defined(_WIN32) && defined(_DEBUG) && !defined(WORK_LIFECYCLE_NO_HEAP)
        _CrtSetAllocHook(old);
        _CrtMemState after = {};
        _CrtMemCheckpoint(&after);
        WORK_LIFECYCLE_CHECK(s_workAllocations == 0);
        WORK_LIFECYCLE_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
#endif
        WORK_LIFECYCLE_CHECK(closed == NI_OK && repeated == NI_OK && state == NI_OK && polled == NI_OK && destroyed == NI_OK);
        WORK_LIFECYCLE_CHECK(generation == 0 && ready == 0 && s_workUnexpectedCallbacks == 0);
    }
    std::printf("Work lifecycle cold close/state/destroy checks=%u PASS\n", checks);
    return true;
#undef WORK_LIFECYCLE_CHECK
}

#ifndef WORK_LIFECYCLE_NO_MAIN
/** 独立测试入口；正式库保留整段注释
@return 零表示成功
*/
int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    return NetWorkLifecycleAudit() ? 0 : 1;
}

#endif

#undef WORK_LIFECYCLE_NO_MAIN
#undef WORK_LIFECYCLE_NO_HEAP

void NetWorkConcurrencyTestCases::workLifecycle()
{
    if (!NetWorkLifecycleAudit())
    {
        throw std::runtime_error("NetWork lifecycle audit failed");
    }
    netTestCheckpoint();
}

#define CSTRING_FORMAT_NO_MAIN
#define CSTRING_FORMAT_NO_HEAP
#include <clocale>
#if defined(_WIN32) && defined(_DEBUG) && !defined(CSTRING_FORMAT_NO_HEAP)
#include <crtdbg.h>
// 单线程main的持续故障位置，不与后台任务或其他CRT注入并行。
static uint32_t s_formatRemaining = 0;
static uint32_t s_formatRejected = 0;
/** 从指定普通堆分配开始持续拒绝
@param [in] operation 本次操作
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 是否允许
*/
static int __cdecl FormatMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
    {
        if (s_formatRemaining == 0)
        {
            ++s_formatRejected;
            return 0;
        }
        --s_formatRemaining;
    }
    return 1;
}

#endif

/** 验证四个Format重载的独立预期、输入别名、宽文本和分配失败
@return 全部检查通过为true
*/
static bool CStringFormatAudit()
{
    uint32_t checks = 0;
#define FORMAT_CHECK(value) { ++checks; if (!(value)) { std::printf("CString format line=%d FAIL\n", __LINE__); return false; } }
    FORMAT_CHECK(CStringManager::Format("%s:%04d:%.2f", "value", 7, 1.25) == "value:0007:1.25");
    FORMAT_CHECK(CStringManager::Format(L"%ls:%04d:%.2f", L"value", 7, 1.25) == L"value:0007:1.25");
    FORMAT_CHECK(CStringManager::Format(static_cast<const char*>(nullptr)).empty());
    FORMAT_CHECK(CStringManager::Format(static_cast<const wchar_t*>(nullptr)).empty());
    FORMAT_CHECK(CStringManager::Format("").empty() && CStringManager::Format(L"").empty());
    FORMAT_CHECK(CStringManager::Format(L"\u4e2d:%ls:%lc", L"\u6587", L'\u56fd') == L"\u4e2d:\u6587:\u56fd");
    FORMAT_CHECK(CStringManager::Format("a%cb", 0) == std::string("a\0b", 3));
    FORMAT_CHECK(CStringManager::Format(L"a%lcb", static_cast<wint_t>(0)) == std::wstring(L"a\0b", 3));
    const uint32_t sizes[] = {1, 15, 16, 255, 256, 257, 4096, 8192};
    for (uint32_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
    {
        const uint32_t size = sizes[index];
        const std::string expected = std::string(size, 'x') + std::string(4095, ' ') + "7";
        const std::wstring wideExpected = std::wstring(size, L'x') + std::wstring(4095, L' ') + L"7";
        std::string value(size, 'x');
        CStringManager::Format(value, "%s%4096d", value.c_str(), 7);
        FORMAT_CHECK(value == expected);
        std::wstring wide(size, L'x');
        CStringManager::Format(wide, L"%ls%4096d", wide.c_str(), 7);
        FORMAT_CHECK(wide == wideExpected);
        FORMAT_CHECK(CStringManager::Format("%0*d", static_cast<int>(size), 7) == std::string(size - 1, '0') + "7");
        FORMAT_CHECK(CStringManager::Format(L"%0*d", static_cast<int>(size), 7) == std::wstring(size - 1, L'0') + L"7");
    }
    std::string format = "%s%04096d";
    CStringManager::Format(format, format.c_str(), "a", 7);
    FORMAT_CHECK(format == "a" + std::string(4095, '0') + "7");
    std::wstring wideFormat = L"%ls%04096d";
    CStringManager::Format(wideFormat, wideFormat.c_str(), L"a", 7);
    FORMAT_CHECK(wideFormat == L"a" + std::wstring(4095, L'0') + L"7");
    CStringManager::Format(format, static_cast<const char*>(nullptr));
    CStringManager::Format(wideFormat, static_cast<const wchar_t*>(nullptr));
    FORMAT_CHECK(format.empty() && wideFormat.empty());
#if defined(_WIN32) && defined(_DEBUG) && !defined(CSTRING_FORMAT_NO_HEAP)
    uint32_t rejected = 0;
    uint32_t accepted = 0;
    const std::string expected = std::string(4095, '0') + "7";
    const std::wstring wideExpected = std::wstring(4095, L'0') + L"7";
    for (uint32_t mode = 0; mode < 4; ++mode)
    {
        for (uint32_t position = 0; position < 8; ++position)
        {
            _CrtMemState before = {};
            _CrtMemCheckpoint(&before);
            bool valid = false;
            bool escaped = false;
            {
                std::string output;
                std::wstring wideOutput;
                s_formatRemaining = position;
                s_formatRejected = 0;
                const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(FormatMemoryHook);
                try
                {
                    if (mode == 0) { CStringManager::Format(output, "%04096d", 7); }
                    else if (mode == 1) { CStringManager::Format(wideOutput, L"%04096d", 7); }
                    else if (mode == 2) { output = CStringManager::Format("%04096d", 7); }
                    else { wideOutput = CStringManager::Format(L"%04096d", 7); }
                }
                catch (const std::bad_alloc&)
                {
                    escaped = true;
                }
                _CrtSetAllocHook(old);
                valid = (output.empty() || output == expected) && (wideOutput.empty() || wideOutput == wideExpected);
                if (s_formatRejected != 0) { ++rejected; }
                else { ++accepted; }
            }
            _CrtMemState after = {};
            _CrtMemCheckpoint(&after);
            FORMAT_CHECK(valid && (!escaped || mode >= 2));
            FORMAT_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
        }
    }
    FORMAT_CHECK(rejected != 0 && accepted != 0);
    std::printf("CSTRING_FORMAT allocation_positions=32 rejected=%u accepted=%u heap_delta=0\n", rejected, accepted);
#endif
    std::printf("CString format alias/wide/variadic checks=%u PASS\n", checks);
    return true;
#undef FORMAT_CHECK
}

#ifndef CSTRING_FORMAT_NO_MAIN
/** 独立测试入口，正式库保留整段注释
@return 零表示成功
*/
int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
#ifndef _WIN32
    // ?CRT????????????locale????main????UTF-8 locale?
    if (std::setlocale(LC_CTYPE, "C.UTF-8") == nullptr)
    {
        std::puts("UTF-8 test locale unavailable");
        return 1;
    }
#endif
    return CStringFormatAudit() ? 0 : 1;
}

#endif

#undef CSTRING_FORMAT_NO_MAIN
#undef CSTRING_FORMAT_NO_HEAP

void NetWorkConcurrencyTestCases::stringFormat()
{
    if (!CStringFormatAudit())
    {
        throw std::runtime_error("CString format audit failed");
    }
    netTestCheckpoint();
}

#define SYSTEM_LEASE_NO_MAIN
#define SYSTEM_LEASE_NO_HEAP
#if defined(_WIN32) && defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
#include <crtdbg.h>
// 独立main的CRT故障注入，不与其他业务线程同时运行。
static uint32_t s_leaseRemaining = 0;
static uint32_t s_leaseRejected = 0;
/** 从指定普通堆分配位置开始持续拒绝
@param [in] operation 分配操作
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 是否允许
*/
static int __cdecl LeaseMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
    {
        if (s_leaseRemaining == 0)
        {
            ++s_leaseRejected;
            return 0;
        }
        --s_leaseRemaining;
    }
    return 1;
}

#endif

/** 使用测试专属名称验证租约取得失败回滚、就绪及释放，不操作正式网关租约
@return 所有检查通过为true；非Windows平台本main暂不作为运行验证
*/
static bool SystemLeaseAudit()
{
#ifdef _WIN32
    uint32_t checks = 0;
    uint32_t failures = 0;
    uint32_t successes = 0;
#define LEASE_CHECK(value) { ++checks; if (!(value)) { std::printf("System lease line=%d FAIL\n", __LINE__); return false; } }
#if defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
    // CRT首次处理分配异常会建立自身常驻资源；先单独触发，不把它计为租约泄漏。
    std::string warm;
    const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(LeaseMemoryHook);
    try { warm.resize(4096); } catch (...) {}
    _CrtSetAllocHook(previous);
#endif
    const std::string name = "NetLeaseAllocationAudit";
    for (uint32_t position = 0; position < 32; ++position)
    {
        const std::string isolated = std::to_string(CSystem::currentProcessPid()) + "." + std::to_string(position);
#if defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
        _CrtMemState before = {};
        _CrtMemCheckpoint(&before);
        DWORD handlesBefore = 0;
        LEASE_CHECK(GetProcessHandleCount(GetCurrentProcess(), &handlesBefore) != FALSE);
#endif
        bool escaped = false;
        bool valid = true;
        {
            std::vector<uint64_t> state;
            int32_t acquired = -99;
#if defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
            s_leaseRemaining = position;
            s_leaseRejected = 0;
            const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(LeaseMemoryHook);
#endif
            try
            {
                acquired = CSystem::acquireProcessLease(name, false, 0, state, isolated);
            }
            catch (...)
            {
                escaped = true;
            }
#if defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
            _CrtSetAllocHook(old);
#endif
            if (acquired == 1)
            {
                ++successes;
                valid = state.size() == 3 && !CSystem::processLeaseStopRequested(state) &&
                    CSystem::setProcessLeaseReady(state, true) && CSystem::isProcessLeaseReady(name, CSystem::currentProcessPid());
                // 重复使用同一个输出容器是非法输入，必须保留已取得租约，不改变ready。
                valid = valid && CSystem::acquireProcessLease(name, false, 0, state, isolated) == -1 &&
                    state.size() == 3 && CSystem::isProcessLeaseReady(name, CSystem::currentProcessPid());
            }
            else
            {
                ++failures;
                valid = acquired == -1 && state.empty() && !CSystem::isProcessLeaseReady(name, CSystem::currentProcessPid());
            }
#if defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
            s_leaseRemaining = 0;
            s_leaseRejected = 0;
            const _CRT_ALLOC_HOOK cleanupHook = _CrtSetAllocHook(LeaseMemoryHook);
#endif
            CSystem::releaseProcessLease(state);
            CSystem::releaseProcessLease(state);
#if defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
            _CrtSetAllocHook(cleanupHook);
            valid = valid && s_leaseRejected == 0;
#endif
            valid = valid && state.empty() && !CSystem::isProcessLeaseReady(name, CSystem::currentProcessPid());
        }
#if defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
        DWORD handlesAfter = 0;
        LEASE_CHECK(GetProcessHandleCount(GetCurrentProcess(), &handlesAfter) != FALSE && handlesBefore == handlesAfter);
        _CrtMemState after = {};
        _CrtMemCheckpoint(&after);
        LEASE_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
#endif
        LEASE_CHECK(!escaped && valid);
    }
#if defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
    LEASE_CHECK(failures != 0 && successes != 0);
#else
    LEASE_CHECK(failures == 0 && successes == 32);
#endif
    std::printf("System lease positions=32 failed=%u acquired=%u checks=%u PASS\n", failures, successes, checks);
    return true;
#undef LEASE_CHECK
#else
    std::puts("System lease Windows fault injection not executed on this platform");
    return false;
#endif
}

#ifndef SYSTEM_LEASE_NO_MAIN
/** 独立测试入口；正式库保留整段注释
@return 零表示成功
*/
int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    return SystemLeaseAudit() ? 0 : 1;
}

#endif

#undef SYSTEM_LEASE_NO_MAIN
#undef SYSTEM_LEASE_NO_HEAP

void NetWorkConcurrencyTestCases::processLeaseResources()
{
    if (!SystemLeaseAudit())
    {
        throw std::runtime_error("process lease resource audit failed");
    }
    netTestCheckpoint();
}