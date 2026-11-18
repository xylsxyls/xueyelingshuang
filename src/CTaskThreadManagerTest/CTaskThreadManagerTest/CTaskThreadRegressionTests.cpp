#include "CTaskThreadRegressionTests.h"
#include "CTaskThreadManagerTest.h"
#include "CTaskThreadTestAccess.h"
#include "CTaskFaultAllocation.h"
#include <algorithm>
#include <thread>
#include <stdexcept>

TestStopFailureTask::TestStopFailureTask(std::atomic<bool>& entered, std::atomic<bool>& notified,
    std::atomic<bool>& release) :
m_entered(&entered),
m_notified(&notified),
m_release(&release)
{

}

void TestStopFailureTask::doTask()
{
    m_entered->store(true, std::memory_order_release);
    bbWait([this]() { return m_release->load(std::memory_order_acquire); }, "throwing stop task release");
}

void TestStopFailureTask::stopTask()
{
    m_notified->store(true, std::memory_order_release);
    throw std::runtime_error("intentional stop notification failure");
}

/** 验证线程池丢弃关闭在停止回调异常后仍等待工作线程退出
*/
static void PoolStopFailureJoin()
{
    std::atomic<bool> entered(false), notified(false), release(false), returned(false);
    CTaskThreadResult result;
    CTaskThreadPool pool;
    CTaskThreadPoolConfig config;
    config.m_initialThreads = 1;
    config.m_maximumThreads = 1;
    bbCheck(pool.initialize(config).m_status == CTaskThreadOk, "stop failure pool initialize");
    std::vector<uint32_t> ids;
    bbCheck(pool.threadIds(ids).m_status == CTaskThreadOk && ids.size() == 1, "stop failure pool worker");
    const std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(ids[0]);
    bbCheck(worker != nullptr, "stop failure pool worker reference");
    std::thread closer;
    bool returnedEarly = false;
    bool exited = false;
    try
    {
        TestPost(worker, std::make_shared<TestStopFailureTask>(entered, notified, release));
        bbWait([&]() { return entered.load(std::memory_order_acquire); }, "pool stop failure task entered");
        closer = std::thread([&]()
        {
            result = pool.shutdown(false);
            returned.store(true, std::memory_order_release);
        });
        bbWait([&]() { return notified.load(std::memory_order_acquire); }, "pool stop failure notification");
        const uint64_t deadline = bbNow() + 100;
        while (!returned.load(std::memory_order_acquire) && bbNow() < deadline)
        {
            bbSleep(1);
        }
        returnedEarly = returned.load(std::memory_order_acquire);
        release.store(true, std::memory_order_release);
        closer.join();
        exited = worker->statistics().m_exited;
    }
    catch (...)
    {
        release.store(true, std::memory_order_release);
        if (closer.joinable())
        {
            closer.join();
        }
        pool.shutdown(true);
        throw;
    }
    const CTaskThreadResult cleanup = pool.shutdown(true);
    bbCheck(!returnedEarly && exited && result.m_status == CTaskThreadCallbackFailure &&
        result.m_committed && cleanup.m_status == CTaskThreadOk &&
        worker->statistics().m_outstanding == 0 && CTaskThreadManager::instance().count() == 0,
        "pool stop callback failure skipped join, lost diagnosis or leaked registration");
}

void taskStopFailureJoinTest()
{
    std::atomic<bool> entered(false), notified(false), release(false), returned(false);
    CTaskThreadResult result;
    TestWorkers workers;
    const std::shared_ptr<CTaskThread> worker = workers.add();
    std::thread closer;
    bool returnedEarly = false;
    bool exited = false;
    try
    {
        TestPost(worker, std::make_shared<TestStopFailureTask>(entered, notified, release));
        bbWait([&]() { return entered.load(std::memory_order_acquire); }, "throwing stop task entered");
        closer = std::thread([&]()
        {
            result = CTaskThreadManager::instance().uninit(worker->threadId());
            returned.store(true, std::memory_order_release);
        });
        bbWait([&]() { return notified.load(std::memory_order_acquire); }, "throwing stop notification entered");
        const uint64_t deadline = bbNow() + 100;
        while (!returned.load(std::memory_order_acquire) && bbNow() < deadline)
        {
            bbSleep(1);
        }
        returnedEarly = returned.load(std::memory_order_acquire);
        release.store(true, std::memory_order_release);
        closer.join();
        exited = worker->statistics().m_exited;
    }
    catch (...)
    {
        release.store(true, std::memory_order_release);
        if (closer.joinable())
        {
            closer.join();
        }
        workers.drain();
        throw;
    }
    // 失败通知保留诊断，已 join 的线程随后仍可完成注册表清理。
    workers.drain();
    bbCheck(!returnedEarly && exited && result.m_status == CTaskThreadCallbackFailure &&
        result.m_committed && worker->statistics().m_errors == 1 &&
        worker->statistics().m_outstanding == 0 && CTaskThreadManager::instance().count() == 0,
        "stop callback failure skipped join, lost diagnosis or leaked registration");
    PoolStopFailureJoin();
}

void taskPoolLowMemoryDestructionTest()
{
#ifndef CTTM_TEST_DLL
    std::atomic<int32_t> entered(0), stopped(0);
    std::atomic<bool> cancel(false);
    std::unique_ptr<CTaskThreadPool> pool(new CTaskThreadPool);
    CTaskThreadPoolConfig config;
    config.m_initialThreads = 2;
    config.m_maximumThreads = 2;
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    try
    {
        bbCheck(manager.count() == 0 && pool->initialize(config).m_status == CTaskThreadOk,
            "low memory destruction pool initialize");
        std::vector<uint32_t> ids;
        bbCheck(pool->threadIds(ids).m_status == CTaskThreadOk && ids.size() == 2,
            "low memory destruction member IDs");
        std::vector<std::shared_ptr<CTaskThread>> threads;
        for (size_t i = 0; i < ids.size(); ++i)
        {
            threads.push_back(manager.getThreadInterface(ids[i]));
            bbCheck(threads.back() != nullptr, "low memory destruction member reference");
            TestPost(threads.back(), std::make_shared<TestTask>(1, [&entered, &stopped, &cancel](TestTask& task)
            {
                ++entered;
                bbWait([&task, &cancel]() { return cancel.load() || task.m_stopped.load(); }, "low memory destruction stop");
                ++stopped;
                bbWait([&stopped, &cancel]() { return cancel.load() || stopped == 2; }, "low memory destruction notify all before join");
            }));
        }
        // 本用例没有其他创建者；白盒核对控制线程在所有成员之后取得的下一个单调编号。
        const uint32_t controllerId = (std::max)(ids[0], ids[1]) + 1;
        const std::shared_ptr<CTaskThread> controller = manager.getThreadInterface(controllerId);
        bbCheck(controller != nullptr && manager.count() == 3, "low memory destruction controller reference");
        bbWait([&]() { return entered == 2 && controller->getCurTaskLevel() == 1; },
            "low memory destruction active workers");
        CTaskFaultAllocation::beginPersistent();
        pool.reset();
        const bool triggered = CTaskFaultAllocation::triggered();
        const uint64_t failedRequests = CTaskFaultAllocation::end();
        bool exited = controller->statistics().m_exited;
        for (size_t i = 0; i < threads.size(); ++i)
        {
            exited = exited && threads[i]->statistics().m_exited && threads[i]->statistics().m_outstanding == 0 &&
                threads[i]->statistics().m_errors == 0;
        }
        const CTaskThreadResult cleanup = manager.uninitAll();
        bbCheck(triggered && failedRequests >= 2 && exited && cleanup.m_status == CTaskThreadOk && manager.count() == 0,
            "persistent low memory destruction released live state or leaked registration");
    }
    catch (...)
    {
        CTaskFaultAllocation::end();
        cancel.store(true);
        if (pool)
        {
            pool->shutdown(false);
        }
        manager.uninitAll();
        throw;
    }
#endif
}

void taskPoolProtectionDestructionTest()
{
#ifndef CTTM_TEST_DLL
    std::atomic<bool> entered(false), releaseReaders(false), cancel(false), destroying(false), destroyed(false);
    std::atomic<int32_t> held(0), readerErrors(0);
    std::unique_ptr<CTaskThreadPool> pool(new CTaskThreadPool);
    std::vector<std::thread> readers;
    readers.reserve(4);
    std::thread destroyer;
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    try
    {
        CTaskThreadPoolConfig config;
        config.m_initialThreads = 1;
        config.m_maximumThreads = 1;
        config.m_worker.m_taskMap.protectionRecordCount = 4;
        bbCheck(pool->initialize(config).m_status == CTaskThreadOk, "protected destruction initialize");
        std::vector<uint32_t> ids;
        bbCheck(pool->threadIds(ids).m_status == CTaskThreadOk && ids.size() == 1, "protected destruction member");
        const std::shared_ptr<CTaskThread> worker = manager.getThreadInterface(ids[0]);
        bbCheck(worker != nullptr, "protected destruction worker reference");
        TestPost(worker, std::make_shared<TestTask>(1, [&](TestTask& task)
        {
            entered.store(true);
            bbWait([&]() { return task.m_stopped.load() || cancel.load(); }, "protected destruction stop");
        }));
        bbWait([&]() { return entered.load(); }, "protected destruction running task");
        for (size_t i = 0; i < 4; ++i)
        {
            readers.push_back(std::thread([&, worker]()
            {
                try
                {
                    const CTaskThreadResult result = CTaskThreadTestAccess::holdDirectoryRead(*worker, [&]()
                    {
                        ++held;
                        bbWait([&]() { return releaseReaders.load(); }, "protected destruction release readers");
                    });
                    if (result.m_status != CTaskThreadOk)
                    {
                        ++readerErrors;
                    }
                }
                catch (...)
                {
                    ++readerErrors;
                }
            }));
        }
        bbWait([&]() { return held == 4; }, "protected destruction occupied all records");
        const CTaskThreadResult stopped = pool->shutdown(false);
        bbCheck(stopped.m_status == CTaskThreadCapacity && stopped.m_committed && !worker->statistics().m_exited,
            "protected destruction did not reach resource failure");
        destroyer = std::thread([&]()
        {
            destroying.store(true);
            pool.reset();
            destroyed.store(true);
        });
        bbWait([&]() { return destroying.load(); }, "protected destruction caller started");
        const uint64_t until = bbNow() + 100;
        bool returnedEarly = false;
        while (bbNow() < until)
        {
            returnedEarly = returnedEarly || destroyed.load();
            std::this_thread::yield();
        }
        releaseReaders.store(true);
        for (size_t i = 0; i < readers.size(); ++i)
        {
            readers[i].join();
        }
        destroyer.join();
        bbCheck(!returnedEarly && destroyed.load() && readerErrors == 0 && worker->statistics().m_exited &&
            worker->statistics().m_errors == 0 && worker->statistics().m_outstanding == 0 && manager.count() == 0,
            "pool destructor returned before protected worker stopped or retained resources");
    }
    catch (...)
    {
        releaseReaders.store(true);
        cancel.store(true);
        for (size_t i = 0; i < readers.size(); ++i)
        {
            if (readers[i].joinable())
            {
                readers[i].join();
            }
        }
        if (destroyer.joinable())
        {
            destroyer.join();
        }
        pool.reset();
        manager.uninitAll();
        throw;
    }
#endif
}

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
m_entered(&entered),
m_release(&release)
{

}

bool TestConcurrentRedoTask::reExecute()
{
    ++*m_entered;
    bbWait([this]()
    {
        return m_release->load();
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
    std::atomic<int> held(0), done(0);
    std::atomic<bool> release(false);
    TestWorkers workers;
    std::vector<std::shared_ptr<CTaskThread>> readers;
    for (int i = 0; i < 4; ++i)
    {
        readers.push_back(workers.add());
    }
    bbCheck(!CTaskThreadTestAccess::holdManagerRead([]()
    {
        throw std::runtime_error("intentional read fixture failure");
    }), "manager read fixture hid its callback failure");
    for (size_t i = 0; i < readers.size(); ++i)
    {
        bbCheck(readers[i]->postTaskResult(std::make_shared<TestTask>(1, [&held, &release, &done](TestTask&)
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
    std::atomic<int> entered(0), nestedStatus(-1), joinStatus(-1), stopCallbacks(0);
    std::atomic<bool> release(false);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> a = workers.add(), b = workers.add();
    const uint32_t idA = workers.m_ids[0];
    /** 通知任务已经进入，并等待外部门控释放
    @param [in,out] task 未使用的测试任务实例
    */
    const std::function<void(TestTask&)> body = [&entered, &release](TestTask&)
    {
        ++entered;
        bbWait([&release]()
        {
            return release.load();
        }, "nested stop release");
    };
    std::shared_ptr<TestTask> taskA(new TestTask(1, body)), taskB(new TestTask(2, body));
    taskA->m_onStop = [b, &stopCallbacks]()
    {
        ++stopCallbacks;
        b->stopCurTask();
    };
    taskB->m_onStop = [a, idA, &nestedStatus, &joinStatus, &stopCallbacks]()
    {
        ++stopCallbacks;
        nestedStatus = a->stopCurTask().m_status;
        joinStatus = CTaskThreadManager::instance().waitForEnd(idA).m_status;
    };
    TestPost(a, taskA);
    TestPost(b, taskB);
    bbWait([&entered]()
    {
        return entered == 2;
    }, "nested tasks entered");
    a->stopCurTask();
    release = true;
    workers.drain();
    taskA->m_onStop = std::function<void()>();
    taskB->m_onStop = std::function<void()>();
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
    std::atomic<int32_t> calls(0);
    TestWorkers workers;
    for (int32_t i = 0; i < 8; ++i)
    {
        std::shared_ptr<CTaskThread> worker = workers.add();
        bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [expected, &calls](TestTask&)
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
    std::atomic<bool> entered(false), release(false);
    std::atomic<int32_t> completed(0);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    std::shared_ptr<TestTask> gate(new TestTask(77, [&entered, &release](TestTask&)
    {
        entered = true;
        bbWait([&release]()
        {
            return release.load();
        }, "property gate release");
    }));
    bbCheck(worker->postTaskResult(gate, INT32_MAX).m_committed, "property gate rejected");
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
    bbCheck(worker->postTaskResult(first, 1).m_committed && worker->postTaskResult(second, 2).m_committed &&
            worker->postTaskResult(negative, 1).m_committed,
        "duplicate/negative property ID was rejected");
    bbCheck(worker->stopTask(0).m_status == CTaskThreadOk && first->m_stops == 1 && second->m_stops == 1 &&
            negative->m_stops == 0 && gate->m_stops == 0,
        "property ID zero did not select both matching tasks");
    bbCheck(worker->stopTask(-7).m_status == CTaskThreadOk && negative->m_stops == 1 &&
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
        bbCheck(worker->postTaskResult(std::make_shared<TestConcurrentRedoTask>(state, callbacks, releaseCallbacks), 1).m_committed,
            "concurrent redo original rejected");
        bbWait([state]()
        {
            return state->m_entered.load();
        }, "concurrent redo original entered");
        for (int32_t i = 0; i < 2; ++i)
        {
            bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [state, i](TestTask&)
            {
                state->m_ledger.add(300 + i);
            }),
                              1)
                        .m_committed,
                "same-level follower rejected");
        }
        for (int32_t i = 0; i < 8; ++i)
        {
            bbCheck(producers.add()->postTaskResult(std::make_shared<TestTask>(1, [worker, state, i](TestTask&)
            {
                bbCheck(worker->postTaskResult(std::make_shared<TestTask>(2, [state, i](TestTask&)
                {
                    state->m_ledger.add(200 + i);
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
        bbCheck(state->m_stops == 8 && state->m_ledger.snapshot() == std::vector<int>(1, 100),
            "preemption lost notification or ran a second task before original returned");
        state->m_release.store(true);
        consumers.drain();
        const std::vector<int> order = state->m_ledger.snapshot();
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
    std::atomic<bool> entered(false), release(false);
    TaskLedger ledger;
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&entered, &release](TestTask&)
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
        bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&ledger, i](TestTask&)
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
    std::shared_ptr<CTaskThread> worker = workers.adopt(id);
    bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&entered, &release](TestTask& task)
    {
        entered = true;
        bbWait([&release, &task]()
        {
            return release.load() || task.m_stopped.load();
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
    bbCheck(worker->postTaskResult(task).m_committed && worker->postTaskResult(task).m_committed, "counter wrap rejected allowed tasks");
    const CTaskThreadResult full = worker->tryPostTaskResult(task, 1, true, false);
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
    std::shared_ptr<CTaskThread> worker = consumers.adopt(id);
    for (int32_t i = 0; i < 4; ++i)
    {
        bbCheck(producers.add()->postTaskResult(std::make_shared<TestTask>(1, [worker, &accepted, &calls](TestTask&)
        {
            for (int32_t sequence = 0; sequence < 5000; ++sequence)
            {
                std::shared_ptr<TestTask> task(new TestTask(1, [&calls](TestTask&)
                {
                    ++calls;
                }));
                TestPost(worker, task);
                ++accepted;
                const CTaskThreadStatistics load = worker->statistics();
                bbCheck(load.m_outstanding <= 3 && load.m_queued <= 3, "statistics exceeded actual admission bound");
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
    /** 登记回归项并统一检查任务及线程无残留
    @param [in] name 稳定英文键
    @param [in] run 测试执行体
    */
    const std::function<void(const char*, const std::function<void()>&)> add =
        [&cases, firstId](const char* name, const std::function<void()>& run)
    {
        BlackBoxCase item = {firstId + static_cast<int32_t>(cases.size()), name,
            "explicit failure; no false success, recursive wait, or retained ownership", false, 30, [run]()
        {
            run();
            bbCheck(TestTask::m_errors == 0 && TestTask::m_live == 0 && CTaskThreadManager::instance().count() == 0,
                "regression left task error/ownership/worker");
        }};
        cases.push_back(item);
    };
#ifndef CTTM_TEST_DLL
    add("manager-traversal-exhaustion", ManagerTraversalExhaustion);
#else
    add("manager-traversal-exhaustion", std::function<void()>());
    cases.back().m_unavailableReason = "DLL intentionally exports no white-box access; covered in static build";
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
    cases.back().m_unavailableReason = "DLL intentionally exports no white-box access; covered in static build";
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
        bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&gateEntered, &gateReleased](TestTask& task)
        {
            gateEntered = true;
            bbWait([&gateReleased, &task]()
            {
                return gateReleased.load() || task.m_stopped.load();
            }, "prefetch gate release");
        }),
                          16)
                    .m_committed,
            "prefetch gate rejected");
        bbWait([&gateEntered]()
        {
            return gateEntered.load();
        }, "prefetch gate entered");
        bbCheck(worker->postTaskResult(std::make_shared<RedoTask>(state, 0, true, true), 2).m_committed, "prefetch original rejected");
        for (int32_t i = 0; i < 200; ++i)
        {
            // 每项独立任务保留属性停止计数，执行时仅追加独立顺序账本。
            std::shared_ptr<TestTask> task = std::make_shared<TestTask>(77, [state, i](TestTask&)
            {
                state->m_ledger.add(300 + i);
            });
            followers.push_back(task);
            bbCheck(worker->postTaskResult(task, 2).m_committed, "prefetch follower rejected");
        }
        gateReleased = true;
        bbWait([state]()
        {
            return state->m_entered.load();
        }, "prefetched original entered");
        bbCheck(worker->getWaitTaskCount() == 200, "prefetch removed logical pending tasks");
        bbCheck(worker->stopTask(77, 2).m_status == CTaskThreadOk, "prefetched property stop failed");
        for (size_t i = 0; i < followers.size(); ++i)
        {
            bbCheck(followers[i]->m_stops == 1, "property stop missed a prefetched task");
        }
        bbCheck(worker->postTaskResult(std::make_shared<TestTask>(9, [state](TestTask&)
        {
            state->m_ledger.add(900);
        }),
                          3)
                    .m_committed,
            "late higher priority rejected");
        bbCheck(worker->getWaitTaskCount() == 201 && state->m_stops > 0, "late preemption/count changed");
        state->m_release.store(true);
        workers.drain();
        std::vector<int> expected;
        expected.push_back(100);
        expected.push_back(900);
        expected.push_back(101);
        for (int32_t i = 0; i < 200; ++i)
        {
            expected.push_back(300 + i);
        }
        bbCheck(state->m_ledger.snapshot() == expected, "prefetch bypassed priority/redo/FIFO");
    }
    catch (...)
    {
        gateReleased = true;
        state->m_release.store(true);
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
    std::shared_ptr<CTaskThread> worker = workers.adopt(id);
    int32_t expected = 0;
    for (int32_t wave = 0; wave < 200; ++wave)
    {
        bbSleep(1);
        const int32_t count = wave % 2 == 0 ? 1 : 16;
        for (int32_t i = 0; i < count; ++i)
        {
            const int32_t level = 2 + (wave + i) % 15;
            TestPost(worker, std::make_shared<TestTask>(1, [&done](TestTask&)
            {
                ++done;
            }), level);
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
        bbCheck(task->clone([&factories]() -> CTask*
        {
            ++factories;
            return nullptr;
        }), "unpreempted factory registration failed");
        for (size_t i = 0; i < 1000; ++i)
        {
            // 普通投递内部处理竞争；测试不在外层重发同一项。
            TestPost(worker, task);
        }
        workers.drain();
        bbCheck(runs == 1000 && factories == 0, "unpreempted execution eagerly created a backup");
        bbCheck(task->clone(std::function<CTask*()>()), "clearing factory failed");
        TestPost(workers.add(), task);
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
        bbCheck(worker->postTaskResult(original).m_committed, "lazy original rejected");
        bbWait([&entered]()
        {
            return entered.load();
        }, "lazy original entered");
        bbCheck(factories == 0, "factory ran before preemption");
        bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&ledger](TestTask&)
        {
            ledger.add(200);
        })).m_committed,
            "same-level follower rejected");
        bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&ledger](TestTask&)
        {
            ledger.add(300);
        }),
                          3)
                    .m_committed &&
                worker->postTaskResult(std::make_shared<TestTask>(1, [&ledger](TestTask&)
        {
            ledger.add(400);
        }),
                          4)
                    .m_committed,
            "factory preemption rejected");
        bbCheck(factories == 0 && original->m_stops == 2, "factory was eager or preemption notifications changed");
        release = true;
        workers.drain();
        const int expected[] = {100, 400, 300, 7, 200};
        bbCheck(ledger.snapshot() == std::vector<int>(expected, expected + 5) && factories == 1 &&
                selfWait == CTaskThreadSelfWait && original->m_stops == 2 && worker->statistics().m_errors == 0,
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
                state->m_ledger.add(999);
            });
        });
        std::vector<std::thread> producers;
        producers.reserve(8);
        try
        {
            bbCheck(worker->postTaskResult(original).m_committed, "concurrent factory original rejected");
            bbWait([state]()
            {
                return state->m_entered.load();
            }, "concurrent factory original entered");
            for (int32_t i = 0; i < 8; ++i)
            {
                producers.push_back(std::thread([worker, state, i, &errors]()
                {
                    try
                    {
                        if (!worker->postTaskResult(std::make_shared<TestTask>(1, [state, i](TestTask&)
                        {
                            state->m_ledger.add(200 + i);
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
            state->m_release.store(true);
            workers.drain();
            const std::vector<int> values = state->m_ledger.snapshot();
            bbCheck(factories == 1 && values.size() == 10 && values.front() == 100 && values.back() == 999,
                "concurrent preemption duplicated factory result or changed redo priority");
        }
        catch (...)
        {
            releaseCallbacks = true;
            state->m_release.store(true);
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
    /** 记录首代执行并等待外部允许首代退出
    @param [in,out] task 未使用的测试任务实例
    */
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
        bbCheck(worker->postTaskResult(original).m_committed, "repeated factory original rejected");
        bbWait([&entered]()
        {
            return entered == 0;
        }, "factory generation zero entered");
        bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&ledger](TestTask&)
        {
            ledger.add(300);
        })).m_committed,
            "repeated factory follower rejected");
        for (int32_t generation = 0; generation < 2; ++generation)
        {
            bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&ledger, generation](TestTask&)
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
            bbCheck(worker->postTaskResult(original).m_committed, "failure factory original rejected");
            bbWait([&entered]()
            {
                return entered.load();
            }, "failure factory original entered");
            bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&higher](TestTask&)
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
        const std::shared_ptr<CTaskThread> worker = workers.adopt(id);
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
            bbCheck(worker->postTaskResult(original).m_committed, "resource original rejected");
            bbWait([&entered]()
            {
                return entered.load();
            }, "resource original entered");
            bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&higher](TestTask&)
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
        /** 提供无捕获替换工厂，供配置分配失败回归使用
        @return 空指针，不创建重做任务
        */
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
        bbCheck(worker->postTaskResult(original).m_committed, "control block failure original rejected");
        bbWait([&entered]()
        {
            return entered.load();
        }, "control block failure original entered");
        bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&higher, &injected](TestTask&)
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
                worker->statistics().m_errors == 1 && worker->statistics().m_outstanding == 0 && TestTask::m_live == 1,
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
            bbCheck(worker->postTaskResult(original).m_committed, "lifecycle original rejected");
            bbWait([&entered]()
            {
                return entered.load();
            }, "lifecycle original entered");
            bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&higher](TestTask&)
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
                bbCheck(worker->postTaskResult(std::make_shared<TestTask>(1, [&higher](TestTask&)
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
                    worker->statistics().m_errors == 0 && worker->statistics().m_outstanding == 0 && TestTask::m_live == 1,
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