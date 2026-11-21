#pragma once
#include "CTaskThread.h"
#include <atomic>
#include <memory>

class CTaskManagerState;

/** 原子线程注册与生命周期门面；批量注销包含串行收尾协调
*/
class CTaskThreadManagerAPI CTaskThreadManager
{
private:
    /** 创建空管理器门面；首次配置或创建线程时再分配注册表
    */
    CTaskThreadManager();

    /** 静止析构并停止仍注册的线程
    */
    ~CTaskThreadManager();

public:
    /** 首次调用时创建管理器；启动阶段由主线程串行预热，再启动并发调用方
    @return 当前链接模块内的唯一实例
    */
    static CTaskThreadManager& instance();

    /** 静止且没有线程时配置注册表及无参 init 的默认工作线程
    @param [in] config 注册表及默认工作线程参数，调用成功时复制
    @return 配置结果；调用者保证没有并发公开调用
    */
    CTaskThreadResult configure(const CTaskThreadManagerConfig& config);

    /** 使用默认配置创建线程
    @return 非零不复用 ID，失败为 0；不得在 Windows DLL loader lock 下调用
    */
    uint32_t init();

    /** 使用指定参数创建线程
    @param [in] config 线程的 Map、Queue 及系统优先级配置
    @return 非零编号，失败为 0
    */
    uint32_t init(const CTaskThreadConfig& config);

    /** 有详细结果的初始化接口
    @param [in] config 线程配置，调用期间借用并复制
    @param [out] threadId 失败清零，成功为不复用编号
    @return 线程已经启动并登记时 committed 为 true
    */
    CTaskThreadResult initialize(const CTaskThreadConfig& config, uint32_t& threadId);

    /** 关闭接纳、排空并注销线程
    @param [in] threadId 线程编号
    @return Ok 表示退出及注销都完成，竞争内部重试
    */
    CTaskThreadResult waitForEnd(uint32_t threadId);

    /** 关闭接纳、停止当前任务、丢弃排队任务并注销
    @param [in] threadId 线程编号
    @return Ok 表示退出及注销都完成。停止回调异常时仍完成 join 和移除登记，并返回 CallbackFailure；竞争内部重试
    */
    CTaskThreadResult uninit(uint32_t threadId);

    /** 先通知全部线程停止，再等待退出；期间新 init 被拒绝
    @return 竞争内部重试；实际错误保留尚未清理的注册引用
    */
    CTaskThreadResult uninitAll();

    /** 安全取得线程强引用，停止后旧引用仍可查询但不能投递
    @param [in] threadId 线程编号
    @return 未找到或发生实际错误时为空，内部竞争不会直接返回空
    */
    std::shared_ptr<CTaskThread> getThreadInterface(uint32_t threadId);

    /** 查询注册线程数
    @return 弱一致数量，超过 INT_MAX 饱和
    */
    int32_t count();

private:
    /** 静止期应用指定配置，或为创建线程取得完整注册表；首次失败留待下次重新创建
    @param [out] current 成功时借用至管理器析构；失败时调用方不得使用
    @param [in] config 非空表示静止期显式配置，空表示沿用已发布状态或按默认值创建
    @return 配置或初始化结果，保留注册表原始错误类别
    */
    CTaskThreadResult prepareState(CTaskManagerState*& current, const CTaskThreadManagerConfig* config);

private:
    // 稳定私有状态，析构时要求外部静止
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<CTaskManagerState*> m_state;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};