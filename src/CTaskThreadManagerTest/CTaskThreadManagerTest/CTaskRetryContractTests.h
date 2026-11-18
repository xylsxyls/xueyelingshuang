#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <functional>

/** 重试契约用例拥有的任务，只执行业务回调，不向生产库注入测试成员
*/
class CTaskRetryContractTask : public CTask
{
public:
    /** 保存本用例执行体
    @param [in] body 借用对象须存活到任务结束
    */
    explicit CTaskRetryContractTask(const std::function<void()>& body);

    /** 执行用例业务体
    */
    void doTask() override;

private:
    // 测试体及其捕获对象的所有权
    std::function<void()> m_body;
};

/** 黑盒验证满载的四种开关、普通投递、关闭取消和消费者自等待拒绝
所有接纳项通过独立计数核对，另核对294912项积压FIFO；每个场景新建线程
静态组合只读观察实际容量重试，DLL通过门控及增长回调验证；收尾先解除门控再join和注销
@return 全部满足契约时true
*/
bool TaskRetryContractTest();