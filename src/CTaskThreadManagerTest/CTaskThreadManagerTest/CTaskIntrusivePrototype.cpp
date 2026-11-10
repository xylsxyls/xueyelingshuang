#include "CTaskIntrusivePrototype.h"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#endif

CTaskPrototypeNode::CTaskPrototypeNode() :
m_next(nullptr),
m_sequence(0),
m_producer(0)
{

}

CTaskPrototypeIntrusive::CTaskPrototypeIntrusive() :
m_head(&m_dummy),
m_tail(&m_dummy)
{

}

void CTaskPrototypeIntrusive::push(CTaskPrototypeNode* node)
{
    for (;;)
    {
        CTaskPrototypeNode* tail = m_tail.load(std::memory_order_acquire);
        CTaskPrototypeNode* next = tail->m_next.load(std::memory_order_acquire);
        if (tail != m_tail.load(std::memory_order_acquire))
        {
            continue;
        }
        if (next != nullptr)
        {
            m_tail.compare_exchange_weak(tail, next, std::memory_order_release, std::memory_order_relaxed);
        }
        else if (tail->m_next.compare_exchange_weak(next, node, std::memory_order_release, std::memory_order_relaxed))
        {
            // 链接已经完整发布，其他线程可以推进尚未更新的尾指针。
            m_tail.compare_exchange_strong(tail, node, std::memory_order_release, std::memory_order_relaxed);
            return;
        }
    }
}

CTaskPrototypeNode* CTaskPrototypeIntrusive::pop()
{
    CTaskPrototypeNode* next = m_head->m_next.load(std::memory_order_acquire);
    if (next != nullptr)
    {
        m_head = next;
    }
    return next;
}

CTaskPrototypeSegments::CTaskPrototypeSegments(size_t count) :
m_segments(new LockFreeQueueChannelSegment<CTaskPrototypeNode>[count]),
m_count(count),
m_next(0)
{

}

LockFreeQueueChannelSegment<CTaskPrototypeNode>* CTaskPrototypeSegments::acquire()
{
    const size_t index = m_next.fetch_add(1, std::memory_order_relaxed);
    if (index >= m_count)
    {
        return nullptr;
    }
    LockFreeQueueChannelSegment<CTaskPrototypeNode>* segment = &m_segments[index];
    segment->reset();
    segment->m_generation = index + 1;
    return segment;
}

CTaskPrototypeSegmentLease::CTaskPrototypeSegmentLease(CTaskPrototypeSegments& segments) :
m_segments(&segments),
m_candidate(nullptr)
{

}

LockFreeQueueChannelSegment<CTaskPrototypeNode>* CTaskPrototypeSegmentLease::acquire()
{
    LockFreeQueueChannelSegment<CTaskPrototypeNode>* candidate = m_candidate;
    m_candidate = nullptr;
    return candidate != nullptr ? candidate : m_segments->acquire();
}

void CTaskPrototypeSegmentLease::release(LockFreeQueueChannelSegment<CTaskPrototypeNode>* segment)
{
    m_candidate = segment;
}

void CTaskPrototypeSegmentLease::retire(LockFreeQueueChannelSegment<CTaskPrototypeNode>* segment)
{
    (void)segment;
}

CTaskPrototypePublisher::CTaskPrototypePublisher() :
m_hazard(nullptr),
m_generation(0),
m_hint(0),
m_retries(0)
{

}

CTaskPrototypeSegmented::CTaskPrototypeSegmented() :
m_channel(nullptr)
{

}

void CTaskPrototypeSegmented::initialize(LockFreeQueueChannelSegment<CTaskPrototypeNode>* first)
{
    m_channel.reset(first);
}

bool CTaskPrototypeSegmented::push(CTaskPrototypeNode* node, CTaskPrototypePublisher& publisher, CTaskPrototypeSegmentLease& lease)
{
    return m_channel.publish(node, publisher, lease);
}

CTaskPrototypeNode* CTaskPrototypeSegmented::pop(CTaskPrototypeSegmentLease& lease)
{
    CTaskPrototypeNode* node = m_channel.front(lease);
    if (node != nullptr)
    {
        m_channel.consume();
    }
    return node;
}

CTaskPrototypeProgress::CTaskPrototypeProgress() :
m_finished(0),
m_consumed(0)
{

}

/** 获取单调纳秒，仅用于测试计时
@return 当前单调时刻
*/
static uint64_t PrototypeNow()
{
#ifdef _WIN32
    LARGE_INTEGER counter, frequency;
    if (!QueryPerformanceCounter(&counter) || !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
    {
        throw std::runtime_error("prototype performance counter failed");
    }
    const uint64_t ticks = static_cast<uint64_t>(counter.QuadPart);
    const uint64_t rate = static_cast<uint64_t>(frequency.QuadPart);
    return ticks / rate * UINT64_C(1000000000) + ticks % rate * UINT64_C(1000000000) / rate;
#else
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}

int taskRunIntrusivePrototype(int argc, char* argv[])
{
    try
    {
        if (argc != 6)
        {
            throw std::runtime_error("usage: intrusive|segmented producers total levels report.csv");
        }
        const std::string mode = argv[1];
        const size_t producers = static_cast<size_t>(std::strtoul(argv[2], nullptr, 10));
        const size_t total = static_cast<size_t>(std::strtoul(argv[3], nullptr, 10));
        const size_t levels = static_cast<size_t>(std::strtoul(argv[4], nullptr, 10));
        if ((mode != "intrusive" && mode != "segmented") || producers == 0 || producers > 32 ||
            total == 0 || total > 10000000 || total % producers != 0 || levels == 0 || levels > 10)
        {
            throw std::runtime_error("invalid prototype arguments");
        }
        const bool intrusive = mode == "intrusive";
        const size_t perProducer = total / producers;
        std::unique_ptr<CTaskPrototypeNode[]> nodes(new CTaskPrototypeNode[total]);
        std::unique_ptr<CTaskPrototypeIntrusive[]> linked(new CTaskPrototypeIntrusive[levels]);
        std::unique_ptr<CTaskPrototypeSegmented[]> segmented(new CTaskPrototypeSegmented[levels]);
        std::unique_ptr<CTaskPrototypeProgress[]> progress(new CTaskPrototypeProgress[producers]);
        CTaskPrototypeSegments segments(total / 256 * 3 + producers + levels * 2 + 8);
        std::vector<uint64_t> expected(producers * levels, 0);
        for (size_t level = 0; level < levels; ++level)
        {
            segmented[level].initialize(segments.acquire());
        }
        for (size_t p = 0; p < producers; ++p)
        {
            for (size_t i = 0; i < perProducer; ++i)
            {
                CTaskPrototypeNode& node = nodes[p * perProducer + i];
                node.m_producer = p;
                node.m_sequence = i;
            }
        }
        std::atomic<size_t> ready(0);
        std::atomic<bool> start(false), stop(false);
        std::atomic<uint64_t> consumed(0), completedAt(0), errors(0);
        std::vector<std::thread> threads;
        threads.reserve(producers + 1);
        try
        {
            threads.push_back(std::thread([&]()
            {
                try
                {
                    CTaskPrototypeSegmentLease lease(segments);
                    ready.fetch_add(1, std::memory_order_release);
                    while (!start.load(std::memory_order_acquire) && !stop.load(std::memory_order_acquire))
                    {
                        std::this_thread::yield();
                    }
                    const uint64_t deadline = PrototypeNow() + UINT64_C(120000000000);
                    uint64_t done = 0, polls = 0;
                    while (done != total && !stop.load(std::memory_order_acquire))
                    {
                        CTaskPrototypeNode* node = nullptr;
                        for (size_t level = levels; level != 0 && node == nullptr; --level)
                        {
                            node = intrusive ? linked[level - 1].pop() : segmented[level - 1].pop(lease);
                        }
                        if (node != nullptr)
                        {
                            if (node->m_producer >= producers || node->m_sequence >= perProducer)
                            {
                                throw std::runtime_error("prototype payload identity corrupted");
                            }
                            uint64_t& next = expected[node->m_producer * levels + node->m_sequence % levels];
                            if (next++ != node->m_sequence / levels)
                            {
                                throw std::runtime_error("prototype FIFO or exactly-once check failed");
                            }
                            ++done;
                            if ((done & 1023) == 0)
                            {
                                consumed.store(done, std::memory_order_release);
                            }
                        }
                        else
                        {
                            std::this_thread::yield();
                        }
                        if ((++polls & 4095) == 0 && PrototypeNow() > deadline)
                        {
                            throw std::runtime_error("prototype consumer timed out");
                        }
                    }
                    consumed.store(done, std::memory_order_release);
                    completedAt.store(PrototypeNow(), std::memory_order_release);
                }
                catch (...)
                {
                    errors.fetch_add(1);
                    stop.store(true, std::memory_order_release);
                }
            }));
            for (size_t p = 0; p < producers; ++p)
            {
                threads.push_back(std::thread([&, p]()
                {
                    try
                    {
                        CTaskPrototypeSegmentLease lease(segments);
                        CTaskPrototypePublisher publisher;
                        ready.fetch_add(1, std::memory_order_release);
                        while (!start.load(std::memory_order_acquire) && !stop.load(std::memory_order_acquire))
                        {
                            std::this_thread::yield();
                        }
                        for (size_t i = 0; i < perProducer && !stop.load(std::memory_order_acquire); ++i)
                        {
                            CTaskPrototypeNode* node = &nodes[p * perProducer + i];
                            if (intrusive)
                            {
                                linked[i % levels].push(node);
                            }
                            else if (!segmented[i % levels].push(node, publisher, lease))
                            {
                                throw std::runtime_error("prototype segment reserve exhausted");
                            }
                        }
                        progress[p].m_consumed = consumed.load(std::memory_order_acquire);
                        progress[p].m_finished = PrototypeNow();
                    }
                    catch (...)
                    {
                        errors.fetch_add(1);
                        stop.store(true, std::memory_order_release);
                    }
                }));
            }
        }
        catch (...)
        {
            stop.store(true, std::memory_order_release);
            start.store(true, std::memory_order_release);
            for (size_t i = 0; i < threads.size(); ++i)
            {
                threads[i].join();
            }
            throw;
        }
        const uint64_t readyDeadline = PrototypeNow() + UINT64_C(30000000000);
        while (ready.load(std::memory_order_acquire) != producers + 1 && !stop.load(std::memory_order_acquire))
        {
            if (PrototypeNow() > readyDeadline)
            {
                errors.fetch_add(1);
                stop.store(true, std::memory_order_release);
            }
            std::this_thread::yield();
        }
        const uint64_t begun = PrototypeNow();
        start.store(true, std::memory_order_release);
        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i].join();
        }
        if (errors.load() != 0 || consumed.load() != total)
        {
            throw std::runtime_error("prototype validation failed");
        }
        uint64_t submittedAt = 0, overlap = 0;
        for (size_t p = 0; p < producers; ++p)
        {
            if (progress[p].m_finished > submittedAt)
            {
                submittedAt = progress[p].m_finished;
                overlap = progress[p].m_consumed;
            }
            for (size_t level = 0; level < levels; ++level)
            {
                const size_t wanted = perProducer / levels + (level < perProducer % levels ? 1 : 0);
                if (expected[p * levels + level] != wanted)
                {
                    throw std::runtime_error("prototype missing producer/priority suffix");
                }
            }
        }
        const double seconds = static_cast<double>(submittedAt - begun) / 1e9;
        std::ofstream output(argv[5], std::ios::binary | std::ios::trunc);
        output << "core_only,producers,levels,total,completed,submit_ops_s,overlap_consume_ops_s,complete_ms,errors\n";
        output << std::fixed << std::setprecision(6) << mode << ',' << producers << ',' << levels << ',' << total << ','
               << consumed.load() << ',' << total / seconds << ',' << overlap / seconds << ','
               << (completedAt.load() - begun) / 1e6 << ',' << errors.load() << '\n';
        output.close();
        if (!output)
        {
            throw std::runtime_error("prototype report failed");
        }
        std::cout << "PASS core-only " << mode << ' ' << producers << '/' << levels << " produce="
                  << total / seconds << " consume=" << overlap / seconds << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL prototype: " << error.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "FAIL prototype: unknown exception\n";
        return 1;
    }
}

#ifdef CTTM_INTRUSIVE_PROTOTYPE_MAIN
int main(int argc, char* argv[])
{
    return taskRunIntrusivePrototype(argc, argv);
}

#endif