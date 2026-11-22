#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>
#include "ProcessWorkReceiveQueue.h"

class ProcessReceiveCallback;

/** 唯一回调任务；逐条借用本地正文并按登记顺序通知观察者，返回后归还正文
*/
class ReceiveTask : public CTask
{
public:
    /** 初始化空借用和未停止状态
    */
    ReceiveTask();

public:
    /** 持续消费至停止，隔离单个回调异常；回调对象和列表由owner保活
    */
    void doTask() override;

    /** 发布停止标志并唤醒等待；不强制中断正在执行的回调，也不等待退出
    */
    void stopTask() override;

    /** 启动前绑定资源，任务退出并完成线程回收前不得销毁
    @param [in] callback 借用的回调列表；启动后不增删，空指针使任务直接返回
    @param [in] receiveQueue 借用的唯一消费队列，空指针使任务直接返回
    @param [in] fullCount 借用的合并满载事件，nullptr表示不分派满载通知
    */
    void setParam(std::vector<ProcessReceiveCallback*>* callback,
        ProcessWorkReceiveQueue* receiveQueue,
        std::atomic<int32_t>* fullCount = nullptr);

private:
    // 合并后的满载快照，0表示没有待分派事件
    std::atomic<int32_t>* m_fullCount;
    // 借用接收观察者列表，接收任务运行期间不可修改或销毁
    std::vector<ProcessReceiveCallback*>* m_callback;
    // 借用正文批次队列，回调返回后归还计费和正文所有权
    ProcessWorkReceiveQueue* m_receiveQueue;
    // 由stopTask发布，接收循环协作退出
    std::atomic<bool> m_exit;
};