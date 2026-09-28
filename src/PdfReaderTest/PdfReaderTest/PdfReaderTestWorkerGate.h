#pragma once
#include "Semaphore/SemaphoreAPI.h"
#include <atomic>
#include <memory>

/** 测试专用可控时序：阻塞唯一执行者，不在GUI等待信号量 */
class PdfReaderTestWorkerGate
{
public:
    /** 通过真实逻辑队列提交有限阻塞任务 */
    PdfReaderTestWorkerGate();
    /** 失败退栈时仍释放执行者 */
    ~PdfReaderTestWorkerGate();
    /** 主动释放阻塞，重复调用无害 */
    void release();
    // 任务入口和预算耗尽的实际观测，引用活到任务返回
    std::shared_ptr<std::atomic<bool>> m_entered;
    std::shared_ptr<std::atomic<bool>> m_timedOut;
private:
    // 持久计数通知；任务结束前由共享引用保活
    std::shared_ptr<Semaphore> m_release;
};