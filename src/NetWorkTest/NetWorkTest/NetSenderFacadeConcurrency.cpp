#include "NetSenderFacadeConcurrency.h"
#include "NetSenderFacadeConcurrency.h"
#include "NetWorkTestCases.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "CSystem/CSystemAPI.h"
#define NET_SENDER_USE_C
#include "NetWork/NetSenderAPI.h"
#include <test/certs.h>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <chrono>
#include <algorithm>

/** 仅用于记录同步发送耗时，不作为正确性阈值
@return 单调时钟微秒数
*/
static uint64_t SenderFacadeMicroseconds()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetSenderFacadeConcurrencySink::NetSenderFacadeConcurrencySink() : m_verifyOrder(false)
{
    std::memset(m_next, 0, sizeof(m_next));
}

int32_t NetSenderFacadeConcurrencySink::send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin)
{
    (void)origin;
    NetSenderFacadeConcurrencyMessage copy = {};
    copy.m_endpoint = endpoint; copy.m_frame = frame;
    if (frame.m_payloadSize != 0) { copy.m_bytes.assign(frame.m_payload, frame.m_payload + frame.m_payloadSize); }
    m_messages.push_back(std::move(copy));
    return NI_OK;
}

void NetSenderFacadeConcurrencySink::close(uint64_t endpoint)
{
    (void)endpoint;
}

bool NetSenderFacadeConcurrencySink::respond(NetPeerEndpointRouter& router)
{
    for (size_t i = 0; i < m_messages.size(); ++i)
    {
        NetSenderFacadeConcurrencyMessage& message = m_messages[i];
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

NetSenderFacadeConcurrencyContext::NetSenderFacadeConcurrencyContext() :
m_client(nullptr), m_mainThread(CSystem::SystemThreadId()), m_callbackThread(0), m_generation(0), m_ready(0), m_closed(0), m_done(0), m_results(0), m_bad(false), m_throwReady(false), m_verifyOrder(false)
{

}

void NetSenderFacadeConcurrencyContext::ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
{
    NetSenderFacadeConcurrencyContext* test = static_cast<NetSenderFacadeConcurrencyContext*>(context);
    test->m_bad = test->m_bad.load() || generation == 0 || requests != 32 || chunk != 4096 || CSystem::SystemThreadId() == test->m_mainThread ||
        netWorkSenderClientDestroy(test->m_client) != NI_INVALID_STATE || netWorkSenderClientPoll(test->m_client) != NI_INVALID_STATE ||
        netWorkSenderClientSend(test->m_client, 0, nullptr, 0) != NI_INVALID_STATE;
    test->m_callbackThread = CSystem::SystemThreadId();
    test->m_generation = generation;
    ++test->m_ready;
    if (test->m_throwReady)
    {
        throw std::runtime_error("controlled ready callback failure");
    }
}

void NetSenderFacadeConcurrencyContext::result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    NetSenderFacadeConcurrencyContext* test = static_cast<NetSenderFacadeConcurrencyContext*>(context);
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

int32_t NetSenderFacadeConcurrencyContext::frame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
{
    (void)generation; (void)frame; static_cast<NetSenderFacadeConcurrencyContext*>(context)->m_bad = true; return 1;
}

void NetSenderFacadeConcurrencyContext::closed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    (void)detail;
    NetSenderFacadeConcurrencyContext* test = static_cast<NetSenderFacadeConcurrencyContext*>(context);
    if (generation != test->m_generation.load() || reason != (test->m_throwReady ? NI_INVALID_STATE : NI_OK) || CSystem::SystemThreadId() != test->m_callbackThread.load())
    {
        std::printf("CONCURRENT_CLOSE_FAILURE reason=%d detail=%d generation=%llu\n", reason, detail, static_cast<unsigned long long>(generation));
        test->m_bad = true;
    }
    ++test->m_closed;
}

NetSenderFacadeConcurrencyProducer::NetSenderFacadeConcurrencyProducer(NetSenderFacadeConcurrencyContext* context, uint32_t producer) : m_context(context), m_producer(producer)
{

}

void NetSenderFacadeConcurrencyProducer::DoTask()
{
    for (uint32_t i = 0; i < 8; ++i)
    {
        uint64_t id = 0; uint8_t bytes[3] = {7, static_cast<uint8_t>(m_producer), static_cast<uint8_t>(i)};
        const uint32_t size = m_context->m_verifyOrder ? 3 : 1;
        if (netWorkSenderClientBegin(m_context->m_client, 101, 700, 5000, &id) != NI_OK || id == 0) { m_context->m_bad = true; break; }
        int32_t submitted = NI_LIMIT_EXCEEDED;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (submitted == NI_LIMIT_EXCEEDED && std::chrono::steady_clock::now() < deadline)
        {
            const uint64_t sendStarted = SenderFacadeMicroseconds();
            // 不同生产者混合同步交付与异步投递，等待不能持有协议提交门。
            submitted = i % 2 == 0 ? netWorkSenderClientSend(m_context->m_client, id, bytes, size) :
                netWorkSenderClientPost(m_context->m_client, id, bytes, size);
            if (submitted == NI_OK && i % 2 == 0)
            {
                const uint32_t micros = static_cast<uint32_t>(SenderFacadeMicroseconds() - sendStarted);
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
            const int32_t cancelled = netWorkSenderClientCancel(m_context->m_client, id);
            if (cancelled != NI_OK && cancelled != NI_NOT_FOUND) { m_context->m_bad = true; }
        }
        std::lock_guard<std::mutex> lock(m_context->m_mutex);
        m_context->m_accepted.push_back(id);
    }
    ++m_context->m_done;
}

NetSenderFacadeConcurrencyOwner::NetSenderFacadeConcurrencyOwner(NetSenderFacadeConcurrencyContext& context) : m_context(&context)
{
    std::memset(m_producers, 0, sizeof(m_producers));
}

NetSenderFacadeConcurrencyOwner::~NetSenderFacadeConcurrencyOwner()
{
    for (uint32_t index = 0; index < 4; ++index)
    {
        if (m_producers[index] != 0) { CTaskThreadManager::Instance().Uninit(m_producers[index]); }
    }
    if (m_context->m_client != nullptr)
    {
        netWorkSenderClientClose(m_context->m_client, 0);
        while (netWorkSenderClientPoll(m_context->m_client) != NI_OK || netWorkSenderClientDestroy(m_context->m_client) != NI_OK)
        {
            CSystem::sleepPrecise(1);
        }
        m_context->m_client = nullptr;
    }
}

void NetSenderFacadeConcurrency::workConcurrency()
{
    const bool verifyOrder = true;

    uint32_t checks = 0;
#define THREAD_CHECK(condition) { netTestCheckpoint(); ++checks; if (!(condition)) { throw std::runtime_error("NetWork concurrent check: " #condition); } }
    const int32_t baseline = CTaskThreadManager::Instance().Count();
    NetSenderFacadeConcurrencyContext test;
    test.m_verifyOrder = verifyOrder;
    NetInterfaceClientCallbacks callbacks = {};
    callbacks.structSize = sizeof(callbacks); callbacks.version = 1; callbacks.context = &test;
    callbacks.ready = test.ready; callbacks.result = test.result; callbacks.frame = test.frame; callbacks.closed = test.closed;
    NetSenderFacadeConcurrencySink sink;
    sink.m_verifyOrder = verifyOrder;
    NetPeerEndpointRouter gateway(&sink);
    NetSenderFacadeConcurrencyOwner owner(test);
    THREAD_CHECK(netWorkSenderClientCreate(&callbacks, &test.m_client) == NI_OK);
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
        THREAD_CHECK(netWorkSenderClientConnect(test.m_client, &options) == NI_OK);
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
            thread->PostTask(std::shared_ptr<CTask>(new NetSenderFacadeConcurrencyProducer(&test, i)));
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
            THREAD_CHECK(netWorkSenderClientBegin(test.m_client, 101, 700, 5000, &id) == NI_OK);
            std::lock_guard<std::mutex> lock(test.m_mutex); test.m_accepted.push_back(id);
        }
        THREAD_CHECK(netWorkSenderClientClose(test.m_client, 0) == NI_OK);
        while (netWorkSenderClientPoll(test.m_client) != NI_OK && NetSenderSubmission::now() < deadline)
        {
            gateway.poll(); CSystem::sleepPrecise(1);
        }
        THREAD_CHECK(test.m_closed.load() == round + 1 && test.m_results.load() == (round + 1) * 38 && !test.m_bad.load());
    }
    THREAD_CHECK(netWorkSenderClientDestroy(test.m_client) == NI_OK); test.m_client = nullptr;
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