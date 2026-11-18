#pragma once
#include "CTaskThread.h"
#include "CTaskPoolPolicy.h"
#include "Semaphore/SemaphoreAPI.h"
#include <vector>
#include <map>

/** 线程池内部状态；成员变更只有初始化 owner 或唯一控制线程执行
*/
class CTaskPoolState
{
public:
    /** 复制配置
    @param [in] config 冻结参数
    */
    explicit CTaskPoolState(const CTaskThreadPoolConfig& config);

    /** 外部调用静止后关闭池；正常收尾未完成时逐个等待，避免线程借用已销毁状态
    */
    ~CTaskPoolState();

    /** 创建成员及控制线程
    @return 完整启动结果
    */
    CTaskThreadResult start();

    /** 请求控制循环停止
    */
    void requestStop();

    /** 唯一控制循环，不把用户策略异常传播到任务入口
    */
    void run();

    /** 关闭本池线程
    @param [in] drain 是否排空
    @return 生命周期结果；停止回调异常不跳过线程等待
    */
    CTaskThreadResult shutdown(bool drain);

    /** 查询统计
    @return 弱一致副本
    */
    CTaskPoolStatistics statistics() const;

    /** 复制成员编号
    @param [out] ids 输出编号
    @return 复制结果
    */
    CTaskThreadResult threadIds(std::vector<uint32_t>& ids) const;

    /** 选择最小负载成员
    @param [in] turn 同负载轮转提示
    @param [out] id 编号
    @param [out] thread 强引用
    @return 选择结果，不预留投递容量
    */
    CTaskThreadResult choose(uint64_t turn, uint32_t& id, std::shared_ptr<CTaskThread>& thread);

private:
    /** 析构阶段完成单线程停止或等待；不持有成员表保护，不因临时资源不足放弃存活线程
    @param [in] thread 本池拥有的非空线程强引用
    @param [in] wait false只通知，true在通知后等待；调用者不得位于该线程的回调内
    */
    static void finishForDestruction(const std::shared_ptr<CTaskThread>& thread, bool wait);

    /** 析构资源不足兜底：通知成员、收敛控制任务后再逐个等待最终成员，不分配快照
    调用方保证没有并发池接口；登记删除仍失败时由管理器保留已退出线程并在资源恢复后清理
    */
    void finishWithoutSnapshot();

    /** 新增一个工作线程
    @param [in] initial 是否属于初始化
    @return 创建并登记结果
    */
    CTaskThreadResult addWorker(bool initial);

    /** 调用策略并执行安全调整
    @param [in] now 单调毫秒数
    */
    void rebalance(uint64_t now);

    /** 读取单调时间
    @return 毫秒值，时钟异常由调用边界捕获
    */
    static uint64_t nowMilliseconds();

private:
    // 冻结配置
    const CTaskThreadPoolConfig m_config;
    // 不变策略强引用，允许调用者自定义继承类
    std::shared_ptr<CTaskPoolPolicy> m_policy;
    // 并发读、单控制线程修改的工作线程索引
    LockFreeMap<uint32_t, std::shared_ptr<CTaskThread>> m_workers;
    // 控制线程编号，生命周期 owner 写入
    uint32_t m_controller;
    // 策略评估通知
    Semaphore m_changed;
    // 是否要求控制任务停止
    std::atomic<bool> m_stop;
    // 任一关闭调用要求丢弃后不可回退；并发排空关闭也必须遵守该意图
    std::atomic<bool> m_discardRequested;
    // 分配器是否允许选择
    std::atomic<bool> m_accepting;
    // 关闭操作权，并发关闭在内部退避等待收尾
    std::atomic<bool> m_closing;
    // 所有线程已经完成注销
    std::atomic<bool> m_closed;
    // 成功增长计数
    std::atomic<uint64_t> m_grows;
    // 成功缩小计数
    std::atomic<uint64_t> m_shrinks;
    // 策略和系统异常计数
    std::atomic<uint64_t> m_errors;
    // 全池空闲起点，只由控制线程读写
    uint64_t m_idleSince;
    // 最近调整时刻，只由控制线程读写
    uint64_t m_lastResize;
    // 每成员连续空闲起点，只有控制线程读写；不属于共享并发容器
    std::map<uint32_t, uint64_t> m_idleStarts;
};