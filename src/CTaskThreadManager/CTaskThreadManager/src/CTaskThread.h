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

    /** 接纳任务；原来忽略返回值的调用仍可编译，新代码检查 committed
    @param [in] task 非空任务强引用
    @param [in] taskLevel 从 1 到 INT_MAX，大值先执行，同级 FIFO
    @return 以 committed 判断是否接纳，后续抢占通知失败可与 committed=true 同时返回
    */
    CTaskThreadResult PostTask(const std::shared_ptr<CTask>& task, int32_t taskLevel = 1);

    /** 尝试投递
    @param [in] task 任务强引用
    @param [in] taskLevel 正数优先级
    @return 是否已接纳，详细结果使用 PostTask
    */
    bool TryPostTask(const std::shared_ptr<CTask>& task, int32_t taskLevel = 1);

    /** 观察是否有排队任务
    @return 不包含当前执行任务
    */
    bool HasTask();

    /** 通知当前任务合作停止
    @return 通知结果
    */
    CTaskThreadResult StopCurTask();

    /** 通知属性 ID 匹配的任务；保留历史规则，当前任务不受 taskLevel 过滤
    @param [in] propertyId 属性 ID，不要求唯一，通知所有匹配任务
    @param [in] taskLevel 排队优先级过滤，0 为全部
    @return 通知结果，不删除任务
    */
    CTaskThreadResult StopTask(int32_t propertyId, int32_t taskLevel = 0);

    /** 通知所有任务合作停止，回调允许查询和投递
    @return 通知结果
    */
    CTaskThreadResult StopAllTask();

    /** 获取排队数量，先清空输出
    @param [out] counts 优先级到等待数量
    @return 查询结果，运行中为弱一致观察
    */
    CTaskThreadResult GetWaitTaskInfo(std::map<int32_t, int32_t>& counts);

    /** 查询等待任务数量
    @return 大于 INT_MAX 时饱和
    */
    int32_t GetWaitTaskCount();

    /** 查询当前任务优先级
    @return 空闲为 0
    */
    int32_t GetCurTaskLevel();

    /** 关闭接纳并等待全部任务完成；管理器同名接口还移除注册
    @return 自等待或同时 join 返回明确失败
    */
    CTaskThreadResult WaitForEnd();

    /** 仅关闭接纳并请求退出，可从任务回调调用，不等待
    @param [in] drain true 排空，false 丢弃等待任务
    @return 意图已经发布
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

    /** 发布退出意图，不等待
    */
    void requestStop();

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