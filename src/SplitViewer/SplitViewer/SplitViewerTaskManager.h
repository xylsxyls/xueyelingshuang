#pragma once
#include "SplitViewerTaskContext.h"
#include <QtCore/QObject>
class QTimer;

/** GUI提交与呈现边界，一次仅接纳一个请求；不在线程间传递QWidget
*/
class SplitViewerTaskManager : public QObject
{
public:
    /** 初始化轻量提交状态及GUI计时器，队列由init分配
    @param [in] parent GUI拥有者，在GUI线程销毁
    */
    explicit SplitViewerTaskManager(QObject* parent);

    /** 提出停止并交给协调者回收，不等待后台
    */
    ~SplitViewerTaskManager();

    /** 显式分配本窗口队列，失败不接纳请求并可安全stop
    @return 是否完成两个队列初始化
    */
    bool init();

    /** 提交独立快照，拒绝停止中或尚有请求的情况
    @param [in] longRunning 是否送耗时执行队列
    @param [in] work 独立数据上的业务函数；不可触碰Qt窗口及应用配置
    @param [in] complete GUI结果呈现；本对象销毁后不再调用
    @return 是否接纳，失败没有执行业务
    */
    bool submit(bool longRunning, const std::function<bool(const std::atomic<bool>&)>& work,
        const std::function<void(bool)>& complete);

    /** 关闭期停止接纳并异步回收；可重复调用
    */
    void stop();

    /** 观察已接纳请求或关闭期尚未回收的队列
    @return 是否仍需保留GUI事件循环
    */
    bool busy() const;

private:
    /** 仅在GUI线程取出实际完成结果并调用呈现函数
    */
    void poll();

private:
    // 在途任务共享上下文，销毁GUI不释放后台数据
    std::shared_ptr<SplitViewerTaskContext> m_context;
    // GUI父子关系拥有的结果计时器
    QTimer* m_timer;
    // 单请求接纳状态，只由GUI访问
    bool m_busy;
    // 幂等停止标志，只由GUI访问
    bool m_stopping;
    // 每窗口请求序列，不在后台递增
    uint64_t m_nextId;
};