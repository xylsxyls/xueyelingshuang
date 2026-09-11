#pragma once
#include "PlayerCompletionTask.h"
#include <memory>

/** 执行凭据；最后执行引用释放后发布已预分配信封，未执行默认Canceled
*/
class PlayerRequestCompletion
{
public:
    /** 提交前准备信封，不调用宿主
    @param [in] request 请求参数副本
    @param [in] callback 回调副本
    @param [in] dispatcher 借用期通过shared_ptr延长，生产者结束后才停止
    */
    PlayerRequestCompletion(const LumaPlayerCoreRequest& request,
        const LumaPlayerCoreCompletionCallback& callback,
        const std::shared_ptr<PlayerCompletionTask>& dispatcher);

    /** 发布终态，不分配内存，不直接执行宿主回调
    */
    ~PlayerRequestCompletion();

public:
    // 提交线程设置接受标记；其释放最后引用前总能完成写入
    std::atomic<bool> m_accepted;
    // 唯一执行者填写的预分配信封
    std::shared_ptr<PlayerCompletionDelivery> m_delivery;

private:
    // 结果任务所有权
    std::shared_ptr<PlayerCompletionTask> m_dispatcher;
};