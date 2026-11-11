#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "ProcessWorkLocalQueue.h"

class Semaphore;
class ProcessWork;
class ProcessWorkSendCache;
/** 发送任务
*/
class SendTask : public CTask
{
public:
    /** 构造函数
    */
    SendTask();

public:
    /** 执行任务
    */
    void doTask() override;

    /** 中断任务
    */
    void stopTask() override;

    /** 设置参数
    @param [in] waitEndPost 发送结束后是否退出循环
    @param [in] postQueue 发送数据队列
    @param [in] ready false时等待start，在任务投递成功后启用
    @param [in] owner 借用的发送门面，活到任务加入完成；空指针使用单例
    @param [in] cache 与owner相同的通道缓存，可空；借用期覆盖任务退出
    */
    void setParam(std::atomic<bool>* waitEndPost, ProcessWorkLocalQueue* postQueue, bool ready = true,
        ProcessWork* owner = nullptr, ProcessWorkSendCache* cache = nullptr);

    /** 初始化全部成功后允许消费队列；不会唤醒或改变停止中的任务
    */
    void start();

    /** 取得当前执行线程的协作停止标记，普通同步调用返回nullptr
    @return 仅本线程借用，作用域由DoTask保持
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