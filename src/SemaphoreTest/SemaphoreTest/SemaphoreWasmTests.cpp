#include "SemaphoreWasmTests.h"
#include "Semaphore.h"
#include "SemaphoreCount.h"
#include <cstdio>
#include <pthread.h>
#include <atomic>
#include <emscripten.h>

#ifdef __EMSCRIPTEN_PTHREADS__
static Semaphore* s_workerSemaphore = nullptr;
static std::atomic<int32_t> s_workerStage(0);
static std::atomic<int32_t> s_workerResult(-1);
// 两个用例的交付通道独立，主循环只在消费67号请求后发送回执。
static Semaphore s_exchangeRequest;
static Semaphore s_exchangeReply;
static std::atomic<bool> s_exchangePending(false);
// 工作线程在发布s_workerResult前写入；主循环取得完成状态后读取。
static int32_t s_exchangeResult = -1;
static bool s_countPublishFailed = false;
static bool s_replyDelivered = false;
static bool s_threadCleanupFailed = false;

/** JavaScript事件循环回调，不在主线程阻塞或回收尚未结束的工作线程
*/
void SemaphoreWasmTests::poll(void* context)
{
    (void)context;
    int32_t ready = 1;
    if (s_workerStage.compare_exchange_strong(ready, 2))
    {
        const SemaphoreResult sent = s_workerSemaphore->trySignal();
        s_countPublishFailed = sent.status != SEMAPHORE_OK || !sent.committed;
    }
    if (s_exchangePending.load())
    {
        const SemaphoreResult request = s_exchangeRequest.waitResult(0);
        if (request.status != SEMAPHORE_TIMEOUT || request.committed)
        {
            s_exchangePending.store(false);
            if (request.status == SEMAPHORE_OK && request.committed)
            {
                const SemaphoreResult reply = s_exchangeReply.trySignal();
                s_replyDelivered = reply.status == SEMAPHORE_OK && reply.committed;
            }
        }
    }
    if (s_workerStage.load() == 5)
    {
        // 池统计只表示记录已占用，发布登记仍可能尚未完成；只有committed才能确认通知命中。
        const SemaphorePoolStats stats = s_workerSemaphore->eventPoolStatistics();
        if (stats.capacity > stats.freeCapacity + stats.retiredCapacity)
        {
            const SemaphoreResult event = s_workerSemaphore->tryEvent();
            if (event.status != SEMAPHORE_OK)
            {
                s_countPublishFailed = true;
            }
            if (event.committed)
            {
                s_workerStage.store(6);
                // 单独许可确认tryEvent已经返回，工作线程据此在通知读者退出后检查回收。
                const SemaphoreResult ack = s_workerSemaphore->trySignal();
                if (ack.status != SEMAPHORE_OK || !ack.committed)
                {
                    s_countPublishFailed = true;
                }
            }
        }
    }
    const int32_t observed = s_workerResult.load();
    if (observed < 0)
    {
        emscripten_async_call(SemaphoreWasmTests::poll, nullptr, 1);
        return;
    }
    const int32_t result = s_countPublishFailed ? 7 : observed;
    const int32_t combined = s_exchangeResult == 0 && !s_replyDelivered ? 7 : s_exchangeResult;
    delete s_workerSemaphore;
    s_workerSemaphore = nullptr;
    std::printf(
        "%s id=49 Wasm pthread count/event timeout and main-loop notification result=%d\n", result == 0 ? "PASS" : "FAIL", result);
    std::printf(
        "%s id=67 Wasm combined timeout and main-loop reply result=%d\n", combined == 0 ? "PASS" : "FAIL", combined);
    emscripten_force_exit(result != 0 || combined != 0 || s_threadCleanupFailed ? 1 : 0);
}
#endif

int32_t SemaphoreWasmTests::admission()
{
    SemaphorePoolConfig config;
    config.initialCapacity = 2;
    config.maxCapacity = 2;
    SemaphoreEventPool events(config);
    SemaphoreCount counts(config);
    SemaphoreWake wake;
    std::atomic<uint64_t> state(0);
    SemaphoreEventRecord* eventRecords[2] = {};
    SemaphoreEventRecord* countRecords[2] = {};
    bool passed = true;
    for (size_t index = 0; index < 2; ++index)
    {
        const SemaphoreResult registered = counts.begin(state, countRecords[index], false);
        passed = events.begin(eventRecords[index]).status == SEMAPHORE_OK && eventRecords[index] != nullptr &&
            registered.status == SEMAPHORE_OK && !registered.committed && countRecords[index] != nullptr && passed;
    }
    for (size_t turn = 0; turn < 32 && passed; ++turn)
    {
        const size_t chosen = turn % 2;
        passed = events.notify(false).committed && counts.signal(state, wake, false).committed;
        passed = passed && eventRecords[chosen]->m_state.load() == 1 && countRecords[chosen]->m_state.load() == 1;
        passed = passed && eventRecords[1 - chosen]->m_state.load() == 0 && countRecords[1 - chosen]->m_state.load() == 0;
        passed = events.finish(eventRecords[chosen]) && passed;
        passed = counts.finish(state, countRecords[chosen]) && passed;
        eventRecords[chosen] = nullptr;
        countRecords[chosen] = nullptr;
        const SemaphoreResult registered = counts.begin(state, countRecords[chosen], false);
        passed = events.begin(eventRecords[chosen]).status == SEMAPHORE_OK && eventRecords[chosen] != nullptr &&
            registered.status == SEMAPHORE_OK && !registered.committed && countRecords[chosen] != nullptr && passed;
    }
    for (size_t index = 0; index < 2; ++index)
    {
        if (eventRecords[index] != nullptr)
        {
            events.finish(eventRecords[index]);
        }
        if (countRecords[index] != nullptr)
        {
            counts.finish(state, countRecords[index]);
        }
    }
    state.store(UINT64_MAX >> 1);
    passed = counts.signal(state, wake, false).committed && passed;
    SemaphoreEventRecord* unused = nullptr;
    passed = counts.begin(state, unused, true).committed && passed;
    passed = state.load() == (UINT64_MAX >> 1) && passed;
    counts.clear(state);
    size_t released = 0;
    passed = counts.maintain(released).status == SEMAPHORE_OK && passed;
    passed = counts.statistics().freeCapacity == 2 && passed;
    std::printf("%s id=87 Wasm internal admission and large-count representation\n", passed ? "PASS" : "FAIL");
    return passed ? 0 : 1;
}

/** Wasm能力验证，主线程不执行阻塞等待
*/
int32_t SemaphoreWasmTests::run()
{
    try
    {
        if (admission() != 0)
        {
            return 1;
        }
        {
            SemaphoreConfig config;
            config.configureEventPool().initialCapacity = 3;
            Semaphore sem(config);
            if (sem.initializationResult().status != SEMAPHORE_OK || sem.eventPoolStatistics().capacity != 3 ||
                sem.waitPolicy().m_localSlice != 32)
            {
                std::printf("FAIL id=48 Wasm instance configuration\n");
                return 1;
            }
            sem.signal();
            if (!sem.wait(0) || sem.wait(0) || sem.waitResult(10).status != SEMAPHORE_UNSUPPORTED ||
                sem.eventWaitResult(10).status != SEMAPHORE_UNSUPPORTED ||
                sem.createProcessResult("SPWasm").status != SEMAPHORE_UNSUPPORTED ||
                sem.processWaitResult(0).status != SEMAPHORE_UNSUPPORTED)
            {
                std::printf("FAIL id=48 Wasm main-thread capability\n");
                return 1;
            }
            const SemaphoreResult posted = sem.processSignalResult();
            const SemaphoreResult tried = sem.tryProcessSignalResult(true, true);
            if (posted.status != SEMAPHORE_UNSUPPORTED || posted.committed ||
                tried.status != SEMAPHORE_UNSUPPORTED || tried.committed ||
                sem.processSignal() || sem.tryProcessSignal() != -1)
            {
                std::printf("FAIL id=48 Wasm named publication capability\n");
                return 1;
            }
            Semaphore reply;
            const SemaphoreSignalWaitResult ipc = sem.processSignalAndWaitResult(reply, 0);
            if (ipc.status != SEMAPHORE_UNSUPPORTED || ipc.signalState != SEMAPHORE_COMMIT_NONE ||
                ipc.waitState != SEMAPHORE_COMMIT_NONE || sem.prepareSignalAndWait().status != SEMAPHORE_UNSUPPORTED)
            {
                std::printf("FAIL id=48 Wasm combined capability\n");
                return 1;
            }
            reply.signal();
            const SemaphoreSignalWaitResult immediate = sem.signalAndWaitResult(reply, 0);
            if (immediate.status != SEMAPHORE_OK || immediate.signalState != SEMAPHORE_COMMIT_DONE ||
                immediate.waitState != SEMAPHORE_COMMIT_DONE || !sem.wait(0) || reply.wait(0))
            {
                std::printf("FAIL id=48 Wasm nonblocking combined permit accounting\n");
                return 1;
            }
            const SemaphoreSignalWaitResult empty = sem.signalAndWaitResult(reply, 0);
            if (empty.status != SEMAPHORE_TIMEOUT || empty.signalState != SEMAPHORE_COMMIT_DONE ||
                empty.waitState != SEMAPHORE_COMMIT_NONE || !sem.wait(0) || reply.countPoolStatistics().capacity != 0)
            {
                std::printf("FAIL id=48 Wasm empty zero-budget reply\n");
                return 1;
            }
            const SemaphoreSignalWaitResult blocked = sem.signalAndWaitResult(reply, 10);
            if (blocked.status != SEMAPHORE_UNSUPPORTED || blocked.signalState != SEMAPHORE_COMMIT_DONE ||
                blocked.waitState != SEMAPHORE_COMMIT_NONE || !sem.wait(0) || sem.wait(0))
            {
                std::printf("FAIL id=48 Wasm committed send with rejected blocking wait\n");
                return 1;
            }
        }
#ifdef __EMSCRIPTEN_PTHREADS__
        SemaphoreConfig workerConfig;
        SemaphoreWaitPolicy policy;
        policy.m_localSlice = UINT32_MAX - 1;
        policy.m_promotionMilliseconds = UINT32_MAX - 1;
        if (workerConfig.setWaitPolicy(policy).status != SEMAPHORE_OK)
        {
            std::fprintf(stderr, "FAIL id=49 invalid worker wait configuration\n");
            return 1;
        }
        s_workerSemaphore = new Semaphore(workerConfig);
        pthread_attr_t attributes;
        if (pthread_attr_init(&attributes) != 0)
        {
            delete s_workerSemaphore;
            s_workerSemaphore = nullptr;
            return 1;
        }
        const int32_t configured = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
        pthread_t thread;
        const int32_t created =
            configured == 0 ? pthread_create(&thread, &attributes, SemaphoreWasmTests::worker, nullptr) : configured;
        const int32_t released = pthread_attr_destroy(&attributes);
        if (created != 0)
        {
            delete s_workerSemaphore;
            s_workerSemaphore = nullptr;
            return 1;
        }
        if (released != 0)
        {
            std::fprintf(stderr, "FAIL worker attribute cleanup: %d\n", released);
            s_threadCleanupFailed = true;
        }
        std::printf("PASS id=48 Wasm main-thread rejection; count probe; IPC unsupported\n");
        emscripten_async_call(SemaphoreWasmTests::poll, nullptr, 1);
        emscripten_exit_with_live_runtime();
#else
        std::printf("NOT_RUN id=49 nonpthread build\nNOT_RUN id=67 nonpthread build\n");
#endif
        std::printf("PASS id=48 Wasm main-thread rejection; count probe; IPC unsupported\n");
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL id=48 exception %s\n", error.what());
        return 1;
    }
}
#ifdef __EMSCRIPTEN_PTHREADS__
void* SemaphoreWasmTests::worker(void* context)
{
    (void)context;
    int32_t result = 0;
    try
    {
        // 长片段仍受单次调用的总预算约束；主循环只有收到登记阶段后才允许发布。
        const double countStarted = emscripten_get_now();
        const SemaphoreResult count = s_workerSemaphore->waitResult(1250);
        if (count.status != SEMAPHORE_TIMEOUT || count.committed || emscripten_get_now() - countStarted < 1240)
        {
            result = 1;
        }
        s_workerStage.store(1);
        if (!s_workerSemaphore->wait(1000))
        {
            result = 2;
        }
        // 先验证提前事件不积存，再验证工作线程在独立记录地址等待并由主循环唤醒。
        const SemaphoreResult early = s_workerSemaphore->tryEvent();
        const double eventStarted = emscripten_get_now();
        const SemaphoreResult emptyEvent = s_workerSemaphore->eventWaitResult(1250);
        if (early.status != SEMAPHORE_OK || early.committed || emptyEvent.status != SEMAPHORE_TIMEOUT ||
            emptyEvent.committed || emscripten_get_now() - eventStarted < 1240)
        {
            result = 8;
        }
        s_workerStage.store(5);
        const SemaphoreResult event = s_workerSemaphore->eventWaitResult(1000);
        const bool completed = s_workerSemaphore->wait(1000);
        size_t released = 0;
        const SemaphoreResult maintained = s_workerSemaphore->maintainEventPool(released);
        const SemaphorePoolStats stats = s_workerSemaphore->eventPoolStatistics();
        if (event.status != SEMAPHORE_OK || !event.committed || !completed || maintained.status != SEMAPHORE_OK ||
            stats.capacity != stats.freeCapacity ||
            stats.retiredCapacity != 0 || s_workerSemaphore->wait(0))
        {
            result = 9;
        }
    }
    catch (...)
    {
        result = 3;
    }
    try
    {
        Semaphore sender, empty;
        const bool timedPrepared = sender.prepareSignalAndWait().status == SEMAPHORE_OK &&
            empty.prepareSignalAndWait().status == SEMAPHORE_OK;
        const SemaphoreSignalWaitResult timed = sender.signalAndWaitResult(empty, 30);
        const bool timeoutValid = timed.status == SEMAPHORE_TIMEOUT && timed.signalState == SEMAPHORE_COMMIT_DONE &&
            timed.waitState == SEMAPHORE_COMMIT_NONE && sender.wait(0) && !sender.wait(0);
        const bool prepared = s_exchangeRequest.prepareSignalAndWait().status == SEMAPHORE_OK &&
            s_exchangeReply.prepareSignalAndWait().status == SEMAPHORE_OK;
        s_exchangePending.store(true);
        const SemaphoreSignalWaitResult replied = s_exchangeRequest.signalAndWaitResult(s_exchangeReply, 1000);
        const bool replyValid = replied.status == SEMAPHORE_OK && replied.signalState == SEMAPHORE_COMMIT_DONE &&
            replied.waitState == SEMAPHORE_COMMIT_DONE && !s_exchangeRequest.wait(0) && !s_exchangeReply.wait(0);
        s_exchangeResult = timedPrepared && prepared && timeoutValid && replyValid ? 0 : 6;
    }
    catch (...)
    {
        s_exchangeResult = 3;
    }
    // 此后不再访问Semaphore，主线程取得结果后才回收对象
    s_workerResult.store(result);
    return nullptr;
}
#endif

/** Wasm测试入口，返回可观察的退出码
@return 普通模块退出码，线程模块由事件循环完成
*/
int32_t main()
{
    return SemaphoreWasmTests::run();
}