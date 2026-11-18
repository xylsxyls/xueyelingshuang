#include "CTaskWasmSmoke.h"
#include <chrono>
#include <cstdio>
#include <thread>

CTaskWasmSmokeTask::CTaskWasmSmokeTask(int32_t value, std::vector<int32_t>& ledger, std::atomic<bool>* gate) :
m_value(value), m_ledger(&ledger), m_gate(gate)
{

}

void CTaskWasmSmokeTask::doTask()
{
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (m_gate != nullptr && !m_gate->load(std::memory_order_acquire))
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            // 不追加门控项，最终201项账本断言必须失败；避免测试任务永久占住唯一消费者。
            return;
        }
        std::this_thread::yield();
    }
    m_ledger->push_back(m_value);
}

/** 运行独立 Wasm 线程和任务顺序冒烟检查
@return 全部检查通过返回 0，失败返回 1
*/
int main()
{
    std::vector<int32_t> ledger;
    std::atomic<bool> gate(false);
    try
    {
        CTaskThreadManager& manager = CTaskThreadManager::instance();
        ledger.reserve(201);
        uint32_t id = 0;
        const CTaskThreadResult initialized = manager.initialize(CTaskThreadConfig(), id);
        if (!initialized.m_committed || initialized.m_status != CTaskThreadOk || id == 0)
        {
            manager.uninitAll();
            std::puts("FAIL: Wasm worker initialization");
            return 1;
        }
        std::shared_ptr<CTaskThread> worker = manager.getThreadInterface(id);
        bool accepted = worker && worker->postTask(std::shared_ptr<CTask>(new CTaskWasmSmokeTask(-1, ledger, &gate)), 100);
        for (int32_t value = 0; accepted && value < 200; ++value)
        {
            accepted = worker->postTask(std::shared_ptr<CTask>(new CTaskWasmSmokeTask(value, ledger, nullptr)), value < 100 ? 1 : 10);
        }
        const bool priorityReported = worker && worker->setSystemPriority(CTaskPriorityHigh).m_status == CTaskThreadUnsupported;
        gate.store(true, std::memory_order_release);
        const bool drained = manager.waitForEnd(id).m_status == CTaskThreadOk;
        bool ordered = ledger.size() == 201 && ledger[0] == -1;
        for (size_t i = 1; ordered && i < ledger.size(); ++i)
        {
            ordered = ledger[i] == static_cast<int32_t>(i <= 100 ? i + 99 : i - 101);
        }
        const bool clean = worker && worker->statistics().m_errors == 0 && worker->statistics().m_outstanding == 0;
        const bool passed = accepted && drained && ordered && priorityReported && clean && manager.count() == 0;
        std::puts(passed ? "PASS: Wasm pthread, 201 tasks, priority/FIFO, unsupported OS priority, drain and cleanup" : "FAIL: Wasm task contract");
        return passed ? 0 : 1;
    }
    catch (...)
    {
        gate.store(true, std::memory_order_release);
        CTaskThreadManager::instance().uninitAll();
        std::puts("FAIL: unexpected Wasm exception");
        return 1;
    }
}