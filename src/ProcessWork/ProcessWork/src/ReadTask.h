#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "ProcessWorkReceiveQueue.h"

class ProcessWorkChannel;
class Semaphore;

/** 唯一共享收件执行者，按链接提交顺序复制正文并维护缓冲区生命周期。
*/
class ReadTask : public CTask
{
public:
    /** 初始化空借用指针。
    */
    ReadTask();

    /** 运行收件与维护；失败保留头槽，有限等待后重试。
    */
    void doTask() override;

    /** 只设置停止标记并唤醒，不在这里等待线程。
    */
    void stopTask() override;

    /** 绑定借用资源；这些资源必须活到任务加入完成。
    @param [in] channel 接收通道
    @param [in] queue 本地正文队列
    @param [in] fullCount 满载快照
    */
    void setParam(ProcessWorkChannel* channel, ProcessWorkReceiveQueue* queue,
        std::atomic<int32_t>* fullCount);

private:
    // 生命周期owner拥有，任务仅借用
    ProcessWorkChannel* m_channel;
    // 借用本地正文队列，活到读任务加入完成
    ProcessWorkReceiveQueue* m_queue;
    // 借用满载通知快照
    std::atomic<int32_t>* m_fullCount;
    // stopTask与doTask共享的协作停止标记
    std::atomic<bool> m_exit;
};