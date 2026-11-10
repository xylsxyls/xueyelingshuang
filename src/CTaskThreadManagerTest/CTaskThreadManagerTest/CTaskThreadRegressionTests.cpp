#include "CTaskThreadRegressionTests.h"
#include "CTaskThreadManagerTest.h"
#include "CTaskThreadTestAccess.h"
#include "CTaskFaultAllocation.h"
#include <algorithm>
#include <thread>
#include <stdexcept>

TestFactoryTask::TestFactoryTask(int32_t propertyId, const std::function<void(TestTask&)>& body,
    bool redo) :
TestTask(propertyId, body),
m_redo(redo)
{

}

bool TestFactoryTask::reExecute()
{
    return m_redo;
}

TestFactoryCopyFailure::TestFactoryCopyFailure(bool& failCopy, std::atomic<int32_t>& calls, bool failAfterCreate) :
m_failCopy(&failCopy),
m_calls(&calls),
m_failAfterCreate(failAfterCreate)
{

}

TestFactoryCopyFailure::TestFactoryCopyFailure(const TestFactoryCopyFailure& source) :
m_failCopy(source.m_failCopy),
m_calls(source.m_calls),
m_failAfterCreate(source.m_failAfterCreate)
{
    if (*m_failCopy)
    {
        throw std::runtime_error("injected factory copy failure");
    }
}

CTask* TestFactoryCopyFailure::operator()() const
{
    ++*m_calls;
    std::unique_ptr<CTask> created(new TestTask(1, [](TestTask&)
    {

    }));
    if (m_failAfterCreate)
    {
        *m_failCopy = true;
    }
    return created.release();
}

TestConcurrentRedoTask::TestConcurrentRedoTask(const std::shared_ptr<RedoState>& state,
    std::atomic<int32_t>& entered, std::atomic<bool>& release) :
RedoTask(state, 0, true, true),
m_entered(entered),
m_release(release)
{

}

bool TestConcurrentRedoTask::reExecute()
{
    ++m_entered;
    bbWait([this]()
    {
        return m_release.load();
    }, "release simultaneous redo callbacks");
    return true;
}

TestPropertyTask::TestPropertyTask() :
CTask()
{

}

TestPropertyTask::TestPropertyTask(int32_t propertyId) :
CTask(propertyId)
{
    clone([propertyId]() -> CTask*
    {
        return new TestPropertyTask(propertyId);
    });
}

void TestPropertyTask::doTask()
{

}

#ifndef CTTM_TEST_DLL
/** 管理器保护记录耗尽不能被当成空目录
*/
static void ManagerTraversalExhaustion()
{
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    CTaskThreadManagerConfig config;
    config.m_threadMap.operationRecordLimit = 4;
    bbCheck(manager.configure(config).m_status == CTaskThreadOk, "manager protection config failed");
    TestWorkers workers;
    std::vector<std::shared_ptr<CTaskThread>> readers;
    for (int i = 0; i < 4; ++i)
    {
        readers.push_back(workers.add());
    }
    std::atomic<int> held(0), done(0);
    std::atomic<bool> release(false);
    for (size_t i = 0; i < readers.size(); ++i)
    {
        bbCheck(readers[i]->postTask(std::make_shared<TestTask>(1, [&held, &release, &done](TestTask&)
        {
            bbCheck(CTaskThreadTestAccess::holdManagerRead([&held, &release]()
            {
                ++held;
                bbWait([&release]()
                {
                    return release.load();
                }, "manager reader release");
            }),
                "failed to acquire manager record");
            ++done;
        })).m_committed,
            "cannot submit manager reader");
    }
    bbWait([&held]()
    {
        return held == 4;
    }, "manager guards all occupied");
    CTaskThreadResult result;
    std::atomic<bool> closing(false), closed(false);
    std::thread closer([&]()
    {
        closing = true;
        result = manager.uninitAll();
        closed = true;
    });
    while (!closing.load())
    {
        std::this_thread::yield();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    const bool returnedEarly = closed.load();
    release = true;
    closer.join();
    bbWait([&done]()
    {
        return done == 4;
    }, "all manager guards released");
    workers.drain();
    bbCheck(!returnedEarly && result.m_status == CTaskThreadOk && result.m_committed && manager.count() == 0,
        "bulk shutdown leaked contention or left registrations behind");
}

#endif

/** 跨任务嵌套停止必须识别整个调用栈，不能递归通知或间接 join 自己
*/
static void NestedStopCallbacks()
{
    TestWorkers workers;
    std::shared_ptr<CTaskThread> a = workers.add(), b = workers.add();
    const uint32_t idA = workers.ids[0];
    std::atomic<int> entered(0), nestedStatus(-1), joinStatus(-1), stopCallbacks(0);
    std::atomic<bool> release(false);
    const std::function<void(TestTask&)> body = [&entered, &release](TestTask&)
    {
        ++entered;
        bbWait([&release]()
        {
            return release.load();
        }, "nested stop release");
    };
    std::shared_ptr<TestTask> taskA(new TestTask(1, body)), taskB(new TestTask(2, body));
    taskA->onStop = [b, &stopCallbacks]()
    {
        ++stopCallbacks;
        b->stopCurTask();
    };
    taskB->onStop = [a, idA, &nestedStatus, &joinStatus, &stopCallbacks]()
    {
        ++stopCallbacks;
        nestedStatus = a->stopCurTask().m_status;
        joinStatus = CTaskThreadManager::instance().waitForEnd(idA).m_status;
    };
    a->postTask(taskA);
    b->postTask(taskB);
    bbWait([&entered]()
    {
        return entered == 2;
    }, "nested tasks entered");
    a->stopCurTask();
    release = true;
    workers.drain();
    taskA->onStop = std::function<void()>();
    taskB->onStop = std::function<void()>();
    bbCheck(nestedStatus == CTaskThreadSelfWait && joinStatus == CTaskThreadSelfWait && stopCallbacks == 2,
        "nested callback recursion or indirect self-wait was not rejected");
}

/** 初始化参数错误和池局部启动失败都要完整回滚
*/
static void InitializationRollback()
{
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    CTaskThreadConfig invalid;
    invalid.m_idleCheckMilliseconds = 0;
    uint32_t id = 123;
    bbCheck(manager.initialize(invalid, id).m_status == CTaskThreadInvalid && id == 0 && manager.count() == 0,
        "invalid thread config leaked worker");
    CTaskThreadPoolConfig config;
    config.m_initialThreads = 3;
    config.m_members.nodes.initialCapacity = 1;
    config.m_members.nodes.maximumCapacity = 1;
    config.m_members.values.initialCapacity = 1;
    config.m_members.values.maximumCapacity = 1;
    CTaskThreadPool pool;
    bbCheck(!pool.initialize(config).m_committed && manager.count() == 0,
        "partial member-map initialization retained manager worker");
    config = CTaskThreadPoolConfig();
    bbCheck(pool.initialize(config).m_committed, "pool failed to recover after rejected initialization");
    bbCheck(pool.shutdown().m_status == CTaskThreadOk && manager.count() == 0, "recovered pool failed cleanup");
}

/** 串行预热后的所有线程取得同一管理器，关闭工作线程不重建单例
*/
static void SingletonAfterPrewarm()
{
    CTaskThreadManager* expected = &CTaskThreadManager::instance();
    TestWorkers workers;
    std::atomic<int32_t> calls(0);
    for (int32_t i = 0; i < 8; ++i)
    {
        std::shared_ptr<CTaskThread> worker = workers.add();
        bbCheck(worker->postTask(std::make_shared<TestTask>(1, [expected, &calls](TestTask&)
        {
            for (int32_t n = 0; n < 10000; ++n)
            {
                bbCheck(&CTaskThreadManager::instance() == expected, "prewarmed singleton identity changed");
            }
            ++calls;
        })).m_committed,
            "singleton observer was not accepted");
    }
    workers.drain();
    bbCheck(calls == 8 && &CTaskThreadManager::instance() == expected && expected->count() == 0,
        "singleton identity or worker cleanup changed");
}

/** 属性 ID 不要求唯一，保留完整取值及按属性批量停止的原有行为
*/
static void PropertyIdentifiers()
{
    TestPropertyTask defaults;
    bbCheck(defaults.getPropertyId() == 1, "default property ID changed");
    const int32_t properties[] = {INT32_MIN, -7, 0, 1, INT32_MAX};
    for (size_t i = 0; i < sizeof(properties) / sizeof(properties[0]); ++i)
    {
        TestPropertyTask task(properties[i]);
        std::unique_ptr<CTask> clone((*task.cloneFactory())());
        bbCheck(task.getPropertyId() == properties[i] && clone && clone.get() != &task &&
                clone->getPropertyId() == properties[i],
            "explicit or cloned property ID changed");
    }
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    std::atomic<bool> entered(false), release(false);
    std::atomic<int32_t> completed(0);
    std::shared_ptr<TestTask> gate(new TestTask(77, [&entered, &release](TestTask&)
    {
        entered = true;
        bbWait([&release]()
        {
            return release.load();
        }, "property gate release");
    }));
    bbCheck(worker->postTask(gate, INT32_MAX).m_committed, "property gate rejected");
    bbWait([&entered]()
    {
        return entered.load();
    }, "property gate entered");
    std::shared_ptr<TestTask> first(new TestTask(0, [&completed](TestTask&)
    {
        ++completed;
    }));
    std::shared_ptr<TestTask> second(new TestTask(0, [&completed](TestTask&)
    {
        ++completed;
    }));
    std::shared_ptr<TestTask> negative(new TestTask(-7, [&completed](TestTask&)
    {
        ++completed;
    }));
    bbCheck(worker->postTask(first, 1).m_committed && worker->postTask(second, 2).m_committed &&
            worker->postTask(negative, 1).m_committed,
        "duplicate/negative property ID was rejected");
    bbCheck(worker->stopTask(0).m_status == CTaskThreadOk && first->stops == 1 && second->stops == 1 &&
            negative->stops == 0 && gate->stops == 0,
        "property ID zero did not select both matching tasks");
    bbCheck(worker->stopTask(-7).m_status == CTaskThreadOk && negative->stops == 1 &&
            first->getPropertyId() == 0 && second->getPropertyId() == 0,
        "negative property selection changed");
    release = true;
    workers.drain();
    bbCheck(completed == 3, "property rename changed stopTask notification-only semantics");
}

/** 八个生产者同时请求重做仍只产生一个副本，并保持原级别队头语义
*/
static void ConcurrentRedoOrdering()
{
    for (int32_t round = 0; round < 20; ++round)
    {
        std::shared_ptr<RedoState> state = std::make_shared<RedoState>();
        std::atomic<int32_t> callbacks(0);
        std::atomic<bool> releaseCallbacks(false);
        TestWorkers consumers, producers;
        std::shared_ptr<CTaskThread> worker = consumers.add();
        bbCheck(worker->postTask(std::make_shared<TestConcurrentRedoTask>(state, callbacks, releaseCallbacks), 1).m_committed,
            "concurrent redo original rejected");
        bbWait([state]()
        {
            return state->entered.load();
        }, "concurrent redo original entered");
        for (int32_t i = 0; i < 2; ++i)
        {
            bbCheck(worker->postTask(std::make_shared<TestTask>(1, [state, i](TestTask&)
            {
                state->ledger.add(300 + i);
            }),
                              1)
                        .m_committed,
                "same-level follower rejected");
        }
        for (int32_t i = 0; i < 8; ++i)
        {
            bbCheck(producers.add()->postTask(std::make_shared<TestTask>(1, [worker, state, i](TestTask&)
            {
                bbCheck(worker->postTask(std::make_shared<TestTask>(2, [state, i](TestTask&)
                {
                    state->ledger.add(200 + i);
                }),
                                  2)
                            .m_committed,
                    "simultaneous high-priority post rejected");
            })).m_committed,
                "producer task rejected");
        }
        bbWait([&callbacks]()
        {
            return callbacks.load() == 8;
        }, "all redo callbacks overlap");
        releaseCallbacks = true;
        producers.drain();
        bbCheck(state->stops == 8 && state->ledger.snapshot() == std::vector<int>(1, 100),
            "preemption lost notification or ran a second task before original returned");
        state->release = true;
        consumers.drain();
        const std::vector<int> order = state->ledger.snapshot();
        bbCheck(order.size() == 12 && order[0] == 100 && order[9] == 101 && order[10] == 300 && order[11] == 301,
            "concurrent preemption duplicated/lost redo or moved it behind same-level followers");
        std::vector<int> high(order.begin() + 1, order.begin() + 9);
        std::sort(high.begin(), high.end());
        for (int32_t i = 0; i < 8; ++i)
        {
            bbCheck(high[i] == 200 + i, "concurrent preemption lost/duplicated a high-priority task");
        }
    }
}

/** 常用级别缓存切换到完整索引时仍保留所有队列和最高优先级顺序
*/
static void CachedPriorityFallback()
{
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    std::atomic<bool> entered(false), release(false);
    TaskLedger ledger;
    bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&entered, &release](TestTask&)
    {
        entered = true;
        bbWait([&release]()
        {
            return release.load();
        }, "cache fallback gate");
    }),
                      16)
                .m_committed,
        "cache fallback gate rejected");
    bbWait([&entered]()
    {
        return entered.load();
    }, "cache fallback gate entered");
    const int32_t levels[] = {1, 16, 1, 17, 16, INT32_MAX, 2};
    for (int32_t i = 0; i < 7; ++i)
    {
        bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&ledger, i](TestTask&)
        {
            ledger.add(i);
        }),
                          levels[i])
                    .m_committed,
            "cache fallback item rejected");
    }
    release = true;
    workers.drain();
    const int expected[] = {5, 3, 1, 4, 6, 0, 2};
    bbCheck(ledger.snapshot() == std::vector<int>(expected, expected + 7), "cache fallback changed priority/FIFO");
}

#ifndef CTTM_TEST_DLL
/** 累计值跨 UINT64_MAX 时，额度上限、FIFO 计数和关闭排空都不能失真
*/
static void SplitCountersWrap()
{
    std::atomic<bool> entered(false), release(false);
    std::atomic<int32_t> calls(0);
    TestWorkers workers;
    CTaskThreadConfig config;
    config.m_maximumTasks = 3;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "counter test initialization failed");
    workers.ids.push_back(id);
    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&entered, &release](TestTask& task)
    {
        entered = true;
        bbWait([&release, &task]()
        {
            return release.load() || task.stopped.load();
        }, "counter wrap gate");
    }),
                      INT_MAX)
                .m_committed,
        "counter gate rejected");
    bbWait([&entered]()
    {
        return entered.load();
    }, "counter gate entered");
    CTaskThreadTestAccess::seedPendingCounters(*worker);
    std::shared_ptr<TestTask> task(new TestTask(2, [&calls](TestTask&)
    {
        ++calls;
    }));
    bbCheck(worker->postTask(task).m_committed && worker->postTask(task).m_committed, "counter wrap rejected allowed tasks");
    const CTaskThreadResult full = worker->postTask(task);
    bbCheck(!full.m_committed && full.m_status == CTaskThreadCapacity, "counter wrap lost maximum task limit");
    const CTaskThreadStatistics pending = worker->statistics();
    bbCheck(pending.m_outstanding == 3 && pending.m_queued == 2, "counter wrap changed pending counts");
    release = true;
    workers.drain();
    const CTaskThreadStatistics ended = worker->statistics();
    bbCheck(calls == 2 && ended.m_outstanding == 0 && ended.m_queued == 0 && ended.m_executed == 3,
        "counter wrap lost tasks or failed to drain");
}

#endif

/** 多生产者与消费归还竞争时，独立统计仍保持在业务上限以内
*/
static void ConcurrentBoundedStatistics()
{
    std::atomic<int32_t> accepted(0), calls(0);
    TestWorkers consumers, producers;
    CTaskThreadConfig config;
    config.m_maximumTasks = 3;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "statistics test initialization failed");
    consumers.ids.push_back(id);
    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    for (int32_t i = 0; i < 4; ++i)
    {
        bbCheck(producers.add()->postTask(std::make_shared<TestTask>(1, [worker, &accepted, &calls](TestTask&)
        {
            const uint64_t deadline = bbNow() + 20000;
            for (int32_t sequence = 0; sequence < 5000; ++sequence)
            {
                std::shared_ptr<TestTask> task(new TestTask(1, [&calls](TestTask&)
                {
                    ++calls;
                }));
                for (;;)
                {
                    const CTaskThreadResult result = worker->postTask(task);
                    const CTaskThreadStatistics load = worker->statistics();
                    bbCheck(load.m_outstanding <= 3 && load.m_queued <= 3, "statistics exceeded actual admission bound");
                    if (result.m_committed)
                    {
                        ++accepted;
                        break;
                    }
                    bbCheck(result.m_status == CTaskThreadCapacity,
                        "unexpected statistics test post rejection");
                    bbCheck(bbNow() < deadline, "bounded statistics producer deadline");
                    // 高竞争回归不逐次 sleep，避免 Windows 定时粒度把测试变成限速器。
                    std::this_thread::yield();
                }
            }
        })).m_committed,
            "statistics producer rejected");
    }
    producers.drain();
    consumers.drain();
    const CTaskThreadStatistics ended = worker->statistics();
    bbCheck(accepted == 20000 && calls == accepted && ended.m_outstanding == 0 && ended.m_queued == 0,
        "statistics sampling changed task delivery or final counts");
}

std::vector<BlackBoxCase> taskThreadRegressionCases(int32_t firstId)
{
    std::vector<BlackBoxCase> cases;
    const std::function<void(const char*, const std::function<void()>&)> add =
        [&cases, firstId](const char* name, const std::function<void()>& run)
    {
        BlackBoxCase item = {firstId + static_cast<int32_t>(cases.size()), name,
            "explicit failure; no false success, recursive wait, or retained ownership", false, 30, [run]()
        {
            run();
            bbCheck(TestTask::errors == 0 && TestTask::live == 0 && CTaskThreadManager::instance().count() == 0,
                "regression left task error/ownership/worker");
        }};
        cases.push_back(item);
    };
#ifndef CTTM_TEST_DLL
    add("manager-traversal-exhaustion", ManagerTraversalExhaustion);
#else
    add("manager-traversal-exhaustion", std::function<void()>());
    cases.back().unavailableReason = "DLL intentionally exports no white-box access; covered in static build";
#endif
    add("nested-stop-self-wait", NestedStopCallbacks);
    add("partial-pool-init-rollback", InitializationRollback);
    add("singleton-after-prewarm", SingletonAfterPrewarm);
    add("property-identifiers", PropertyIdentifiers);
    add("concurrent-redo-ordering", ConcurrentRedoOrdering);
    add("cached-priority-fallback", CachedPriorityFallback);
#ifndef CTTM_TEST_DLL
    add("split-counters-wrap", SplitCountersWrap);
#else
    add("split-counters-wrap", std::function<void()>());
    cases.back().unavailableReason = "DLL intentionally exports no white-box access; covered in static build";
#endif
    add("concurrent-bounded-statistics", ConcurrentBoundedStatistics);
    return cases;
}

void taskPrefetchedPriorityTest()
{
    std::shared_ptr<RedoState> state = std::make_shared<RedoState>();
    std::atomic<bool> gateEntered(false), gateReleased(false);
    std::vector<std::shared_ptr<TestTask>> followers;
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    try
    {
        // 先固定队列输入；原任务开始时，消费者已有机会预读同级前缀。
        bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&gateEntered, &gateReleased](TestTask& task)
        {
            gateEntered = true;
            bbWait([&gateReleased, &task]()
            {
                return gateReleased.load() || task.stopped.load();
            }, "prefetch gate release");
        }),
                          16)
                    .m_committed,
            "prefetch gate rejected");
        bbWait([&gateEntered]()
        {
            return gateEntered.load();
        }, "prefetch gate entered");
        bbCheck(worker->postTask(std::make_shared<RedoTask>(state, 0, true, true), 2).m_committed, "prefetch original rejected");
        for (int32_t i = 0; i < 200; ++i)
        {
            // 每项独立任务保留属性停止计数，执行时仅追加独立顺序账本。
            std::shared_ptr<TestTask> task = std::make_shared<TestTask>(77, [state, i](TestTask&)
            {
                state->ledger.add(300 + i);
            });
            followers.push_back(task);
            bbCheck(worker->postTask(task, 2).m_committed, "prefetch follower rejected");
        }
        gateReleased = true;
        bbWait([state]()
        {
            return state->entered.load();
        }, "prefetched original entered");
        bbCheck(worker->getWaitTaskCount() == 200, "prefetch removed logical pending tasks");
        bbCheck(worker->stopTask(77, 2).m_status == CTaskThreadOk, "prefetched property stop failed");
        for (size_t i = 0; i < followers.size(); ++i)
        {
            bbCheck(followers[i]->stops == 1, "property stop missed a prefetched task");
        }
        bbCheck(worker->postTask(std::make_shared<TestTask>(9, [state](TestTask&)
        {
            state->ledger.add(900);
        }),
                          3)
                    .m_committed,
            "late higher priority rejected");
        bbCheck(worker->getWaitTaskCount() == 201 && state->stops > 0, "late preemption/count changed");
        state->release = true;
        workers.drain();
        std::vector<int> expected;
        expected.push_back(100);
        expected.push_back(900);
        expected.push_back(101);
        for (int32_t i = 0; i < 200; ++i)
        {
            expected.push_back(300 + i);
        }
        bbCheck(state->ledger.snapshot() == expected, "prefetch bypassed priority/redo/FIFO");
    }
    catch (...)
    {
        gateReleased = true;
        state->release = true;
        throw;
    }
}

void taskPriorityWakeTest()
{
    std::atomic<int32_t> done(0);
    TestWorkers workers;
    CTaskThreadConfig config;
    config.m_idleCheckMilliseconds = 60000;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "priority wake initialization failed");
    workers.ids.push_back(id);
    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    int32_t expected = 0;
    for (int32_t wave = 0; wave < 200; ++wave)
    {
        bbSleep(1);
        const int32_t count = wave % 2 == 0 ? 1 : 16;
        for (int32_t i = 0; i < count; ++i)
        {
            const int32_t level = 2 + (wave + i) % 15;
            bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&done](TestTask&)
            {
                ++done;
            }),
                              level)
                        .m_committed,
                "priority wake task rejected");
        }
        expected += count;
        bbWait([&done, expected]()
        {
            return done.load() == expected;
        }, "priority wake without heartbeat", 500);
    }
    workers.drain();
    bbCheck(done == 1700, "priority wake completion count changed");
}

void taskRedoFactoryOrderTest()
{
    std::atomic<int32_t> factories(0), runs(0);
    {
        TestWorkers workers;
        const std::shared_ptr<CTaskThread> worker = workers.add();
        std::shared_ptr<TestFactoryTask> task(new TestFactoryTask(1, [&runs](TestTask&)
        {
            ++runs;
        }));
        task->clone([&factories]() -> CTask*
        {
            ++factories;
            return nullptr;
        });
        for (size_t i = 0; i < 1000; ++i)
        {
            // 普通投递内部处理默认小池的临时竞争；已接纳调用绝不重复投递。
            bbWait([worker, task]()
            {
                const CTaskThreadResult posted = worker->postTask(task);
                bbCheck(posted.m_status == CTaskThreadOk && posted.m_committed,
                    "unpreempted factory task rejected: " + bbNumber(posted.m_status));
                return posted.m_committed;
            }, "unpreempted factory admission");
        }
        workers.drain();
        bbCheck(runs == 1000 && factories == 0, "unpreempted execution eagerly created a backup");
        task->clone(std::function<CTask*()>());
        bbCheck(workers.add()->postTask(task).m_committed, "factory-free task rejected");
        workers.drain();
        bbCheck(factories == 0 && runs == 1001, "clearing factory retained a callback or lost task");
    }
    TaskLedger ledger;
    std::atomic<bool> entered(false), release(false), returned(false);
    std::atomic<int32_t> selfWait(-1);
    std::thread::id executionThread;
    int32_t parameter = 7;
    TestWorkers workers;
    const std::shared_ptr<CTaskThread> worker = workers.add();
    std::shared_ptr<TestFactoryTask> original(new TestFactoryTask(9317,
        [&ledger, &entered, &release, &returned, &executionThread, &parameter](TestTask&)
    {
        executionThread = std::this_thread::get_id();
        ledger.add(100);
        parameter = 99;
        entered = true;
        bbWait([&release]()
        {
            return release.load();
        }, "lazy factory original release");
        returned = true;
    }));
    original->clone([&ledger, &factories, &returned, &executionThread, &selfWait, worker, parameter]() -> CTask*
    {
        bbCheck(returned.load() && executionThread == std::this_thread::get_id(), "factory ran before return or on a producer");
        ++factories;
        selfWait = worker->waitForEnd().m_status;
        bbCheck(worker->stopTask(9317).m_status == CTaskThreadOk, "factory property stop reentry failed");
        return new TestTask(777, [&ledger, parameter](TestTask&)
        {
            ledger.add(parameter);
        });
    });
    try
    {
        bbCheck(worker->postTask(original).m_committed, "lazy original rejected");
        bbWait([&entered]()
        {
            return entered.load();
        }, "lazy original entered");
        bbCheck(factories == 0, "factory ran before preemption");
        bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&ledger](TestTask&)
        {
            ledger.add(200);
        })).m_committed,
            "same-level follower rejected");
        bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&ledger](TestTask&)
        {
            ledger.add(300);
        }),
                          3)
                    .m_committed &&
                worker->postTask(std::make_shared<TestTask>(1, [&ledger](TestTask&)
        {
            ledger.add(400);
        }),
                          4)
                    .m_committed,
            "factory preemption rejected");
        bbCheck(factories == 0 && original->stops == 2, "factory was eager or preemption notifications changed");
        release = true;
        workers.drain();
        const int expected[] = {100, 400, 300, 7, 200};
        bbCheck(ledger.snapshot() == std::vector<int>(expected, expected + 5) && factories == 1 &&
                selfWait == CTaskThreadSelfWait && original->stops == 2 && worker->statistics().m_errors == 0,
            "factory snapshot, priority, front position or callback guard changed");
    }
    catch (...)
    {
        release = true;
        throw;
    }
}

void taskRedoFactoryConcurrentTest()
{
    {
        std::shared_ptr<RedoState> state = std::make_shared<RedoState>();
        std::atomic<int32_t> callbacks(0), factories(0), errors(0);
        std::atomic<bool> releaseCallbacks(false);
        TestWorkers workers;
        const std::shared_ptr<CTaskThread> worker = workers.add();
        std::shared_ptr<TestConcurrentRedoTask> original(new TestConcurrentRedoTask(state, callbacks, releaseCallbacks));
        original->clone([state, &factories]() -> CTask*
        {
            ++factories;
            return new TestTask(1, [state](TestTask&)
            {
                state->ledger.add(999);
            });
        });
        std::vector<std::thread> producers;
        producers.reserve(8);
        try
        {
            bbCheck(worker->postTask(original).m_committed, "concurrent factory original rejected");
            bbWait([state]()
            {
                return state->entered.load();
            }, "concurrent factory original entered");
            for (int32_t i = 0; i < 8; ++i)
            {
                producers.push_back(std::thread([worker, state, i, &errors]()
                {
                    try
                    {
                        if (!worker->postTask(std::make_shared<TestTask>(1, [state, i](TestTask&)
                        {
                            state->ledger.add(200 + i);
                        }),
                                       2)
                                .m_committed)
                        {
                            ++errors;
                        }
                    }
                    catch (...)
                    {
                        ++errors;
                    }
                }));
            }
            bbWait([&callbacks]()
            {
                return callbacks == 8;
            }, "all factory preemptions entered");
            releaseCallbacks = true;
            for (size_t i = 0; i < producers.size(); ++i)
            {
                producers[i].join();
            }
            bbCheck(factories == 0 && errors == 0, "factory ran in concurrent preemption callbacks");
            state->release = true;
            workers.drain();
            const std::vector<int> values = state->ledger.snapshot();
            bbCheck(factories == 1 && values.size() == 10 && values.front() == 100 && values.back() == 999,
                "concurrent preemption duplicated factory result or changed redo priority");
        }
        catch (...)
        {
            releaseCallbacks = true;
            state->release = true;
            for (size_t i = 0; i < producers.size(); ++i)
            {
                if (producers[i].joinable())
                {
                    producers[i].join();
                }
            }
            throw;
        }
    }
    std::atomic<int32_t> factories(0), entered(-1), released(-1);
    TaskLedger ledger;
    TestWorkers workers;
    const std::shared_ptr<CTaskThread> worker = workers.add();
    const std::function<void(TestTask&)> initialBody = [&ledger, &entered, &released](TestTask&)
    {
        ledger.add(100);
        entered = 0;
        bbWait([&released]()
        {
            return released.load() >= 0;
        }, "factory generation zero release");
    };
    std::shared_ptr<TestFactoryTask> original(new TestFactoryTask(1, initialBody));
    original->clone([&ledger, &entered, &released, &factories]() -> CTask*
    {
        const int32_t generation = ++factories;
        return new TestFactoryTask(1, [&ledger, &entered, &released, generation](TestTask&)
        {
            ledger.add(100 + generation);
            entered = generation;
            if (generation < 2)
            {
                bbWait([&released, generation]()
                {
                    return released.load() >= generation;
                }, "factory next generation release");
            }
        });
    });
    try
    {
        bbCheck(worker->postTask(original).m_committed, "repeated factory original rejected");
        bbWait([&entered]()
        {
            return entered == 0;
        }, "factory generation zero entered");
        bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&ledger](TestTask&)
        {
            ledger.add(300);
        })).m_committed,
            "repeated factory follower rejected");
        for (int32_t generation = 0; generation < 2; ++generation)
        {
            bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&ledger, generation](TestTask&)
            {
                ledger.add(201 + generation);
            }),
                              2)
                        .m_committed,
                "repeated factory higher task rejected");
            released = generation;
            bbWait([&entered, generation]()
            {
                return entered.load() >= generation + 1;
            }, "inherited factory next generation");
        }
        workers.drain();
        const int expected[] = {100, 201, 101, 202, 102, 300};
        bbCheck(factories == 2 && ledger.snapshot() == std::vector<int>(expected, expected + 6),
            "inherited factory lost repeated redo or same-level front ordering");
    }
    catch (...)
    {
        released = INT32_MAX;
        throw;
    }
}

void taskRedoFactoryFailureTest()
{
    for (int32_t mode = 0; mode < 7; ++mode)
    {
        std::atomic<int32_t> factories(0), children(0), higher(0);
        std::atomic<bool> entered(false), release(false);
        TestWorkers workers;
        const std::shared_ptr<CTaskThread> worker = workers.add();
        std::shared_ptr<TestFactoryTask> original(new TestFactoryTask(1, [&entered, &release](TestTask&)
        {
            entered = true;
            bbWait([&release]()
            {
                return release.load();
            }, "factory failure original release");
        }, mode != 6));
        const std::weak_ptr<CTask> weakOriginal(original);
        original->clone([mode, worker, weakOriginal, &factories, &children]() -> CTask*
        {
            ++factories;
            if (mode == 0)
            {
                throw std::runtime_error("injected redo factory failure");
            }
            if (mode == 1)
            {
                return nullptr;
            }
            if (mode == 2)
            {
                return weakOriginal.lock().get();
            }
            if (mode == 3)
            {
                worker->requestShutdown(false);
            }
            if (mode == 5)
            {
                throw std::bad_alloc();
            }
            return new TestTask(1, [&children](TestTask&)
            {
                ++children;
            });
        });
        try
        {
            bbCheck(worker->postTask(original).m_committed, "failure factory original rejected");
            bbWait([&entered]()
            {
                return entered.load();
            }, "failure factory original entered");
            bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&higher](TestTask&)
            {
                ++higher;
            }),
                              2)
                        .m_committed,
                "failure factory high task rejected");
            if (mode == 4)
            {
                worker->requestShutdown(false);
            }
            release = true;
            workers.drain();
            const uint64_t expectedErrors = mode == 0 || mode == 2 || mode == 5 ? 1 : 0;
            bbCheck(factories == (mode == 4 || mode == 6 ? 0 : 1) && children == 0 &&
                    higher == (mode == 3 || mode == 4 ? 0 : 1) && worker->statistics().m_errors == expectedErrors &&
                    worker->statistics().m_outstanding == 0,
                "factory failure/close path lost cleanup or executed forbidden work");
        }
        catch (...)
        {
            release = true;
            throw;
        }
    }
}

void taskRedoFactoryResourceTest()
{
    std::atomic<int32_t> oldCalls(0), newCalls(0);
    bool failCopy = false;
    std::shared_ptr<CTask> configurable(new TestFactoryTask(1, [](TestTask&)
    {

    }));
    configurable->clone([&oldCalls]() -> CTask*
    {
        ++oldCalls;
        return nullptr;
    });
    std::function<CTask*()> replacement = TestFactoryCopyFailure(failCopy, newCalls, false);
    failCopy = true;
    const bool replacementSaved = configurable->clone(replacement);
    bool directThrew = false;
    const TestFactoryCopyFailure direct(failCopy, newCalls, false);
    try
    {
        configurable->clone(direct);
    }
    catch (...)
    {
        directThrew = true;
    }
    failCopy = false;
    bbCheck(!replacementSaved && directThrew && (*configurable->cloneFactory())() == nullptr && oldCalls == 1 && newCalls == 0,
        "failed factory copy replaced the prior configuration");
    bbCheck(configurable->clone(*configurable->cloneFactory()), "aliasing factory registration rejected");
    bbCheck((*configurable->cloneFactory())() == nullptr && oldCalls == 2,
        "aliasing factory registration destroyed its source");
    configurable.reset();
    {
        TestPropertyTask original(46);
        CTask& base = original;
        base.clone([&oldCalls]() -> CTask*
        {
            ++oldCalls;
            return nullptr;
        });
        TestPropertyTask copied(original), assigned(99);
        assigned = original;
        base.clone(std::function<CTask*()>());
        bbCheck(!base.cloneFactory() && copied.cloneFactory() && assigned.cloneFactory() &&
                copied.getPropertyId() == 46 && assigned.getPropertyId() == 46,
            "CTask copy/assignment compatibility or independent configuration changed");
        (*copied.cloneFactory())();
        (*assigned.cloneFactory())();
        bbCheck(oldCalls == 4, "copied factory lifetime did not survive source reconfiguration");
    }
    for (int32_t mode = 0; mode < 2; ++mode)
    {
        std::atomic<int32_t> factories(0), higher(0);
        std::atomic<bool> entered(false), release(false);
        failCopy = false;
        TestWorkers workers;
        CTaskThreadConfig config;
        if (mode == 1)
        {
            config.m_taskMap.initialCapacity = 2;
            config.m_taskMap.maximumCapacity = 2;
        }
        const uint32_t id = CTaskThreadManager::instance().init(config);
        bbCheck(id != 0, "factory resource worker initialization failed");
        workers.ids.push_back(id);
        const std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
        std::shared_ptr<CTask> original(new TestFactoryTask(1, [&entered, &release](TestTask&)
        {
            entered = true;
            bbWait([&release]()
            {
                return release.load();
            }, "factory resource gate release");
        }));
        original->clone(TestFactoryCopyFailure(failCopy, factories, mode == 0));
        try
        {
            bbCheck(worker->postTask(original).m_committed, "resource original rejected");
            bbWait([&entered]()
            {
                return entered.load();
            }, "resource original entered");
            bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&higher](TestTask&)
            {
                ++higher;
            }),
                              2)
                        .m_committed,
                "resource high task rejected before deferred creation");
            release = true;
            workers.drain();
            bbCheck(factories == 1 && higher == 1 && worker->statistics().m_errors == 1 &&
                    worker->statistics().m_outstanding == 0,
                "factory inheritance/capacity failure did not clean up");
        }
        catch (...)
        {
            release = true;
            throw;
        }
    }
}

void taskRedoFactoryAllocationTest()
{
    {
        std::atomic<int32_t> oldCalls(0);
        TestFactoryTask configured(1, [](TestTask&)
        {

        });
        configured.clone([&oldCalls]() -> CTask*
        {
            ++oldCalls;
            return nullptr;
        });
        const std::function<CTask*()> replacement = []() -> CTask*
        {
            return nullptr;
        };
        CTaskFaultAllocation::begin(1);
        const bool saved = configured.clone(replacement);
        CTaskFaultAllocation::end();
        bbCheck(!saved && CTaskFaultAllocation::triggered() && configured.cloneFactory(),
            "configuration allocation failure was hidden or removed prior factory");
        (*configured.cloneFactory())();
        bbCheck(oldCalls == 1, "configuration allocation failure installed new factory");
        CTaskFaultAllocation::begin(1);
        const bool cleared = configured.clone(std::function<CTask*()>());
        const uint64_t allocations = CTaskFaultAllocation::end();
        bbCheck(cleared && allocations == 0 && !configured.cloneFactory(), "clearing factory allocated or retained configuration");
    }
    std::atomic<int32_t> factories(0), children(0), higher(0);
    std::atomic<bool> entered(false), release(false), injected(false);
    TestWorkers workers;
    const std::shared_ptr<CTaskThread> worker = workers.add();
    std::shared_ptr<TestFactoryTask> original(new TestFactoryTask(1, [&entered, &release](TestTask&)
    {
        entered = true;
        bbWait([&release]()
        {
            return release.load();
        }, "control block allocation gate release");
    }));
    original->clone([&factories, &children]() -> CTask*
    {
        ++factories;
        CTask* created = new TestTask(1, [&children](TestTask&)
        {
            ++children;
        });
        // 工厂已成功创建对象；故障只作用于工作线程接管返回指针时的控制块申请。
        CTaskFaultAllocation::begin(1);
        return created;
    });
    try
    {
        bbCheck(worker->postTask(original).m_committed, "control block failure original rejected");
        bbWait([&entered]()
        {
            return entered.load();
        }, "control block failure original entered");
        bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&higher, &injected](TestTask&)
        {
            injected = CTaskFaultAllocation::triggered();
            CTaskFaultAllocation::end();
            ++higher;
        }),
                          2)
                    .m_committed,
            "control block failure high task rejected");
        release = true;
        workers.drain();
        bbCheck(injected && factories == 1 && children == 0 && higher == 1 &&
                worker->statistics().m_errors == 1 && worker->statistics().m_outstanding == 0 && TestTask::live == 1,
            "raw clone was leaked, executed, or swallowed allocation failure");
    }
    catch (...)
    {
        release = true;
        throw;
    }
}

void taskRedoFactoryLifecycleTest()
{
    for (int32_t discard = 0; discard <= 1; ++discard)
    {
        std::atomic<bool> entered(false), release(false), factoryEntered(false), factoryRelease(false);
        std::atomic<bool> childEntered(false), childRelease(false);
        std::atomic<int32_t> factories(0), childFactories(0), children(0), higher(0);
        TestWorkers workers;
        const std::shared_ptr<CTaskThread> worker = workers.add();
        std::shared_ptr<TestFactoryTask> original(new TestFactoryTask(1, [&entered, &release](TestTask&)
        {
            entered = true;
            bbWait([&release]()
            {
                return release.load();
            }, "lifecycle original release");
        }));
        original->clone([&factories, &factoryEntered, &factoryRelease, &childEntered, &childRelease,
                            &childFactories, &children]() -> CTask*
        {
            ++factories;
            factoryEntered = true;
            bbWait([&factoryRelease]()
            {
                return factoryRelease.load();
            }, "lifecycle factory release");
            std::unique_ptr<TestFactoryTask> child(new TestFactoryTask(2, [&childEntered, &childRelease, &children](TestTask&)
            {
                childEntered = true;
                bbWait([&childRelease]()
                {
                    return childRelease.load();
                }, "lifecycle child release");
                ++children;
            }));
            child->clone([&childFactories, &children]() -> CTask*
            {
                ++childFactories;
                return new TestTask(3, [&children](TestTask&)
                {
                    ++children;
                });
            });
            return child.release();
        });
        try
        {
            bbCheck(worker->postTask(original).m_committed, "lifecycle original rejected");
            bbWait([&entered]()
            {
                return entered.load();
            }, "lifecycle original entered");
            bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&higher](TestTask&)
            {
                ++higher;
            }),
                              2)
                        .m_committed,
                "lifecycle higher task rejected");
            release = true;
            bbWait([&factoryEntered]()
            {
                return factoryEntered.load();
            }, "lifecycle factory entered");
            if (discard != 0)
            {
                bbCheck(worker->requestShutdown(false).m_status == CTaskThreadOk, "in-flight factory shutdown rejected");
            }
            factoryRelease = true;
            if (discard == 0)
            {
                bbWait([&childEntered]()
                {
                    return childEntered.load();
                }, "lifecycle child entered");
                bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&higher](TestTask&)
                {
                    ++higher;
                }),
                                  2)
                            .m_committed,
                    "lifecycle second higher task rejected");
                childRelease = true;
            }
            workers.drain();
            bbCheck(factories == 1 && childFactories == (discard == 0 ? 1 : 0) &&
                    children == (discard == 0 ? 2 : 0) && higher == (discard == 0 ? 2 : 0) &&
                    worker->statistics().m_errors == 0 && worker->statistics().m_outstanding == 0 && TestTask::live == 1,
                "child factory was overwritten or shutdown committed/leaked deferred task");
        }
        catch (...)
        {
            release = true;
            factoryRelease = true;
            childRelease = true;
            throw;
        }
    }
}