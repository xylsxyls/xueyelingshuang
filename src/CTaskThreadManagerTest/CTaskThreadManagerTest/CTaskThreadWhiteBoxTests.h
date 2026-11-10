#pragma once

#include "BlackBoxTestSupport.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

/** 保护记录耗尽时，真实抢占回调重入不得等待外层投递释放保护 */
void taskReentrantProtectionTest();

/** 普通及 try 投递区分容量、参数、关闭和已提交状态，不向外抛异常 */
void taskPostResultContractTest();

/** 充足资源下并发普通投递，竞争由内部重试且每项恰好执行一次 */
void taskPostContentionContractTest();

/** 业务执行的受控暂停夹具，不使用产品专用测试分支。
*/
class TestPreparingTask : public CTask
{
public:
    /** 借用测试同步变量。
    @param [in,out] entered doTask 已进入
    @param [in,out] release 是否允许 doTask 返回
    @param [in,out] calls 执行次数
    @param [in,out] stops 停止通知次数
    */
    TestPreparingTask(std::atomic<bool>& entered, std::atomic<bool>& release,
        std::atomic<int32_t>& calls, std::atomic<int32_t>& stops);

    /** 等待放行后增加执行次数。
    */
    virtual void doTask();

    /** 增加停止次数。
    */
    virtual void stopTask();

private:
    // doTask 进入通知
    std::atomic<bool>& m_entered;
    // doTask 释放条件
    std::atomic<bool>& m_release;
    // 执行计数
    std::atomic<int32_t>& m_calls;
    // 停止计数
    std::atomic<int32_t>& m_stops;
};

/** 返回独立白盒及故障测试。
@param [in] firstId 起始稳定编号
@return 按编号递增的用例
*/
std::vector<BlackBoxCase> taskThreadWhiteBoxCases(int32_t firstId);