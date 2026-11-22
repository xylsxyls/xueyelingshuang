#pragma once
#include "NetInterface/NetInterfaceAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

struct NetWorkClientOpaque;

/** 生命周期命令，调用者保留到工作线程发布完成；业务发送不创建此对象
*/
struct NetWorkWorkerCall
{
    // 仅调用者构造、工作线程执行的操作，借用至完成
    const std::function<int32_t()>* m_action;
    // 工作线程在最后一次访问时发布完成，调用者随后才能释放
    std::atomic<bool> m_done;
    // 在m_done发布前写入的操作结果
    int32_t m_result;

    /** 初始化一次生命周期交接
    @param [in] action 在调用结束前有效的操作
    */
    explicit NetWorkWorkerCall(const std::function<int32_t()>& action);
};

/** 一个CTask执行多个会话和它们的网络循环；同一会话不跨执行者并发
*/
class NetWorkWorker : public CTask
{
public:
    /** 创建静止工作者，start后移交执行循环
    */
    NetWorkWorker();

    /** 所有客户端已退出后停止并回收线程
    */
    ~NetWorkWorker();

    /** 启动前设置并行send的线程数，串行post独占另外一条执行者
    @param [in] count 非零线程数；实际线程按使用惰性创建
    @return NI_OK或参数、资源、生命周期错误
    */
    static int32_t configure(uint32_t count);

    /** 生命周期owner为新会话选择执行者；禁止与其他创建/销毁并发
    @param [in] parallel true轮转选择send执行者，false共用post执行者
    @return 已启动工作者，失败为空
    */
    static std::shared_ptr<NetWorkWorker> acquire(bool parallel);

    /** 唯一生命周期owner启动CTask
    @return 初始化或投递结果
    */
    int32_t start();

    /** 在执行者中运行生命周期操作，外部调用等待完成，禁止从业务回调调用
    @param [in] action 只在本执行者调用的操作
    @return 操作结果或队列/资源错误
    */
    int32_t invoke(const std::function<int32_t()>& action);

    /** owner在连接启动前准备一个会话登记位置，避免连接已启动才分配失败
    @return NI_OK或资源错误
    */
    int32_t prepareClient();

    /** owner登记已接纳连接，下一轮公平推进
    @param [in] client 借用到其m_running最后置false
    @return NI_OK或资源错误
    */
    int32_t add(NetWorkClientOpaque* client);

    /** 从任意提交线程唤醒，必须在销毁前收敛这些调用
    */
    void wake();

    /** 生命周期线程判断是否正在本执行者重入
    @return 当前线程是否本执行者
    */
    bool onOwner() const;

    /** 在一个共享loop中轮流推进会话，空闲阻塞等待真实事件
    */
    virtual void doTask();

    /** 请求合作停止，不访问owner独占的会话容器
    */
    virtual void stopTask();

private:
    // 先于线程创建，晚于线程回收；本worker全部连接共享一个事件循环
    NetExecution m_execution;
    // 多个生命周期调用者向唯一owner交接命令，业务消息使用各自submission
    LockFreeQueue<NetWorkWorkerCall*> m_commands;
    // 唯一owner使用的无序活动会话；移除以末尾补位，不影响单会话发送顺序
    std::vector<NetWorkClientOpaque*> m_clients;
    // 实际任务线程登记，析构时注销
    uint32_t m_thread;
    // 线程身份只用于拒绝回调重入
    std::atomic<uintptr_t> m_owner;
    // 生命周期交接中的调用数，停止后收敛已接纳命令再退出
    std::atomic<uint32_t> m_invoking;
    // 合作停止标志，stopTask仅写此项并唤醒
    std::atomic<bool> m_stopping;
    // 仅生命周期owner读写的弱登记，不延长工作线程寿命
    static std::vector<std::weak_ptr<NetWorkWorker>> s_workers;
    // 并行线程配置，不包含串行post执行者
    static uint32_t s_count;
    // 下一次并行分配的位置，按连接分配而非按消息迁移
    static uint32_t s_next;
};