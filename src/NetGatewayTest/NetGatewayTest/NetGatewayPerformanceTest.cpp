#include "NetGatewayPerformanceTest.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "NetGatewayOneWayTest.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>
#ifdef _WIN32
#include <windows.h>

/** 取得高分辨率计时频率，初始化在启动生产者前由主线程完成
@return 每秒计数，0表示系统计时不可用
*/
static uint64_t PerformanceFrequency()
{
    LARGE_INTEGER frequency = {};
    return QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0 ?
        static_cast<uint64_t>(frequency.QuadPart) : 0;
}
#endif

/** 返回单调微秒数，仅用于同一进程内计时
@return Windows高分辨率性能计数微秒数；其他平台或计时不可用时使用steady_clock
*/
static uint64_t PerformanceNow()
{
#ifdef _WIN32
    static const uint64_t frequency = PerformanceFrequency();
    LARGE_INTEGER counter = {};
    if (frequency != 0 && QueryPerformanceCounter(&counter) && counter.QuadPart >= 0)
    {
        const uint64_t ticks = static_cast<uint64_t>(counter.QuadPart);
        return ticks / frequency * 1000000ULL + ticks % frequency * 1000000ULL / frequency;
    }
#endif
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

/** 严格解析有界无符号十进制参数
@param [in] text NUL结尾参数
@param [out] value 有效时赋值
@return 是否为不溢出的纯十进制数
*/
static bool PerformanceNumber(const char* text, uint32_t& value)
{
    value = 0;
    if (text == nullptr || *text == 0)
    {
        return false;
    }
    for (; *text; ++text)
    {
        if (*text < '0' || *text > '9' || value > (UINT32_MAX - (*text - '0')) / 10)
        {
            return false;
        }
        value = value * 10 + (*text - '0');
    }
    return true;
}

/** 可选的测试线程亲和性；不设置时保留系统调度，库本身不固定CPU
@param [in] variable 指定逻辑处理器编号的测试环境变量，未设置为成功空操作
@return 配置生效或未设置时true，平台不支持/参数错误/系统拒绝时false
*/
static bool PerformancePinThread(const char* variable)
{
    const char* requested = std::getenv(variable);
    if (requested == nullptr)
    {
        return true;
    }
#ifdef _WIN32
    uint32_t processor = 0;
    if (!PerformanceNumber(requested, processor) || processor >= sizeof(DWORD_PTR) * 8)
    {
        return false;
    }
    const DWORD_PTR mask = static_cast<DWORD_PTR>(1) << processor;
    DWORD_PTR allowed = 0;
    DWORD_PTR available = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &allowed, &available) || (mask & allowed) == 0 ||
        SetThreadAffinityMask(GetCurrentThread(), mask) == 0 || GetCurrentProcessorNumber() != processor)
    {
        return false;
    }
    std::printf("ONEWAY_CPU %s=%u thread=%lu\n", variable, processor, static_cast<unsigned long>(GetCurrentThreadId()));
    return true;
#else
    return false;
#endif
}

NetGatewayPerformanceSlot::NetGatewayPerformanceSlot() :
m_request(0),
m_started(0),
m_received(0)
{

}

NetGatewayPerformanceTest* NetGatewayPerformanceTest::s_active = nullptr;

NetGatewayPerformanceTest::NetGatewayPerformanceTest() :
m_wireSent(0),
m_wireReceived(0),
m_client(nullptr),
m_window(128),
m_generation(0),
m_completed(0),
m_failure(0),
m_stopping(false),
m_submitted(0),
m_backpressure()
{

}

void NetGatewayPerformanceTest::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    if (generation == 0 || maxRequests < m_window || maxChunk == 0 || m_generation.load() != 0)
    {
        std::fprintf(stderr, "PERFORMANCE_READY_FAILED generation=%llu requests=%u window=%u chunk=%u\n",
            static_cast<unsigned long long>(generation), maxRequests, m_window, maxChunk);
        m_failure.store(NI_PROTOCOL_ERROR);
    }
    m_generation.store(generation);
}

void NetGatewayPerformanceTest::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
    uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    if (!m_slots || generation != m_generation.load() || product != 1 || type != 0x7345 || status != NI_RPC_SUCCESS ||
        size != m_body.size() || (size != 0 && (bytes == nullptr || std::memcmp(bytes, m_body.data(), size) != 0)))
    {
        m_failure.store(NI_PROTOCOL_ERROR);
        return;
    }
    for (uint32_t index = 0; index < m_window; ++index)
    {
        NetGatewayPerformanceSlot& slot = m_slots[index];
        if (slot.m_request.load(std::memory_order_acquire) == requestId && requestId != 0)
        {
            const uint32_t result = m_completed.load(std::memory_order_relaxed);
            if (result >= m_latency.size())
            {
                m_failure.store(NI_PROTOCOL_ERROR);
                return;
            }
            m_latency[result] = PerformanceNow() - slot.m_started;
            ++slot.m_received;
            slot.m_request.store(0, std::memory_order_release);
            m_completed.store(result + 1, std::memory_order_release);
            for (uint32_t waiter = 0; waiter < 5; ++waiter)
            {
                m_progress[waiter].signal();
            }
            return;
        }
    }
    m_failure.store(NI_PROTOCOL_ERROR);
}

bool NetGatewayPerformanceTest::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    (void)generation;
    (void)frame;
    m_failure.store(NI_PROTOCOL_ERROR);
    return true;
}

void NetGatewayPerformanceTest::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)generation;
    if (!m_stopping.load() || reason != NI_OK)
    {
        std::fprintf(stderr, "PERFORMANCE_CLOSED reason=%d detail=%d\n", reason, detail);
        m_failure.store(reason == 0 ? NI_INVALID_STATE : reason);
    }
}

void NetGatewayPerformanceTest::produce(uint32_t producer, uint32_t producers, uint32_t count,
    bool synchronous, uint64_t deadline)
{
    NetWork& sender = NetWork::instance();
    uint32_t sent = 0;
    while (sent < count && m_failure.load() == 0 && PerformanceNow() < deadline)
    {
        bool advanced = false;
        for (uint32_t index = producer; index < m_window && sent < count; index += producers)
        {
            NetGatewayPerformanceSlot& slot = m_slots[index];
            if (slot.m_request.load(std::memory_order_acquire) != 0)
            {
                continue;
            }
            uint64_t request = 0;
            int32_t result = sender.beginRequest(m_client, 1, 0x7345, 10000, request);
            if (result == NI_LIMIT_EXCEEDED)
            {
                continue;
            }
            if (result != NI_OK || request == 0)
            {
                m_failure.store(result == NI_OK ? NI_PROTOCOL_ERROR : result);
                return;
            }
            slot.m_started = PerformanceNow();
            slot.m_request.store(request, std::memory_order_release);
            do
            {
                result = synchronous ? sender.sendRequest(m_client, request, m_body.data(), static_cast<uint32_t>(m_body.size())) :
                    sender.postRequest(m_client, request, m_body.data(), static_cast<uint32_t>(m_body.size()));
                if (result == NI_LIMIT_EXCEEDED)
                {
                    m_progress[producer].wait(1000);
                }
            } while (result == NI_LIMIT_EXCEEDED && m_failure.load() == 0 && PerformanceNow() < deadline);
            if (result != NI_OK)
            {
                m_failure.store(result);
                return;
            }
            ++sent;
            advanced = true;
        }
        if (!advanced)
        {
            m_progress[producer].wait(1000);
        }
    }
    if (sent != count && m_failure.load() == 0)
    {
        m_failure.store(NI_TIMEOUT);
    }
}

int32_t NetGatewayPerformanceTest::createHost(void* providerContext, void* observerContext, NetHostConnected connected,
    NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance)
{
    NetGatewayPerformanceTest* owner = static_cast<NetGatewayPerformanceTest*>(providerContext);
    if (!PerformancePinThread("NET_GATEWAY_PERF_OWNER_CPU"))
    {
        *instance = nullptr;
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::unique_ptr<NetGatewayPerformanceObserver> observer(new NetGatewayPerformanceObserver());
        observer->m_context = observerContext;
        observer->m_connected = connected;
        observer->m_data = data;
        observer->m_writable = writable;
        observer->m_closed = closed;
        observer->m_owner = owner;
        NetGatewayPerformanceObserver* borrowed = observer.get();
        owner->m_observers.push_back(std::move(observer));
        return LibuvTcp::hostCreate(nullptr, borrowed, hostConnected, hostData, hostWritable, hostClosed, instance);
    }
    catch (...)
    {
        *instance = nullptr;
        return -1;
    }
}

int32_t NetGatewayPerformanceTest::writeHost(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    const int32_t result = LibuvTcp::post(instance, peer, bytes, size);
    if (result == 0) { s_active->m_wireSent.fetch_add(size, std::memory_order_relaxed); }
    return result;
}

void NetGatewayPerformanceTest::hostConnected(void* context, uint64_t peer)
{
    NetGatewayPerformanceObserver* observer = static_cast<NetGatewayPerformanceObserver*>(context);
    observer->m_connected(observer->m_context, peer);
}

void NetGatewayPerformanceTest::hostData(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    NetGatewayPerformanceObserver* observer = static_cast<NetGatewayPerformanceObserver*>(context);
    observer->m_owner->m_wireReceived.fetch_add(size, std::memory_order_relaxed);
    observer->m_data(observer->m_context, peer, bytes, size);
}

void NetGatewayPerformanceTest::hostWritable(void* context, uint64_t peer)
{
    NetGatewayPerformanceObserver* observer = static_cast<NetGatewayPerformanceObserver*>(context);
    observer->m_writable(observer->m_context, peer);
}

void NetGatewayPerformanceTest::hostClosed(void* context, uint64_t peer, int32_t reason)
{
    NetGatewayPerformanceObserver* observer = static_cast<NetGatewayPerformanceObserver*>(context);
    observer->m_closed(observer->m_context, peer, reason);
}

bool NetGatewayPerformanceTest::batch(uint32_t count, uint32_t producers, bool synchronous, bool report)
{
    m_latency.assign(count, 0);
    m_completed.store(0);
    for (uint32_t waiter = 0; waiter < 5; ++waiter) { m_progress[waiter].reset(); }
    for (uint32_t index = 0; index < m_window; ++index)
    {
        m_slots[index].m_received = 0;
    }
    const uint64_t wireSent = m_wireSent.load();
    const uint64_t wireReceived = m_wireReceived.load();
    const uint64_t started = PerformanceNow();
    const uint64_t deadline = started + 120000000;
    std::vector<std::thread> threads;
    threads.reserve(producers);
    try
    {
        for (uint32_t index = 0; index < producers; ++index)
        {
            threads.push_back(std::thread(&NetGatewayPerformanceTest::produce, this, index, producers,
                count / producers + (index < count % producers ? 1 : 0), synchronous, deadline));
        }
    }
    catch (...)
    {
        m_failure.store(NI_NO_MEMORY);
    }
    for (size_t index = 0; index < threads.size(); ++index)
    {
        threads[index].join();
    }
    while (m_completed.load(std::memory_order_acquire) != count && m_failure.load() == 0 && PerformanceNow() < deadline)
    {
        m_progress[4].wait(1000);
    }
    const uint64_t elapsed = PerformanceNow() - started;
    if (m_failure.load() != 0 || m_completed.load() != count)
    {
        std::fprintf(stderr, "PERFORMANCE_FAILED result=%d received=%u expected=%u\n", m_failure.load(), m_completed.load(), count);
        return false;
    }
    if (report)
    {
        std::sort(m_latency.begin(), m_latency.end());
        const uint64_t bytes = static_cast<uint64_t>(count) * m_body.size();
        std::printf("PERFORMANCE_RESULT {\"bodyBytes\":%u,\"requests\":%u,\"responses\":%u,\"clientSentBodyBytes\":%llu,"
            "\"clientReceivedBodyBytes\":%llu,\"elapsedUs\":%llu,\"requestsPerSecond\":%.3f,\"p50Us\":%llu,\"p95Us\":%llu,"
            "\"p99Us\":%llu,\"producers\":%u,\"method\":\"%s\",\"producerReceived\":[",
            static_cast<uint32_t>(m_body.size()), count, count, static_cast<unsigned long long>(bytes),
            static_cast<unsigned long long>(bytes), static_cast<unsigned long long>(elapsed),
            1000000.0 * count / elapsed, static_cast<unsigned long long>(m_latency[count / 2]),
            static_cast<unsigned long long>(m_latency[(count * 95) / 100]),
            static_cast<unsigned long long>(m_latency[(count * 99) / 100]), producers, synchronous ? "send" : "post");
        for (uint32_t producer = 0; producer < producers; ++producer)
        {
            uint32_t received = 0;
            for (uint32_t index = producer; index < m_window; index += producers)
            {
                received += m_slots[index].m_received;
            }
            std::printf("%s%u", producer ? "," : "", received);
        }
        std::printf("],\"window\":%u,\"tcpSentBytes\":%llu,\"tcpReceivedBytes\":%llu}\n", m_window,
            static_cast<unsigned long long>(m_wireSent.load() - wireSent),
            static_cast<unsigned long long>(m_wireReceived.load() - wireReceived));
    }
    return true;
}

void NetGatewayPerformanceTest::produceData(uint32_t producer, uint32_t count, bool synchronous, uint64_t deadline)
{
    if (!PerformancePinThread("NET_GATEWAY_PERF_PRODUCER_CPU"))
    {
        m_failure.store(NI_INVALID_ARGUMENT);
        return;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_DATA;
    frame.m_productId = 1;
    frame.m_messageType = 0x7351;
    frame.m_payload = m_body.empty() ? nullptr : m_body.data();
    frame.m_payloadSize = static_cast<uint32_t>(m_body.size());
    NetWork& sender = NetWork::instance();
    NetWorkSenderState* client = m_senders.empty() ? m_client : m_senders[producer % m_senders.size()];
    uint32_t submitted = 0;
    for (uint32_t index = 0; index < count; ++index)
    {
        // 总计时仍覆盖每条消息；期限每1024条检查一次，不把逐条读时钟当成库成本。
        if (m_failure.load(std::memory_order_relaxed) != 0 || ((index & 1023) == 0 && PerformanceNow() >= deadline))
        {
            m_failure.store(NI_TIMEOUT);
            break;
        }
        frame.m_requestId = (static_cast<uint64_t>(producer + 1) << 48) | (index + 1);
        int32_t result;
        do
        {
            result = synchronous ? sender.sendData(client, frame) : sender.postData(client, frame);
            if (result == NI_LIMIT_EXCEEDED)
            {
                ++m_backpressure[producer];
                // 没有人工在途窗口；实际容量不足时让执行者推进，不重新创建消息。
                std::this_thread::yield();
            }
        } while (result == NI_LIMIT_EXCEEDED && m_failure.load() == 0 && PerformanceNow() < deadline);
        if (result != NI_OK)
        {
            m_failure.store(result);
            break;
        }
        ++submitted;
    }
    // 主线程join后核对总数；生产者不为测试统计逐条争用同一个缓存行。
    m_submitted.fetch_add(submitted, std::memory_order_relaxed);
}

bool NetGatewayPerformanceTest::oneWay(uint32_t count, uint32_t producers, bool synchronous, const char* stopFile)
{
#ifndef _WIN32
    // 当前跨进程计时已验证Windows QPC；不把其他平台未经确认的时钟纪元当作相同。
    (void)count;
    (void)producers;
    (void)synchronous;
    (void)stopFile;
    return false;
#else
    const std::wstring stopPath = CStringManager::Utf8ToUnicode(stopFile);
    if (stopPath.empty() || CSystem::fileExists(stopPath))
    {
        return false;
    }
    std::vector<std::thread> threads;
    threads.reserve(producers);
    const uint64_t wireSent = m_wireSent.load();
    const uint64_t wireReceived = m_wireReceived.load();
    const uint64_t started = NetGatewayOneWayTest::now();
    const uint64_t deadline = started + 120000000;
    if (started == 0)
    {
        return false;
    }
    try
    {
        for (uint32_t index = 0; index < producers; ++index)
        {
            threads.push_back(std::thread(&NetGatewayPerformanceTest::produceData, this, index,
                count / producers + (index < count % producers ? 1 : 0), synchronous, deadline));
        }
    }
    catch (...)
    {
        m_failure.store(NI_NO_MEMORY);
    }
    for (size_t index = 0; index < threads.size(); ++index)
    {
        threads[index].join();
    }
    if (m_failure.load() != 0 || m_submitted.load() != count)
    {
        std::fprintf(stderr, "ONEWAY_FAILED code=%d submitted=%u\n", m_failure.load(), m_submitted.load());
        return false;
    }
    uint64_t backpressure = 0;
    for (uint32_t index = 0; index < producers; ++index)
    {
        backpressure += m_backpressure[index];
    }
    std::printf("ONEWAY_SUBMITTED {\"submitted\":%u,\"startedUs\":%llu,\"submitFinishedUs\":%llu,\"backpressure\":%llu}\n",
        count, static_cast<unsigned long long>(started), static_cast<unsigned long long>(NetGatewayOneWayTest::now()),
        static_cast<unsigned long long>(backpressure));
    std::fflush(stdout);
    // 提交完成后仍保持连接及执行者存活。此处检查不计入服务端已捕获的完成时间。
    while (m_failure.load() == 0 && PerformanceNow() < deadline)
    {
        if (CSystem::fileExists(stopPath))
        {
            std::printf("ONEWAY_WIRE {\"tcpSentBytes\":%llu,\"tcpReceivedBytes\":%llu}\n",
                static_cast<unsigned long long>(m_wireSent.load() - wireSent),
                static_cast<unsigned long long>(m_wireReceived.load() - wireReceived));
            return true;
        }
        CSystem::sleepPrecise(10);
    }
    return false;
#endif
}

int NetGatewayPerformanceTest::run(int argc, char** argv)
{
    uint32_t port = 0;
    uint32_t size = 0;
    uint32_t count = 0;
    uint32_t producers = 0;
    const bool oneWay = argc == 7 && (std::strcmp(argv[5], "data-send") == 0 || std::strcmp(argv[5], "data-post") == 0);
    if ((!oneWay && argc != 6) || !PerformanceNumber(argv[1], port) || port == 0 || port > 65535 ||
        !PerformanceNumber(argv[2], size) || size > 1048560 || !PerformanceNumber(argv[3], count) || count < 8 || count > (oneWay ? 10000000U : 100000U) ||
        !PerformanceNumber(argv[4], producers) || (producers != 1 && producers != 4) ||
        (!oneWay && std::strcmp(argv[5], "send") != 0 && std::strcmp(argv[5], "post") != 0) || (oneWay && size > 65536))
    {
        return 2;
    }
    NetGatewayPerformanceTest test;
    NetWork& sender = NetWork::instance();
    s_active = &test;
    bool passed = false;
    uint32_t sessions = 1;
    uint32_t workers = 4;
    uint32_t queueInitial = 128;
    const char* requestedQueue = std::getenv("NET_GATEWAY_PERF_QUEUE_INITIAL");
    const char* requestedSessions = std::getenv("NET_GATEWAY_PERF_SESSIONS");
    const char* requestedWorkers = std::getenv("NET_GATEWAY_PERF_WORKERS");
    if ((requestedSessions != nullptr && (!PerformanceNumber(requestedSessions, sessions) ||
        (sessions != 1 && sessions != 4) || sessions > producers)) ||
        (requestedWorkers != nullptr && (!PerformanceNumber(requestedWorkers, workers) || workers == 0 || workers > 16))) { return 2; }
    if (requestedQueue != nullptr && (!PerformanceNumber(requestedQueue, queueInitial) || queueInitial < 128 || queueInitial > 65536))
    {
        return 2;
    }
    NetSenderSubmissionPools submissionPools;
    submissionPools.m_queue.initialCapacity = queueInitial;
    std::vector<std::unique_ptr<NetGatewayPerformanceTest>> extra;
    try
    {
        const char* window = oneWay ? nullptr : std::getenv("NET_GATEWAY_PERF_WINDOW");
        if (window != nullptr && (!PerformanceNumber(window, test.m_window) || test.m_window < producers || test.m_window > 128))
        {
            return 2;
        }
        test.m_slots.reset(new NetGatewayPerformanceSlot[test.m_window]);
        test.m_body.resize(size);
        for (uint32_t index = 0; index < size; ++index)
        {
            test.m_body[index] = size == 6 ? static_cast<uint8_t>('1' + index) : static_cast<uint8_t>(index * 31U + 17U);
        }
        NetHostFunctions functions = {};
        functions.structSize = sizeof(functions);
        functions.version = 1;
        functions.context = &test;
        functions.create = NetGatewayPerformanceTest::createHost;
        functions.listen = LibuvTcp::hostListen;
        functions.connect = LibuvTcp::hostConnect;
        functions.poll = LibuvTcp::hostPoll;
        functions.write = NetGatewayPerformanceTest::writeHost;
        functions.pauseRead = LibuvTcp::hostPauseRead;
        functions.close = LibuvTcp::hostClose;
        functions.shutdown = LibuvTcp::hostShutdown;
        functions.destroy = LibuvTcp::hostDestroy;
        functions.localPort = LibuvTcp::hostLocalPort;
        functions.peerCount = LibuvTcp::hostPeerCount;
        functions.entropy = CSystem::secureRandomBytesCallback;
        functions.execution.create = LibuvTcp::hostLoopCreate;
        functions.execution.exchange = LibuvTcp::hostLoopExchange;
        functions.execution.begin = LibuvTcp::hostLoopBegin;
        functions.execution.wait = LibuvTcp::hostLoopWait;
        functions.execution.wake = LibuvTcp::hostLoopWake;
        functions.execution.destroy = LibuvTcp::hostLoopDestroy;
        NetSenderGatewayOptions options = {};
        options.m_network.structSize = sizeof(options.m_network);
        options.m_network.version = 1;
        options.m_network.address = "127.0.0.1";
        options.m_network.port = port;
        options.m_network.platform = 1;
        options.m_network.capabilities = 7;
        options.m_network.maxRequests = test.m_window;
        options.m_network.maxChunk = 65536;
        options.m_network.handshakeTimeoutMs = 10000;
        options.m_network.tls.handshakeTimeoutMs = 10000;
        int32_t setup = NetWork::configureHost(functions);
        if (setup == NI_OK) { setup = CSystem::secureRandomBytesCallback(nullptr, options.m_network.device, sizeof(options.m_network.device)); }
        if (setup == NI_OK) { setup = sender.createClient(test.m_client); }
        if (setup == NI_OK) { setup = sender.prepareGateway(test.m_client); }
        if (setup == NI_OK && requestedQueue != nullptr && queueInitial != 128) { setup = sender.configureSubmissionPools(test.m_client, submissionPools); }
#ifdef NET_GATEWAY_WORKER_CONFIG
        if (setup == NI_OK) { setup = NetWork::configureWorkers(workers); }
        if (setup == NI_OK) { setup = sender.configureDispatch(test.m_client, std::strstr(argv[5], "send") != nullptr); }
#endif
        if (setup == NI_OK) { setup = sender.connectServiceGateway(test.m_client, 1, options, &test); }
        passed = setup == NI_OK;
        if (!passed) { std::fprintf(stderr, "PERFORMANCE_SETUP_FAILED code=%d\n", setup); }
        const uint64_t deadline = PerformanceNow() + 15000000;
        while (passed && test.m_generation.load() == 0 && test.m_failure.load() == 0 && PerformanceNow() < deadline)
        {
            CSystem::sleepPrecise(1);
        }
        passed = passed && test.m_generation.load() != 0 && test.m_failure.load() == 0;
        if (!passed) { std::fprintf(stderr, "PERFORMANCE_CONNECT_FAILED generation=%llu code=%d\n", static_cast<unsigned long long>(test.m_generation.load()), test.m_failure.load()); }
        if (passed)
        {
            test.m_senders.push_back(test.m_client);
            for (uint32_t session = 1; session < sessions && passed; ++session)
            {
                extra.push_back(std::unique_ptr<NetGatewayPerformanceTest>(new NetGatewayPerformanceTest));
                NetGatewayPerformanceTest& child = *extra.back();
                child.m_body = test.m_body;
                child.m_slots.reset(new NetGatewayPerformanceSlot[child.m_window]);
                setup = sender.createClient(child.m_client);
                if (setup == NI_OK) { setup = sender.prepareGateway(child.m_client); }
                if (setup == NI_OK && requestedQueue != nullptr && queueInitial != 128) { setup = sender.configureSubmissionPools(child.m_client, submissionPools); }
#ifdef NET_GATEWAY_WORKER_CONFIG
                if (setup == NI_OK) { setup = sender.configureDispatch(child.m_client, std::strstr(argv[5], "send") != nullptr); }
#endif
                if (setup == NI_OK) { setup = CSystem::secureRandomBytesCallback(nullptr, options.m_network.device, sizeof(options.m_network.device)); }
                if (setup == NI_OK) { setup = sender.connectServiceGateway(child.m_client, 1, options, &child); }
                while (setup == NI_OK && child.m_generation.load() == 0 && child.m_failure.load() == 0 && PerformanceNow() < deadline)
                {
                    CSystem::Sleep(0);
                }
                passed = setup == NI_OK && child.m_generation.load() != 0 && child.m_failure.load() == 0;
                if (passed) { test.m_senders.push_back(child.m_client); }
            }
        }
        if (passed)
        {
            const bool send = std::strstr(argv[5], "send") != nullptr;
            passed = test.batch(size >= 65536 ? 8 : 64, producers, send, false);
            for (size_t index = 0; index < extra.size() && passed; ++index)
            {
                passed = extra[index]->batch(size >= 65536 ? 8 : 64, 1, send, false);
            }
            if (passed) { passed = oneWay ? test.oneWay(count, producers, send, argv[6]) : test.batch(count, producers, send, true); }
        }
    }
    catch (...)
    {
        passed = false;
    }
    for (size_t index = 0; index < extra.size(); ++index)
    {
        NetGatewayPerformanceTest& child = *extra[index];
        child.m_stopping.store(true);
        if (child.m_client != nullptr)
        {
            sender.uninitGateway(child.m_client);
            while (sender.pollGateway(child.m_client) == NI_READY) { CSystem::Sleep(0); }
            passed = sender.destroyClient(child.m_client) == NI_OK && child.m_failure.load() == 0 && passed;
        }
    }
    test.m_stopping.store(true);
    if (test.m_client != nullptr)
    {
        sender.uninitGateway(test.m_client);
        while (sender.pollGateway(test.m_client) == NI_READY)
        {
            CSystem::sleepPrecise(1);
        }
        passed = sender.destroyClient(test.m_client) == NI_OK && passed;
    }
    s_active = nullptr;
    return passed && test.m_failure.load() == 0 ? 0 : 1;
}

#ifdef NET_GATEWAY_PERFORMANCE_STANDALONE
int main(int argc, char** argv)
{
    return NetGatewayPerformanceTest::run(argc, argv);
}
#endif