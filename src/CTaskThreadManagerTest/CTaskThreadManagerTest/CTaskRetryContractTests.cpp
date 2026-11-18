#include "CTaskRetryContractTests.h"
#include "CTaskThreadManagerTest.h"
#ifndef CTTM_TEST_DLL
#include "CTaskThreadManager/CTaskResultAdapter.h"
#include "CTaskThreadTestAccess.h"
#endif
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

CTaskRetryContractTask::CTaskRetryContractTask(const std::function<void()>& body) :
m_body(body)
{

}

void CTaskRetryContractTask::doTask()
{
    m_body();
}

/** 门控消费者形成大积压，验证普通和try投递的顺序及精确执行
@return 全部接纳并排空，无内部异常、遗漏或顺序错误时true
*/
static bool BackloggedCase()
{
    const size_t pending = 262144;
    const size_t additional = 32768;
    std::atomic<bool> entered(false), release(false);
    size_t completed = 0;
    bool ordered = true;
    CTaskThreadConfig config;
    config.m_taskMap.initialCapacity = pending + additional + 4096;
    config.m_taskMap.maximumCapacity = static_cast<int64_t>(config.m_taskMap.initialCapacity);
    config.m_queue.initialCapacity = pending + additional + 4096;
    config.m_queue.maximumCapacity = static_cast<int64_t>(config.m_queue.initialCapacity);
    TestWorkers workers;
    const std::shared_ptr<CTaskThread> worker = workers.add(config);
    try
    {
        const std::shared_ptr<CTask> gate(new CTaskRetryContractTask([&]()
        {
            entered.store(true, std::memory_order_release);
            bbWait([&]() { return release.load(std::memory_order_acquire); }, "backlog gate release", 15000);
        }));
        bbCheck(worker->postTask(gate), "backlog gate rejected");
        bbWait([&]() { return entered.load(std::memory_order_acquire); }, "backlog gate entry");
        std::vector<std::shared_ptr<CTask>> tasks;
        for (size_t i = 0; i < 32; ++i)
        {
            tasks.push_back(std::shared_ptr<CTask>(new CTaskRetryContractTask([&, i]()
            {
                // 固定对象身份循环投递；消费者独占账本，排空后才由协调线程读取。
                ordered = ordered && completed % 32 == i;
                ++completed;
            })));
        }
        for (size_t i = 0; i < pending + additional; ++i)
        {
            const CTaskThreadResult result = i < pending ? worker->postTaskResult(tasks[i % 32]) :
                worker->tryPostTaskResult(tasks[i % 32], 1, false, false);
            bbCheck(result.m_committed && result.m_status == CTaskThreadOk, "backlog admission failed");
        }
        bbCheck(worker->getWaitTaskCount() == pending + additional, "backlog count changed before release");
        release.store(true, std::memory_order_release);
        workers.drain();
        return ordered && completed == pending + additional && worker->statistics().m_errors == 0 &&
            worker->statistics().m_outstanding == 0;
    }
    catch (...)
    {
        release.store(true, std::memory_order_release);
        workers.drain();
        throw;
    }
}

/** 验证有界接纳的等待、拒绝和关闭
@param [in] mode 0到3为开关组合，4为普通投递，5为等待时关闭，6为增长策略拒绝后等待复用
@return 无误接纳、丢失、内部异常或重复时true
*/
static bool CapacityCase(size_t mode)
{
    std::atomic<bool> entered(false), release(false), started(false), finished(false), capacityObserved(false);
    std::atomic<size_t> calls(0);
    CTaskThreadResult result;
    CTaskThreadConfig config;
    config.m_maximumTasks = 1;
    if (mode == 6)
    {
        config.m_maximumTasks = -1;
        config.m_taskMap.initialCapacity = 1;
        config.m_taskMap.maximumCapacity = -1;
        config.m_taskMap.growPolicy = [&](const LockFreeHashMapGrowContext&)
        {
            // 记录实际满池路径到达；拒绝增长后只能等待原记录归还。
            capacityObserved.store(true, std::memory_order_release);
            return size_t(0);
        };
    }
    TestWorkers workers;
    const std::shared_ptr<CTaskThread> worker = workers.add(config);
    std::thread producer;
    try
    {
        const std::shared_ptr<CTask> gate(new CTaskRetryContractTask([&]()
        {
            entered.store(true);
            bbWait([&]() { return release.load(); }, "capacity gate release", 15000);
            ++calls;
        }));
        const std::shared_ptr<CTask> next(new CTaskRetryContractTask([&]() { ++calls; }));
        bbCheck(worker->postTask(gate), "capacity gate rejected");
        bbWait([&]() { return entered.load(); }, "capacity gate entry");
        producer = std::thread([&, next]()
        {
            started.store(true);
            try
            {
                result = mode == 4 || mode == 6 ? worker->postTaskResult(next) :
                    worker->tryPostTaskResult(next, 1, (mode & 1) != 0, mode >= 2);
            }
            catch (...)
            {
                result = CTaskThreadResult(CTaskThreadSystemFailure);
            }
            finished.store(true);
        });
        bbWait([&]() { return started.load(); }, "capacity producer entry");
        if (mode == 6)
        {
            bbWait([&]() { return capacityObserved.load(std::memory_order_acquire); }, "capacity policy observation");
        }
#ifndef CTTM_TEST_DLL
        if (mode >= 2)
        {
            // 等到真实投递尝试命中容量不足，不能仅凭生产线程已启动就声称验证了等待。
            bbWait([&]() { return CTaskThreadTestAccess::capacityPressure(*worker); }, "capacity retry observation");
        }
#endif
        if (mode < 2)
        {
            bbWait([&]() { return finished.load(); }, "nonwaiting capacity result");
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        bool valid = finished.load() == (mode < 2);
        if (mode == 5)
        {
            valid = worker->requestShutdown(false).m_status == CTaskThreadOk && valid;
        }
        release.store(true);
        producer.join();
        workers.drain();
        valid = worker->statistics().m_errors == 0 && valid;
        if (mode < 2)
        {
            return valid && result.m_status == CTaskThreadCapacity && !result.m_committed && calls.load() == 1;
        }
        if (mode == 5)
        {
            return valid && result.m_status == CTaskThreadClosed && !result.m_committed && calls.load() == 1;
        }
        return valid && result.m_status == CTaskThreadOk && result.m_committed && calls.load() == 2;
    }
    catch (...)
    {
        release.store(true);
        worker->requestShutdown(false);
        if (producer.joinable())
        {
            producer.join();
        }
        workers.drain();
        throw;
    }
}

bool TaskRetryContractTest()
{
#ifndef CTTM_TEST_DLL
    uint32_t attempts = 0;
    for (uint32_t i = 0; i < 4096; ++i)
    {
        if (CTaskResultAdapter::pauseCapacity(attempts).m_status != CTaskThreadOk || attempts != i + 1)
        {
            return false;
        }
    }
    const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (size_t i = 0; i < 8; ++i)
    {
        if (CTaskResultAdapter::pauseCapacity(attempts).m_status != CTaskThreadOk || attempts != 4096)
        {
            return false;
        }
    }
    if (std::chrono::steady_clock::now() - begin < std::chrono::milliseconds(8))
    {
        return false;
    }
#endif
    for (size_t mode = 0; mode < 7; ++mode)
    {
        if (!CapacityCase(mode))
        {
            std::cout << "FAIL task capacity mode=" << mode << std::endl;
            return false;
        }
    }
    if (!BackloggedCase())
    {
        return false;
    }
    CTaskThreadResult nested;
    std::atomic<bool> done(false);
    std::atomic<size_t> unwanted(0);
    CTaskThreadConfig config;
    config.m_maximumTasks = 1;
    TestWorkers workers;
    const std::shared_ptr<CTaskThread> worker = workers.add(config);
    const std::shared_ptr<CTask> next(new CTaskRetryContractTask([&]() { ++unwanted; }));
    const std::shared_ptr<CTask> task(new CTaskRetryContractTask([&, next, worker]()
    {
        nested = worker->tryPostTaskResult(next, 1, true, true);
        done.store(true);
    }));
    bbCheck(worker->postTask(task), "self-wait gate rejected");
    bbWait([&]() { return done.load(); }, "consumer self-wait result");
    workers.drain();
    return nested.m_status == CTaskThreadSelfWait && !nested.m_committed && unwanted.load() == 0 &&
        worker->statistics().m_errors == 0;
}