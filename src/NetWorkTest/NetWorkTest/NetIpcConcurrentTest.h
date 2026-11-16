#pragma once
#include "NetSenderTestAccess.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>

// 测试生命周期owner，先收敛所有回调生产者，再释放收件箱。
class NetIpcConcurrentContext
{
public:
    /** 清零16个生产者的进度和任务句柄
    */
    NetIpcConcurrentContext();

    /** 所有异常路径也等待本用例创建的任务停止
    */
    ~NetIpcConcurrentContext();

    /** 停止新回调并加入任务，重复调用安全
    @return 每个已创建任务均成功收敛
    */
    bool join();
public:
    // 真实网关收件箱，接收函数由16个任务并发调用。
    NetSenderLocalInbox m_inbox;
    // owner核对一条后归还发送窗口，生产者只读取自己的元素。
    std::atomic<uint32_t> m_received[16];
    // 每个生产者真实提交回调的次数，结束时核对不遗漏。
    std::atomic<uint32_t> m_sent[16];
    // 任务完成计数，不代替join。
    std::atomic<uint32_t> m_done;
    // 所有任务进入后统一放行，避免把线程创建先后误记为并发公平性。
    std::atomic<uint32_t> m_ready;
    // owner确认16个任务已就绪后发布。
    std::atomic<bool> m_start;
    // 所有生产者的协作退出标记。
    std::atomic<bool> m_stop;
    // 生产者编码或调度失败，owner持续观察。
    std::atomic<bool> m_bad;
    // 仅生命周期owner修改的CTask编号。
    uint32_t m_threads[16];
};

// 每个生产者只有自己的顺序流，四包窗口与正式IPC约定相同。
class NetIpcConcurrentProducer : public CTask
{
public:
    /** 借用上下文，owner在任务加入前不销毁
    @param [in] context 本用例上下文
    @param [in] index 独立生产者号0至15
    */
    NetIpcConcurrentProducer(NetIpcConcurrentContext& context, uint32_t index);

    /** 在当前CTask线程按顺序投递256条，窗口满时等owner消费
    */
    virtual void doTask();

    /** 协作停止整轮用例，唤醒窗口等待以便收敛
    */
    virtual void stopTask();
private:
    // 借用至任务退出的上下文。
    NetIpcConcurrentContext* m_context;
    // 构造后不变的流编号。
    uint32_t m_index;
};