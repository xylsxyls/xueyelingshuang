#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>
#include "ProcessWorkReceiveQueue.h"

class ProcessReceiveCallback;

/** 接收任务
*/
class ReceiveTask : public CTask
{
public:
    /** 构造函数
    */
    ReceiveTask();

public:
    /** 执行任务
    */
    void doTask() override;

    /** 中断任务
    */
    void stopTask() override;

    /** 设置参数
    @param [in] callback 接收回调类
    @param [in] receiveQueue 接收队列
    @param [in] fullCount 合并的满载事件，借用至任务加入完成
    */
    void setParam(std::vector<ProcessReceiveCallback*>* callback,
        ProcessWorkReceiveQueue* receiveQueue,
        std::atomic<int32_t>* fullCount = nullptr);

private:
    // 合并后的满载快照，0表示没有待分派事件
    std::atomic<int32_t>* m_fullCount;
    std::vector<ProcessReceiveCallback*>* m_callback;
    ProcessWorkReceiveQueue* m_receiveQueue;
    std::atomic<bool> m_exit;
};