#include "ProcessWorkFairnessTests.h"
#include <cstring>
#include <iostream>

ProcessWorkFairnessReceiver::ProcessWorkFairnessReceiver(bool mixed) :
m_errors(0)
{
    for (int32_t stream = 0; stream < 32; ++stream)
    {
        m_bodies[stream] = mixed && (stream & 1) != 0 ? packetBody(stream, 65536) : "123456";
        m_received[stream].store(0, std::memory_order_relaxed);
        m_next[stream] = 0;
    }
}

void ProcessWorkFairnessReceiver::receive(int32_t pid, const char* data, int32_t length, MessageType type)
{
    const uint32_t value = static_cast<uint32_t>(type);
    const uint32_t stream = value >> 26;
    const int32_t sequence = static_cast<int32_t>(value & 0x3ffffffU);
    if (pid != bbPid() || stream >= 32 || data == nullptr || length != static_cast<int32_t>(m_bodies[stream].size()) ||
        sequence != m_next[stream] || std::memcmp(data, m_bodies[stream].data(), static_cast<size_t>(length)) != 0)
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    ++m_next[stream];
    m_received[stream].store(m_next[stream], std::memory_order_release);
}

void testProcessWorkFairProgress(bool mixed)
{
    ProcessWorkFairnessReceiver ledger(mixed);
    IpcScope scope;
    // 极小共享容量持续制造实际竞争；大包测试的本地预算也限制为约四个大包。
    ProcessWorkMemoryConfig memory;
    memory.m_localQueueBytes = mixed ? 4 * (65536 + 277) : 4 * 283;
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(ipc.setReceiveName(receiveName(mixed ? "fair-mixed" : "fair-single-slot")), "fairness receive name failed");
    ipc.addProcessReceiveCallback(&ledger);
    bbCheck(ipc.initReceive(mixed ? 65548 : 32, mixed ? 8 : 1, 0, 0, 0, 0, memory), "fairness receive initialization failed");
    std::atomic<bool> stop(false);
    std::atomic<int32_t> ready(0);
    std::atomic<int32_t> active[32];
    std::atomic<int32_t> entered[32];
    std::atomic<int32_t> sent[32];
    for (int32_t stream = 0; stream < 32; ++stream)
    {
        active[stream].store(0);
        entered[stream].store(0);
        sent[stream].store(0);
    }
    IpcJobs jobs;
    try
    {
        for (int32_t stream = 0; stream < 32; ++stream)
        {
            // 每个任务占有独立受管线程；每次send返回后才开始该流下一条消息。
            jobs.add([&ledger, &stop, &ready, &active, &entered, &sent, stream]()
            {
                ready.fetch_add(1);
                int32_t sequence = 0;
                while (!stop.load(std::memory_order_acquire))
                {
                    const int32_t phase = active[stream].load(std::memory_order_acquire);
                    if (phase == 0)
                    {
                        bbSleep(1);
                        continue;
                    }
                    entered[stream].store(phase, std::memory_order_release);
                    bbCheck(sequence < 0x4000000, "fairness sequence range exhausted");
                    const MessageType type = static_cast<MessageType>((static_cast<uint32_t>(stream) << 26) | sequence);
                    const std::string& body = ledger.m_bodies[stream];
                    const int32_t result = ProcessWork::instance().sendResult(bbPid(), body.data(), static_cast<int32_t>(body.size()), type);
                    bbCheck(result == PROCESS_WORK_SENT, "ordinary fairness send returned Busy/capacity or failed");
                    sent[stream].store(++sequence, std::memory_order_release);
                }
            });
        }
        bbWait([&ready]() { return ready.load() == 32; }, "all fairness producers created", 10000);
        for (int32_t window = 0; window < 6; ++window)
        {
            for (int32_t stream = 0; stream < 32; ++stream)
            {
                const bool enabled = !mixed || !((window < 2 && stream >= 16) || (window == 3 && stream % 4 == 0));
                active[stream].store(enabled ? window + 1 : 0, std::memory_order_release);
            }
            // 先确认活跃任务实际获得调度，再开始观察；未运行的线程不能算成库饥饿。
            bbWait([&active, &entered, window]()
            {
                for (int32_t stream = 0; stream < 32; ++stream)
                {
                    if (active[stream].load() != 0 && entered[stream].load() != window + 1)
                    {
                        return false;
                    }
                }
                return true;
            }, "active senders entered current fairness window", 5000);
            int32_t beforeSent[32] = {};
            int32_t beforeReceived[32] = {};
            for (int32_t stream = 0; stream < 32; ++stream)
            {
                beforeSent[stream] = sent[stream].load(std::memory_order_acquire);
                beforeReceived[stream] = ledger.m_received[stream].load(std::memory_order_acquire);
            }
            bbSleep(1000);
            bbCheck(ledger.m_errors.load() == 0, "fairness payload/FIFO validation failed");
            for (int32_t stream = 0; stream < 32; ++stream)
            {
                if (active[stream].load() == 0)
                {
                    continue;
                }
                const int32_t produced = sent[stream].load(std::memory_order_acquire) - beforeSent[stream];
                const int32_t consumed = ledger.m_received[stream].load(std::memory_order_acquire) - beforeReceived[stream];
                std::cout << "fairness window=" << window << " stream=" << stream << " bytes=" << ledger.m_bodies[stream].size()
                    << " sent=" << produced << " received=" << consumed << std::endl;
                bbCheck(produced > 0 && consumed > 0, "active stream made no progress: " + bbNumber(stream));
            }
        }
        stop.store(true, std::memory_order_release);
        jobs.drain();
        bbWait([&ledger, &sent]()
        {
            bbCheck(ledger.m_errors.load() == 0, "fairness final FIFO validation failed");
            for (int32_t stream = 0; stream < 32; ++stream)
            {
                if (ledger.m_received[stream].load(std::memory_order_acquire) != sent[stream].load(std::memory_order_acquire))
                {
                    return false;
                }
            }
            return true;
        }, "every accepted fairness packet delivered", 15000);
        scope.close();
        bbCheck(ledger.m_errors.load() == 0, "fairness shutdown validation failed");
    }
    catch (...)
    {
        stop.store(true, std::memory_order_release);
        // 撤销接收入口使仍在普通send中等待的任务退出；在释放其借用账本之前由jobs回收线程。
        scope.close();
        throw;
    }
}