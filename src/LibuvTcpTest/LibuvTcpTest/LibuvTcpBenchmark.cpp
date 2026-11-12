#include "LibuvTcpBenchmark.h"
#include "LibuvTcpTestModels.h"
#include "uv.h"
#include "LibuvTcp/LibuvBase.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#ifdef _WIN32
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

int LibuvTcpBenchmark::profile()
{
    const uint32_t count = 2000000;
    LibuvTcp allocator;
    LibuvLifecycleGate gate;
    gate.open();
    LibuvTestClient owner;
    if (!owner.initServer("127.0.0.1", 0))
    {
        return 1;
    }
    LibuvBase& state = *owner.m_libuv;
    if (!state.registerTcp(state.mainTcp()))
    {
        return 1;
    }
    uint64_t started = uv_hrtime();
    for (uint32_t index = 0; index < count; ++index)
    {
        char* data = allocator.memoryForSend(18);
        data[0] = 'x';
        allocator.releaseSendMemory(data);
    }
    std::printf("PROFILE,allocate_free,%.3f\n", double(uv_hrtime() - started) / count);
    started = uv_hrtime();
    for (uint32_t index = 0; index < count; ++index)
    {
        LibuvCall call(gate);
        if (!call.accepted())
        {
            return 2;
        }
    }
    std::printf("PROFILE,lifecycle,%.3f\n", double(uv_hrtime() - started) / count);
    started = uv_hrtime();
    for (uint32_t index = 0; index < count; ++index)
    {
        uint64_t generation = 0;
        if (!state.connectionGeneration(state.mainTcp(), generation) || generation != 1)
        {
            return 3;
        }
    }
    std::printf("PROFILE,connection,%.3f\n", double(uv_hrtime() - started) / count);
    started = uv_hrtime();
    for (uint32_t index = 0; index < count; ++index)
    {
        if (state.tryReserveSend(200, true) != 0)
        {
            return 4;
        }
        state.releaseSend(200);
    }
    std::printf("PROFILE,budget,%.3f\n", double(uv_hrtime() - started) / count);
    LibuvPendingSend pending;
    LibuvPendingSend output[64];
    started = uv_hrtime();
    for (uint32_t index = 0; index < count; index += 64)
    {
        for (size_t item = 0; item < 64; ++item)
        {
            if (state.m_queue->tryPush(pending, true, false) != LockFreeQueueSuccess)
            {
                return 5;
            }
        }
        size_t consumed = 0;
        while (consumed != 64)
        {
            size_t taken = 0;
            if (!state.m_queue->popReadyPrefix(output, 64 - consumed, taken) || taken == 0)
            {
                return 6;
            }
            consumed += taken;
        }
    }
    std::printf("PROFILE,queue_pair,%.3f\n", double(uv_hrtime() - started) / count);
    started = uv_hrtime();
    for (uint32_t index = 0; index < count; ++index)
    {
        if (LibuvBase::Wake(state.m_asyncHandle) != 0)
        {
            return 7;
        }
    }
    std::printf("PROFILE,wake,%.3f\n", double(uv_hrtime() - started) / count);
    gate.close();
    return 0;
}

/** 可选固定基准执行角色的CPU，减少调度迁移；只用于性能进程且前后版采用相同设置
@param [in] role 0发送循环，1接收循环，2及以后生产者
@return 原线程亲和掩码，未指定或非Windows时为0
*/
static uintptr_t PinBenchmarkRole(uint32_t role)
{
#if defined(_WIN32) && defined(LIBUV_TEST_BENCHMARK)
    if (std::getenv("LIBUV_TEST_FIXED_CPUS") != nullptr)
    {
        const uint32_t cpus[] = {14, 12, 6, 8, 10, 2};
        if (role < sizeof(cpus) / sizeof(cpus[0]))
        {
            return static_cast<uintptr_t>(SetThreadAffinityMask(GetCurrentThread(), DWORD_PTR(1) << cpus[role]));
        }
    }
#endif
    (void)role;
    return 0;
}

/** 恢复性能任务进入前的线程亲和设置，不让线程复用继承本场景配置
@param [in] previous PinBenchmarkRole返回的有效掩码或0
*/
static void RestoreBenchmarkRole(uintptr_t previous)
{
#ifdef _WIN32
    if (previous != 0)
    {
        SetThreadAffinityMask(GetCurrentThread(), previous);
    }
#else
    (void)previous;
#endif
}

/** 读取本测试进程累计 CPU 时间，不把等待时间计入执行时间
@return 进程 CPU 毫秒；系统查询失败时为负
*/
static double ProcessCpuMs()
{
#ifdef _WIN32
    FILETIME created, exited, kernel, user;
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
    {
        return -1;
    }
    const uint64_t ticks = (uint64_t(kernel.dwHighDateTime) << 32) + kernel.dwLowDateTime +
        (uint64_t(user.dwHighDateTime) << 32) + user.dwLowDateTime;
    return double(ticks) / 10000.0;
#else
    rusage usage = {};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
        return -1;
    }
    return (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000.0 +
        (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000.0;
#endif
}

/** 输出测试进程峰值驻留量；它包括收发双方和测试校验器，不等于库独占内存
@return 峰值字节数，无法取得时为零
*/
static uint64_t PeakResidentBytes()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS counters = {};
    counters.cb = sizeof(counters);
    return GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)) ?
        static_cast<uint64_t>(counters.PeakWorkingSetSize) : 0;
#else
    rusage usage = {};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
        return 0;
    }
#if defined(__APPLE__)
    return static_cast<uint64_t>(usage.ru_maxrss);
#else
    return static_cast<uint64_t>(usage.ru_maxrss) * 1024;
#endif
#endif
}

void LibuvTcpBenchmarkProbe::tick(uv_timer_t* timer)
{
    LibuvTcpBenchmarkProbe* probe = static_cast<LibuvTcpBenchmarkProbe*>(timer->data);
    LibuvBase* state = probe->m_client->m_libuv;
    if (state->m_stopping.load())
    {
        std::fprintf(stderr, "probe STOP queue=%llu ready=%d uvWriteBytes=%llu\n",
            static_cast<unsigned long long>(state->m_queue->size()), int(state->m_queue->possiblyReady()),
            static_cast<unsigned long long>(reinterpret_cast<uv_stream_t*>(state->mainTcp())->write_queue_size));
        uv_close(reinterpret_cast<uv_handle_t*>(timer), nullptr);
        return;
    }
    if (!probe->m_notified && uv_hrtime() - probe->m_started > 2000000000ULL)
    {
        probe->m_notified = true;
        std::fprintf(stderr, "probe queue=%llu ready=%d uvWriteBytes=%llu stopping=%d\n",
            static_cast<unsigned long long>(state->m_queue->size()), int(state->m_queue->possiblyReady()),
            static_cast<unsigned long long>(reinterpret_cast<uv_stream_t*>(state->mainTcp())->write_queue_size),
            int(state->m_stopping.load()));
        uv_async_send(state->m_asyncHandle);
    }
}

int LibuvTcpBenchmark::run(uint32_t producers, uint32_t payload, uint32_t count, uint32_t window, bool probe)
{
    if (producers < 1 || producers > 16 || payload < 16 || payload > 65536 ||
        count < 1 || count > 1000000 || window < 1 || window > 128)
    {
        return 2;
    }
    LibuvTestReceiver receiver(payload, producers, true);
    LibuvTestClient client;
    std::atomic<bool> start(false);
    std::atomic<bool> cancel(false);
    std::atomic<uint32_t> ready(0);
    std::atomic<bool> loopDone(false);
    LibuvTcpBenchmarkProbe observation = {};
    LibuvTestThread server;
    LibuvTestThread loop;
    LibuvTestThread senders[16];
    if (receiver.m_channel.listen("127.0.0.1", 0, &receiver, 1) != 0 ||
        !client.initClient("127.0.0.1", receiver.m_channel.localPort()))
    {
        return 3;
    }
    if (probe)
    {
        observation.m_client = &client;
        observation.m_started = uv_hrtime();
        if (uv_timer_init(client.loopPtr(), &observation.m_timer) != 0)
        {
            return 3;
        }
        observation.m_timer.data = &observation;
        if (uv_timer_start(&observation.m_timer, &LibuvTcpBenchmarkProbe::tick, 10, 10) != 0)
        {
            uv_close(reinterpret_cast<uv_handle_t*>(&observation.m_timer), nullptr);
            return 3;
        }
    }
    const uint64_t deadline = uv_hrtime() + 60000000000ULL;
    if (!server.start([&]()
    {
        while (!receiver.m_exit.load() && uv_hrtime() < deadline)
        {
            receiver.m_channel.poll();
            uv_sleep(0);
        }
        receiver.m_channel.shutdown();
    }, [&]() { receiver.m_exit.store(true); }) ||
        !loop.start([&]() { client.loop(); loopDone.store(true); }, [&]() { client.stop(); }))
    {
        return 3;
    }
    while (client.m_target.load() == nullptr && !client.m_failed.load() && uv_hrtime() < deadline)
    {
        uv_sleep(0);
    }
    bool launched = client.m_target.load() != nullptr;
    for (uint32_t producer = 0; launched && producer < producers; ++producer)
    {
        launched = senders[producer].start([&, producer]()
        {
            ready.fetch_add(1);
            while (!start.load() && !cancel.load() && uv_hrtime() < deadline)
            {
                uv_sleep(0);
            }
            for (uint32_t sequence = 0; sequence < count && !cancel.load(); ++sequence)
            {
                while (sequence >= receiver.m_counts[producer].load() + window &&
                    !cancel.load() && !receiver.m_failed.load() && uv_hrtime() < deadline)
                {
                    uv_sleep(0);
                }
                if (cancel.load() || receiver.m_failed.load() || uv_hrtime() >= deadline)
                {
                    break;
                }
                uv_tcp_t* target = client.m_target.load();
                char* buffer = client.memoryForSend(static_cast<int32_t>(sizeof(target) + 4 + payload));
                std::memcpy(buffer, &target, sizeof(target));
                std::memcpy(buffer + sizeof(target), &payload, 4);
                char* body = buffer + sizeof(target) + 4;
                std::memcpy(body, &producer, 4);
                std::memcpy(body + 4, &sequence, 4);
                for (uint32_t index = 16; index < payload; ++index)
                {
                    body[index] = static_cast<char>(sequence + producer + index);
                }
                const uint64_t sent = uv_hrtime();
                std::memcpy(body + 8, &sent, 8);
                client.send(buffer);
            }
        }, [&]() { cancel.store(true); });
    }
    while (launched && ready.load() < producers && uv_hrtime() < deadline)
    {
        uv_sleep(0);
    }
    const double cpuStart = ProcessCpuMs();
    const uint64_t began = uv_hrtime();
    start.store(true);
    bool complete = false;
    while (launched && !receiver.m_failed.load() && !client.m_failed.load() && !loopDone.load() && uv_hrtime() < deadline)
    {
        complete = true;
        for (uint32_t producer = 0; producer < producers; ++producer)
        {
            complete = receiver.m_counts[producer].load() == count && complete;
        }
        if (complete)
        {
            break;
        }
        uv_sleep(0);
    }
    const double cpuEnd = ProcessCpuMs();
    const uint64_t ended = receiver.m_lastReceive.load();
    if (!complete)
    {
        std::fprintf(stderr, "incomplete receiverFailed=%d clientFailed=%d target=%p elapsedMs=%.3f counts:",
            int(receiver.m_failed.load()), int(client.m_failed.load()), client.m_target.load(),
            double(uv_hrtime() - began) / 1000000.0);
        for (uint32_t producer = 0; producer < producers; ++producer)
        {
            std::fprintf(stderr, " %u", receiver.m_counts[producer].load());
        }
        std::fputc('\n', stderr);
    }
    cancel.store(true);
    for (uint32_t producer = 0; producer < producers; ++producer)
    {
        complete = senders[producer].join() && complete;
    }
    client.stop();
    complete = loop.join() && complete;
    receiver.m_exit.store(true);
    complete = server.join() && complete;
    if (!complete || receiver.m_failed.load() || client.m_failed.load() ||
        ended <= began || receiver.m_latencies.empty() || cpuStart < 0 || cpuEnd < cpuStart)
    {
        std::fputs("benchmark did not deliver the requested verified frames\n", stderr);
        return 4;
    }
    std::sort(receiver.m_latencies.begin(), receiver.m_latencies.end());
    const size_t samples = receiver.m_latencies.size();
    const double ms = double(ended - began) / 1000000.0;
    const double messages = double(producers) * count;
    std::printf("BENCH,%u,%u,%u,%u,%.6f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.0f\n",
        producers, payload, count, window, ms, messages * 1000.0 / ms,
        messages * payload / 1048576.0 * 1000.0 / ms,
        receiver.m_latencies[(samples - 1) * 50 / 100] / 1000.0,
        receiver.m_latencies[(samples - 1) * 95 / 100] / 1000.0,
        receiver.m_latencies[(samples - 1) * 99 / 100] / 1000.0,
        cpuEnd - cpuStart, double(PeakResidentBytes()));
    return observation.m_notified ? 5 : 0;
}

LibuvTinyReceiver::LibuvTinyReceiver(uint64_t expected, bool ordered, uint32_t producers) :
m_received(0),
m_finished(0),
m_failed(false),
m_expected(expected),
m_ordered(ordered),
m_producers(producers),
m_partialSize(0)
{
    std::memset(m_sequences, 0, sizeof(m_sequences));
}

bool LibuvTinyReceiver::verify(const char* frame)
{
    uint32_t size = 0;
    std::memcpy(&size, frame, 4);
    if (size != 6)
    {
        return false;
    }
    if (!m_ordered)
    {
        return std::memcmp(frame + 4, "123456", 6) == 0;
    }
    const uint32_t producer = static_cast<unsigned char>(frame[4]);
    uint32_t sequence = 0;
    std::memcpy(&sequence, frame + 5, 4);
    if (producer >= m_producers || sequence != m_sequences[producer] ||
        static_cast<unsigned char>(frame[9]) != ((sequence + producer) & 255U))
    {
        return false;
    }
    ++m_sequences[producer];
    return true;
}

void LibuvTinyReceiver::receive(uv_tcp_t* sender, const char* buffer, int32_t length)
{
    (void)sender;
    uint32_t offset = 0;
    uint64_t count = m_received.load(std::memory_order_relaxed);
    while (offset < static_cast<uint32_t>(length))
    {
        const char* frame = buffer + offset;
        if (m_partialSize != 0 || static_cast<uint32_t>(length) - offset < 10)
        {
            const uint32_t take = (std::min)(10U - m_partialSize, static_cast<uint32_t>(length) - offset);
            std::memcpy(m_partial + m_partialSize, frame, take);
            m_partialSize += take;
            offset += take;
            if (m_partialSize != 10)
            {
                break;
            }
            frame = m_partial;
            m_partialSize = 0;
        }
        else
        {
            offset += 10;
        }
        if (!verify(frame) || ++count > m_expected)
        {
            m_failed.store(true);
            return;
        }
    }
    if (count == m_expected && m_partialSize != 0)
    {
        m_failed.store(true);
    }
    if (count == m_expected && m_finished.load(std::memory_order_relaxed) == 0)
    {
        m_finished.store(uv_hrtime(), std::memory_order_release);
    }
    m_received.store(count, std::memory_order_release);
}

void LibuvTinyReceiver::uvClientConnected(uv_tcp_t* client)
{
    (void)client;
}

void LibuvTinyReceiver::uvClientDisconnected(uv_tcp_t* client)
{
    (void)client;
}

int LibuvTcpBenchmark::tiny(uint32_t producers, uint32_t ordered, uint32_t count, uint32_t batch, bool congested, bool copy, bool post)
{
    if (producers < 1 || producers > 16 || ordered > 2 || count < 1 || count > 200000000 || batch < 1 || batch > 4096 ||
        (post && batch != 1))
    {
        return 2;
    }
    LibuvTinyReceiver receiver(uint64_t(producers) * count, ordered != 0, producers);
    LibuvTestClient client;
    client.m_sendBufferSize = congested ? 1024 : 0;
    std::atomic<bool> start(false);
    std::atomic<bool> cancel(false);
    std::atomic<uint32_t> ready(0);
    std::atomic<bool> readReady(!congested);
    // 线程对象晚于其借用状态构造，任何提前返回都先回收线程再销毁状态。
    LibuvTestThread receiverThread;
    LibuvTestThread clientThread;
    LibuvTestThread senders[16];
#ifdef LIBUV_TEST_BEFORE_BATCH
    if (!receiver.initServer(0))
#else
    if (!receiver.initServer("127.0.0.1", 0))
#endif
    {
        return 3;
    }
    sockaddr_in address = {};
    int addressSize = sizeof(address);
    if (uv_tcp_getsockname(receiver.m_libuv->mainTcp(), reinterpret_cast<sockaddr*>(&address), &addressSize) != 0 ||
        !client.initClient("127.0.0.1", ntohs(address.sin_port)))
    {
        return 3;
    }
    if (!receiverThread.start([&]()
    {
        const uintptr_t affinity = PinBenchmarkRole(1);
        while (!readReady.load() && !cancel.load())
        {
            uv_sleep(1);
        }
        receiver.loop();
        RestoreBenchmarkRole(affinity);
    }, [&]() { readReady.store(true); receiver.stop(); }) ||
        !clientThread.start([&]()
        {
            const uintptr_t affinity = PinBenchmarkRole(0);
            client.loop();
            RestoreBenchmarkRole(affinity);
        }, [&]() { client.stop(); }))
    {
        return 3;
    }
    const uint64_t deadline = uv_hrtime() + 60000000000ULL;
    while (client.m_target.load() == nullptr && !client.m_failed.load() && uv_hrtime() < deadline)
    {
        uv_sleep(1);
    }
    bool valid = client.m_target.load() != nullptr;
    for (uint32_t producer = 0; valid && producer < producers; ++producer)
    {
        valid = senders[producer].start([&, producer]()
        {
            const uintptr_t affinity = PinBenchmarkRole(producer + 2);
#ifndef LIBUV_TEST_BEFORE_BATCH
            std::vector<LibuvSendBuffer> descriptions(batch);
            std::vector<char> bodies(batch * 6);
            for (uint32_t index = 0; index < batch; ++index)
            {
                descriptions[index].m_data = ordered ? &bodies[index * 6] : "123456";
                descriptions[index].m_size = 6;
            }
#endif
            ready.fetch_add(1);
            while (!start.load() && !cancel.load())
            {
                uv_sleep(0);
            }
            uv_tcp_t* target = client.m_target.load();
            const uint32_t payload = 6;
            for (uint32_t sequence = 0; sequence < count && !cancel.load();)
            {
#ifndef LIBUV_TEST_BEFORE_BATCH
                if (batch != 1 && (ordered != 2 || ((sequence / batch) & 1U) == 0))
                {
                    const uint32_t take = (std::min)(batch, count - sequence);
                    if (ordered)
                    {
                        for (uint32_t index = 0; index < take; ++index)
                        {
                            char* body = &bodies[index * 6];
                            const uint32_t number = sequence + index;
                            body[0] = static_cast<char>(producer);
                            std::memcpy(body + 1, &number, 4);
                            body[5] = static_cast<char>(number + producer);
                        }
                    }
                    if (!client.sendBatch(target, &descriptions[0], take))
                    {
                        cancel.store(true);
                    }
                    sequence += take;
                    continue;
                }
#else
                if (batch != 1)
                {
                    cancel.store(true);
                    break;
                }
#endif
                char localBody[6];
                char* buffer = nullptr;
                char* body = localBody;
                const bool copyOne = copy && (ordered != 2 || (sequence & 1U) == 0);
                if (!copyOne)
                {
                    buffer = client.memoryForSend(static_cast<int32_t>(sizeof(target) + 10));
                    std::memcpy(buffer, &target, sizeof(target));
                    std::memcpy(buffer + sizeof(target), &payload, 4);
                    body = buffer + sizeof(target) + 4;
                }
                if (ordered)
                {
                    body[0] = static_cast<char>(producer);
                    std::memcpy(body + 1, &sequence, 4);
                    body[5] = static_cast<char>(sequence + producer);
                }
                else
                {
                    std::memcpy(body, "123456", 6);
                }
                bool sent = false;
#ifndef LIBUV_TEST_BEFORE_COPY
                if (copyOne)
                {
                    sent = post ? client.post(target, body, payload) : client.send(target, body, payload);
                }
                else
#endif
                {
                    sent = post ? client.post(buffer) : client.send(buffer);
                }
                if (!sent)
                {
                    cancel.store(true);
                }
                ++sequence;
            }
            RestoreBenchmarkRole(affinity);
        }, [&]() { cancel.store(true); client.stop(); });
    }
    while (valid && ready.load() != producers && uv_hrtime() < deadline)
    {
        uv_sleep(0);
    }
    const double cpuStart = ProcessCpuMs();
    const uint64_t began = uv_hrtime();
    start.store(true);
    if (congested)
    {
        uv_sleep(50);
        readReady.store(true);
    }
    const uint64_t expected = uint64_t(producers) * count;
    while (valid && !cancel.load() && !receiver.m_failed.load() && !client.m_failed.load() &&
        receiver.m_received.load() != expected && uv_hrtime() < deadline)
    {
        uv_sleep(1);
    }
    valid = valid && !cancel.load() && receiver.m_received.load() == expected;
    cancel.store(true);
    // 失败时先让可能正在等待容量的send退出，再回收发送者。
    if (!valid)
    {
        client.stop();
    }
    for (uint32_t index = 0; index < producers; ++index)
    {
        valid = senders[index].join() && valid;
    }
    const uint64_t ended = receiver.m_finished.load();
    const double cpuEnd = ProcessCpuMs();
    client.stop();
    valid = clientThread.join() && valid;
    receiver.stop();
    valid = receiverThread.join() && valid;
    valid = valid && !receiver.m_failed.load() && !client.m_failed.load() && ended > began;
    const double ms = valid ? double(ended - began) / 1000000.0 : 0;
    std::printf("%s,%u,%u,%u,%u,%.6f,%.3f,%.3f,%.0f,%llu,%d\n",
        post ? (copy ? "TINYCOPYPOST" : "TINYPOST") : (copy ? "TINYCOPY" : "TINY"), producers, ordered, count, batch,
        ms, ms > 0 ? expected * 1000.0 / ms : 0, cpuEnd - cpuStart, double(PeakResidentBytes()),
        static_cast<unsigned long long>(receiver.m_received.load()), int(valid));
    return valid ? 0 : 4;
}