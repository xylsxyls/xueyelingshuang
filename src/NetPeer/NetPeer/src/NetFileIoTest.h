#pragma once
#include "NetFileIo.h"

// 无线程/磁盘的门面契约观察器，验证转交、异常和所有权
class NetFileIoTestBackend : public NetFileIoBackend
{
public:
    /** 初始化未停止后端，并记录本轮对象
    */
    NetFileIoTestBackend();

    /** 记录实际析构
    */
    virtual ~NetFileIoTestBackend();

    /** 不创建资源
    @return OK
    */
    virtual int32_t init();

    /** 分配本后端递增号，按测试模式抛异常
    @param [out] id 新编号
    @return OK
    */
    virtual int32_t nextRequestId(uint64_t& id);

    /** 记录转交，不模拟真实文件工作
    @param [in] request 未使用
    @return OK
    */
    virtual int32_t submit(const NetFileIoRequest& request);

    /** 记录取消入口
    @param [in] id 未使用
    @return NOT_FOUND
    */
    virtual int32_t cancel(uint64_t id);

    /** 无结果；异常模式先污染结果，以验证外层清零
    @param [out] result 无结果时清零
    @return NEED_MORE
    */
    virtual int32_t peek(NetFileIoResult& result);

    /** 无可消费结果
    @param [in] id 未使用
    @return INVALID_STATE
    */
    virtual int32_t consume(uint64_t id);

    /** 仅记录停止，不假设工作立即完成
    */
    virtual void stop();

    /** 返回由用例设置的完成标志
    @return 是否完成
    */
    virtual bool done() const;

    /** 后端已完成，不再有资源
    */
    virtual void uninit();

    /** 返回指定模式的后端，测试工厂异常/空返回
    @return 模式0新后端、1空、2抛异常
    */
    static NetFileIoBackend* create();

public:
    // 工厂模式，仅测试单执行者使用
    static uint32_t m_mode;
    // 最新创建实例，完成/销毁前有效
    static NetFileIoTestBackend* m_current;
    // 回入停止/释放测试的门面，不由后端拥有。
    static NetFileIo* m_owner;
    // 实际析构计数
    static uint32_t m_destroyed;
    // 后端本地递增号
    uint64_t m_next;
    // 被转交的请求数
    uint32_t m_submitted;
    // 控制next/peek异常
    bool m_throw;
    // stop是否到达
    bool m_stopped;
    // 工作是否完成，stop不替代此状态
    bool m_done;
};