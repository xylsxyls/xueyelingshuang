#include "SemaphoreTests.h"
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
#include "SemaphoreCount.h"
#endif
#include <algorithm>
#include <cstdio>

bool SemaphoreTests::fairness(int32_t id)
{
    if (id == 82)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        SemaphorePoolConfig config;
        config.initialCapacity = 64;
        config.maxCapacity = 64;
        SemaphoreEventPool pool(config);
        SemaphoreEventRecord* records[64] = {};
        for (size_t index = 0; index < 64; ++index)
        {
            check(pool.begin(records[index]).status == SEMAPHORE_OK, "event admission");
        }
        for (size_t turn = 0; turn < 4096; ++turn)
        {
            const size_t chosen = turn % 64;
            check(pool.notify(false).committed, "one event delivered");
            for (size_t index = 0; index < 64; ++index)
            {
                check(records[index]->m_state.load() == (index == chosen ? 1u : 0u),
                    "new arrivals cannot repeatedly bypass an older event waiter");
            }
            check(pool.finish(records[chosen]), "oldest event consumed");
            check(pool.begin(records[chosen]).status == SEMAPHORE_OK, "completed waiter joins again");
        }
        for (size_t index = 0; index < 64; ++index)
        {
            check(!pool.finish(records[index]), "cancel remaining event registration");
        }
        size_t released = 0;
        check(pool.maintain(released).status == SEMAPHORE_OK && pool.statistics().freeCapacity == 64,
            "event fairness records reclaimed");
        return true;
#else
        return false;
#endif
    }
    if (id == 83)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        {
            SemaphorePoolConfig config;
            config.initialCapacity = 64;
            config.maxCapacity = 64;
            SemaphoreCount queue(config);
            SemaphoreWake wake;
            std::atomic<uint64_t> state(0);
            SemaphoreEventRecord* records[64] = {};
            for (size_t index = 0; index < 64; ++index)
            {
                check(queue.begin(state, records[index], false).status == SEMAPHORE_OK, "count admission");
            }
            for (size_t turn = 0; turn < 4096; ++turn)
            {
                const size_t chosen = turn % 64;
                check(queue.signal(state, wake).committed, "count FIFO delivery");
                for (size_t index = 0; index < 64; ++index)
                {
                    check(records[index]->m_state.load() == (index == chosen ? 1u : 0u),
                        "every admitted count waiter progresses before newer arrivals");
                }
                check(queue.finish(state, records[chosen]), "delivered count consumed");
                check(queue.begin(state, records[chosen], false).status == SEMAPHORE_OK, "count waiter joins again");
            }
            for (size_t index = 0; index < 64; ++index)
            {
                check(!queue.finish(state, records[index]), "count FIFO remainder cancelled");
            }
            check(state.load() == 0, "empty registration chain restores the direct count representation");
            SemaphoreEventRecord* first = nullptr;
            SemaphoreEventRecord* second = nullptr;
            check(queue.begin(state, first, false).status == SEMAPHORE_OK &&
                    queue.begin(state, second, false).status == SEMAPHORE_OK, "paused publisher admissions");
            SemaphoreTestGate gate(4);
            SemaphoreTestThreads publishers;
            SemaphoreResult sent;
            publishers.start([&]() { sent = queue.signal(state, wake); });
            const bool paused = gate.reached();
            const bool firstReceived = queue.finish(state, first);
            const SemaphoreResult another = queue.signal(state, wake);
            const bool secondReceived = queue.finish(state, second);
            SemaphoreEventRecord* reused = nullptr;
            const SemaphoreResult joined = queue.begin(state, reused, false);
            const bool protectedAddress = reused != first && reused != second;
            if (reused != nullptr)
            {
                queue.finish(state, reused);
            }
            gate.release();
            publishers.join();
            check(paused && firstReceived && another.committed && secondReceived && sent.committed,
                "paused publisher does not block subsequent handoffs");
            check(joined.status == SEMAPHORE_OK && protectedAddress, "publisher keeps its notified address alive");
            size_t released = 0;
            check(queue.maintain(released).status == SEMAPHORE_OK && queue.statistics().freeCapacity == 64,
                "count FIFO and paused publisher records reclaimed");
        }
        for (int32_t prepared = 0; prepared != 2; ++prepared)
        {
            Semaphore sem;
            if (prepared != 0)
            {
                check(sem.prepareSignalAndWait().status == SEMAPHORE_OK, "prepared handoff backend");
            }
            SemaphoreTestGate gate(8);
            SemaphoreTestThreads threads;
            SemaphoreResult received;
            threads.start([&]() { received = sem.waitResult(2000); });
            const bool registered = gate.reached();
            const SemaphoreResult first = sem.trySignal();
            const bool stolen = sem.wait(0);
            const SemaphoreResult second = sem.trySignal();
            const bool subsequent = sem.wait(100);
            gate.release();
            threads.join();
            check(registered && first.committed && !stolen && second.committed && subsequent,
                "reserved handoff prevents barging without blocking subsequent permits");
            check(received.status == SEMAPHORE_OK && received.committed && !sem.wait(0),
                "paused admitted waiter receives exactly its own permit");
            const SemaphorePoolStats stats = sem.countPoolStatistics();
            check(stats.capacity == stats.freeCapacity && stats.retiredCapacity == 0, "count handoff records reclaimed");
        }
        return true;
#else
        return false;
#endif
    }
    if (id == 84)
    {
        // 发布线程也立即尝试消费，构造正在运行的线程与已睡眠消费者的真实竞争。
        // 每个消费者持续参与到同一截止时间，不用固定配额迫使较快者提前退出。
        const size_t count = 8;
        Semaphore sem, ready, start;
        std::atomic<bool> stop(false);
        uint64_t completed[count] = {};
        double maximumGap[count] = {};
        SemaphoreTestThreads threads;
        try
        {
            for (size_t index = 0; index < count; ++index)
            {
                threads.start([&, index]()
                {
                    ready.signal();
                    check(start.wait(5000), "fairness start barrier");
                    double last = now();
                    while (!stop.load())
                    {
                        if (sem.wait(50))
                        {
                            const double current = now();
                            maximumGap[index] = (std::max)(maximumGap[index], current - last);
                            last = current;
                            ++completed[index];
                        }
                    }
                    maximumGap[index] = (std::max)(maximumGap[index], now() - last);
                });
            }
            for (size_t index = 0; index < count; ++index)
            {
                check(ready.wait(5000), "fairness worker ready");
            }
            for (size_t index = 0; index < count; ++index)
            {
                start.signal();
            }
            uint64_t published = 0;
            uint64_t hot = 0;
            const double started = now();
            do
            {
                sem.signal();
                ++published;
                if (sem.wait(0))
                {
                    ++hot;
                }
            } while ((published & 255) != 0 || now() - started < 2000);
            stop.store(true);
            for (size_t index = 0; index < count; ++index)
            {
                sem.signal();
                ++published;
            }
            threads.join();
            uint64_t total = hot;
            for (size_t index = 0; index < count; ++index)
            {
                total += completed[index];
                std::printf("METRIC {\"name\":\"fairness_worker_%u\",\"completed\":%.0f,\"max_gap_ms\":%.3f}\n",
                    static_cast<unsigned>(index), static_cast<double>(completed[index]), maximumGap[index]);
                check(completed[index] != 0, "every persistent consumer makes progress");
                check(maximumGap[index] < 1000, "no consumer stalls for an entire one-second observation window");
            }
            while (sem.wait(0))
            {
                ++total;
            }
            check(total == published, "fairness workload exact permit accounting");
            std::printf("METRIC {\"name\":\"fairness_total\",\"published\":%.0f,\"hot_consumer\":%.0f,\"elapsed_ms\":%.3f}\n",
                static_cast<double>(published), static_cast<double>(hot), now() - started);
            return true;
        }
        catch (...)
        {
            // 主线程失败时先结束持续循环，再由线程所有者回收，避免异常路径无限join。
            stop.store(true);
            throw;
        }
    }
    if (id == 86)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        for (size_t threshold = 0; threshold <= 1; ++threshold)
        {
            size_t evaluations = 0;
            SemaphorePoolConfig policy;
            policy.initialCapacity = 1;
            policy.maxCapacity = 3;
            policy.freeThreshold = threshold;
            policy.grow = [&](const SemaphoreGrowContext&) { ++evaluations; return size_t(1); };
            SemaphoreCount pool(policy);
            std::atomic<uint64_t> state(0);
            SemaphoreEventRecord* first = nullptr;
            SemaphoreEventRecord* second = nullptr;
            check(pool.begin(state, first, false).status == SEMAPHORE_OK && evaluations == threshold,
                "count pool respects zero and positive free thresholds");
            check(pool.begin(state, second, false).status == SEMAPHORE_OK && evaluations == threshold + 1,
                "each count registration evaluates growth at most once");
            check(!pool.finish(state, first) && !pool.finish(state, second), "policy records cancelled");
        }
        {
            SemaphorePoolConfig policy;
            policy.initialCapacity = 1;
            policy.freeThreshold = 1;
            policy.grow = [](const SemaphoreGrowContext&) -> size_t { throw std::runtime_error("count grow policy"); };
            SemaphoreCount pool(policy);
            std::atomic<uint64_t> state(0);
            SemaphoreEventRecord* record = nullptr;
            check(pool.begin(state, record, false).status == SEMAPHORE_OK && record != nullptr &&
                pool.statistics().lastGrowStatus == SEMAPHORE_POLICY,
                "failed count pre-growth preserves the admitted record and reports policy status");
            check(!pool.finish(state, record), "failed pre-growth record recovered");
        }
        Semaphore sem;
        SemaphorePoolConfig config;
        config.initialCapacity = 2;
        config.maxCapacity = 2;
        check(sem.configureCountPool(config).status == SEMAPHORE_OK, "bounded cancellation pool");
        SemaphorePoolConfig invalid = config;
        invalid.maxCapacity = -2;
        check(sem.configureCountPool(invalid).status == SEMAPHORE_INVALID && sem.countPoolStatistics().capacity == 2,
            "invalid count configuration preserves the existing pool");
        for (int32_t index = 0; index < 100; ++index)
        {
            check(sem.waitResult(35).status == SEMAPHORE_TIMEOUT, "finite priority waiter cancelled within its original budget");
            check(sem.trySignal().committed && sem.wait(0) && !sem.wait(0), "cancelled waiter cannot consume a later permit");
        }
        {
            SemaphoreTestGate gate(5);
            SemaphoreTestThreads threads;
            SemaphoreResult result;
            threads.start([&]() { result = sem.waitResult(100); });
            const bool boundary = gate.reached();
            const SemaphoreResult sent = sem.trySignal();
            gate.release();
            threads.join();
            check(boundary && sent.committed && result.status == SEMAPHORE_OK && result.committed && !sem.wait(0),
                "handoff wins against uncommitted timeout cancellation");
        }
        const SemaphorePoolStats stats = sem.countPoolStatistics();
        check(stats.capacity == 2 && stats.freeCapacity == 2 && stats.retiredCapacity == 0,
            "bounded cancellation and handoff leave no records outstanding");
        SemaphorePoolConfig blocked;
        blocked.initialCapacity = 0;
        blocked.maxCapacity = 0;
        check(sem.configureCountPool(blocked).status == SEMAPHORE_OK, "zero capacity count pool");
        check(sem.waitResult(1).status == SEMAPHORE_TIMEOUT, "short competitive wait does not require a record");
        check(sem.waitResult(100).status == SEMAPHORE_CAPACITY, "priority admission failure is observable within the original budget");
        check(sem.trySignal().committed && sem.wait(0), "uncontended count does not need a pool record");
        return true;
#else
        return false;
#endif
    }
    return false;
}