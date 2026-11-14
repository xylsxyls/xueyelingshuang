#include "LibuvRawPostBenchmark.h"
#include "uv.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <algorithm>

/** 获取同一进程内各线程通用的单调时间
@return 纳秒数
*/
static uint64_t RawPostNow()
{
    return uv_hrtime();
}

LibuvRawPostBenchmark::LibuvRawPostBenchmark() :
m_host(nullptr), m_loop(nullptr), m_peer(0), m_target(nullptr), m_port(0), m_stop(false),
m_failed(false), m_received(0), m_finished(0), m_payload(6), m_expected(0), m_framed(false),
m_ordered(false), m_offset(0)
{
    std::memset(m_sequences, 0, sizeof(m_sequences));
}

LibuvRawPostBenchmark::~LibuvRawPostBenchmark()
{
    m_stop.store(true);
    stop();
    if (m_network.joinable())
    {
        m_network.join();
    }
}

void LibuvRawPostBenchmark::uvServerConnected(uv_tcp_t* target)
{
    m_target.store(target, std::memory_order_release);
}

void LibuvRawPostBenchmark::uvServerNotFind()
{
    m_failed.store(true);
}

void LibuvRawPostBenchmark::connected(void* context, uint64_t peer)
{
    static_cast<LibuvRawPostBenchmark*>(context)->m_peer.store(peer, std::memory_order_release);
}

void LibuvRawPostBenchmark::writable(void* context, uint64_t peer)
{
    (void)context;
    (void)peer;
}

void LibuvRawPostBenchmark::closed(void* context, uint64_t peer, int32_t reason)
{
    (void)peer;
    (void)reason;
    LibuvRawPostBenchmark& test = *static_cast<LibuvRawPostBenchmark*>(context);
    if (!test.m_stop.load() && test.m_received.load() < test.m_expected)
    {
        test.m_failed.store(true);
    }
}

void LibuvRawPostBenchmark::data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    LibuvRawPostBenchmark& test = *static_cast<LibuvRawPostBenchmark*>(context);
    if (peer != test.m_peer.load(std::memory_order_acquire))
    {
        test.m_failed.store(true);
        return;
    }
    uint64_t received = test.m_received.load(std::memory_order_relaxed);
    const uint32_t width = static_cast<uint32_t>(test.m_pattern.size());
    while (size != 0)
    {
        const uint8_t* frame = bytes;
        if (test.m_offset != 0 || size < width)
        {
            const uint32_t take = (std::min)(width - test.m_offset, size);
            std::memcpy(&test.m_partial[test.m_offset], bytes, take);
            test.m_offset += take;
            bytes += take;
            size -= take;
            if (test.m_offset != width)
            {
                break;
            }
            test.m_offset = 0;
            frame = test.m_partial.data();
        }
        else
        {
            bytes += width;
            size -= width;
        }
        bool valid = true;
        if (test.m_ordered)
        {
            uint32_t sequence = 0;
            std::memcpy(&sequence, frame + 1, sizeof(sequence));
            valid = frame[0] < 16 && frame[5] == 0x5a;
            if (valid)
            {
                valid = sequence == test.m_sequences[frame[0]]++;
            }
        }
        else
        {
            valid = std::memcmp(frame, test.m_pattern.data(), width) == 0;
        }
        if (!valid || ++received > test.m_expected)
        {
            test.m_failed.store(true);
            return;
        }
    }
    if (received == test.m_expected)
    {
        if (test.m_offset != 0)
        {
            test.m_failed.store(true);
        }
        test.m_finished.store(RawPostNow(), std::memory_order_release);
    }
    test.m_received.store(received, std::memory_order_release);
}

void LibuvRawPostBenchmark::network(uint32_t port)
{
    void* previous = nullptr;
    bool exchanged = false;
    if (LibuvTcp::hostLoopCreate(&m_loop) == 0)
    {
        previous = LibuvTcp::hostLoopExchange(m_loop);
        exchanged = true;
        if (LibuvTcp::hostCreate(nullptr, this, connected, data, writable, closed, &m_host) != 0)
        {
            m_failed.store(true);
        }
    }
    else
    {
        m_failed.store(true);
    }
    if (!m_failed.load())
    {
        LibuvChannelWritePoolConfig pool;
        pool.m_records.initialCapacity = 65536;
        pool.m_records.maximumCapacity = 65536;
        pool.m_submission.initialCapacity = 65536;
        pool.m_submission.maximumCapacity = 65536;
        if (LibuvTcp::hostConfigureWritePool(m_host, pool) != 0 ||
            LibuvTcp::hostConfigureReadBuffer(m_host, 65536) != 0)
        {
            m_failed.store(true);
        }
    }
    if (!m_failed.load())
    {
        uint64_t peer = 0;
        const int32_t result = port == 0 ? LibuvTcp::hostListen(m_host, "127.0.0.1", 0, 1) :
            LibuvTcp::hostConnect(m_host, "127.0.0.1", port, &peer);
        if (result != 0)
        {
            m_failed.store(true);
        }
        else if (port == 0)
        {
            m_port.store(LibuvTcp::hostLocalPort(m_host), std::memory_order_release);
        }
    }
    while (!m_stop.load())
    {
        if (m_failed.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        const uint64_t observed = LibuvTcp::hostLoopBegin(m_loop);
        if (LibuvTcp::hostLoopWait(m_loop, observed, 10) < 0)
        {
            m_failed.store(true);
        }
    }
    if (m_host != nullptr && LibuvTcp::hostDestroy(m_host) != 0)
    {
        m_failed.store(true);
    }
    m_host = nullptr;
    if (exchanged)
    {
        LibuvTcp::hostLoopExchange(previous);
    }
    if (m_loop != nullptr && LibuvTcp::hostLoopDestroy(m_loop) != 0)
    {
        m_failed.store(true);
    }
    m_loop = nullptr;
}

int LibuvRawPostBenchmark::run(int argc, char** argv)
{
    if (argc != 6)
    {
        return 2;
    }
    uint32_t values[3] = {};
    for (uint32_t index = 0; index < 3; ++index)
    {
        errno = 0;
        char* end = nullptr;
        const char* text = argv[index + 2];
        const unsigned long value = std::strtoul(text, &end, 10);
        if (errno != 0 || *text < '0' || *text > '9' || *end != 0 || value == 0 || value > 100000000)
        {
            return 2;
        }
        values[index] = static_cast<uint32_t>(value);
    }
    const uint32_t producers = values[0], payload = values[1], count = values[2];
    const bool preallocated = std::strcmp(argv[5], "framedpool") == 0;
    const bool framed = preallocated || std::strcmp(argv[5], "framed") == 0;
    const bool ordered = std::strcmp(argv[5], "ordered") == 0;
    if (producers > 16 || payload > 32768 || count % producers != 0 || (ordered && payload != 6) ||
        (!framed && !ordered && std::strcmp(argv[5], "raw") != 0))
    {
        return 2;
    }
    // 两阶段使用同一组生产者和连接；预热不计时，避免重建线程丢失生产者缓存。
    const uint32_t warm = (std::min)(100000U, 8388608U / payload) / producers;
    LibuvRawPostBenchmark receiver;
    LibuvRawPostBenchmark sender;
    if (preallocated)
    {
        // 给三参数路径同样增加预分配，避免仅与低容量默认配置比较。
        LibuvQueuePoolConfig queue;
        queue.m_initialCapacity = 65536;
        queue.m_maximumCapacity = 65536;
        LibuvSendPoolConfig send;
        send.m_laneCapacity = 16384;
        if (sender.configureSendQueue(queue) != 0 || sender.configureSendPool(send) != 0)
        {
            return 3;
        }
    }
    receiver.m_payload = payload;
    receiver.m_framed = framed;
    receiver.m_ordered = ordered;
    receiver.m_expected = static_cast<uint64_t>(warm) * producers + count;
    receiver.m_pattern.resize(payload + (framed ? 4 : 0));
    if (framed)
    {
        std::memcpy(receiver.m_pattern.data(), &payload, 4);
    }
    for (uint32_t index = 0; index < payload; ++index)
    {
        receiver.m_pattern[index + (framed ? 4 : 0)] = static_cast<uint8_t>('1' + index % 6);
    }
    receiver.m_partial.resize(receiver.m_pattern.size());
    receiver.m_network = std::thread([&receiver]() { receiver.network(0); });
    const uint64_t deadline = RawPostNow() + 120000000000ULL;
    while (receiver.m_port.load(std::memory_order_acquire) == 0 && !receiver.m_failed.load() && RawPostNow() < deadline)
    {
        std::this_thread::yield();
    }
    if (receiver.m_port.load() == 0 || receiver.m_failed.load())
    {
        return 3;
    }
    const uint32_t port = receiver.m_port.load();
    sender.m_network = std::thread([&sender, port, framed]()
    {
        if (framed)
        {
            if (!sender.initClient("127.0.0.1", port))
            {
                sender.m_failed.store(true);
                return;
            }
            sender.loop();
        }
        else
        {
            sender.network(port);
        }
    });
    while ((framed ? sender.m_target.load(std::memory_order_acquire) == nullptr : sender.m_peer.load(std::memory_order_acquire) == 0) &&
        !sender.m_failed.load() && RawPostNow() < deadline)
    {
        std::this_thread::yield();
    }
    if (sender.m_failed.load() || RawPostNow() >= deadline)
    {
        return 3;
    }
    std::atomic<uint32_t> phase(0), ready(0);
    std::atomic<bool> failed(false);
    uint64_t retries[16] = {}, submissions[16] = {};
    std::vector<std::thread> threads;
    try
    {
        threads.reserve(producers);
        for (uint32_t producer = 0; producer < producers; ++producer)
        {
            threads.push_back(std::thread([&, producer]()
            {
                try
                {
                    std::vector<uint8_t> bytes(payload);
                    for (uint32_t index = 0; index < payload; ++index)
                    {
                        bytes[index] = static_cast<uint8_t>('1' + index % 6);
                    }
                    uint32_t sequence = 0;
                    for (uint32_t stage = 0; stage < 2 && !failed.load(); ++stage)
                    {
                        const uint32_t limit = stage == 0 ? warm : count / producers;
                        if (stage == 1)
                        {
                            ready.fetch_add(1);
                            while (phase.load(std::memory_order_acquire) == 0 && !failed.load())
                            {
                                std::this_thread::yield();
                            }
                        }
                        for (uint32_t index = 0; index < limit && !failed.load(); ++index, ++sequence)
                        {
                            if (ordered)
                            {
                                bytes[0] = static_cast<uint8_t>(producer);
                                std::memcpy(bytes.data() + 1, &sequence, 4);
                                bytes[5] = 0x5a;
                            }
                            uint32_t spins = 0;
                            for (;;)
                            {
                                const int32_t result = framed ?
                                    (sender.post(sender.m_target.load(), reinterpret_cast<const char*>(bytes.data()), payload) ? 0 : -1) :
                                    LibuvTcp::post(sender.m_host, sender.m_peer.load(), bytes.data(), payload);
                                if (result == 0)
                                {
                                    break;
                                }
                                if (result != 1 || failed.load() || RawPostNow() >= deadline)
                                {
                                    failed.store(true);
                                    break;
                                }
                                if (stage == 1)
                                {
                                    ++retries[producer];
                                }
                                if (++spins > 4096)
                                {
                                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                }
                            }
                            if ((index & 4095) == 0 && RawPostNow() >= deadline)
                            {
                                failed.store(true);
                            }
                        }
                    }
                    submissions[producer] = RawPostNow();
                }
                catch (...)
                {
                    failed.store(true);
                }
            }));
        }
    }
    catch (...)
    {
        failed.store(true);
    }
    while (!failed.load() && (ready.load() != producers || receiver.m_received.load() != static_cast<uint64_t>(warm) * producers))
    {
        if (receiver.m_failed.load() || sender.m_failed.load() || RawPostNow() >= deadline)
        {
            failed.store(true);
        }
        std::this_thread::yield();
    }
    const uint64_t started = RawPostNow();
    phase.store(1, std::memory_order_release);
    while (!failed.load() && receiver.m_finished.load(std::memory_order_acquire) == 0)
    {
        if (receiver.m_failed.load() || sender.m_failed.load() || RawPostNow() >= deadline)
        {
            failed.store(true);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (failed.load())
    {
        sender.stop();
    }
    for (size_t index = 0; index < threads.size(); ++index)
    {
        threads[index].join();
    }
    receiver.m_stop.store(true);
    sender.m_stop.store(true);
    sender.stop();
    sender.m_network.join();
    receiver.m_network.join();
    const uint64_t finished = receiver.m_finished.load();
    bool passed = !failed.load() && !sender.m_failed.load() && !receiver.m_failed.load() &&
        receiver.m_received.load() == receiver.m_expected && finished > started;
    if (ordered)
    {
        for (uint32_t producer = 0; producer < producers; ++producer)
        {
            passed = passed && receiver.m_sequences[producer] == warm + count / producers;
        }
    }
    uint64_t backpressure = 0, lastSubmit = started;
    for (uint32_t producer = 0; producer < producers; ++producer)
    {
        backpressure += retries[producer];
        lastSubmit = (std::max)(lastSubmit, submissions[producer]);
    }
    const double seconds = finished > started ? (finished - started) / 1e9 : 0;
    std::printf("HOST_POST mode=%s producers=%u payload=%u count=%u warmup=%u received=%llu bytes=%llu seconds=%.9f mps=%.3f submitSeconds=%.9f retries=%llu status=%s\n",
        argv[5], producers, payload, count, warm * producers,
        static_cast<unsigned long long>(receiver.m_received.load()),
        static_cast<unsigned long long>(count) * (payload + (framed ? 4 : 0)), seconds,
        seconds > 0 ? count / seconds : 0, (lastSubmit - started) / 1e9,
        static_cast<unsigned long long>(backpressure), passed ? "PASS" : "FAIL");
    return passed ? 0 : 4;
}

#ifdef LIBUV_RAW_POST_STANDALONE
/** 与正式测试入口共用同一基准，不编译白盒故障注入
@param [in] argc 参数数量
@param [in] argv 基准参数
@return 0通过，非零失败
*/
int main(int argc, char** argv)
{
    try
    {
        return LibuvRawPostBenchmark::run(argc, argv);
    }
    catch (...)
    {
        return 5;
    }
}
#endif