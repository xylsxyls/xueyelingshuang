#pragma once
#include "NetPeerMacro.h"
#include "NetFileIoTypes.h"
#include "NetInterface/NetInterfaceAPI.h"

// 可注入的独占异步文件后端；submit复制输入，peek借用结果，取消不回滚磁盘副作用
class NetPeerAPI NetFileIoBackend
{
  public:
    /** 构造不启动线程
    */
    NetFileIoBackend();

    /** 兜底停止及回收，仅生命周期负责人调用，不在自身/网络回调中销毁
    */
    virtual ~NetFileIoBackend();

    /** 初始化宿主独占异步工作，不能在网络poll中等待磁盘
    @return NI_OK或负数，可在uninit后重新初始化
    */
    virtual int32_t init() = 0;

    /** 在多个流之间分配本工作队列唯一编号，不回退复用
    @param [out] id 成功非零，失败0
    @return OK或负数；之后submit拒绝也消耗该编号
    */
    virtual int32_t nextRequestId(uint64_t& id) = 0;

    /** 复制任务至有界队列；生产者只有一个网络/业务执行者
    @param [in] request 参数仅调用时借用，READ/WRITE最大65536，路径最大4096
    @return OK已接纳，LIMIT_EXCEEDED未接纳；不是I/O已完成
    */
    virtual int32_t submit(const NetFileIoRequest& request) = 0;

    /** 请求取消排队/执行中工作，系统单次I/O协作结束后回报
    @param [in] id 当前任务号
    @return OK或NOT_FOUND，不能撤销已经发生的磁盘副作用
    */
    virtual int32_t cancel(uint64_t id) = 0;

    /** 借用一个完成结果，不等待工作线程
    @param [out] result 成功结果，否则清零
    @return READY结果，NEED_MORE暂无
    */
    virtual int32_t peek(NetFileIoResult& result) = 0;

    /** 仅在结果已消费后删除任务及擦除缓存
    @param [in] id 当前peek的编号
    @return OK或INVALID_STATE
    */
    virtual int32_t consume(uint64_t id) = 0;

    /** 非阻塞协作停止，拒绝新工作，排队项回报CANCELLED
    */
    virtual void stop() = 0;

    /** 工作执行者已释放全部句柄
    @return 是否已结束
    */
    virtual bool done() const = 0;

    /** 最终回收线程；调用前stop并在生命周期执行者等待done，禁止GUI/网络线程长等待
    */
    virtual void uninit() = 0;

};

// 工厂只创建未初始化后端；返回对象由NetFileIo独占并在创建模块释放
typedef NetFileIoBackend* (*NetFileIoBackendFactory)();