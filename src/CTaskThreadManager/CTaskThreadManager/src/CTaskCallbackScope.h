#pragma once

/** 当前线程的回调借用栈，记录间接自等待和重复停止通知
*/
class CTaskCallbackScope
{
public:
    /** 在调用栈上登记一次借用，不分配内存
    @param [in] owner 借用状态的地址
    @param [in] stopped 正在执行 StopTask 的任务地址，可为空
    */
    explicit CTaskCallbackScope(const void* owner, const void* stopped = nullptr);

    /** 恢复上一层借用
    */
    ~CTaskCallbackScope();

    /** 查询当前调用栈是否持有指定状态
    @param [in] owner 状态地址
    @return 是否持有
    */
    static bool contains(const void* owner);

    /** 查询任务是否正在当前调用栈中接受停止通知
    @param [in] task 任务地址
    @return 是否正在通知
    */
    static bool stopping(const void* task);

    /** 查询当前调用栈是否持有任何受保护状态
    @return 是否持有
    */
    static bool active();

private:
    /** 禁止复制栈节点
    @param [in] other 原节点
    */
    CTaskCallbackScope(const CTaskCallbackScope& other) = delete;

    /** 禁止赋值栈节点
    @param [in] other 原节点
    @return 当前节点
    */
    CTaskCallbackScope& operator=(const CTaskCallbackScope& other) = delete;

private:
    // 每个系统线程独立的借用栈顶，所有翻译单元共享同一 TLS 符号
#ifdef _MSC_VER
    static __declspec(thread) CTaskCallbackScope* m_current;
#else
    static thread_local CTaskCallbackScope* m_current;
#endif
    // 借用状态
    const void* m_owner;
    // 接受停止通知的任务
    const void* m_stopped;
    // 上一层调用栈节点
    CTaskCallbackScope* m_previous;
};

#include "CTaskCallbackScope.inl"