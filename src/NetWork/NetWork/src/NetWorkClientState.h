#pragma once
#include "NetWorkTypes.h"
#include <NetPeer/NetPeerAPI.h>
#include "NetInterface/NetInterfaceAPI.h"
#include <CTaskThreadManager/CTaskThreadManagerAPI.h>
#include <mutex>
#include <atomic>
#include "NetWorkPlatform.h"

struct NetWorkClientOpaque;

// 每个客户端只投递一次长任务；停止仅提交关闭，不在回调中等待
class NetWorkClientTask : public CTask
{
public:
    /** 借用客户端，直到任务退出且生命期owner回收线程
    @param [in] client 唯一上下文
    */
    explicit NetWorkClientTask(NetWorkClientOpaque* client);

    /** 在本CTask中独占推进Peer，所有用户回调也在此处
    */
    virtual void DoTask();

    /** 只标记关闭，由DoTask收敛
    */
    virtual void StopTask();
private:
    // 生命期owner在回收本任务之前保留
    NetWorkClientOpaque* m_client;
};

// 仅本库分配／释放；生产者只访问submission，不能访问Peer
struct NetWorkClientOpaque
{
public:
    // 先声明门，端口析构期间仍可使用；所有原生门在创建时验证，停止不再分配。
    NetWorkGate m_submitMutex;
    // 只唤醒协议owner；通知标志与检查、等待在本门下配对。
    NetWorkGate m_wakeGate;
    // 同步交付回执有独立等待门，不能抢走协议owner的工作通知。
    NetWorkGate m_sendGate;
    // 合并重复通知
    std::atomic<bool> m_notified;
    // 从投递接纳至任务最后一条访问；拒绝提前释放
    std::atomic<bool> m_running;
    // 任务实际开始，不能用PostTask返回代替
    std::atomic<bool> m_entered;
    // 非零为协议执行者的身份；Wasm使用完整pthread指针值，不能在wasm64截断
    std::atomic<uintptr_t> m_owner;
    // 最近内部错误；提交错误由各次调用直接返回
    std::atomic<int32_t> m_error;
    // 统一有界接纳，只有其短临界区可被生产者访问
    NetSenderSubmission m_submission;
    // 用户表只读，context由外部保留
    NetWorkClientCallbacks m_callbacks;
    // 只在创建／连接生命期owner或已移交的协议执行者访问
    NetPeerClientHandle m_peer;
    // 仅生命期owner回收，不能被外部线程池停止
    uint32_t m_thread;

public:
    /** 初始化静止上下文
    @param [in] callbacks 完整v1回调，context借用到释放
    */
    explicit NetWorkClientOpaque(const NetWorkClientCallbacks& callbacks);

    /** 仅在任务、Peer及所有外部调用均已收敛后释放
    */
    ~NetWorkClientOpaque();

    /** 进入生产者提交短锁，不在锁内执行协议或用户回调
    @param [in] context 本实例，提交端口销毁前保持有效
    */
    static void enter(void* context);

    /** 释放生产者提交短锁
    @param [in] context 与enter配对的本实例
    */
    static void leave(void* context);

    /** 释放提交锁后合并通知协议执行者
    @param [in] context 本实例，全部外部调用退出前保持有效
    */
    static void wake(void* context);

    /** 创建独占CTask并投递一次任务；不把投递视作协议就绪
    @return OK仅本地接纳，失败保存可观察错误
    */
    int32_t start();

    /** 生命期owner回收已退出的独占任务；不能由GUI／回调调用
    @return 活跃时INVALID_STATE，否则OK
    */
    int32_t reap();

    /** 生产者等待本次同步交付；不持提交锁，禁止owner或回调线程调用
    @param [in] ticket 当前同步票据
    @return 交付结果，不等待远端业务结果
    */
    int32_t waitSynchronous(uint64_t ticket);

    /** 判断调用者是否为本协议执行者
    @return 是否同一系统线程
    */
    bool onOwner() const;

    /** 读取当前执行线程的身份，只用于存活期间判断回调重入
    @return 原生系统线程号或Wasm pthread身份；不作为跨进程/持久身份
    */
    static uintptr_t currentThreadId();

    /** 在协议执行者转交本代就绪回调
    @param [in] context 本实例
    @param [in] generation 本次非零会话代次
    @param [in] requests 协商的最大并行请求数
    @param [in] chunk 协商的最大数据块字节数
    */
    static void ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk);

    /** 在协议执行者转交已关联的业务结果
    @param [in] context 本实例
    @param [in] generation 结果所属代次
    @param [in] id 本代请求号
    @param [in] product 产品号
    @param [in] type 业务消息类型
    @param [in] status 业务结果码
    @param [in] bytes 回调期间借用的结果字节，可为空仅当size为0
    @param [in] size 结果字节数
    */
    static void result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 在协议执行者转交通道帧，保留上层背压语义
    @param [in] context 本实例
    @param [in] generation 帧所属代次
    @param [in] value 回调期间借用的帧及正文
    @return 用户帧回调结果；非0已消费，0未消费
    */
    static int32_t frame(void* context, uint64_t generation, const NetInterfaceFrame* value);

    /** 在协议执行者转交本代终态通知
    @param [in] context 本实例
    @param [in] generation 已关闭的代次
    @param [in] reason NI关闭原因
    @param [in] detail 下层诊断码，不作为业务结果码
    */
    static void closed(void* context, uint64_t generation, int32_t reason, int32_t detail);
};