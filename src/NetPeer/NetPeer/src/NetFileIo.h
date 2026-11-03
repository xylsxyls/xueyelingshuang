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
    NET_FILE_BACKEND_RELEASING
};

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

    /** 兜底停止及回收，仅生命周期负责人调用，不在自身/网络回调中销毁
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

    /** 非阻塞协作停止，拒绝新工作，排队项回报CANCELLED
    */
    void stop();

    /** 工作执行者已释放全部句柄
    @return 是否已结束
    */
    bool done() const;

    /** done后释放后端；未完成时记录INVALID_STATE并保留上下文，稍后重试
    */
    void uninit();

    /** 最近初始化或后端异常结果，未完成uninit明确INVALID_STATE且保留后端
    @return NI结果；不代替每个工作自己的结果
    */
    int32_t status() const;

private:
    /** 有界检查可空路径，最多读取4097字节
    @param [in] path 调用方提供的NUL结尾路径
    @return 空指针或长度不超过4096
    */
    static bool validPath(const char* path);

    /** 结束一次后端借用，并执行其间被延迟的停止请求
    */
    void finishCall();

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
    // 本实例独占后端，uninit前不得释放宿主依赖
    NetFileIoBackend* m_backend;
    // 门面初始化/异常/生命周期违约结果
    int32_t m_error;
    // 唯一生命周期状态，stop抛异常也不能回到可接纳状态。
    NetFileIoPhase m_phase;
    // 非const后端调用正在借用自身，禁止回入提交或释放。
    bool m_calling;
};