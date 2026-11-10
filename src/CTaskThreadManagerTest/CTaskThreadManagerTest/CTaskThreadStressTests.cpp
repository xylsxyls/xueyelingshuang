#include "CTaskThreadStressTests.h"
#include "CTaskThreadManagerTest.h"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <thread>

/** 投递与排空关闭竞争，独立比较成功接纳集合和执行集合。
*/
static void PostDrainRace()
{
    TestWorkers consumers, producers;
    std::shared_ptr<CTaskThread> worker = consumers.add();
    std::atomic<int> accepted(0);
    std::vector<std::vector<int>> submitted(8);
    TaskLedger completed;
    for (int p = 0; p < 8; ++p)
    {
        producers.add()->postTask(std::make_shared<TestTask>(p, [&, p, worker](TestTask&)
        {
            for (int sequence = 0; sequence < 10000; ++sequence)
            {
                const int value = p * 10000 + sequence;
                std::shared_ptr<TestTask> task(new TestTask(1, [&, value](TestTask&)
                {
                    completed.add(value);
                }));
                for (;;)
                {
                    const CTaskThreadResult result = worker->postTask(task);
                    if (result.m_committed)
                    {
                        submitted[p].push_back(value);
                        ++accepted;
                        break;
                    }
                    if (result.m_status == CTaskThreadClosed)
                    {
                        return;
                    }
                    bbCheck(false, "ordinary post/drain call leaked contention or failed unexpectedly");
                }
            }
        }));
    }
    bbWait([&accepted]()
    {
        return accepted >= 500;
    }, "race established");
    bbCheck(CTaskThreadManager::instance().waitForEnd(consumers.ids[0]).m_status == CTaskThreadOk, "racing drain failed");
    producers.drain();
    consumers.drain();
    std::vector<int> expected;
    for (size_t p = 0; p < submitted.size(); ++p)
    {
        expected.insert(expected.end(), submitted[p].begin(), submitted[p].end());
    }
    std::vector<int> actual = completed.snapshot();
    std::sort(expected.begin(), expected.end());
    std::sort(actual.begin(), actual.end());
    bbCheck(actual == expected && actual.size() == static_cast<size_t>(accepted.load()), "drain lost/duplicated accepted set");
}

/** 两个调用者同时 join，只有一个拥有系统 join 操作权。
*/
static void ConcurrentJoin()
{
    TestWorkers target, joiners;
    std::shared_ptr<CTaskThread> worker = target.add();
    std::atomic<bool> running(false), release(false), start(false);
    std::atomic<int> enteredJoin(0), completed(0), failures(0);
    worker->postTask(std::make_shared<TestTask>(1, [&](TestTask&)
    {
        running = true;
        bbWait([&release]()
        {
            return release.load();
        }, "join target release");
    }));
    bbWait([&running]()
    {
        return running.load();
    }, "join target running");
    for (int i = 0; i < 2; ++i)
    {
        joiners.add()->postTask(std::make_shared<TestTask>(i, [&](TestTask&)
        {
            bbWait([&start]()
            {
                return start.load();
            }, "join barrier");
            ++enteredJoin;
            const CTaskThreadResult result = worker->waitForEnd();
            if (result.m_status != CTaskThreadOk || !result.m_committed)
            {
                ++failures;
            }
            ++completed;
        }));
    }
    start = true;
    bbWait([&enteredJoin]()
    {
        return enteredJoin == 2;
    }, "both join callers entered");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    const bool returnedEarly = completed != 0;
    release = true;
    joiners.drain();
    target.drain();
    bbCheck(!returnedEarly && completed == 2 && failures == 0,
        "concurrent join exposed Busy/Retry or returned before the worker exited");
}

/** 排空与丢弃关闭交错时，后来的停止意图不能被等待 join 的关闭者挡住。
*/
static void ConcurrentPoolShutdown()
{
    CTaskThreadPoolConfig config;
    config.m_initialThreads = 1;
    config.m_maximumThreads = 1;
    CTaskThreadPool pool;
    bbCheck(pool.initialize(config).m_status == CTaskThreadOk, "concurrent shutdown pool initialization failed");
    CTaskThreadAllocator allocator(pool);
    uint32_t id = 0;
    std::shared_ptr<CTaskThread> worker;
    bbCheck(allocator.select(id, worker).m_status == CTaskThreadOk && worker, "no pool worker selected");
    std::atomic<bool> entered(false), rescue(false);
    std::atomic<int> completed(0);
    std::shared_ptr<TestTask> task(new TestTask(1, [&](TestTask& self)
    {
        entered = true;
        while (self.stops.load() == 0 && !rescue.load())
        {
            std::this_thread::yield();
        }
    }));
    bbCheck(worker->postTask(task).m_committed, "shutdown gate not accepted");
    bbWait([&entered]()
    {
        return entered.load();
    }, "shutdown gate running");
    CTaskThreadResult drained, discarded;
    std::thread first([&]()
    {
        drained = pool.shutdown(true);
        ++completed;
    });
    const uint64_t enteringDeadline = bbNow() + 5000;
    while (worker->statistics().m_accepting && bbNow() < enteringDeadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const bool enteredDrain = !worker->statistics().m_accepting;
    std::thread second([&]()
    {
        discarded = pool.shutdown(false);
        ++completed;
    });
    const uint64_t completionDeadline = bbNow() + 5000;
    while (completed != 2 && bbNow() < completionDeadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const bool neededRescue = completed != 2;
    rescue = true;
    first.join();
    second.join();
    bbCheck(enteredDrain && !neededRescue && task->stops > 0 &&
            drained.m_status == CTaskThreadOk && discarded.m_status == CTaskThreadOk &&
            drained.m_committed && discarded.m_committed && pool.statistics().m_threads == 0,
        "concurrent pool shutdown withheld stop notification, exposed contention or retained workers");
}

/** 在自动扩缩容期间选择目标并核对每项只执行一次。
*/
static void PoolResizeTraffic()
{
    CTaskThreadPool pool;
    CTaskThreadPoolConfig config;
    config.m_initialThreads = 2;
    config.m_maximumThreads = 6;
    config.m_checkMilliseconds = 5;
    config.m_worker.m_maximumTasks = 128;
    std::shared_ptr<TestPoolPolicy> policy(new TestPoolPolicy);
    config.m_policy = policy;
    bbCheck(pool.initialize(config).m_committed, "traffic pool initialize failed");
    CTaskThreadAllocator allocator(pool);
    const int total = 10000;
    std::unique_ptr<std::atomic<int>[]> seen(new std::atomic<int>[total]);
    for (int i = 0; i < total; ++i)
    {
        seen[i].store(0);
    }
    std::atomic<int> done(0);
    for (int wave = 0; wave < 10; ++wave)
    {
        policy->m_target = 6;
        bbWait([&pool]()
        {
            return pool.statistics().m_threads == 6;
        }, "traffic pool growth");
        for (int index = wave * 1000; index < (wave + 1) * 1000; ++index)
        {
            std::shared_ptr<TestTask> task(new TestTask(1, [&, index](TestTask&)
            {
                bbCheck(seen[index].fetch_add(1) == 0, "duplicate pool task");
                ++done;
            }));
            if (index % 100 == 0)
            {
                policy->m_target = 2;
            }
            bbWait([&allocator, task]()
            {
                uint32_t id = 0;
                std::shared_ptr<CTaskThread> worker;
                if (!allocator.select(id, worker).m_committed)
                {
                    return false;
                }
                const CTaskThreadResult result = worker->postTask(task);
                bbCheck(result.m_committed || result.m_status == CTaskThreadClosed ||
                        result.m_status == CTaskThreadCapacity,
                    "resize traffic unexpected rejection");
                return result.m_committed;
            }, "resize traffic admission");
            const size_t count = pool.statistics().m_threads;
            bbCheck(count >= 2 && count <= 6, "thread count left configured bounds");
        }
        bbWait([&done, wave]()
        {
            return done == (wave + 1) * 1000;
        }, "traffic wave completes");
        bbWait([&pool]()
        {
            return pool.statistics().m_threads == 2;
        }, "traffic pool shrink");
    }
    const CTaskPoolStatistics state = pool.statistics();
    bbCheck(pool.shutdown().m_status == CTaskThreadOk && done == total, "pool traffic drain failed");
    for (int i = 0; i < total; ++i)
    {
        bbCheck(seen[i] == 1, "missing pool task");
    }
    bbCheck(state.m_grows == 40 && state.m_shrinks == 40 && state.m_errors == 0, "resize accounting or errors wrong");
}

/** 用长空闲补查间隔验证正常通知没有依赖超时兜底。
*/
static void WakeWithoutHeartbeat()
{
    CTaskThreadConfig config;
    config.m_idleCheckMilliseconds = 60000;
    TestWorkers workers;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "long heartbeat initialization failed");
    workers.ids.push_back(id);
    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    std::atomic<int> done(0);
    for (int i = 1; i <= 100; ++i)
    {
        bbSleep(1);
        bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&done](TestTask&)
        {
            ++done;
        })).m_committed,
            "wake task rejected");
        bbWait([&done, i]()
        {
            return done == i;
        }, "signal wakes without heartbeat", 500);
    }
    workers.drain();
}

/** 持续满载，有限接纳容量、八生产者、独立序号与校验和预期。
*/
static void SustainedPressure()
{
    uint64_t milliseconds = 60000;
    const char* configured = std::getenv("CTTM_STRESS_MS");
    if (configured != nullptr)
    {
        milliseconds = std::strtoul(configured, nullptr, 10);
        bbCheck(milliseconds >= 1000 && milliseconds <= 600000, "CTTM_STRESS_MS out of range");
    }
    CTaskThreadConfig config;
    config.m_maximumTasks = 4096;
    TestWorkers consumers, producers;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "sustained consumer initialize failed");
    consumers.ids.push_back(id);
    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    std::vector<uint64_t> next(8, 0);
    std::atomic<uint64_t> accepted(0), executed(0), expectedSum(0), actualSum(0), retries(0), resources(0);
    std::atomic<bool> start(false);
    uint64_t deadline = 0;
    for (int p = 0; p < 8; ++p)
    {
        producers.add()->postTask(std::make_shared<TestTask>(p, [&, p, worker](TestTask&)
        {
            bbWait([&start]()
            {
                return start.load();
            }, "sustained start");
            uint64_t sequence = 0;
            while (bbNow() < deadline)
            {
                const uint64_t checksum = (sequence + 1) * 2654435761u + static_cast<uint64_t>(p);
                std::shared_ptr<TestTask> task(new TestTask(p, [&, p, sequence, checksum](TestTask&)
                {
                    bbCheck(next[p]++ == sequence, "sustained FIFO/duplicate failure");
                    actualSum.fetch_add(checksum, std::memory_order_relaxed);
                    executed.fetch_add(1, std::memory_order_relaxed);
                }));
                for (;;)
                {
                    const CTaskThreadResult result = worker->postTask(task);
                    if (result.m_committed)
                    {
                        ++sequence;
                        accepted.fetch_add(1, std::memory_order_relaxed);
                        expectedSum.fetch_add(checksum, std::memory_order_relaxed);
                        break;
                    }
                    bbCheck(result.m_status == CTaskThreadCapacity || result.m_status == CTaskThreadResourceUnavailable,
                        (std::string("sustained unexpected failure status=") + std::to_string(result.m_status)).c_str());
                    resources.fetch_add(result.m_status == CTaskThreadResourceUnavailable ? 1 : 0, std::memory_order_relaxed);
                    ++retries;
                    if (bbNow() >= deadline)
                    {
                        return;
                    }
                    std::this_thread::yield();
                }
            }
        }));
    }
    const uint64_t begun = bbNow();
    deadline = begun + milliseconds;
    start = true;
    producers.drain();
    consumers.drain();
    const uint64_t elapsed = bbNow() - begun;
    const CTaskThreadStatistics state = worker->statistics();
    bbCheck(accepted == executed && actualSum == expectedSum && accepted > 10000, "sustained count/checksum mismatch");
    bbCheck(state.m_outstanding == 0 && state.m_queued == 0 && state.m_errors == 0, "sustained resource/error accounting failed");
    std::cout << "duration_ms=" << elapsed << " accepted=" << accepted.load() << " executed=" << executed.load()
              << " uncommitted_resource_retries=" << retries.load() << " unsafe_reuse=" << resources.load()
              << " checksum=" << actualSum.load() << '\n';
}

std::vector<BlackBoxCase> taskThreadStressCases(int32_t firstId)
{
    std::vector<BlackBoxCase> cases;
    const std::function<void(const char*, const char*, int, const std::function<void()>&)> add =
        [&cases, firstId](const char* name, const char* expected, int timeout, const std::function<void()>& run)
    {
        BlackBoxCase item = {firstId + static_cast<int>(cases.size()), name, expected, true, timeout, [run]()
        {
            run();
            bbCheck(TestTask::live == 0 && TestTask::errors == 0, "stress task references or fixture errors remain");
            bbCheck(CTaskThreadManager::instance().count() == 0, "stress left managed workers");
        }};
        cases.push_back(item);
    };
    add("post-drain-race", "accepted set exactly equals completed set under concurrent shutdown", 60, PostDrainRace);
    add("concurrent-join", "one native join owner; both callers complete successfully without Busy/Retry", 30, ConcurrentJoin);
    add("concurrent-pool-shutdown", "discard request interrupts an ongoing drain; both callers finish", 30, ConcurrentPoolShutdown);
    add("pool-resize-traffic", "10 grow/shrink waves; 10000 items exactly once; 2..6 threads", 120, PoolResizeTraffic);
    add("wake-without-heartbeat", "100 idle transitions wake within 500ms despite 60s heartbeat", 30, WakeWithoutHeartbeat);
    add("sustained-pressure", "8 producers; 4096 capacity; FIFO/checksum/exact count; default60s", 660, SustainedPressure);
    return cases;
}