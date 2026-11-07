#pragma once
#include "CTaskThread.h"
#include <atomic>
#include <memory>

class CTaskManagerState;

/** 无锁线程注册与生命周期门面，保留 Init / ID / GetThreadInterface 使用方式
*/
class CTaskThreadManagerAPI CTaskThreadManager
{
private:
    /** 创建默认空注册表
    */
    CTaskThreadManager();

    /** 静止析构并停止仍注册的线程
    */
    ~CTaskThreadManager();

public:
    /** 首次调用时创建管理器；启动阶段由主线程串行预热，再启动并发调用方
    @return 当前链接模块内的唯一实例
    */
    static CTaskThreadManager& Instance();

    /** 静止且没有线程时配置注册表
    @param [in] config Map 初始化参数
    @return 配置结果；调用者保证没有并发公开调用
    */
    CTaskThreadResult configure(const CTaskThreadManagerConfig& config);

    /** 使用默认配置创建线程
    @return 非零不复用 ID，失败为 0；不得在 Windows DLL loader lock 下调用
    */
    uint32_t Init();

    /** 使用指定参数创建线程
    @param [in] config 线程的 Map、Queue 及系统优先级配置
    @return 非零编号，失败为 0
    */
    uint32_t Init(const CTaskThreadConfig& config);

    /** 有详细结果的初始化接口
    @param [in] config 线程配置，调用期间借用并复制
    @param [out] threadId 失败清零，成功为不复用编号
    @return 线程已经启动并登记时 committed 为 true
    */
    CTaskThreadResult initialize(const CTaskThreadConfig& config, uint32_t& threadId);

    /** 关闭接纳、排空并注销线程
    @param [in] threadId 线程编号
    @return 关闭结果
    */
    CTaskThreadResult WaitForEnd(uint32_t threadId);

    /** 关闭接纳、停止当前任务、丢弃排队任务并注销
    @param [in] threadId 线程编号
    @return 关闭结果
    */
    CTaskThreadResult Uninit(uint32_t threadId);

    /** 先通知全部线程停止，再等待退出；期间新 Init 被拒绝
    @return 部分收尾失败时保留可重试的注册引用
    */
    CTaskThreadResult UninitAll();

    /** 安全取得线程强引用，停止后旧引用仍可查询但不能投递
    @param [in] threadId 线程编号
    @return 未找到或资源暂不可用时为空
    */
    std::shared_ptr<CTaskThread> GetThreadInterface(uint32_t threadId);

    /** 查询注册线程数
    @return 弱一致数量，超过 INT_MAX 饱和
    */
    int32_t Count();

private:
    /** 并发创建完整私有状态后 CAS 发布；失败返回空，不发布半成品
    @return 运行期间地址不变的私有状态
    */
    CTaskManagerState* state();

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