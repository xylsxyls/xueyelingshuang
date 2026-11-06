#include "CTaskThreadModels.h"

CTaskThreadResult::CTaskThreadResult(CTaskThreadStatus status, bool committed, int32_t nativeError) :
m_status(status),
m_committed(committed),
m_nativeError(nativeError)
{

}

CTaskThreadConfig::CTaskThreadConfig() :
m_taskBuckets(16384),
m_maximumTasks(-1),
m_idleCheckMilliseconds(32),
m_systemPriority(CTaskPriorityInherited)
{
    // 元数据随允许的并发生产者增长；默认不为每个空队列保留 1024 个生产者记录。
    m_queue.maximumProducers = 64;
    m_taskMap.lookupCacheCapacity = 0;
}

CTaskThreadManagerConfig::CTaskThreadManagerConfig()
{

}

CTaskThreadStatistics::CTaskThreadStatistics() :
m_outstanding(0),
m_queued(0),
m_executed(0),
m_rejected(0),
m_errors(0),
m_currentLevel(0),
m_accepting(false),
m_exited(false)
{

}

const char* taskThreadStatusText(CTaskThreadStatus status)
{
    switch (status)
    {
    case CTaskThreadOk:
    {
        return "成功";
    }
    case CTaskThreadInvalid:
    {
        return "参数非法";
    }
    case CTaskThreadClosed:
    {
        return "已停止接纳任务";
    }
    case CTaskThreadCapacity:
    {
        return "达到容量上限";
    }
    case CTaskThreadRetry:
    {
        return "资源暂不可用，可重试未接纳操作";
    }
    case CTaskThreadNoMemory:
    {
        return "内存分配失败";
    }
    case CTaskThreadPolicyFailure:
    {
        return "扩缩容策略拒绝或异常";
    }
    case CTaskThreadCallbackFailure:
    {
        return "任务回调异常";
    }
    case CTaskThreadSystemFailure:
    {
        return "系统调用失败";
    }
    case CTaskThreadUnsupported:
    {
        return "当前平台不支持";
    }
    case CTaskThreadNotFound:
    {
        return "对象不存在";
    }
    case CTaskThreadSelfWait:
    {
        return "不能等待自身或自己的回调";
    }
    case CTaskThreadBusy:
    {
        return "生命周期操作正在进行";
    }
    case CTaskThreadSequenceExhausted:
    {
        return "编号已耗尽";
    }
    default:
    {
        return "未知线程错误";
    }
    }
}