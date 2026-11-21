#include "CTaskThreadModuleTests.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#ifndef CTTM_TEST_DLL
#include "CTaskThreadManager/CTaskThreadPriority.h"
#include "CTaskThreadManager/CTaskAdmission.h"
#include "CTaskThreadManager/CTaskPriorityBucket.h"
#include "CTaskThreadManager/CTaskResultAdapter.h"
#endif
#include <thread>
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#endif

void taskThreadModelsTest()
{
#ifndef CTTM_TEST_DLL
    CTaskPoolPolicy policy;
    CTaskPoolLoad load;
    load.m_initialThreads = 2;
    load.m_currentThreads = 4;
    load.m_runningThreads = 4;
    load.m_queuedTasks = 8;
    bbCheck(policy.evaluate(load).m_targetThreads == 6, "default proportional growth incorrect");
    load.m_runningThreads = 0;
    load.m_queuedTasks = 0;
    load.m_idleThreads = 4;
    bbCheck(policy.evaluate(load).m_targetThreads == 3 && policy.evaluate(load).m_shrinkIdleMilliseconds == 1000,
        "default dynamic shrink incorrect");
    load.m_currentThreads = 2;
    load.m_idleThreads = 2;
    bbCheck(policy.evaluate(load).m_targetThreads == 2, "policy shrinks below initial count");
    CTaskThreadConfig config;
    CTaskAdmission admission;
    bbCheck(admission.supported() && admission.enter() && admission.enter(), "admission must allow concurrent users");
    bbCheck(!admission.closeIfEmpty() && admission.active() == 2, "retired a bucket with producers");
    admission.close();
    bbCheck(!admission.enter() && admission.active() == 2, "close lost in-flight users");
    admission.leave();
    admission.leave();
    bbCheck(admission.active() == 0 && admission.closed(), "close/drain protocol failed");
    admission.reopen();
    bbCheck(admission.closeIfEmpty() && !admission.enter(), "idle retirement failed");
    admission.reopen();
    bbCheck(admission.enter(), "reopened admission did not accept");
    admission.close();
    admission.reopen();
    bbCheck(admission.active() == 1 && admission.enter() && admission.active() == 2,
        "reopen overwrote an outstanding admission");
    admission.leave();
    admission.leave();
    bbCheck(admission.active() == 0 && admission.closeIfEmpty(), "reopen cleanup underflowed admission");
    CTaskPriorityBucket bucket(7);
    bbCheck(bucket.initialize(config).m_status == CTaskThreadOk, "bucket configuration failed");
    bbCheck(bucket.m_level == 7 && bucket.m_queued == 0 && bucket.m_queue.empty(), "bucket not empty");
    bbCheck(bucket.m_writers.enter(), "cannot admit bucket producer");
    bucket.reserveQueued();
    bucket.reserveQueued();
    bucket.m_writers.leave();
    bbCheck(!bucket.m_writers.closeIfEmpty() && bucket.m_queued == 2, "queued reservations failed to protect bucket");
    bucket.releaseQueued();
    bbCheck(!bucket.m_writers.closeIfEmpty() && bucket.m_queued == 1, "partial rollback dropped bucket protection");
    bucket.releaseQueued();
    bbCheck(bucket.m_queued == 0 && bucket.m_writers.closeIfEmpty(), "final dequeue or rollback blocked retirement");
    bbCheck(CTaskResultAdapter::queue(LockFreeQueueCapacityLimit).m_status == CTaskThreadCapacity, "capacity conversion lost");
    bbCheck(CTaskResultAdapter::map(LOCK_FREE_MAP_COMMITTED).m_committed, "map commit conversion lost");
    bbCheck(CTaskResultAdapter::queue(LockFreeQueueResourceUnavailable).m_status == CTaskThreadResourceUnavailable &&
            !CTaskResultAdapter::queue(LockFreeQueueResourceUnavailable).m_committed,
        "temporary safe-reuse pressure was treated as an invalid configuration or committed task");
    bbCheck(CTaskResultAdapter::queue(LockFreeQueueBusy).m_status == CTaskThreadRetry &&
        CTaskResultAdapter::map(LOCK_FREE_MAP_CONFLICT).m_status == CTaskThreadRetry,
        "pure contention lost internal retry classification");
    bbCheck(CTaskResultAdapter::map(LOCK_FREE_MAP_RESERVED).m_status == CTaskThreadCapacity &&
        CTaskResultAdapter::map(LOCK_FREE_MAP_RECORD_LIMIT).m_status == CTaskThreadCapacity &&
        CTaskResultAdapter::queue(LockFreeQueueGrowthDeclined).m_status == CTaskThreadCapacity,
        "bounded resource exhaustion was reported as contention");
    bbCheck(config.m_maximumTasks == -1 && config.m_queue.growThreshold == 0, "incorrect unlimited/on-demand defaults");
    bbCheck(config.m_priorityMap.nodes.maximumCapacity == -1 && config.m_queue.maximumProducers > 1, "invalid map/queue defaults");
    bbCheck(config.m_systemPriority == CTaskPriorityInherited, "default changes inherited system priority");
    CTaskThreadStatistics statistics;
    bbCheck(statistics.m_queued == 0 && !statistics.m_accepting, "statistics not initialized");
    bbCheck(taskThreadStatusText(static_cast<CTaskThreadStatus>(999)) != nullptr, "missing unknown status text");
    Semaphore semaphore;
    bbCheck(semaphore.initializationResult().status == SEMAPHORE_OK, "priority fixture semaphore initialization failed");
    std::atomic<int32_t> waitStatus(SEMAPHORE_TIMEOUT);
    std::thread thread([&semaphore, &waitStatus]()
    {
        waitStatus.store(semaphore.waitResult(5000).status, std::memory_order_release);
    });
    bool passed = true;
    const CTaskThreadResult invalid = CTaskThreadPriority::apply(thread, static_cast<CTaskSystemPriority>(999));
    passed = passed && invalid.m_status == CTaskThreadInvalid && !invalid.m_committed;
    const CTaskThreadResult normal = CTaskThreadPriority::apply(thread, CTaskPriorityNormal);
#ifdef __EMSCRIPTEN__
    passed = passed && normal.m_status == CTaskThreadUnsupported && !normal.m_committed;
#else
    passed = passed && normal.m_status == CTaskThreadOk && normal.m_committed;
#endif
#ifdef _WIN32
    passed = passed && GetThreadPriority(thread.native_handle()) == THREAD_PRIORITY_NORMAL;
    const CTaskSystemPriority levels[] = {CTaskPriorityIdle, CTaskPriorityLow, CTaskPriorityHigh, CTaskPriorityHighest};
    const int native[] = {THREAD_PRIORITY_IDLE, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST};
    for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); ++i)
    {
        const CTaskThreadResult changed = CTaskThreadPriority::apply(thread, levels[i]);
        passed = passed && changed.m_status == CTaskThreadOk && changed.m_committed && changed.m_nativeError == 0 &&
            GetThreadPriority(thread.native_handle()) == native[i];
    }
    const CTaskThreadResult inherited = CTaskThreadPriority::apply(thread, CTaskPriorityInherited);
    passed = passed && inherited.m_status == CTaskThreadOk && inherited.m_committed &&
        GetThreadPriority(thread.native_handle()) == THREAD_PRIORITY_HIGHEST;
#else
    const CTaskThreadResult low = CTaskThreadPriority::apply(thread, CTaskPriorityLow);
    passed = passed && low.m_status == CTaskThreadUnsupported && !low.m_committed;
#endif
    const SemaphoreResult signaled = semaphore.trySignal(true, false);
    thread.join();
    passed = passed && signaled.committed && signaled.status == SEMAPHORE_OK && waitStatus.load(std::memory_order_acquire) == SEMAPHORE_OK;
    passed = passed && CTaskThreadPriority::apply(thread, CTaskPriorityNormal).m_status == CTaskThreadClosed;
    bbCheck(passed, "system priority result/native value mismatch");
    CTaskThreadPoolConfig poolConfig;
    poolConfig.m_initialThreads = 1;
    poolConfig.m_maximumThreads = 1;
    poolConfig.m_control.m_maximumTasks = 0;
    CTaskThreadPool pool;
    const size_t before = CTaskThreadManager::instance().count();
    bbCheck(pool.initialize(poolConfig).m_status == CTaskThreadCapacity, "control admission limit ignored or initialization hung");
    bbCheck(CTaskThreadManager::instance().count() == before, "failed control initialization retained workers");
    poolConfig.m_control.m_maximumTasks = -1;
    poolConfig.m_control.m_queue.initialCapacity = 2;
    poolConfig.m_control.m_queue.maximumCapacity = 2;
    poolConfig.m_control.m_taskMap.initialCapacity = 2;
    poolConfig.m_control.m_taskMap.maximumCapacity = 2;
    bbCheck(pool.initialize(poolConfig).m_status == CTaskThreadOk && pool.shutdown().m_status == CTaskThreadOk,
        "independent control pool configuration failed to recover");
    bbCheck(CTaskThreadManager::instance().count() == before, "configured control pool leaked workers");
#endif
}

#ifdef CTTM_MODULE_TEST_MAIN
/** 独立构建时执行线程模型与系统优先级检查
@return 0表示所选检查成功，非零表示失败或当前组合不适用
*/
int main()
{
    try
    {
        taskThreadModelsTest();
        std::cout << "PASS models-and-native-priority\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    catch (...)
    {
        return 1;
    }
}

#endif