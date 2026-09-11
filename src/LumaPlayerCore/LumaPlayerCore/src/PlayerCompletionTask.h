#pragma once
#include "PlayerCompletionDelivery.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>

/** 有界结果派发任务，宿主回调只在无锁区执行
*/
class PlayerCompletionTask : public CTask
{
public:
    /** 初始化空队列
    */
    PlayerCompletionTask();

    /** 提交前登记结果信封；接受之后发布结果不再分配队列节点
    @param [in] delivery 未发布的信封
    @return false表示退出或达到容量上限，不能接受业务请求
    */
    bool reserve(const std::shared_ptr<PlayerCompletionDelivery>& delivery);

    /** 发布已登记信封，保证通知与等待之间不丢唤醒
    @param [in] delivery 完成或取消结果
    */
    void publish(const std::shared_ptr<PlayerCompletionDelivery>& delivery);

    /** 等待并发送结果，停止时排空；线程未启动就关闭时允许协调方补排空
    */
    virtual void DoTask();

    /** 生产者及提交者全部结束后排空退出，不允许中途强杀回调
    */
    virtual void StopTask();

private:
    // 每实例最大未交付请求数，达到上限直接拒绝新提交
    static const size_t kCapacity = 4096;
    // 提交前登记的信封
    std::list<std::shared_ptr<PlayerCompletionDelivery>> m_deliveries;
    // 队列及等待同步
    std::mutex m_mutex;
    // 完成或停止通知
    std::condition_variable m_ready;
    // 退出标记
    std::atomic<bool> m_exit;
};