#pragma once
#include "NetPeerMacro.h"
#include "NetFileIoBackend.h"

// 文件后端由同一执行者管理；停止标记不依赖后端是否正常完成stop。
enum NetFileIoPhase
{
    // 尚未拥有后端或已经完整释放。
    NET_FILE_BACKEND_IDLE,
    // 工厂或初始化正在进行，不接纳任务。
    NET_FILE_BACKEND_STARTING,
    // 后端已准备，可以接纳新编号和任务。
    NET_FILE_BACKEND_ACTIVE,
    // 拒绝新任务，但允许取消及消费已有结果。
    NET_FILE_BACKEND_STOPPING,
    // done后正在释放，禁止后端回入释放自身。
    NET_FILE_BACKEND_RELEASING,
    // 已请求停止，待当前宿主调用结束后转交通知；宿主抛异常可重试。
    NET_FILE_BACKEND_STOP_PENDING
};

struct NetPeerFileIoState;

// 无线程文件协议门面；所有平台须显式提供后端，原生默认适配由NetWork配置
class NetPeerAPI NetFileIo
{
public:
    /** 构造不启动线程
    */
    NetFileIo();

    /** 应用启动前一次配置后端工厂，工作线程启动后禁止改动
    @param [in] factory 非空，返回新独占后端；上下文由宿主保持至全部使用者退出
    @return OK、INVALID_ARGUMENT或INVALID_STATE，不启动后端
    */
    static int32_t configureBackend(NetFileIoBackendFactory factory);

    /** 生命周期负责人最终停止并回收后端，可能等待工作线程；禁止在后端调用、网络回调或工作线程中销毁
    */
    ~NetFileIo();

    /** 创建并初始化后端；未配置后端时返回NOT_SUPPORTED
    @return NI_OK或负数，可在uninit后重新初始化
    */
    int32_t init();

    /** 在多个流之间分配本工作队列唯一编号，不回退复用
    @param [out] id 成功非零，失败0
    @return OK或负数；之后submit拒绝也消耗该编号
    */
    int32_t nextRequestId(uint64_t& id);

    /** 复制任务至有界队列；生产者只有一个网络/业务执行者
    @param [in] request 参数仅调用时借用，READ/WRITE最大65536，路径最大4096
    @return OK已接纳，LIMIT_EXCEEDED未接纳；不是I/O已完成
    */
    int32_t submit(const NetFileIoRequest& request);

    /** 请求取消排队/执行中工作，系统单次I/O协作结束后回报
    @param [in] id 当前任务号
    @return OK或NOT_FOUND，不能撤销已经发生的磁盘副作用
    */
    int32_t cancel(uint64_t id);

    /** 借用一个完成结果，不等待工作线程
    @param [out] result 成功结果，否则清零
    @return READY结果，NEED_MORE暂无
    */
    int32_t peek(NetFileIoResult& result);

    /** 仅在结果已消费后删除任务及擦除缓存
    @param [in] id 当前peek的编号
    @return OK或INVALID_STATE
    */
    int32_t consume(uint64_t id);

    /** 非阻塞请求停止，拒绝新工作；后端调用期间推迟通知，已成功通知后可重复调用
    */
    void stop();

    /** 查询能否接管新文件流，须在同一执行者上调用
    @return 后端处于活动状态且明确回报尚未结束时为true
    */
    bool accepting() const;

    /** 查询工作执行者是否已释放全部句柄；后端回入查询返回false
    @return 是否已结束
    */
    bool done() const;

    /** done后释放后端；未完成时记录INVALID_STATE并保留上下文，稍后重试
    */
    void uninit();

    /** 查询文件后端是否已完全释放，可再次初始化
    @return 初始空闲或已完成uninit时为true
    */
    bool released() const;

    /** 最近初始化或后端异常结果，未完成uninit明确INVALID_STATE且保留后端
    @return NI结果；不代替每个工作自己的结果
    */
    int32_t status() const;

private:
    /** 借用后端查询是否结束，查询返回后处理其间的停止请求
    @return READY已结束，NEED_MORE仍在运行，INVALID_STATE表示查询失败或期间请求停止
    */
    int32_t queryCompletion() const;

    /** 结束一次后端借用，并执行其间被延迟的停止请求
    */
    void finishCall() const;

private:
    /** 禁止复制所有权
    @param [in] other 原对象
    */
    NetFileIo(const NetFileIo& other);

    /** 禁止赋值
    @param [in] other 原对象
    @return 不可调用
    */
    NetFileIo& operator=(const NetFileIo& other);

private:
    // 启动前配置一次，之后只读的宿主选择
    static NetFileIoBackendFactory m_factory;
    // 当前初始化周期独占的后端状态；空指针表示已完整释放。
    NetPeerFileIoState* m_state;
    // 空闲时保留最近一次初始化或宿主错误，直到下次初始化。
    int32_t m_idleError;
};