#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

class ProcessWorkChannel;
class Semaphore;

/** 唯一共享收件执行者，按提交票号复制正文并维护缓冲区生命周期。 */
class ReadTask : public CTask
{
public:
    /** 初始化空借用指针。 */
    ReadTask();
    /** 运行收件与维护；失败保留头槽，有限等待后重试。 */
    void DoTask();
    /** 只设置停止标记并唤醒，不在这里等待线程。 */
    void StopTask();
    /** 绑定借用资源；这些资源必须活到任务加入完成。
    @param [in] channel 接收通道
    @param [in] queue 本地正文队列
    @param [in] notification 本地回调通知
    @param [in] fullCount 满载快照
    */
    void setParam(ProcessWorkChannel* channel, LockFreeQueue<char*>* queue,
        Semaphore* notification, std::atomic<int32_t>* fullCount);
private:
    // 生命周期owner拥有，任务仅借用
    ProcessWorkChannel* m_channel;
    LockFreeQueue<char*>* m_queue;
    Semaphore* m_notification;
    std::atomic<int32_t>* m_fullCount;
    // StopTask与DoTask共享的协作停止标记
    std::atomic<bool> m_exit;
};