#pragma once
#include "CTaskThreadModels.h"
#include <atomic>

class CTaskThreadPool;
class CTaskThread;

/** 独立分配器；只选择目标 ID，由上层调用该线程的 postTask
*/
class CTaskThreadManagerAPI CTaskThreadAllocator
{
public:
    /** 绑定线程池，不拥有它
    @param [in,out] pool 必须活得比分配器及其所有调用更久
    */
    explicit CTaskThreadAllocator(CTaskThreadPool& pool);

    /** 按当前未完成任务数选择目标，同负载轮转
    @param [out] threadId 成功为目标编号，失败为 0
    @return 选择结果；postTask 未接纳时可重新选取，不重发已接纳任务
    */
    CTaskThreadResult select(uint32_t& threadId);

    /** 同时返回安全强引用，避免上层再次查询管理器
    @param [out] threadId 目标编号
    @param [out] thread 线程强引用；选择不预留额度，缩容仍可能关闭接纳
    @return 选择结果
    */
    CTaskThreadResult select(uint32_t& threadId, std::shared_ptr<CTaskThread>& thread);

private:
    // 借用线程池
    CTaskThreadPool* m_pool;
    // 相同负载的轮转提示，回绕不涉及资源身份
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<uint64_t> m_turn;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};