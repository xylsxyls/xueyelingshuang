#pragma once
#include "CTask.h"
#include "CTaskThreadModels.h"
#include <map>
#include <memory>

class CTaskThreadState;

/** 单条工作线程门面；强引用可跨停止存活，停止后的投递明确拒绝
*/
class CTaskThreadManagerAPI CTaskThread
{
    friend class CTaskThreadManager;
    friend class CTaskManagerState;
    friend class CTaskPoolState;

private:
    /** 由管理器构造线程
    @param [in] threadId 不复用编号
    @param [in] config 初始化配置
    */
    CTaskThread(uint32_t threadId, const CTaskThreadConfig& config);

public:
    /** 在管理器已经等待退出后释放资源
    */
    ~CTaskThread();

    /** 接纳任务；竞争与可恢复容量不足在内部重试；容量退避采用该任务级别的 configureCapacityWait 设置
    @param [in] task 非空任务强引用
    @param [in] taskLevel 从 1 到 INT_MAX，大值先执行，同级 FIFO
    @return true已接纳但不表示执行完成，不能重发；false为未接纳，需要详细结果时改用postTaskResult投递
    */
    bool postTask(const std::shared_ptr<CTask>& task, int32_t taskLevel = 1);

    /** 按普通投递规则等待接纳，同时保留提交后通知错误的诊断信息
    @param [in] task 非空任务强引用
    @param [in] taskLevel 正数任务优先级
    @return m_committed为true时不可重复投递；查询具体诊断不能只检查m_status
    */
    CTaskThreadResult postTaskResult(const std::shared_ptr<CTask>& task, int32_t taskLevel = 1);

    /** 按开关尝试投递；默认不等待暂时占用的保护记录或发布机会
    @param [in] task 任务强引用
    @param [in] taskLevel 正数优先级
    @param [in] retryContention true持续重试CAS竞争，false允许返回未提交竞争状态
    @param [in] retryCapacity true 等待可复用容量；本级默认快速重试 4096 次，随后每次请求休眠 1 毫秒，可通过 configureCapacityWait 调整
    @return 是否已接纳；具体错误使用 tryPostTaskResult，普通投递使用 postTask
    */
    bool tryPostTask(const std::shared_ptr<CTask>& task, int32_t taskLevel = 1,
        bool retryContention = false, bool retryCapacity = false);

    /** 按开关尝试投递并返回具体失败原因；默认不持续重试竞争或容量不足
    @param [in] task 非空任务强引用
    @param [in] taskLevel 正数优先级
    @param [in] retryContention true持续重试CAS竞争，false允许返回未提交竞争状态
    @param [in] retryCapacity true 等待可复用容量；本级默认快速重试 4096 次，随后每次请求休眠 1 毫秒，可通过 configureCapacityWait 调整
    @return Busy 仅表示未提交的竞争失败；Capacity 表示额度用满，ResourceUnavailable 表示暂不可复用
    */
    CTaskThreadResult tryPostTaskResult(const std::shared_ptr<CTask>& task, int32_t taskLevel = 1,
        bool retryContention = false, bool retryCapacity = false);

    /** 观察是否有排队任务
    @return 本次弱一致观察中有排队项为true，不包含当前执行任务，不能作为完成或退出屏障
    */
    bool hasTask();

    /** 通知当前任务合作停止
    @return 通知结果
    */
    CTaskThreadResult stopCurTask();

    /** 通知属性 ID 匹配的任务；当前任务不受 taskLevel 过滤
    @param [in] propertyId 属性 ID，不要求唯一，通知所有匹配任务
    @param [in] taskLevel 排队优先级过滤，0 为全部
    @return 通知结果，不删除任务
    */
    CTaskThreadResult stopTask(int32_t propertyId, int32_t taskLevel = 0);

    /** 通知本次观察到的任务合作停止，不删除排队项；回调允许查询和投递
    @return 通知结果
    */
    CTaskThreadResult stopAllTask();

    /** 获取排队数量，先清空输出
    @param [out] counts 优先级到等待数量
    @return 运行中成功返回 Ok 和弱一致计数；观察入口关闭返回 Closed，保护记录不足返回 ResourceUnavailable，输出分配失败返回 NoMemory。失败时 counts 为空
    */
    CTaskThreadResult getWaitTaskInfo(std::map<int32_t, int32_t>& counts);

    /** 查询等待任务数量
    @return 弱一致排队数，包含预取未执行项且不含当前执行项；大于INT_MAX时饱和
    */
    int32_t getWaitTaskCount();

    /** 查询当前任务优先级
    @return 空闲为 0
    */
    int32_t getCurTaskLevel();

    /** 关闭接纳并等待全部任务完成；管理器同名接口还移除注册
    @return 自等待明确拒绝；并发 join 在内部等待，成功表示线程已经退出
    */
    CTaskThreadResult waitForEnd();

    /** 仅关闭接纳并请求退出，可从任务回调调用，不等待线程退出
    @param [in] drain true 排空，false 丢弃等待任务
    @return 意图已经发布；停止观察的临时竞争内部重试
    */
    CTaskThreadResult requestShutdown(bool drain = true);

    /** 设置系统线程优先级，与任务优先级无关
    @param [in] priority 系统等级
    @return 实际系统结果及原生错误码
    */
    CTaskThreadResult setSystemPriority(CTaskSystemPriority priority);

    /** 查询负载和错误计数
    @return 弱一致统计
    */
    CTaskThreadStatistics statistics() const;

    /** 取得管理器线程编号
    @return 不复用的编号
    */
    uint32_t threadId() const;

private:
    /** 创建线程
    @return 初始化结果
    */
    CTaskThreadResult createThread();

    /** 请求停止并等待
    @param [in] drain 是否排空
    @return 关闭结果
    */
    CTaskThreadResult finish(bool drain);

    /** 检查是否会同步等待自身
    @return 是否处于本线程的业务回调
    */
    bool selfWait() const;

private:
    // 不变编号
    const uint32_t m_threadId;
    // 独占状态；公开调用由门面的 shared_ptr 保证存活
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::unique_ptr<CTaskThreadState> m_state;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};