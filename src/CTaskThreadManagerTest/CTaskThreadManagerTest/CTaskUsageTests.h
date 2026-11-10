#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <functional>

/** 示例任务：执行体在构造后冻结，停止标志允许并发访问
*/
class UsageTask : public CTask
{
public:
    /** 保存业务执行体，不创建线程
    @param [in] propertyId 可重复的属性 ID
    @param [in] body 任务执行体，捕获对象须覆盖所有调用
    @param [in] redo 是否允许高优先级抢占后重做
    */
    UsageTask(int32_t propertyId, const std::function<void(UsageTask&)>& body, bool redo = false);

    /** 调用业务执行体，异常交由库记录
    */
    virtual void doTask();

    /** 合作停止，只发布状态，不等待 doTask
    */
    virtual void stopTask();

    /** 返回构造时固定的重做意愿
    @return 是否请求重做，还须配置 clone 工厂
    */
    virtual bool reExecute();

    /** 观察合作停止状态
    @return 是否曾收到停止通知
    */
    bool stopped() const;

    /** 观察已经开始的执行次数，不代表执行完成
    @return 执行入口次数
    */
    uint32_t runs() const;

    /** 观察停止回调次数
    @return 回调次数，可能大于一
    */
    uint32_t stops() const;

private:
    // 构造后冻结的业务回调
    std::function<void(UsageTask&)> m_body;
    // 构造后冻结的重做决定
    bool m_redo;
    // 不在 doTask 入口清零，避免覆盖提前收到的通知
    std::atomic<bool> m_stopped;
    // 示例结果观察计数
    std::atomic<uint32_t> m_runs;
    // 并发停止通知计数
    std::atomic<uint32_t> m_stops;
};

/** 示例扩缩容策略：外部可改变期望数，池负责上下限和安全关闭
*/
class UsagePoolPolicy : public CTaskPoolPolicy
{
public:
    /** 初始请求两条工作线程
    */
    UsagePoolPolicy();

    /** 修改示例期望线程总数
    @param [in] target 期望总数，不是新增数量
    */
    void setTarget(size_t target);

    /** 有积压时不主动缩小，空闲时按目标和空闲时间请求调整
    @param [in] load 初始数、当前数、增长历史、负载和时间观测
    @return 库会截断到配置的初始数及上限
    */
    virtual CTaskPoolDecision evaluate(const CTaskPoolLoad& load);

private:
    // 可由外部线程并发修改的算法输入
    std::atomic<size_t> m_target;
};

/** 执行一个公开接口示例，独立建立并清理管理器状态
@param [in] scenario 1到7选择场景，0执行全部七项
@return 0为断言和清理全部通过，非零为失败
*/
int32_t runTaskUsageTests(size_t scenario);