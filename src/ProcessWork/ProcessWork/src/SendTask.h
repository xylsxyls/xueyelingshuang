#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "ProcessWorkLocalQueue.h"

class Semaphore;
class ProcessWork;
class ProcessWorkSendCache;
/** 唯一后台Post发送任务；保留待发正文至共享提交、终止错误或显式取消
*/
class SendTask : public CTask
{
public:
    /** 初始化空借用和可启动状态
    */
    SendTask();

public:
    /** 逐项发送已接纳的本地副本；资源恢复重试保持原顺序，停止时归还未发送正文
    */
    void doTask() override;

    /** 发布停止标志并唤醒等待，发送循环协作取消；不在本函数等待任务退出
    */
    void stopTask() override;

    /** 启动前绑定资源，借用对象须保持到任务退出并完成线程回收
    @param [in] waitEndPost 借用的排空标志，true表示无待发消息后退出；nullptr使任务直接返回
    @param [in] postQueue 借用的唯一消费队列，nullptr使任务直接返回
    @param [in] ready false时等待start，在任务投递成功后启用
    @param [in] owner 借用的发送门面，空指针使用单例
    @param [in] cache 与owner相同的通道缓存，可空；借用期覆盖任务退出
    */
    void setParam(std::atomic<bool>* waitEndPost, ProcessWorkLocalQueue* postQueue, bool ready = true,
        ProcessWork* owner = nullptr, ProcessWorkSendCache* cache = nullptr);

    /** 初始化全部成功后允许消费队列；不会唤醒或改变停止中的任务
    */
    void start();

    /** 取得当前执行线程的协作停止标记，普通同步调用返回nullptr
    @return 仅本线程借用，作用域由doTask保持
    */
    static const std::atomic<bool>* currentStopFlag();

    /** 在任务作用域内绑定停止标记，不引入进程级共享锁
    @param [in] flag 本线程当前任务的标记，nullptr解除绑定
    @return 原标记，离开作用域时恢复
    */
    static const std::atomic<bool>* setCurrentStopFlag(const std::atomic<bool>* flag);

private:
    // 绑定本任务所属的ProcessWork，保持独立实例的发送归属
    ProcessWork* m_owner;
    // 借用门面的快缓存，只在一个已就绪前缀内复用通道保护
    ProcessWorkSendCache* m_cache;
    // 借用排空标记，外部先停止新增post
    std::atomic<bool>* m_waitEndPost;
    // 借用本地待发队列，任务退出前不得销毁
    ProcessWorkLocalQueue* m_postQueue;
    // 跨线程协作停止标记，不强行终止发送代码
    std::atomic<bool> m_exit;
    // Post任务完成初始化前，不消费调用方已经排队的消息
    std::atomic<bool> m_ready;
};