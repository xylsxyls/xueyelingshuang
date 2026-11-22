#pragma once
#include "NetFileIoBackend.h"
#include "NetWorkFilePools.h"
#include <stdint.h>
#include <memory>

class NetFileIoState;

// nextRequestId/submit/cancel/peek/consume由同一协议owner串行调用，StopTask仅操作原子停止标志。
// 由SDK拥有的一名CTask文件执行者。记录、正文和文件句柄由实际池控制；网络线程不执行磁盘I/O。
class NetNativeFileIo : public NetFileIoBackend
{
  public:
    /** 构造不启动线程
    */
    NetNativeFileIo();

    /** 为Peer创建独占原生文件后端，不启动任务线程
    @return 由NetFileIo持有并释放的新后端；分配失败由调用边界转换
    */
    static NetFileIoBackend* create();

    /** 兜底停止及回收，仅生命周期负责人调用，不在自身/网络回调中销毁
    */
    ~NetNativeFileIo();

    /** 创建任何文件后端前配置默认实际池；生命周期owner串行调用
    @param [in] pools 各组件配置，失败保持原配置
    @return NI_OK或配置、内存、状态错误
    */
    static int32_t configurePools(const NetWorkFilePools& pools);

    /** 创建CTask执行者
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

    /** 最终回收线程；调用前stop并在生命周期执行者等待done，禁止GUI/网络线程长等待
    */
    void uninit();

  private:
    /** 禁止复制所有权
    @param [in] other 原对象
    */
    NetNativeFileIo(const NetNativeFileIo& other);

    /** 禁止赋值
    @param [in] other 原对象
    @return 不可调用
    */
    NetNativeFileIo& operator=(const NetNativeFileIo& other);

  private:
    // 创建后端前选定的不可变配置，存在协调状态时禁止替换
    static std::unique_ptr<NetWorkFilePools> m_pools;
    // 本后端与唯一文件任务共同保留的协调状态，uninit等待任务后释放
    std::shared_ptr<NetFileIoState> m_state;
    // 本实例独占的CTask线程编号；0表示未创建或已回收
    uint32_t m_thread;
};