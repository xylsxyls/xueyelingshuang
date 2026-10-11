#pragma once
#include "SplitViewerTaskContext.h"

/** 进程唯一的线程回收拥有者，只回收本工程创建的ID
*/
class SplitViewerTaskLifecycleManager
{
public:
    /** 显式启动后取得协调者
    @return 进程内唯一拥有者
    */
    static SplitViewerTaskLifecycleManager& instance();

    /** 在GUI启动阶段分配协调队列
    @return 是否取得有效队列
    */
    bool init();

    /** 提交上下文回收，不在调用方等待
    @param [in] context 本窗口创建且尚未回收的两个ID
    */
    void retire(const std::shared_ptr<SplitViewerTaskContext>& context);

    /** 事件循环及全部窗口结束后，外层受控等待协调Task返回
    */
    void finish();

private:
    /** 初始化未启动状态
    */
    SplitViewerTaskLifecycleManager();

    /** 禁止复制进程拥有者
    @param [in] other 不可复制的源对象
    */
    SplitViewerTaskLifecycleManager(const SplitViewerTaskLifecycleManager& other);

    /** 禁止赋值进程拥有者
    @param [in] other 不可赋值的源对象
    @return 不提供实现
    */
    SplitViewerTaskLifecycleManager& operator=(const SplitViewerTaskLifecycleManager& other);

private:
    // 仅GUI启动/提交及外层最终清理访问
    uint32_t m_retireId;
};