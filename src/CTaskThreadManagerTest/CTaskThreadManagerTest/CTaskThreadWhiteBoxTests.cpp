#include "CTaskThreadWhiteBoxTests.h"
#include "CTaskThreadManagerTest.h"
#include "CTaskFaultAllocation.h"
#include "CTaskThreadTestAccess.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <thread>

TestPreparingTask::TestPreparingTask(std::atomic<bool>& entered, std::atomic<bool>& release,
    std::atomic<int32_t>& calls, std::atomic<int32_t>& stops) :
m_entered(entered),
m_release(release),
m_calls(calls),
m_stops(stops)
{

}

void TestPreparingTask::doTask()
{
    m_entered = true;
    bbWait([this]()
    {
        return m_release.load();
    }, "controlled task release");
    ++m_calls;
}

void TestPreparingTask::stopTask()
{
    ++m_stops;
}

/** 创建一个实际进入 doTask 的受控任务。
@param [in] worker 目标线程
@param [in,out] entered 开始通知
@param [in,out] release 结束条件
@return 任务强引用
*/
static std::shared_ptr<TestTask> WhiteGate(const std::shared_ptr<CTaskThread>& worker,
    std::atomic<bool>& entered, std::atomic<bool>& release)
{
    std::shared_ptr<TestTask> task(new TestTask(999, [&entered, &release](TestTask&)
    {
        entered = true;
        bbWait([&release]()
        {
            return release.load();
        }, "white-box gate release");
    }));
    bbCheck(worker->postTask(task, INT_MAX).m_committed, "white-box gate not accepted");
    bbWait([&entered]()
    {
        return entered.load();
    }, "white-box gate start");
    return task;
}

#ifndef CTTM_TEST_DLL
/** 验证任务编号不会回绕，也不消耗失败投递额度。
*/
static void TaskSequenceBoundary()
{
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    std::atomic<int> calls(0);
    std::shared_ptr<TestTask> task(new TestTask(1, [&calls](TestTask&)
    {
        ++calls;
    }));
    CTaskThreadTestAccess::setTaskSequence(*worker, UINT64_MAX - 1);
    bbCheck(worker->postTask(task).m_committed, "last nonzero task sequence rejected");
    const CTaskThreadResult overflow = worker->postTask(task);
    bbCheck(!overflow.m_committed && overflow.m_status == CTaskThreadSequenceExhausted, "task sequence wrapped");
    workers.drain();
    bbCheck(calls == 1 && worker->statistics().m_outstanding == 0, "sequence rejection leaked budget");
    bbCheck(CTaskThreadTestAccess::recordCount(*worker) == 0, "shutdown retained directory payloads");
}

/** 验证线程编号耗尽后不复用。
*/
static void ManagerSequenceBoundary()
{
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    CTaskThreadTestAccess::setManagerSequence(UINT32_MAX - 1);
    const uint32_t last = manager.init();
    bbCheck(last == UINT32_MAX, "last thread ID incorrect");
    bbCheck(manager.init() == 0, "thread ID wrapped");
    bbCheck(manager.uninit(last).m_status == CTaskThreadOk, "last thread failed to stop");
    bbCheck(manager.init() == 0 && manager.count() == 0, "exhausted thread ID reused after erase");
}

/** 生产者帮助摘除已关闭空桶，不依赖关闭者继续运行。
*/
static void HelpClosedBucket()
{
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    bbCheck(CTaskThreadTestAccess::closeIdleBucket(*worker, 1), "cannot close initial empty bucket");
    std::atomic<int> calls(0);
    bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&calls](TestTask&)
    {
        ++calls;
    })).m_committed,
        "producer could not help remove closed bucket");
    workers.drain();
    bbCheck(calls == 1, "replacement bucket lost task");
}

/** 已完成投递的迟到通知不能停止后来的低优先级任务。
*/
static void LatePreemption()
{
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    std::atomic<bool> entered(false), release(false);
    std::shared_ptr<TestTask> task(new TestTask(1, [&entered, &release](TestTask&)
    {
        entered = true;
        bbWait([&release]()
        {
            return release.load();
        }, "late notice gate");
    }));
    worker->postTask(task, 1);
    bbWait([&entered]()
    {
        return entered.load();
    }, "late notice current");
    const CTaskThreadResult result = CTaskThreadTestAccess::latePreempt(*worker);
    const int stopped = task->stops.load();
    release = true;
    workers.drain();
    bbCheck(result.m_status == CTaskThreadOk && stopped == 0, "completed post preempted unrelated current task");
}

/** 外层投递持有最后一条保护时，回调重入识别自等待并支持后续恢复。
*/
void taskReentrantProtectionTest()
{
    CTaskThreadConfig config;
    config.m_taskMap.protectionRecordCount = 4;
    std::atomic<bool> running(false), releaseTask(false), releaseReaders(false);
    std::atomic<int> held(0), calls(0), callbacks(0);
    CTaskThreadResult nestedPost, nestedStop;
    TestWorkers consumers, readers;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "reentrant protection initialization");
    consumers.ids.push_back(id);
    const std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    const std::shared_ptr<TestTask> nested(new TestTask(2, [&calls](TestTask&)
    {
        ++calls;
    }));
    const std::shared_ptr<TestTask> low(new TestTask(1, [&](TestTask&)
    {
        running = true;
        bbWait([&releaseTask]()
        {
            return releaseTask.load();
        }, "reentrant task release");
    }));
    low->onStop = [&, worker, nested]()
    {
        ++callbacks;
        nestedPost = worker->postTask(nested);
        nestedStop = worker->stopAllTask();
        releaseTask = true;
    };
    bbCheck(worker->postTask(low).m_committed, "reentrant low task admission");
    bbWait([&running]()
    {
        return running.load();
    }, "reentrant task running");
    for (int i = 0; i < 3; ++i)
    {
        const std::shared_ptr<TestTask> readerTask(new TestTask(i, [&, worker](TestTask&)
        {
            const CTaskThreadResult heldResult = CTaskThreadTestAccess::holdDirectoryRead(*worker, [&]()
            {
                ++held;
                bbWait([&releaseReaders]()
                {
                    return releaseReaders.load();
                }, "reentrant reader release");
            });
            bbCheck(heldResult.m_status == CTaskThreadOk, "reentrant reader acquisition");
        }));
        readerTask->onStop = [&releaseReaders]()
        {
            releaseReaders = true;
        };
        bbCheck(readers.add()->postTask(readerTask).m_committed, "reentrant reader task admission");
    }
    bbWait([&held]()
    {
        return held == 3;
    }, "three readers protected");
    const CTaskThreadResult outer = worker->postTask(std::make_shared<TestTask>(3, [&calls](TestTask&)
    {
        ++calls;
    }), 2);
    releaseReaders = true;
    readers.drain();
    const CTaskThreadResult recovery = worker->postTask(nested);
    releaseTask = true;
    consumers.drain();
    bbCheck(outer.m_committed && outer.m_status == CTaskThreadOk && callbacks == 1 &&
        nestedPost.m_status == CTaskThreadSelfWait && !nestedPost.m_committed &&
        nestedStop.m_status == CTaskThreadSelfWait && !nestedStop.m_committed &&
        recovery.m_committed && calls == 2, "reentrant exhaustion blocked or duplicated work");
}

/** 固定保护额度用满时返回容量错误，恢复后可再次投递且不重复接纳 */
static void ProtectionExhaustion()
{
    CTaskThreadConfig config;
    config.m_taskMap.protectionRecordCount = 4;
    TestWorkers consumers, readers;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "small protection config failed");
    consumers.ids.push_back(id);
    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    std::atomic<bool> entered(false), release(false), releaseReaders(false);
    std::atomic<int> held(0), calls(0);
    WhiteGate(worker, entered, release);
    for (int i = 0; i < 4; ++i)
    {
        readers.add()->postTask(std::make_shared<TestTask>(i, [worker, &held, &releaseReaders](TestTask&)
        {
            const CTaskThreadResult result = CTaskThreadTestAccess::holdDirectoryRead(*worker, [&held, &releaseReaders]()
            {
                ++held;
                bbWait([&releaseReaders]()
                {
                    return releaseReaders.load();
                }, "release protected reader");
            });
            bbCheck(result.m_status == CTaskThreadOk, "protected reader failed");
        }));
    }
    bbWait([&held]()
    {
        return held == 4;
    }, "all protection records held");
    std::shared_ptr<TestTask> task(new TestTask(1, [&calls](TestTask&)
    {
        ++calls;
    }));
    const uint64_t started = bbNow();
    const bool tried = worker->tryPostTask(task);
    const CTaskThreadResult triedResult = worker->tryPostTaskResult(task);
    const uint64_t elapsed = bbNow() - started;
    CTaskThreadResult post, stop;
    std::atomic<int> startedCalls(0), finishedCalls(0);
    std::thread poster([&]()
    {
        ++startedCalls;
        post = worker->postTask(task);
        ++finishedCalls;
    });
    std::thread stopper([&]()
    {
        ++startedCalls;
        stop = worker->stopCurTask();
        ++finishedCalls;
    });
    while (startedCalls != 2)
    {
        std::this_thread::yield();
    }
    const uint64_t deadline = bbNow() + 1000;
    while (finishedCalls != 2 && bbNow() < deadline)
    {
        std::this_thread::yield();
    }
    const bool returnedEarly = finishedCalls == 2;
    releaseReaders = true;
    poster.join();
    stopper.join();
    readers.drain();
    const CTaskThreadResult recovered = worker->postTask(task);
    release = true;
    consumers.drain();
    bbCheck(!tried && elapsed < 1000, "try post waited for protection or accepted without it");
    bbCheck(returnedEarly && post.m_status == CTaskThreadCapacity && !post.m_committed &&
            stop.m_status == CTaskThreadCapacity && !stop.m_committed &&
            triedResult.m_status == CTaskThreadCapacity && !triedResult.m_committed &&
            recovered.m_committed && calls == 1,
        "resource exhaustion became busy/retry, blocked or duplicated a recovered task");
    bbCheck(worker->statistics().m_rejected == 3, "failed public calls counted incorrectly");
}

#endif

void taskPostResultContractTest()
{
    for (int32_t physical = 0; physical < 2; ++physical)
    {
        CTaskThreadConfig config;
        std::atomic<int32_t> policies(0), calls(0);
        config.m_maximumTasks = physical ? -1 : 2;
        if (physical)
        {
            config.m_taskMap.initialCapacity = 2;
            config.m_taskMap.maximumCapacity = 2;
            // 满额时不应再调用扩容算法，也不能把容量错误改报策略异常。
            config.m_taskMap.growPolicy = [&policies](const LockFreeHashMapGrowContext&) -> size_t
            {
                ++policies;
                throw std::runtime_error("full capacity must precede growth policy");
            };
        }
        TestWorkers workers;
        const uint32_t id = CTaskThreadManager::instance().init(config);
        bbCheck(id != 0, "result contract initialization");
        workers.ids.push_back(id);
        const std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
        std::atomic<bool> entered(false), release(false);
        WhiteGate(worker, entered, release);
        const std::shared_ptr<TestTask> task(new TestTask(11, [&calls](TestTask&)
        {
            ++calls;
        }));
        const CTaskThreadResult accepted = worker->postTask(task);
        const CTaskThreadResult full = worker->postTask(task);
        const CTaskThreadResult tried = worker->tryPostTaskResult(task);
        const bool triedBool = worker->tryPostTask(task);
        const CTaskThreadResult invalid = worker->postTask(std::shared_ptr<CTask>());
        const CTaskThreadResult invalidTry = worker->tryPostTaskResult(task, 0);
        release = true;
        workers.drain();
        const CTaskThreadResult closed = worker->postTask(task);
        const CTaskThreadResult closedTry = worker->tryPostTaskResult(task);
        bbCheck(accepted.m_committed && accepted.m_status == CTaskThreadOk &&
            full.m_status == CTaskThreadCapacity && !full.m_committed &&
            tried.m_status == CTaskThreadCapacity && !tried.m_committed && !triedBool &&
            invalid.m_status == CTaskThreadInvalid && invalidTry.m_status == CTaskThreadInvalid &&
            closed.m_status == CTaskThreadClosed && closedTry.m_status == CTaskThreadClosed &&
            calls == 1 && policies == 0, "capacity/try/closed contract or exact delivery failed");
    }
}

void taskPostContentionContractTest()
{
    CTaskThreadConfig config;
    config.m_taskMap.initialCapacity = 131072;
    config.m_taskMap.protectionRecordCount = 128;
    config.m_queue.initialCapacity = 8192;
    config.m_queue.maximumProducers = 128;
    TestWorkers workers;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "contention contract initialization");
    workers.ids.push_back(id);
    const std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    std::atomic<bool> start(false);
    std::atomic<int32_t> completed(0), failures(0);
    const std::shared_ptr<TestTask> task(new TestTask(12, [&completed](TestTask&)
    {
        ++completed;
    }));
    std::vector<std::thread> producers;
    producers.reserve(32);
    try
    {
        for (int32_t i = 0; i < 32; ++i)
        {
            producers.push_back(std::thread([&, i]()
            {
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                for (int32_t j = 0; j < 2000; ++j)
                {
                    const CTaskThreadResult result = worker->postTask(task, 1 + (i + j) % 10);
                    if (!result.m_committed || result.m_status != CTaskThreadOk)
                    {
                        ++failures;
                    }
                }
            }));
        }
    }
    catch (...)
    {
        start = true;
        for (size_t i = 0; i < producers.size(); ++i)
        {
            producers[i].join();
        }
        throw;
    }
    start = true;
    for (size_t i = 0; i < producers.size(); ++i)
    {
        producers[i].join();
    }
    workers.drain();
    bbCheck(failures == 0 && completed == 64000 && worker->statistics().m_outstanding == 0,
        "ordinary post exposed contention, threw or lost/duplicated work");
}

/** 逐个真实初始化分配位置注入一次失败，验证回滚和重试。
*/
static void InitializeAllocationFailures()
{
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    manager.uninit(manager.init());
    CTaskThreadConfig config;
    uint32_t id = 0;
    CTaskFaultAllocation::begin(UINT64_MAX);
    const CTaskThreadResult measured = manager.initialize(config, id);
    const uint64_t allocations = CTaskFaultAllocation::end();
    bbCheck(measured.m_committed && allocations != 0, "cannot measure initialization allocations");
    manager.uninit(id);
    uint64_t triggered = 0;
    for (uint64_t point = 1; point <= allocations; ++point)
    {
        id = 0;
        CTaskFaultAllocation::begin(point);
        const CTaskThreadResult result = manager.initialize(config, id);
        CTaskFaultAllocation::end();
        triggered += CTaskFaultAllocation::triggered() ? 1 : 0;
        bbCheck((id != 0) == result.m_committed, "partial initialization advertised wrong ownership");
        if (id != 0)
        {
            bbCheck(manager.uninit(id).m_status == CTaskThreadOk, "recovered initialization cannot stop");
        }
        bbCheck(manager.count() == 0, "failed initialization left registered worker");
    }
    const uint32_t recovered = manager.init(config);
    bbCheck(recovered != 0, "normal initialization did not recover");
    manager.uninit(recovered);
    std::cout << "allocation_positions=" << allocations << " triggered=" << triggered << '\n';
    bbCheck(triggered > 10, "allocation sweep did not exercise actual sites");
}

/** 在一次投递的真实 new 路径注入错误。
@param [in] point 失败位置，UINT64_MAX 只计数
@param [in] level 使用已有或新建优先级
@return 实际申请次数
*/
static uint64_t PostAllocationTrial(uint64_t point, int32_t level)
{
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    std::atomic<bool> entered(false), release(false);
    std::atomic<int> calls(0);
    WhiteGate(worker, entered, release);
    std::shared_ptr<TestTask> task(new TestTask(1, [&calls](TestTask&)
    {
        ++calls;
    }));
    CTaskFaultAllocation::begin(point);
    const CTaskThreadResult result = worker->postTask(task, level);
    const uint64_t allocations = CTaskFaultAllocation::end();
    const CTaskThreadResult recovery = worker->postTask(task, level);
    release = true;
    workers.drain();
    bbCheck(recovery.m_committed, "post did not recover after one allocation failure");
    bbCheck(calls == (result.m_committed ? 2 : 1), "allocation failure lost or duplicated an accepted task");
    bbCheck(worker->statistics().m_outstanding == 0 && worker->getWaitTaskCount() == 0, "failure leaked task quota");
    return allocations;
}

/** 扫描热队列或冷优先级创建路径。
@param [in] level 待测优先级
*/
static void PostAllocationFailures(int32_t level)
{
    const uint64_t count = PostAllocationTrial(UINT64_MAX, level);
    for (uint64_t point = 1; point <= count; ++point)
    {
        PostAllocationTrial(point, level);
    }
    std::cout << "post_allocation_positions=" << count << " level=" << level << '\n';
    if (level == 1 && count == 0)
    {
        bbCheck(PostAllocationTrial(1, level) == 0, "preallocated hot post unexpectedly allocated");
    }
    else
    {
        bbCheck(count != 0, "post allocation injection did not run");
    }
}

/** 暂停高优先级的分配，随后拒绝发布；未接纳预留不能抢占低优先级。
*/
static void UnpublishedPriority()
{
    std::atomic<bool> pauseGrowth(false), growEntered(false), releaseGrow(false), taskEntered(false), releaseTask(false);
    std::atomic<int32_t> calls(0), stops(0), highCalls(0), highStatus(-1);
    CTaskThreadConfig config;
    config.m_queue.initialCapacity = 1;
    config.m_queueGrow = [&](const LockFreeQueueGrowContext&) -> size_t
    {
        if (pauseGrowth.load())
        {
            growEntered = true;
            bbWait([&releaseGrow]()
            {
                return releaseGrow.load();
            }, "release paused growth policy");
            return 0;
        }
        return 64;
    };
    TestWorkers consumers, producers;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "grow policy config failed");
    consumers.ids.push_back(id);
    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    worker->postTask(std::make_shared<TestPreparingTask>(taskEntered, releaseTask, calls, stops), 1);
    bbWait([&taskEntered]()
    {
        return taskEntered.load();
    }, "consumer paused in doTask");
    pauseGrowth = true;
    producers.add()->postTask(std::make_shared<TestTask>(2, [worker, &highCalls, &highStatus](TestTask&)
    {
        const CTaskThreadResult result = worker->postTask(std::make_shared<TestTask>(3, [&highCalls](TestTask&)
        {
            ++highCalls;
        }),
            2);
        highStatus = result.m_status;
        bbCheck(!result.m_committed, "denied growth unexpectedly committed");
    }));
    bbWait([&growEntered]()
    {
        return growEntered.load();
    }, "producer paused before FIFO publication");
    releaseTask = true;
    bbWait([&calls]()
    {
        return calls == 1;
    }, "consumer progresses past unpublished higher priority");
    const int earlyStops = stops.load();
    releaseGrow = true;
    producers.drain();
    consumers.drain();
    bbCheck(earlyStops == 0 && stops == 0 && highCalls == 0 && highStatus == CTaskThreadPolicyFailure,
        "unpublished or rejected priority preempted current task");
}

/** 有限优先级池可回收缓存空桶，再接纳新的级别。
*/
static void BoundedPriorityReuse()
{
    CTaskThreadConfig config;
    config.m_priorityMap.nodes.initialCapacity = 2;
    config.m_priorityMap.nodes.maximumCapacity = 2;
    config.m_priorityMap.values.initialCapacity = 2;
    config.m_priorityMap.values.maximumCapacity = 2;
    TestWorkers workers;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "bounded priority map initialize failed");
    workers.ids.push_back(id);
    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    std::atomic<int> calls(0);
    for (int32_t level = 1; level <= 20; ++level)
    {
        std::shared_ptr<TestTask> task(new TestTask(1, [&calls](TestTask&)
        {
            ++calls;
        }));
        bbWait([worker, task, level]()
        {
            const CTaskThreadResult result = worker->postTask(task, level);
            bbCheck(result.m_committed || result.m_status == CTaskThreadCapacity,
                "unexpected bounded-map failure");
            return result.m_committed;
        }, "reclaim bounded priority pool");
        bbWait([&calls, level]()
        {
            return calls == level;
        }, "bounded priority task executed");
#ifndef CTTM_TEST_DLL
        bbCheck(CTaskThreadTestAccess::bucketCount(*worker) <= 2, "bounded priority pool exceeded configured live keys");
#endif
    }
    workers.drain();
    bbCheck(calls == 20, "priority reuse lost tasks");
}

std::vector<BlackBoxCase> taskThreadWhiteBoxCases(int32_t firstId)
{
    std::vector<BlackBoxCase> cases;
    const std::function<void(const char*, const char*, bool, const std::function<void()>&)> add =
        [&cases, firstId](const char* name, const char* expectation, bool stress, const std::function<void()>& run)
    {
        BlackBoxCase item = {firstId + static_cast<int32_t>(cases.size()), name, expectation, stress, 120, [run]()
        {
            run();
            bbCheck(TestTask::errors == 0 && TestTask::live == 0, "white-box fixture error or retained task");
            bbCheck(CTaskThreadManager::instance().count() == 0, "white-box case retained managed threads");
        }};
        cases.push_back(item);
    };
#ifndef CTTM_TEST_DLL
    add("white-task-sequence", "UINT64_MAX boundary rejects without wrap/quota leak", false, TaskSequenceBoundary);
    add("white-manager-sequence", "UINT32_MAX ID never reused", false, ManagerSequenceBoundary);
    add("white-help-closed-bucket", "producer helps remove idle closed bucket", false, HelpClosedBucket);
    add("white-late-preemption", "completed post cannot stop later unrelated task", false, LatePreemption);
    add("white-protection-exhaustion", "fixed protection budget returns capacity; recovery accepts exactly once", false, ProtectionExhaustion);
#else
    const char* unavailableNames[] = {"white-task-sequence", "white-manager-sequence",
        "white-help-closed-bucket", "white-late-preemption", "white-protection-exhaustion"};
    for (size_t i = 0; i < sizeof(unavailableNames) / sizeof(unavailableNames[0]); ++i)
    {
        add(unavailableNames[i], "internal-state test runs against the production static library", false, std::function<void()>());
        cases.back().unavailableReason = "DLL intentionally exports no white-box access; covered in static build";
    }
#endif
    add("fault-init-allocation-sweep", "every measured init allocation site; rollback and recovery", true, InitializeAllocationFailures);
    add("fault-hot-post-allocation-sweep", "existing FIFO allocation failures; exact accepted count", true, []()
    {
        PostAllocationFailures(1);
    });
    add("fault-cold-post-allocation-sweep", "new priority creation failures; quota/ref recovery", true, []()
    {
        PostAllocationFailures(9);
    });
    add("white-unpublished-priority", "pause before publication then reject; no false preemption", false, UnpublishedPriority);
    add("white-bounded-priority-reuse", "2-key budget reused across 20 priorities", false, BoundedPriorityReuse);
    return cases;
}