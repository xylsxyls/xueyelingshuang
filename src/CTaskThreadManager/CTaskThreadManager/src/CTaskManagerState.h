#pragma once
#include "CTaskThread.h"
#include "CTaskAdmission.h"

/** 管理器私有注册状态，构造完成后一次性发布，运行期间不移动
*/
class CTaskManagerState
{
    friend class CTaskThreadTestAccess;
public:
    /** 创建空注册表
    */
    CTaskManagerState();

    /** 静止期停止全部线程
    */
    ~CTaskManagerState();

    /** 静止期配置注册表
    @param [in] config 初始化参数
    @return 配置结果
    */
    CTaskThreadResult configure(const CTaskThreadManagerConfig& config);

    /** 创建并登记线程
    @param [in] config 线程参数
    @param [out] threadId 成功编号
    @return 接纳结果
    */
    CTaskThreadResult initialize(const CTaskThreadConfig& config, uint32_t& threadId);

    /** 关闭并注销指定线程
    @param [in] threadId 编号
    @param [in] drain 是否排空
    @return 关闭结果
    */
    CTaskThreadResult finish(uint32_t threadId, bool drain);

    /** 先通知全部线程，再等待退出
    @return 关闭结果
    */
    CTaskThreadResult shutdown();

    /** 查询强引用
    @param [in] threadId 编号
    @return 未找到或暂不可用时为空
    */
    std::shared_ptr<CTaskThread> get(uint32_t threadId);

    /** 查询注册数
    @return 弱一致数量
    */
    int32_t count();
private:
    /** 分配单调编号
    @return 耗尽返回 0
    */
    uint32_t nextThreadId();
private:
    // 有序注册表，允许并发维护
    LockFreeMap<uint32_t, std::shared_ptr<CTaskThread>> m_threads;
    // 创建调用的接纳门
    CTaskAdmission m_creators;
    // 一次性批量关闭操作权
    std::atomic<bool> m_bulk;
    // 永不回绕的编号
    std::atomic<uint32_t> m_threadId;
};